/*!
 * @file obj_loader.h
 * @brief WaveFront OBJ file format loader with support for positions, normals, and UVs.
 */

#ifndef RAKTR_RENDER_IO_OBJ_LOADER_H
#define RAKTR_RENDER_IO_OBJ_LOADER_H

#include <expected>
#include <string>
#include <vector>
#include <istream>
#include <glm/glm.hpp>

namespace raktr::render::io
{
    /*!
     * @brief Mesh data loaded from OBJ file.
     */
    struct MeshData
    {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> uvs;
        std::vector<uint32_t> indices;
    };

    /*!
     * @brief Error codes for OBJ loading.
     */
    enum class ObjError
    {
        InvalidFormat,
        FileNotFound,
        ParseError
    };

    /*!
     * @brief Loads mesh data from an OBJ stream.
     * @param stream Input stream containing OBJ data.
     * @return MeshData on success, ObjError on failure.
     * @example
     * std::ifstream file("model.obj");
     * auto mesh = load_obj(file);
     */
    std::expected<MeshData, ObjError> load_obj(std::istream& stream);

    /*!
     * @brief Loads mesh data from an OBJ file path.
     * @param filepath Path to OBJ file.
     * @return MeshData on success, ObjError on failure.
     */
    std::expected<MeshData, ObjError> load_obj_file(const std::string& filepath);

} // namespace raktr::render::io

#endif // RAKTR_RENDER_IO_OBJ_LOADER_H
