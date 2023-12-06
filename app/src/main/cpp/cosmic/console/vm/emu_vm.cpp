// SPDX-short-identifier: MIT, Version N/A
// This file is protected by the MIT license (please refer to LICENSE.md before making any changes, copying, or redistributing this software)
#include <common/global.h>
#include <console/vm/emu_vm.h>
#include <console/backdoor.h>

#include <eeiv/ee_info.h>
#include <vu/v01_cop2vu.h>
#define TEST_BIOS_ACCESS 0
namespace cosmic::console::vm {
    EmuVM::EmuVM(JNIEnv* env,
        std::shared_ptr<VirtDevices>& devices,
        std::shared_ptr<gpu::ExhibitionEngine>& dsp)
            : memCtrl(devices->controller),
              mips(devices->mipsEeR5900),
              iop(devices->mipsIop),
              screenEngine(dsp),
              emuThread(*this) {

        biosHLE = std::make_shared<hle::BiosPatcher>(env, mips);
        scheduler = std::make_shared<Scheduler>();
        frames = 30;

        intc = std::make_shared<INTCInfra>(*this);
        // Our way to perform interconnection between different isolated components
        redBox = std::make_shared<RedPillow>(*this);

        vu01 = devices->VUs;
        vu01->populate(intc, memCtrl);

        raw_reference<vu::VectorUnit> vus[]{
            vu01->vpu0Cop2,
            vu01->vpu1DLO
        };
        mips->cop2 = std::make_unique<vu::MacroModeCop2>(vus);
        mips->timer.wakeUp = scheduler;
    }

    void EmuVM::startVM() {
        userLog->info("Starting VM from an improper context; this should be fixed later");
        auto emuMem{memCtrl->memoryChips};
        std::span<u8> eeKernelRegion{emuMem->makeRealAddress(0, mio::BiosMemory), emuMem->biosSize()};
        try {
            biosHLE->group->readBios(eeKernelRegion);
            biosHLE->resetBIOS();
#if TEST_BIOS_ACCESS
            u32* eeFirst{bit_cast<u32*>(emuMem->makeRealAddress(0x1fc00000, true))};
            *eeFirst = 0xcafebabe;
#endif
            emuThread.runVM();
        } catch (const NonAbort& except) {
            return;
        }
    }
    void EmuVM::stopVM() {
        emuThread.haltVM();
    }

    void EmuVM::resetVM() {
        scheduler->resetCycles();

        // Resetting all co-processors
        mips->ctrl0.resetCoP();
        mips->fpu1.resetFlu();
        mips->resetCore();

        memCtrl->resetMA();
        mips->timer.resetTimers();

        for (u8 vu{}; vu < 2; vu++)
            vu01->vifs[vu].resetVif();
        vu01->vpu0Cop2.resetVU();
        vu01->vpu1DLO.resetVU();

        iop->resetIOP();
        iop->cop.resetIOCop();
    }
    void EmuVM::dealWithSyscalls() {
        hle::SyscallOrigin ori{};
        // 08: Syscall Generated unconditionally by syscall instruction
        if (mips->ctrl0.cause.exCode == 0x8)
            ori = hle::SysEmotionEngine;
        else if (iop->cop.cause.code == 0x8)
            ori = hle::SysIop;
        if (ori == hle::SysEmotionEngine) {
            i16 eeSystem{*mips->gprAt<i16>(eeiv::$v1)};
            dealer.doSyscall(ori, eeSystem);
            mips->ctrl0.cause.exCode = 0;
        } else {
            iop->handleException(0x8);
        }
    }
}
