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

#ifndef CONE_ANTENNA_H
#define CONE_ANTENNA_H

#include "abstract-antenna.h"

namespace ns3 {

/**
 * \brief Cone Antenna functionality for wireless devices
 */
class ConeAntenna : public AbstractAntenna {
public:
  static TypeId GetTypeId (void);

  ConeAntenna ();
  virtual ~ConeAntenna ();

  double GetTxGainDbi (double azimuth, double elevation) const;
  double GetRxGainDbi (double azimuth, double elevation) const;

  double GetGainDbi (void) const;
  void SetGainDbi (double gainDbi);

  double GetBeamwidth (void) const;
  void SetBeamwidth (double beamwidth);
  double GetBeamwidthDegrees (void) const;
  void SetBeamwidthDegrees (double beamwidth);

  double GetAzimuthAngle (void) const;
  void SetAzimuthAngle (double azimuth);

  double GetElevationAngle (void) const;
  void SetElevationAngle (double elevation);

  static double GainDbiToBeamwidth (double gainDbi);
  static double BeamwidthToGainDbi (double gainDbi);

private:
  double m_gainDbi;
  double m_beamwidth;
  double m_azimuth;
  double m_elevation;

};

} // namespace ns3

#endif /* CONE_ANTENNA_H */
