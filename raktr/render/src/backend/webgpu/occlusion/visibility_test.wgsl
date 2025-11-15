// visibility_test.wgsl
// Compute shader for testing AABB visibility against hierarchical depth buffer.
// Projects AABBs to screen space and samples appropriate mip level of depth pyramid.

@group(0) @binding(0) var hi_z_texture: texture_2d<f32>;
@group(0) @binding(1) var hi_z_sampler: sampler;
@group(0) @binding(2) var<storage, read> aabb_buffer: array<AABB>;
@group(0) @binding(3) var<storage, read_write> visibility_buffer: array<u32>; // Bitfield

struct AABB {
    min: vec3<f32>,
    max: vec3<f32>,
}

struct ViewProjection {
    matrix: mat4x4<f32>,
    viewport_width: f32,
    viewport_height: f32,
    near_plane: f32,
    far_plane: f32,
}

@group(1) @binding(0) var<uniform> vp: ViewProjection;

// Project point to normalized device coordinates
fn project_to_ndc(world_pos: vec3<f32>) -> vec4<f32> {
    return vp.matrix * vec4<f32>(world_pos, 1.0);
}

// Convert NDC to screen-space UV coordinates
fn ndc_to_uv(ndc: vec2<f32>) -> vec2<f32> {
    return (ndc * vec2<f32>(0.5, -0.5)) + vec2<f32>(0.5, 0.5);
}

// Compute screen-space AABB from world-space AABB
fn compute_screen_aabb(aabb: AABB) -> vec4<f32> {
    // Project all 8 corners of the AABB
    var min_uv = vec2<f32>(1.0, 1.0);
    var max_uv = vec2<f32>(0.0, 0.0);
    var min_depth = 1.0;
    
    let corners = array<vec3<f32>, 8>(
        vec3<f32>(aabb.min.x, aabb.min.y, aabb.min.z),
        vec3<f32>(aabb.max.x, aabb.min.y, aabb.min.z),
        vec3<f32>(aabb.min.x, aabb.max.y, aabb.min.z),
        vec3<f32>(aabb.max.x, aabb.max.y, aabb.min.z),
        vec3<f32>(aabb.min.x, aabb.min.y, aabb.max.z),
        vec3<f32>(aabb.max.x, aabb.min.y, aabb.max.z),
        vec3<f32>(aabb.min.x, aabb.max.y, aabb.max.z),
        vec3<f32>(aabb.max.x, aabb.max.y, aabb.max.z),
    );
    
    for (var i = 0u; i < 8u; i = i + 1u) {
        let proj = project_to_ndc(corners[i]);
        
        // Skip if behind camera
        if (proj.w <= 0.0) {
            continue;
        }
        
        let ndc = proj.xy / proj.w;
        let uv = ndc_to_uv(ndc);
        
        // Clamp to screen bounds
        min_uv = min(min_uv, uv);
        max_uv = max(max_uv, uv);
        
        // Track nearest depth (for occlusion test)
        let depth = proj.z / proj.w;
        min_depth = min(min_depth, depth);
    }
    
    return vec4<f32>(min_uv, max_uv);
}

// Determine appropriate mip level based on screen-space size
fn compute_mip_level(screen_aabb: vec4<f32>) -> f32 {
    let width_px = (screen_aabb.z - screen_aabb.x) * vp.viewport_width;
    let height_px = (screen_aabb.w - screen_aabb.y) * vp.viewport_height;
    let max_size = max(width_px, height_px);
    
    // Mip level: log2(max_size)
    return max(0.0, log2(max_size));
}

@compute @workgroup_size(64, 1, 1)
fn main(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let aabb_idx = global_id.x;
    let total_aabbs = arrayLength(&aabb_buffer);
    
    if (aabb_idx >= total_aabbs) {
        return;
    }
    
    let aabb = aabb_buffer[aabb_idx];
    
    // Compute screen-space AABB
    let screen_aabb = compute_screen_aabb(aabb);
    
    // Check if AABB is off-screen
    if (screen_aabb.z < 0.0 || screen_aabb.x > 1.0 ||
        screen_aabb.w < 0.0 || screen_aabb.y > 1.0) {
        // Off-screen, mark as not visible
        let word_idx = aabb_idx / 32u;
        let bit_idx = aabb_idx % 32u;
        atomicAnd(&visibility_buffer[word_idx], ~(1u << bit_idx));
        return;
    }
    
    // Compute appropriate mip level
    let mip_level = compute_mip_level(screen_aabb);
    
    // Sample Hi-Z texture at center of screen AABB
    let uv_center = (screen_aabb.xy + screen_aabb.zw) * 0.5;
    let hi_z_depth = textureSampleLevel(hi_z_texture, hi_z_sampler, uv_center, mip_level).r;
    
    // Compute AABB's nearest depth
    let nearest_depth = compute_screen_aabb(aabb).x; // Simplified: should track min depth separately
    
    // Conservative occlusion test: if AABB's nearest point is farther than Hi-Z depth, it's occluded
    let is_visible = nearest_depth <= hi_z_depth;
    
    // Write to visibility bitfield
    let word_idx = aabb_idx / 32u;
    let bit_idx = aabb_idx % 32u;
    
    if (is_visible) {
        atomicOr(&visibility_buffer[word_idx], 1u << bit_idx);
    } else {
        atomicAnd(&visibility_buffer[word_idx], ~(1u << bit_idx));
    }
}
