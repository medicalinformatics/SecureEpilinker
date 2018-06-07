#include "remoteconfiguration.h"
#include "util.h"

sel::RemoteConfiguration::RemoteConfiguration()
    : m_worker_thread(&sel::RemoteConfiguration::run_queued_jobs, this) {}
sel::RemoteConfiguration::RemoteConfiguration(sel::RemoteId c_id)
    : m_connection_id(std::move(c_id)), m_worker_thread(&sel::RemoteConfiguration::run_queued_jobs, this) {}

sel::RemoteConfiguration::~RemoteConfiguration() {
  m_worker_thread.join();
}

void sel::RemoteConfiguration::run_queued_jobs() {
  while (true) {
    if (!m_job_queue.empty()) {
      for (auto& job : m_job_queue) {
        if (job.second->get_status() == JobStatus::QUEUED) {
          job.second->run_job();
        }
        /*
        if(job.get_status() == JobStatus::FINISHED) {
          remove_job(job.get_id());
        }*/
      }
    }
  }
}

void sel::RemoteConfiguration::remove_job(const sel::JobId& j_id){
  std::lock_guard<std::mutex> lock(m_queue_mutex);
  auto job = m_job_queue.find(j_id);
  if(job!= m_job_queue.end()){
    m_job_queue.erase(job);}
}

uint16_t sel::RemoteConfiguration::get_remote_port() const {
  auto parts{sel::split(m_connection_profile.url, ':')};
  // {{192.168.1.1},{8080}}
  return std::stoi(parts.back());
}

std::string sel::RemoteConfiguration::get_remote_host() const {
  auto parts{sel::split(m_connection_profile.url,':')};
  // {{192.168.1.1},{8080}}
  return parts.front();
}
