/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2020 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#include "ns3/log.h"
#include "ns3/traced-value.h"
#include "rf-chain.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RFChain");

NS_OBJECT_ENSURE_REGISTERED (RFChain);

/**
 * We need this destructor for the
 */
PatternConfig::~PatternConfig ()
{
}

void
PhasedAntennaArrayConfig::SetQuasiOmniConfig (Ptr<PatternConfig> quasiPattern)
{
  m_quasiOmniConfig = quasiPattern;
}

TypeId
RFChain::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RFChain")
    .SetGroupName ("Wifi")
    .SetParent<Object> ()
    .AddTraceSource ("ActiveAntennaID",
                     "Trace source for tracking the active antenna ID",
                     MakeTraceSourceAccessor (&RFChain::m_antennaID),
                     "ns3::TracedValueCallback::Uint8")
    .AddTraceSource ("ActiveTxSectorID",
                     "Trace source for tracking the active Tx Sector ID",
                     MakeTraceSourceAccessor (&RFChain::m_txSectorID),
                     "ns3::TracedValueCallback::Uint8")
    .AddTraceSource ("ActiveRxSectorID",
                     "Trace source for tracking the active Rx Sector ID",
                     MakeTraceSourceAccessor (&RFChain::m_rxSectorID),
                     "ns3::TracedValueCallback::Uint8")
  ;
  return tid;
}

RFChain::RFChain ()
{
  NS_LOG_FUNCTION (this);
}

RFChain::~RFChain ()
{
  NS_LOG_FUNCTION (this);
}

void
RFChain::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

void
RFChain::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (m_antennaArrayList.size () > 0, "At least one antenna array must be connected to this RF chain.");
  Reset ();
}

void
RFChain::Reset (void)
{
  NS_LOG_FUNCTION (this);
  /* We activate the first antenna array in the RF chain */
  SetActiveAntennaID (m_antennaArrayList.begin ()->first);
  SetActiveTxSectorID (m_antennaConfig->sectorList.begin ()->first);
  /* Set reception in Quasi-omni pattern */
  SetReceivingInQuasiOmniMode ();
}

void
RFChain::ConnectPhasedAntennaArray (AntennaID antennaID, Ptr<PhasedAntennaArrayConfig> array)
{
  NS_ASSERT_MSG (!array->isConnected, "The antenna array is already connected to an RF Chain.");
  m_antennaArrayList[antennaID] = array;
  array->isConnected = true;
}

void
RFChain::SetActiveTxAwvID (AWV_ID awvId)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (awvId));
  m_useAWV = true;
  Ptr<PhasedAntennaArrayConfig> antennaConfig = StaticCast<PhasedAntennaArrayConfig> (m_antennaArrayList[m_antennaID]);
  Ptr<SectorConfig> sectorConfig = DynamicCast<SectorConfig> (antennaConfig->sectorList[m_txSectorID]);
  m_currentAwvList = &sectorConfig->awvList;
  m_currentAwvI = m_currentAwvList->begin () + awvId;
  m_txPattern = (*m_currentAwvI);
}

void
RFChain::SetActiveRxAwvID (AWV_ID awvId)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (awvId));
  m_useAWV = true;
  Ptr<PhasedAntennaArrayConfig> antennaConfig = StaticCast<PhasedAntennaArrayConfig> (m_antennaArrayList[m_antennaID]);
  Ptr<SectorConfig> sectorConfig = DynamicCast<SectorConfig> (antennaConfig->sectorList[m_rxSectorID]);
  m_currentAwvList = &sectorConfig->awvList;
  m_currentAwvI = m_currentAwvList->begin () + awvId;
  m_rxPattern = (*m_currentAwvI);
}

void
RFChain::SetActiveTxSectorID (SectorID sectorID)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (sectorID));
  m_txSectorID = sectorID;
  m_txPattern = m_antennaConfig->sectorList[sectorID];
  m_useAWV = false;
}

void
RFChain::SetActiveRxSectorID (SectorID sectorID)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (sectorID));
  m_rxSectorID = sectorID;
  m_rxPattern = m_antennaConfig->sectorList[sectorID];
  m_useAWV = false;
}

void
RFChain::SetActiveTxSectorID (AntennaID antennaID, SectorID sectorID)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (antennaID) << static_cast<uint16_t> (sectorID));
  m_antennaConfig = m_antennaArrayList[antennaID];
  m_antennaID = antennaID;
  m_txSectorID = sectorID;
  m_txPattern = m_antennaConfig->sectorList[sectorID];
  m_useAWV = false;
}

void
RFChain::SetActiveRxSectorID (AntennaID antennaID, SectorID sectorID)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (antennaID) << static_cast<uint16_t> (sectorID));
  m_antennaConfig = m_antennaArrayList[antennaID];
  m_antennaID = antennaID;
  m_rxSectorID = sectorID;
  m_rxPattern = m_antennaConfig->sectorList[sectorID];
  m_useAWV = false;
}

SectorID
RFChain::GetActiveTxSectorID (void) const
{
  return m_txSectorID;
}

SectorID
RFChain::GetActiveRxSectorID (void) const
{
  return m_rxSectorID;
}

AntennaID
RFChain::GetActiveAntennaID (void) const
{
  return m_antennaID;
}

uint8_t
RFChain::GetNumberOfAWVs (AntennaID antennaID, SectorID sectorID) const
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

uint8_t
RFChain::GetActiveTxPatternID (void) const
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
RFChain::GetActiveRxPatternID (void) const
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

Ptr<PatternConfig>
RFChain::GetTxPatternConfig (void) const
{
  return m_txPattern;
}

Ptr<PatternConfig>
RFChain::GetRxPatternConfig (void) const
{
  return m_rxPattern;
}

Ptr<PhasedAntennaArrayConfig>
RFChain::GetAntennaArrayConfig (void) const
{
  return m_antennaConfig;
}

bool
RFChain::GetNextAWV (void)
{
  NS_LOG_FUNCTION (this << m_currentAwvList->size ());
  m_currentAwvI++;
  m_remainingAwvs--;
  if (m_currentAwvI == m_currentAwvList->end ())
    {
      m_currentAwvI = m_currentAwvList->begin ();
      if (m_beamRefinmentType == RefineTransmitSector)
        {
          m_txPattern = (*m_currentAwvI);
        }
      else
        {
          m_rxPattern = (*m_currentAwvI);
        }
      return true;
    }
  else
    {
      if (m_beamRefinmentType == RefineTransmitSector)
        {
          m_txPattern = (*m_currentAwvI);
        }
      else
        {
          m_rxPattern = (*m_currentAwvI);
        }
      return true;
    }
  return false;
}

void
RFChain::UseLastTxSector (void)
{
  Ptr<SectorConfig> sectorConfig = DynamicCast<SectorConfig> (m_antennaConfig->sectorList[m_txSectorID]);
  m_txPattern = sectorConfig;
  m_useAWV = false;
}

void
RFChain::UseCustomAWV (BeamRefinementType type)
{
  Ptr<PhasedAntennaArrayConfig> antennaConfig = StaticCast<PhasedAntennaArrayConfig> (m_antennaArrayList[m_antennaID]);
  if (type == RefineTransmitSector)
    {
      Ptr<SectorConfig> sectorConfig = StaticCast<SectorConfig> (antennaConfig->sectorList[m_txSectorID]);
      m_currentAwvList = &sectorConfig->awvList;
      m_currentAwvI = m_currentAwvList->begin ();
      m_txPattern = (*m_currentAwvI);
    }
  else
    {
      Ptr<SectorConfig> sectorConfig = StaticCast<SectorConfig> (antennaConfig->sectorList[m_rxSectorID]);
      m_currentAwvList = &sectorConfig->awvList;
      m_currentAwvI = m_currentAwvList->begin ();
      m_rxPattern = (*m_currentAwvI);
    }
  m_beamRefinmentType = type;
  m_useAWV = true;
}

bool
RFChain::IsCustomAWVUsed (void) const
{
  return m_useAWV;
}

bool
RFChain::IsQuasiOmniMode (void) const
{
  NS_LOG_FUNCTION (this);
  return m_quasiOmniMode;
}

void
RFChain::SetReceivingInQuasiOmniMode (void)
{
  NS_LOG_FUNCTION (this);
  m_quasiOmniMode = true;
  m_useAWV = false;
  m_rxSectorID = 255;
  m_rxPattern = m_antennaConfig->m_quasiOmniConfig;
}

void
RFChain::SetReceivingInQuasiOmniMode (AntennaID antennaID)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (antennaID));
  m_quasiOmniMode = true;
  m_useAWV = false;
  m_rxSectorID = 255;
  m_rxPattern = m_antennaArrayList[antennaID]->m_quasiOmniConfig;
  SetActiveAntennaID (antennaID);
}

void
RFChain::StartReceivingInQuasiOmniMode (void)
{
  NS_LOG_FUNCTION (this);
  m_quasiOmniMode = true;
  m_useAWV = false;
  m_quasiAntennaIter = m_antennaArrayList.begin ();
  SetReceivingInQuasiOmniMode (m_quasiAntennaIter->first);
}

bool
RFChain::SwitchToNextQuasiPattern (void)
{
  NS_LOG_FUNCTION (this);
  m_quasiAntennaIter++;
  if (m_quasiAntennaIter == m_antennaArrayList.end ())
    {
      m_quasiAntennaIter = m_antennaArrayList.begin ();
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
RFChain::SetReceivingInDirectionalMode (void)
{
  NS_LOG_FUNCTION (this);
  m_quasiOmniMode = false;
}

void
RFChain::SetActiveAntennaID (AntennaID antennaID)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (antennaID));
  m_antennaConfig = m_antennaArrayList[antennaID];
  m_antennaID = antennaID;
}

void
RFChain::InitiateBRP (AntennaID antennaID, SectorID sectorID, BeamRefinementType type)
{
  NS_LOG_FUNCTION (this << uint16_t (antennaID) << uint16_t (sectorID) << type);
  Ptr<PhasedAntennaArrayConfig> antennaConfig = StaticCast<PhasedAntennaArrayConfig> (m_antennaArrayList[antennaID]);
  Ptr<SectorConfig> sectorConfig = DynamicCast<SectorConfig> (antennaConfig->sectorList[sectorID]);
  NS_ASSERT_MSG (sectorConfig->awvList.size () > 0, "Cannot initiate BRP or BT, because we have 0 custom AWVs.");
  NS_ASSERT_MSG (sectorConfig->awvList.size () % 4 == 0, "The number of AWVs should be multiple of 4.");
  m_useAWV = true;
  m_currentAwvList = &sectorConfig->awvList;
  m_currentAwvI = m_currentAwvList->begin ();
  m_beamRefinmentType = type;
  if (type == RefineTransmitSector)
    {
      m_txPattern = (*m_currentAwvI);
    }
  else
    {
      m_rxPattern = (*m_currentAwvI);
    }
}

void
RFChain::StartSectorRefinement (void)
{
  Ptr<PhasedAntennaArrayConfig> antennaConfig = StaticCast<PhasedAntennaArrayConfig> (m_antennaArrayList[m_antennaID]);
  Ptr<SectorConfig> sectorConfig = DynamicCast<SectorConfig> (antennaConfig->sectorList[m_rxSectorID]);
  if (sectorConfig->awvList.size () > 0)
    {
      m_useAWV = true;
      m_currentAwvList = &sectorConfig->awvList;
      m_currentAwvI = m_currentAwvList->begin ();
      m_beamRefinmentType = RefineReceiveSector;
      m_rxPattern = (*m_currentAwvI);
      m_remainingAwvs = m_currentAwvList->size () - 1;
      NS_LOG_DEBUG ("AWV Index=" << uint16_t (GetActiveRxPatternID ()));
    }
}

uint8_t
RFChain::GetRemaingAWVCount (void) const
{
  return m_remainingAwvs;
}

//// NINA ////
void
RFChain::SetRfChainId (RFChainID rfChainId)
{
  m_rfChainId = rfChainId;
}

RFChainID
RFChain::GetRfChainId (void) const
{
  return m_rfChainId;
}

void
RFChain::SetUpMimoSectorSweeping (SectorSweepType type, SectorIDList beamformingSectors, bool useAwv)
{
  NS_LOG_FUNCTION (this << type << useAwv);
  m_currentBeamformingSectors = beamformingSectors;
  m_currentSectorI = m_currentBeamformingSectors.begin();
  m_sectorSweepType = type;
  m_usingAWVs = useAwv;
  if (useAwv)
    {
      m_currentAwvList = &m_antennaConfig->sectorList[*m_currentBeamformingSectors.begin()]->awvList;
      if (!m_currentAwvList->empty ())
        m_currentAwvI = m_currentAwvList->begin ();
    }
}

void
RFChain::GetNextSector (bool firstSector)
{
  NS_LOG_FUNCTION (this << firstSector);
  if (firstSector)
    {
      if (m_sectorSweepType == TransmitSectorSweep)
        m_oldSectorId = GetActiveTxSectorID ();
      else
        m_oldSectorId = GetActiveRxSectorID ();
    }
  if (m_usingAWVs)
    {
      // If we are not sending the first AWV to be trained, move to the next one.
      if (!firstSector)
        m_currentAwvI++;
      // If at the end of the AWV list move on to the next sector.
      if (m_currentAwvI == m_currentAwvList->end ())
        {
          m_currentSectorI ++;
          // if at the end of the list of sectors start from the start again
          if (m_currentSectorI == m_currentBeamformingSectors.end ())
            m_currentSectorI = m_currentBeamformingSectors.begin ();
          m_currentAwvList = &m_antennaConfig->sectorList[*m_currentSectorI]->awvList;
          m_currentAwvI = m_currentAwvList->begin ();
        }
    }
  else
    {
      if (!firstSector)
        m_currentSectorI++;
      if (m_currentSectorI == m_currentBeamformingSectors.end ())
        m_currentSectorI = m_currentBeamformingSectors.begin ();
    }
  SetActiveMimoAntennaConfiguration ();
}

void
RFChain::UseOldTxSector (void)
{
  NS_LOG_FUNCTION (this);
  Ptr<SectorConfig> sectorConfig = DynamicCast<SectorConfig> (m_antennaConfig->sectorList[m_oldSectorId]);
  if (m_sectorSweepType == TransmitSectorSweep)
    {
      m_txPattern = sectorConfig;
      m_txSectorID = m_oldSectorId;
    }
  else
    {
      m_rxPattern = sectorConfig;
      m_rxSectorID = m_oldSectorId;
    }
  m_useAWV = false;
}

bool
RFChain::GetNextAwvWithCombinations (bool firstSector, bool switchAwv)
{
  NS_LOG_FUNCTION (this << firstSector << switchAwv);
  bool endOfList = false;
  if (firstSector)
    {
      if (m_sectorSweepType == TransmitSectorSweep)
        m_oldSectorId = GetActiveTxSectorID ();
      else
        m_oldSectorId = GetActiveRxSectorID ();
    }
  if (m_usingAWVs && switchAwv)
    {
      // If we are not sending the first sector to be trained, move to the next one.
      if (!firstSector)
        m_currentAwvI++;
      if (m_currentAwvI == m_currentAwvList->end ())
        {
          m_currentAwvI = m_currentAwvList->begin ();
          endOfList = true;
        }
      else
        endOfList = false;
    }
  else if (!m_usingAWVs)
    {
      if (!firstSector)
        m_currentSectorI++;
      if (m_currentSectorI == m_currentBeamformingSectors.end ())
        m_currentSectorI = m_currentBeamformingSectors.begin ();
    }
  SetActiveMimoAntennaConfiguration ();
  return endOfList;
}

void RFChain::GetNextSectorWithCombinations (void)
{
  NS_LOG_FUNCTION (this);
  if (m_usingAWVs)
    {
      m_currentSectorI ++;
      if (m_currentSectorI == m_currentBeamformingSectors.end ())
          m_currentSectorI = m_currentBeamformingSectors.begin ();
      m_currentAwvList = &m_antennaConfig->sectorList[*m_currentSectorI]->awvList;
      m_currentAwvI = m_currentAwvList->begin ();
    }
  SetActiveMimoAntennaConfiguration ();
}

void
RFChain::SetActiveMimoAntennaConfiguration (void)
{
  NS_LOG_FUNCTION (this);
  if (m_sectorSweepType == TransmitSectorSweep)
    {
      SetActiveTxSectorID (*m_currentSectorI);
      if (m_usingAWVs)
        {
          m_txPattern = (*m_currentAwvI);
          m_useAWV = true;
          NS_LOG_DEBUG ("AWV Index=" << uint16_t (GetActiveTxPatternID ()));
        }
    }
  else
    {
      SetActiveRxSectorID (*m_currentSectorI);
      if (m_usingAWVs)
        {
          m_rxPattern = (*m_currentAwvI);
          m_useAWV = true;
          NS_LOG_DEBUG ("AWV Index=" << uint16_t (GetActiveRxPatternID ()));
        }
    }
}

void
RFChain::SetFirstAntennaConfiguration (void)
{
  NS_LOG_FUNCTION (this);
  m_currentSectorI = m_currentBeamformingSectors.begin ();
  if (m_usingAWVs)
    {
      m_currentAwvList = &m_antennaConfig->sectorList[*m_currentSectorI]->awvList;
      m_currentAwvI = m_currentAwvList->begin ();
    }
  SetActiveMimoAntennaConfiguration ();
}
//// NINA ////

}

