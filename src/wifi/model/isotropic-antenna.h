/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Daniel Halperin <dhalperi@cs.washington.edu>
 */

#ifndef ISOTROPIC_ANTENNA_H
#define ISOTROPIC_ANTENNA_H

#include "abstract-antenna.h"

namespace ns3 {

/**
 * \brief Antenna functionality for wireless devices
 *
 */
class IsotropicAntenna : public AbstractAntenna
{

public:
  static TypeId GetTypeId (void);

  IsotropicAntenna ();
  ~IsotropicAntenna ();

  double GetTxGainDbi (double azimuth, double elevation) const;
  double GetRxGainDbi (double azimuth, double elevation) const;
};

} // namespace ns3

#endif /* ISOTROPIC_ANTENNA_H */
