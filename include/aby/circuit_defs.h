/**
 \file    circuit_defs.h
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
 \brief type aliases relevant for SEL circuit code
*/

#ifndef SEL_CIRCUIT_DEFS_H
#define SEL_CIRCUIT_DEFS_H
#pragma once

#include <memory>
#include <vector>
class share; // forward declaration

namespace sel {

using share_p = std::shared_ptr<share>;
using v_share_p = std::vector<share_p>;

struct share_quot {
  share_p numerator;
  share_p denominator;
};

} // namespace sel
#endif /* end of include guard: SEL_CIRCUIT_DEFS_H */
