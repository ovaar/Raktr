# Hi-Z Occlusion Culling Implementation Notes

## Current Status

The Hi-Z occlusion culling infrastructure is **implemented but not working correctly** due to a fundamental architectural issue with how the depth pyramid is built.

## The Problem

Currently, the depth pyramid is built from the **full scene depth buffer** (including all geometry: occluders AND occludees). This creates a circular dependency:

1. Frame N: Render all geometry → Build pyramid from all depth → Test visibility for Frame N+1
2. Frame N+1: Test against pyramid (but occludees from Frame N are IN the pyramid) → They pass the test → Render them again
3. Result: **Nothing gets culled** because objects prevent themselves from being occluded

## The Solution (from References)

According to GPU Gems Chapter 29, VTK WebGPU Occlusion, and standard practice:

### Two-Pass Rendering (Correct Approach)

1. **Occluder Pass** (Early Z-pass):
   - Render ONLY large occluder geometry (walls, terrain, buildings)
   - Write to depth buffer only (can skip color writes for performance)
   - This should be a small subset of total geometry

2. **Build Hi-Z Pyramid**:
   - From the occluder-only depth buffer
   - Pyramid now represents "blocking" geometry only
   - No occludees are in the pyramid

3. **Visibility Testing**:
   - Test ALL objects (including occluders) against the occluder pyramid
   - Objects behind occluders will fail the depth test
   - Occluders themselves will pass (they're in the pyramid)

4. **Main Rendering Pass**:
   - Render only visible objects
   - Depth test set to EQUAL or LEQUAL (Early-Z will reject occluded fragments)

### Implementation Requirements

To implement this correctly, we need:

1. **API Changes**:
   ```cpp
   // Need ability to render to depth-only (no color)
   device->begin_depth_only_pass();
   device->render_geometry(occluders_only);
   device->end_depth_only_pass();
   
   // Build pyramid from current depth
   hi_z_buffer->build_pyramid(device->get_depth_texture());
   
   // Test visibility
   auto visibility = hi_z_buffer->test_visibility(all_aabbs, view_proj);
   
   // Render visible objects normally
   device->begin_render_pass();
   device->render_geometry(visible_objects);
   device->end_render_pass();
   ```

2. **Scene Organization**:
   - Objects must be tagged as "occluder" or "occludee"
   - Occluders should be large, static geometry
   - Small objects should not be occluders (overhead > benefit)

3. **Depth Buffer Management**:
   - Same depth buffer used for both passes
   - First pass writes occluder depth
   - Second pass uses existing depth for early-Z rejection

## Alternative Approaches

### Conservative Approach (Current Attempt)

- Always render occluders (they define visibility)
- Only test occludees for culling
- Build pyramid from all geometry
- **Problem**: Occludees still pass because they're in previous frame's pyramid

### Hierarchical Z-Buffer with Reprojection

- Build pyramid from previous frame
- Reproject previous camera position to current
- Adjust depth values for camera movement
- **Complex** but handles temporal coherence better

### Software Occlusion Culling

- Rasterize occluders to CPU depth buffer
- Test AABBs on CPU
- Simpler but slower than GPU approach
- Good fallback for platforms without compute shaders

## Performance Characteristics

### Current (Broken) Implementation

- **Pyramid build**: ~0.4ms
- **Visibility test**: ~15-20ms (for 103 objects)
- **Culling rate**: 0% (nothing culled)
- **Net benefit**: **NEGATIVE** (pure overhead)

### Expected (Working) Implementation

- **Pyramid build**: ~0.5ms (from ~3 occluders only)
- **Visibility test**: ~15-20ms
- **Culling rate**: 60-80% (for properly occluded scenes)
- **Net benefit**: **POSITIVE** (skip rendering 60-80% of geometry)

## Test Scene Analysis

Current test scene (`test_visual_triangle.cpp::OcclusionCullingDemo`):

- **3 occluders**: Large walls (red, green, blue)
- **100 occludees**: Small boxes behind walls
- **Camera position**: `(0, 5, 30)` looking at origin

**Expected behavior** (if working):
- Yellow boxes behind central wall at `z=-10`: Should be **70-80% occluded**
- Orange boxes behind left wall: Should be **60-70% occluded**
- Purple boxes behind right wall: Should be **60-70% occluded**
- Cyan boxes in front at `z=15`: Should be **100% visible**

**Actual behavior**: Everything visible (0% culled)

## Recommendations

### Short Term (Get It Working)

1. Add `device->begin_depth_only_pass()` API
2. Modify test to render occluders first
3. Build pyramid from occluder depth only
4. Verify culling works before optimizing

### Medium Term (Optimize)

1. Add frustum culling BEFORE occlusion culling
2. Implement two-frame latency hiding
3. Profile and tune mip selection
4. Add debug visualization (show pyramid as overlay)

### Long Term (Production)

1. Implement automatic occluder selection (size-based)
2. Add reprojection for temporal coherence  
3. Integrate with streaming/LOD system
4. Add fallback for platforms without compute shaders

## References

- [NVIDIA GPU Gems: Efficient Occlusion Culling](https://developer.nvidia.com/gpugems/gpugems/part-v-performance-and-practicalities/chapter-29-efficient-occlusion-culling)
- [VTK WebGPU Occlusion Culling](https://www.kitware.com/webgpu-occlusion-culling-in-vtk/)
- [Hi-Z Culling Research Paper](https://www.techscience.com/csse/v47n2/53678/pdf)

## Next Steps

1. Implement depth-only rendering pass in WebGPU backend
2. Modify test scene to use two-pass approach
3. Verify culling percentages match expectations
4. Document working implementation
5. Add to rendering guide
