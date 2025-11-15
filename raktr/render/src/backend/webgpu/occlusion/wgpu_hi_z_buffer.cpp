/*!
 * @file wgpu_hi_z_buffer.cpp
 * @brief WebGPU implementation of Hierarchical Z-Buffer occlusion culling.
 */

#include "wgpu_hi_z_buffer.h"
#include "render_error.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>
#include <vector>

namespace raktr::render::backend::webgpu
{

    namespace
    {
        // Helper to create WGPUStringView from C string
        static WGPUStringView make_string_view(const char* str)
        {
            WGPUStringView view = {};
            view.data           = str;
            view.length         = str ? strlen(str) : 0;
            return view;
        }

        /*!
         * @brief Load WGSL shader from embedded string or file.
         */
        std::string load_shader_source(const char* filename)
        {
            // TODO: For now, return embedded shader code
            // In production, load from file or embed at build time

            if (std::string(filename) == "depth_pyramid.wgsl")
            {
                return R"(
@group(0) @binding(0) var input_depth: texture_2d<f32>;
@group(0) @binding(1) var output_depth: texture_storage_2d<r32float, write>;
@group(0) @binding(2) var depth_sampler: sampler;

struct PushConstants {
    mip_level: u32,
    src_width: u32,
    src_height: u32,
    dst_width: u32,
    dst_height: u32,
}

@group(1) @binding(0) var<uniform> constants: PushConstants;

@compute @workgroup_size(8, 8, 1)
fn main(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let dst_coord = vec2<i32>(global_id.xy);
    
    if (dst_coord.x >= i32(constants.dst_width) || dst_coord.y >= i32(constants.dst_height)) {
        return;
    }
    
    let src_coord = vec2<f32>(dst_coord) * 2.0;
    let texel_size = vec2<f32>(1.0 / f32(constants.src_width), 1.0 / f32(constants.src_height));
    
    let uv_base = (src_coord + vec2<f32>(0.5, 0.5)) * texel_size;
    
    let d00 = textureSampleLevel(input_depth, depth_sampler, uv_base + vec2<f32>(0.0, 0.0) * texel_size, 0.0).r;
    let d10 = textureSampleLevel(input_depth, depth_sampler, uv_base + vec2<f32>(1.0, 0.0) * texel_size, 0.0).r;
    let d01 = textureSampleLevel(input_depth, depth_sampler, uv_base + vec2<f32>(0.0, 1.0) * texel_size, 0.0).r;
    let d11 = textureSampleLevel(input_depth, depth_sampler, uv_base + vec2<f32>(1.0, 1.0) * texel_size, 0.0).r;
    
    let max_depth = max(max(d00, d10), max(d01, d11));
    
    textureStore(output_depth, dst_coord, vec4<f32>(max_depth, 0.0, 0.0, 0.0));
}
)";
            }
            else if (std::string(filename) == "visibility_test.wgsl")
            {
                return R"(
@group(0) @binding(0) var hi_z_texture: texture_2d<f32>;
@group(0) @binding(1) var hi_z_sampler: sampler;
@group(0) @binding(2) var<storage, read> aabb_buffer: array<AABB>;
@group(0) @binding(3) var<storage, read_write> visibility_buffer: array<atomic<u32>>;

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

fn project_to_ndc(world_pos: vec3<f32>) -> vec4<f32> {
    return vp.matrix * vec4<f32>(world_pos, 1.0);
}

fn ndc_to_uv(ndc: vec2<f32>) -> vec2<f32> {
    return (ndc * vec2<f32>(0.5, -0.5)) + vec2<f32>(0.5, 0.5);
}

fn compute_screen_aabb(aabb: AABB) -> vec4<f32> {
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
        if (proj.w <= 0.0) { continue; }
        
        let ndc = proj.xy / proj.w;
        let uv = ndc_to_uv(ndc);
        min_uv = min(min_uv, uv);
        max_uv = max(max_uv, uv);
        min_depth = min(min_depth, proj.z / proj.w);
    }
    
    return vec4<f32>(min_uv.x, min_uv.y, max_uv.x, max_uv.y);
}

fn compute_mip_level(screen_aabb: vec4<f32>) -> f32 {
    let width_px = (screen_aabb.z - screen_aabb.x) * vp.viewport_width;
    let height_px = (screen_aabb.w - screen_aabb.y) * vp.viewport_height;
    let max_size = max(width_px, height_px);
    return max(0.0, log2(max_size));
}

@compute @workgroup_size(64, 1, 1)
fn main(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let aabb_idx = global_id.x;
    let total_aabbs = arrayLength(&aabb_buffer);
    
    if (aabb_idx >= total_aabbs) { return; }
    
    let aabb = aabb_buffer[aabb_idx];
    let screen_aabb = compute_screen_aabb(aabb);
    
    if (screen_aabb.z < 0.0 || screen_aabb.x > 1.0 || screen_aabb.w < 0.0 || screen_aabb.y > 1.0) {
        let word_idx = aabb_idx / 32u;
        let bit_idx = aabb_idx % 32u;
        atomicAnd(&visibility_buffer[word_idx], ~(1u << bit_idx));
        return;
    }
    
    let mip_level = compute_mip_level(screen_aabb);
    let uv_center = (screen_aabb.xy + screen_aabb.zw) * 0.5;
    let hi_z_depth = textureSampleLevel(hi_z_texture, hi_z_sampler, uv_center, mip_level).r;
    
    let nearest_depth = screen_aabb.x;
    let is_visible = nearest_depth <= hi_z_depth;
    
    let word_idx = aabb_idx / 32u;
    let bit_idx = aabb_idx % 32u;
    
    if (is_visible) {
        atomicOr(&visibility_buffer[word_idx], 1u << bit_idx);
    } else {
        atomicAnd(&visibility_buffer[word_idx], ~(1u << bit_idx));
    }
}
)";
            }

            return "";
        }

    } // anonymous namespace

    WgpuHiZBuffer::WgpuHiZBuffer(WGPUDevice device, WGPUQueue queue, uint32_t width, uint32_t height)
        : _device(device),
          _queue(queue),
          _width(width),
          _height(height),
          _mip_levels(compute_mip_levels(width, height))
    {
        if (!_device || !_queue)
        {
            spdlog::error("WgpuHiZBuffer: Invalid device or queue");
            return;
        }

        spdlog::info("Creating Hi-Z buffer {}x{} with {} mip levels", width, height, _mip_levels);

        create_depth_pyramid_texture();
        create_depth_pyramid_pipeline();
        create_visibility_test_pipeline();

        spdlog::info("WgpuHiZBuffer initialized successfully");
    }

    WgpuHiZBuffer::~WgpuHiZBuffer()
    {
        // Release WebGPU resources
        if (_pyramid_pipeline)
            wgpuComputePipelineRelease(_pyramid_pipeline);
        if (_visibility_pipeline)
            wgpuComputePipelineRelease(_visibility_pipeline);
        if (_pyramid_bind_group_layout)
            wgpuBindGroupLayoutRelease(_pyramid_bind_group_layout);
        if (_visibility_bind_group_layout)
            wgpuBindGroupLayoutRelease(_visibility_bind_group_layout);
        if (_depth_pyramid)
            wgpuTextureRelease(_depth_pyramid);
        if (_depth_pyramid_view)
            wgpuTextureViewRelease(_depth_pyramid_view);
        if (_depth_sampler)
            wgpuSamplerRelease(_depth_sampler);
        if (_aabb_buffer)
            wgpuBufferRelease(_aabb_buffer);
        if (_visibility_buffer)
            wgpuBufferRelease(_visibility_buffer);
        if (_uniform_buffer)
            wgpuBufferRelease(_uniform_buffer);
    }

    WgpuHiZBuffer::WgpuHiZBuffer(WgpuHiZBuffer&& other) noexcept
        : _device(nullptr),
          _queue(nullptr),
          _width(0),
          _height(0),
          _mip_levels(0),
          _depth_pyramid(nullptr),
          _depth_pyramid_view(nullptr),
          _depth_sampler(nullptr),
          _pyramid_pipeline(nullptr),
          _visibility_pipeline(nullptr),
          _pyramid_bind_group_layout(nullptr),
          _visibility_bind_group_layout(nullptr),
          _aabb_buffer(nullptr),
          _visibility_buffer(nullptr),
          _uniform_buffer(nullptr),
          _stats{}
    {
        swap(*this, other);
    }

    WgpuHiZBuffer& WgpuHiZBuffer::operator=(WgpuHiZBuffer&& other) noexcept
    {
        WgpuHiZBuffer tmp(std::move(other));
        swap(*this, tmp);
        return *this;
    }

    void swap(WgpuHiZBuffer& first, WgpuHiZBuffer& second) noexcept
    {
        using std::swap;

        swap(first._device, second._device);
        swap(first._queue, second._queue);
        swap(first._width, second._width);
        swap(first._height, second._height);
        swap(first._mip_levels, second._mip_levels);
        swap(first._depth_pyramid, second._depth_pyramid);
        swap(first._depth_pyramid_view, second._depth_pyramid_view);
        swap(first._depth_sampler, second._depth_sampler);
        swap(first._pyramid_pipeline, second._pyramid_pipeline);
        swap(first._visibility_pipeline, second._visibility_pipeline);
        swap(first._pyramid_bind_group_layout, second._pyramid_bind_group_layout);
        swap(first._visibility_bind_group_layout, second._visibility_bind_group_layout);
        swap(first._aabb_buffer, second._aabb_buffer);
        swap(first._visibility_buffer, second._visibility_buffer);
        swap(first._uniform_buffer, second._uniform_buffer);
        swap(first._stats, second._stats);
    }

    uint32_t WgpuHiZBuffer::compute_mip_levels(uint32_t width, uint32_t height) const
    {
        uint32_t max_dim = std::max(width, height);
        return static_cast<uint32_t>(std::floor(std::log2(static_cast<float>(max_dim)))) + 1;
    }

    void WgpuHiZBuffer::create_depth_pyramid_texture()
    {
        // Create depth pyramid texture with mip levels
        WGPUTextureDescriptor texture_desc{};
        texture_desc.label         = make_string_view("Hi-Z Depth Pyramid");
        texture_desc.size          = { _width, _height, 1 };
        texture_desc.mipLevelCount = _mip_levels;
        texture_desc.sampleCount   = 1;
        texture_desc.dimension     = WGPUTextureDimension_2D;
        texture_desc.format        = WGPUTextureFormat_R32Float;
        texture_desc.usage         = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_StorageBinding |
                             WGPUTextureUsage_CopySrc;

        _depth_pyramid = wgpuDeviceCreateTexture(_device, &texture_desc);
        if (!_depth_pyramid)
        {
            spdlog::error("Failed to create depth pyramid texture");
            return;
        }

        // Create texture view for sampling
        WGPUTextureViewDescriptor view_desc{};
        view_desc.label           = make_string_view("Hi-Z Depth Pyramid View");
        view_desc.format          = WGPUTextureFormat_R32Float;
        view_desc.dimension       = WGPUTextureViewDimension_2D;
        view_desc.baseMipLevel    = 0;
        view_desc.mipLevelCount   = _mip_levels;
        view_desc.baseArrayLayer  = 0;
        view_desc.arrayLayerCount = 1;
        view_desc.aspect          = WGPUTextureAspect_All;

        _depth_pyramid_view = wgpuTextureCreateView(_depth_pyramid, &view_desc);
        if (!_depth_pyramid_view)
        {
            spdlog::error("Failed to create depth pyramid view");
            return;
        }

        // Create sampler for depth pyramid (R32Float doesn't support filtering)
        WGPUSamplerDescriptor sampler_desc{};
        sampler_desc.label         = make_string_view("Hi-Z Depth Sampler");
        sampler_desc.addressModeU  = WGPUAddressMode_ClampToEdge;
        sampler_desc.addressModeV  = WGPUAddressMode_ClampToEdge;
        sampler_desc.addressModeW  = WGPUAddressMode_ClampToEdge;
        sampler_desc.magFilter     = WGPUFilterMode_Nearest; // R32Float is not filterable
        sampler_desc.minFilter     = WGPUFilterMode_Nearest;
        sampler_desc.mipmapFilter  = WGPUMipmapFilterMode_Nearest;
        sampler_desc.lodMinClamp   = 0.0f;
        sampler_desc.lodMaxClamp   = static_cast<float>(_mip_levels);
        sampler_desc.compare       = WGPUCompareFunction_Undefined;
        sampler_desc.maxAnisotropy = 1;

        _depth_sampler = wgpuDeviceCreateSampler(_device, &sampler_desc);
        if (!_depth_sampler)
        {
            spdlog::error("Failed to create depth sampler");
            return;
        }

        spdlog::info("Created depth pyramid texture and sampler");
    }

    void WgpuHiZBuffer::create_depth_pyramid_pipeline()
    {
        // Load shader
        std::string shader_source = load_shader_source("depth_pyramid.wgsl");
        if (shader_source.empty())
        {
            spdlog::error("Failed to load depth pyramid shader");
            return;
        }

        // Create shader module
        WGPUShaderSourceWGSL wgsl_desc{};
        wgsl_desc.chain.sType = WGPUSType_ShaderSourceWGSL;
        wgsl_desc.code        = make_string_view(shader_source.c_str());

        WGPUShaderModuleDescriptor shader_desc{};
        shader_desc.nextInChain = &wgsl_desc.chain;
        shader_desc.label       = make_string_view("Depth Pyramid Shader");

        WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(_device, &shader_desc);
        if (!shader_module)
        {
            spdlog::error("Failed to create depth pyramid shader module");
            return;
        }

        // Create bind group layout for depth pyramid generation
        std::vector<WGPUBindGroupLayoutEntry> entries(3);

        // @binding(0): input depth texture
        entries[0].binding               = 0;
        entries[0].visibility            = WGPUShaderStage_Compute;
        entries[0].texture.sampleType    = WGPUTextureSampleType_Float;
        entries[0].texture.viewDimension = WGPUTextureViewDimension_2D;

        // @binding(1): output depth storage texture
        entries[1].binding                      = 1;
        entries[1].visibility                   = WGPUShaderStage_Compute;
        entries[1].storageTexture.access        = WGPUStorageTextureAccess_WriteOnly;
        entries[1].storageTexture.format        = WGPUTextureFormat_R32Float;
        entries[1].storageTexture.viewDimension = WGPUTextureViewDimension_2D;

        // @binding(2): depth sampler
        entries[2].binding      = 2;
        entries[2].visibility   = WGPUShaderStage_Compute;
        entries[2].sampler.type = WGPUSamplerBindingType_Filtering;

        WGPUBindGroupLayoutDescriptor bg_layout_desc{};
        bg_layout_desc.label      = make_string_view("Depth Pyramid Bind Group Layout");
        bg_layout_desc.entryCount = static_cast<uint32_t>(entries.size());
        bg_layout_desc.entries    = entries.data();

        _pyramid_bind_group_layout = wgpuDeviceCreateBindGroupLayout(_device, &bg_layout_desc);
        if (!_pyramid_bind_group_layout)
        {
            spdlog::error("Failed to create pyramid bind group layout");
            wgpuShaderModuleRelease(shader_module);
            return;
        }

        // Create uniform buffer bind group layout (for push constants)
        WGPUBindGroupLayoutEntry uniform_entry{};
        uniform_entry.binding               = 0;
        uniform_entry.visibility            = WGPUShaderStage_Compute;
        uniform_entry.buffer.type           = WGPUBufferBindingType_Uniform;
        uniform_entry.buffer.minBindingSize = 5 * sizeof(uint32_t); // PushConstants struct

        WGPUBindGroupLayoutDescriptor uniform_layout_desc{};
        uniform_layout_desc.label      = make_string_view("Push Constants Layout");
        uniform_layout_desc.entryCount = 1;
        uniform_layout_desc.entries    = &uniform_entry;

        WGPUBindGroupLayout uniform_layout = wgpuDeviceCreateBindGroupLayout(_device, &uniform_layout_desc);
        if (!uniform_layout)
        {
            spdlog::error("Failed to create uniform bind group layout");
            wgpuShaderModuleRelease(shader_module);
            return;
        }

        // Create pipeline layout
        WGPUBindGroupLayout          layouts[] = { _pyramid_bind_group_layout, uniform_layout };
        WGPUPipelineLayoutDescriptor pipeline_layout_desc{};
        pipeline_layout_desc.label                = make_string_view("Depth Pyramid Pipeline Layout");
        pipeline_layout_desc.bindGroupLayoutCount = 2;
        pipeline_layout_desc.bindGroupLayouts     = layouts;

        WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(_device, &pipeline_layout_desc);
        if (!pipeline_layout)
        {
            spdlog::error("Failed to create depth pyramid pipeline layout");
            wgpuBindGroupLayoutRelease(uniform_layout);
            wgpuShaderModuleRelease(shader_module);
            return;
        }

        // Create compute pipeline
        WGPUComputePipelineDescriptor pipeline_desc{};
        pipeline_desc.label              = make_string_view("Depth Pyramid Pipeline");
        pipeline_desc.layout             = pipeline_layout;
        pipeline_desc.compute.module     = shader_module;
        pipeline_desc.compute.entryPoint = make_string_view("main");

        _pyramid_pipeline = wgpuDeviceCreateComputePipeline(_device, &pipeline_desc);
        if (!_pyramid_pipeline)
        {
            spdlog::error("Failed to create depth pyramid compute pipeline");
        }
        else
        {
            spdlog::info("Created depth pyramid compute pipeline");
        }

        // Cleanup temporary resources
        wgpuPipelineLayoutRelease(pipeline_layout);
        wgpuBindGroupLayoutRelease(uniform_layout);
        wgpuShaderModuleRelease(shader_module);
    }

    void WgpuHiZBuffer::create_visibility_test_pipeline()
    {
        std::string shader_source = load_shader_source("visibility_test.wgsl");
        if (shader_source.empty())
        {
            spdlog::error("Failed to load visibility test shader");
            return;
        }

        WGPUShaderSourceWGSL wgsl_desc{};
        wgsl_desc.chain.sType = WGPUSType_ShaderSourceWGSL;
        wgsl_desc.code        = make_string_view(shader_source.c_str());

        WGPUShaderModuleDescriptor shader_desc{};
        shader_desc.nextInChain = &wgsl_desc.chain;
        shader_desc.label       = make_string_view("Visibility Test Shader");

        WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(_device, &shader_desc);
        if (!shader_module)
        {
            spdlog::error("Failed to create visibility test shader module");
            return;
        }

        std::vector<WGPUBindGroupLayoutEntry> entries(4);

        entries[0].binding               = 0;
        entries[0].visibility            = WGPUShaderStage_Compute;
        entries[0].texture.sampleType    = WGPUTextureSampleType_UnfilterableFloat; // R32Float is not filterable
        entries[0].texture.viewDimension = WGPUTextureViewDimension_2D;

        entries[1].binding      = 1;
        entries[1].visibility   = WGPUShaderStage_Compute;
        entries[1].sampler.type = WGPUSamplerBindingType_NonFiltering;

        entries[2].binding     = 2;
        entries[2].visibility  = WGPUShaderStage_Compute;
        entries[2].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
        // WGSL vec3 has 16-byte alignment: min (16 bytes) + max (16 bytes) = 32 bytes per AABB
        entries[2].buffer.minBindingSize = 32;

        entries[3].binding               = 3;
        entries[3].visibility            = WGPUShaderStage_Compute;
        entries[3].buffer.type           = WGPUBufferBindingType_Storage;
        entries[3].buffer.minBindingSize = sizeof(uint32_t);

        WGPUBindGroupLayoutDescriptor bg_layout_desc{};
        bg_layout_desc.label      = make_string_view("Visibility Test Bind Group Layout");
        bg_layout_desc.entryCount = static_cast<uint32_t>(entries.size());
        bg_layout_desc.entries    = entries.data();

        _visibility_bind_group_layout = wgpuDeviceCreateBindGroupLayout(_device, &bg_layout_desc);
        if (!_visibility_bind_group_layout)
        {
            spdlog::error("Failed to create visibility bind group layout");
            wgpuShaderModuleRelease(shader_module);
            return;
        }

        WGPUBindGroupLayoutEntry uniform_entry{};
        uniform_entry.binding               = 0;
        uniform_entry.visibility            = WGPUShaderStage_Compute;
        uniform_entry.buffer.type           = WGPUBufferBindingType_Uniform;
        uniform_entry.buffer.minBindingSize = sizeof(float) * 20;

        WGPUBindGroupLayoutDescriptor uniform_layout_desc{};
        uniform_layout_desc.label      = make_string_view("Visibility Uniform Layout");
        uniform_layout_desc.entryCount = 1;
        uniform_layout_desc.entries    = &uniform_entry;

        WGPUBindGroupLayout uniform_layout = wgpuDeviceCreateBindGroupLayout(_device, &uniform_layout_desc);
        if (!uniform_layout)
        {
            spdlog::error("Failed to create visibility uniform layout");
            wgpuShaderModuleRelease(shader_module);
            return;
        }

        WGPUBindGroupLayout          layouts[] = { _visibility_bind_group_layout, uniform_layout };
        WGPUPipelineLayoutDescriptor pipeline_layout_desc{};
        pipeline_layout_desc.label                = make_string_view("Visibility Test Pipeline Layout");
        pipeline_layout_desc.bindGroupLayoutCount = 2;
        pipeline_layout_desc.bindGroupLayouts     = layouts;

        WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(_device, &pipeline_layout_desc);
        if (!pipeline_layout)
        {
            spdlog::error("Failed to create visibility test pipeline layout");
            wgpuBindGroupLayoutRelease(uniform_layout);
            wgpuShaderModuleRelease(shader_module);
            return;
        }

        WGPUComputePipelineDescriptor pipeline_desc{};
        pipeline_desc.label              = make_string_view("Visibility Test Pipeline");
        pipeline_desc.layout             = pipeline_layout;
        pipeline_desc.compute.module     = shader_module;
        pipeline_desc.compute.entryPoint = make_string_view("main");

        _visibility_pipeline = wgpuDeviceCreateComputePipeline(_device, &pipeline_desc);
        if (!_visibility_pipeline)
        {
            spdlog::error("Failed to create visibility test compute pipeline");
        }
        else
        {
            spdlog::info("Created visibility test compute pipeline");
        }

        wgpuPipelineLayoutRelease(pipeline_layout);
        wgpuBindGroupLayoutRelease(uniform_layout);
        wgpuShaderModuleRelease(shader_module);
    }

    std::expected<void, std::error_code>
    WgpuHiZBuffer::build_pyramid(void* depth_texture)
    {
        if (!depth_texture)
        {
            spdlog::error("Invalid depth texture provided");
            return std::unexpected(make_error_code(RenderError::InvalidOperation));
        }

        if (!_pyramid_pipeline)
        {
            spdlog::error("Depth pyramid pipeline not initialized");
            return std::unexpected(make_error_code(RenderError::InitializationFailed));
        }

        auto start_time = std::chrono::high_resolution_clock::now();

        WGPUTexture input_texture = static_cast<WGPUTexture>(depth_texture);

        struct PushConstants
        {
            uint32_t mip_level;
            uint32_t src_width;
            uint32_t src_height;
            uint32_t dst_width;
            uint32_t dst_height;
        };

        if (!_uniform_buffer || wgpuBufferGetSize(_uniform_buffer) < sizeof(PushConstants))
        {
            if (_uniform_buffer)
                wgpuBufferRelease(_uniform_buffer);

            WGPUBufferDescriptor uniform_desc{};
            uniform_desc.label            = make_string_view("Hi-Z Uniform Buffer");
            uniform_desc.size             = sizeof(PushConstants);
            uniform_desc.usage            = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
            uniform_desc.mappedAtCreation = false;

            _uniform_buffer = wgpuDeviceCreateBuffer(_device, &uniform_desc);
            if (!_uniform_buffer)
            {
                spdlog::error("Failed to create uniform buffer");
                return std::unexpected(make_error_code(RenderError::BufferCreationFailed));
            }
        }

        WGPUCommandEncoderDescriptor encoder_desc{};
        encoder_desc.label         = make_string_view("Hi-Z Pyramid Builder");
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(_device, &encoder_desc);

        uint32_t src_width  = _width;
        uint32_t src_height = _height;

        for (uint32_t mip = 0; mip < _mip_levels - 1; ++mip)
        {
            uint32_t dst_width  = std::max(1u, src_width / 2);
            uint32_t dst_height = std::max(1u, src_height / 2);

            PushConstants constants{
                mip,
                src_width,
                src_height,
                dst_width,
                dst_height
            };

            wgpuQueueWriteBuffer(_queue, _uniform_buffer, 0, &constants, sizeof(PushConstants));

            WGPUTextureViewDescriptor input_view_desc{};
            input_view_desc.format          = WGPUTextureFormat_R32Float;
            input_view_desc.dimension       = WGPUTextureViewDimension_2D;
            input_view_desc.baseMipLevel    = mip;
            input_view_desc.mipLevelCount   = 1;
            input_view_desc.baseArrayLayer  = 0;
            input_view_desc.arrayLayerCount = 1;
            input_view_desc.aspect          = WGPUTextureAspect_All;

            WGPUTextureView input_view = wgpuTextureCreateView(
                (mip == 0) ? input_texture : _depth_pyramid,
                &input_view_desc);

            WGPUTextureViewDescriptor output_view_desc{};
            output_view_desc.format          = WGPUTextureFormat_R32Float;
            output_view_desc.dimension       = WGPUTextureViewDimension_2D;
            output_view_desc.baseMipLevel    = mip + 1;
            output_view_desc.mipLevelCount   = 1;
            output_view_desc.baseArrayLayer  = 0;
            output_view_desc.arrayLayerCount = 1;
            output_view_desc.aspect          = WGPUTextureAspect_All;

            WGPUTextureView output_view = wgpuTextureCreateView(_depth_pyramid, &output_view_desc);

            std::vector<WGPUBindGroupEntry> bg_entries(3);
            bg_entries[0].binding     = 0;
            bg_entries[0].textureView = input_view;

            bg_entries[1].binding     = 1;
            bg_entries[1].textureView = output_view;

            bg_entries[2].binding = 2;
            bg_entries[2].sampler = _depth_sampler;

            WGPUBindGroupDescriptor bg_desc{};
            bg_desc.layout     = _pyramid_bind_group_layout;
            bg_desc.entryCount = static_cast<uint32_t>(bg_entries.size());
            bg_desc.entries    = bg_entries.data();

            WGPUBindGroup bind_group_0 = wgpuDeviceCreateBindGroup(_device, &bg_desc);

            WGPUBindGroupEntry uniform_entry{};
            uniform_entry.binding = 0;
            uniform_entry.buffer  = _uniform_buffer;
            uniform_entry.offset  = 0;
            uniform_entry.size    = sizeof(PushConstants);

            WGPUBindGroupDescriptor uniform_bg_desc{};
            uniform_bg_desc.layout     = wgpuComputePipelineGetBindGroupLayout(_pyramid_pipeline, 1);
            uniform_bg_desc.entryCount = 1;
            uniform_bg_desc.entries    = &uniform_entry;

            WGPUBindGroup bind_group_1 = wgpuDeviceCreateBindGroup(_device, &uniform_bg_desc);

            WGPUComputePassDescriptor pass_desc{};
            WGPUComputePassEncoder    pass = wgpuCommandEncoderBeginComputePass(encoder, &pass_desc);

            wgpuComputePassEncoderSetPipeline(pass, _pyramid_pipeline);
            wgpuComputePassEncoderSetBindGroup(pass, 0, bind_group_0, 0, nullptr);
            wgpuComputePassEncoderSetBindGroup(pass, 1, bind_group_1, 0, nullptr);

            uint32_t workgroup_x = (dst_width + 7) / 8;
            uint32_t workgroup_y = (dst_height + 7) / 8;
            wgpuComputePassEncoderDispatchWorkgroups(pass, workgroup_x, workgroup_y, 1);

            wgpuComputePassEncoderEnd(pass);
            wgpuComputePassEncoderRelease(pass);

            wgpuBindGroupRelease(bind_group_0);
            wgpuBindGroupRelease(bind_group_1);
            wgpuBindGroupLayoutRelease(uniform_bg_desc.layout);
            wgpuTextureViewRelease(input_view);
            wgpuTextureViewRelease(output_view);

            src_width  = dst_width;
            src_height = dst_height;
        }

        WGPUCommandBufferDescriptor cmd_buffer_desc{};
        WGPUCommandBuffer           cmd_buffer = wgpuCommandEncoderFinish(encoder, &cmd_buffer_desc);
        wgpuQueueSubmit(_queue, 1, &cmd_buffer);

        wgpuCommandBufferRelease(cmd_buffer);
        wgpuCommandEncoderRelease(encoder);

        auto end_time           = std::chrono::high_resolution_clock::now();
        _stats.pyramid_build_ms = std::chrono::duration<float, std::milli>(end_time - start_time).count();

        return {};
    }

    std::expected<std::vector<bool>, std::error_code>
    WgpuHiZBuffer::test_visibility(std::span<const occlusion::AABB> aabbs,
                                   const glm::mat4&                 view_projection)
    {
        if (aabbs.empty())
        {
            return std::vector<bool>{};
        }

        if (!_visibility_pipeline)
        {
            spdlog::error("Visibility test pipeline not initialized");
            return std::unexpected(make_error_code(RenderError::InitializationFailed));
        }

        auto start_time = std::chrono::high_resolution_clock::now();

        // AABB struct uses alignas(16) for direct GPU upload (32 bytes per AABB)
        static_assert(sizeof(occlusion::AABB) == 32, "AABB must be 32 bytes for GPU layout");
        static_assert(alignof(occlusion::AABB) == 16, "AABB must be 16-byte aligned");

        size_t aabb_buffer_size = aabbs.size() * sizeof(occlusion::AABB);

        if (!_aabb_buffer || wgpuBufferGetSize(_aabb_buffer) < aabb_buffer_size)
        {
            if (_aabb_buffer)
                wgpuBufferRelease(_aabb_buffer);

            WGPUBufferDescriptor aabb_desc{};
            aabb_desc.label            = make_string_view("AABB Buffer");
            aabb_desc.size             = aabb_buffer_size;
            aabb_desc.usage            = WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst;
            aabb_desc.mappedAtCreation = false;

            _aabb_buffer = wgpuDeviceCreateBuffer(_device, &aabb_desc);
            if (!_aabb_buffer)
            {
                spdlog::error("Failed to create AABB buffer");
                return std::unexpected(make_error_code(RenderError::BufferCreationFailed));
            }
        }

        // Direct upload - no padding needed thanks to alignas
        wgpuQueueWriteBuffer(_queue, _aabb_buffer, 0, aabbs.data(), aabb_buffer_size);

        uint32_t word_count             = (static_cast<uint32_t>(aabbs.size()) + 31) / 32;
        size_t   visibility_buffer_size = word_count * sizeof(uint32_t);

        if (!_visibility_buffer || wgpuBufferGetSize(_visibility_buffer) < visibility_buffer_size)
        {
            if (_visibility_buffer)
                wgpuBufferRelease(_visibility_buffer);

            WGPUBufferDescriptor vis_desc{};
            vis_desc.label            = make_string_view("Visibility Buffer");
            vis_desc.size             = visibility_buffer_size;
            vis_desc.usage            = WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst;
            vis_desc.mappedAtCreation = false;

            _visibility_buffer = wgpuDeviceCreateBuffer(_device, &vis_desc);
            if (!_visibility_buffer)
            {
                spdlog::error("Failed to create visibility buffer");
                return std::unexpected(make_error_code(RenderError::BufferCreationFailed));
            }
        }

        std::vector<uint32_t> init_visibility(word_count, 0xFFFFFFFF);
        wgpuQueueWriteBuffer(_queue, _visibility_buffer, 0, init_visibility.data(), visibility_buffer_size);

        struct ViewProjectionUniform
        {
            glm::mat4 matrix;
            float     viewport_width;
            float     viewport_height;
            float     near_plane;
            float     far_plane;
        };

        ViewProjectionUniform vp_uniform{
            view_projection,
            static_cast<float>(_width),
            static_cast<float>(_height),
            0.1f,
            1000.0f
        };

        WGPUBuffer           vp_buffer = nullptr;
        WGPUBufferDescriptor vp_desc{};
        vp_desc.label            = make_string_view("ViewProjection Buffer");
        vp_desc.size             = sizeof(ViewProjectionUniform);
        vp_desc.usage            = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
        vp_desc.mappedAtCreation = false;

        vp_buffer = wgpuDeviceCreateBuffer(_device, &vp_desc);
        if (!vp_buffer)
        {
            spdlog::error("Failed to create view-projection buffer");
            return std::unexpected(make_error_code(RenderError::BufferCreationFailed));
        }

        wgpuQueueWriteBuffer(_queue, vp_buffer, 0, &vp_uniform, sizeof(ViewProjectionUniform));

        std::vector<WGPUBindGroupEntry> bg_entries(4);
        bg_entries[0].binding     = 0;
        bg_entries[0].textureView = _depth_pyramid_view;

        bg_entries[1].binding = 1;
        bg_entries[1].sampler = _depth_sampler;

        bg_entries[2].binding = 2;
        bg_entries[2].buffer  = _aabb_buffer;
        bg_entries[2].offset  = 0;
        bg_entries[2].size    = aabb_buffer_size;

        bg_entries[3].binding = 3;
        bg_entries[3].buffer  = _visibility_buffer;
        bg_entries[3].offset  = 0;
        bg_entries[3].size    = visibility_buffer_size;

        WGPUBindGroupDescriptor bg_desc{};
        bg_desc.layout     = _visibility_bind_group_layout;
        bg_desc.entryCount = static_cast<uint32_t>(bg_entries.size());
        bg_desc.entries    = bg_entries.data();

        WGPUBindGroup bind_group_0 = wgpuDeviceCreateBindGroup(_device, &bg_desc);

        WGPUBindGroupEntry vp_entry{};
        vp_entry.binding = 0;
        vp_entry.buffer  = vp_buffer;
        vp_entry.offset  = 0;
        vp_entry.size    = sizeof(ViewProjectionUniform);

        WGPUBindGroupDescriptor vp_bg_desc{};
        vp_bg_desc.layout     = wgpuComputePipelineGetBindGroupLayout(_visibility_pipeline, 1);
        vp_bg_desc.entryCount = 1;
        vp_bg_desc.entries    = &vp_entry;

        WGPUBindGroup bind_group_1 = wgpuDeviceCreateBindGroup(_device, &vp_bg_desc);

        WGPUCommandEncoderDescriptor encoder_desc{};
        encoder_desc.label         = make_string_view("Visibility Test");
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(_device, &encoder_desc);

        WGPUComputePassDescriptor pass_desc{};
        WGPUComputePassEncoder    pass = wgpuCommandEncoderBeginComputePass(encoder, &pass_desc);

        wgpuComputePassEncoderSetPipeline(pass, _visibility_pipeline);
        wgpuComputePassEncoderSetBindGroup(pass, 0, bind_group_0, 0, nullptr);
        wgpuComputePassEncoderSetBindGroup(pass, 1, bind_group_1, 0, nullptr);

        uint32_t workgroup_count = (static_cast<uint32_t>(aabbs.size()) + 63) / 64;
        wgpuComputePassEncoderDispatchWorkgroups(pass, workgroup_count, 1, 1);

        wgpuComputePassEncoderEnd(pass);
        wgpuComputePassEncoderRelease(pass);

        WGPUBufferDescriptor readback_desc{};
        readback_desc.label            = make_string_view("Visibility Readback Buffer");
        readback_desc.size             = visibility_buffer_size;
        readback_desc.usage            = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
        readback_desc.mappedAtCreation = false;

        WGPUBuffer readback_buffer = wgpuDeviceCreateBuffer(_device, &readback_desc);
        wgpuCommandEncoderCopyBufferToBuffer(encoder, _visibility_buffer, 0, readback_buffer, 0, visibility_buffer_size);

        WGPUCommandBufferDescriptor cmd_buffer_desc{};
        WGPUCommandBuffer           cmd_buffer = wgpuCommandEncoderFinish(encoder, &cmd_buffer_desc);
        wgpuQueueSubmit(_queue, 1, &cmd_buffer);

        wgpuCommandBufferRelease(cmd_buffer);
        wgpuCommandEncoderRelease(encoder);

        struct MapContext
        {
            bool               done   = false;
            WGPUMapAsyncStatus status = WGPUMapAsyncStatus_Unknown;
        };

        MapContext map_ctx;

        WGPUBufferMapCallbackInfo callback_info{};
        callback_info.mode     = WGPUCallbackMode_WaitAnyOnly;
        callback_info.callback = [](WGPUMapAsyncStatus status, WGPUStringView, void* userdata1, void*)
        {
            auto* ctx   = static_cast<MapContext*>(userdata1);
            ctx->status = status;
            ctx->done   = true;
        };
        callback_info.userdata1 = &map_ctx;
        callback_info.userdata2 = nullptr;

        WGPUFuture map_future = wgpuBufferMapAsync(readback_buffer, WGPUMapMode_Read, 0, visibility_buffer_size, callback_info);
        (void)map_future; // Unused for now

        while (!map_ctx.done)
        {
            std::this_thread::yield();
        }

        if (map_ctx.status != WGPUMapAsyncStatus_Success)
        {
            spdlog::error("Failed to map visibility readback buffer");
            wgpuBufferRelease(readback_buffer);
            wgpuBufferRelease(vp_buffer);
            wgpuBindGroupRelease(bind_group_0);
            wgpuBindGroupRelease(bind_group_1);
            wgpuBindGroupLayoutRelease(vp_bg_desc.layout);
            return std::unexpected(make_error_code(RenderError::InvalidOperation));
        }

        const uint32_t* visibility_data = static_cast<const uint32_t*>(
            wgpuBufferGetConstMappedRange(readback_buffer, 0, visibility_buffer_size));

        std::vector<bool> visibility(aabbs.size());
        uint32_t          visible_count = 0;

        for (size_t i = 0; i < aabbs.size(); ++i)
        {
            uint32_t word_idx = static_cast<uint32_t>(i) / 32;
            uint32_t bit_idx  = static_cast<uint32_t>(i) % 32;
            visibility[i]     = (visibility_data[word_idx] & (1u << bit_idx)) != 0;
            if (visibility[i])
                ++visible_count;
        }

        wgpuBufferUnmap(readback_buffer);
        wgpuBufferRelease(readback_buffer);
        wgpuBufferRelease(vp_buffer);
        wgpuBindGroupRelease(bind_group_0);
        wgpuBindGroupRelease(bind_group_1);
        wgpuBindGroupLayoutRelease(vp_bg_desc.layout);

        auto end_time             = std::chrono::high_resolution_clock::now();
        _stats.visibility_test_ms = std::chrono::duration<float, std::milli>(end_time - start_time).count();
        _stats.objects_tested     = static_cast<uint32_t>(aabbs.size());
        _stats.objects_visible    = visible_count;
        _stats.objects_culled     = static_cast<uint32_t>(aabbs.size()) - visible_count;

        return visibility;
    }

} // namespace raktr::render::backend::webgpu
