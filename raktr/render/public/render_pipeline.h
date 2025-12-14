/*!
 * @file render_pipeline.h
 * @brief Type-erased render pipeline for graphics rendering.
 *
 * Provides a platform-agnostic API for creating immutable graphics pipeline state objects.
 * Includes vertex layouts, shader stages, blend state, depth/stencil state, and primitive topology.
 */

#pragma once

#include "shader_module.h"
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace raktr::render
{

    /*!
     * @brief Vertex attribute format.
     */
    enum class VertexFormat : uint8_t
    {
        Float32x2 = 0, //!< 2D float vector
        Float32x3 = 1, //!< 3D float vector
        Float32x4 = 2, //!< 4D float vector
        Uint32    = 3, //!< Unsigned 32-bit integer
        Sint32    = 4  //!< Signed 32-bit integer
    };

    /*!
     * @brief Vertex input rate.
     */
    enum class VertexStepMode : uint8_t
    {
        Vertex   = 0, //!< Advance per vertex
        Instance = 1  //!< Advance per instance
    };

    /*!
     * @brief Vertex attribute descriptor.
     */
    struct VertexAttribute
    {
        VertexFormat format;          //!< Attribute format
        uint64_t     offset;          //!< Byte offset in vertex buffer
        uint32_t     shader_location; //!< Shader input location
    };

    /*!
     * @brief Vertex buffer layout descriptor.
     */
    struct VertexBufferLayout
    {
        uint64_t                     array_stride; //!< Stride between vertices
        VertexStepMode               step_mode = VertexStepMode::Vertex;
        std::vector<VertexAttribute> attributes; //!< Vertex attributes
    };

    /*!
     * @brief Primitive topology type.
     */
    enum class PrimitiveTopology : uint8_t
    {
        PointList     = 0, //!< Points
        LineList      = 1, //!< Disconnected lines
        LineStrip     = 2, //!< Connected lines
        TriangleList  = 3, //!< Disconnected triangles (most common)
        TriangleStrip = 4  //!< Connected triangles
    };

    /*!
     * @brief Front face winding order.
     */
    enum class FrontFace : uint8_t
    {
        CCW = 0, //!< Counter-clockwise (default)
        CW  = 1  //!< Clockwise
    };

    /*!
     * @brief Face culling mode.
     */
    enum class CullMode : uint8_t
    {
        None  = 0, //!< No culling
        Front = 1, //!< Cull front faces
        Back  = 2  //!< Cull back faces (most common)
    };

    /*!
     * @brief Primitive state descriptor.
     */
    struct PrimitiveState
    {
        PrimitiveTopology topology   = PrimitiveTopology::TriangleList;
        FrontFace         front_face = FrontFace::CCW;
        CullMode          cull_mode  = CullMode::None;
    };

    /*!
     * @brief Blend operation.
     */
    enum class BlendOperation : uint8_t
    {
        Add             = 0, //!< source + destination
        Subtract        = 1, //!< source - destination
        ReverseSubtract = 2, //!< destination - source
        Min             = 3, //!< min(source, destination)
        Max             = 4  //!< max(source, destination)
    };

    /*!
     * @brief Blend factor.
     */
    enum class BlendFactor : uint8_t
    {
        Zero              = 0,  //!< 0
        One               = 1,  //!< 1
        Src               = 2,  //!< source color
        OneMinusSrc       = 3,  //!< 1 - source color
        SrcAlpha          = 4,  //!< source alpha
        OneMinusSrcAlpha  = 5,  //!< 1 - source alpha
        Dst               = 6,  //!< destination color
        OneMinusDst       = 7,  //!< 1 - destination color
        DstAlpha          = 8,  //!< destination alpha
        OneMinusDstAlpha  = 9,  //!< 1 - destination alpha
        SrcAlphaSaturated = 10, //!< min(source alpha, 1 - destination alpha)
        Constant          = 11, //!< blend constant
        OneMinusConstant  = 12  //!< 1 - blend constant
    };

    /*!
     * @brief Blend component state.
     */
    struct BlendComponent
    {
        BlendOperation operation  = BlendOperation::Add;
        BlendFactor    src_factor = BlendFactor::One;
        BlendFactor    dst_factor = BlendFactor::Zero;
    };

    /*!
     * @brief Color write mask.
     */
    enum class ColorWriteMask : uint8_t
    {
        None  = 0x0, //!< No channels
        Red   = 0x1, //!< Red channel
        Green = 0x2, //!< Green channel
        Blue  = 0x4, //!< Blue channel
        Alpha = 0x8, //!< Alpha channel
        All   = 0xF  //!< All channels
    };

    /*!
     * @brief Blend state descriptor.
     */
    struct BlendState
    {
        BlendComponent color;
        BlendComponent alpha;
        ColorWriteMask write_mask = ColorWriteMask::All;
    };

    /*!
     * @brief Comparison function.
     */
    enum class CompareFunction : uint8_t
    {
        Never        = 0, //!< Never pass
        Less         = 1, //!< Pass if less
        Equal        = 2, //!< Pass if equal
        LessEqual    = 3, //!< Pass if less or equal
        Greater      = 4, //!< Pass if greater
        NotEqual     = 5, //!< Pass if not equal
        GreaterEqual = 6, //!< Pass if greater or equal
        Always       = 7  //!< Always pass
    };

    /*!
     * @brief Stencil operation.
     */
    enum class StencilOperation : uint8_t
    {
        Keep           = 0, //!< Keep current value
        Zero           = 1, //!< Set to zero
        Replace        = 2, //!< Replace with reference
        Invert         = 3, //!< Bitwise invert
        IncrementClamp = 4, //!< Increment and clamp
        DecrementClamp = 5, //!< Decrement and clamp
        IncrementWrap  = 6, //!< Increment and wrap
        DecrementWrap  = 7  //!< Decrement and wrap
    };

    /*!
     * @brief Stencil face state.
     */
    struct StencilFaceState
    {
        CompareFunction  compare       = CompareFunction::Always;
        StencilOperation fail_op       = StencilOperation::Keep;
        StencilOperation depth_fail_op = StencilOperation::Keep;
        StencilOperation pass_op       = StencilOperation::Keep;
    };

    /*!
     * @brief Depth/stencil state descriptor.
     */
    struct DepthStencilState
    {
        CompareFunction  depth_compare       = CompareFunction::Less;
        bool             depth_write_enabled = true;
        StencilFaceState stencil_front;
        StencilFaceState stencil_back;
        uint32_t         stencil_read_mask  = 0xFFFFFFFF;
        uint32_t         stencil_write_mask = 0xFFFFFFFF;
    };

    /*!
     * @brief Texture format.
     */
    enum class TextureFormat : uint8_t
    {
        BGRA8Unorm          = 0, //!< 8-bit BGRA (most common for surfaces)
        RGBA8Unorm          = 1, //!< 8-bit RGBA
        Depth24PlusStencil8 = 2, //!< 24-bit depth + 8-bit stencil
        Depth32Float        = 3  //!< 32-bit float depth
    };

    /*!
     * @brief Color target state.
     */
    struct ColorTargetState
    {
        TextureFormat             format = TextureFormat::BGRA8Unorm;
        std::optional<BlendState> blend; //!< Blending disabled if nullopt
    };

    /*!
     * @brief Render pipeline descriptor.
     *
     * Describes all immutable state for a graphics pipeline.
     */
    struct RenderPipelineDescriptor
    {
        std::string_view label = "";

        // Vertex stage
        ShaderModule                    vertex_shader;
        std::string_view                vertex_entry_point = "vs_main";
        std::vector<VertexBufferLayout> vertex_buffers;

        // Fragment stage
        ShaderModule                  fragment_shader;
        std::string_view              fragment_entry_point = "fs_main";
        std::vector<ColorTargetState> color_targets; //!< Up to 8 color attachments

        // Fixed-function state
        PrimitiveState                   primitive;
        std::optional<DepthStencilState> depth_stencil;

        // Bind group layouts (Phase 3 - will be set later)
        // std::vector<BindGroupLayout> bind_group_layouts;
    };

    /*!
     * @brief Type-erased render pipeline.
     *
     * Represents an immutable graphics pipeline state object.
     * Once created, pipeline state cannot be changed.
     */
    class RenderPipeline
    {
    public:
        RenderPipeline() : _self(nullptr), _ptr(nullptr), _owns(false)
        {
        }

        template <typename T>
        explicit RenderPipeline(T impl)
            : _self(std::make_unique<Model<T>>(std::move(impl))),
              _ptr(_self.get()),
              _owns(true)
        {
        }

        RenderPipeline(const RenderPipeline& other)
            : _self(other._self ? other._self->clone() : nullptr),
              _ptr(_self.get()),
              _owns(true)
        {
        }

        RenderPipeline(RenderPipeline&& other) noexcept
            : _self(std::move(other._self)),
              _ptr(other._ptr),
              _owns(other._owns)
        {
            other._ptr  = nullptr;
            other._owns = false;
        }

        RenderPipeline& operator=(const RenderPipeline& other)
        {
            if (this != &other)
            {
                _self = other._self ? other._self->clone() : nullptr;
                _ptr  = _self.get();
                _owns = true;
            }
            return *this;
        }

        RenderPipeline& operator=(RenderPipeline&& other) noexcept
        {
            if (this != &other)
            {
                _self       = std::move(other._self);
                _ptr        = other._ptr;
                _owns       = other._owns;
                other._ptr  = nullptr;
                other._owns = false;
            }
            return *this;
        }

        [[nodiscard]] bool is_valid() const
        {
            return _ptr != nullptr;
        }
        explicit operator bool() const
        {
            return is_valid();
        }

        [[nodiscard]] void* native_handle() const
        {
            return _ptr ? _ptr->native_handle() : nullptr;
        }

    private:
        struct Concept
        {
            virtual ~Concept()                                     = default;
            virtual std::unique_ptr<Concept> clone() const         = 0;
            virtual void*                    native_handle() const = 0;
        };

        template <typename T>
        struct Model final : Concept
        {
            explicit Model(T impl) : _impl(std::move(impl))
            {
            }

            std::unique_ptr<Concept> clone() const override
            {
                return std::make_unique<Model<T>>(_impl);
            }

            void* native_handle() const override
            {
                if constexpr (requires { _impl.native_handle(); })
                {
                    return _impl.native_handle();
                }
                return nullptr;
            }

            T _impl;
        };

        std::unique_ptr<Concept> _self;
        Concept*                 _ptr;
        bool                     _owns;
    };

} // namespace raktr::render
