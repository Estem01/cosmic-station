#include <common/global.h>

#include <eeiv/ee_engine.h>
#include <eeiv/c0/cop0.h>

#include <fuji/mipsiv_interpreter.h>
#include <tokyo3/tokyo3_arm64_jitter.h>
namespace cosmic::eeiv {
    EEMipsCore::EEMipsCore(std::shared_ptr<mio::DMAController>& dma)
        : cop0(dma),
          memory(dma->memoryChips),
          eeTLB(std::make_shared<mio::TLBCache>(dma->memoryChips)) {
        GPRs = new eeRegister[countOfGPRs];

        device->getStates()->eeModeWay.observer = [this]() {
            procCpuMode = static_cast<EEExecutionMode>(*device->getStates()->eeModeWay);
            if (eeExecutor)
                eeExecutor.reset();

            if (procCpuMode == EEExecutionMode::CachedInterpreter)
                eeExecutor = std::make_unique<fuji::MipsIVInterpreter>(*this);
            else if (procCpuMode == EEExecutionMode::JitRe)
                eeExecutor = std::make_unique<tokyo3::EEArm64Jitter>(*this);
        };
    }

    EEMipsCore::~EEMipsCore() {
        delete[] GPRs;
    }

    void EEMipsCore::resetCore() {
        // The BIOS should be around here somewhere
        eePC = 0xbfc00000;
        tlbMap = cop0.mapVirtualTLB(eeTLB);

        // Cleaning up all registers, including the $zero register
        auto gprs{reinterpret_cast<u64*>(GPRs)};
        for (u8 regRange{}; regRange != countOfGPRs; regRange += 8) {
            static u256 zero{};
            // Writing 256 bits (32 bytes) per write call

            vst1_u64_x4(gprs + regRange, zero);
            vst1_u64_x4(gprs + regRange + 2, zero);
            vst1_u64_x4(gprs + regRange + 4, zero);
            vst1_u64_x4(gprs + regRange + 6, zero);
        }
        cyclesToWaste = cycles = 0;
    }
    void EEMipsCore::pulse(u32 cycles) {
        if (!irqTrigger) {
            cyclesToWaste += cycles;
            eeExecutor->executeCode();
        } else {
            cyclesToWaste = 0;
            this->cycles += cycles;
        }
        cop0.rectifyTimer(cycles);
        if (cop0.isIntEnabled()) {
            if (cop0.cause.timerIP)
                ;
        }
    }
    u32 EEMipsCore::fetchByPC() {
        const u32 orderPC{*lastPC};
        [[unlikely]] if (!eeTLB->isCached(*eePC)) {
            // However, the EE loads two instructions at once
            u32 punishment{8};
            if ((orderPC + 4) != *eePC) {
                // When reading an instruction out of sequential order, a penalty of 32 cycles is applied
                punishment = 32;
            }
            // Loading just one instruction, so, we will divide this penalty by 2
            cyclesToWaste -= punishment / 2;
            chPC(*eePC + 4);
            return tableRead<u32>(*lastPC);
        }
        if (!cop0.isCacheHit(*eePC, 0) && !cop0.isCacheHit(*eePC, 1)) {
            cop0.loadCacheLine(*eePC, *this);
        }
        chPC(*eePC + 4);
        return cop0.readCache(*lastPC);
    }
}