/*
 * Copyright (C) 2010-2022 The ESPResSo project
 * Copyright (C) 2002,2003,2004,2005,2006,2007,2008,2009,2010
 *   Max-Planck-Institute for Polymer Research, Theory Group
 *
 * This file is part of ESPResSo.
 *
 * ESPResSo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ESPResSo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/** \file
 *
 *  Implementation of \ref thole.hpp
 */
#include "thole.hpp"

#ifdef THOLE
#include "interactions.hpp"

#include <utils/constants.hpp>

int thole_set_params(int part_type_a, int part_type_b, double scaling_coeff,
                     double q1q2) {
  IA_parameters *data = get_ia_param_safe(part_type_a, part_type_b);

  if (!data)
    return ES_ERROR;

  data->thole.scaling_coeff = scaling_coeff;
  data->thole.q1q2 = q1q2;

  /* broadcast interaction parameters */
  mpi_bcast_ia_params(part_type_a, part_type_b);

  return ES_OK;
}
#endif
