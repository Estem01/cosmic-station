#include <pshook/hk_psx.h>

namespace cosmic::pshook{
    NO_MANGLE {
        struct PsxRegs r;
        void psxAbs() {
            r.v0->s = abs(r.a0->s);
            *r.pc = *r.ra;
        }
    }
}
