/*
  Copyright (C) 2010-2018 The ESPResSo project
  Copyright (C) 2002,2003,2004,2005,2006,2007,2008,2009,2010
    Max-Planck-Institute for Polymer Research, Theory Group

  This file is part of ESPResSo.

  ESPResSo is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  ESPResSo is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef ANGLE_COSINE_H
#define ANGLE_COSINE_H
/** \file
 *  Routines to calculate the angle energy or/and and force
 *  for a particle triple using the potential described in
 *  @ref bondedIA_angle_cosine.
 */

#include "bonded_interaction_data.hpp"
#include "particle_data.hpp"

#include "angle_common.hpp"
#include "grid.hpp"
#include <tuple>

/** Set parameters for the angle potential. */
int angle_cosine_set_params(int bond_type, double bend, double phi0);

/************************************************************/

/** Compute the three-body angle interaction force.
 *  @param[in]  p_mid     Second/middle particle.
 *  @param[in]  p_left    First/left particle.
 *  @param[in]  p_right   Third/right particle.
 *  @param[in]  iaparams  Bonded parameters for the angle interaction.
 *  @param[out] f_mid     Force on @p p_mid.
 *  @param[out] f_left    Force on @p p_left.
 *  @retval 0
 */
inline int calc_angle_cosine_force(Particle const *p_mid,
                                   Particle const *p_left,
                                   Particle const *p_right,
                                   Bonded_ia_parameters const *iaparams,
                                   double f_mid[3], double f_left[3]) {

  auto forceFactor = [&iaparams](double const cos_phi) {
    auto const sin_phi = sqrt(1 - Utils::sqr(cos_phi));
    auto const cos_phi0 = iaparams->p.angle_cosine.cos_phi0;
    auto const sin_phi0 = iaparams->p.angle_cosine.sin_phi0;
    auto const K = iaparams->p.angle_cosine.bend;
    return K * (sin_phi0 * (cos_phi / sin_phi) + cos_phi0);
  };

  calc_angle_generic_force(p_mid->r.p, p_left->r.p, p_right->r.p, forceFactor,
                           f_mid, f_left, true);

  return 0;
}

/** Compute the three-body angle interaction force.
 *  @param[in]  p_mid     Second/middle particle.
 *  @param[in]  p_left    First/left particle.
 *  @param[in]  p_right   Third/right particle.
 *  @param[in]  iaparams  Bonded parameters for the angle interaction.
 *  @param[out] f_mid     Force on @p p_mid.
 *  @param[out] f_left    Force on @p p_left.
 *  @param[out] f_right   Force on @p p_right.
 */
inline void calc_angle_cosine_3body_forces(Particle const *p_mid,
                                           Particle const *p_left,
                                           Particle const *p_right,
                                           Bonded_ia_parameters const *iaparams,
                                           Vector3d &f_mid, Vector3d &f_left,
                                           Vector3d &f_right) {

  auto forceFactor = [&iaparams](double const cos_phi) {
    auto const sin_phi = sqrt(1 - Utils::sqr(cos_phi));
    auto const K = iaparams->p.angle_cosine.bend;
    auto const sin_phi0 = iaparams->p.angle_cosine.sin_phi0;
    auto const cos_phi0 = iaparams->p.angle_cosine.cos_phi0;
    // potential dependent term [dU/dphi = K * sin(phi - phi0)]
    // trig identity: sin(a - b) = sin(a)cos(b) - cos(a)sin(b)
    return K * (sin_phi * cos_phi0 - cos_phi * sin_phi0) / sin_phi;
  };

  std::tie(f_mid, f_left, f_right) = calc_angle_generic_3body_forces(
      p_mid->r.p, p_left->r.p, p_right->r.p, forceFactor, false);
}

/** Computes the three-body angle interaction energy.
 *  @param[in]  p_mid     Second/middle particle.
 *  @param[in]  p_left    First/left particle.
 *  @param[in]  p_right   Third/right particle.
 *  @param[in]  iaparams  Bonded parameters for the angle interaction.
 *  @param[out] _energy   Energy.
 *  @retval 0
 */
inline int angle_cosine_energy(Particle const *p_mid, Particle const *p_left,
                               Particle const *p_right,
                               Bonded_ia_parameters const *iaparams,
                               double *_energy) {
  auto const vectors =
      calc_vectors_and_cosine(p_mid->r.p, p_left->r.p, p_right->r.p, true);
  auto const cos_phi = std::get<4>(vectors);
  auto const sin_phi = sqrt(1 - Utils::sqr(cos_phi));
  auto const cos_phi0 = iaparams->p.angle_cosine.cos_phi0;
  auto const sin_phi0 = iaparams->p.angle_cosine.sin_phi0;
  auto const K = iaparams->p.angle_cosine.bend;
  *_energy = K * (cos_phi * cos_phi0 - sin_phi * sin_phi0 + 1);
  return 0;
}

#endif /* ANGLE_COSINE_H */
