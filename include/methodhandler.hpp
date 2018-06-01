/**
\file    methodhandler.hpp
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
\brief Baseclass and fectory for REST request and data handling
*/

#ifndef SEL_METHODHANDLER_HPP
#define SEL_METHODHANDLER_HPP
#pragma once

#include "validator.h"
#include <memory>
#include <string>

// Forward Declarations
namespace restbed{
class Session;
} // namespace restbed

namespace sel {

class MethodHandler {
  /**
   * Abstract base class & factory for handling HTTP methods
   */
 public:
  explicit MethodHandler(const std::string& method) : m_method{method} {};
  explicit MethodHandler(const std::string& method, std::shared_ptr<Validator> validator) : m_method{method}, m_validator(validator) {};
  virtual void handle_method(std::shared_ptr<restbed::Session>) const = 0;
  void set_validator(std::shared_ptr<Validator> new_validator) {m_validator = new_validator;}
  void set_validator(Validator* new_validator) {m_validator = std::shared_ptr<Validator>(new_validator);}
  std::shared_ptr<Validator> get_validator() const {return m_validator;}
  const std::string& get_method() const {return m_method;}

  template <class T, typename... Args>
  static std::shared_ptr<MethodHandler> create_methodhandler(const std::string& method, Args... arguments) {
    return std::shared_ptr<MethodHandler>(new T(method, arguments ... ));
  }
 protected:
  virtual ~MethodHandler() = default;
  std::string m_method;
  std::shared_ptr<Validator> m_validator;
};

}  // Namespace sel
#endif  // SEL_METHODHANDLER_HPP
