#pragma once
#include <mio/mem_pipe.h>
#include <common/types.h>
namespace cosmic {
    namespace console {
        class IntCInfra;
    }
    namespace iop {
        class IopDma;
    }
}

namespace cosmic::spu {
    struct SpuStatus {
        bool dmaReady;
        bool dmaBusy;
    };
    class Spu2 {
    public:
        Spu2(std::shared_ptr<console::IntCInfra>& infra,
             std::shared_ptr<iop::IopDma>& ioDma,
             std::shared_ptr<mio::MemoryPipe>& pipe) :
             intc(infra), dmac(ioDma), spuRam(pipe)
            {
        }
        SpuStatus status;
        u32 control{0}; // Added: SPU2 control register directly in class

        void resetSound();
        u32 transferAddr;
        u32 currentAddr;

        void writeDmaData(u32 data);
        u32 requestDmaData();
        u32 getControl() const { return control; } // Added: Getter for control
    private:
        u16 spuRead(u32 address);
        void spuWrite(u32 address, u16 value);
        void spuWrite(u16 value);

        std::shared_ptr<console::IntCInfra> intc;
        std::shared_ptr<iop::IopDma> dmac;
        std::shared_ptr<mio::MemoryPipe> spuRam;
    };
}
