/*!
 * @file octree.h
 * @brief Thread-safe Octree for spatial partitioning and culling.
 *
 * Provides a high-performance spatial data structure for managing game objects
 * in 3D space. Supports concurrent queries from multiple threads while allowing
 * updates from a single writer thread.
 *
 * Key features:
 * - Thread-safe queries (frustum culling, sphere queries, ray casting)
 * - Automatic subdivision based on object density
 * - Integration with Camera for frustum culling
 * - Cache-friendly memory layout
 * - C++23 synchronization primitives (std::shared_mutex)
 *
 * @example
 * using namespace raktr::engine::scene;
 *
 * // Create Octree covering [-1000, 1000] in all axes
 * Octree octree(glm::vec3(0), 1000.0f, 8);  // max depth 8
 *
 * // Insert objects
 * octree.insert(ObjectId{1}, glm::vec3(10, 0, 0), 5.0f);  // radius 5
 * octree.insert(ObjectId{2}, glm::vec3(-20, 10, 0), 3.0f);
 *
 * // Query from render thread (thread-safe, concurrent)
 * Camera camera(...);
 * Frustum frustum = camera.frustum();
 * auto visible = octree.query_frustum(frustum);
 *
 * // Update from game thread (exclusive access)
 * octree.update(ObjectId{1}, glm::vec3(15, 0, 0));
 *
 * // Get statistics
 * auto stats = octree.stats();
 * std::cout << "Nodes: " << stats.total_nodes << "\n";
 */

#ifndef RAKTR_ENGINE_SCENE_OCTREE_H
#define RAKTR_ENGINE_SCENE_OCTREE_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <optional>
#include <utility>
#include <vector>


namespace raktr::engine::scene
{
    // Forward declaration
    struct Frustum;

    /*!
     * @brief Thread-safe Octree for spatial partitioning.
     *
     * An Octree recursively subdivides 3D space into eight octants (children)
     * when the number of objects exceeds a threshold. This enables efficient
     * spatial queries by pruning large portions of the scene.
     *
     * Thread Safety:
     * - All query operations (query_frustum, query_sphere, query_ray) are
     *   thread-safe and can run concurrently from multiple threads.
     * - Modification operations (insert, remove, update, clear) require
     *   exclusive access and cannot run concurrently with each other or
     *   with queries.
     * - Uses std::shared_mutex internally for read-write lock.
     *
     * Performance:
     * - Subdivision occurs when node.size() > max_objects_per_node
     * - Max depth prevents degenerate cases (clustered objects)
     * - Objects outside root bounds are rejected
     * - Cache-friendly layout (std::array for children)
     *
     * Coordinate System:
     * - Right-handed (matches Raktr convention)
     * - Center + half_size defines AABB bounds
     * - Example: center=(0,0,0), half_size=100 => bounds [-100,100] each axis
     */
    class Octree
    {
    public:
        /*!
         * @brief Unique identifier for objects in the Octree.
         *
         * ObjectId is opaque to the Octree. Caller is responsible for
         * mapping ObjectId to game entities (e.g., ECS entity IDs).
         */
        using ObjectId = uint64_t;

        /*!
         * @brief Construct empty Octree.
         *
         * Creates a root node with specified bounds. The Octree covers
         * a cubic region from [center - half_size] to [center + half_size]
         * in all three axes.
         *
         * @param center World-space center of root node.
         * @param half_size Half the side length of root node (world units).
         *                  Root AABB = [center-half_size, center+half_size].
         * @param max_depth Maximum subdivision depth (default 8).
         *                  Depth 0 = root, depth 1 = 8 children, etc.
         *                  Max depth prevents infinite subdivision.
         * @param max_objects_per_node Split threshold (default 8).
         *                              Node subdivides when objects exceed this.
         *
         * @example
         * // Create Octree covering [-500, 500] in all axes
         * Octree octree(glm::vec3(0), 500.0f);
         *
         * // Create Octree with tighter subdivision
         * Octree octree(glm::vec3(0), 1000.0f, 10, 4);  // depth 10, split at 4 objects
         */
        Octree(const glm::vec3& center,
               float            half_size,
               uint8_t          max_depth            = 8,
               size_t           max_objects_per_node = 8);

        /*!
         * @brief Destructor.
         *
         * Automatically cleans up all nodes and internal state.
         */
        ~Octree();

        // Non-copyable (contains std::unique_ptr and std::shared_mutex)
        Octree(const Octree&)            = delete;
        Octree& operator=(const Octree&) = delete;

        // Movable (transfer ownership)
        Octree(Octree&&) noexcept;
        Octree& operator=(Octree&&) noexcept;

        // ====================================================================
        // Modification Operations (Exclusive Access)
        // ====================================================================

        /*!
         * @brief Insert object into Octree.
         *
         * Adds an object with specified position and bounding sphere radius.
         * If the object's bounding sphere extends outside the root bounds,
         * insertion fails.
         *
         * Thread Safety: Requires exclusive access (no concurrent queries).
         *
         * @param id Unique object identifier.
         * @param position World-space position (center of bounding sphere).
         * @param radius Bounding sphere radius (default 0 = point).
         * @return true if inserted successfully, false if:
         *         - Object with same ID already exists
         *         - Object is outside root bounds
         *
         * @example
         * octree.insert(ObjectId{1}, glm::vec3(10, 0, 0), 5.0f);
         */
        bool insert(ObjectId id, const glm::vec3& position, float radius = 0.0f);

        /*!
         * @brief Remove object from Octree.
         *
         * Thread Safety: Requires exclusive access (no concurrent queries).
         *
         * @param id Object identifier.
         * @return true if removed successfully, false if not found.
         *
         * @example
         * bool removed = octree.remove(ObjectId{1});
         */
        bool remove(ObjectId id);

        /*!
         * @brief Update object position (remove + reinsert).
         *
         * Convenience method equivalent to remove(id) followed by
         * insert(id, new_position, radius). Preserves radius from
         * original insertion.
         *
         * Thread Safety: Requires exclusive access (no concurrent queries).
         *
         * @param id Object identifier.
         * @param new_position New world-space position.
         * @return true if updated successfully, false if:
         *         - Object not found
         *         - New position is outside root bounds
         *
         * @example
         * octree.update(ObjectId{1}, glm::vec3(20, 0, 0));
         */
        bool update(ObjectId id, const glm::vec3& new_position);

        /*!
         * @brief Clear all objects from Octree.
         *
         * Removes all objects and collapses tree structure back to root.
         *
         * Thread Safety: Requires exclusive access (no concurrent queries).
         */
        void clear();

        // ====================================================================
        // Query Operations (Concurrent Read Access)
        // ====================================================================

        /*!
         * @brief Query objects inside view frustum (thread-safe).
         *
         * Returns all objects whose bounding spheres intersect the frustum.
         * Uses hierarchical culling: entire subtrees are skipped if their
         * AABB is outside the frustum.
         *
         * Thread Safety: Thread-safe. Can run concurrently with other queries.
         *
         * @param frustum View frustum from Camera::frustum().
         * @return Vector of object IDs that are potentially visible.
         *         May include false positives (object sphere intersects
         *         frustum but object itself doesn't).
         *
         * @example
         * Camera camera(...);
         * Frustum frustum = camera.frustum();
         * auto visible = octree.query_frustum(frustum);
         * for (ObjectId id : visible) {
         *     // Render object
         * }
         */
        std::vector<ObjectId> query_frustum(const Frustum& frustum) const;

        /*!
         * @brief Query objects near point (thread-safe).
         *
         * Returns all objects whose bounding spheres intersect a query sphere
         * centered at 'point' with radius 'radius'.
         *
         * Thread Safety: Thread-safe. Can run concurrently with other queries.
         *
         * @param point Query sphere center.
         * @param radius Query sphere radius.
         * @return Vector of object IDs within distance.
         *
         * @example
         * // Find objects within 50 units of (0, 0, 0)
         * auto nearby = octree.query_sphere(glm::vec3(0), 50.0f);
         */
        std::vector<ObjectId> query_sphere(const glm::vec3& point, float radius) const;

        /*!
         * @brief Query nearest object along ray (thread-safe).
         *
         * Casts a ray and returns the nearest object whose bounding sphere
         * intersects the ray. Does NOT perform precise ray-object intersection;
         * only tests against bounding spheres.
         *
         * Thread Safety: Thread-safe. Can run concurrently with other queries.
         *
         * @param origin Ray origin.
         * @param direction Ray direction (should be normalized).
         * @param max_distance Maximum ray distance (default 1000.0).
         * @return Pair of (ObjectId, distance) for nearest hit, or std::nullopt
         *         if no objects intersect the ray within max_distance.
         *
         * @example
         * auto hit = octree.query_ray(camera.position(), camera.forward(), 100.0f);
         * if (hit) {
         *     std::cout << "Hit object " << hit->first << " at distance " << hit->second;
         * }
         */
        std::optional<std::pair<ObjectId, float>>
        query_ray(const glm::vec3& origin,
                  const glm::vec3& direction,
                  float            max_distance = 1000.0f) const;

        // ====================================================================
        // State Query
        // ====================================================================

        /*!
         * @brief Get total number of objects in Octree.
         *
         * Thread Safety: Thread-safe.
         *
         * @return Number of objects currently stored.
         */
        [[nodiscard]] size_t size() const;

        /*!
         * @brief Octree statistics for debugging and profiling.
         */
        struct Stats
        {
            size_t  total_nodes;         //!< Total nodes (including root and leaves)
            size_t  leaf_nodes;          //!< Nodes with no children
            size_t  total_objects;       //!< Total objects across all nodes
            uint8_t max_depth_used;      //!< Maximum depth reached (0 = root only)
            size_t  max_objects_in_node; //!< Largest number of objects in a single node
        };

        /*!
         * @brief Get statistics about Octree structure.
         *
         * Thread Safety: Thread-safe.
         *
         * @return Statistics snapshot.
         *
         * @example
         * auto stats = octree.stats();
         * std::cout << "Nodes: " << stats.total_nodes << "\n"
         *           << "Max depth: " << (int)stats.max_depth_used << "\n";
         */
        [[nodiscard]] Stats stats() const;

    private:
        class Impl; // Pimpl pattern for ABI stability and hiding implementation
        Impl* _impl;
    };

    // ========================================================================
    // Frustum Culling
    // ========================================================================

    /*!
     * @brief View frustum for culling queries.
     *
     * A frustum is defined by six planes: left, right, top, bottom, near, far.
     * Each plane is represented as a vec4: (A, B, C, D) where the plane equation
     * is Ax + By + Cz + D = 0. The normal (A, B, C) points inward (toward visible space).
     *
     * Frustum can be extracted from a combined view-projection matrix using
     * the Gribb & Hartmann method.
     */
    struct Frustum
    {
        /*!
         * @brief Six frustum planes: [0]=left, [1]=right, [2]=top, [3]=bottom, [4]=near, [5]=far.
         *
         * Each plane is a vec4 (A, B, C, D) where:
         * - (A, B, C) is the plane normal (normalized)
         * - D is the distance from origin along normal
         * - Plane equation: Ax + By + Cz + D = 0
         * - Normals point inward (toward visible space)
         */
        std::array<glm::vec4, 6> planes;

        /*!
         * @brief Extract frustum from view-projection matrix.
         *
         * Uses the Gribb & Hartmann method to extract planes from the
         * combined view * projection matrix. Planes are normalized.
         *
         * @param vp Combined view * projection matrix.
         * @return Frustum with normalized plane equations.
         *
         * @example
         * Camera camera(...);
         * glm::mat4 vp = camera.projection().matrix() * camera.view().matrix();
         * Frustum frustum = Frustum::from_matrix(vp);
         */
        static Frustum from_matrix(const glm::mat4& vp);

        /*!
         * @brief Test if AABB intersects frustum.
         *
         * Tests if an axis-aligned bounding box (defined by center and half-size)
         * is fully outside, fully inside, or intersects the frustum.
         *
         * Returns true if AABB is fully or partially inside (visible).
         * Returns false if AABB is completely outside (culled).
         *
         * @param center AABB center point.
         * @param half_size Half the AABB side length (same for all axes).
         * @return true if AABB should be rendered (visible or intersecting).
         *
         * @example
         * if (frustum.intersects_aabb(node_center, node_half_size)) {
         *     // Node is visible, traverse children
         * }
         */
        [[nodiscard]] bool intersects_aabb(const glm::vec3& center, float half_size) const;

        /*!
         * @brief Test if bounding sphere intersects frustum.
         *
         * Tests if a bounding sphere (defined by center and radius)
         * is fully outside, fully inside, or intersects the frustum.
         *
         * Returns true if sphere is fully or partially inside (visible).
         * Returns false if sphere is completely outside (culled).
         *
         * @param center Sphere center point.
         * @param radius Sphere radius.
         * @return true if sphere should be rendered (visible or intersecting).
         *
         * @example
         * if (frustum.intersects_sphere(object_position, object_radius)) {
         *     // Object is visible, render it
         * }
         */
        [[nodiscard]] bool intersects_sphere(const glm::vec3& center, float radius) const;
    };

} // namespace raktr::engine::scene

#endif // RAKTR_ENGINE_SCENE_OCTREE_H
