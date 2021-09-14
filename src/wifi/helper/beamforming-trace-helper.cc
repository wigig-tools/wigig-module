/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 HANY ASSASA
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
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#include "ns3/log.h"
#include "ns3/wifi-net-device.h"

#include "beamforming-trace-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BeamformingTraceHelper");

BeamformingTraceHelper::BeamformingTraceHelper (Ptr<QdPropagationEngine> qdPropagationEngine,
                                                std::string tracesFolder,
                                                std::string runNumber,
                                                NODE_ID_MAPPING mapping)
  : m_tracesFolder (tracesFolder),
    m_qdPropagationEngine (qdPropagationEngine),
    m_runNumber (runNumber),
    m_mapping (mapping)
{
  NS_LOG_FUNCTION (this << qdPropagationEngine << tracesFolder << runNumber << mapping );
}

BeamformingTraceHelper::~BeamformingTraceHelper (void)
{
}

void
BeamformingTraceHelper::ConnectTrace (Ptr<DmgWifiMac> wifiMac)
{
  NS_LOG_FUNCTION (this << wifiMac);
  if (m_mapping == NODE_ID_MAPPING_NS3_IDS)
    {
      map_Mac2ID[wifiMac->GetAddress ()] = wifiMac->GetDevice ()->GetNode ()->GetId ();
    }
  else if (m_mapping == NODE_ID_MAPPING_QD_CUSTOM_IDS)
    {
      map_Mac2ID[wifiMac->GetAddress ()] = m_qdPropagationEngine->GetQdID (wifiMac->GetDevice ()->GetNode ()->GetId ());
    }
  m_mapMac2Class[wifiMac->GetAddress ()] = wifiMac;
  DoConnectTrace (wifiMac);
}

void
BeamformingTraceHelper::ConnectTrace (const NetDeviceContainer &deviceConteiner)
{
  NS_LOG_FUNCTION (this);
  for (NetDeviceContainer::Iterator i = deviceConteiner.Begin (); i != deviceConteiner.End (); ++i)
    {
      ConnectTrace (StaticCast<DmgWifiMac> (StaticCast<WifiNetDevice> (*i)->GetMac ()));
    }
}

void
BeamformingTraceHelper::SetTracesFolder (std::string traceFolder)
{
    NS_LOG_FUNCTION (this << traceFolder);
    m_tracesFolder = traceFolder;
}

std::string
BeamformingTraceHelper::GetTracesFolder (void) const
{
    return m_tracesFolder;
}

Ptr<OutputStreamWrapper>
BeamformingTraceHelper::GetStreamWrapper (void) const
{
  return m_stream;
}

void
BeamformingTraceHelper::SetRunNumber (string runNumber)
{
  m_runNumber = runNumber;
}

std::string
BeamformingTraceHelper::GetRunNumber (void) const
{
  return m_runNumber;
}

/******************************************************************/

SlsBeamformingTraceHelper::SlsBeamformingTraceHelper (Ptr<QdPropagationEngine> qdPropagationEngine,
                                                      std::string tracesFolder,
                                                      std::string runNumber,
                                                      NODE_ID_MAPPING mapping)
  : BeamformingTraceHelper (qdPropagationEngine, tracesFolder, runNumber, mapping)
{
  NS_LOG_FUNCTION (this << qdPropagationEngine << tracesFolder << runNumber << mapping );
  DoGenerateTraceFiles ();
}

SlsBeamformingTraceHelper::~SlsBeamformingTraceHelper (void)
{
}

void
SlsBeamformingTraceHelper::DoGenerateTraceFiles (void)
{
  NS_LOG_FUNCTION (this);
  m_stream = m_ascii.CreateFileStream (m_tracesFolder + "sls_" + m_runNumber + ".csv");
  // Add MetaData?
  *m_stream->GetStream () << "TIME,TRACE_ID,SRC_ID,DST_ID,BFT_ID,ANTENNA_ID,SECTOR_ID,ROLE,BSS_ID,SINR_DB" << std::endl;
}

void
SlsBeamformingTraceHelper::DoConnectTrace (Ptr<DmgWifiMac> wifiMac)
{
  wifiMac->TraceConnectWithoutContext ("SLSCompleted",
                                       MakeBoundCallback (&SlsBeamformingTraceHelper::SLSCompleted, this, wifiMac));
}

void
SlsBeamformingTraceHelper::SLSCompleted (Ptr<SlsBeamformingTraceHelper> recorder,
                                         Ptr<DmgWifiMac> wifiMac,
                                         SlsCompletionAttrbitutes attributes)
{
  uint32_t srcID = recorder->map_Mac2ID[wifiMac->GetAddress ()];
  uint32_t dstID = recorder->map_Mac2ID[attributes.peerStation];
  uint32_t AP_ID = recorder->map_Mac2ID[wifiMac->GetBssid ()];
  double linkSnr =   RatioToDb (attributes.maxSnr);
  *recorder->m_stream->GetStream () << Simulator::Now ().GetNanoSeconds () << ","
                                    << recorder->m_qdPropagationEngine->GetCurrentTraceIndex () << ","
                                    << srcID << "," << dstID << "," << attributes.bftID << ","
                                    << uint16_t (attributes.antennaID - 1) << "," << uint16_t (attributes.sectorID - 1) << ","
                                    << wifiMac->GetTypeOfStation () << ","
                                    << AP_ID << "," << linkSnr << std::endl;
}

/******************************************************************/

SuMimoBeamformingTraceHelper::SuMimoBeamformingTraceHelper (Ptr<QdPropagationEngine> qdPropagationEngine,
                                                            std::string tracesFolder,
                                                            std::string runNumber,
                                                            NODE_ID_MAPPING mapping)
  : SlsBeamformingTraceHelper (qdPropagationEngine, tracesFolder, runNumber, mapping)
{
  NS_LOG_FUNCTION (this << qdPropagationEngine << tracesFolder << runNumber << mapping );
  DoGenerateTraceFiles ();
}

SuMimoBeamformingTraceHelper::~SuMimoBeamformingTraceHelper (void)
{
}

void
SuMimoBeamformingTraceHelper::DoGenerateTraceFiles (void)
{
  NS_LOG_FUNCTION (this);
  /** Create SU-MIMO Traces files **/

  /* 1. Create SU-MIMO SISO Phase Measurements Trace File */
  m_sisoPhaseMeasurements = m_ascii.CreateFileStream (m_tracesFolder + "SuMimoSisoPhaseMeasurements_" + m_runNumber + ".csv");
  *m_sisoPhaseMeasurements->GetStream () << "TIME,TRACE_ID,SRC_ID,DST_ID,BFT_ID,RX_ANTENNA_ID,TX_ANTENNA_ID,TX_SECTOR_ID,SINR_DB" << std::endl;

  /* 2. Create SU-MIMO SISO Feedback Measurements Trace File */
  m_sisoPhaseResults = m_ascii.CreateFileStream (m_tracesFolder + "SuMimoSisoPhaseResults_" + m_runNumber + ".csv");
  *m_sisoPhaseResults->GetStream () << "TIME,TRACE_ID,SRC_ID,DST_ID,RX_ANTENNA_ID,TX_ANTENNA_ID,TX_SECTOR_ID,SINR_DB" << std::endl;
}

void
SuMimoBeamformingTraceHelper::DoConnectTrace (Ptr<DmgWifiMac> wifiMac)
{
  NS_LOG_FUNCTION (this << wifiMac);
  SlsBeamformingTraceHelper::DoConnectTrace (wifiMac);

  /* Connect SU-MIMO traces */
  wifiMac->TraceConnectWithoutContext ("SuMimoSisoPhaseMeasurements",
                                       MakeBoundCallback (&SuMimoBeamformingTraceHelper::SuMimoSisoPhaseMeasurements, this, wifiMac));
  wifiMac->TraceConnectWithoutContext ("SuMimoSisoPhaseCompleted",
                                       MakeBoundCallback (&SuMimoBeamformingTraceHelper::SuMimoSisoPhaseCompleted, this, wifiMac));
  wifiMac->TraceConnectWithoutContext ("SuMimoMimoCandidatesSelected",
                                       MakeBoundCallback (&SuMimoBeamformingTraceHelper::SuMimoMimoCandidatesSelected, this, wifiMac));
  wifiMac->TraceConnectWithoutContext ("SuMimoMimoPhaseMeasurements",
                                       MakeBoundCallback (&SuMimoBeamformingTraceHelper::SuMimoMimoPhaseMeasurements, this, wifiMac));
}

void
SuMimoBeamformingTraceHelper::SuMimoSisoPhaseMeasurements (Ptr<SuMimoBeamformingTraceHelper> recorder,
                                                           Ptr<DmgWifiMac> wifiMac,
                                                           Mac48Address from, SU_MIMO_SNR_MAP measurementsMap, uint8_t edmgTrnN, uint16_t bftID)
{
  uint32_t srcID = recorder->map_Mac2ID[wifiMac->GetAddress ()];
  uint32_t dstID = recorder->map_Mac2ID[from];
  SNR_LIST_ITERATOR start;
  for (SU_MIMO_SNR_MAP::iterator it = measurementsMap.begin (); it != measurementsMap.end (); it++)
    {
      start = it->second.begin();
      SNR_LIST_ITERATOR snrIt = it->second.begin ();
      while (snrIt != it->second.end ())
        {
          uint32_t awv = std::distance(start,snrIt);
          *recorder->m_sisoPhaseMeasurements->GetStream () << Simulator::Now ().GetNanoSeconds () << ","
                                                           << recorder->m_qdPropagationEngine->GetCurrentTraceIndex () << ","
                                                           << srcID << "," << dstID << "," << bftID << ","
                                                           << uint16_t (std::get<1> (it->first) - 1) << "," << uint16_t (std::get<2> (it->first) - 1) << ","
                                                           << uint16_t (awv / edmgTrnN) << "," <<  RatioToDb (*snrIt)  << std::endl;
          snrIt++;
        }
    }
}

void
SuMimoBeamformingTraceHelper::SuMimoSisoPhaseCompleted (Ptr<SuMimoBeamformingTraceHelper> recorder, Ptr<DmgWifiMac> wifiMac,
                                                        Mac48Address from, MIMO_FEEDBACK_MAP feedbackMap,
                                                        uint8_t numberOfTxAntennas, uint8_t numberOfRxAntennas, uint16_t bftID)
{
  uint32_t srcID = recorder->map_Mac2ID[wifiMac->GetAddress ()];
  uint32_t dstID = recorder->map_Mac2ID[from];
  for (MIMO_FEEDBACK_MAP::iterator it = feedbackMap.begin (); it != feedbackMap.end (); it++)
    {
      *recorder->m_sisoPhaseResults->GetStream () << Simulator::Now ().GetNanoSeconds () << ","
                                                  << recorder->m_qdPropagationEngine->GetCurrentTraceIndex () << ","
                                                  << srcID << "," << dstID << "," << bftID << ","
                                                  << uint16_t (std::get<1> (it->first) - 1) << ","
                                                  << uint16_t (std::get<0> (it->first) - 1) << "," << uint16_t (std::get<2> (it->first) - 1)
                                                  << "," <<  RatioToDb ((it->second)) << "," << std::endl;
    }
}

//TODO: Check how can add time instead of attaching it to each candidate
void
SuMimoBeamformingTraceHelper::SuMimoMimoCandidatesSelected (Ptr<SuMimoBeamformingTraceHelper> helper, Ptr<DmgWifiMac> wifiMac,
                                                            Mac48Address from, Antenna2SectorList txCandidates,
                                                            Antenna2SectorList rxCandidates, uint16_t bftID)
{
  uint32_t srcID = helper->map_Mac2ID[wifiMac->GetAddress ()];
  uint32_t dstID = helper->map_Mac2ID[from];
  uint8_t numberOfAntennas = txCandidates.size ();

  SRC_DST_ID_PAIR pair = std::make_pair (srcID, dstID);
  MAP_PAIR2STREAM_I it = helper->m_mimoTxCandidates.find (pair);
  if (it == helper->m_mimoTxCandidates.end ())
    {
      /* Save MIMO TX candidates */
      Ptr<OutputStreamWrapper> outputMimoTxCandidates = helper->m_ascii.CreateFileStream (helper->m_tracesFolder + "SuMimoMimoTxCandidates_" +
                                                                                          std::to_string (srcID) + "_" + std::to_string (dstID) + "_"
                                                                                          + helper->m_runNumber + ".csv");
      *outputMimoTxCandidates->GetStream () << "TIME,TRACE_ID,SRC_ID,DST_ID,BFT_ID,";
      for (uint8_t i = 1; i <= numberOfAntennas; i++)
        {
          *outputMimoTxCandidates->GetStream () << "ANTENNA_ID" << uint16_t(i) << ",SECTOR_ID" << uint16_t (i) << ",";
        }
      *outputMimoTxCandidates->GetStream () << std::endl;
      helper->m_mimoTxCandidates[pair] = outputMimoTxCandidates;
    }

  uint16_t numberOfCandidates = txCandidates.begin ()->second.size ();
  for (uint16_t i = 0; i < numberOfCandidates; i++)
    {
      *helper->m_mimoTxCandidates[pair]->GetStream () << Simulator::Now ().GetNanoSeconds () << ","
                                            << helper->m_qdPropagationEngine->GetCurrentTraceIndex () << ","
                                            << srcID << "," << dstID << "," << bftID << ",";
      for (Antenna2SectorListI it = txCandidates.begin (); it != txCandidates.end (); it++)
        {
          *helper->m_mimoTxCandidates[pair]->GetStream () << uint16_t (it->first - 1) << "," << uint16_t (it->second.at (i) - 1) << ",";
        }
      *helper->m_mimoTxCandidates[pair]->GetStream () << std::endl;
    }

  /* Save MIMO RX candidates */
  it = helper->m_mimoRxCandidates.find (pair);
  if (it == helper->m_mimoRxCandidates.end ())
    {
      Ptr<OutputStreamWrapper> outputMimoRxCandidates = helper->m_ascii.CreateFileStream (helper->m_tracesFolder + "SuMimoMimoRxCandidates_" +
                                                                                          std::to_string (srcID) + "_" + helper->m_runNumber + ".csv");
      *outputMimoRxCandidates->GetStream () << "TIME,TRACE_ID,SRC_ID,DST_ID,BFT_ID,";
      for (uint8_t i = 1; i <= numberOfAntennas; i++)
        {
          *outputMimoRxCandidates->GetStream () << "ANTENNA_ID" << uint16_t(i) << ",SECTOR_ID" << uint16_t(i) << ",";
        }
      *outputMimoRxCandidates->GetStream () << std::endl;
      helper->m_mimoRxCandidates[pair] = outputMimoRxCandidates;
    }
  numberOfCandidates = rxCandidates.begin ()->second.size ();
  for (uint16_t i = 0; i < numberOfCandidates; i++)
    {
      *helper->m_mimoRxCandidates[pair]->GetStream () << Simulator::Now ().GetNanoSeconds () << ","
                                            << helper->m_qdPropagationEngine->GetCurrentTraceIndex () << ","
                                            << srcID << "," << dstID << ","<< bftID << ",";
      for (Antenna2SectorListI it = rxCandidates.begin (); it != rxCandidates.end (); it++)
        {
          *helper->m_mimoRxCandidates[pair]->GetStream () << uint16_t (it->first - 1) << "," << uint16_t (it->second.at (i) - 1) << ",";
        }
      *helper->m_mimoRxCandidates[pair]->GetStream () << std::endl;
    }
}

void
SuMimoBeamformingTraceHelper::SuMimoMimoPhaseMeasurements (Ptr<SuMimoBeamformingTraceHelper> helper,
                                                           Ptr<DmgWifiMac> srcWifiMac, MimoPhaseMeasurementsAttributes attributes,
                                                           SU_MIMO_ANTENNA2ANTENNA antenna2antenna)
{
  uint32_t dstID = helper->map_Mac2ID[srcWifiMac->GetAddress ()];
  uint32_t srcID = helper->map_Mac2ID[attributes.peerStation];
  uint32_t AP_ID = helper->map_Mac2ID[srcWifiMac->GetBssid ()];
  Ptr<DmgWifiMac> dstWifiMac = helper->m_mapMac2Class[attributes.peerStation];

  SRC_DST_ID_PAIR pair = std::make_pair (srcID, dstID);
  MAP_PAIR2STREAM_I it = helper->m_mimoPhaseMeasurements.find (pair);
  if (it == helper->m_mimoPhaseMeasurements.end ())
    {
      /* Generate the file to store the full MIMO phase measurements. */
      Ptr<OutputStreamWrapper> outputMimoPhase = helper->m_ascii.CreateFileStream (helper->m_tracesFolder + "SuMimoMimoPhaseMeasurements_" +
                                                                                   std::to_string (srcID) + "_" + std::to_string (dstID) + "_"
                                                                                   + helper->m_runNumber + ".csv");
       *outputMimoPhase->GetStream () << "TRACE_ID,SRC_ID,DST_ID,BFT_ID,";
      for (uint8_t i = 1; i <= attributes.nTxAntennas; i++)
        {
          *outputMimoPhase->GetStream () << "TX_ANTENNA_ID" << uint16_t (i) << ",TX_SECTOR_ID" << uint16_t (i) << ",TX_AWV_ID" << uint16_t (i) << ",";
        }
      for (uint8_t i = 1; i <= attributes.nRxAntennas; i++)
        {
          *outputMimoPhase->GetStream () << "RX_ANTENNA_ID" << uint16_t (i) << ",RX_SECTOR_ID" << uint16_t (i) << ",RX_AWV_ID" << uint16_t (i) << ",";
        }
      for (uint8_t i = 0; i < attributes.nTxAntennas; i++)
        {
          for (uint8_t j = 0; j < attributes.nRxAntennas; j++)
            {
              *outputMimoPhase->GetStream () << "SINR_" << uint16_t (i) << "_" << uint16_t (j) << ",";
            }
        }
      *outputMimoPhase->GetStream () << "MIN_STREAM_SINR_DB" << std::endl;
      helper->m_mimoPhaseMeasurements[pair] = outputMimoPhase;
      /* Generate the file to store the optimal MIMO configuration chosen at the end of the training. */
      Ptr<OutputStreamWrapper> outputMimoResults = helper->m_ascii.CreateFileStream (helper->m_tracesFolder + "SuMimo_" +
                                                                                   std::to_string (srcID) + "_" + std::to_string (dstID) + "_"
                                                                                   + helper->m_runNumber + ".csv");
       *outputMimoResults->GetStream () << "TIME,TRACE_ID,SRC_ID,DST_ID,BFT_ID,";
      for (uint8_t i = 1; i <= attributes.nTxAntennas; i++)
        {
          *outputMimoResults->GetStream () << "PEER_RX_ID" << uint16_t (i) << ",TX_ANTENNA_ID" << uint16_t (i) << ",TX_SECTOR_ID" << uint16_t (i) << ",TX_AWV_ID" << uint16_t (i) << ",";
        }
      for (uint8_t i = 1; i <= attributes.nRxAntennas; i++)
        {
          *outputMimoResults->GetStream () << "RX_ANTENNA_ID" << uint16_t (i) << ",RX_SECTOR_ID" << uint16_t (i) << ",RX_AWV_ID" << uint16_t (i) << ",";
        }
      for (uint8_t i = 0; i < attributes.nTxAntennas; i++)
        {
          for (uint8_t j = 0; j < attributes.nRxAntennas; j++)
            {
              *outputMimoResults->GetStream () << "SINR_" << uint16_t (i) << "_" << uint16_t (j) << ",";
            }
        }
      *outputMimoResults->GetStream () << "BSS_ID,MIN_STREAM_SINR_DB" << std::endl;
      helper->m_mimoPhaseResults[pair] = outputMimoResults;
    }
  /* Write the optimal MIMO configuration chosen. */
  MEASUREMENT_AWV_IDs awvId = attributes.queue.top ().second;
  MIMO_AWV_CONFIGURATION rxCombination =
      srcWifiMac->GetCodebook ()->GetMimoConfigFromRxAwvId (awvId.second, dstWifiMac->GetAddress ());
  MIMO_AWV_CONFIGURATION txCombination =
      dstWifiMac->GetCodebook ()->GetMimoConfigFromTxAwvId (awvId.first, srcWifiMac->GetAddress ());
  uint16_t txId = awvId.first;
  MIMO_SNR_LIST measurements;
  for (auto & rxId : awvId.second)
    {
      measurements.push_back (attributes.mimoSnrList.at ((txId - 1) * attributes.rxCombinationsTested + rxId.second - 1));
    }
  *helper->m_mimoPhaseResults[pair]->GetStream () << Simulator::Now ().GetNanoSeconds () << "," << helper->m_qdPropagationEngine->GetCurrentTraceIndex () << ","
                                 << srcID << "," << dstID << "," << attributes.bftID << ",";
  for (uint8_t i = 0; i < attributes.nTxAntennas; i ++)
    {
      *helper->m_mimoPhaseResults[pair]->GetStream () << uint16_t (antenna2antenna[txCombination.at (i).first.first] - 1) << ","
                                     << uint16_t (txCombination.at (i).first.first - 1)
                                     << "," << uint16_t (txCombination.at (i).first.second - 1)
                                     << "," << uint16_t (txCombination.at (i).second) << ",";
    }
  for (uint8_t i = 0; i < attributes.nRxAntennas; i++)
    {
      *helper->m_mimoPhaseResults[pair]->GetStream () << uint16_t (rxCombination.at (i).first.first - 1)
                                     << "," << uint16_t (rxCombination.at (i).first.second - 1)
                                     << "," << uint16_t (rxCombination.at (i).second) << ",";
    }
  uint8_t snrIndex = 0;
  for (uint8_t i = 0; i < attributes.nTxAntennas; i++)
    {
      for (uint8_t j = 0; j < attributes.nRxAntennas; j++)
        {
          *helper->m_mimoPhaseResults[pair]->GetStream () << RatioToDb (measurements.at (j).second.at (snrIndex)) << ",";
          snrIndex++;
        }
    }
  *helper->m_mimoPhaseResults[pair]->GetStream () << AP_ID << "," << RatioToDb (attributes.queue.top ().first) << std::endl;
  /* Write the full set of MIMO measurements reported. */
  while (!attributes.queue.empty ())
    {
      MEASUREMENT_AWV_IDs awvId = attributes.queue.top ().second;
      MIMO_AWV_CONFIGURATION rxCombination =
          srcWifiMac->GetCodebook ()->GetMimoConfigFromRxAwvId (awvId.second, dstWifiMac->GetAddress ());
      MIMO_AWV_CONFIGURATION txCombination =
          dstWifiMac->GetCodebook ()->GetMimoConfigFromTxAwvId (awvId.first, srcWifiMac->GetAddress ());
      uint16_t txId = awvId.first;
      MIMO_SNR_LIST measurements;
      for (auto & rxId : awvId.second)
        {
          measurements.push_back (attributes.mimoSnrList.at ((txId - 1) * attributes.rxCombinationsTested + rxId.second - 1));
        }
      *helper->m_mimoPhaseMeasurements[pair]->GetStream () << helper->m_qdPropagationEngine->GetCurrentTraceIndex () << ","
                                     << srcID << "," << dstID << "," << attributes.bftID << ",";
      for (uint8_t i = 0; i < attributes.nTxAntennas; i ++)
        {
          *helper->m_mimoPhaseMeasurements[pair]->GetStream () << uint16_t (txCombination.at (i).first.first - 1)
                                         << "," << uint16_t (txCombination.at (i).first.second - 1)
                                         << "," << uint16_t (txCombination.at (i).second) << ",";
        }
      for (uint8_t i = 0; i < attributes.nRxAntennas; i++)
        {
          *helper->m_mimoPhaseMeasurements[pair]->GetStream () << uint16_t (rxCombination.at (i).first.first - 1)
                                         << "," << uint16_t (rxCombination.at (i).first.second - 1)
                                         << "," << uint16_t (rxCombination.at (i).second) << ",";
        }
      uint8_t snrIndex = 0;
      for (uint8_t i = 0; i < attributes.nTxAntennas; i++)
        {
          for (uint8_t j = 0; j < attributes.nRxAntennas; j++)
            {
              *helper->m_mimoPhaseMeasurements[pair]->GetStream () << RatioToDb (measurements.at (j).second.at (snrIndex)) << ",";
              snrIndex++;
            }
        }
      *helper->m_mimoPhaseMeasurements[pair]->GetStream () << RatioToDb (attributes.queue.top ().first) << std::endl;
      attributes.queue.pop ();
    }
}

/******************************************************************/

MuMimoBeamformingTraceHelper::MuMimoBeamformingTraceHelper (Ptr<QdPropagationEngine> qdPropagationEngine,
                                                            std::string tracesFolder,
                                                            std::string runNumber,
                                                            NODE_ID_MAPPING mapping)
  : SlsBeamformingTraceHelper (qdPropagationEngine, tracesFolder, runNumber, mapping)
{
  NS_LOG_FUNCTION (this << qdPropagationEngine << tracesFolder << runNumber << mapping );
  DoGenerateTraceFiles ();
}

MuMimoBeamformingTraceHelper::~MuMimoBeamformingTraceHelper (void)
{
}

void
MuMimoBeamformingTraceHelper::DoGenerateTraceFiles (void)
{
  NS_LOG_FUNCTION (this);
  /** Create MU-MIMO Traces files **/

  /* 1. Create MU-MIMO SISO Phase Measurements Trace File */
  m_sisoPhaseMeasurements = m_ascii.CreateFileStream (m_tracesFolder + "MuMimoSisoPhaseMeasurements_" + m_runNumber + ".csv");
  *m_sisoPhaseMeasurements->GetStream () << "TIME,TRACE_ID,SRC_ID,DST_ID,MU_GROUP_ID,BFT_ID,RX_ANTENNA_ID,PEER_TX_ANTENNA_ID,PEER_TX_SECTOR_ID,BSS_ID,SINR_DB" << std::endl;

  /* 1. Create MU-MIMO SISO Phase Results Trace File */
  m_sisoPhaseResults = m_ascii.CreateFileStream (m_tracesFolder + "MuMimoSisoPhaseResults_" + m_runNumber + ".csv");
  *m_sisoPhaseResults->GetStream () << "TIME,TRACE_ID,SRC_ID,DST_ID,MU_GROUP_ID,BFT_ID,STA_AID,TX_ANTENNA_ID,TX_SECTOR_ID,BSS_ID,SINR_DB" << std::endl;
}

void
MuMimoBeamformingTraceHelper::DoConnectTrace (Ptr<DmgWifiMac> wifiMac)
{
  NS_LOG_FUNCTION (this << wifiMac);
  SlsBeamformingTraceHelper::DoConnectTrace (wifiMac);
  wifiMac->TraceConnectWithoutContext ("MuMimoSisoPhaseMeasurements",
                                       MakeBoundCallback (&MuMimoBeamformingTraceHelper::MuMimoSisoPhaseMeasurements, this, wifiMac));
  wifiMac->TraceConnectWithoutContext ("MuMimoSisoPhaseCompleted",
                                       MakeBoundCallback (&MuMimoBeamformingTraceHelper::MuMimoSisoPhaseCompleted, this, wifiMac));
  wifiMac->TraceConnectWithoutContext ("MuMimoMimoCandidatesSelected",
                                       MakeBoundCallback (&MuMimoBeamformingTraceHelper::MuMimoMimoCandidatesSelected, this, wifiMac));
  wifiMac->TraceConnectWithoutContext ("MuMimoMimoPhaseMeasurements",
                                       MakeBoundCallback (&MuMimoBeamformingTraceHelper::MuMimoMimoPhaseMeasurements, this, wifiMac));
  wifiMac->TraceConnectWithoutContext ("MuMimoOptimalConfiguration",
                                       MakeBoundCallback (&MuMimoBeamformingTraceHelper::MuMimoOptimalConfiguration, this, wifiMac));
}

void
MuMimoBeamformingTraceHelper::MuMimoSisoPhaseMeasurements (Ptr<MuMimoBeamformingTraceHelper> helper, Ptr<DmgWifiMac> srcWifiMac,
                                                           Mac48Address from, MU_MIMO_SNR_MAP measurementsMap, uint8_t muGroupID, uint16_t bftID)
{
  uint32_t srcID = helper->map_Mac2ID[srcWifiMac->GetAddress ()];
  uint32_t dstID = helper->map_Mac2ID[from];
  uint32_t AP_ID = helper->map_Mac2ID[srcWifiMac->GetBssid ()];
  Ptr<DmgWifiMac> dstWifiMac = helper->m_mapMac2Class[from];
  for (MU_MIMO_SNR_MAP::iterator it = measurementsMap.begin (); it != measurementsMap.end (); it++)
    {
      ANTENNA_CONFIGURATION antennaConfig = dstWifiMac->GetCodebook ()->GetAntennaConfigurationShortSSW (std::get<0> (it->first));
      *helper->m_sisoPhaseMeasurements->GetStream () << Simulator::Now ().GetNanoSeconds ()  << ","
                                                     << helper->m_qdPropagationEngine->GetCurrentTraceIndex () << ","
                                                     << srcID << "," << dstID << "," << uint16_t (muGroupID) << "," << bftID << ","
                                                     << uint16_t (std::get<1> (it->first) - 1) << "," << uint16_t (antennaConfig.first - 1) << ","
                                                     << uint16_t (antennaConfig.second - 1) << "," << AP_ID << ","
                                                     << RatioToDb (it->second) << "," << std::endl;
    }
}

void
MuMimoBeamformingTraceHelper::MuMimoSisoPhaseCompleted (Ptr<MuMimoBeamformingTraceHelper> helper,
                                                        Ptr<DmgWifiMac> srcWifiMac,
                                                        MIMO_FEEDBACK_MAP feedbackMap,
                                                        uint8_t numberOfTxAntennas, uint8_t numberOfRxAntennas, uint8_t muGroupID, uint16_t bftID)
{
  uint32_t srcID = helper->map_Mac2ID[srcWifiMac->GetAddress ()];
  uint32_t dstID = 0;
  uint32_t AP_ID = helper->map_Mac2ID[srcWifiMac->GetBssid ()];
  for (MIMO_FEEDBACK_MAP::iterator it = feedbackMap.begin (); it != feedbackMap.end (); it++)
    {
      *helper->m_sisoPhaseResults->GetStream () << Simulator::Now ().GetNanoSeconds () << ","
                                                << helper->m_qdPropagationEngine->GetCurrentTraceIndex () << ","
                                                << srcID << "," << dstID << "," << uint16_t (muGroupID) << "," << bftID << ","
                                                << uint16_t (std::get<1> (it->first)) << "," << uint16_t (std::get<0> (it->first) - 1) << ","
                                                << uint16_t (std::get<2> (it->first) - 1) << "," << AP_ID << ","
                                                << RatioToDb ((it->second)) << std::endl;
    }
}

void
MuMimoBeamformingTraceHelper::MuMimoMimoCandidatesSelected (Ptr<MuMimoBeamformingTraceHelper> helper,
                                                            Ptr<DmgWifiMac> srcWifiMac,
                                                            uint8_t muGroupId, Antenna2SectorList txCandidates, uint16_t bftID)
{
  uint32_t srcID = helper->map_Mac2ID[srcWifiMac->GetAddress ()];
  uint32_t AP_ID = helper->map_Mac2ID[srcWifiMac->GetBssid ()];
  uint8_t numberOfAntennas = txCandidates.size ();
  /* Save the MIMO candidates to a trace file */
  SRC_DST_ID_PAIR pair = std::make_pair (srcID, muGroupId);
  MAP_PAIR2STREAM_I it = helper->m_mimoTxCandidates.find (pair);
  if (it == helper->m_mimoTxCandidates.end ())
    {
      Ptr<OutputStreamWrapper> outputMimoTxCandidates = helper->m_ascii.CreateFileStream (helper->m_tracesFolder + "MuMimoMimoTxCandidates_" +
                                                                                          std::to_string (srcID) + "_" + std::to_string (muGroupId) + "_"
                                                                                          + helper->m_runNumber + ".csv");
      *outputMimoTxCandidates->GetStream () << "TRACE_ID,SRC_ID,MU_GROUP_ID,BFT_ID,";
      for (uint8_t i = 1; i <= numberOfAntennas; i++)
        {
          *outputMimoTxCandidates->GetStream () << "ANTENNA_ID" << uint16_t(i) << ",SECTOR_ID" << uint16_t (i) << ",";
        }
      *outputMimoTxCandidates->GetStream () << "BSS_ID" << std::endl;
      helper->m_mimoTxCandidates [pair] = outputMimoTxCandidates;
    }

  uint8_t numberOfCandidates = txCandidates.begin ()->second.size ();
  for (uint8_t i = 0; i < numberOfCandidates; i++)
    {
      *helper->m_mimoTxCandidates [pair]->GetStream () << helper->m_qdPropagationEngine->GetCurrentTraceIndex () << ","
                                            << srcID << "," << uint16_t (muGroupId) << "," << bftID << "," ;
      for (Antenna2SectorListI it = txCandidates.begin (); it != txCandidates.end (); it++)
        {
          *helper->m_mimoTxCandidates [pair]->GetStream () << uint16_t (it->first - 1) << "," << uint16_t (it->second.at (i) - 1) << ",";
        }
      *helper->m_mimoTxCandidates [pair]->GetStream () << AP_ID << std::endl;
    }
}

void
MuMimoBeamformingTraceHelper::MuMimoMimoPhaseMeasurements (Ptr<MuMimoBeamformingTraceHelper> helper,
                                                           Ptr<DmgWifiMac> srcWifiMac,
                                                           MimoPhaseMeasurementsAttributes attributes,
                                                           uint8_t muGroupID)
{
  uint32_t srcID = helper->map_Mac2ID[srcWifiMac->GetAddress ()];
  uint32_t dstID = helper->map_Mac2ID[attributes.peerStation];
  uint32_t AP_ID = helper->map_Mac2ID[srcWifiMac->GetBssid ()];
  Ptr<DmgWifiMac> dstWifiMac = helper->m_mapMac2Class[attributes.peerStation];
  /* Save the MIMO Phase Measurements to a trace file */
  SRC_DST_ID_PAIR pair = std::make_pair (srcID, dstID);
  MAP_PAIR2STREAM_I it = helper->m_mimoPhaseMeasurements.find (pair);
  if (it == helper->m_mimoPhaseMeasurements.end ())
    {
      Ptr<OutputStreamWrapper> outputMimoPhase = helper->m_ascii.CreateFileStream (helper->m_tracesFolder + "MuMimoMimoPhaseMeasurements_" +
                                                                                   std::to_string (srcID) + "_" + std::to_string (dstID) + "_"
                                                                                   + helper->m_runNumber + ".csv");
      *outputMimoPhase->GetStream () << "TRACE_ID,SRC_ID,DST_ID,MU_GROUP_ID,BFT_ID,";
      for (uint8_t i = 1; i <= attributes.nTxAntennas; i++)
        {
          *outputMimoPhase->GetStream () << "TX_ANTENNA_ID" << uint16_t (i) << ",TX_SECTOR_ID" << uint16_t (i) << ",TX_AWV_ID" << uint16_t (i) << ",";
        }
      for (uint8_t i = 1; i <= attributes.nRxAntennas; i++)
        {
          *outputMimoPhase->GetStream () << "RX_ANTENNA_ID" << uint16_t (i) << ",RX_SECTOR_ID" << uint16_t (i) << ",RX_AWV_ID" << uint16_t (i) << ",";
        }
      for (uint8_t i = 0; i < attributes.nTxAntennas; i++)
        {
          for (uint8_t j = 0; j < attributes.nRxAntennas; j++)
            {
              *outputMimoPhase->GetStream () << "SINR_" << uint16_t (i) << "_" << uint16_t (j) << ",";
            }
        }
      *outputMimoPhase->GetStream () << "BSS_ID,MIN_STREAM_SINR_DB" << std::endl;
      helper->m_mimoPhaseMeasurements [pair] = outputMimoPhase;

      Ptr<OutputStreamWrapper> outputMimoPhaseR = helper->m_ascii.CreateFileStream (helper->m_tracesFolder + "MuMimoMimoPhaseMeasurements_Reduced_" +
                                                                                    std::to_string (srcID) + "_" + std::to_string (dstID) + "_"
                                                                                    + "_" + helper->m_runNumber + ".csv");
      *outputMimoPhaseR->GetStream () << "TRACE_ID,SRC_ID,DST_ID,MU_GROUP_ID,BFT_ID,";
      for (uint8_t i = 1; i <= attributes.nTxAntennas; i++)
        {
          *outputMimoPhaseR->GetStream () << "TX_ANTENNA_ID" << uint16_t (i) << ",TX_SECTOR_ID" << uint16_t (i) << ",TX_AWV_ID" << uint16_t (i) << ",";
        }
      for (uint8_t i = 1; i <= attributes.nRxAntennas; i++)
        {
          *outputMimoPhaseR->GetStream () << "RX_ANTENNA_ID" << uint16_t (i) << ",RX_SECTOR_ID" << uint16_t (i) << ",RX_AWV_ID" << uint16_t (i) << ",";
        }
      for (uint8_t i = 0; i < attributes.nTxAntennas; i++)
        {
          for (uint8_t j = 0; j < attributes.nRxAntennas; j++)
            {
              *outputMimoPhaseR->GetStream () << "SINR_" << uint16_t (i) << "_" << uint16_t (j) << ",";
            }
        }
      *outputMimoPhaseR->GetStream () << "BSS_ID,MIN_STREAM_SINR_DB" << std::endl;
      helper->m_mimoPhaseMeasurementsReduced [pair] = outputMimoPhaseR;
    }

  std::vector<uint16_t> txIds;
  while (!attributes.queue.empty ())
    {
      MEASUREMENT_AWV_IDs awvId = attributes.queue.top ().second;
      //      std::map<RX_ANTENNA_ID, uint16_t> rxAwvIds;
      //      SNR_MEASUREMENT_INDEX measurementIdx = std::make_pair (awvId.second[1], 0);
      //      uint16_t rxSectorId = measurementIdx.first  % rxCombinationsTested;
      //      if (rxSectorId == 0)
      //        rxSectorId = rxCombinationsTested;
      //      uint8_t rxAntennaIdx = (measurementIdx.second % nRxAntennas);
      //      if (rxAntennaIdx == 0)
      //        rxAntennaIdx = nRxAntennas;
      //      rxAwvIds [1] = rxSectorId;
      MIMO_AWV_CONFIGURATION rxCombination
          = srcWifiMac->GetCodebook ()->GetMimoConfigFromRxAwvId (awvId.second, attributes.peerStation);
      MIMO_AWV_CONFIGURATION txCombination
          = dstWifiMac->GetCodebook ()->GetMimoConfigFromTxAwvId (awvId.first, dstWifiMac->GetAddress ());
      uint16_t txId = awvId.first;
      MIMO_SNR_LIST measurements;
      for (auto & rxId : awvId.second)
        {
          measurements.push_back (attributes.mimoSnrList.at ((txId - 1) * attributes.rxCombinationsTested + rxId.second - 1));
        }
      *helper->m_mimoPhaseMeasurements [pair]->GetStream () << helper->m_qdPropagationEngine->GetCurrentTraceIndex () << ","
                                     << srcID << "," << dstID << "," << uint16_t (muGroupID) << "," << attributes.bftID << "," ;
      for (uint8_t i = 0; i < attributes.nTxAntennas; i ++)
        {
          *helper->m_mimoPhaseMeasurements [pair]->GetStream () << uint16_t (txCombination.at (i).first.first - 1) << "," << uint16_t (txCombination.at (i).first.second - 1)
                                         << "," << uint16_t (txCombination.at (i).second) << ",";
        }
      for (uint8_t i = 0; i < attributes.nRxAntennas; i++)
        {
          *helper->m_mimoPhaseMeasurements [pair]->GetStream () << uint16_t (rxCombination.at (i).first.first - 1) << "," << uint16_t (rxCombination.at (i).first.second - 1)
                                         << "," << uint16_t (rxCombination.at (i).second) << ",";
        }
      uint8_t snrIndex = 0;
      for (uint8_t i = 0; i < attributes.nTxAntennas; i++)
        {
          for (uint8_t j = 0; j < attributes.nRxAntennas; j++)
            {
              *helper->m_mimoPhaseMeasurements [pair]->GetStream () << RatioToDb (measurements.at (j).second.at (snrIndex)) << ",";
              snrIndex++;
            }
        }
      *helper->m_mimoPhaseMeasurements [pair]->GetStream () << AP_ID << "," << RatioToDb (attributes.queue.top ().first) << std::endl;
      if (attributes.differentRxCombinations || (std::find (txIds.begin (), txIds.end (), awvId.first) == txIds.end ()))
        {
          txIds.push_back (attributes.queue.top ().second.first);
          *helper->m_mimoPhaseMeasurementsReduced [pair]->GetStream () << helper->m_qdPropagationEngine->GetCurrentTraceIndex () << ","
                                          << srcID << "," << dstID << "," << uint16_t (muGroupID) << "," << attributes.bftID << ",";
          for (uint8_t i = 0; i < attributes.nTxAntennas; i ++)
            {
              *helper->m_mimoPhaseMeasurementsReduced [pair]->GetStream () << uint16_t (txCombination.at (i).first.first - 1)
                                                                           << "," << uint16_t (txCombination.at (i).first.second - 1)
                                                                            << "," << uint16_t (txCombination.at (i).second) << ",";
            }
          for (uint8_t i = 0; i < attributes.nRxAntennas; i++)
            {
              *helper->m_mimoPhaseMeasurementsReduced [pair]->GetStream () << uint16_t (rxCombination.at (i).first.first - 1) << ","
                                                                           << uint16_t (rxCombination.at (i).first.second - 1)
                                                                           << "," << uint16_t (rxCombination.at (i).second) << ",";
            }
          uint8_t snrIndex = 0;
          for (uint8_t i = 0; i < attributes.nTxAntennas; i++)
            {
              for (uint8_t j = 0; j < attributes.nRxAntennas; j++)
                {
                  *helper->m_mimoPhaseMeasurementsReduced [pair]->GetStream () << RatioToDb (measurements.at (j).second.at (snrIndex)) << ",";
                  snrIndex++;
                }
            }
          *helper->m_mimoPhaseMeasurementsReduced [pair]->GetStream () << AP_ID << "," << RatioToDb (attributes.queue.top ().first) << std::endl;
        }
      attributes.queue.pop ();
    }
}

void
MuMimoBeamformingTraceHelper::MuMimoOptimalConfiguration (Ptr<MuMimoBeamformingTraceHelper> helper,
                                                           Ptr<DmgWifiMac> srcWifiMac,
                                                           MIMO_AWV_CONFIGURATION config,
                                                           uint8_t muGroupID, uint16_t bftID,
                                                           MU_MIMO_ANTENNA2RESPONDER antenna2responder, bool isInitiator)
{
  uint32_t srcID = helper->map_Mac2ID[srcWifiMac->GetAddress ()];
  uint32_t AP_ID = helper->map_Mac2ID[srcWifiMac->GetBssid ()];
  /* Save the MIMO Phase Measurements to a trace file */
  SRC_DST_ID_PAIR pair = std::make_pair (srcID, muGroupID);
  MAP_PAIR2STREAM_I it = helper->m_mimoOptimalConfiguration.find (pair);
  if (it == helper->m_mimoOptimalConfiguration.end ())
    {
      if (isInitiator)
        {
          Ptr<OutputStreamWrapper> outputMimoPhase = helper->m_ascii.CreateFileStream (helper->m_tracesFolder + "MuMimo_I_" +
                                                                                       std::to_string (srcID) + "_" + std::to_string (muGroupID) + "_"
                                                                                       + helper->m_runNumber + ".csv");
          *outputMimoPhase->GetStream () << "TIME,TRACE_ID,SRC_ID,MU_GROUP_ID,BFT_ID,";
          for (uint8_t i = 1; i <= config.size (); i++)
            {
              *outputMimoPhase->GetStream () << "RESPONDER_ID" << uint16_t (i) << ",ANTENNA_ID" << uint16_t (i)
                                             << ",SECTOR_ID" << uint16_t (i) << ",AWV_ID" << uint16_t (i) << ",";
            }
          *outputMimoPhase->GetStream () << "BSS_ID" << std::endl;
          helper->m_mimoOptimalConfiguration [pair] = outputMimoPhase;
        }
      else
        {
          Ptr<OutputStreamWrapper> outputMimoPhase = helper->m_ascii.CreateFileStream (helper->m_tracesFolder + "MuMimo_R_" +
                                                                                       std::to_string (srcID) + "_" + std::to_string (muGroupID) + "_"
                                                                                       + helper->m_runNumber + ".csv");
          *outputMimoPhase->GetStream () << "TIME,TRACE_ID,SRC_ID,MU_GROUP_ID,BFT_ID,";
          for (uint8_t i = 1; i <= config.size (); i++)
            {
              *outputMimoPhase->GetStream () << "ANTENNA_ID" << uint16_t (i) << ",SECTOR_ID" << uint16_t (i) << ",AWV_ID" << uint16_t (i) << ",";
            }
          *outputMimoPhase->GetStream () << "BSS_ID" << std::endl;
          helper->m_mimoOptimalConfiguration [pair] = outputMimoPhase;
        }
    }
  *helper->m_mimoOptimalConfiguration [pair]->GetStream () << Simulator::Now ().GetNanoSeconds () << ","
                                                               << helper->m_qdPropagationEngine->GetCurrentTraceIndex () << ","
                                                               << srcID << "," << uint16_t (muGroupID) << "," << bftID << "," ;
  if (isInitiator)
    {
      for (uint8_t i = 0; i < config.size (); i ++)
        {
          *helper->m_mimoOptimalConfiguration [pair]->GetStream () << helper->map_Mac2ID[antenna2responder[config.at (i).first.first]] << "," << uint16_t (config.at (i).first.first - 1) << ","
                                                                   << uint16_t (config.at (i).first.second - 1) << "," << uint16_t (config.at (i).second) << ",";
        }
    }
  else
    {
      for (uint8_t i = 0; i < config.size (); i ++)
        {
          *helper->m_mimoOptimalConfiguration [pair]->GetStream () << uint16_t (config.at (i).first.first - 1) << "," << uint16_t (config.at (i).first.second - 1)
                                             << "," << uint16_t (config.at (i).second) << ",";
        }
    }

  *helper->m_mimoOptimalConfiguration [pair]->GetStream () << AP_ID << std::endl;
}

/******************************************************************/

GroupBeamformingTraceHelper::GroupBeamformingTraceHelper (Ptr<QdPropagationEngine> qdPropagationEngine,
                                                      std::string tracesFolder,
                                                      std::string runNumber,
                                                      NODE_ID_MAPPING mapping)
  : BeamformingTraceHelper (qdPropagationEngine, tracesFolder, runNumber, mapping)
{
  NS_LOG_FUNCTION (this << qdPropagationEngine << tracesFolder << runNumber << mapping );
  DoGenerateTraceFiles ();
}

GroupBeamformingTraceHelper::~GroupBeamformingTraceHelper (void)
{
}

void
GroupBeamformingTraceHelper::DoGenerateTraceFiles (void)
{
  NS_LOG_FUNCTION (this);
  m_stream = m_ascii.CreateFileStream (m_tracesFolder + "group_" + m_runNumber + ".csv");
  // Add MetaData?
  *m_stream->GetStream () << "TIME,TRACE_ID,SRC_ID,DST_ID,BFT_ID,ANTENNA_ID,SECTOR_ID,AWV_ID,ROLE,BSS_ID,SINR_DB" << std::endl;
}

void
GroupBeamformingTraceHelper::DoConnectTrace (Ptr<DmgWifiMac> wifiMac)
{
  wifiMac->TraceConnectWithoutContext ("GroupBeamformingCompleted",
                                       MakeBoundCallback (&GroupBeamformingTraceHelper::GroupCompleted, this, wifiMac));
}

void
GroupBeamformingTraceHelper::GroupCompleted (Ptr<GroupBeamformingTraceHelper> recorder,
                                         Ptr<DmgWifiMac> wifiMac,
                                         GroupBfCompletionAttrbitutes attributes)
{
  uint32_t srcID = recorder->map_Mac2ID[wifiMac->GetAddress ()];
  uint32_t dstID = recorder->map_Mac2ID[attributes.peerStation];
  uint32_t AP_ID = recorder->map_Mac2ID[wifiMac->GetBssid ()];
  double linkSnr =   RatioToDb (attributes.maxSnr);
  *recorder->m_stream->GetStream () << Simulator::Now ().GetNanoSeconds () << ","
                                    << recorder->m_qdPropagationEngine->GetCurrentTraceIndex () << ","
                                    << srcID << "," << dstID << "," << attributes.bftID << ","
                                    << uint16_t (attributes.antennaID - 1) << "," << uint16_t (attributes.sectorID - 1) << ","
                                    << uint16_t (attributes.awvID) << ","
                                    << attributes.beamformingDirection << ","
                                    << AP_ID << "," << linkSnr << std::endl;
}

/******************************************************************/

} //namespace ns3
