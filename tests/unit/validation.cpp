#include <gtest/gtest.h>

#include <utf8/error.hpp>
#include <utf8/validation.hpp>

#include <cstdint>
#include <ranges>

using namespace utf8;

TEST(Utf8LeadingTests, leader_bits) {
    EXPECT_EQ(detail::leading_header(1U), 0b00000000U);
    EXPECT_EQ(detail::leading_header(2U), 0b11000000U);
    EXPECT_EQ(detail::leading_header(3U), 0b11100000U);
    EXPECT_EQ(detail::leading_header(4U), 0b11110000U);

    EXPECT_EQ(detail::leading_mask(1U), 0b01111111U);
    EXPECT_EQ(detail::leading_mask(2U), 0b00011111U);
    EXPECT_EQ(detail::leading_mask(3U), 0b00001111U);
    EXPECT_EQ(detail::leading_mask(4U), 0b00000111U);
}

TEST(Utf8LeadingTests, decoded_length_success) {
    static constexpr auto test_case = [](const char8_t unit, const std::uint8_t length) noexcept -> void {
        const auto result = decoded_length(unit);

        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, length);
    };

    test_case(0b01111111, 1U);
    test_case(0b11011111, 2U);
    test_case(0b11101111, 3U);
    test_case(0b11110111, 4U);
}

TEST(Utf8LeadingTests, decoded_length_error) {
    static constexpr auto test_case = [](const char8_t unit) noexcept -> void {
        const auto result = decoded_length(unit);

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), Error::InvalidByteSequence);
    };

    test_case(0b10000000U);
    test_case(0b11111111U);
}

TEST(Utf8LeadingTests, make_leading) {
    EXPECT_EQ(make_leading(0b00000001U, 1U), 0b00000001U);
    EXPECT_EQ(make_leading(0b11111111U, 1U), 0b01111111U);
    EXPECT_EQ(make_leading(0b00000001U, 2U), 0b11000001U);
    EXPECT_EQ(make_leading(0b11111111U, 2U), 0b11011111U);
    EXPECT_EQ(make_leading(0b00000001U, 3U), 0b11100001U);
    EXPECT_EQ(make_leading(0b11111111U, 3U), 0b11101111U);
    EXPECT_EQ(make_leading(0b00000001U, 4U), 0b11110001U);
    EXPECT_EQ(make_leading(0b11111111U, 4U), 0b11110111U);
}

TEST(Utf8LeadingTests, read_leading_success) {
    static constexpr auto test_case = [](
        const char8_t      input_unit,
        const char8_t      expected_unit,
        const std::uint8_t expected_length
    ) noexcept -> void {
        const auto result = read_leading(input_unit);
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
    static constexpr auto test_case = [](const char8_t unit) noexcept -> void {
        const auto result = read_leading(unit);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), Error::InvalidByteSequence);
    };

    test_case(0b10000000U);
    test_case(0b11111000U);
}

TEST(Utf8ContinationTests, is_continuation) {
    EXPECT_TRUE(is_continuation(0b10000000U));

    EXPECT_FALSE(is_continuation(0b00000000U));
    EXPECT_FALSE(is_continuation(0b01000000U));
    EXPECT_FALSE(is_continuation(0b11000000U));
}

TEST(Utf8ContinationTests, make_continuation) {
    EXPECT_EQ(make_continuation(0b00000000U), 0b10000000U);
    EXPECT_EQ(make_continuation(0b11111111U), 0b10111111U);
}

TEST(Utf8ContinationTests, read_continuation_success) {
    static constexpr auto test_case = [](
        const char8_t input_unit,
        const char8_t expected_unit
    ) noexcept -> void {
        const auto result = read_continuation(input_unit);
        ASSERT_TRUE(result.has_value());

        EXPECT_EQ(*result, expected_unit);
    };

    test_case(0b10000000U, 0b00000000U);
    test_case(0b10111111U, 0b00111111U);
}

TEST(Utf8ContinationTests, read_continuation_error) {
    static constexpr auto test_case = [](const char8_t unit) noexcept -> void {
        const auto result = read_continuation(unit);
        ASSERT_FALSE(result.has_value());

        EXPECT_EQ(result.error(), Error::InvalidByteSequence);
    };

    test_case(0b00000000U);
    test_case(0b11111111U);
}

TEST(Utf8CodepointTests, encoded_length_success) {
    static constexpr auto test_case = [](const char32_t codepoint, const std::uint8_t length) noexcept -> void {
        const auto result = encoded_length(codepoint);
        ASSERT_TRUE(result.has_value());

        EXPECT_EQ(*result, length);
    };

    test_case(0x00007FU, 1U);
    test_case(0x0007FFU, 2U);
    test_case(0x00FFFFU, 3U);
    test_case(0x10FFFFU, 4U);
}

TEST(Utf8CodepointTests, overlong) {
    ASSERT_FALSE(is_overlong(0x0000U, 1U));
    ASSERT_FALSE(is_overlong(0x007FU, 1U));

    ASSERT_TRUE(is_overlong(0x007FU, 2U));
    ASSERT_TRUE(is_overlong(0x07FFU, 3U));
    ASSERT_TRUE(is_overlong(0xFFFFU, 4U));

    ASSERT_FALSE(is_overlong(0x007FU, 1U));
    ASSERT_FALSE(is_overlong(0x07FFU, 2U));
    ASSERT_FALSE(is_overlong(0xFFFFU, 3U));
    ASSERT_FALSE(is_overlong(0x10FFFFU, 4U));
}

TEST(Utf8CodepointTests, out_of_range) {
    ASSERT_TRUE(is_invalid(0xFFFFFFFFU));
}

TEST(Utf8CodepointTests, surrogate) {
    ASSERT_TRUE(is_invalid(0xD800U));
    ASSERT_TRUE(is_invalid(0xDFFFU));
}
