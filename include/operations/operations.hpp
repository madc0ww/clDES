/*
 =========================================================================
 This file is part of clDES

 clDES: an OpenCL library for Discrete Event Systems computing.

 clDES is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 clDES is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with clDES.  If not, see <http://www.gnu.org/licenses/>.

 Copyright (c) 2018 - Adriano Mourao <adrianomourao@protonmail.com>
                      madc0ww @ [https://github.com/madc0ww]

 LacSED - Laboratório de Sistemas a Eventos Discretos
 Universidade Federal de Minas Gerais

 File: operations.hpp
 Description: Declaration of operation functions.
 =========================================================================
*/

#ifndef OPERATIONS_HPP
#define OPERATIONS_HPP

#include <CL/cl.hpp>
#include "constants.hpp"

namespace cldes {

class DESystem;

namespace op {

typedef struct StatesTuple {
    cl_uint x0;
    cl_uint x1;
} StatesTuple;

cldes::ScalarType GetTransitions(cldes::DESystem const &aSys);

cldes::ScalarType GetPrivateTransitions(cldes::DESystem const &aSysTarget,
                                        cldes::DESystem const &aSys);
cldes::DESystem Synchronize(cldes::DESystem const &aSys0,
                            cldes::DESystem const &aSys1);

StatesTuple *SynchronizeStage1(cldes::DESystem const &aSys0,
                               cldes::DESystem const &aSys1);

}  // namespace op
}  // namespace cldes
#endif  // DESYSTEM_HPP
