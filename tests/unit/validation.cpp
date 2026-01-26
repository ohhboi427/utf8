#include <gtest/gtest.h>

#include <utf8/error.hpp>
#include <utf8/validation.hpp>

#include <algorithm>
#include <cstdint>
#include <ranges>

TEST(Utf8LeadingTests, decoded_length_success) {
    static constexpr auto test_case = [](const char8_t input, const std::uint8_t expected_length) noexcept -> void {
        const auto length = utf8::detail::decoded_length(input);

        ASSERT_TRUE(length.has_value());
        EXPECT_EQ(*length, expected_length);
    };

    test_case(0b01111111, 1U);
    test_case(0b11011111, 2U);
    test_case(0b11101111, 3U);
    test_case(0b11110111, 4U);
}

TEST(Utf8LeadingTests, decoded_length_error) {
    static constexpr auto test_case = [](const char8_t input) noexcept -> void {
        const auto unit = utf8::detail::decoded_length(input);

        ASSERT_FALSE(unit.has_value());
        EXPECT_EQ(unit.error(), utf8::Error::InvalidByteSequence);
    };

    test_case(0b10000000U);
    test_case(0b11111111U);
}

TEST(Utf8LeadingTests, make_leading) {
    EXPECT_EQ(utf8::detail::make_leading(0b00000001U, 1U), 0b00000001U);
    EXPECT_EQ(utf8::detail::make_leading(0b11111111U, 1U), 0b01111111U);
    EXPECT_EQ(utf8::detail::make_leading(0b00000001U, 2U), 0b11000001U);
    EXPECT_EQ(utf8::detail::make_leading(0b11111111U, 2U), 0b11011111U);
    EXPECT_EQ(utf8::detail::make_leading(0b00000001U, 3U), 0b11100001U);
    EXPECT_EQ(utf8::detail::make_leading(0b11111111U, 3U), 0b11101111U);
    EXPECT_EQ(utf8::detail::make_leading(0b00000001U, 4U), 0b11110001U);
    EXPECT_EQ(utf8::detail::make_leading(0b11111111U, 4U), 0b11110111U);
}

TEST(Utf8LeadingTests, read_leading_success) {
    static constexpr auto test_case = [](
        const char8_t      input,
        const char8_t      expected_unit,
        const std::uint8_t expected_length
    ) noexcept -> void {
        const auto result = utf8::detail::read_leading(input);

        ASSERT_TRUE(result.has_value());
        const auto [unit, length] = *result;

        EXPECT_EQ(unit, expected_unit);
        EXPECT_EQ(length, expected_length);
    };

    test_case(0b01111111U, 0b01111111U, 1U);
    test_case(0b11011111U, 0b00011111U, 2U);
    test_case(0b11101111U, 0b00001111U, 3U);
    test_case(0b11110111U, 0b00000111U, 4U);
}

TEST(Utf8LeadingTests, read_leading_error) {
    static constexpr auto test_case = [](const char8_t input) noexcept -> void {
        const auto unit = utf8::detail::read_leading(input);

        ASSERT_FALSE(unit.has_value());
        EXPECT_EQ(unit.error(), utf8::Error::InvalidByteSequence);
    };

    test_case(0b10000000U);
    test_case(0b11111000U);
}

TEST(Utf8ContinationTests, make_continuation) {
    EXPECT_EQ(utf8::detail::make_continuation(0b00000000U), 0b10000000U);
    EXPECT_EQ(utf8::detail::make_continuation(0b11111111U), 0b10111111U);
}

TEST(Utf8ContinationTests, read_continuation_success) {
    static constexpr auto test_case = [](
        const char8_t input,
        const char8_t expected_unit
    ) noexcept -> void {
        const auto unit = utf8::detail::read_continuation(input);

        ASSERT_TRUE(unit.has_value());
        EXPECT_EQ(*unit, expected_unit);
    };

    test_case(0b10000000U, 0b00000000U);
    test_case(0b10111111U, 0b00111111U);
}

TEST(Utf8ContinationTests, read_continuation_error) {
    static constexpr auto test_case = [](const char8_t input) noexcept -> void {
        const auto unit = utf8::detail::read_continuation(input);

        ASSERT_FALSE(unit.has_value());
        EXPECT_EQ(unit.error(), utf8::Error::InvalidByteSequence);
    };

    test_case(0b00000000U);
    test_case(0b11111111U);
}

TEST(Utf8CodepointTests, encoded_length_success) {
    static constexpr auto test_case = [](const char32_t input, const std::uint8_t expected_length) noexcept -> void {
        const auto length = utf8::detail::encoded_length(input);

        ASSERT_TRUE(length.has_value());
        EXPECT_EQ(*length, expected_length);
    };

    test_case(0x00007FU, 1U);
    test_case(0x0007FFU, 2U);
    test_case(0x00FFFFU, 3U);
    test_case(0x10FFFFU, 4U);
}

TEST(Utf8CodepointTests, overlong) {
    EXPECT_TRUE(utf8::detail::is_overlong(0x007FU, 2U));
    EXPECT_TRUE(utf8::detail::is_overlong(0x07FFU, 3U));
    EXPECT_TRUE(utf8::detail::is_overlong(0xFFFFU, 4U));

    EXPECT_FALSE(utf8::detail::is_overlong(0x007FU, 1U));
    EXPECT_FALSE(utf8::detail::is_overlong(0x07FFU, 2U));
    EXPECT_FALSE(utf8::detail::is_overlong(0xFFFFU, 3U));
    EXPECT_FALSE(utf8::detail::is_overlong(0x10FFFFU, 4U));
}

TEST(Utf8CodepointTests, out_of_range) {
    EXPECT_TRUE(utf8::is_invalid(0xFFFFFFFFU));

    EXPECT_FALSE(utf8::is_invalid(0x10FFFFU));
}

TEST(Utf8CodepointTests, surrogate) {
    EXPECT_TRUE(utf8::is_invalid(0xD800U));
    EXPECT_TRUE(utf8::is_invalid(0xDFFFU));

    EXPECT_FALSE(utf8::is_invalid(0xD7FFU));
    EXPECT_FALSE(utf8::is_invalid(0xE000U));
}

TEST(Utf8EncodingTests, decode_success) {
    static constexpr auto test_case = [](
        std::ranges::input_range auto&& unit,
        const char32_t                  expected_codepoint,
        const std::ptrdiff_t            expected_length
    ) noexcept -> void {
        const auto begin = std::ranges::begin(unit);
        const auto end   = std::ranges::end(unit);

        std::array<char8_t, 4U> buffer{};

        const auto [it, out, codepoint] = utf8::decode_into(begin, end, buffer.begin());
        EXPECT_EQ(std::distance(begin, it), expected_length);

        const auto [it1, it2] = std::mismatch(buffer.begin(), out, begin);
        EXPECT_EQ(it1, out);

        ASSERT_TRUE(codepoint.has_value());
        EXPECT_EQ(*codepoint, expected_codepoint);
    };

    test_case(std::initializer_list<char8_t>{ 0x7FU }, 0x007FU, 1U);
    test_case(std::initializer_list<char8_t>{ 0xDFU, 0xBFU }, 0x07FFU, 2U);
    test_case(std::initializer_list<char8_t>{ 0xEFU, 0xBFU, 0xBFU }, 0xFFFFU, 3U);
    test_case(std::initializer_list<char8_t>{ 0xF4U, 0x8FU, 0xBFU, 0xBFU }, 0x10FFFFU, 4U);
}

TEST(Utf8EncodingTests, decode_error) {
    static constexpr auto test_case = [](
        std::ranges::input_range auto&& input,
        const utf8::Error               expected_error,
        const std::ptrdiff_t            expected_length
    ) noexcept -> void {
        const auto begin = std::ranges::begin(input);
        const auto end   = std::ranges::end(input);

        const auto [it, codepoint] = utf8::decode(begin, end);
        EXPECT_EQ(std::distance(begin, it), expected_length);

        ASSERT_FALSE(codepoint.has_value());
        EXPECT_EQ(codepoint.error(), expected_error);
    };

    // Invalid leading
    test_case(std::initializer_list<char8_t>{ 0xFFU }, utf8::Error::InvalidByteSequence, 1U);
    test_case(std::initializer_list<char8_t>{ 0xF8U }, utf8::Error::InvalidByteSequence, 1U);
    test_case(std::initializer_list<char8_t>{ 0x80U }, utf8::Error::InvalidByteSequence, 1U);

    // Too short
    test_case(std::initializer_list<char8_t>{ 0xC2U }, utf8::Error::InvalidByteSequence, 1U);
    test_case(std::initializer_list<char8_t>{ 0xE2U, 0x80U }, utf8::Error::InvalidByteSequence, 2U);
    test_case(std::initializer_list<char8_t>{ 0xF0U, 0x80U, 0x80U }, utf8::Error::InvalidByteSequence, 3U);

    // Invalid continuation
    test_case(std::initializer_list<char8_t>{ 0xC2U, 0x00U }, utf8::Error::InvalidByteSequence, 1U);
    test_case(std::initializer_list<char8_t>{ 0xE2U, 0x00U }, utf8::Error::InvalidByteSequence, 1U);
    test_case(std::initializer_list<char8_t>{ 0xF0U, 0x00U }, utf8::Error::InvalidByteSequence, 1U);

    // Overlong
    test_case(std::initializer_list<char8_t>{ 0xC0U, 0xAFU }, utf8::Error::OverlongEncoding, 2U);
    test_case(std::initializer_list<char8_t>{ 0xF0U, 0x82U, 0x82U, 0xACU }, utf8::Error::OverlongEncoding, 4U);

    // Invalid codepoints
    test_case(std::initializer_list<char8_t>{ 0xEDU, 0xA0U, 0x80U }, utf8::Error::InvalidCodepoint, 3U);
    test_case(std::initializer_list<char8_t>{ 0xF4U, 0x90U, 0x80U, 0x80U }, utf8::Error::InvalidCodepoint, 4U);
}

TEST(Utf8EncodingTests, encode_success) {
    static constexpr auto test_case = [](
        const char32_t                  input,
        std::ranges::input_range auto&& expected_units
    ) noexcept -> void {
        const auto units = utf8::encode(input);

        ASSERT_TRUE(units.has_value());
        auto       it  = units->begin();
        const auto end = units->end();

        for(const auto expected_unit : expected_units) {
            ASSERT_NE(it, end);
            EXPECT_EQ(*it, expected_unit);

            ++it;
        }

        for(; it < end; ++it) {
            EXPECT_EQ(*it, 0U);
        }
    };

    test_case(0x007FU, std::initializer_list<char8_t>{ 0x7FU });
    test_case(0x07FFU, std::initializer_list<char8_t>{ 0xDFU, 0xBFU });
    test_case(0xFFFFU, std::initializer_list<char8_t>{ 0xEFU, 0xBFU, 0xBFU });
    test_case(0x10FFFFU, std::initializer_list<char8_t>{ 0xF4U, 0x8FU, 0xBFU, 0xBFU });
}

TEST(Utf8EncodingTests, encode_error) {
    static constexpr auto test_case = [](const char32_t input) noexcept -> void {
        const auto units = utf8::encode(input);

        ASSERT_FALSE(units.has_value());
        EXPECT_EQ(units.error(), utf8::Error::InvalidCodepoint);
    };

    test_case(0xD800U);
    test_case(0xDFFFU);
    test_case(0xFFFFFFFFU);
}
