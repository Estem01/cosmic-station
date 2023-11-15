// SPDX-short-identifier: MIT, Version N/A
// This file is protected by the MIT license (please refer to LICENSE.md before making any changes, copying, or redistributing this software)
#include <console/emu_vm.h>
#include <common/global.h>

#define TestBiosAccess 0
namespace zenith::console {
    EmuVM::EmuVM(JNIEnv* env,
        std::shared_ptr<VirtDevices>& devices,
        std::shared_ptr<gpu::ExhibitionEngine>& dsp)
            : memCtrl(devices->controller),
              mips(devices->mipsEER5900),
              iop(devices->mipsIOP),
              screenEngine(dsp),
              emuThread(*this) {

        biosHLE = std::make_shared<hle::BiosPatcher>(env, mips);
        scheduler = std::make_shared<Scheduler>();
        render = std::make_unique<gpu::RenderScene>();

        mips->timer.wakeUp = scheduler;
        frames = 30;

        intc = std::make_shared<INTCInfra>(*this);
    }

    void EmuVM::startVM() {
        userLog->info("Starting VM from an improper context; this should be fixed later");
        render->pickUserRender();

        auto emuMem{memCtrl->memoryChips};
        std::span<u8> eeKernelRegion{emuMem->makeRealAddress(0, mio::BiosMemory), emuMem->biosSize()};
        try {
            biosHLE->group->readBios(eeKernelRegion);
            biosHLE->resetBIOS();
#if TestBiosAccess
            u32* eeFirst{bit_cast<u32*>(emuMem->makeRealAddress(0x1fc00000, true))};
            *eeFirst = 0xcafebabe;
#endif
            emuThread.runVM();
        } catch (const NonAbort& except) {
            return;
        }
    }

    void EmuVM::resetVM() {
        scheduler->resetCycles();

        // Resetting all co-processors
        memCtrl->resetMA();
        mips->cop0.resetCoP();
        mips->fuCop1.resetFlu();
        mips->timer.resetTimers();

        mips->resetCore();
        iop->resetIOP();
    }
}
