#pragma once

#include <expected>

namespace utf8 {
    enum class Error {
        InvalidByteSequence,
        InvalidCodepoint,
        OverlongEncoding,
    };

    template<typename T>
    using Expected   = std::expected<T, Error>;
    using Unexpected = std::unexpected<Error>;
}
