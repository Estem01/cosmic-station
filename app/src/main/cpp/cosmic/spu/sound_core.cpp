#include <spu/sound_core.h>
namespace cosmic::spu {
    void Spu2::resetSound() {
        status = {};
        transferAddr = 0;
        currentAddr = 0;
        control = 0; // Added: Reset control register

    }
    void Spu2::writeDmaData(u32 data) {
        if (!status.dmaReady) {
        }
        spuWrite(data & 0xffff);

        status.dmaBusy = true;
        status.dmaReady = false;
    }
    u32 Spu2::requestDmaData() {
        return 0;
    }

    u16 Spu2::spuRead(u32 address) {
        return 0;
    }
    void Spu2::spuWrite(u32 address, u16 value) {
        *PipeCraftPtr<u16*>(spuRam, address, mio::Spu2Dev) = value;
    }
    void Spu2::spuWrite(u16 value) {
        *PipeCraftPtr<u16*>(spuRam, currentAddr, mio::Spu2Dev) = value;
    }
}
