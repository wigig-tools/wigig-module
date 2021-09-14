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
#ifndef BEAMFORMING_TRACE_HELPER_H
#define BEAMFORMING_TRACE_HELPER_H

#include <map>
#include <string>

#include "ns3/object-factory.h"
#include "ns3/net-device-container.h"
#include "ns3/trace-helper.h"
#include "ns3/dmg-wifi-mac.h"
#include "ns3/qd-propagation-engine.h"

using namespace std;

namespace ns3 {

typedef std::map<Mac48Address, uint32_t> MAP_MAC2ID;                            //!< Typedef for mapping between MAC address and ns-3 node ID.
typedef MAP_MAC2ID::iterator MAP_MAC2ID_I;                                      //!< Typedef for iterator over MAC to ns-3 node IDs.
typedef std::map<Mac48Address, Ptr<DmgWifiMac> > MAP_MAC2CLASS;                 //!< Typedef for mapping between MAC address and ns-3 node ID.
typedef std::map<Mac48Address, Ptr<OutputStreamWrapper> > MAP_MAC2STREAM;       //!< Typedef for mapping between MAC address and ns-3 node ID.
typedef std::pair<uint32_t, uint32_t> SRC_DST_ID_PAIR;                          //!< Typedef for a source and destionation ID pair.
typedef std::map<SRC_DST_ID_PAIR, Ptr<OutputStreamWrapper>> MAP_PAIR2STREAM;    //!< Typedef for mapping between a source and destination ID pair and a stream.
typedef MAP_PAIR2STREAM::iterator MAP_PAIR2STREAM_I;                            //!< Typedef for a iterator over a source and destionatio ID pair and a stream.

enum NODE_ID_MAPPING {
  NODE_ID_MAPPING_NS3_IDS = 0,
  NODE_ID_MAPPING_QD_CUSTOM_IDS,
};

class BeamformingTraceHelper : public SimpleRefCount<BeamformingTraceHelper>
{
public:
  /**
   * Beamforming trace helper constructor.
   * \param qdPropagationEngine Pointer to the QdPropagationEngine class.
   * \param tracesFolder Path to the folder where we store traces files.
   * \param mapping Type of node ID mapping either directly use ns-3 IDs or map ns-3 nodes IDs to a specific ones
   * based on QdPropagationEngine class.
   */
  BeamformingTraceHelper (Ptr<QdPropagationEngine> qdPropagationEngine,
                          std::string tracesFolder,
                          string runNumber,
                          NODE_ID_MAPPING mapping = NODE_ID_MAPPING_NS3_IDS);
  virtual ~BeamformingTraceHelper (void);

  /**
   * Connect beamforming trace for the given WifiMac.
   * \param wifiMac Pointer to the DmgWifiMac class.
   */
  void ConnectTrace (Ptr<DmgWifiMac> wifiMac);
  /**
   * Connect beamforming trace for all the devices in the NetDeviceContainer
   * \param deviceConteiner List of device containers containing WifiNetDevices.
   */
  void ConnectTrace (const NetDeviceContainer &deviceConteiner);
  /**
   * Set the output folder where you want to dump the trace files.
   * \param traceFolder Path to the trace folder.
   */
  void SetTracesFolder (std::string traceFolder);
  /**
   * Get the current trace folder.
   * \return Path of the current trace folder
   */
  std::string GetTracesFolder (void) const;
  /**
   * Get pointer to the stream wrapper.
   * \return
   */
  Ptr<OutputStreamWrapper> GetStreamWrapper (void) const;
  /**
   * Set the simulation run number.
   * \return
   */
  void SetRunNumber (std::string runNumber);
  /**
   * Get the current simulation run number.
   * \return Simulation run number
   */
  std::string GetRunNumber (void) const;

protected:
  /**
   * In this function, we generate all the traces files related to the helper.
   */
  virtual void DoGenerateTraceFiles (void) = 0;
  virtual void DoConnectTrace (Ptr<DmgWifiMac> wifiMac) = 0;

protected:
  AsciiTraceHelper m_ascii;                           //!< Ascii tracer helper instance.
  std::string m_tracesFolder;                         //!< Path to the folder where we store the traces.
  Ptr<OutputStreamWrapper> m_stream;                  //!< Pointer to the stream wrapper.
  MAP_MAC2ID map_Mac2ID;                              //!< Map between WifiNetDevice MAC address and ns-3 node ID.
  Ptr<QdPropagationEngine> m_qdPropagationEngine;     //!< Pointer to the Q-D propagation engine.
  std::string m_runNumber;                            //!< Simulation Run Number
  NODE_ID_MAPPING m_mapping;                          //!< The type of mapping between ns-3 IDs and Q-D software IDs.
  MAP_MAC2CLASS m_mapMac2Class;                       //!< Data structure for mapping between MAC Address and the corresponding DmgWifiMac class.

};

class SlsBeamformingTraceHelper : public BeamformingTraceHelper
{
public:
  /**
   * SLS beamforming trace helper constructor.
   * \param qdPropagationEngine Pointer to the QdPropagationEngine class.
   * \param tracesFolder Path to the folder where we store traces files.
   * \param mapping Type of node ID mapping either directly use ns-3 IDs or map ns-3 nodes IDs to a specific ones
   * based on QdPropagationEngine class.
   */
  SlsBeamformingTraceHelper (Ptr<QdPropagationEngine> qdPropagationEngine,
                             std::string tracesFolder,
                             std::string runNumber,
                             NODE_ID_MAPPING mapping = NODE_ID_MAPPING_NS3_IDS);
  virtual ~SlsBeamformingTraceHelper (void);

protected:
  virtual void DoGenerateTraceFiles (void);
  virtual void DoConnectTrace (Ptr<DmgWifiMac> wifiMac);

private:
  /**
   * This function is called when a SLS beamforming training is completed.
   * \param helper Pointer to the SLS beamforming trace helper.
   * \param wifiMac Pointer to the DMG Wifi MAC class.
   * \param attributes SLS completion attributes structure.
   */
  static void SLSCompleted (Ptr<SlsBeamformingTraceHelper> helper, Ptr<DmgWifiMac> wifiMac, SlsCompletionAttrbitutes attributes);

};

class SuMimoBeamformingTraceHelper : public SlsBeamformingTraceHelper
{
public:
  /**
   * SU-MIMO beamforming trace helper constructor.
   * \param qdPropagationEngine Pointer to the QdPropagationEngine class.
   * \param tracesFolder Path to the folder where we store traces files.
   * \param mapping Type of node ID mapping either directly use ns-3 IDs or map ns-3 nodes IDs to a specific ones
   * based on QdPropagationEngine class.
   */
  SuMimoBeamformingTraceHelper (Ptr<QdPropagationEngine> qdPropagationEngine,
                                std::string tracesFolder, string runNumber,
                                NODE_ID_MAPPING mapping = NODE_ID_MAPPING_NS3_IDS);
  virtual ~SuMimoBeamformingTraceHelper (void);

protected:
  virtual void DoGenerateTraceFiles (void);
  virtual void DoConnectTrace (Ptr<DmgWifiMac> wifiMac);

private:
  /**
   * SuMimoSisoPhaseMeasurements
   * \param helper
   * \param wifiMac
   * \param from
   * \param measurementsMap
   * \param edmgTrnN
   * \param bftID
   */
  static void SuMimoSisoPhaseMeasurements (Ptr<SuMimoBeamformingTraceHelper> helper, Ptr<DmgWifiMac> wifiMac,
                                           Mac48Address from, SU_MIMO_SNR_MAP measurementsMap, uint8_t edmgTrnN, uint16_t bftID);
  /**
   * SuMimoSisoPhaseComplete
   * \param helper
   * \param wifiMac
   * \param from
   * \param feedbackMap
   * \param numberOfTxAntennas
   * \param numberOfRxAntennas
   * \param bftID
   */
  static void SuMimoSisoPhaseCompleted (Ptr<SuMimoBeamformingTraceHelper> helper, Ptr<DmgWifiMac> wifiMac,
                                        Mac48Address from, MIMO_FEEDBACK_MAP feedbackMap,
                                        uint8_t numberOfTxAntennas, uint8_t numberOfRxAntennas, uint16_t bftID);
  /**
   * SuMimoMimoCandidatesSelected
   * \param helper
   * \param wifiMac
   * \param from
   * \param txCandidates
   * \param rxCandidates
   * \param bftID
   */
  static void SuMimoMimoCandidatesSelected (Ptr<SuMimoBeamformingTraceHelper> helper, Ptr<DmgWifiMac> wifiMac,
                                            Mac48Address from, Antenna2SectorList txCandidates, Antenna2SectorList rxCandidates, uint16_t bftID);
  /**
   * SuMimoMimoPhaseMeasurements
   * \param helper
   * \param srcWifiMac
   * \param attributes
   */
  static void SuMimoMimoPhaseMeasurements (Ptr<SuMimoBeamformingTraceHelper> helper, Ptr<DmgWifiMac> srcWifiMac,
                                           MimoPhaseMeasurementsAttributes attributes, SU_MIMO_ANTENNA2ANTENNA antenna2antenna);

private:
  Ptr<OutputStreamWrapper> m_sisoPhaseMeasurements;
  Ptr<OutputStreamWrapper> m_sisoPhaseResults;
  MAP_PAIR2STREAM m_mimoTxCandidates;
  MAP_PAIR2STREAM m_mimoRxCandidates;
  MAP_PAIR2STREAM m_mimoPhaseMeasurements;
  MAP_PAIR2STREAM m_mimoPhaseResults;
};

class MuMimoBeamformingTraceHelper : public SlsBeamformingTraceHelper
{
public:
  /**
   * MU-MIMO beamforming trace helper constructor.
   * \param qdPropagationEngine Pointer to the QdPropagationEngine class.
   * \param tracesFolder Path to the folder where we store traces files.
   * \param mapping Type of node ID mapping either directly use ns-3 IDs or map ns-3 nodes IDs to a specific ones
   * based on QdPropagationEngine class.
   */
  MuMimoBeamformingTraceHelper (Ptr<QdPropagationEngine> qdPropagationEngine,
                                std::string tracesFolder, string runNumber,
                                NODE_ID_MAPPING mapping = NODE_ID_MAPPING_NS3_IDS);
  virtual ~MuMimoBeamformingTraceHelper (void);

protected:
  virtual void DoGenerateTraceFiles (void);
  virtual void DoConnectTrace (Ptr<DmgWifiMac> wifiMac);

private:
  /**
   * MuMimoSisoPhaseMeasurements
   * \param helper
   * \param srcWifiMac
   * \param from
   * \param measurementsMap
   * \param MU Group ID
   * \param BFT ID
   */
  static void MuMimoSisoPhaseMeasurements (Ptr<MuMimoBeamformingTraceHelper> helper, Ptr<DmgWifiMac> srcWifiMac,
                                           Mac48Address from, MU_MIMO_SNR_MAP measurementsMap, uint8_t muGroupID, uint16_t bftID);
  /**
   * MuMimoSisoPhaseComplete
   * \param helper
   * \param srcWifiMac
   * \param dstWifiMac
   * \param feedbackMap
   * \param numberOfTxAntennas
   * \param numberOfRxAntennas
   * \param MU Group ID
   * \param BFT ID
   */
  static void MuMimoSisoPhaseCompleted (Ptr<MuMimoBeamformingTraceHelper> helper,
                                        Ptr<DmgWifiMac> srcWifiMac,
                                        MIMO_FEEDBACK_MAP feedbackMap,
                                        uint8_t numberOfTxAntennas, uint8_t numberOfRxAntennas, uint8_t muGroupID, uint16_t bftID);
  /**
   * MuMimoMimoCandidatesSelected
   * \param helper
   * \param srcWifiMac
   * \param dstWifiMac
   * \param muGroupId
   * \param txCandidates
   */
  static void MuMimoMimoCandidatesSelected (Ptr<MuMimoBeamformingTraceHelper> helper,
                                            Ptr<DmgWifiMac> srcWifiMac,
                                            uint8_t muGroupId, Antenna2SectorList txCandidates, uint16_t bftID);
  /**
   * MuMimoMimoPhaseMeasurements
   * \param helper
   * \param srcWifiMac
   * \param attributes
   * \param muGroupID
   */
  static void MuMimoMimoPhaseMeasurements (Ptr<MuMimoBeamformingTraceHelper> helper, Ptr<DmgWifiMac> srcWifiMac,
                                           MimoPhaseMeasurementsAttributes attributes, uint8_t muGroupID);
  /**
   * MuMimoMimoPhaseMeasurements
   * \param helper
   * \param srcWifiMac
   * \param config
   * \param muGroupID
   * \param bftID
   * \param antenna2responder
   * \param isInitiator
   */
  static void MuMimoOptimalConfiguration (Ptr<MuMimoBeamformingTraceHelper> helper, Ptr<DmgWifiMac> srcWifiMac,
                                           MIMO_AWV_CONFIGURATION config, uint8_t muGroupID, uint16_t bftID,
                                           MU_MIMO_ANTENNA2RESPONDER antenna2responder, bool isInitiator);

private:
  Ptr<OutputStreamWrapper> m_sisoPhaseMeasurements;
  Ptr<OutputStreamWrapper> m_sisoPhaseResults;
  MAP_PAIR2STREAM m_mimoTxCandidates;
  MAP_PAIR2STREAM m_mimoPhaseMeasurements;
  MAP_PAIR2STREAM m_mimoPhaseMeasurementsReduced;
  MAP_PAIR2STREAM m_mimoOptimalConfiguration;
};

class GroupBeamformingTraceHelper : public BeamformingTraceHelper
{
public:
  /**
   * Group beamforming trace helper constructor.
   * \param qdPropagationEngine Pointer to the QdPropagationEngine class.
   * \param tracesFolder Path to the folder where we store traces files.
   * \param runNumber The run number of the simulation.
   * \param mapping Type of node ID mapping either directly use ns-3 IDs or map ns-3 nodes IDs to a specific ones
   * based on QdPropagationEngine class.
   */
  GroupBeamformingTraceHelper (Ptr<QdPropagationEngine> qdPropagationEngine,
                             std::string tracesFolder,
                             std::string runNumber,
                             NODE_ID_MAPPING mapping = NODE_ID_MAPPING_NS3_IDS);
  virtual ~GroupBeamformingTraceHelper (void);

protected:
  virtual void DoGenerateTraceFiles (void);
  virtual void DoConnectTrace (Ptr<DmgWifiMac> wifiMac);

private:
  /**
   * This function is called when a Group beamforming training is completed.
   * \param helper Pointer to the Group beamforming trace helper.
   * \param wifiMac Pointer to the DMG Wifi MAC class.
   * \param attributes Group completion attributes structure.
   */
  static void GroupCompleted (Ptr<GroupBeamformingTraceHelper> helper, Ptr<DmgWifiMac> wifiMac, GroupBfCompletionAttrbitutes attributes);

};

} //namespace ns3

#endif // BEAMFORMING_TRACE_HELPER_H
