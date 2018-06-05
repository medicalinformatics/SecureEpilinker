/**
 \file    sel.h
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
 \brief Secure 2-party computation implementation of the EpiLink record linkage
      algorithm using the ABY Framework
*/

#ifndef SEL_SEL_H_
#define SEL_SEL_H_
#pragma once

#include "abycore/aby/abyparty.h"

/**
 \param   role    role played by the program which can be server or client part.
 \param   address   IP Address
 \param   seclvl    Security level
 \param   nvals   Number of values
 \param   bitlen    Bit length of the inputs
 \param   nthreads  Number of threads
 \param   mt_alg    The algorithm for generation of multiplication triples
 \param   nfields   Number of fields per record
 \brief   Test the secure epilinker circuit with random data
 millionaire's problem
 */
int32_t test_sel_circuit(e_role role, char* address, uint16_t port, seclvl seclvl,
    uint32_t nvals, uint32_t bitlen, uint32_t nthreads, e_mt_gen_alg mt_alg,
    uint32_t nfields);

#endif /* SEL_SEL_H_ */
