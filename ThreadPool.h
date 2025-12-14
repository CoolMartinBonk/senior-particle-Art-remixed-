#pragma once
#include "GameConfig.h"
#include "SpatialGrid.h"
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

class ThreadPool {
public:
    ThreadPool(size_t num_threads) : jobs_in_progress(0), stop_flag(false) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers.emplace_back(&ThreadPool::worker_loop, this, i);
            thread_local_forces.emplace_back(TOTAL_PARTICLES, Vector2D());
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop_flag = true;
        }
        cv_job_ready.notify_all();
        for (std::thread& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    void dispatch_repulsion_calc(const std::vector<int>& keys, const SpatialGrid& grid) {
        if (keys.empty()) return;

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            this->grid_ptr = &grid;

            size_t num_workers = workers.size();
            size_t num_keys = keys.size();
            size_t active_workers = (num_keys < num_workers) ? num_keys : num_workers;

            jobs.assign(workers.size(), {});

            size_t keys_per_thread = num_keys / active_workers;
            size_t remainder = num_keys % active_workers;

            size_t current_key_idx = 0;
            for (size_t i = 0; i < active_workers; ++i) {
                size_t count = keys_per_thread + (i < remainder ? 1 : 0);
                for (size_t k = 0; k < count; ++k) {
                    jobs[i].push_back(keys[current_key_idx++]);
                }
            }

            jobs_in_progress = active_workers;
            generation++;
        }

        cv_job_ready.notify_all();
    }

    void wait() {
        std::unique_lock<std::mutex> lock(queue_mutex);
        cv_job_done.wait(lock, [this] {
            return jobs_in_progress == 0;
            });
    }

    void reduce_forces(std::vector<Vector2D>& main_forces) {
        for (const auto& local_f : thread_local_forces) {
            for (size_t i = 0; i < main_forces.size(); ++i) {
                main_forces[i].fx += local_f[i].fx;
                main_forces[i].fy += local_f[i].fy;
            }
        }
    }

private:
    void worker_loop(size_t thread_id) {
        size_t last_generation = 0;

        while (true) {
            std::vector<int> task_keys;
            {
                std::unique_lock<std::mutex> lock(queue_mutex);

                cv_job_ready.wait(lock, [this, last_generation] {
                    return generation != last_generation || stop_flag;
                    });

                if (stop_flag) return;

                if (generation != last_generation) {
                    last_generation = generation;
                    if (thread_id < jobs.size() && !jobs[thread_id].empty()) {
                        task_keys = jobs[thread_id];
                    }
                    else {
                        continue;
                    }
                }
                else {
                    continue;
                }
            }

            std::fill(thread_local_forces[thread_id].begin(), thread_local_forces[thread_id].end(), Vector2D());

            if (grid_ptr) {
                calculate_forces_for_keys(task_keys, *grid_ptr, thread_local_forces[thread_id]);
            }

            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                if (jobs_in_progress > 0) {
                    jobs_in_progress--;
                }
                if (jobs_in_progress == 0) {
                    cv_job_done.notify_one();
                }
            }
        }
    }

    std::vector<std::thread> workers;
    std::vector<std::vector<int>> jobs;
    std::vector<std::vector<Vector2D>> thread_local_forces;
    const SpatialGrid* grid_ptr = nullptr;

    std::mutex queue_mutex;
    std::condition_variable cv_job_ready;
    std::condition_variable cv_job_done;

    size_t jobs_in_progress = 0;
    size_t generation = 0;
    bool stop_flag = false;
};