/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
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
      return 0;
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
  double lowerLimit, upperLimit;

  if (angle < 0)
    {
      angle = 2 * M_PI + angle;
    }

  lowerLimit = m_mainLobeWidth * double (m_txSectorId - 1);
  upperLimit = m_mainLobeWidth * double (m_txSectorId);

  if ((lowerLimit <= angle) && (angle <= upperLimit))
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
  double gain, lowerLimit, upperLimit, center;

  if (angle < 0)
    {
      angle = 2 * M_PI + angle;
    }

  /* Calculate both lower and upper limits of the current sector */
  lowerLimit = m_mainLobeWidth * double (sectorId - 1);
  upperLimit = m_mainLobeWidth * double (sectorId);
  center = m_mainLobeWidth * (sectorId - 1) + m_mainLobeWidth / 2;

  if (angle < 0)
    {
      angle = 2 * M_PI + angle;
    }
  angle = angle + m_mainLobeWidth/2;
  angle = fmod (angle, 2 * M_PI);

  if ((lowerLimit <= angle) && (angle <= upperLimit))
    {
      /* Calculate relative angle with respect to the current sector */
      double virtualAngle = std::abs (angle - center);
      gain = GetMaxGainDbi () - 3.01 * pow (2 * virtualAngle/GetHalfPowerBeamWidth (), 2);
      NS_LOG_DEBUG ("VirtualAngle=" << virtualAngle);
    }
  else
    {
      gain = GetSideLobeGain ();
    }

  NS_LOG_DEBUG ("Angle=" << angle << ", LowerLimit=" << lowerLimit << ", UpperLimit=" << upperLimit
                << ", MainLobeWidth=" << m_mainLobeWidth << ", Gain=" << gain);

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
