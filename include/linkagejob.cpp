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
#include "fmt/format.h"
#include <chrono>
#include <random>
#include <vector>
#include <unordered_map>
#include <string>
#include <exception>

sel::LinkageJob::LinkageJob() {
  const auto timestamp{std::chrono::system_clock::now().time_since_epoch()};
  m_id = std::to_string(
      std::chrono::duration_cast<std::chrono::milliseconds>(timestamp).count());
  std::random_shuffle(m_id.begin(), m_id.end());
}

void sel::LinkageJob::run_job() {
  m_status = JobStatus::RUNNING;
  std::vector<sel::DataField> compare_data;
  compare_data.reserve(m_data.size());
  for(auto& field : m_data) {
    compare_data.emplace_back(std::move(field.second));
  }
  try{
  // Construct ABY Client
    sel::run_aby(1);
  } catch (const std::exception& e) {
    fmt::print(stderr, "Error running MPC Client: {}\n", e.what());
    m_status = JobStatus::FAULT;
  }
  m_status = JobStatus::DONE;
}
