/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */

#include "ns3/double.h"
#include "ns3/fatal-error.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "directional-antenna.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DirectionalAntenna");

NS_OBJECT_ENSURE_REGISTERED (DirectionalAntenna);

TypeId
DirectionalAntenna::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DirectionalAntenna")
    .SetParent<Object> ()
    .SetGroupName ("Wifi")
    .AddAttribute ("Antennas", "The number of antenna arrays.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&DirectionalAntenna::SetNumberOfAntennas,
                                         &DirectionalAntenna::GetNumberOfAntennas),
                   MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("Sectors", "The number of sectors per antenna.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&DirectionalAntenna::SetNumberOfSectors,
                                         &DirectionalAntenna::GetNumberOfSectors),
                   MakeUintegerChecker<uint8_t> ())
  ;
  return tid;
}

void
DirectionalAntenna::SetNumberOfAntennas (uint8_t antennas)
{
  NS_ASSERT (1 <= antennas && antennas <= 4);
  m_antennas = antennas;
  m_antennaAperature = 2 * M_PI/m_antennas;
  m_mainLobeWidth = 2 * M_PI/(m_antennas * m_sectors);
}

void
DirectionalAntenna::SetNumberOfSectors (uint8_t sectors)
{
  NS_ASSERT (1 <= sectors && sectors <= 127);
  m_sectors = sectors;
  m_mainLobeWidth = 2 * M_PI/(m_antennas * m_sectors);
}

void
DirectionalAntenna::SetCurrentTxSectorID (uint8_t sectorId)
{
  NS_ASSERT ((sectorId >= 1) && (sectorId <= 127));
  m_txSectorId = sectorId;
}

void
DirectionalAntenna::SetCurrentTxAntennaID (uint8_t antennaId)
{
  NS_ASSERT (1 <= antennaId && antennaId <= 4);
  m_txAntennaId = antennaId;
}

void
DirectionalAntenna::SetCurrentRxSectorID (uint8_t sectorId)
{
  NS_ASSERT ((sectorId >= 1) && (sectorId <= 127));
  m_rxSectorId = sectorId;
}

void
DirectionalAntenna::SetCurrentRxAntennaID (uint8_t antennaId)
{
  NS_ASSERT (1 <= antennaId && antennaId <= 4);
  m_rxAntennaId = antennaId;
}

uint8_t
DirectionalAntenna::GetNextTxSectorID (void) const
{
  uint8_t nextSector;
  if (m_txSectorId < m_sectors)
    {
      nextSector = m_txSectorId + 1;
    }
  else
    {
      nextSector = 1;
    }
  return nextSector;
}

uint8_t
DirectionalAntenna::GetNextRxSectorID (void) const
{
  uint8_t nextSector;
  if (m_rxSectorId < m_sectors)
    {
      nextSector = m_rxSectorId + 1;
    }
  else
    {
      nextSector = 1;
    }
  return nextSector;
}

double
DirectionalAntenna::GetAntennaAperature (void) const
{
  return m_antennaAperature;
}

double
DirectionalAntenna::GetMainLobeWidth (void) const
{
  return m_mainLobeWidth;
}

uint8_t
DirectionalAntenna::GetNumberOfAntennas (void) const
{
  return m_antennas;
}

uint8_t
DirectionalAntenna::GetNumberOfSectors (void) const
{
  return m_sectors;
}

uint8_t
DirectionalAntenna::GetCurrentTxSectorID (void) const
{
  return m_txSectorId;
}

uint8_t
DirectionalAntenna::GetCurrentTxAntennaID (void) const
{
  return m_txAntennaId;
}

uint8_t
DirectionalAntenna::GetCurrentRxSectorID (void) const
{
  return m_rxSectorId;
}

uint8_t
DirectionalAntenna::GetCurrentRxAntennaID (void) const
{
  return m_rxAntennaId;
}

void
DirectionalAntenna::SetInOmniReceivingMode (void)
{
  m_omniAntenna = true;
}

void
DirectionalAntenna::SetInDirectionalReceivingMode (void)
{
  m_omniAntenna = false;
}

}
