/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "codebook.h"

#include <algorithm>
#include <fstream>
#include <string>

namespace ns3 {

PatternConfig::~PatternConfig ()
{
}

NS_LOG_COMPONENT_DEFINE ("Codebook");

NS_OBJECT_ENSURE_REGISTERED (Codebook);

TypeId
Codebook::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Codebook")
    .SetGroupName ("Wifi")
    .SetParent<Object> ()
    .AddAttribute ("ActiveAntenneID", "The ID of the current active phased antenna array. With this antenna array we start the "
                   "BTI access period with.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&Codebook::m_antennaID),
                   MakeUintegerChecker<uint8_t> ())
    .AddTraceSource ("ActiveTxSectorID",
                     "Traced value for Active Tx Sectot Changes",
                     MakeTraceSourceAccessor (&Codebook::m_txSectorID),
                     "ns3::TracedValueCallback::Uint8")
  ;
  return tid;
}

Codebook::Codebook ()
  : m_totalTxSectors (0),
  m_totalRxSectors (0),
  m_totalSectors (0),
  m_totalAntennas (0),
  m_beaconRandomization (false),
  m_btiSectorOffset (0),
  m_currentSectorIndex (0)
{
  NS_LOG_FUNCTION (this);
}

Codebook::~Codebook ()
{
  NS_LOG_FUNCTION (this);
}

void
Codebook::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

std::string
Codebook::GetCodebookFileName (void) const
{
  return m_fileName;
}

uint8_t
Codebook::GetTotalNumberOfTransmitSectors (void) const
{
  return m_totalTxSectors;
}

uint8_t
Codebook::GetTotalNumberOfReceiveSectors (void) const
{
  return m_totalRxSectors;
}

uint8_t
Codebook::GetTotalNumberOfSectors (void) const
{
  return m_totalSectors;
}

uint8_t
Codebook::GetTotalNumberOfAntennas (void) const
{
  return m_totalAntennas;
}

void
Codebook::SetBeaconingSectors (Antenna2SectorList sectors)
{
  m_bhiAntennasList = sectors;
}

void
Codebook::AppendToSectorList (Antenna2SectorList &globalList, AntennaID antennaID, SectorID sectorID)
{
  Antenna2SectorListI iter = globalList.find (antennaID);
  if (iter != globalList.end ())
    {
      SectorIDList *sectorList = &(iter->second);
      sectorList->push_back (sectorID);
    }
  else
    {
      SectorIDList sectorList;
      sectorList.push_back (sectorID);
      globalList[antennaID] = sectorList;
    }
}

void
Codebook::AppendBeaconingSector (AntennaID antennaID, SectorID sectorID)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (antennaID) << static_cast<uint16_t> (sectorID));
  AppendToSectorList (m_bhiAntennasList, antennaID, sectorID);
}

void
Codebook::RemoveBeaconingSector (AntennaID antennaID, SectorID sectorID)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (antennaID) << static_cast<uint16_t> (sectorID));
  Antenna2SectorListI iter = m_bhiAntennasList.find (antennaID);
  if (iter != m_bhiAntennasList.end ())
    {
      SectorIDList *sectorIDList = &(iter->second);
      sectorIDList->erase (std::remove (sectorIDList->begin (), sectorIDList->end (), sectorID), sectorIDList->end ());
    }
  else
    {
      NS_ABORT_MSG ("Cannot find the specified antenna ID=" << static_cast<uint16_t> (antennaID));
    }
}

uint8_t
Codebook::GetNumberOfSectorsInBHI (void)
{
  return CountNumberOfSectors (&m_bhiAntennasList);
}

void
Codebook::SetGlobalBeamformingSectorList (SectorSweepType type, Antenna2SectorList &sectorList)
{
  if (type == TransmitSectorSweep)
    {
      m_txBeamformingSectors = sectorList;
    }
  else
    {
      m_rxBeamformingSectors = sectorList;
    }
}

void
Codebook::AppendBeamformingSector (SectorSweepType type, AntennaID antennaID, SectorID sectorID)
{
  NS_LOG_FUNCTION (this << static_cast<SectorSweepType> (type)
                        << static_cast<uint16_t> (antennaID) << static_cast<uint16_t> (sectorID));
  if (type == TransmitSectorSweep)
    {
      AppendToSectorList (m_txBeamformingSectors, antennaID, sectorID);
    }
  else
    {
      AppendToSectorList (m_rxBeamformingSectors, antennaID, sectorID);
    }
}

uint8_t
Codebook::GetNumberOfSectorsForBeamforming (SectorSweepType type)
{
  if (type == TransmitSectorSweep)
    {
      return CountNumberOfSectors (&m_txBeamformingSectors);
    }
  else
    {
      return CountNumberOfSectors (&m_rxBeamformingSectors);
    }
}

void
Codebook::SetBeamformingSectorList (SectorSweepType type, Mac48Address address, Antenna2SectorList &sectorList)
{
  if (type == TransmitSectorSweep)
    {
      m_txCustomSectors[address] = sectorList;
    }
  else
    {
      m_rxCustomSectors[address] = sectorList;
    }
}

uint8_t
Codebook::GetNumberOfSectorsForBeamforming (Mac48Address address, SectorSweepType type)
{
  if (type == TransmitSectorSweep)
    {
      return GetNumberOfSectors (address, m_txCustomSectors);
    }
  else
    {
      return GetNumberOfSectors (address, m_rxCustomSectors);
    }
}

uint8_t
Codebook::GetNumberOfSectors (Mac48Address address, BeamformingSectorList &list)
{
  BeamformingSectorListI iter = list.find (address);
  if (iter != list.end ())
    {
      return CountNumberOfSectors (&iter->second);
    }
  else
    {
      return 0;
    }
}

void
Codebook::SetActiveTxSectorID (SectorID sectorID, AntennaID antennaID)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (antennaID) << static_cast<uint16_t> (sectorID));
  m_antennaConfig = m_antennaArrayList[antennaID];
  m_antennaID = antennaID;
  m_txSectorID = sectorID;
  m_txPattern = m_antennaConfig->sectorList[sectorID];
  m_useAWV = false;
}

void
Codebook::SetActiveRxSectorID (SectorID sectorID, AntennaID antennaID)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (antennaID) << static_cast<uint16_t> (sectorID));
  m_antennaConfig = m_antennaArrayList[antennaID];
  m_antennaID = antennaID;
  m_rxSectorID = sectorID;
  m_rxPattern = m_antennaConfig->sectorList[sectorID];
  m_useAWV = false;
}

void
Codebook::SetActiveTxSectorID (SectorID sectorID)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (sectorID));
  m_txSectorID = sectorID;
  m_txPattern = m_antennaConfig->sectorList[sectorID];
  m_useAWV = false;
}

void
Codebook::SetActiveRxSectorID (SectorID sectorID)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (sectorID));
  m_rxSectorID = sectorID;
  m_rxPattern = m_antennaConfig->sectorList[sectorID];
  m_useAWV = false;
}

void
Codebook::SetActiveAntennaID (AntennaID antennaID)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (antennaID));
  m_antennaConfig = m_antennaArrayList[antennaID];
  m_antennaID = antennaID;
}

AntennaID
Codebook::GetActiveAntennaID (void) const
{
  return m_antennaID;
}

SectorID
Codebook::GetActiveTxSectorID (void) const
{
  return m_txSectorID;
}

SectorID
Codebook::GetActiveRxSectorID (void) const
{
  return m_rxSectorID;
}

void
Codebook::RandomizeBeacon (bool beaconRandomization)
{
  m_beaconRandomization = beaconRandomization;
}

void
Codebook::InitializeCodebook (void)
{
  NS_LOG_FUNCTION (this);
  m_bhiAntennaI = m_bhiAntennasList.find (m_antennaID);
  NS_ASSERT_MSG (m_bhiAntennaI != m_bhiAntennasList.end (), "Cannot find the specified AntennaID");
  m_beamformingSectorList = m_bhiAntennaI->second;
  SetActiveAntennaID (m_bhiAntennaI->first);
}

void
Codebook::StartBTIAccessPeriod (void)
{
  NS_LOG_FUNCTION (this);
  SectorID sector;
  m_currentBFPhase = BHI_PHASE;
  if (m_beaconRandomization)
    {
      if (m_btiSectorOffset == m_bhiAntennasList.size ())
        {
          m_btiSectorOffset = 0;
        }
      m_currentSectorIndex = m_btiSectorOffset;
      sector = m_beamformingSectorList.at (m_currentSectorIndex);
      m_btiSectorOffset++;
    }
  else
    {
      sector = m_beamformingSectorList.front ();
      m_currentSectorIndex = 0;
    }
  SetActiveTxSectorID (sector, m_bhiAntennaI->first);
  m_remainingSectors = m_beamformingSectorList.size () - 1;
}

bool
Codebook::GetNextSectorInBTI (void)
{
  NS_LOG_FUNCTION (this);
  if (m_remainingSectors == 0)
    {
      m_bhiAntennaI++;
      if (m_bhiAntennaI == m_bhiAntennasList.end ())
        {
          m_bhiAntennaI = m_bhiAntennasList.begin ();
        }
      m_beamformingSectorList = m_bhiAntennaI->second;
      return false;
    }
  else
    {
      m_currentSectorIndex++;
      if (m_beaconRandomization && (m_currentSectorIndex == m_beamformingSectorList.size ()))
        {
          m_currentSectorIndex = 0;
        }
      SetActiveTxSectorID (m_beamformingSectorList.at (m_currentSectorIndex));
      m_remainingSectors--;
      return true;
    }
}

uint8_t
Codebook::GetNumberOfBIs (void) const
{
  return m_bhiAntennasList.size ();
}

uint8_t
Codebook::GetRemaingSectorCount (void) const
{
  return m_remainingSectors;
}

uint8_t
Codebook::CountNumberOfSectors (Antenna2SectorList *sectorList)
{
  uint8_t totalSector = 0;
  for (Antenna2SectorListI iter = sectorList->begin (); iter != sectorList->end (); iter++)
    {
      totalSector += iter->second.size ();
    }
  return totalSector;
}

void
Codebook::InitiateABFT (Mac48Address address)
{
  NS_LOG_DEBUG (this << address);
  m_currentBFPhase = BHI_PHASE;
  m_bhiAntennaI = m_bhiAntennasList.begin ();
  m_beamformingSectorList = m_bhiAntennaI->second;
  SetActiveTxSectorID (m_beamformingSectorList.front (), m_bhiAntennaI->first);
  m_peerStation = address;
  m_currentSectorIndex = 0;
  m_sectorSweepType = TransmitSectorSweep;
  m_remainingSectors = m_beamformingSectorList.size () - 1;
}

bool
Codebook::GetNextSectorInABFT (void)
{
  NS_LOG_FUNCTION (this);
  if (m_remainingSectors == 0)
    {
      m_bhiAntennaI++;
      if (m_bhiAntennaI == m_bhiAntennasList.end ())
        {
          m_bhiAntennaI = m_bhiAntennasList.begin ();
        }
      m_beamformingSectorList = m_bhiAntennaI->second;
      return false;
    }
  else
    {
      m_currentSectorIndex++;
      SetActiveTxSectorID (m_beamformingSectorList.at (m_currentSectorIndex));
      m_remainingSectors--;
      return true;
    }
}

void
Codebook::StartSectorSweeping (Mac48Address address, SectorSweepType type, uint8_t peerAntennas)
{
  NS_LOG_DEBUG (this << address << static_cast<uint16_t> (type));
  BeamformingSectorListCI iter;
  if (type == TransmitSectorSweep)
    {
      iter = m_txCustomSectors.find (address);
      if (iter != m_txCustomSectors.end ())
        {
          m_currentSectorList = (Antenna2SectorList *) &iter->second;
        }
      else
        {
          m_currentSectorList = &m_txBeamformingSectors;
        }
      m_beamformingAntenna = m_currentSectorList->begin ();
      m_beamformingSectorList = m_beamformingAntenna->second;
      SetActiveTxSectorID (m_beamformingSectorList.front (), m_beamformingAntenna->first);
    }
  else
    {
      iter = m_rxCustomSectors.find (address);
      if (iter != m_rxCustomSectors.end ())
        {
          m_currentSectorList = (Antenna2SectorList *) &iter->second;
        }
      else
        {
          m_currentSectorList = &m_rxBeamformingSectors;
        }
      m_beamformingAntenna = m_currentSectorList->begin ();
      m_beamformingSectorList = m_beamformingAntenna->second;
      SetActiveRxSectorID (m_beamformingSectorList.front (), m_beamformingAntenna->first);
    }
  m_currentBFPhase = SLS_PHASE;
  m_sectorSweepType = type;
  m_peerStation = address;
  m_currentSectorIndex = 0;
  m_remainingSectors = CountNumberOfSectors (m_currentSectorList) * peerAntennas - 1;
}

bool
Codebook::GetNextSector (bool &changeAntenna)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("Remaining Sectors=" << static_cast<uint16_t> (m_remainingSectors));
  if (m_remainingSectors == 0)
    {
      changeAntenna = false;
      return false;
    }
  else
    {
      m_currentSectorIndex++;
      if (m_currentSectorIndex == m_beamformingSectorList.size ())
        {
          changeAntenna = true;
          m_beamformingAntenna++;
          if (m_beamformingAntenna == m_currentSectorList->end ())
            {
              m_beamformingAntenna = m_currentSectorList->begin ();
            }
          m_beamformingSectorList = m_beamformingAntenna->second;
          m_currentSectorIndex = 0;
        }
      else
        {
          changeAntenna = false;
        }

      if (m_sectorSweepType == TransmitSectorSweep)
        {
          SetActiveTxSectorID (m_beamformingSectorList[m_currentSectorIndex], m_beamformingAntenna->first);
        }
      else
        {
          SetActiveRxSectorID (m_beamformingSectorList[m_currentSectorIndex], m_beamformingAntenna->first);
        }

      m_remainingSectors--;
      return true;
    }
}

void
Codebook::SetReceivingInQuasiOmniMode (void)
{
  NS_LOG_FUNCTION (this);
  m_quasiOmniMode = true;
  m_useAWV = false;
  m_rxSectorID = 255;
}

void
Codebook::SetReceivingInQuasiOmniMode (AntennaID antennaID)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (antennaID));
  m_quasiOmniMode = true;
  m_useAWV = false;
  m_rxSectorID = 255;
  SetActiveAntennaID (antennaID);
}

void
Codebook::StartReceivingInQuasiOmniMode (void)
{
  NS_LOG_FUNCTION (this);
  m_quasiOmniMode = true;
  m_useAWV = false;
  m_quasiAntennaIter = m_bhiAntennasList.begin ();
  SetReceivingInQuasiOmniMode (m_quasiAntennaIter->first);
}

bool
Codebook::SwitchToNextQuasiPattern (void)
{
  NS_LOG_FUNCTION (this);
  m_quasiAntennaIter++;
  if (m_quasiAntennaIter == m_bhiAntennasList.end ())
    {
      m_quasiAntennaIter = m_bhiAntennasList.begin ();
      SetReceivingInQuasiOmniMode (m_quasiAntennaIter->first);
      return false;
    }
  else
    {
      SetReceivingInQuasiOmniMode (m_quasiAntennaIter->first);
      return true;
    }
}

void
Codebook::SetReceivingInDirectionalMode (void)
{
  NS_LOG_FUNCTION (this);
  m_quasiOmniMode = false;
}

void
Codebook::CopyCodebook (const Ptr<Codebook> codebook)
{
  m_antennaArrayList = codebook->m_antennaArrayList;
  m_txBeamformingSectors = codebook->m_txBeamformingSectors;
  m_rxBeamformingSectors = codebook->m_rxBeamformingSectors;
  m_totalTxSectors = codebook->m_totalTxSectors;
  m_totalRxSectors = codebook->m_totalRxSectors;
  m_totalAntennas = codebook->m_totalAntennas;
  m_txCustomSectors = codebook->m_txCustomSectors;
  m_rxCustomSectors = codebook->m_rxCustomSectors;
  m_bhiAntennasList = codebook->m_bhiAntennasList;
}

void
Codebook::AppendAWV (AntennaID antennaID, SectorID sectorID, Ptr<AWV_Config> awvConfig)
{
  AntennaArrayListI iter = m_antennaArrayList.find (antennaID);
  if (iter != m_antennaArrayList.end ())
    {
      Ptr<PhasedAntennaArrayConfig> antennaConfig = StaticCast<PhasedAntennaArrayConfig> (iter->second);
      SectorListI sectorI = antennaConfig->sectorList.find (sectorID);
      if (sectorI != antennaConfig->sectorList.end ())
        {
          Ptr<SectorConfig> sectorConfig = StaticCast<SectorConfig> (sectorI->second);
          sectorConfig->awvList.push_back (awvConfig);
        }
      else
        {
          NS_ABORT_MSG ("Cannot find the specified sector ID=" << static_cast<uint16_t> (sectorID));
        }
    }
  else
    {
      NS_ABORT_MSG ("Cannot find the specified antenna ID=" << static_cast<uint16_t> (antennaID));
    }
}

void
Codebook::ChangeAntennaOrientation (AntennaID antennaID, double azimuthOrientation, double elevationOrientation)
{
  NS_LOG_FUNCTION (this << azimuthOrientation << elevationOrientation);
  AntennaArrayListI iter = m_antennaArrayList.find (antennaID);
  if (iter != m_antennaArrayList.end ())
    {
      Ptr<PhasedAntennaArrayConfig> antennaConfig = StaticCast<PhasedAntennaArrayConfig> (iter->second);
      antennaConfig->azimuthOrientationDegree = azimuthOrientation;
      antennaConfig->elevationOrientationDegree = elevationOrientation;
    }
  else
    {
      NS_ABORT_MSG ("Cannot find the specified antenna ID=" << static_cast<uint16_t> (antennaID));
    }
}

uint8_t
Codebook::GetNumberOfAWVs (AntennaID antennaID, SectorID sectorID) const
{
  AntennaArrayListCI it = m_antennaArrayList.find (antennaID);
  if (it != m_antennaArrayList.end ())
    {
      Ptr<PhasedAntennaArrayConfig> config = StaticCast<PhasedAntennaArrayConfig> (it->second);
      SectorListCI sectorIt = config->sectorList.find (sectorID);
      if (sectorIt != config->sectorList.end ())
        {
          Ptr<SectorConfig> sectorConfig = StaticCast<SectorConfig> (sectorIt->second);
          return sectorConfig->awvList.size ();
        }
      else
        {
          NS_FATAL_ERROR ("Sector [" << sectorID << "] does not exist");
        }
    }
  else
    {
      NS_FATAL_ERROR ("Antenna [" << antennaID << "] does not exist");
    }
}

void
Codebook::InitiateBRP (AntennaID antennaID, SectorID sectorID, BeamRefinementType type)
{
  NS_LOG_FUNCTION (this << uint16_t (antennaID) << uint16_t (sectorID) << type);
  Ptr<PhasedAntennaArrayConfig> antennaConfig = StaticCast<PhasedAntennaArrayConfig> (m_antennaArrayList[antennaID]);
  Ptr<SectorConfig> sectorConfig = DynamicCast<SectorConfig> (antennaConfig->sectorList[sectorID]);
  NS_ASSERT_MSG (sectorConfig->awvList.size () > 0, "Cannot initiate BRP or BT, because we have 0 custom AWVs.");
  NS_ASSERT_MSG (sectorConfig->awvList.size () % 4 == 0, "The number of AWVs should be multiple of 4.");
  m_useAWV = true;
  m_currentAwvList = &sectorConfig->awvList;
  m_currentAwvI = m_currentAwvList->begin ();
  if (type == RefineTransmitSector)
    {
      m_txPattern = (*m_currentAwvI);
    }
}

bool
Codebook::GetNextAWV (void)
{
  NS_LOG_FUNCTION (this << m_currentAwvList->size ());
  m_currentAwvI++;
  if (m_currentAwvI == m_currentAwvList->end ())
    {
      m_currentAwvI = m_currentAwvList->begin ();
      m_txPattern = (*m_currentAwvI);
      return true;
    }
  else
    {
      m_txPattern = (*m_currentAwvI);
    }
  return false;
}

void
Codebook::UseLastTxSector (void)
{
  Ptr<SectorConfig> sectorConfig = DynamicCast<SectorConfig> (m_antennaConfig->sectorList[m_txSectorID]);
  m_txPattern = sectorConfig;
  m_useAWV = false;
}

void
Codebook::UseCustomAWV (void)
{
  m_txPattern = (*m_currentAwvI);
  m_useAWV = true;
}

bool
Codebook::IsCustomAWVUsed (void) const
{
  return m_useAWV;
}

uint8_t
Codebook::GetActiveTxPatternID (void) const
{
  if (!m_useAWV)
    {
      return GetActiveTxSectorID ();
    }
  else
    {
      return m_currentAwvI - m_currentAwvList->begin ();
    }
}

uint8_t
Codebook::GetActiveRxPatternID (void) const
{
  if (!m_useAWV)
    {
      return GetActiveRxSectorID ();
    }
  else
    {
      return m_currentAwvI - m_currentAwvList->begin ();
    }
}

bool
Codebook::GetReceivingMode (void) const
{
  NS_LOG_FUNCTION (this);
  return m_quasiOmniMode;
}

Orientation
Codebook::GetOrientation (uint8_t antennaId)
{
  return m_antennaArrayList[antennaId]->orientation;
}

}
