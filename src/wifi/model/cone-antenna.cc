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

#include "ns3/log.h"
#include "ns3/double.h"
#include "cone-antenna.h"
#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ConeAntenna");

NS_OBJECT_ENSURE_REGISTERED (ConeAntenna);

/* Utility Functions */
static double
getAngleDiff (double a, double b)
{
  double d = std::abs (a - b);
  while (d > 2 * M_PI)
    d -= 2 * M_PI;
  if (d > M_PI)
    d = 2 * M_PI - d;
  return d;
}

TypeId
ConeAntenna::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ConeAntenna")
    .SetParent<AbstractAntenna> ()
    .AddConstructor<ConeAntenna> ()
    .AddAttribute ("Beamwidth",
                   "The beamwidth of this Cone antenna in radians.",
                   DoubleValue (2 * M_PI),
                   MakeDoubleAccessor (&ConeAntenna::SetBeamwidth,
                                       &ConeAntenna::GetBeamwidth),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Azimuth",
                   "The azimuth angle (XY-plane) in which this Cone antenna is pointed in radians.",
                   DoubleValue (0),
                   MakeDoubleAccessor (&ConeAntenna::SetAzimuthAngle,
                                       &ConeAntenna::GetAzimuthAngle),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Elevation",
                   "The elevation angle (Z-plane) in which this Cone antenna is pointed in radians.",
                   DoubleValue (0),
                   MakeDoubleAccessor (&ConeAntenna::SetElevationAngle,
                                       &ConeAntenna::GetElevationAngle),
                   MakeDoubleChecker<double> ())
  ;
  return tid;
}

ConeAntenna::ConeAntenna ()
  : m_gainDbi (0),
    m_beamwidth (2 * M_PI),
    m_azimuth (0),
    m_elevation (0)
{
  NS_LOG_FUNCTION (this);
}

ConeAntenna::~ConeAntenna ()
{
  NS_LOG_FUNCTION (this);
}

double
ConeAntenna::GetTxGainDbi (double azimuth, double elevation) const
{
  NS_LOG_FUNCTION (this << azimuth << elevation);
  if ((getAngleDiff (azimuth, m_azimuth) > m_beamwidth/2) || (getAngleDiff (elevation, m_elevation) > m_beamwidth/2))
    return -1000000;
  return m_gainDbi;
}

double
ConeAntenna::GetRxGainDbi (double azimuth, double elevation) const
{
  NS_LOG_FUNCTION (this << azimuth << elevation);
  if ((getAngleDiff(azimuth, m_azimuth) > m_beamwidth/2) || (getAngleDiff(elevation, m_elevation) > m_beamwidth/2))
    return -1000000;
  return m_gainDbi;
}

double
ConeAntenna::GetGainDbi (void) const
{
  NS_LOG_FUNCTION (m_gainDbi);
  return m_gainDbi;
}

void
ConeAntenna::SetGainDbi (double gainDbi)
{
  NS_LOG_FUNCTION (this << gainDbi);
  m_gainDbi = gainDbi;
  m_beamwidth = GainDbiToBeamwidth (gainDbi);
}

double
ConeAntenna::GetBeamwidth (void) const
{
  NS_LOG_FUNCTION (this);
  return m_beamwidth;
}

void
ConeAntenna::SetBeamwidth (double beamwidth)
{
  NS_LOG_FUNCTION (this << beamwidth);
  m_beamwidth = beamwidth;
  m_gainDbi = BeamwidthToGainDbi(beamwidth);
}

double
ConeAntenna::GetBeamwidthDegrees (void) const
{
  NS_LOG_FUNCTION (m_beamwidth * 180/M_PI);
  return m_beamwidth * 180/M_PI;
}

void
ConeAntenna::SetBeamwidthDegrees (double beamwidth)
{
  NS_LOG_FUNCTION (m_beamwidth);
  SetBeamwidth (beamwidth * M_PI/180);
}

double
ConeAntenna::GetAzimuthAngle (void) const
{
  NS_LOG_FUNCTION (this << m_azimuth);
  return m_azimuth;
}

void
ConeAntenna::SetAzimuthAngle (double azimuth)
{
  NS_LOG_FUNCTION (this << azimuth);
  m_azimuth = azimuth;
}

double
ConeAntenna::GetElevationAngle (void) const
{
  NS_LOG_FUNCTION (this << m_elevation);
  return m_elevation;
}

void
ConeAntenna::SetElevationAngle (double elevation)
{
  NS_LOG_FUNCTION (this << elevation);
  m_elevation = elevation;
}

double
ConeAntenna::GainDbiToBeamwidth (double gainDbi)
{
  double gain = pow (10, gainDbi/10);
  double solidAngle = 4 * M_PI / gain;
  return 2 * acos(1 - solidAngle / (2*M_PI));
}

double
ConeAntenna::BeamwidthToGainDbi (double beamwidth)
{
  double solidAngle = 2 * M_PI * (1 - cos(beamwidth/2));
  double gain = 4 * M_PI / solidAngle;
  return 10 * log10 (gain);
}

}
