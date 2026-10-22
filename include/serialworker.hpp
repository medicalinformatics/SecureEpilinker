/**
 \file    serialworker.hpp
 \author  Sebastian Stammler <sebastian.stammler@cysec.de>
 \copyright SEL - Secure EpiLinker
      Copyright (C) 2018 Computational Biology & Simulation Group TU-Darmstadt
      This program is free software: you can redistribute it and/or modify
      it under the terms of the GNU Affero General Public License as published
      by the Free Software Foundation, either version 3 of the License, or
      (at your option) any later version.
      This program is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
      GNU Affero General Public License for more details.
      You should have received a copy of the GNU Affero General Public License
      along with this program. If not, see <http://www.gnu.org/licenses/>.
 \brief Thread-safe worker thread with serial job queue
*/

#ifndef SEL_SERIALWORKER_HPP
#define SEL_SERIALWORKER_HPP
#pragma once

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>

namespace sel {

/**
 * A serial worker thread
 *
 * Runs pushed jobs serially using the specified consumer.
 * Inspired by https://juanchopanzacpp.wordpress.com/2013/02/26/concurrent-queue-c11/
 */
template<typename T>
class SerialWorker {
  using JobConsumer = std::function<void (const std::shared_ptr<T>&)>;

public:
  explicit SerialWorker(JobConsumer job_consumer)
    : job_consumer_{std::move(job_consumer)},
      thread_{&SerialWorker<T>::worker_loop, this}
  {}

  ~SerialWorker() {
    interrupt();
    join();
  }

  void push(std::shared_ptr<T> job) {
    {
      std::lock_guard<std::mutex> lock{mutex_};
      if (interrupted_) {
        return;
      }
      queue_.push(std::move(job));
    }
    cond_.notify_one();
  }

  void interrupt() {
    {
      std::lock_guard<std::mutex> lock{mutex_};
      interrupted_ = true;
    }
    cond_.notify_all();
  }

  void join() {
    if (thread_.joinable()) {
      thread_.join();
    }
  }

  SerialWorker()=delete;
  SerialWorker(const SerialWorker&) = delete;
  SerialWorker& operator=(const SerialWorker&) = delete;

private:
  // The worker thread must be declared last so all state it accesses is
  // constructed before the thread starts.
  std::queue<std::shared_ptr<T>> queue_;
  std::mutex mutex_;
  std::condition_variable cond_;
  bool interrupted_{false};
  JobConsumer job_consumer_;
  std::thread thread_;

  void worker_loop() {
    while (true) {
      std::shared_ptr<T> job;
      {
        std::unique_lock<std::mutex> lock{mutex_};
        cond_.wait(lock, [this] {
          return interrupted_ || !queue_.empty();
        });
        if (interrupted_) {
          return;
        }
        job = std::move(queue_.front());
        queue_.pop();
      }

      // Jobs may enqueue more work, so do not hold the queue mutex while a
      // consumer runs.
      job_consumer_(job);
    }
  }
};

} /* end of namespace: sel */
#endif /* end of include guard: SEL_SERIALWORKER_HPP */
