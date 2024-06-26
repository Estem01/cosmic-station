#pragma once
#include <array>
#include <list>

#include <common/types.h>
#include <os/neon_simd.h>
#include <mio/blocks.h>
namespace cosmic {
    namespace vu {
        class VifMalice;
    }
    namespace engine {
        class EeMipsCore;
    }
}

namespace cosmic::mio {
    class MemoryPipe;

    struct HardWithDmaCap {
    public:
        HardWithDmaCap() {}
        Ref<vu::VifMalice> vif0, vif1;
        std::shared_ptr<engine::EeMipsCore> ee;
    };

    enum DirectChannels {
        Vif0, Vif1,
        Gif,
        IpuFrom, IpuTo,
        Sif0, Sif1, Sif2,
        SprFrom,
        SprTo
    };
    using DmaChannelId = u8;
    struct DmaChannel {
        bool request{false};

        alignas(4) u16 qwc;
        // This will result in having both Dn_MADR and Dn_TADR,
        // including Dn_ASRX and Dn_SADR in a single location
        struct {
            u32 adr;
            u32 tagAdr;
            // SPR_FROM, SPR_TO
            bool isScratch;
            bool isChan;
            u8 tagType;
        };

        DmaChannelId index;
    };
    union DmaRegister {
        u32 address;
        u32 value;
        bool writeBack{false};

        auto operator=(u32 vReg) {
            value = vReg;
            writeBack = true;
        }
        auto operator*() {
            return value;
        }
    };
    struct DmaStatus {
        bool isDmaEnabled;
        bool isCycleStealing;

        u8 mFiFoChannel;
        u8 stallChannel;
        u8 drainChannel;
        // When isCycleStealing is true, releaseCyclePeriod will determine how many machine
        // cycles the EE can freely use the BUS without congestion
        u8 releaseCyclePeriod;
    };
    class OwnerChannel {
    public:
        OwnerChannel() : id(), locked() {
        }
        operator bool() const {
            return locked;
        }
        void select(u8 cid) {
            if (!locked)
                return;
            id = cid;
            locked = true;
        }
        u8 unselect() {
            bool isl{locked};
            locked = false;
            if (isl)
                return id;
            return 0;
        }
        DmaChannelId id;
        bool locked;
    };

    class DmaController {
    public:
        DmaController();

        void resetMa();
        void pulse(u32 cycles);
        os::vec performRead(u32 address);
        Ref<u32> dmaVirtSolver(u32 address);

        Ref<u128> dmaAddrSolver(u32 address, bool isScr, bool isVu);
        os::vec dmacRead(u32& address);
        void dmacWrite(u32 address, const os::vec& val);

        void issueADmacRequest(DirectChannels channel);
        void connectDevices(HardWithDmaCap& devices);

        std::shared_ptr<GlobalMemory> mapped;
        DmaStatus status;

        // Changes the internal behavior of the DMAC, causing interruptions
        union InterruptReg {
            struct {
                u8 control: 6;
                u8 unused: 7;
                bool busError: 1;
                u8 chIntMask: 6;
                u8 chInt: 6;
                u8 iopMasterInt: 1;
            };
            u32 dicr;
        } ir; // DICR
        // Clean the DMA request from the channel (Doesn't clear the channel itself, only the request flag)
        void disableChannel(DirectChannels channel, bool disableRequest = false);
        void advanceSrcDma(Ref<DmaChannel>& chan);
        void advanceSrcDma(DirectChannels id) {
            if (static_cast<u32>(id) > channels.size()) {
            }
            Ref<DmaChannel> chan{std::ref(channels[id])};
            advanceSrcDma(chan);
        }
    private:
        std::list<DmaChannel> queued;
        using ChannelIterator = std::list<DmaChannel>::iterator;

        u32 intStatus;
        OwnerChannel hasOwner;
        i64 highCycles;

        std::pair<u32, u8> feedVif0Pipe(Ref<DmaChannel> vifc);
        std::pair<bool, u32> pipeQuad2Transfer(Ref<DmaChannel> ch);

        void switchChannel();
        void findNextChannel();
        union {
            std::array<DmaChannel, 0x9> channels;
        };
        std::function<ChannelIterator(DmaChannelId)> getStagedChannel;

        DmaRegister priorityCtrl{0x1f8010f0}; // PCR
        u32 stallAddress; // STADR
        std::shared_ptr<MemoryPipe> pipe;

        struct {
            Ref<vu::VifMalice> vif1, vif0;
            std::shared_ptr<engine::EeMipsCore> core;
        } hw;
    };
}
