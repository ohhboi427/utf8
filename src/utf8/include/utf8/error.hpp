#pragma once

#include <expected>

namespace utf8 {
    enum class Utf8Error {
        InvalidByteSequence,
        InvalidCodepoint,
        OverlongEncoding,
    };

    template<typename T>
    using Utf8Expected = std::expected<T, Utf8Error>;
}
