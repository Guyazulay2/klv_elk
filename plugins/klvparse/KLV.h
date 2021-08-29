#pragma once

#include <gst/gst.h>

namespace KLV {

enum class KEY_FORMATS {
    _ = -99,
    _BER_OID = -1,
    _1 = 1,
    _2 = 2,
    _4 = 4,
    _16 = 16,
};

enum class LENGTH_FORMATS {
    _ = -99,
    _BER = -1,
    _1 = 1,
    _2 = 2,
    _4 = 4,
    _16 = 16,
};

}  // namespace KLV
