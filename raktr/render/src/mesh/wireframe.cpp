/*!
 * @file wireframe.cpp
 * @brief Implementation of wireframe edge extraction algorithm.
 */

#include "mesh/wireframe.h"
#include <algorithm>
#include <unordered_set>

namespace raktr::render::mesh
{
    /*!
     * @brief Edge representation for hash set deduplication.
     *
     * Edges are normalized such that v0 <= v1, ensuring that
     * edges (a,b) and (b,a) are treated as the same edge.
     */
    struct Edge
    {
        uint32_t v0;
        uint32_t v1;

        /*!
         * @brief Construct edge with normalized vertex order.
         * @param a First vertex index.
         * @param b Second vertex index.
         */
        Edge(uint32_t a, uint32_t b)
            : v0(std::min(a, b)), v1(std::max(a, b))
        {
        }

        /*!
         * @brief Equality comparison for hash set.
         */
        bool operator==(const Edge& other) const
        {
            return v0 == other.v0 && v1 == other.v1;
        }
    };

    /*!
     * @brief Hash function for Edge.
     *
     * Combines two 32-bit indices into a single 64-bit hash value.
     * Uses simple bit-shifting to create unique hash for each edge pair.
     */
    struct EdgeHash
    {
        size_t operator()(const Edge& edge) const
        {
            // Combine two 32-bit values into 64-bit hash
            uint64_t combined = (static_cast<uint64_t>(edge.v0) << 32) | edge.v1;
            return std::hash<uint64_t>()(combined);
        }
    };

    std::vector<uint32_t> extract_wireframe_indices(
        const std::vector<uint32_t>& triangle_indices)
    {
        // Early exit for empty or incomplete input
        if (triangle_indices.size() < 3)
            return {};

        // Hash set for O(1) average-case edge deduplication
        std::unordered_set<Edge, EdgeHash> unique_edges;

        // Reserve approximate capacity (upper bound: 3 edges per triangle)
        unique_edges.reserve(triangle_indices.size());

        // Extract edges from each triangle
        for (size_t i = 0; i + 2 < triangle_indices.size(); i += 3)
        {
            uint32_t v0 = triangle_indices[i];
            uint32_t v1 = triangle_indices[i + 1];
            uint32_t v2 = triangle_indices[i + 2];

            // Add three edges of the triangle (normalized automatically)
            unique_edges.insert(Edge(v0, v1));
            unique_edges.insert(Edge(v1, v2));
            unique_edges.insert(Edge(v2, v0));
        }

        // Convert hash set to line index buffer
        std::vector<uint32_t> line_indices;
        line_indices.reserve(unique_edges.size() * 2);

        for (const auto& edge : unique_edges)
        {
            line_indices.push_back(edge.v0);
            line_indices.push_back(edge.v1);
        }

        return line_indices;
    }

} // namespace raktr::render::mesh
