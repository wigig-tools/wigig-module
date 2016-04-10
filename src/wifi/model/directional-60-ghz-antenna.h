/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
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
 * Author: Hany Assasa <Hany.assasa@gmail.com>
 */
#ifndef DIRECTIONAL_60_GHZ_ANTENNA_H
#define DIRECTIONAL_60_GHZ_ANTENNA_H

#include "directional-antenna.h"

namespace ns3 {

/**
 * \brief Directional Antenna functionality for 60 GHz based on IEEE 802.15.3c Antenna Model.
 */
class Directional60GhzAntenna : public DirectionalAntenna
{
public:
  static TypeId GetTypeId (void);
  Directional60GhzAntenna (void);
  virtual ~Directional60GhzAntenna (void);

  double GetHalfPowerBeamWidth (void) const;
  double GetSideLobeGain (void) const;

  /* Virtual Functions */
  double GetTxGainDbi (double angle) const;
  double GetRxGainDbi (double angle) const;
  double GetMaxGainDbi (void) const;

  virtual bool IsPeerNodeInTheCurrentSector (double angle) const;

protected:
  double GetGainDbi (double angle, uint8_t sectorId, uint8_t antennaId) const;

};

} // namespace ns3

#endif /* DIRECTIONAL_60_GHZ_ANTENNA_H */
