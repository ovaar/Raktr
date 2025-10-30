/*!
 * @file obj_loader.cpp
 * @brief WaveFront OBJ file format loader implementation.
 */

#include "io/obj_loader.h"
#include <sstream>
#include <fstream>
#include <string_view>
#include <charconv>
#include <algorithm>

namespace raktr::render::io
{
    namespace
    {
        struct FaceVertex
        {
            int pos_idx = -1;
            int uv_idx = -1;
            int normal_idx = -1;
        };

        [[maybe_unused]] std::expected<float, ObjError> parse_float(std::string_view sv)
        {
            // Trim whitespace
            while (!sv.empty() && std::isspace(sv.front())) sv.remove_prefix(1);
            while (!sv.empty() && std::isspace(sv.back())) sv.remove_suffix(1);
            
            float value;
            auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);
            if (ec != std::errc{})
            {
                return std::unexpected(ObjError::ParseError);
            }
            return value;
        }

        std::expected<int, ObjError> parse_int(std::string_view sv)
        {
            // Trim whitespace
            while (!sv.empty() && std::isspace(sv.front())) sv.remove_prefix(1);
            while (!sv.empty() && std::isspace(sv.back())) sv.remove_suffix(1);
            
            int value;
            auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);
            if (ec != std::errc{})
            {
                return std::unexpected(ObjError::ParseError);
            }
            return value;
        }

        std::expected<FaceVertex, ObjError> parse_face_vertex(std::string_view sv)
        {
            FaceVertex fv;
            
            size_t first_slash = sv.find('/');
            if (first_slash == std::string_view::npos)
            {
                // Format: v
                auto pos = parse_int(sv);
                if (!pos) return std::unexpected(pos.error());
                fv.pos_idx = pos.value();
                return fv;
            }

            // Parse position
            auto pos = parse_int(sv.substr(0, first_slash));
            if (!pos) return std::unexpected(pos.error());
            fv.pos_idx = pos.value();

            size_t second_slash = sv.find('/', first_slash + 1);
            if (second_slash == std::string_view::npos)
            {
                // Format: v/vt
                if (first_slash + 1 < sv.size())
                {
                    auto uv = parse_int(sv.substr(first_slash + 1));
                    if (!uv) return std::unexpected(uv.error());
                    fv.uv_idx = uv.value();
                }
                return fv;
            }

            // Parse UV (if present)
            if (second_slash > first_slash + 1)
            {
                auto uv = parse_int(sv.substr(first_slash + 1, second_slash - first_slash - 1));
                if (!uv) return std::unexpected(uv.error());
                fv.uv_idx = uv.value();
            }

            // Parse normal (if present)
            if (second_slash + 1 < sv.size())
            {
                auto normal = parse_int(sv.substr(second_slash + 1));
                if (!normal) return std::unexpected(normal.error());
                fv.normal_idx = normal.value();
            }

            return fv;
        }
    }

    std::expected<MeshData, ObjError> load_obj(std::istream& stream)
    {
        MeshData mesh;
        std::vector<glm::vec3> temp_positions;
        std::vector<glm::vec2> temp_uvs;
        std::vector<glm::vec3> temp_normals;

        std::string line;
        while (std::getline(stream, line))
        {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#')
                continue;

            std::istringstream iss(line);
            std::string type;
            iss >> type;

            if (type == "v")
            {
                // Vertex position
                glm::vec3 pos;
                iss >> pos.x >> pos.y >> pos.z;
                temp_positions.push_back(pos);
            }
            else if (type == "vt")
            {
                // Texture coordinate
                glm::vec2 uv;
                iss >> uv.x >> uv.y;
                temp_uvs.push_back(uv);
            }
            else if (type == "vn")
            {
                // Vertex normal
                glm::vec3 normal;
                iss >> normal.x >> normal.y >> normal.z;
                temp_normals.push_back(normal);
            }
            else if (type == "f")
            {
                // Face - can be triangle or quad
                std::vector<FaceVertex> face_vertices;
                std::string vertex_str;
                
                while (iss >> vertex_str)
                {
                    auto fv = parse_face_vertex(vertex_str);
                    if (!fv)
                        return std::unexpected(fv.error());
                    face_vertices.push_back(fv.value());
                }

                // Triangulate face (assumes convex polygon)
                // Triangle fan: (0, 1, 2), (0, 2, 3), (0, 3, 4), ...
                for (size_t i = 1; i + 1 < face_vertices.size(); ++i)
                {
                    const size_t triangle_indices[3] = {0, i, i + 1};
                    for (size_t j : triangle_indices)
                    {
                        const auto& fv = face_vertices[j];
                        
                        // Add position (OBJ indices are 1-based)
                        int pos_idx = fv.pos_idx - 1;
                        if (pos_idx < 0 || pos_idx >= static_cast<int>(temp_positions.size()))
                            return std::unexpected(ObjError::InvalidFormat);
                        
                        mesh.positions.push_back(temp_positions[pos_idx]);
                        mesh.indices.push_back(static_cast<uint32_t>(mesh.positions.size() - 1));

                        // Add UV if present
                        if (fv.uv_idx > 0)
                        {
                            int uv_idx = fv.uv_idx - 1;
                            if (uv_idx < 0 || uv_idx >= static_cast<int>(temp_uvs.size()))
                                return std::unexpected(ObjError::InvalidFormat);
                            mesh.uvs.push_back(temp_uvs[uv_idx]);
                        }

                        // Add normal if present
                        if (fv.normal_idx > 0)
                        {
                            int normal_idx = fv.normal_idx - 1;
                            if (normal_idx < 0 || normal_idx >= static_cast<int>(temp_normals.size()))
                                return std::unexpected(ObjError::InvalidFormat);
                            mesh.normals.push_back(temp_normals[normal_idx]);
                        }
                    }
                }
            }
        }

        return mesh;
    }

    std::expected<MeshData, ObjError> load_obj_file(const std::string& filepath)
    {
        std::ifstream file(filepath);
        if (!file.is_open())
        {
            return std::unexpected(ObjError::FileNotFound);
        }
        return load_obj(file);
    }

} // namespace raktr::render::io
