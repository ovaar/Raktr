/*!
 * @file octree.cpp
 * @brief Implementation of thread-safe Octree spatial partitioning.
 */

#include "scene/octree.h"
#include <shared_mutex>
#include <memory>
#include <unordered_map>
#include <algorithm>
#include <utility>

namespace raktr::engine::scene
{
    // ========================================================================
    // Internal Implementation (Pimpl)
    // ========================================================================

    /*!
     * @brief Octree node representing a cubic region of space.
     */
    struct OctreeNode
    {
        glm::vec3 center;       //!< World-space center
        float     half_size;    //!< Half the side length (radius)
        uint8_t   depth;        //!< Depth in tree (0 = root)

        std::array<std::unique_ptr<OctreeNode>, 8> children; //!< 8 octants (nullptr if leaf)
        std::vector<Octree::ObjectId>              objects;  //!< Objects in this node

        /*!
         * @brief Check if this node is a leaf (no children).
         */
        [[nodiscard]] bool is_leaf() const
        {
            return children[0] == nullptr;
        }
    };

    /*!
     * @brief Object data stored in Octree.
     */
    struct OctreeObject
    {
        glm::vec3 position; //!< World-space position
        float     radius;   //!< Bounding sphere radius
    };

    /*!
     * @brief Pimpl implementation of Octree.
     */
    class Octree::Impl
    {
    public:
        Impl(const glm::vec3& center, float half_size, uint8_t max_depth, size_t max_objects)
            : _max_depth(max_depth)
            , _max_objects_per_node(max_objects)
        {
            // Create root node
            _root             = std::make_unique<OctreeNode>();
            _root->center     = center;
            _root->half_size  = half_size;
            _root->depth      = 0;
        }

        // Non-copyable, non-movable (contains std::shared_mutex)
        Impl(const Impl&)            = delete;
        Impl& operator=(const Impl&) = delete;
        Impl(Impl&&)                 = delete;
        Impl& operator=(Impl&&)      = delete;

        ~Impl() = default;

        // ====================================================================
        // Modification Operations
        // ====================================================================

        bool insert(Octree::ObjectId id, const glm::vec3& position, float radius)
        {
            std::unique_lock lock(_mutex);

            // Check if object already exists
            if (_objects.contains(id))
            {
                return false;
            }

            // Check if object is within root bounds
            if (!is_point_in_bounds(position, _root->center, _root->half_size))
            {
                return false;
            }

            // Store object data
            _objects[id] = OctreeObject{position, radius};

            // Insert into tree
            insert_into_node(_root.get(), id);

            return true;
        }

        bool remove(Octree::ObjectId id)
        {
            std::unique_lock lock(_mutex);

            // Check if object exists
            auto it = _objects.find(id);
            if (it == _objects.end())
            {
                return false;
            }

            // Remove from tree
            remove_from_node(_root.get(), id);

            // Remove object data
            _objects.erase(it);

            return true;
        }

        bool update(Octree::ObjectId id, const glm::vec3& new_position)
        {
            std::unique_lock lock(_mutex);

            // Check if object exists
            auto it = _objects.find(id);
            if (it == _objects.end())
            {
                return false;
            }

            // Check if new position is within root bounds
            if (!is_point_in_bounds(new_position, _root->center, _root->half_size))
            {
                return false;
            }

            // Remove from old location
            remove_from_node(_root.get(), id);

            // Update position
            it->second.position = new_position;

            // Reinsert at new location
            insert_into_node(_root.get(), id);

            return true;
        }

        void clear()
        {
            std::unique_lock lock(_mutex);

            // Clear all objects
            _objects.clear();

            // Rebuild root node (collapse tree)
            glm::vec3 center    = _root->center;
            float     half_size = _root->half_size;
            _root               = std::make_unique<OctreeNode>();
            _root->center       = center;
            _root->half_size    = half_size;
            _root->depth        = 0;
        }

        // ====================================================================
        // Query Operations
        // ====================================================================

        std::vector<Octree::ObjectId> query_frustum(const Frustum& frustum) const
        {
            std::shared_lock lock(_mutex);

            std::vector<Octree::ObjectId> result;
            query_frustum_recursive(_root.get(), frustum, result);
            return result;
        }

        std::vector<Octree::ObjectId> query_sphere(const glm::vec3& point, float radius) const
        {
            std::shared_lock lock(_mutex);

            std::vector<Octree::ObjectId> result;
            query_sphere_recursive(_root.get(), point, radius, result);
            return result;
        }

        std::optional<std::pair<Octree::ObjectId, float>>
        query_ray(const glm::vec3& origin, const glm::vec3& direction, float max_distance) const
        {
            std::shared_lock lock(_mutex);

            std::optional<std::pair<Octree::ObjectId, float>> nearest;
            query_ray_recursive(_root.get(), origin, direction, max_distance, nearest);
            return nearest;
        }

        // ====================================================================
        // State Query
        // ====================================================================

        [[nodiscard]] size_t size() const
        {
            std::shared_lock lock(_mutex);
            return _objects.size();
        }

        [[nodiscard]] Octree::Stats stats() const
        {
            std::shared_lock lock(_mutex);

            Octree::Stats stats{};
            stats.total_objects = _objects.size();
            compute_stats_recursive(_root.get(), stats);
            return stats;
        }

    private:
        // Configuration
        uint8_t _max_depth;
        size_t  _max_objects_per_node;

        // Data
        std::unique_ptr<OctreeNode>                         _root;
        std::unordered_map<Octree::ObjectId, OctreeObject> _objects;

        // Synchronization
        mutable std::shared_mutex _mutex;

        // ====================================================================
        // Helper Functions
        // ====================================================================

        /*!
         * @brief Check if point is within AABB bounds.
         */
        static bool is_point_in_bounds(const glm::vec3& point,
                                       const glm::vec3& center,
                                       float            half_size)
        {
            return (point.x >= center.x - half_size && point.x <= center.x + half_size)
                && (point.y >= center.y - half_size && point.y <= center.y + half_size)
                && (point.z >= center.z - half_size && point.z <= center.z + half_size);
        }

        /*!
         * @brief Insert object ID into appropriate node.
         */
        void insert_into_node(OctreeNode* node, Octree::ObjectId id)
        {
            // If node is a leaf and not at max depth, check if we should subdivide
            if (node->is_leaf())
            {
                node->objects.push_back(id);

                // Subdivide if threshold exceeded and depth allows
                if (node->objects.size() > _max_objects_per_node && node->depth < _max_depth)
                {
                    subdivide(node);
                }
            }
            else
            {
                // Node has children, find appropriate octant
                int octant        = get_octant(node, _objects[id].position);
                insert_into_node(node->children[octant].get(), id);
            }
        }

        /*!
         * @brief Remove object ID from tree.
         */
        void remove_from_node(OctreeNode* node, Octree::ObjectId id)
        {
            // Search in current node
            auto it = std::find(node->objects.begin(), node->objects.end(), id);
            if (it != node->objects.end())
            {
                node->objects.erase(it);
                return;
            }

            // Search in children
            if (!node->is_leaf())
            {
                for (auto& child : node->children)
                {
                    if (child)
                    {
                        remove_from_node(child.get(), id);
                    }
                }
            }
        }

        /*!
         * @brief Subdivide node into 8 children and redistribute objects.
         */
        void subdivide(OctreeNode* node)
        {
            float child_half_size = node->half_size * 0.5f;
            uint8_t child_depth   = node->depth + 1;

            // Create 8 children
            for (int i = 0; i < 8; ++i)
            {
                node->children[i]            = std::make_unique<OctreeNode>();
                node->children[i]->center    = get_octant_center(node->center, node->half_size, i);
                node->children[i]->half_size = child_half_size;
                node->children[i]->depth     = child_depth;
            }

            // Redistribute objects to children
            std::vector<Octree::ObjectId> remaining_objects;
            for (Octree::ObjectId id : node->objects)
            {
                const glm::vec3& pos = _objects[id].position;
                int              octant = get_octant(node, pos);
                node->children[octant]->objects.push_back(id);
            }

            // Clear parent's object list (objects now in children)
            node->objects.clear();
        }

        /*!
         * @brief Get octant index for a position relative to node center.
         *
         * Octant indexing:
         * - Bit 0: x >= center.x (0=left, 1=right)
         * - Bit 1: y >= center.y (0=bottom, 1=top)
         * - Bit 2: z >= center.z (0=back, 1=front)
         */
        static int get_octant(const OctreeNode* node, const glm::vec3& position)
        {
            int octant = 0;
            if (position.x >= node->center.x)
                octant |= 1;
            if (position.y >= node->center.y)
                octant |= 2;
            if (position.z >= node->center.z)
                octant |= 4;
            return octant;
        }

        /*!
         * @brief Calculate center of octant child.
         */
        static glm::vec3 get_octant_center(const glm::vec3& parent_center,
                                           float            parent_half_size,
                                           int              octant)
        {
            float offset = parent_half_size * 0.5f;
            return glm::vec3(parent_center.x + ((octant & 1) ? offset : -offset),
                             parent_center.y + ((octant & 2) ? offset : -offset),
                             parent_center.z + ((octant & 4) ? offset : -offset));
        }

        // ====================================================================
        // Query Recursion
        // ====================================================================

        void query_frustum_recursive(const OctreeNode*              node,
                                     const Frustum&                 frustum,
                                     std::vector<Octree::ObjectId>& result) const
        {
            // Test if node AABB intersects frustum
            if (!frustum.intersects_aabb(node->center, node->half_size))
            {
                return; // Cull entire subtree
            }

            // Add objects in this node
            for (Octree::ObjectId id : node->objects)
            {
                result.push_back(id);
            }

            // Recurse into children
            if (!node->is_leaf())
            {
                for (const auto& child : node->children)
                {
                    if (child)
                    {
                        query_frustum_recursive(child.get(), frustum, result);
                    }
                }
            }
        }

        void query_sphere_recursive(const OctreeNode*              node,
                                    const glm::vec3&               point,
                                    float                          radius,
                                    std::vector<Octree::ObjectId>& result) const
        {
            // Test if node AABB intersects query sphere (simple box-sphere test)
            glm::vec3 closest = glm::clamp(point, node->center - node->half_size,
                                           node->center + node->half_size);
            float dist_sq     = glm::dot(closest - point, closest - point);

            if (dist_sq > radius * radius)
            {
                return; // Cull subtree
            }

            // Check objects in this node
            for (Octree::ObjectId id : node->objects)
            {
                const OctreeObject& obj = _objects.at(id);
                float               d   = glm::distance(point, obj.position);
                if (d <= radius + obj.radius)
                {
                    result.push_back(id);
                }
            }

            // Recurse into children
            if (!node->is_leaf())
            {
                for (const auto& child : node->children)
                {
                    if (child)
                    {
                        query_sphere_recursive(child.get(), point, radius, result);
                    }
                }
            }
        }

        void query_ray_recursive(const OctreeNode*                                  node,
                                 const glm::vec3&                                   origin,
                                 const glm::vec3&                                   direction,
                                 float                                              max_distance,
                                 std::optional<std::pair<Octree::ObjectId, float>>& nearest) const
        {
            // Test ray-AABB intersection
            // (Simplified: always traverse for MVP, optimize later)

            // Check objects in this node
            for (Octree::ObjectId id : node->objects)
            {
                const OctreeObject& obj = _objects.at(id);

                // Ray-sphere intersection
                glm::vec3 oc         = origin - obj.position;
                float     b          = glm::dot(oc, direction);
                float     c          = glm::dot(oc, oc) - obj.radius * obj.radius;
                float     discriminant = b * b - c;

                if (discriminant >= 0.0f)
                {
                    float t = -b - std::sqrt(discriminant);
                    if (t >= 0.0f && t <= max_distance)
                    {
                        if (!nearest || t < nearest->second)
                        {
                            nearest = std::make_pair(id, t);
                        }
                    }
                }
            }

            // Recurse into children
            if (!node->is_leaf())
            {
                for (const auto& child : node->children)
                {
                    if (child)
                    {
                        query_ray_recursive(child.get(), origin, direction, max_distance, nearest);
                    }
                }
            }
        }

        void compute_stats_recursive(const OctreeNode* node, Octree::Stats& stats) const
        {
            stats.total_nodes++;

            if (node->is_leaf())
            {
                stats.leaf_nodes++;
            }

            stats.max_depth_used = std::max(stats.max_depth_used, node->depth);

            if (!node->objects.empty())
            {
                stats.max_objects_in_node = std::max(stats.max_objects_in_node, node->objects.size());
            }

            // Recurse into children
            if (!node->is_leaf())
            {
                for (const auto& child : node->children)
                {
                    if (child)
                    {
                        compute_stats_recursive(child.get(), stats);
                    }
                }
            }
        }
    };

    // ========================================================================
    // Octree Public API
    // ========================================================================

    Octree::Octree(const glm::vec3& center,
                   float            half_size,
                   uint8_t          max_depth,
                   size_t           max_objects_per_node)
        : _impl(new Impl(center, half_size, max_depth, max_objects_per_node))
    {
    }

    Octree::~Octree()
    {
        delete _impl;
    }

    Octree::Octree(Octree&& other) noexcept : _impl(std::exchange(other._impl, nullptr))
    {
    }

    Octree& Octree::operator=(Octree&& other) noexcept
    {
        if (this != &other)
        {
            delete _impl;
            _impl = std::exchange(other._impl, nullptr);
        }
        return *this;
    }

    bool Octree::insert(ObjectId id, const glm::vec3& position, float radius)
    {
        return _impl->insert(id, position, radius);
    }

    bool Octree::remove(ObjectId id)
    {
        return _impl->remove(id);
    }

    bool Octree::update(ObjectId id, const glm::vec3& new_position)
    {
        return _impl->update(id, new_position);
    }

    void Octree::clear()
    {
        _impl->clear();
    }

    std::vector<Octree::ObjectId> Octree::query_frustum(const Frustum& frustum) const
    {
        return _impl->query_frustum(frustum);
    }

    std::vector<Octree::ObjectId> Octree::query_sphere(const glm::vec3& point, float radius) const
    {
        return _impl->query_sphere(point, radius);
    }

    std::optional<std::pair<Octree::ObjectId, float>>
    Octree::query_ray(const glm::vec3& origin, const glm::vec3& direction, float max_distance) const
    {
        return _impl->query_ray(origin, direction, max_distance);
    }

    size_t Octree::size() const
    {
        return _impl->size();
    }

    Octree::Stats Octree::stats() const
    {
        return _impl->stats();
    }

    // ========================================================================
    // Frustum Implementation
    // ========================================================================

    Frustum Frustum::from_matrix(const glm::mat4& vp)
    {
        Frustum frustum;

        // Extract planes using Gribb & Hartmann method
        // Plane equations are in form: Ax + By + Cz + D = 0

        // Left plane: row4 + row1
        frustum.planes[0] = glm::vec4(vp[0][3] + vp[0][0], vp[1][3] + vp[1][0],
                                      vp[2][3] + vp[2][0], vp[3][3] + vp[3][0]);

        // Right plane: row4 - row1
        frustum.planes[1] = glm::vec4(vp[0][3] - vp[0][0], vp[1][3] - vp[1][0],
                                      vp[2][3] - vp[2][0], vp[3][3] - vp[3][0]);

        // Top plane: row4 - row2
        frustum.planes[2] = glm::vec4(vp[0][3] - vp[0][1], vp[1][3] - vp[1][1],
                                      vp[2][3] - vp[2][1], vp[3][3] - vp[3][1]);

        // Bottom plane: row4 + row2
        frustum.planes[3] = glm::vec4(vp[0][3] + vp[0][1], vp[1][3] + vp[1][1],
                                      vp[2][3] + vp[2][1], vp[3][3] + vp[3][1]);

        // Near plane: row4 + row3
        frustum.planes[4] = glm::vec4(vp[0][3] + vp[0][2], vp[1][3] + vp[1][2],
                                      vp[2][3] + vp[2][2], vp[3][3] + vp[3][2]);

        // Far plane: row4 - row3
        frustum.planes[5] = glm::vec4(vp[0][3] - vp[0][2], vp[1][3] - vp[1][2],
                                      vp[2][3] - vp[2][2], vp[3][3] - vp[3][2]);

        // Normalize planes
        for (auto& plane : frustum.planes)
        {
            float length = glm::length(glm::vec3(plane));
            plane /= length;
        }

        return frustum;
    }

    bool Frustum::intersects_aabb(const glm::vec3& center, float half_size) const
    {
        // Test AABB against all six planes
        for (const auto& plane : planes)
        {
            // Get AABB corner farthest along plane normal (p-vertex)
            glm::vec3 normal(plane.x, plane.y, plane.z);
            glm::vec3 p_vertex = center + glm::vec3((normal.x >= 0.0f) ? half_size : -half_size,
                                                    (normal.y >= 0.0f) ? half_size : -half_size,
                                                    (normal.z >= 0.0f) ? half_size : -half_size);

            // If p-vertex is outside plane, AABB is completely outside
            if (glm::dot(normal, p_vertex) + plane.w < 0.0f)
            {
                return false;
            }
        }

        return true; // AABB is inside or intersecting frustum
    }

} // namespace raktr::engine::scene
