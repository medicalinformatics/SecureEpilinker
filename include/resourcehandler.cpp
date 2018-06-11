/**
\file    resourcehandler.cpp
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

#include "resourcehandler.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "jsonmethodhandler.h"
#include "methodhandler.hpp"
#include "nlohmann/json.hpp"
#include "restbed"

using namespace std;
namespace sel {
ResourceHandler::ResourceHandler(const string& uri)
    : m_resource(make_shared<restbed::Resource>()) {
  m_resource->set_path(uri);
}

void ResourceHandler::add_method(shared_ptr<MethodHandler> method_handler) {
  m_methods.emplace_back(method_handler);
  m_resource->set_method_handler(
      method_handler->get_method(),
      bind(&MethodHandler::handle_method, method_handler, placeholders::_1));
}

void ResourceHandler::publish(restbed::Service& service) const {
  service.publish(m_resource);
}
void ResourceHandler::suppress(restbed::Service& service) const {
  service.suppress(m_resource);
}
}  // namespace sel
