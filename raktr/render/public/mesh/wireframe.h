/*!
 * @file wireframe.h
 * @brief Wireframe edge extraction utilities for mesh rendering.
 */

#ifndef RAKTR_RENDER_MESH_WIREFRAME_H
#define RAKTR_RENDER_MESH_WIREFRAME_H

#include <vector>
#include <cstdint>

namespace raktr::render::mesh
{
    /*!
     * @brief Extract unique edges from triangle mesh for wireframe rendering.
     * 
     * Uses edge hash set algorithm (O(N)) to deduplicate shared edges
     * between adjacent triangles. Each triangle contributes 3 edges, but
     * shared edges between adjacent triangles are only included once.
     * 
     * @param triangle_indices Index buffer with triangles (every 3 indices = 1 triangle).
     * @return Line index buffer (every 2 indices = 1 line segment).
     * 
     * @example
     * std::vector<uint32_t> tri_indices = {0, 1, 2, 1, 3, 2};  // 2 triangles
     * auto line_indices = extract_wireframe_indices(tri_indices);
     * // line_indices contains 5 unique edges: (0,1), (0,2), (1,2), (1,3), (2,3)
     * // Total size: 10 vertices (5 edges × 2 vertices per edge)
     * 
     * @note For closed meshes, the number of unique edges is approximately 1.5 × number of triangles
     *       (based on Euler's formula: E ≈ 3V/2 for a mesh with V vertices).
     * @note Incomplete triangles (index count not divisible by 3) are ignored.
     * @note Edges are normalized (min vertex index first) to ensure consistent deduplication.
     */
    std::vector<uint32_t> extract_wireframe_indices(
        const std::vector<uint32_t>& triangle_indices);

} // namespace raktr::render::mesh

#endif // RAKTR_RENDER_MESH_WIREFRAME_H
