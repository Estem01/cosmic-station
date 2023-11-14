#pragma once
#include <array>

#include <common/types.h>
#include <mio/blocks.h>
namespace zenith::mio {
    enum DirectChannels {
        Vif0, Vif1,
        Gif,
        IpuFrom, IpuTo,
        Sif0, Sif1, Sif2,
        SprFrom,
        SprTo
    };

    struct DmaChannel {
        bool request{false};
        u8 index;
    };

    class DMAController {
    public:
        DMAController();

        void resetMA();
        void pulse(u32 cycles);
        u32 performRead(u32 address);

        std::shared_ptr<GlobalMemory> memoryChips;
    private:
        std::array<DmaChannel, 0x9> channels;
    };
}