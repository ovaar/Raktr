/*!
 * @file aspect_ratio.h
 * @brief Aspect ratio utilities for maintaining proper rendering proportions.
 */

#ifndef RAKTR_RENDER_ASPECT_RATIO_H
#define RAKTR_RENDER_ASPECT_RATIO_H

#include <cstdint>
#include <cmath>

namespace raktr::render
{

/*!
 * @brief Common aspect ratios for rendering.
 * 
 * Used to prevent stretching by maintaining proper width:height proportions
 * when the window is resized. Uses letterboxing (black bars top/bottom) or
 * pillarboxing (black bars left/right) to maintain the aspect ratio.
 */
enum class AspectRatio
{
    /*!
     * @brief No aspect ratio constraint - use full window size.
     * 
     * This allows the viewport to match the window size exactly,
     * which may result in stretching if the window aspect ratio changes.
     */
    Auto,

    /*!
     * @brief 16:9 aspect ratio (≈1.778).
     * 
     * Most common widescreen format used in:
     * - Modern monitors and TVs
     * - HD video (1920x1080, 2560x1440, 3840x2160)
     * - Most console games
     * 
     * This is the default aspect ratio.
     */
    Ratio_16_9,

    /*!
     * @brief 4:3 aspect ratio (≈1.333).
     * 
     * Classic format used in:
     * - Older CRT monitors
     * - Standard definition TV
     * - Retro games
     */
    Ratio_4_3,

    /*!
     * @brief 21:9 aspect ratio (≈2.333).
     * 
     * Ultrawide format used in:
     * - Ultrawide monitors (2560x1080, 3440x1440)
     * - Cinematic displays
     */
    Ratio_21_9,

    /*!
     * @brief 16:10 aspect ratio (1.6).
     * 
     * Common in:
     * - Professional monitors
     * - Some laptops
     * - MacBook displays
     */
    Ratio_16_10,

    /*!
     * @brief 1:1 aspect ratio (square).
     * 
     * Perfect square format.
     */
    Ratio_1_1,

    /*!
     * @brief Custom aspect ratio.
     * 
     * Allows specifying a custom width:height ratio.
     * Use get_custom_ratio() to retrieve the value.
     */
    Custom
};

/*!
 * @brief Viewport rectangle for rendering with aspect ratio constraints.
 * 
 * Defines the region of the window where rendering occurs.
 * Coordinates are in pixels, with origin at top-left.
 */
struct Viewport
{
    uint32_t x = 0;      //!< X offset from left edge
    uint32_t y = 0;      //!< Y offset from top edge
    uint32_t width = 0;  //!< Viewport width in pixels
    uint32_t height = 0; //!< Viewport height in pixels
};

/*!
 * @brief Get the numeric ratio value for a given AspectRatio.
 * 
 * @param ratio The aspect ratio enum value.
 * @param custom_value Custom ratio value (width/height), only used if ratio == Custom.
 * @return The numeric ratio as a float (width / height).
 * 
 * @example
 * float ratio = get_aspect_ratio_value(AspectRatio::Ratio_16_9);
 * // ratio == 1.777...
 * 
 * float custom = get_aspect_ratio_value(AspectRatio::Custom, 2.35f);
 * // custom == 2.35
 */
constexpr float get_aspect_ratio_value(AspectRatio ratio, float custom_value = 1.0f)
{
    switch (ratio)
    {
        case AspectRatio::Auto:
            return 0.0F; // Special value: use window aspect ratio
        case AspectRatio::Ratio_16_9:
            return 16.0F / 9.0F; // ≈1.778
        case AspectRatio::Ratio_4_3:
            return 4.0F / 3.0F; // ≈1.333
        case AspectRatio::Ratio_21_9:
            return 21.0F / 9.0F; // ≈2.333
        case AspectRatio::Ratio_16_10:
            return 16.0F / 10.0F; // 1.6
        case AspectRatio::Ratio_1_1:
            return 1.0F;
        case AspectRatio::Custom:
            return custom_value;
        default:
            return 16.0F / 9.0F; // Default to 16:9
    }
}

/*!
 * @brief Calculate viewport that maintains aspect ratio within window bounds.
 * 
 * Computes the largest viewport that fits inside the window while maintaining
 * the specified aspect ratio. The viewport is centered in the window.
 * 
 * Letterboxing: If window is too tall, black bars appear top/bottom.
 * Pillarboxing: If window is too wide, black bars appear left/right.
 * 
 * @param window_width Width of the window in pixels.
 * @param window_height Height of the window in pixels.
 * @param ratio Desired aspect ratio to maintain.
 * @param custom_ratio Custom ratio value (only used if ratio == Custom).
 * @return Viewport rectangle centered in window with correct aspect ratio.
 * 
 * @example
 * // Window is 1920x1200, maintain 16:9
 * auto vp = calculate_viewport(1920, 1200, AspectRatio::Ratio_16_9);
 * // Result: vp.width = 1920, vp.height = 1080 (letterbox 60px top, 60px bottom)
 * 
 * // Window is 800x600, maintain 16:9
 * auto vp2 = calculate_viewport(800, 600, AspectRatio::Ratio_16_9);
 * // Result: vp2.width = 800, vp2.height = 450 (letterbox 75px top, 75px bottom)
 */
inline Viewport calculate_viewport(uint32_t window_width, 
                                   uint32_t window_height,
                                   AspectRatio ratio,
                                   float custom_ratio = 1.0F)
{
    Viewport viewport;

    // Auto mode: use full window
    if (ratio == AspectRatio::Auto)
    {
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = window_width;
        viewport.height = window_height;
        return viewport;
    }

    // Get target aspect ratio
    const float target_ratio = get_aspect_ratio_value(ratio, custom_ratio);
    const float window_ratio = static_cast<float>(window_width) / static_cast<float>(window_height);

    if (window_ratio > target_ratio)
    {
        // Window is wider than target ratio → pillarbox (black bars left/right)
        viewport.height = window_height;
        viewport.width = static_cast<uint32_t>(std::roundf(static_cast<float>(window_height) * target_ratio));
        viewport.x = (window_width - viewport.width) / 2;
        viewport.y = 0;
    }
    else
    {
        // Window is taller than target ratio → letterbox (black bars top/bottom)
        viewport.width = window_width;
        viewport.height = static_cast<uint32_t>(std::roundf(static_cast<float>(window_width) / target_ratio));
        viewport.x = 0;
        viewport.y = (window_height - viewport.height) / 2;
    }

    return viewport;
}

} // namespace raktr::render

#endif // RAKTR_RENDER_ASPECT_RATIO_H
