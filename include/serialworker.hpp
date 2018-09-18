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

#include <memory>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

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
  SerialWorker(const JobConsumer& job_consumer) :
    thread_{&SerialWorker<T>::worker_loop, this, job_consumer}
  {}

  void push(std::shared_ptr<T> job) {
    std::unique_lock<std::mutex> mlock(mutex_);
    queue_.push(job);
    mlock.unlock();
    cond_.notify_one();
  }

  void interrupt() {
    interrupted = true;
  }

  void join() {
    thread_.join();
  }

  SerialWorker()=delete;
  SerialWorker(const SerialWorker&) = delete;
  SerialWorker& operator=(const SerialWorker&) = delete;

private:
  std::thread thread_;
  std::queue<std::shared_ptr<T>> queue_;
  std::mutex mutex_;
  std::condition_variable cond_;
  bool interrupted = false;

  void worker_loop(const JobConsumer job_consumer) {
    do {
      std::unique_lock<std::mutex> mlock(mutex_);
      while (queue_.empty()) {
        if (interrupted) return;
        cond_.wait(mlock);
      }
      if (interrupted) return;
      auto job = queue_.front();
      queue_.pop();

      job_consumer(job);

    } while(!interrupted);
  }
};

} /* end of namespace: sel */
#endif /* end of include guard: SEL_SERIALWORKER_HPP */
