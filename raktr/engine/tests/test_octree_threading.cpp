/*!
 * @file test_octree_threading.cpp
 * @brief Tests for multithreaded Octree access patterns.
 */

#include "gtest/gtest.h"

#include "scene/octree.h"

#include <glm/gtc/matrix_transform.hpp>

#include <atomic>
#include <barrier>
#include <chrono>
#include <random>
#include <thread>
#include <vector>

using namespace raktr::engine::scene;

// ============================================================================
// Helper Classes
// ============================================================================

/*!
 * @brief Test fixture for Octree threading tests.
 */
class OctreeThreadingTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create a standard octree for testing
        octree = std::make_unique<Octree>(glm::vec3(0.0f), 100.0f);
    }

    void TearDown() override
    {
        octree.reset();
    }

    std::unique_ptr<Octree> octree;
};

// ============================================================================
// Concurrent Query Tests
// ============================================================================

/*!
 * @brief Tests that multiple threads can query the octree simultaneously.
 *
 * @details Validates that std::shared_mutex allows concurrent readers.
 * Creates multiple threads performing queries in parallel.
 */
TEST_F(OctreeThreadingTest, concurrent_queries_from_multiple_threads)
{
    // Arrange: Insert objects
    octree->insert(1, glm::vec3(0.0f, 0.0f, 0.0f));
    octree->insert(2, glm::vec3(10.0f, 10.0f, 10.0f));
    octree->insert(3, glm::vec3(-10.0f, -10.0f, -10.0f));

    constexpr size_t         num_threads        = 8;
    constexpr size_t         queries_per_thread = 100;
    std::vector<std::thread> threads;
    std::atomic<size_t>      total_queries{ 0 };

    // Act: Multiple threads query simultaneously
    for (size_t i = 0; i < num_threads; ++i)
    {
        threads.emplace_back([&]()
                             {
                                 for (size_t j = 0; j < queries_per_thread; ++j)
                                 {
                                     auto results = octree->query_sphere(glm::vec3(0.0f), 50.0f);
                                     EXPECT_EQ(results.size(), 3); // All objects should be found
                                     ++total_queries;
                                 }
                             });
    }

    // Join all threads
    for (auto& thread : threads)
    {
        thread.join();
    }

    // Assert: All queries completed
    EXPECT_EQ(total_queries, num_threads * queries_per_thread);
}

/*!
 * @brief Tests concurrent queries using different query types.
 *
 * @details Validates that sphere, ray, and frustum queries can run concurrently.
 */
TEST_F(OctreeThreadingTest, concurrent_mixed_query_types)
{
    // Arrange: Insert objects
    octree->insert(1, glm::vec3(0.0f, 0.0f, 0.0f));
    octree->insert(2, glm::vec3(10.0f, 10.0f, 10.0f));
    octree->insert(3, glm::vec3(-10.0f, -10.0f, -10.0f));

    std::atomic<size_t> sphere_queries{ 0 };
    std::atomic<size_t> ray_queries{ 0 };
    std::atomic<size_t> frustum_queries{ 0 };

    // Act: Different threads use different query types
    std::thread sphere_thread([&]()
                              {
                                  for (size_t i = 0; i < 50; ++i)
                                  {
                                      octree->query_sphere(glm::vec3(0.0f), 50.0f);
                                      ++sphere_queries;
                                  }
                              });

    std::thread ray_thread([&]()
                           {
                               for (size_t i = 0; i < 50; ++i)
                               {
                                   octree->query_ray(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), 100.0f);
                                   ++ray_queries;
                               }
                           });

    std::thread frustum_thread([&]()
                               {
                                   glm::mat4 projection =
                                       glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
                                   glm::mat4 view = glm::lookAt(
                                       glm::vec3(0.0f, 0.0f, 50.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                                   Frustum frustum = Frustum::from_matrix(projection * view);

                                   for (size_t i = 0; i < 50; ++i)
                                   {
                                       octree->query_frustum(frustum);
                                       ++frustum_queries;
                                   }
                               });

    // Join all threads
    sphere_thread.join();
    ray_thread.join();
    frustum_thread.join();

    // Assert: All queries completed
    EXPECT_EQ(sphere_queries, 50);
    EXPECT_EQ(ray_queries, 50);
    EXPECT_EQ(frustum_queries, 50);
}

// ============================================================================
// Concurrent Query + Update Tests
// ============================================================================

/*!
 * @brief Tests that queries can run while updates are queued.
 *
 * @details Validates that std::shared_mutex serializes writes without
 * blocking concurrent readers except during actual write operations.
 */
TEST_F(OctreeThreadingTest, queries_run_concurrently_with_infrequent_updates)
{
    // Arrange: Insert initial objects
    for (size_t i = 0; i < 10; ++i)
    {
        octree->insert(i, glm::vec3(i * 5.0f, 0.0f, 0.0f));
    }

    std::atomic<size_t> query_count{ 0 };
    std::atomic<size_t> update_count{ 0 };
    std::atomic<bool>   stop{ false };

    // Act: Multiple query threads + one update thread
    std::vector<std::thread> query_threads;
    for (size_t i = 0; i < 4; ++i)
    {
        query_threads.emplace_back([&]()
                                   {
                                       while (!stop.load())
                                       {
                                           octree->query_sphere(glm::vec3(0.0f), 100.0f);
                                           ++query_count;
                                       }
                                   });
    }

    std::thread update_thread([&]()
                              {
                                  for (size_t i = 0; i < 10; ++i)
                                  {
                                      std::this_thread::sleep_for(std::chrono::milliseconds(5));
                                      octree->insert(100 + i, glm::vec3(i * 5.0f, 10.0f, 0.0f));
                                      ++update_count;
                                  }
                                  stop.store(true);
                              });

    // Join all threads
    update_thread.join();
    for (auto& thread : query_threads)
    {
        thread.join();
    }

    // Assert: Queries ran many times, updates completed
    EXPECT_EQ(update_count, 10);
    EXPECT_GT(query_count, 100); // Should run many queries during update pauses
}

/*!
 * @brief Tests that updates are serialized (no concurrent writes).
 *
 * @details Validates that std::unique_lock prevents concurrent modifications.
 * All insertions should complete successfully without data races.
 */
TEST_F(OctreeThreadingTest, updates_are_serialized)
{
    // Arrange: Prepare objects to insert
    constexpr size_t         num_threads        = 8;
    constexpr size_t         inserts_per_thread = 50;
    std::vector<std::thread> threads;

    // Act: Multiple threads insert objects
    for (size_t i = 0; i < num_threads; ++i)
    {
        threads.emplace_back([&, thread_id = i]()
                             {
                                 for (size_t j = 0; j < inserts_per_thread; ++j)
                                 {
                                     Octree::ObjectId id = thread_id * inserts_per_thread + j;
                                     glm::vec3        pos(j * 0.5f, thread_id * 0.5f, 0.0f);
                                     octree->insert(id, pos);
                                 }
                             });
    }

    // Join all threads
    for (auto& thread : threads)
    {
        thread.join();
    }

    // Assert: All objects inserted
    EXPECT_EQ(octree->size(), num_threads * inserts_per_thread);
}

/*!
 * @brief Tests mixed insert/remove operations from multiple threads.
 *
 * @details Validates that tree structure remains consistent under
 * concurrent modifications.
 */
TEST_F(OctreeThreadingTest, mixed_insert_remove_operations)
{
    // Arrange: Pre-populate octree (within bounds [-100, 100])
    for (size_t i = 0; i < 100; ++i)
    {
        octree->insert(i, glm::vec3(i * 0.5f - 25.0f, 0.0f, 0.0f));
    }

    std::atomic<size_t> insert_count{ 0 };
    std::atomic<size_t> remove_count{ 0 };

    // Act: Concurrent inserts and removes
    std::thread insert_thread([&]()
                              {
                                  for (size_t i = 200; i < 250; ++i)
                                  {
                                      octree->insert(i, glm::vec3((i - 200) * 0.5f - 25.0f, 10.0f, 0.0f));
                                      ++insert_count;
                                  }
                              });

    std::thread remove_thread([&]()
                              {
                                  for (size_t i = 0; i < 50; ++i)
                                  {
                                      octree->remove(i);
                                      ++remove_count;
                                  }
                              });

    insert_thread.join();
    remove_thread.join();

    // Assert: Final size is correct (100 - 50 + 50 = 100)
    EXPECT_EQ(octree->size(), 100);
    EXPECT_EQ(insert_count, 50);
    EXPECT_EQ(remove_count, 50);
}

// ============================================================================
// Stress Tests
// ============================================================================

/*!
 * @brief Stress test with many objects and many threads.
 *
 * @details Validates that the octree performs correctly under high load.
 * Tests subdivision, queries, and updates with realistic object counts.
 */
TEST_F(OctreeThreadingTest, stress_test_many_objects_many_threads)
{
    // Arrange: Insert many objects to trigger subdivisions
    constexpr size_t num_objects = 1000;
    for (size_t i = 0; i < num_objects; ++i)
    {
        float x = (i % 20) * 5.0f - 50.0f;
        float y = ((i / 20) % 20) * 5.0f - 50.0f;
        float z = (i / 400) * 5.0f - 5.0f;
        octree->insert(i, glm::vec3(x, y, z));
    }

    ASSERT_EQ(octree->size(), num_objects);

    // Act: Many threads perform queries
    constexpr size_t         num_threads        = 16;
    constexpr size_t         queries_per_thread = 50;
    std::vector<std::thread> threads;
    std::atomic<size_t>      total_results{ 0 };

    for (size_t i = 0; i < num_threads; ++i)
    {
        threads.emplace_back([&, thread_id = i]()
                             {
                                 std::mt19937                          rng(thread_id);
                                 std::uniform_real_distribution<float> dist(-50.0f, 50.0f);

                                 for (size_t j = 0; j < queries_per_thread; ++j)
                                 {
                                     glm::vec3 center(dist(rng), dist(rng), dist(rng));
                                     auto      results = octree->query_sphere(center, 20.0f);
                                     total_results += results.size();
                                 }
                             });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    // Assert: Queries completed (exact result count varies by randomness)
    EXPECT_GT(total_results, 0); // Should find some objects
}

/*!
 * @brief Tests synchronized start with std::barrier.
 *
 * @details Validates that threads can simultaneously hit the octree.
 * Uses C++20 std::barrier for coordinated thread start.
 */
TEST_F(OctreeThreadingTest, synchronized_start_stress_test)
{
    // Arrange: Insert objects
    for (size_t i = 0; i < 100; ++i)
    {
        octree->insert(i, glm::vec3(i * 2.0f, 0.0f, 0.0f));
    }

    constexpr size_t         num_threads = 8;
    std::barrier             sync_point(static_cast<std::ptrdiff_t>(num_threads));
    std::vector<std::thread> threads;
    std::atomic<size_t>      query_count{ 0 };

    // Act: Threads synchronize then query simultaneously
    for (size_t i = 0; i < num_threads; ++i)
    {
        threads.emplace_back([&]()
                             {
                                 sync_point.arrive_and_wait(); // All threads wait here

                                 // Now all threads query at the same time
                                 for (size_t j = 0; j < 100; ++j)
                                 {
                                     octree->query_sphere(glm::vec3(0.0f), 100.0f);
                                     ++query_count;
                                 }
                             });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    // Assert: All queries completed
    EXPECT_EQ(query_count, num_threads * 100);
}

// ============================================================================
// Update During Query Tests
// ============================================================================

/*!
 * @brief Tests clear operation during concurrent queries.
 *
 * @details Validates that clear() safely acquires exclusive lock
 * and queries see consistent state (either before or after clear).
 */
TEST_F(OctreeThreadingTest, clear_during_concurrent_queries)
{
    // Arrange: Insert objects
    for (size_t i = 0; i < 50; ++i)
    {
        octree->insert(i, glm::vec3(i * 2.0f, 0.0f, 0.0f));
    }

    std::atomic<bool>   cleared{ false };
    std::atomic<size_t> queries_before_clear{ 0 };
    std::atomic<size_t> queries_after_clear{ 0 };

    // Act: Query threads + clear thread
    std::vector<std::thread> query_threads;
    for (size_t i = 0; i < 4; ++i)
    {
        query_threads.emplace_back([&]()
                                   {
                                       for (size_t j = 0; j < 100; ++j)
                                       {
                                           auto results = octree->query_sphere(glm::vec3(0.0f), 200.0f);

                                           // Count queries based on results (before/after clear)
                                           if (!results.empty())
                                           {
                                               ++queries_before_clear;
                                           }
                                           else
                                           {
                                               ++queries_after_clear;
                                           }

                                           std::this_thread::sleep_for(std::chrono::microseconds(10));
                                       }
                                   });
    }

    std::thread clear_thread([&]()
                             {
                                 std::this_thread::sleep_for(std::chrono::milliseconds(10));
                                 octree->clear();
                                 cleared.store(true);
                             });

    clear_thread.join();
    for (auto& thread : query_threads)
    {
        thread.join();
    }

    // Assert: Clear completed, octree is empty
    EXPECT_TRUE(cleared.load());
    EXPECT_EQ(octree->size(), 0);
    EXPECT_GT(queries_before_clear, 0); // Some queries ran before clear
    EXPECT_GT(queries_after_clear, 0);  // Some queries ran after clear
}

/*!
 * @brief Tests remove operations during concurrent queries.
 *
 * @details Validates that remove() safely modifies tree structure
 * while queries are running. Query results should be consistent.
 */
TEST_F(OctreeThreadingTest, remove_during_concurrent_queries)
{
    // Arrange: Insert objects (within bounds [-100, 100])
    constexpr size_t num_objects = 100;
    for (size_t i = 0; i < num_objects; ++i)
    {
        octree->insert(i, glm::vec3(i * 0.5f - 25.0f, 0.0f, 0.0f));
    }

    std::atomic<size_t> query_count{ 0 };
    std::atomic<size_t> remove_count{ 0 };
    std::atomic<bool>   stop_queries{ false };

    // Act: Query threads + remove thread
    std::vector<std::thread> query_threads;
    for (size_t i = 0; i < 4; ++i)
    {
        query_threads.emplace_back([&]()
                                   {
                                       while (!stop_queries.load())
                                       {
                                           octree->query_sphere(glm::vec3(0.0f), 200.0f);
                                           ++query_count;
                                           std::this_thread::yield(); // CRITICAL: Yield to allow remove thread to acquire lock
                                       }
                                   });
    }

    std::thread remove_thread([&]()
                              {
                                  for (size_t i = 0; i < num_objects / 2; ++i)
                                  {
                                      octree->remove(i);
                                      ++remove_count;
                                  }
                                  stop_queries.store(true);
                              });

    remove_thread.join();
    for (auto& thread : query_threads)
    {
        thread.join();
    }

    // Assert: Half objects removed, queries ran many times
    EXPECT_EQ(octree->size(), num_objects / 2);
    EXPECT_EQ(remove_count, num_objects / 2);
    EXPECT_GT(query_count, 100);
}
