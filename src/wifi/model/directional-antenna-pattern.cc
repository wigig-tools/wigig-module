/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */

#include "ns3/log.h"
#include "ns3/string.h"
#include "directional-antenna-pattern.h"
#include <fstream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DirectionalAntennaPattern");

NS_OBJECT_ENSURE_REGISTERED (DirectionalAntennaPattern);

TypeId
DirectionalAntennaPattern::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DirectionalAntennaPattern")
    .SetGroupName ("Wifi")
    .SetParent<DirectionalAntenna> ()
    .AddConstructor<DirectionalAntennaPattern> ()
    .AddAttribute ("FileName", "The name of the file which contains the radiation pattern for antenna.",
                   StringValue (""),
                   MakeStringAccessor (&DirectionalAntennaPattern::SetAntennaRadiationPattern),
                   MakeStringChecker ())
  ;
  return tid;
}

DirectionalAntennaPattern::DirectionalAntennaPattern ()
{
  NS_LOG_FUNCTION (this);
  m_antennas = 1;
  m_sectors = 1;
  m_omniAntenna = true;
}

DirectionalAntennaPattern::~DirectionalAntennaPattern ()
{
  NS_LOG_FUNCTION (this);
}

void
DirectionalAntennaPattern::SetAntennaRadiationPattern (std::string filename)
{
  m_fileName = filename;
  LoadPattern ();
}

void
DirectionalAntennaPattern::LoadPattern (void)
{
  NS_LOG_FUNCTION (this << "Loading Antenna radiation patterns file" << m_fileName);
  std::ifstream file;
  file.open (m_fileName.c_str (), std::ifstream::in);
  NS_ASSERT_MSG (file.good (), " Antenna radiation patterns file not found");
  std::string line;
  PatternIndex patternIndex = 0;
  uint16_t gainIndex = 0;
  GainVector gainVector;
  double gain;
  while (std::getline (file, line))
    {
      std::stringstream gainString (line);
      gainString >> gain;
      gainVector.push_back (gain);
      if (gainIndex < 360)
        {
          gainIndex++;
        }
      else
        {
          m_antennaPatterns[patternIndex] = gainVector;
          gainVector.clear ();
          patternIndex++;
          gainIndex = 0;
        }
    }
  /* For the last pattern sicne we exit the loop */
  m_antennaPatterns[patternIndex] = gainVector;
}

double
DirectionalAntennaPattern::GetTxGainDbi (double angle) const
{
  NS_LOG_FUNCTION (this << angle);
  return GetGainDbi (angle, m_txSectorId, m_txAntennaId);
}

double
DirectionalAntennaPattern::GetRxGainDbi (double angle) const
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
DirectionalAntennaPattern::IsPeerNodeInTheCurrentSector (double angle) const
{
  NS_LOG_FUNCTION (this << angle);
  return true;
}

double
DirectionalAntennaPattern::GetGainDbi (double angle, uint8_t sectorId, uint8_t antennaId) const
{
  NS_LOG_FUNCTION (this << angle << uint (sectorId) << uint (antennaId));
  double gain;
  if (angle < 0)
    {
      angle = 2 * M_PI + angle;
    }
  /* Convert from radian to Degree to get the index */
  uint32_t index = angle * 180/M_PI;
  /* Calculate the gain */
  AntennaPatternsCI iter = m_antennaPatterns.find (sectorId - 1);
  NS_ASSERT_MSG (iter != m_antennaPatterns.end (), "SectorID=" << uint (sectorId) << " does not exist");
  GainVector gainVector = iter->second;
  gain = gainVector[uint16_t (index)];
  NS_LOG_DEBUG ("angle=" << angle << ", gain=" << gain);
  return gain;
}

}
