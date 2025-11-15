// depth_pyramid.wgsl
// Compute shader for building hierarchical depth buffer (Hi-Z).
// Each pass downsamples the previous mip level by taking the maximum depth
// of a 2x2 block (conservative occlusion).

@group(0) @binding(0) var input_depth: texture_2d<f32>;
@group(0) @binding(1) var output_depth: texture_storage_2d<r32float, write>;
@group(0) @binding(2) var depth_sampler: sampler;

// Push constants for mip level info
struct PushConstants {
    mip_level: u32,
    src_width: u32,
    src_height: u32,
    dst_width: u32,
    dst_height: u32,
}

@group(1) @binding(0) var<uniform> constants: PushConstants;

// Work group size: 8x8 threads per group
@compute @workgroup_size(8, 8, 1)
fn main(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let dst_coord = vec2<i32>(global_id.xy);
    
    // Check if thread is within output bounds
    if (dst_coord.x >= i32(constants.dst_width) || dst_coord.y >= i32(constants.dst_height)) {
        return;
    }
    
    // Calculate source texture coordinates (2x2 block in previous mip level)
    let src_coord = vec2<f32>(dst_coord) * 2.0;
    let texel_size = vec2<f32>(1.0 / f32(constants.src_width), 1.0 / f32(constants.src_height));
    
    // Sample 2x2 block from previous mip level
    let uv_base = (src_coord + vec2<f32>(0.5, 0.5)) * texel_size;
    
    let d00 = textureSampleLevel(input_depth, depth_sampler, uv_base + vec2<f32>(0.0, 0.0) * texel_size, 0.0).r;
    let d10 = textureSampleLevel(input_depth, depth_sampler, uv_base + vec2<f32>(1.0, 0.0) * texel_size, 0.0).r;
    let d01 = textureSampleLevel(input_depth, depth_sampler, uv_base + vec2<f32>(0.0, 1.0) * texel_size, 0.0).r;
    let d11 = textureSampleLevel(input_depth, depth_sampler, uv_base + vec2<f32>(1.0, 1.0) * texel_size, 0.0).r;
    
    // Take maximum depth (farthest point) - conservative occlusion
    // If any texel in the 2x2 block is visible, the entire block is considered potentially visible
    let max_depth = max(max(d00, d10), max(d01, d11));
    
    // Write to output mip level
    textureStore(output_depth, dst_coord, vec4<f32>(max_depth, 0.0, 0.0, 0.0));
}
