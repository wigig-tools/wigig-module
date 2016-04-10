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
#include "ns3/log.h"
#include "directional-60-ghz-antenna.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Directional60GhzAntenna");

NS_OBJECT_ENSURE_REGISTERED (Directional60GhzAntenna);

TypeId
Directional60GhzAntenna::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Directional60GhzAntenna")
    .SetGroupName ("Wifi")
    .SetParent<DirectionalAntenna> ()
    .AddConstructor<Directional60GhzAntenna> ()
  ;
  return tid;
}

Directional60GhzAntenna::Directional60GhzAntenna ()
{
  NS_LOG_FUNCTION (this);
  m_boresight = 0;
  m_antennas = 1;
  m_sectors = 1;
  m_omniAntenna = true;
}

Directional60GhzAntenna::~Directional60GhzAntenna ()
{
  NS_LOG_FUNCTION (this);
}

double
Directional60GhzAntenna::GetTxGainDbi (double angle) const
{
  NS_LOG_FUNCTION (this << angle);
  return GetGainDbi (angle, m_txSectorId, m_txAntennaId);
}

double
Directional60GhzAntenna::GetRxGainDbi (double angle) const
{
  NS_LOG_FUNCTION (this << angle);
  if (m_omniAntenna)
    {
      return 1;
    }
  else
    {
      return GetGainDbi (angle, m_rxSectorId, m_rxAntennaId);
    }
}

bool
Directional60GhzAntenna::IsPeerNodeInTheCurrentSector (double angle) const
{
  NS_LOG_FUNCTION (this << angle);
  double virtualAngle;
  if (angle < 0)
    angle = 2 * M_PI + angle;
  virtualAngle = std::abs (angle - (m_angleOffset + m_mainLobeWidth * double (m_txSectorId - 1)));
  if ((0 <= virtualAngle) && (virtualAngle <= GetHalfPowerBeamWidth () / 2))
    {
      return true;
    }
  else
    {
      return false;
    }
}

double
Directional60GhzAntenna::GetGainDbi (double angle, uint8_t sectorId, uint8_t antennaId) const
{
  NS_LOG_FUNCTION (this << angle << sectorId << antennaId);
  double gain;
  if (angle < 0)
    angle = 2 * M_PI + angle;
  /* The virtual angle is to calculate where does the angle fall in */
  double virtualAngle;// = abs (angle - m_antennaAperature * (m_antennaId - 1) - m_mainLobeWidth * (m_sectorId - 1));
  virtualAngle = std::abs (angle - (m_angleOffset + m_mainLobeWidth * double (sectorId - 1)));
  if ((0 <= virtualAngle) && (virtualAngle <= GetHalfPowerBeamWidth () / 2))
    {
      gain = GetMaxGainDbi () - 3.01 * pow (2 * virtualAngle/GetHalfPowerBeamWidth (), 2);
    }
  else
    {
      gain = GetSideLobeGain ();
    }
  NS_LOG_DEBUG ("angle=" << angle << ", virtualAngle=" << virtualAngle << ", gain=" << gain);
  return gain;
}

double
Directional60GhzAntenna::GetMaxGainDbi (void) const
{
  NS_LOG_FUNCTION (this);
  double maxGain;
  maxGain = 10 * log10 (pow (1.6162/sin (GetHalfPowerBeamWidth () / 2), 2));
  return maxGain;
}

double
Directional60GhzAntenna::GetHalfPowerBeamWidth (void) const
{
  NS_LOG_FUNCTION (this);
  return m_mainLobeWidth/2.6;
}

double
Directional60GhzAntenna::GetSideLobeGain (void) const
{
  NS_LOG_FUNCTION (this);
  double sideLobeGain;
  sideLobeGain = -0.4111 * log(GetHalfPowerBeamWidth ()) - 10.597;
  return sideLobeGain;
}

}
