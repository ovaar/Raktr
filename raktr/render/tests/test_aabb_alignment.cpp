/*!
 * @file test_aabb_alignment.cpp
 * @brief Verify AABB struct alignment for GPU compatibility
 */
#include "occlusion/hi_z_buffer.h"
#include <cstddef>
#include <gtest/gtest.h>


using namespace raktr::render::occlusion;

TEST(AABB, alignment_matches_gpu_layout)
{
    // GPU WGSL layout: vec3 uses 16-byte alignment, so AABB = 32 bytes total
    EXPECT_EQ(sizeof(AABB), 32) << "AABB must be 32 bytes for GPU compatibility";
    EXPECT_EQ(alignof(AABB), 16) << "AABB must be 16-byte aligned";

    // Verify member offsets
    EXPECT_EQ(offsetof(AABB, min), 0) << "min should be at offset 0";
    EXPECT_EQ(offsetof(AABB, max), 16) << "max should be at offset 16 (after min + padding)";
}

TEST(AABB, array_layout_is_contiguous)
{
    AABB aabbs[3] = {
        { { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
        { { 2.0f, 2.0f, 2.0f }, { 3.0f, 3.0f, 3.0f } },
        { { 4.0f, 4.0f, 4.0f }, { 5.0f, 5.0f, 5.0f } }
    };

    // Verify array elements are 32 bytes apart
    auto* ptr0 = reinterpret_cast<const char*>(&aabbs[0]);
    auto* ptr1 = reinterpret_cast<const char*>(&aabbs[1]);
    auto* ptr2 = reinterpret_cast<const char*>(&aabbs[2]);

    EXPECT_EQ(ptr1 - ptr0, 32);
    EXPECT_EQ(ptr2 - ptr1, 32);
}
