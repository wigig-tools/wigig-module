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

#ifndef M2D_ANTENNA_H
#define M2D_ANTENNA_H

#include "abstract-antenna.h"
#include "ns3/vector.h"

namespace ns3 {

/**
 * \brief Antenna Measurement Object as a part of the antenna
 */
class M2D : public Object
{
public:
  static TypeId GetTypeId (void);

  M2D ();
  M2D(double angle, double gain);
  ~M2D ();

  double GetGain (void) const;
  void SetGain (double);

  double GetAngle (void) const;
  void SetAngle (double);

private:
  M2D (const M2D &o);
  M2D & operator = (const M2D &o);
  double m_angle;
  double m_gain;
};

/**
 * \brief Antenna functionality for wireless devices
 */
class Measured2DAntenna : public AbstractAntenna
{
public:
  static TypeId GetTypeId (void);

  Measured2DAntenna ();
  ~Measured2DAntenna ();

  double GetTxGainDbi (double azimuth, double elevation) const;
  double GetRxGainDbi (double azimuth, double elevation) const;

  double GetAzimuthAngle (void) const;
  void SetAzimuthAngle (double azimuth);

  double GetElevationAngle (void) const;
  void SetElevationAngle (double elevation);

  double GetBeamwidth (void) const;

  double GetMode (void) const;
  void SetMode (double);

private:
  Measured2DAntenna (const Measured2DAntenna &o);
  Measured2DAntenna & operator = (const Measured2DAntenna &o);

  double GetGain(double angle) const;

  double m_mode;
  double m_verticalBeamwidth;
  double m_elevation;
  double m_azimuth;
  std::vector<Ptr<M2D> > m_measurements;

};

} // namespace ns3

#endif /* M2D_ANTENNA_H */
