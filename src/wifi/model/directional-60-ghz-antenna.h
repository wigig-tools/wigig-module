/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
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

  virtual bool IsPeerNodeInTheCurrentSector (double angle) const;

protected:
  double GetMaxGainDbi (void) const;
  double GetGainDbi (double angle, uint8_t sectorId, uint8_t antennaId) const;

};

} // namespace ns3

#endif /* DIRECTIONAL_60_GHZ_ANTENNA_H */
