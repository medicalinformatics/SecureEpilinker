/**
\file    resourcehandler.h
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
\brief Handles a REST ressource
*/

#ifndef SEL_RESOURCEHANDLER_H
#define SnEL_RESOURCEHANDLER_H
#pragma once

#include "nlohmann/json.hpp"
#include "restbed"
#include <memory>
#include <string>
#include <vector>

// Forward Declarations
namespace sel {
class Validator;
class MethodHandler;

class ResourceHandler {
  /**
   * Responsible for handling a REST resource
   */

 public:
  struct Method {
    std::string method;
    std::function<void(const std::shared_ptr<restbed::Session>)>& handler;
  };

  ResourceHandler() = default;
  explicit ResourceHandler(const std::string& uri);

  //void add_method(const std::string&, std::function<void(std::shared_ptr<restbed::Session>)>);
  void add_method(std::shared_ptr<sel::MethodHandler>);
  void publish(restbed::Service&) const;
  void suppress(restbed::Service&) const;

  const std::vector<std::shared_ptr<MethodHandler>>& get_methods() const { return m_methods; }

 private:
  std::vector<std::shared_ptr<MethodHandler> > m_methods;
  std::shared_ptr<restbed::Resource> m_resource;
};
}  // Namespace sel
#endif  // SEL_RESOURCEHANDLER_H
