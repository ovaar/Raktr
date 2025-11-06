/*!
 * @file test_aspect_ratio.cpp
 * @brief Tests for aspect ratio calculations and viewport utilities.
 */

#include <gtest/gtest.h>
#include "backend/aspect_ratio.h"

#include <cstdint>

using namespace raktr::render;

// Test fixture for aspect ratio tests
class AspectRatioTest : public ::testing::Test
{
protected:
    // Common window sizes for testing
    static constexpr uint32_t HD_WIDTH = 1920;
    static constexpr uint32_t HD_HEIGHT = 1080;
    static constexpr uint32_t WUXGA_WIDTH = 1920;
    static constexpr uint32_t WUXGA_HEIGHT = 1200;
    static constexpr uint32_t SQUARE_SIZE = 800;
};

// ============================================================================
// Test: Aspect Ratio Value Calculations
// ============================================================================

TEST_F(AspectRatioTest, GetAspectRatioValue_Auto_ReturnsZero)
{
    // Arrange & Act
    float ratio = get_aspect_ratio_value(AspectRatio::Auto);
    
    // Assert
    EXPECT_FLOAT_EQ(ratio, 0.0f);
}

TEST_F(AspectRatioTest, GetAspectRatioValue_16_9_ReturnsCorrectRatio)
{
    // Arrange & Act
    float ratio = get_aspect_ratio_value(AspectRatio::Ratio_16_9);
    
    // Assert
    EXPECT_FLOAT_EQ(ratio, 16.0f / 9.0f);
    EXPECT_NEAR(ratio, 1.778f, 0.001f);
}

TEST_F(AspectRatioTest, GetAspectRatioValue_4_3_ReturnsCorrectRatio)
{
    // Arrange & Act
    float ratio = get_aspect_ratio_value(AspectRatio::Ratio_4_3);
    
    // Assert
    EXPECT_FLOAT_EQ(ratio, 4.0f / 3.0f);
    EXPECT_NEAR(ratio, 1.333f, 0.001f);
}

TEST_F(AspectRatioTest, GetAspectRatioValue_21_9_ReturnsCorrectRatio)
{
    // Arrange & Act
    float ratio = get_aspect_ratio_value(AspectRatio::Ratio_21_9);
    
    // Assert
    EXPECT_FLOAT_EQ(ratio, 21.0f / 9.0f);
    EXPECT_NEAR(ratio, 2.333f, 0.001f);
}

TEST_F(AspectRatioTest, GetAspectRatioValue_16_10_ReturnsCorrectRatio)
{
    // Arrange & Act
    float ratio = get_aspect_ratio_value(AspectRatio::Ratio_16_10);
    
    // Assert
    EXPECT_FLOAT_EQ(ratio, 1.6f);
}

TEST_F(AspectRatioTest, GetAspectRatioValue_1_1_ReturnsOne)
{
    // Arrange & Act
    float ratio = get_aspect_ratio_value(AspectRatio::Ratio_1_1);
    
    // Assert
    EXPECT_FLOAT_EQ(ratio, 1.0f);
}

TEST_F(AspectRatioTest, GetAspectRatioValue_Custom_ReturnsCustomValue)
{
    // Arrange
    float custom_ratio = 2.35f; // Cinemascope
    
    // Act
    float ratio = get_aspect_ratio_value(AspectRatio::Custom, custom_ratio);
    
    // Assert
    EXPECT_FLOAT_EQ(ratio, 2.35f);
}

// ============================================================================
// Test: Viewport Calculation - Auto Mode
// ============================================================================

TEST_F(AspectRatioTest, CalculateViewport_Auto_UsesFullWindow)
{
    // Arrange & Act
    Viewport vp = calculate_viewport(HD_WIDTH, HD_HEIGHT, AspectRatio::Auto);
    
    // Assert
    EXPECT_EQ(vp.x, 0u);
    EXPECT_EQ(vp.y, 0u);
    EXPECT_EQ(vp.width, HD_WIDTH);
    EXPECT_EQ(vp.height, HD_HEIGHT);
}

// ============================================================================
// Test: Viewport Calculation - Letterboxing (Window too tall)
// ============================================================================

TEST_F(AspectRatioTest, CalculateViewport_16_9_Letterbox_CentersVertically)
{
    // Arrange: WUXGA (1920x1200) is taller than 16:9
    // Expected: 1920 x 1080 viewport, 60px bars top/bottom
    
    // Act
    Viewport vp = calculate_viewport(WUXGA_WIDTH, WUXGA_HEIGHT, AspectRatio::Ratio_16_9);
    
    // Assert
    EXPECT_EQ(vp.width, 1920u);
    EXPECT_EQ(vp.height, 1080u);
    EXPECT_EQ(vp.x, 0u);
    EXPECT_EQ(vp.y, 60u); // (1200 - 1080) / 2 = 60
}

TEST_F(AspectRatioTest, CalculateViewport_4_3_Letterbox_WithHDWindow)
{
    // Arrange: HD (1920x1080) with 4:3 aspect ratio
    // Expected: 1440 x 1080 viewport, pillarbox left/right
    
    // Act
    Viewport vp = calculate_viewport(HD_WIDTH, HD_HEIGHT, AspectRatio::Ratio_4_3);
    
    // Assert
    EXPECT_EQ(vp.height, 1080u);
    EXPECT_EQ(vp.width, 1440u); // 1080 * (4/3)
    EXPECT_EQ(vp.y, 0u);
    EXPECT_EQ(vp.x, 240u); // (1920 - 1440) / 2
}

// ============================================================================
// Test: Viewport Calculation - Pillarboxing (Window too wide)
// ============================================================================

TEST_F(AspectRatioTest, CalculateViewport_21_9_Pillarbox_WithHDWindow)
{
    // Arrange: HD (1920x1080) with 21:9 aspect ratio
    // Expected: 1920 width, but height reduced for 21:9, letterbox
    
    // Act
    Viewport vp = calculate_viewport(HD_WIDTH, HD_HEIGHT, AspectRatio::Ratio_21_9);
    
    // Assert
    EXPECT_EQ(vp.width, 1920u);
    // Height should be approximately 823 (1920 / (21/9) ≈ 822.857)
    EXPECT_TRUE(vp.height == 822u || vp.height == 823u); // Allow for rounding
    EXPECT_EQ(vp.x, 0u);
    EXPECT_GT(vp.y, 0u); // Should have top/bottom bars
}

// ============================================================================
// Test: Viewport Calculation - Square Aspect Ratio
// ============================================================================

TEST_F(AspectRatioTest, CalculateViewport_1_1_WithSquareWindow_UsesFullWindow)
{
    // Arrange & Act
    Viewport vp = calculate_viewport(SQUARE_SIZE, SQUARE_SIZE, AspectRatio::Ratio_1_1);
    
    // Assert
    EXPECT_EQ(vp.x, 0u);
    EXPECT_EQ(vp.y, 0u);
    EXPECT_EQ(vp.width, SQUARE_SIZE);
    EXPECT_EQ(vp.height, SQUARE_SIZE);
}

TEST_F(AspectRatioTest, CalculateViewport_1_1_WithWideWindow_Pillarboxes)
{
    // Arrange: Wide window (1600x900) with square aspect ratio
    
    // Act
    Viewport vp = calculate_viewport(1600, 900, AspectRatio::Ratio_1_1);
    
    // Assert
    EXPECT_EQ(vp.height, 900u);
    EXPECT_EQ(vp.width, 900u); // Square viewport
    EXPECT_EQ(vp.y, 0u);
    EXPECT_EQ(vp.x, 350u); // (1600 - 900) / 2
}

TEST_F(AspectRatioTest, CalculateViewport_1_1_WithTallWindow_Letterboxes)
{
    // Arrange: Tall window (900x1600) with square aspect ratio
    
    // Act
    Viewport vp = calculate_viewport(900, 1600, AspectRatio::Ratio_1_1);
    
    // Assert
    EXPECT_EQ(vp.width, 900u);
    EXPECT_EQ(vp.height, 900u); // Square viewport
    EXPECT_EQ(vp.x, 0u);
    EXPECT_EQ(vp.y, 350u); // (1600 - 900) / 2
}

// ============================================================================
// Test: Viewport Calculation - Custom Aspect Ratio
// ============================================================================

TEST_F(AspectRatioTest, CalculateViewport_Custom_UsesCustomRatio)
{
    // Arrange: 2.35:1 cinemascope ratio
    float cinemascope = 2.35f;
    
    // Act
    Viewport vp = calculate_viewport(1920, 1080, AspectRatio::Custom, cinemascope);
    
    // Assert
    EXPECT_EQ(vp.width, 1920u);
    EXPECT_EQ(vp.height, static_cast<uint32_t>(1920 / cinemascope));
    EXPECT_EQ(vp.x, 0u);
    EXPECT_GT(vp.y, 0u); // Should have letterbox bars
}

// ============================================================================
// Test: Edge Cases
// ============================================================================

TEST_F(AspectRatioTest, CalculateViewport_SmallWindow_StillCalculatesCorrectly)
{
    // Arrange & Act
    Viewport vp = calculate_viewport(320, 240, AspectRatio::Ratio_16_9);
    
    // Assert
    EXPECT_EQ(vp.width, 320u);
    EXPECT_LT(vp.height, 240u); // Should be letterboxed
    EXPECT_EQ(vp.x, 0u);
    EXPECT_GT(vp.y, 0u);
}

TEST_F(AspectRatioTest, CalculateViewport_ExactMatch_NoBlackBars)
{
    // Arrange: 1920x1080 is exactly 16:9
    
    // Act
    Viewport vp = calculate_viewport(HD_WIDTH, HD_HEIGHT, AspectRatio::Ratio_16_9);
    
    // Assert - viewport should match window exactly
    EXPECT_EQ(vp.x, 0u);
    EXPECT_EQ(vp.y, 0u);
    EXPECT_EQ(vp.width, HD_WIDTH);
    EXPECT_EQ(vp.height, HD_HEIGHT);
}

TEST_F(AspectRatioTest, CalculateViewport_16_10_WithWUXGA_NoBlackBars)
{
    // Arrange: 1920x1200 is exactly 16:10
    
    // Act
    Viewport vp = calculate_viewport(WUXGA_WIDTH, WUXGA_HEIGHT, AspectRatio::Ratio_16_10);
    
    // Assert - viewport should match window exactly
    EXPECT_EQ(vp.x, 0u);
    EXPECT_EQ(vp.y, 0u);
    EXPECT_EQ(vp.width, WUXGA_WIDTH);
    EXPECT_EQ(vp.height, WUXGA_HEIGHT);
}
