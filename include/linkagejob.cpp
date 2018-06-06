/**
\file    linkagejob.cpp
\author  Tobias Kussel <kussel@cbs.tu-darmstadt.de>
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
\brief Holds information and data for one linkage job
*/

#include "linkagejob.h"
#include <chrono>
#include <exception>
#include <map>
#include <random>
#include <string>
#include <vector>
#include "fmt/format.h"
#include "localconfiguration.h"

void sel::LinkageJob::set_id() {
  const auto timestamp{std::chrono::system_clock::now().time_since_epoch()};
  m_id = std::to_string(
      std::chrono::duration_cast<std::chrono::milliseconds>(timestamp).count());
  std::random_shuffle(m_id.begin(), m_id.end());
}

sel::LinkageJob::LinkageJob() {
  set_id();
}

sel::LinkageJob::LinkageJob(std::shared_ptr<sel::LocalConfiguration> l_conf)
    : m_local_config(std::move(l_conf)) {
  set_id();
}

void sel::LinkageJob::run_job() {
  m_status = JobStatus::RUNNING;

  // Prepare Data, weights and exchange groups
  std::vector<sel::DataField> compare_data;
  compare_data.reserve(m_data.size());
  const std::vector<std::set<sel::FieldName>>& config_exchange_group{
      m_local_config->get_exchange_group()};
  std::vector<std::set<size_t>> exchange_groups;
  exchange_groups.resize(config_exchange_group.size());
  for (auto i = m_data.begin(); i != m_data.end(); ++i) {
    const size_t index{static_cast<size_t>(std::distance(m_data.begin(), i))};
    for (auto j = config_exchange_group.begin();
         j != config_exchange_group.end(); ++j) {
      if (j->count(i->first)) {
        const size_t groupindex{static_cast<size_t>(
            std::distance(config_exchange_group.begin(), j))};
        exchange_groups[groupindex].emplace(index);
      }
    }
    compare_data.emplace_back(std::move(i->second));
  }
  try {
    // Construct ABY Client
  } catch (const std::exception& e) {
    fmt::print(stderr, "Error running MPC Client: {}\n", e.what());
    m_status = JobStatus::FAULT;
  }
  m_status = JobStatus::DONE;
}

void sel::LinkageJob::set_local_config(
    std::shared_ptr<sel::LocalConfiguration> l_config) {
  m_local_config = std::move(l_config);
}
