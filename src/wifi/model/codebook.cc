/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "codebook.h"
#include "wifi-net-device.h"

#include <algorithm>
#include <fstream>
#include <string>
#include <numeric>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Codebook");

NS_OBJECT_ENSURE_REGISTERED (Codebook);

TypeId
Codebook::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Codebook")
    .SetGroupName ("Wifi")
    .SetParent<Object> ()
    .AddAttribute ("ActiveRFChainID", "The ID of the current active RF chain.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&Codebook::m_activeRFChainID),
                   MakeUintegerChecker<uint8_t> ())
  ;
  return tid;
}

Codebook::Codebook ()
  : m_activeRFChainID (1),
    m_communicationMode (SISO_MODE),
    m_currentSectorIndex (0),
    m_totalTxSectors (0),
    m_totalRxSectors (0),
    m_totalSectors (0),
    m_totalAntennas (0),
    m_beaconRandomization (false),
    m_btiSectorOffset (0)
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

void
Codebook::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);

  /* Activate the first phased antenna array */
  SetActiveRFChainID (m_activeRFChainID);

  /* Initialize each RF chain associated with this codebook and set the RF chain ID */
  for (auto const &rfChain : m_rfChainList)
    {
      rfChain.second->Initialize ();
      //// NINA ////
      rfChain.second->SetRfChainId (rfChain.first);
      //// NINA ////
    }

  /* Get the first antenna array in the list and set the active antenna to it. */
  m_bhiAntennaI = m_bhiAntennaList.find (m_activeRFChain->GetActiveAntennaID ());
  NS_ASSERT_MSG (m_bhiAntennaI != m_bhiAntennaList.end (), "Cannot find the specified AntennaID");
  m_beamformingSectorList = m_bhiAntennaI->second;

  /* At initialization stage, a DMG/EDMG STA should be in quasi-omni receiving mode */
  SetReceivingInQuasiOmniMode ();
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
  m_bhiAntennaList = sectors;
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
  AppendToSectorList (m_bhiAntennaList, antennaID, sectorID);
}

void
Codebook::RemoveBeaconingSector (AntennaID antennaID, SectorID sectorID)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (antennaID) << static_cast<uint16_t> (sectorID));
  Antenna2SectorListI iter = m_bhiAntennaList.find (antennaID);
  if (iter != m_bhiAntennaList.end ())
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
  return m_bhiAntennaI->second.size ();
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

void
Codebook::SetMimoBeamformingSectorList (SectorSweepType type, Mac48Address address, Antenna2SectorList &sectorList)
{
  if (type == TransmitSectorSweep)
    {
      m_txMimoCustomSectors[address] = sectorList;
    }
  else
    {
      m_rxMimoCustomSectors[address] = sectorList;
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
Codebook::SetDevice (Ptr<WifiNetDevice> device)
{
  m_device = device;
}

Ptr<WifiNetDevice>
Codebook::GetDevice (void) const
{
  return m_device;
}

void
Codebook::SetActiveTxAwvID (AWV_ID awvId)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (awvId));
  m_activeRFChain->SetActiveTxAwvID (awvId);
}

void
Codebook::SetActiveRxAwvID (AWV_ID awvId)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (awvId));
  m_activeRFChain->SetActiveRxAwvID (awvId);
}

void
Codebook::SetActiveTxSectorID (SectorID sectorID)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (sectorID));
  m_activeRFChain->SetActiveTxSectorID (sectorID);
}

void
Codebook::SetActiveRxSectorID (SectorID sectorID)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (sectorID));
  m_activeRFChain->SetActiveRxSectorID (sectorID);
}

void
Codebook::SetActiveTxSectorID (AntennaID antennaID, SectorID sectorID)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (antennaID) << static_cast<uint16_t> (sectorID));
  SetActiveRFChainID (m_antennaArrayList[antennaID]->rfChain->GetRfChainId ());
  m_activeRFChain->SetActiveTxSectorID (antennaID, sectorID);
}

void
Codebook::SetActiveRxSectorID (AntennaID antennaID, SectorID sectorID)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (antennaID) << static_cast<uint16_t> (sectorID));
  SetActiveRFChainID (m_antennaArrayList[antennaID]->rfChain->GetRfChainId ());
  m_activeRFChain->SetActiveRxSectorID (antennaID, sectorID);
}

AntennaID
Codebook::GetActiveAntennaID (void) const
{
  return m_activeRFChain->GetActiveAntennaID ();
}

SectorID
Codebook::GetActiveTxSectorID (void) const
{
  return m_activeRFChain->GetActiveTxSectorID ();
}

SectorID
Codebook::GetActiveRxSectorID (void) const
{
  return m_activeRFChain->GetActiveRxSectorID ();
}

void
Codebook::RandomizeBeacon (bool beaconRandomization)
{
  NS_LOG_FUNCTION (this << beaconRandomization);
  m_beaconRandomization = beaconRandomization;
}

void
Codebook::StartBTIAccessPeriod (void)
{
  NS_LOG_FUNCTION (this);
  SectorID sectorID;
  m_currentBFPhase = BHI_PHASE;
  if (m_beaconRandomization)
    {
      if (m_btiSectorOffset == m_beamformingSectorList.size ())
        {
          m_btiSectorOffset = 0;
        }
      m_currentSectorIndex = m_btiSectorOffset;
      sectorID = m_beamformingSectorList.at (m_currentSectorIndex);
      m_btiSectorOffset++;
    }
  else
    {
      sectorID = m_beamformingSectorList.front ();
      m_currentSectorIndex = 0;
    }
  /* Set the directional sector for transmission in BTI */
  SetActiveTxSectorID (m_bhiAntennaI->first, sectorID);
  /* Set the remaining number of sectors in the current BTI */
  m_remainingSectors = m_beamformingSectorList.size () - 1;
}

bool
Codebook::GetNextSectorInBTI (void)
{
  NS_LOG_FUNCTION (this);
  if (m_remainingSectors == 0)
    {
      /* The PCP/AP shall not change DMG antennas within a BTI. */
      m_bhiAntennaI++;
      if (m_bhiAntennaI == m_bhiAntennaList.end ())
        {
          m_bhiAntennaI = m_bhiAntennaList.begin ();
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
  return m_bhiAntennaList.size ();
}

uint16_t
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

//// NINA ////
uint16_t
Codebook::CountMimoNumberOfSectors (Antenna2SectorList sectorList, bool useAwv)
{
  uint16_t maxNumberOfSectors = 0;
  for (Antenna2SectorListI iter = sectorList.begin (); iter != sectorList.end (); iter++)
    {
      uint16_t numberOfSectors = 0;
      if (useAwv)
        {
          for (SectorIDListI it = iter->second.begin(); it != iter->second.end (); it++)
            if (GetNumberOfAWVs (iter->first, (*it)) != 0)
              numberOfSectors += GetNumberOfAWVs (iter->first, (*it));
          else
              numberOfSectors +=1;
        }
      else
        numberOfSectors = iter->second.size();
      if (numberOfSectors > maxNumberOfSectors)
        maxNumberOfSectors = numberOfSectors;
    }
  return maxNumberOfSectors;
}

uint16_t
Codebook::CountMimoNumberOfTxSubfields (Mac48Address peer)
{
  Antenna2SectorList *txSectorList = (Antenna2SectorList *) &m_txMimoCustomSectors.find (peer)->second;
  uint16_t numberOfSectors =  txSectorList->begin ()->second.size ();
  std::vector<uint16_t> numberOfAwsPerSector (numberOfSectors, 1);
  for (Antenna2SectorListI iter = txSectorList->begin (); iter != txSectorList->end (); iter++)
    {
      if (m_useAWVsMimoBft)
        {
          uint16_t counter = 0;
          for (SectorIDListI it = iter->second.begin(); it != iter->second.end (); it++)
            {
              if (GetNumberOfAWVs (iter->first, (*it)) != 0)
                numberOfAwsPerSector.at (counter) = numberOfAwsPerSector.at (counter) * GetNumberOfAWVs (iter->first, (*it));
              counter++;
            }
        }
    }
   return std::accumulate (numberOfAwsPerSector.begin (), numberOfAwsPerSector.end (), 0);
}
//// NINA ////

void
Codebook::InitiateABFT (Mac48Address address)
{
  NS_LOG_DEBUG (this << address);
  m_currentBFPhase = BHI_PHASE;
  m_bhiAntennaI = m_bhiAntennaList.begin ();
  m_beamformingSectorList = m_bhiAntennaI->second;
  SetActiveTxSectorID (m_bhiAntennaI->first, m_beamformingSectorList.front ());
  m_peerStation = address;
  m_currentSectorIndex = 0;
  m_sectorSweepType = TransmitSectorSweep;
  /* Set the remaining number of sectors in the current BTI */
  m_remainingSectors = m_beamformingSectorList.size () - 1;
}

bool
Codebook::GetNextSectorInABFT (void)
{
  NS_LOG_FUNCTION (this);
  if (m_remainingSectors == 0)
    {
      /* The PCP/AP shall not change DMG antennas within a A-BFT. */
      m_bhiAntennaI++;
      if (m_bhiAntennaI == m_bhiAntennaList.end ())
        {
          m_bhiAntennaI = m_bhiAntennaList.begin ();
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
  NS_LOG_DEBUG (this << address << static_cast<uint16_t> (type) << static_cast<uint16_t> (peerAntennas));
  BeamformingSectorListCI iter;
  if (type == TransmitSectorSweep)
    {
      /* Transmit Sector Sweep */
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
      SetActiveTxSectorID (m_beamformingAntenna->first, m_beamformingSectorList.front ());
    }
  else
    {
      /* Receive Sector Sweep */
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
      SetActiveRxSectorID (m_beamformingAntenna->first, m_beamformingSectorList.front ());
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
      /* Check if we have changed the current used Antenna */
      if (m_currentSectorIndex == m_beamformingSectorList.size ())
        {
          changeAntenna = true;
          m_beamformingAntenna++;
          /* This is a special case when we have multiple antennas */
          if (m_beamformingAntenna == m_currentSectorList->end ())
            {
              m_beamformingAntenna = m_currentSectorList->begin ();
            }
          m_beamformingSectorList = m_beamformingAntenna->second;
          m_currentSectorIndex = 0;
          NS_LOG_DEBUG ("Switch to the next AntennaID=" << uint16_t (m_beamformingAntenna->first));
        }
      else
        {
          changeAntenna = false;
        }

      if (m_sectorSweepType == TransmitSectorSweep)
        {
          SetActiveTxSectorID (m_beamformingAntenna->first, m_beamformingSectorList[m_currentSectorIndex]);
        }
      else
        {
          SetActiveRxSectorID (m_beamformingAntenna->first, m_beamformingSectorList[m_currentSectorIndex]);
        }

      m_remainingSectors--;
      return true;
    }
}

void
Codebook::SetReceivingInQuasiOmniMode (void)
{
  NS_LOG_FUNCTION (this);
  m_activeRFChain->SetReceivingInQuasiOmniMode ();
}

void
Codebook::SetReceivingInQuasiOmniMode (AntennaID antennaID)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (antennaID));
  m_activeRFChain->SetReceivingInQuasiOmniMode (antennaID);
}

void
Codebook::StartReceivingInQuasiOmniMode (void)
{
  NS_LOG_FUNCTION (this);
  m_activeRFChain->StartReceivingInQuasiOmniMode ();
}

bool
Codebook::SwitchToNextQuasiPattern (void)
{
  NS_LOG_FUNCTION (this);
  return m_activeRFChain->SwitchToNextQuasiPattern ();
}

void
Codebook::SetReceivingInDirectionalMode (void)
{
  NS_LOG_FUNCTION (this);
  m_activeRFChain->SetReceivingInDirectionalMode ();
}

void
Codebook::CopyCodebook (const Ptr<Codebook> codebook)
{
  NS_LOG_FUNCTION (this << codebook);
  /* Sectors Data */
  m_txBeamformingSectors = codebook->m_txBeamformingSectors;
  m_rxBeamformingSectors = codebook->m_rxBeamformingSectors;
  m_totalTxSectors = codebook->m_totalTxSectors;
  m_totalRxSectors = codebook->m_totalRxSectors;
  m_totalAntennas = codebook->m_totalAntennas;
  m_txCustomSectors = codebook->m_txCustomSectors;
  m_rxCustomSectors = codebook->m_rxCustomSectors;
  /**/
  m_activeRFChainID = codebook->m_activeRFChainID;
  /* BTI Variables */
  m_bhiAntennaList = codebook->m_bhiAntennaList;
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
Codebook::DeleteAWVs (AntennaID antennaID, SectorID sectorID)
{
  AntennaArrayListI iter = m_antennaArrayList.find (antennaID);
  if (iter != m_antennaArrayList.end ())
    {
      Ptr<PhasedAntennaArrayConfig> antennaConfig = StaticCast<PhasedAntennaArrayConfig> (iter->second);
      SectorListI sectorI = antennaConfig->sectorList.find (sectorID);
      if (sectorI != antennaConfig->sectorList.end ())
        {
          Ptr<SectorConfig> sectorConfig = StaticCast<SectorConfig> (sectorI->second);
          for (auto &awvIt : sectorConfig->awvList)
            {
              awvIt = 0;
            }
          sectorConfig->awvList.clear ();
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

AntennaArrayList
Codebook::GetActiveAntennaList (void) const
{
  return m_activeAntennaArrayList;
}

ActivePatternList
Codebook::GetActiveTxPatternList (void)
{
  ActivePatternList patternList;
  Ptr<RFChain> chain;
  for (auto &rfChain : m_activeRFChainList)
    {
      chain = m_rfChainList[rfChain.first];
      patternList.push_back (std::make_pair (rfChain.second, chain->GetTxPatternConfig ()));
    }
  return patternList;
}

ActivePatternList
Codebook::GetActiveRxPatternList (void)
{
  ActivePatternList patternList;
  Ptr<RFChain> chain;
  for (auto &rfChain : m_activeRFChainList)
    {
      chain = m_rfChainList[rfChain.first];
      patternList.push_back (std::make_pair (rfChain.second, chain->GetRxPatternConfig ()));
    }
  return patternList;
}

void
Codebook::SetCommunicationMode (CommunicationMode mode)
{
  NS_LOG_FUNCTION (this << mode);
  m_communicationMode = mode;
  /* If switching from MIMO to SISO mode reset all active RF chains to the default mode
   * (First antenna in the list active, Tx directionally to sector 1, Rx in quasi-omni mode)
   *  and then shut off all of them except RF chain 1 */
  if (mode == SISO_MODE)
    {
      SetActiveRFChainID (1);
    }
}

CommunicationMode
Codebook::GetCommunicationMode (void) const
{
  return m_communicationMode;
}

void
Codebook::ChangeAntennaOrientation (AntennaID antennaID, double psi, double theta, double phi)
{
  NS_LOG_FUNCTION (this << psi << theta << phi);
  AntennaArrayListI iter = m_antennaArrayList.find (antennaID);
  if (iter != m_antennaArrayList.end ())
    {
      Ptr<PhasedAntennaArrayConfig> antennaConfig = StaticCast<PhasedAntennaArrayConfig> (iter->second);
      antennaConfig->orientation.psi = DegreesToRadians (psi);
      antennaConfig->orientation.theta = DegreesToRadians (theta);
      antennaConfig->orientation.phi = DegreesToRadians (phi);
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
  m_activeRFChain->InitiateBRP (antennaID, sectorID, type);
}

void
Codebook::GetNextAWV (void)
{
  NS_LOG_FUNCTION (this);
  for (auto const &rfChain : m_activeRFChainList)
    {
      m_rfChainList[rfChain.first]->GetNextAWV ();
    }
}

void
Codebook::UseLastTxSector (void)
{
  NS_LOG_FUNCTION (this);
  for (auto const &rfChain : m_activeRFChainList)
    {
      m_rfChainList[rfChain.first]->UseLastTxSector ();
    }
}

void
Codebook::UseCustomAWV (BeamRefinementType type)
{
  NS_LOG_FUNCTION (this << type);
  for (auto const &rfChain : m_activeRFChainList)
    {
      m_rfChainList[rfChain.first]->UseCustomAWV (type);
    }
}

bool
Codebook::IsCustomAWVUsed (void) const
{
  return m_activeRFChain->IsCustomAWVUsed ();
}

uint8_t
Codebook::GetActiveTxPatternID (void) const
{
  return m_activeRFChain->GetActiveTxPatternID ();
}

uint8_t
Codebook::GetActiveRxPatternID (void) const
{
  return m_activeRFChain->GetActiveRxPatternID ();
}

MIMO_AWV_CONFIGURATION Codebook::GetActiveTxPatternIDs(void)
{
  MIMO_AWV_CONFIGURATION activeConfigs;
  for (auto const &rfChain : m_activeRFChainList)
    {
      AntennaID antenna = m_rfChainList[rfChain.first]->GetActiveAntennaID ();
      SectorID sector = m_rfChainList[rfChain.first]->GetActiveTxSectorID ();
      ANTENNA_CONFIGURATION config = std::make_pair (antenna, sector);
      AWV_ID awv = 255;
      if (m_rfChainList[rfChain.first]->IsCustomAWVUsed ())
        awv = m_rfChainList[rfChain.first]->GetActiveTxPatternID ();
     activeConfigs.push_back (std::make_pair (config, awv));
    }
  return activeConfigs;
}

MIMO_AWV_CONFIGURATION Codebook::GetActiveRxPatternIDs(void)
{
  MIMO_AWV_CONFIGURATION activeConfigs;
  for (auto const &rfChain : m_activeRFChainList)
    {
      AntennaID antenna = m_rfChainList[rfChain.first]->GetActiveAntennaID ();
      SectorID sector = m_rfChainList[rfChain.first]->GetActiveRxSectorID ();
      ANTENNA_CONFIGURATION config = std::make_pair (antenna, sector);
      AWV_ID awv = 255;
      if (m_rfChainList[rfChain.first]->IsCustomAWVUsed ())
        awv = m_rfChainList[rfChain.first]->GetActiveRxPatternID ();
     activeConfigs.push_back (std::make_pair (config, awv));
    }
  return activeConfigs;
}

bool
Codebook::IsQuasiOmniMode (void) const
{
  NS_LOG_FUNCTION (this);
  return m_activeRFChain->IsQuasiOmniMode ();
}

Orientation
Codebook::GetOrientation (AntennaID antennaId)
{
  return m_antennaArrayList[antennaId]->orientation;
}

void
Codebook::ActivateRFChain (RFChainID chainID)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (chainID));
  if (m_communicationMode == SISO_MODE)
    {
     for (auto &rfChain : m_activeRFChainList)
       m_rfChainList[rfChain.first]->Reset ();
      m_activeRFChainList.clear ();
    }
  ActiveRFChainListI it = m_activeRFChainList.find (chainID);
  if (it == m_activeRFChainList.end ())
    {
      m_activeRFChainList[chainID] = m_rfChainList[chainID]->GetActiveAntennaID ();
    }
  else
    {
      it->second = m_rfChainList[chainID]->GetActiveAntennaID ();;
    }
}

void
Codebook::SetActiveRFChainID (RFChainID chainID)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (chainID));
  RFChainListI iter = m_rfChainList.find (chainID);
  if (iter != m_rfChainList.end ())
    {
      m_activeRFChainID = chainID;
      m_activeRFChain = iter->second;
      ActivateRFChain (chainID);
    }
  else
    {
      NS_ABORT_MSG ("Cannot find the specified RF Chain ID=" << static_cast<uint16_t> (chainID));
    }
}

void
Codebook::SetActiveRFChainIDs (ActiveRFChainList &chainList)
{
  NS_LOG_FUNCTION (this);
  /* Check if the antennas correspond to the given RF chains */
  for (auto const &rfChain : chainList)
    {
      AntennaArrayListI it = m_antennaArrayList.find (rfChain.second);
      if (it != m_antennaArrayList.end ())
        {
          if (it->second->rfChain != m_rfChainList[rfChain.first])
            {
              NS_FATAL_ERROR ("Phased antenna array" << uint16_t (rfChain.first)
                              << " is not connected to RF Chain " << uint16_t (rfChain.first));
            }
        }
      else
        {
          NS_FATAL_ERROR ("Phased antenna array" << uint16_t (rfChain.first) << " does not exist");
        }
    }
  m_activeRFChainList = chainList;
}

RFChainID
Codebook::GetActiveRFChainID (void) const
{
  return m_activeRFChainID;
}

ActiveRFChainList
Codebook::GetActiveRFChainIDs (void) const
{
  return m_activeRFChainList;
}

AntennaList
Codebook::GetActiveAntennasIDs (void) const
{
  AntennaList antennaList;
  for (auto const &rfChain : m_activeRFChainList)
    {
      antennaList.push_back (rfChain.second);
    }
  return antennaList;
}

Ptr<PatternConfig>
Codebook::GetTxPatternConfig (void) const
{
  return m_activeRFChain->GetTxPatternConfig ();
}

Ptr<PatternConfig>
Codebook::GetRxPatternConfig (void) const
{
  return m_activeRFChain->GetRxPatternConfig ();
}

Ptr<PhasedAntennaArrayConfig>
Codebook::GetAntennaArrayConfig (void) const
{
  return m_activeRFChain->GetAntennaArrayConfig ();
}

uint8_t
Codebook::GetNumberOfActiveRFChains (void) const
{
  return m_activeRFChainList.size ();
}

uint8_t
Codebook::GetTotalNumberOfRFChains (void) const
{
  return m_rfChainList.size ();
}

void
Codebook::SetFirstAWV (void)
{
  NS_LOG_FUNCTION (this);
  m_firstAwvI = m_currentAwvI;
}

void
Codebook::UseFirstAWV (void)
{
  NS_LOG_FUNCTION (this);
  m_currentAwvI = m_firstAwvI;
}

void
Codebook::StartSectorRefinement (void)
{
  NS_LOG_FUNCTION (this);
  m_activeRFChain->StartSectorRefinement ();
}

void
Codebook::StartBeaconTraining (void)
{
  SetReceivingInDirectionalMode ();
  m_currentSectorList = &m_rxBeamformingSectors;
  m_beamformingAntenna = m_currentSectorList->begin ();
  m_beamformingSectorList = m_beamformingAntenna->second;
  SetActiveRxSectorID (m_beamformingAntenna->first, m_beamformingSectorList.front ());
  m_sectorSweepType = ReceiveSectorSweep;
  m_currentSectorIndex = 0;
  m_remainingSectors = CountNumberOfSectors (m_currentSectorList) - 1;
  StartSectorRefinement ();
}

uint8_t
Codebook::GetRemaingAWVCount (void) const
{
  return m_activeRFChain->GetRemaingAWVCount ();
}

//// NINA ////
uint8_t
Codebook::SetUpMimoBrpTxss (std::vector<AntennaID> antennaIds, Mac48Address peerStation)
{
  // Iterate over all antennas that we want to train
  m_mimoCombinations.clear ();
  m_txMimoCustomSectors.erase (peerStation);
  m_rxMimoCustomSectors.erase (peerStation);
  for (auto const &antennaId : antennaIds)
    {
      AntennaArrayListI it = m_antennaArrayList.find (antennaId);
      // If the Phased antenna exists
      if (it != m_antennaArrayList.end ())
        {
          // For the first antenna, create a new combination and add the first pair of RFChainId, AntennaID to be tested in the first BRP packet
          if (m_mimoCombinations.empty ())
            {
              ActiveRFChainList newCombination;
              newCombination[it->second->rfChain->GetRfChainId ()] = antennaId;
              m_mimoCombinations.push_back (newCombination);
            }
          else
            {
              std::vector<ActiveRFChainList> newCombinations;
              // Iterate over all combinations already created
              for (std::vector<ActiveRFChainList>::iterator it1 = m_mimoCombinations.begin ();
                   it1 != m_mimoCombinations.end (); it1++)
                {
                  ActiveRFChainListI iter = it1->find(it->second->rfChain->GetRfChainId ());
                  // If in this combination there is already an antenna connected to the same RF Chain, we need another combination (BRP Packet),
                  // since they can't both be tested at the same time
                  if ( iter != it1->end ())
                    {
                      ActiveRFChainList newCombination = (*it1);
                      newCombination[it->second->rfChain->GetRfChainId ()] = antennaId;
                      newCombinations.push_back (newCombination);
                    }
                  // Otherwise add this pair of RfChainId, AntennaId to the list of antennas that we want to test at the same time with one BRP packet
                  else
                    {
                      (*it1)[it->second->rfChain->GetRfChainId ()] = antennaId;
                    }
                }
              // Add all newly created combinations to the list
              for (auto const &newCombination : newCombinations)
                {
                  m_mimoCombinations.push_back (newCombination);
                }
            }
        }
      else
        {
          NS_FATAL_ERROR ("Phased antenna array" << uint16_t (antennaId) << " does not exist");
        }
    }
  return m_mimoCombinations.size ();
}

std::vector<AntennaID>
Codebook::GetTotalAntennaIdList (void)
{
  std::vector<AntennaID> antennaIds;
  for (auto const &antennaList : m_antennaArrayList)
    {
      antennaIds.push_back (antennaList.first);
    }
  return antennaIds;
}

std::vector<AntennaID>
Codebook::GetCurrentMimoAntennaIdList (void)
{
  std::vector<AntennaID> antennaIds;
  ActiveRFChainList currentMimoBrpTxssList = (*m_currentMimoCombination);
  for (auto const &rfChain : currentMimoBrpTxssList)
    {
      antennaIds.push_back (rfChain.second);
    }
  return antennaIds;
}

uint8_t
Codebook::GetNumberOfTrnSubfieldsForMimoBrpTxss (void)
{
  uint8_t numberOfTrnSubfields = 0;
  ActiveRFChainList currentMimoBrpTxssList = (*m_currentMimoCombination);
  for (auto const &rfChain : currentMimoBrpTxssList)
    {
       AntennaArrayListI it = m_antennaArrayList.find (rfChain.second);
       if (it != m_antennaArrayList.end ())
         {
           uint8_t numberOfSectors = it->second->sectorList.size ();
           if (numberOfSectors > numberOfTrnSubfields)
             numberOfTrnSubfields = numberOfSectors;
         }
       else
         {
           NS_FATAL_ERROR ("Phased antenna array" << uint16_t (rfChain.first) << " does not exist");
         }
    }
  NS_LOG_DEBUG ("Number of TRN Subfields for MIMO BRP TxSS=" << numberOfTrnSubfields);
  return numberOfTrnSubfields;
}

void
Codebook::StartMimoTx (void)
{
  NS_LOG_FUNCTION (this);
  SetActiveRFChainIDs ((*m_currentMimoCombination));
  SetCommunicationMode (MIMO_MODE);
}

void
Codebook::StartMimoRx (uint8_t combination)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (combination));
  SetActiveRFChainIDs ((m_mimoCombinations.at (combination)));
  SetCommunicationMode (MIMO_MODE);
}

void
Codebook::InitializeMimoSectorSweeping (Mac48Address address, SectorSweepType type, bool firstCombination)
{
  NS_LOG_FUNCTION (this << address << static_cast<uint16_t> (type));
  BeamformingSectorListCI iter;
  if (firstCombination)
    // Set up the initiator to the first MIMO BRP TXSS Combination in preparation for the first packet to be sent/received
    m_currentMimoCombination = m_mimoCombinations.begin ();
  else
    m_currentMimoCombination++;
  // Find the sector list to train for the given peer station - either the full list of sectors or a custom list (if it exists).
  if (type == TransmitSectorSweep)
    {
      /* Transmit Sector Sweep */
      iter = m_txMimoCustomSectors.find (address);
      Antenna2SectorList *fullSectorList;
      if (iter != m_txMimoCustomSectors.end ())
        {
          fullSectorList = (Antenna2SectorList *) &iter->second;
        }
      else
        {
          fullSectorList = &m_txBeamformingSectors;
        }
      m_currentSectorList = fullSectorList;
      for (auto const &rfChain : *m_currentMimoCombination)
        {
          m_rfChainList[rfChain.first]->SetUpMimoSectorSweeping (TransmitSectorSweep, (m_currentSectorList->find (rfChain.second)->second), m_useAWVsMimoBft);
        }
    }
  else
    {
      /* Receive Sector Sweep */
      iter = m_rxMimoCustomSectors.find (address);
      Antenna2SectorList *fullSectorList;
      if (iter != m_rxMimoCustomSectors.end ())
        {
          fullSectorList = (Antenna2SectorList *) &iter->second;
        }
      else
        {
          fullSectorList = &m_rxBeamformingSectors;
        }
      m_currentSectorList = fullSectorList;
      for (auto const &rfChain : *m_currentMimoCombination)
        {
          m_rfChainList[rfChain.first]->SetUpMimoSectorSweeping (ReceiveSectorSweep, (m_currentSectorList->find (rfChain.second)->second), m_useAWVsMimoBft);
        }
    }
  m_sectorSweepType = type;
  m_peerStation = address;
  m_remainingSectors = CountMimoNumberOfSectors (*m_currentSectorList, m_useAWVsMimoBft) - 1;
}

void
Codebook::GetMimoNextSector (bool firstSector)
{
  NS_LOG_FUNCTION (this << firstSector);
  NS_LOG_DEBUG ("Remaining Sectors=" << static_cast<uint16_t> (m_remainingSectors));
  for (auto const &rfChain : m_activeRFChainList)
    {
      m_rfChainList[rfChain.first]->GetNextSector (firstSector);
    }
  if (!firstSector)
    {
      m_remainingSectors--;
    }
}

void
Codebook::UseOldTxSector (void)
{
  NS_LOG_FUNCTION (this);
  for (auto const &rfChain : m_activeRFChainList)
    {
      m_rfChainList[rfChain.first]->UseOldTxSector ();
    }
}

uint16_t
Codebook::GetSMBT_Subfields (Mac48Address from, std::vector<AntennaID> antennaCandidates,
                             Antenna2SectorList txSectorCandidates, Antenna2SectorList rxSectorCandidates)
{
  NS_LOG_FUNCTION (this << from);
  // Set the antenna combination that we want to train
  m_mimoCombinations.clear ();
  ActiveRFChainList newCombination;
  for (auto const &antennaId : antennaCandidates )
    {
      AntennaArrayListI it = m_antennaArrayList.find (antennaId);
      // If the Phased antenna exists
      if (it != m_antennaArrayList.end ())
        {
           newCombination[it->second->rfChain->GetRfChainId ()] = antennaId;
        }
      else
        {
          NS_FATAL_ERROR ("Phased antenna array" << uint16_t (antennaId) << " does not exist");
        }
    }
  m_mimoCombinations.push_back (newCombination);
  // Set the list of sectors that we want to train for each antenna
  SetMimoBeamformingSectorList (TransmitSectorSweep, from, txSectorCandidates);
  SetMimoBeamformingSectorList (ReceiveSectorSweep, from, rxSectorCandidates);
  // Count the number of subfields that we want to train
  uint16_t numberOfSubfields = CountMimoNumberOfSectors (rxSectorCandidates, m_useAWVsMimoBft);
  return numberOfSubfields;
}

void
Codebook::GetMimoNextSectorWithCombinations (bool firstSector)
{
  NS_LOG_FUNCTION (this << firstSector);
  bool endOfList = true;
  for (auto const &rfChain : m_activeRFChainList)
    {
      endOfList = m_rfChainList[rfChain.first]->GetNextAwvWithCombinations (firstSector, endOfList);
    }
  if (endOfList)
    {
      for (auto const &rfChain : m_activeRFChainList)
        {
          m_rfChainList[rfChain.first]->GetNextSectorWithCombinations ();
        }
    }
}

void
Codebook::SetMimoFirstCombination (void)
{
  NS_LOG_FUNCTION (this);
  for (auto const &rfChain : m_activeRFChainList)
    {
      m_rfChainList[rfChain.first]->SetFirstAntennaConfiguration ();
    }
  m_remainingSectors = CountMimoNumberOfSectors (*m_currentSectorList, m_useAWVsMimoBft) - 1;
}

MIMO_AWV_CONFIGURATION
Codebook::GetMimoConfigFromTxAwvId (uint32_t index, Mac48Address peer)
{
  MIMO_AWV_CONFIGURATION bestCombination;
  Antenna2SectorList *txSectorList = (Antenna2SectorList *) &m_txMimoCustomSectors.find (peer)->second;
  uint16_t numberOfSectors =  txSectorList->begin ()->second.size ();
  std::vector<uint16_t> numberOfAwsPerSector (numberOfSectors, 1);
  if (m_useAWVsMimoBft)
    {
      for (Antenna2SectorListI iter = txSectorList->begin (); iter != txSectorList->end (); iter++)
        {
          uint16_t counter = 0;
          for (SectorIDListI it = iter->second.begin(); it != iter->second.end (); it++)
            {
              if (GetNumberOfAWVs (iter->first, (*it)) != 0)
                numberOfAwsPerSector.at (counter) = numberOfAwsPerSector.at (counter) * GetNumberOfAWVs (iter->first, (*it));
              counter++;
            }
        }
      uint32_t currentIndex = 0;
      uint32_t nextIndex = 1;
      for (uint16_t i = 0; i < numberOfAwsPerSector.size (); i++)
        {
          if (currentIndex + numberOfAwsPerSector.at (i) >= index)
            {
              index = index - currentIndex;
              for (Antenna2SectorListI iter = txSectorList->begin (); iter != txSectorList->end (); iter++)
                {
                  AntennaID antenna = iter->first;
                  SectorID sector = iter->second.at (i);
                  ANTENNA_CONFIGURATION config = std::make_pair(antenna,sector);
                  AWV_ID awv;
                  uint8_t awsPerSector = GetNumberOfAWVs (antenna, sector);
                  awv = ceil (static_cast<double> (index) / nextIndex);
                  if (awv > awsPerSector)
                    {
                      awv = awv % awsPerSector;
                      if (awv == 0)
                        awv = awsPerSector;
                    }
                  nextIndex = nextIndex * awsPerSector;
                  AWV_CONFIGURATION antennaConfig = std::make_pair(config, awv - 1);
                  bestCombination.push_back (antennaConfig);
                }
              break;
            }
          currentIndex = currentIndex + numberOfAwsPerSector.at (i);
        }
    }
  else
    {
      for (Antenna2SectorListI iter = txSectorList->begin (); iter != txSectorList->end (); iter++)
        {
          AntennaID antenna = iter->first;
          SectorID sector = iter->second.at (index - 1);
          ANTENNA_CONFIGURATION config = std::make_pair(antenna,sector);
          AWV_CONFIGURATION antennaConfig = std::make_pair(config, NO_AWV_ID);
          bestCombination.push_back (antennaConfig);
        }
    }
  return bestCombination;
}

MIMO_AWV_CONFIGURATION
Codebook::GetMimoConfigFromRxAwvId (std::map<RX_ANTENNA_ID, uint16_t> indices, Mac48Address peer)
{
  MIMO_AWV_CONFIGURATION bestCombination;
  BeamformingSectorListCI iter;
  iter = m_rxMimoCustomSectors.find (peer);
  Antenna2SectorList *fullSectorList;
  if (iter != m_rxMimoCustomSectors.end ())
    {
      fullSectorList = (Antenna2SectorList *) &iter->second;
    }
  else
    {
      fullSectorList = &m_rxBeamformingSectors;
    }
  Antenna2SectorList *rxSectorList = (Antenna2SectorList *) fullSectorList;
  uint16_t numberOfSectors =  rxSectorList->begin ()->second.size ();
  if (m_useAWVsMimoBft)
    {
      std::vector<uint16_t> numberOfAwsPerSector (numberOfSectors, 1);
      for (Antenna2SectorListI iter = rxSectorList->begin (); iter != rxSectorList->end (); iter++)
        {
          uint16_t counter = 0;
          for (SectorIDListI it = iter->second.begin(); it != iter->second.end (); it++)
            {
              if (GetNumberOfAWVs (iter->first, (*it)) != 0 && (numberOfAwsPerSector.at (counter) < GetNumberOfAWVs (iter->first, (*it))))
                numberOfAwsPerSector.at (counter) = GetNumberOfAWVs (iter->first, (*it));
              counter++;
            }
        }
      for (Antenna2SectorListI iter = rxSectorList->begin (); iter != rxSectorList->end (); iter++)
        {
          uint16_t currentIndex = 0;
          uint16_t oldIndex = 0;
          for (uint16_t i = 0; i < numberOfAwsPerSector.size (); i++)
            {
              oldIndex = currentIndex;
              currentIndex = currentIndex + numberOfAwsPerSector.at (i);
              if (currentIndex >= indices[iter->first])
                {
                  AntennaID antenna = iter->first;
                  SectorID sector = iter->second.at (i);
                  ANTENNA_CONFIGURATION config = std::make_pair(antenna,sector);
                  AWV_ID awv = indices[iter->first] - oldIndex - 1;
                  AWV_CONFIGURATION antennaConfig = std::make_pair(config, awv);
                  bestCombination.push_back (antennaConfig);
                  break;
                }
            }
        }
    }
  else
    {
      uint8_t index = 0;
      for (Antenna2SectorListI iter = rxSectorList->begin (); iter != rxSectorList->end (); iter++)
        {
          AntennaID antenna = iter->first;
          SectorID sector = iter->second.at (indices[iter->first] - 1);
          ANTENNA_CONFIGURATION config = std::make_pair(antenna,sector);
          AWV_CONFIGURATION antennaConfig = std::make_pair(config, NO_AWV_ID);
          bestCombination.push_back (antennaConfig);
          index++;
        }
    }
  return bestCombination;
}

void
Codebook::StartMuMimoInitiatorTXSS ()
{
  NS_LOG_DEBUG (this);
  /* Set up the current Sector list and antenna that we are training */
  m_currentSectorList = &m_txBeamformingSectors;
  m_beamformingAntenna = m_currentSectorList->begin ();
  m_beamformingSectorList = m_beamformingAntenna->second;
  /* Set the current antenna to the first sector to train */
  SetActiveTxSectorID (m_beamformingAntenna->first, m_beamformingSectorList.front ());
  /* Initialize parameters to keep track of the sweeping */
  m_sectorSweepType = TransmitSectorSweep;
  m_currentSectorIndex = 0;
  m_remainingSectors = CountNumberOfSectors (m_currentSectorList) - 1;
}

ANTENNA_CONFIGURATION
Codebook::GetAntennaConfigurationShortSSW (uint16_t cdown)
{
  Antenna2SectorList *testedSectors = &m_txBeamformingSectors;
  Antenna2SectorListI currentAntenna = testedSectors->begin ();
  SectorIDList currentSectors = currentAntenna->second;
  uint8_t currentSectorIndex = 0;
  uint16_t nSectors = CountNumberOfSectors (testedSectors) - 1 - cdown;
  for (uint8_t i = 0; i < nSectors; i++)
    {
      currentSectorIndex++;
      /* Check if we have changed the current used Antenna */
      if (currentSectorIndex == currentSectors.size ())
        {
          currentAntenna++;
          currentSectors = currentAntenna->second;
          currentSectorIndex = 0;
        }
    }
  ANTENNA_CONFIGURATION antennaConfig = std::make_pair(currentAntenna->first, currentSectors[currentSectorIndex]);
  return antennaConfig;
}

SectorID
Codebook::GetSectorIdMimoBrpTxss (AntennaID antenna, SectorID sector)
{
  uint8_t numberOfSectorsTrained = m_currentSectorList->find (antenna)->second.size ();
  if (sector > numberOfSectorsTrained)
    {
      sector = std::remainder (sector, numberOfSectorsTrained);
    }
  return sector;
}

//// NINA ////

void
Codebook::SetUpMuMimoSectorSweeping (Mac48Address own, std::vector<AntennaID> antennaCandidates,
                             Antenna2SectorList txSectorCandidates)
{
  NS_LOG_FUNCTION (this << own );
  // Set the antenna combination that we want to train
  m_mimoCombinations.clear ();
  ActiveRFChainList newCombination;
  for (auto const &antennaId : antennaCandidates )
    {
      AntennaArrayListI it = m_antennaArrayList.find (antennaId);
      // If the Phased antenna exists
      if (it != m_antennaArrayList.end ())
        {
           newCombination[it->second->rfChain->GetRfChainId ()] = antennaId;
        }
      else
        {
          NS_FATAL_ERROR ("Phased antenna array" << uint16_t (antennaId) << " does not exist");
        }
    }
  m_mimoCombinations.push_back (newCombination);
  // Set the list of sectors that we want to train for each antenna
  SetMimoBeamformingSectorList (TransmitSectorSweep, own, txSectorCandidates);
}

void
Codebook::SetUseAWVsMimoBft (bool useAwvs)
{
  NS_LOG_FUNCTION (this << useAwvs);
  m_useAWVsMimoBft = useAwvs;
}

uint8_t
Codebook::GetTotalNumberOfReceiveSectorsOrAWVs (void)
{
  if (!m_useAWVsMimoBft)
    return GetTotalNumberOfReceiveSectors ();
  else
    return CountMimoNumberOfSectors (m_rxBeamformingSectors, m_useAWVsMimoBft);
}

Antenna2SectorList
Codebook::GetRxSectorsList (void)
{
  return m_rxBeamformingSectors;
}

}
