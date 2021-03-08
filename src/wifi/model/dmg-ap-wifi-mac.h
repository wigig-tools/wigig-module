/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#ifndef DMG_AP_WIFI_MAC_H
#define DMG_AP_WIFI_MAC_H

#include "ns3/random-variable-stream.h"

#include "amsdu-subframe-header.h"
#include "dmg-beacon-txop.h"
#include "dmg-wifi-mac.h"

namespace ns3 {

#define TU                      MicroSeconds (1024)     /* Time Unit defined in 802.11 std */
#define aMaxBIDuration          TU * 1024               /* Maximum BI Duration Defined in 802.11ad */
#define aMinChannelTime         aMaxBIDuration          /* Minimum Channel Time for Clustering */
#define aMinSSSlotsPerABFT      1                       /* Minimum Number of Sector Sweep Slots Per A-BFT */
#define aSSFramesPerSlot        8                       /* Number of SSW Frames per Sector Sweep Slot */
#define aDMGPPMinListeningTime  150                     /* The minimum time between two adjacent SPs with the same source or destination AIDs */

/**
 * \brief Wi-Fi DMG AP state machine
 * \ingroup wifi
 *
 * Handle association, dis-association and authentication,
 * of DMG STAs within an infrastructure DMG BSS.
 */
class DmgApWifiMac : public DmgWifiMac
{
public:
  static TypeId GetTypeId (void);

  DmgApWifiMac ();
  virtual ~DmgApWifiMac ();

  /**
   * Get Association Identifier (AID).
   * \return The AID of the station.
   */
  virtual uint16_t GetAssociationID (void);
  /**
   * \param stationManager the station manager attached to this MAC.
   */
  virtual void SetWifiRemoteStationManager (Ptr<WifiRemoteStationManager> stationManager);

  /**
   * \param linkUp the callback to invoke when the link becomes up.
   */
  virtual void SetLinkUpCallback (Callback<void> linkUp);
  /**
   * \param packet the packet to send.
   * \param to the address to which the packet should be sent.
   *
   * The packet should be enqueued in a tx queue, and should be
   * dequeued as soon as the channel access function determines that
   * access is granted to this MAC.
   */
  virtual void Enqueue (Ptr<Packet> packet, Mac48Address to);
  /**
   * \param packet the packet to send.
   * \param to the address to which the packet should be sent.
   * \param from the address from which the packet should be sent.
   *
   * The packet should be enqueued in a tx queue, and should be
   * dequeued as soon as the channel access function determines that
   * access is granted to this MAC. The extra parameter "from" allows
   * this device to operate in a bridged mode, forwarding received
   * frames without altering the source address.
   */
  virtual void Enqueue (Ptr<Packet> packet, Mac48Address to, Mac48Address from);
  virtual bool SupportsSendFrom (void) const;
  /**
   * \param address the current address of this MAC layer.
   */
  virtual void SetAddress (Mac48Address address);
  /**
   * \param interval the interval between two beacon transmissions.
   */
  void SetBeaconInterval (Time interval);
  /**
   * \return the interval between two beacon transmissions.
   */
  Time GetBeaconInterval (void) const;
  /**
   * Get Datas Transmission Interval Duration
   * \return The duration of DTI.
   */
  Time GetDTIDuration (void) const;
  /**
   * Get the amount of the remaining time in the DTI access period.
   * \return The remaining time in the DTI.
   */
  Time GetDTIRemainingTime (void) const;
  /**
   * \param interval the interval.
   */
  void SetBeaconTransmissionInterval (Time interval);
  /**
   * \return the interval between two beacon transmissions.
   */
  Time GetBeaconTransmissionInterval (void) const;
  /**
   * \param periodicity the A-BFT periodicity.
   */
  void SetAbftPeriodicity (uint8_t periodicity);
  /**
   * \return the A-BFT periodicity.
   */
  uint8_t GetAbftPeriodicity (void) const;
  /**
   * Allocate CBAP period to be announced in DMG Beacon or Announce Frame.
   * \param staticAllocation Is the allocation static.
   * \param allocationStart The start time of the allocation relative to the beginning of DTI.
   * \param blockDuration The duration of the allocation period.
   * \return The start of the next allocation period.
   */
  uint32_t AllocateCbapPeriod (bool staticAllocation, uint32_t allocationStart, uint16_t blockDuration);
  /**
   * Add a new allocation with one single block. The duration of the block is limited to 32 767 microseconds for a SP allocation.
   * and to 65 535 microseconds for a CBAP allocation. The allocation is announced in the following DMG Beacon or Announce Frame.
   * \param allocationID A unique identifier for the allocation.
   * \param allocationType The type of the allocation (CBAP or SP).
   * \param staticAllocation Is the allocation static.
   * \param srcAid The AID of the source DMG STA.
   * \param dstAid The AID of the destination DMG STA.
   * \param allocationStart The start time of the allocation relative to the beginning of DTI.
   * \param blockDuration The duration of the allocation period.
   * \return The start of the next allocation period.
   */
  uint32_t AllocateSingleContiguousBlock (AllocationID allocationID,
                                          AllocationType allocationType, bool staticAllocation,
                                          uint8_t srcAid, uint8_t dstAid,
                                          uint32_t allocationStart, uint16_t blockDuration);
  /**
   * Add a new allocation consisting of consectuive allocation blocks.
   * The allocation is announced in the following DMG Beacon or Announce Frame.
   * \param allocationID A unique identifier for the allocation.
   * \param allocationType The type of the allocation (CBAP or SP).
   * \param staticAllocation Is the allocation static.
   * \param srcAid The AID of the source DMG STA.
   * \param dstAid The AID of the destination DMG STA.
   * \param allocationStart The start time of the allocation relative to the beginning of DTI.
   * \param blockDuration The duration of the allocation period.
   * \param blocks The number of blocks making up the allocation.
   * \return The start of the next allocation period.
   */
  uint32_t AllocateMultipleContiguousBlocks (AllocationID allocationID,
                                             AllocationType allocationType, bool staticAllocation,
                                             uint8_t srcAid, uint8_t dstAid,
                                             uint32_t allocationStart, uint16_t blockDuration, uint8_t blocks);
  /**
   * Allocate maximum part of DTI as a service period channel access.
   * \param id A unique identifier for the allocation.
   * \param srcAid The AID of the source DMG STA.
   * \param dstAid The AID of the destination DMG STA.
   */
  void AllocateDTIAsServicePeriod (AllocationID id, uint8_t srcAid, uint8_t dstAid);
  /**
   * Add a new allocation period to be announced in DMG Beacon or Announce Frame.
   * \param allocationID A unique identifier for the allocation.
   * \param allocationType The type of the allocation (CBAP or SP).
   * \param staticAllocation Is the allocation static.
   * \param srcAid The AID of the source DMG STA.
   * \param dstAid The AID of the destination DMG STA.
   * \param allocationStart The start time of the allocation relative to the beginning of DTI.
   * \param blockDuration The duration of the allocation period.
   * \param blocks The number of blocks making up the allocation.
   * \return The start time of the following allocation period.
   */
  uint32_t AddAllocationPeriod (AllocationID allocationID,
                                AllocationType allocationType, bool staticAllocation,
                                uint8_t srcAid, uint8_t dstAid,
                                uint32_t allocationStart, uint16_t blockDuration,
                                uint16_t blockPeriod, uint8_t blocks);
  /**
   * ContinueBeamformingInDTI
   */
  void ContinueBeamformingInDTI (void);
  /**
   * Allocate SP allocation for Beamforming training.
   * \param srcAid The AID of the source DMG STA.
   * \param dstAid The AID of the destination DMG STA.
   * \param allocationStart The start time of the allocation relative to the beginning of DTI.
   * \param isTXSS Is the Beamforming TXSS or RXSS for both the initiator and the responder.
   * \return The start of the next allocation period.
   */
  uint32_t AllocateBeamformingServicePeriod (uint8_t srcAid, uint8_t dstAid,
                                             uint32_t allocationStart, bool isTXSS);
  /**
   * Allocate SP allocation for Beamforming training.
   * \param srcAid The AID of the source DMG STA.
   * \param dstAid The AID of the destination DMG STA.
   * \param allocationStart The start time of the allocation relative to the beginning of DTI.
   * \param allocationDuration The duration of the beamforming allocation.
   * \param isInitiatorTXSS Is the Initiator Beamforming TXSS or RXSS.
   * \param isResponderTXSS Is the Responder Beamforming TXSS or RXSS.
   * \return The start of the next allocation period.
   */
  uint32_t AllocateBeamformingServicePeriod (uint8_t sourceAid, uint8_t destAid,
                                             uint32_t allocationStart, uint16_t allocationDuration,
                                             bool isInitiatorTXSS, bool isResponderTXSS);
  /**
   * Modify schedulling parameters of an existing allocation.
   * \param id A unique identifier for the allocation.
   * \param srcAid The AID of the source DMG STA.
   * \param dstAid The AID of the destination DMG STA.
   * \param newStartTime The new starting time of the allocation.
   * \param newDuration The new duration of the allocation.
   */
  void ModifyAllocation (AllocationID id, uint8_t srcAid, uint8_t dstAid, uint32_t newStartTime, uint16_t newDuration);
  /**
   * Initiate dynamic channel access procedure in the following BI.
   */
  void InitiateDynamicAllocation (void);
  /**
   * Initiate Polling Period with the specified length.
   * \param ppLength The length of the polling period.
   */
  void InitiatePollingPeriod (Time ppLength);
  /**
   * Get the duration of a polling period.
   * \param polledStationsCount The number of stations to be polled.
   * \return The corresponding polling period duration.
   */
  Time GetPollingPeriodDuration (uint8_t polledStationsCount);
  /**
   * Get associated station AID from its MAC address.
   * \param address The MAC address of the associated station.
   * \return The AID of the associated station.
   */
  uint8_t GetStationAid (Mac48Address address) const;
  /**
   * Get associated station MAC address from AID.
   * \param aid The AID of the associated station.
   * \return The MAC address of the assoicated station.
   */
  Mac48Address GetStationAddress (uint8_t aid) const;
  /**
   * Get Allocation List
   * \return
   */
  AllocationFieldList GetAllocationList (void) const;
  /**
   * Send DMG Add TS Response to DMG STA.
   * \param to The MAC address of the DMG STA which sent the DMG ADDTS Request.
   * \param code The status of the allocation.
   * \param delayElem The TS Delay element.
   * \param elem The TSPEC element.
   */
  void SendDmgAddTsResponse (Mac48Address to, StatusCode code, TsDelayElement &delayElem, DmgTspecElement &elem);
  /**
   * Get list of dynamic allocation info in the SPRs received during polling period.
   * \return
   */
  AllocationDataList GetSprList (void) const;
  /**
   * Add new dynamic allocation info field to the list of allocations to be announced in the Grant Period.
   * \param info The dynamic allocation information field.
   */
  void AddGrantData (AllocationData info);
  /**
   * Send Directional Channel Quality request.
   * \param to The MAC address of the DMG STA.
   * \param numOfRepts Number of measurements repetitions.
   * \param element The Directional Channel Quality Request Information Element.
   */
  void SendDirectionalChannelQualityRequest (Mac48Address to, uint16_t numOfRepts,
                                             Ptr<DirectionalChannelQualityRequestElement> element);

  /**
   * Start DMG AP Operation by transmitting Beaconing.
   */
  void StartAccessPoint (void);
  /* EDMG MU-MIMO BF functions */
  /**
   * Define groups of MU-MIMO capable EDMG STAs to perform DL MU-MIMO beamforming training and transmissions.
   * Create EDMG Group ID set element which will be transmitted in the beacons.
   */
  //void StartAccessPoint (void);

protected:
  friend class DmgBeaconTxop;
  Time GetBTIRemainingTime (void) const;
  /**
   * Start monitoring Beacon SP for DMG Beacons.
   */
  void StartMonitoringBeaconSP (uint8_t beaconSPIndex);
  /**
   * End monitoring Beacon SP for DMG Beacons.
   * \param beaconSPIndex The index of the Beacon SP.
   */
  void EndMonitoringBeaconSP (uint8_t beaconSPIndex);
  /**
   * End channel monitoring for DMG Beacons during Beacon SPs.
   * \param clusterID The MAC address of the cluster.
   */
  void EndChannelMonitoring (Mac48Address clusterID);
  /**
   * Start Syn Beacon Interval.
   */
  void StartSynBeaconInterval (void);
  /**
   * Return the DMG capability of the current PCP/AP.
   * \return the DMG capabilities the PCP/AP supports.
   */
  Ptr<DmgCapabilities> GetDmgCapabilities (void) const;

private:
  virtual void DoDispose (void);
  virtual void DoInitialize (void);

  void StartBeaconInterval (void);
  void EndBeaconInterval (void);
  void StartBeaconTransmissionInterval (void);
  void StartAssociationBeamformTraining (void);
  void StartAnnouncementTransmissionInterval (void);
  void StartDataTransmissionInterval (void);
  void FrameTxOk (const WifiMacHeader &hdr);
  virtual void BrpSetupCompleted (Mac48Address address);
  virtual void NotifyBrpPhaseCompleted (void);
  virtual void Receive (Ptr<WifiMacQueueItem> mpdu);

  /**
   * Start Beacon Header Interval (BHI).
   */
  void StartBeaconHeaderInterval (void);
  /**
   * The packet we sent was successfully received by the receiver
   * (i.e. we received an ACK from the receiver). If the packet
   * was an association response to the receiver, we record that
   * the receiver is now associated with us.
   *
   * \param hdr the header of the packet that we successfully sent
   */
  virtual void TxOk (Ptr<const Packet> packet, const WifiMacHeader &hdr);
  /**
   * The packet we sent was not successfully received by the receiver
   * (i.e. we did not receive an ACK from the receiver). If the packet
   * was an association response to the receiver, we record that
   * the receiver is not associated with us yet.
   *
   * \param hdr the header of the packet that we failed to sent
   */
  virtual void TxFailed (const WifiMacHeader &hdr);
  /**
   * This method can be called to de-aggregate an A-MSDU and forward
   * the constituent packets up the stack.
   *
   * \param mpdu the MPDU containing the A-MSDU.
   */
  virtual void DeaggregateAmsduAndForward (Ptr<WifiMacQueueItem> mpdu);
  /**
   * Get MultiBand Element corresponding to this DMG STA.
   * \return Pointer to the MultiBand element.
   */
  Ptr<MultiBandElement> GetMultiBandElement (void) const;
  /**
   * Start A-BFT Sector Sweep Slot.
   */
  void StartSectorSweepSlot (void);
  /**
   * Establish BRP Setup Subphase
   */
  void DoBrpSetupSubphase (void);
  /**
   * Forward the packet down to DCF/EDCAF (enqueue the packet). This method
   * is a wrapper for ForwardDown with traffic id.
   *
   * \param packet the packet we are forwarding to DCF/EDCAF
   * \param from the address to be used for Address 3 field in the header
   * \param to the address to be used for Address 1 field in the header
   */
  void ForwardDown (Ptr<Packet> packet, Mac48Address from, Mac48Address to);
  /**
   * Forward the packet down to DCF/EDCAF (enqueue the packet).
   *
   * \param packet the packet we are forwarding to DCF/EDCAF
   * \param from the address to be used for Address 3 field in the header
   * \param to the address to be used for Address 1 field in the header
   * \param tid the traffic id for the packet
   */
  void ForwardDown (Ptr<Packet> packet, Mac48Address from, Mac48Address to, uint8_t tid);
  /**
   * Forward a probe response packet to the DCF. The standard is not clear on the correct
   * queue for management frames if QoS is supported. We always use the DCF.
   *
   * \param to the MAC address of the STA we are sending a probe response to.
   */
  void SendProbeResp (Mac48Address to);
  /**
   * Forward an association response packet to the DCF. The standard is not clear on the correct
   * queue for management frames if QoS is supported. We always use the DCF.
   *
   * \param to the MAC address of the STA we are sending an association response to.
   * \param success indicates whether the association was successful or not.
   * \return If success return the AID of the station, otherwise 0.
   */
  uint16_t SendAssocResp (Mac48Address to, bool success);
  /**
   * Get the duration of a polling period.
   * \param pollFrameTxTime The TX time of a poll frame.
   * \param sprFrameTxTime The TX time of an SPR frame.
   * \param polledStationsCount The number of stations to be polled.
   * \return The corresponding polling period duration.
   */
  Time GetPollingPeriodDuration (Time pollFrameTxTime, Time sprFrameTxTime, uint8_t polledStationsCount);
  /**
   * Start Polling Period for dynamic allocation of service period.
   */
  void StartPollingPeriod (void);
  /**
   * Polling Period for dynamic allocation of service period has completed.
   */
  void PollingPeriodCompleted (void);
  /**
   * Start Grant Period for dynamic allocation of service period.
   */
  void StartGrantPeriod (void);
  /**
   * Send Grant frame(s) for DMG STAs during GP period.
   */
  void SendGrantFrames (void);
  /**
   * Grant Period for dynamic allocation of service period has completed.
   */
  void GrantPeriodCompleted (void);
  /**
   * GetOffsetOfSprTransmission
   * \param index
   * \return
   */
  Time GetOffsetOfSprTransmission (uint32_t index);
  /**
   * Get Duration Of ongoing Poll Transmission.
   * \return
   */
  Time GetDurationOfPollTransmission (void);
  /**
   * Get Poll Response Offset in MicroSeconds.
   * \return The response offset value to be used in the Poll Frame.
   */
  Time GetResponseOffset (void);
  /**
   * Get Poll frame header duration.
   * \return The duration value of the Poll frame in MicroSeconds
   */
  Time GetPollFrameDuration (void);
  /**
   * Send Poll frame to the specified DMG STA.
   * \param to The MAC address of the DMG STA.
   */
  void SendPollFrame (Mac48Address to);
  /**
   * Send Grant frame to a specified DMG STA.
   * \param to The MAC address of the DMG STA.
   * \param duration The duration of the grant frame as described in 802.11ad-2012 8.3.1.13.
   * \param info The dynamic allocation info field.
   */
  void SendGrantFrame (Mac48Address to, Time duration, DynamicAllocationInfoField &info, BF_Control_Field &bf);
  /**
   * Send directional Announce frame to DMG STA.
   * \param to The MAC address of the DMG STA.
   */
  void SendAnnounceFrame (Mac48Address to);
  /**
   * Get DMG Operation element.
   * \return Pointer to the DMG Operation element.
   */
  Ptr<DmgOperationElement> GetDmgOperationElement (void) const;
  /**
   * Get EDMG Operation element.
   * \return Pointer to the EDMG Operation element.
   */
  Ptr<EdmgOperationElement> GetEdmgOperationElement (void) const;
  /**
   * Get Next DMG ATI Information element.
   * \return The DMG ATI information element.
   */
  Ptr<NextDmgAti> GetNextDmgAtiElement (void) const;
  /**
   * Get Extended Schedule element.
   * \return The extended schedule element.
   */
  Ptr<ExtendedScheduleElement> GetExtendedScheduleElement (void) const;
  /**
   * Get EDMG Training Field Schedule Element.
   * \return The EDMG Training Field Schedule Element.
   */
  Ptr<EdmgTrainingFieldScheduleElement> GetEdmgTrainingFieldScheduleElement (void) const;
  /**
   * Cleanup non-static allocations. This is method is called after the transmission of the last DMG Beacon.
   */
  void CleanupAllocations (void);
  /**
   * Calculate BTI access period variables.
   */
  void CalculateBTIVariables (void);
  /**
   * Send One DMG Beacon frame with the provided arguments.
   */
  void SendOneDMGBeacon (void);
  /**
   * Get Beacon Header Interval Duration
   * \return The duration of BHI.
   */
  Time GetBHIDuration (void) const;
  /**
   * \return the next Association ID to be allocated by the DMG PCP/AP.
   */
  uint16_t GetNextAssociationId (void);
  /**
   * Calculates the duration of the EDMG TRN-R subfields appended to beacons. Depends on the TRN Sequence Length,
   * the number of Tx Chains used to transmit the packet and whether aggregation is used as described in
   * section 29.12.3, 802.11ay - Draft 4.
   * \return EDMG TRN-R Subfield Duration.
   */
  double CalculateEdmgTrnSubfieldDuration (void) const;
  /**
   * Defines groups of MU capable EDMG STAs to perform DL MU-MIMO beamforming training and transmissions.
   * Creates and updates the EDMG Group ID Set element, which is then announced in the beacons transmitted.
   */
  void CreateEdmgMuMimoGroups (void);

private:
  /** DMG PCP/AP Power Status **/
  bool m_startedAP;                     //!< Flag to indicate whether we started DMG AP.

  /** Assoication Information **/
  std::map<uint16_t, Mac48Address> m_staList;//!< Map of all stations currently associated to the DMG PCP/AP with their association ID.

  /** BTI Period Variables **/
  Ptr<DmgBeaconTxop> m_beaconTxop;        //!< Dedicated Txop for DMG Beacons.
  EventId m_beaconEvent;		//!< Event to generate one DMG Beacon.
  Time m_btiStarted;                    //!< The time at which we started BTI access period.
  Time m_beaconTrnFieldsDuration;       //!< Duration of the TRN fields appended to the beacon.S
  Time m_dmgBeaconDuration;             //!< Exact DMG beacon duration.
  Time m_dmgBeaconDurationUs;           //!< DMG BEacon Duration in Microseconds.
  Time m_nextDmgBeaconDelay;            //!< DMG Beacon transmission delay due to difference in clock + duration of TRN fields if there are any.
  Time m_btiDuration;                   //!< The length of the Beacon Transmission Interval (BTI).
  bool m_beaconRandomization;           //!< Flag to indicate whether we want to randomize selection of DMG Beacon at each BI.
  Ptr<RandomVariableStream> m_beaconJitter; //!< RandomVariableStream used to randomize the time of the first DMG beacon.
  bool m_enableBeaconJitter;            //!< Flag whether the first beacon should be generated at random time.
  bool m_allowBeaconing;                //!< Flag to indicate whether we want to start Beaconing upon initialization.
  bool m_announceDmgCapabilities;       //!< Flag to indicate whether we announce DMG Capabilities in DMG Beacons.
  bool m_announceEdmgCapabilities;      //!< Flag to indicate whether we announce EDMG Capabilities in DMG Beacons.
  //bool m_announceBeamformingCapability; //!< Flag to indicate whether the Beamforming Capability Subelement is transmitted as part of EDMG Capabilites.
  bool m_announceOperationElement;      //!< Flag to indicate whether we transmit DMG operation element in DMG Beacons.
  bool m_scheduleElement;               //!< Flag to indicate whether we transmit Extended Schedule element in DMG Beacons.
  bool m_isABFTResponderTXSS;           //!< Flag to indicate whether the responder in A-BFT is TXSS or RXSS.
  bool m_announceTrainingSchedule;      //!< Flag to indicate whether we transmit the schedule for TRN-R fields in DMG Beacons.
  std::vector<Mac48Address> m_beamformingInDTI; //!< List of the stations to train in DTI because beamforming is not completed in BTI.
  uint8_t m_trnUnitsBeacon;             //!< Number of TRN-R units appended  to EDMG Beacons.
  bool m_firstBeacon;                   //!< Flag to identify the first DMG Beacon that we send.
  bool m_groupTraining;                 //!< Flag to indicate whether we beamforming training using TRN fields in Beacons is enabled or not.

  /** DMG PCP/AP Clustering **/
  bool m_enableDecentralizedClustering; //!< Flag to inidicate if decentralized clustering is enabled.
  bool m_enableCentralizedClustering;   //!< Flag to indicate if centralized clustering is enabled.
  Mac48Address m_ClusterID;             //!< The ID of the cluster.
  uint8_t m_clusterMaxMem;              //!< The maximum number of cluster members.
  uint8_t m_beaconSPDuration;           //!< The size of a Beacon SP in MicroSeconds.
  ClusterMemberRole m_clusterRole;      //!< The role of the node in the current cluster.
  typedef std::map<uint8_t, bool> BEACON_SP_STATUS_MAP;                 //!< Typedef for mapping the status of each BeaconSP.
  typedef BEACON_SP_STATUS_MAP::const_iterator BEACON_SP_STATUS_MAP_CI; //!< Typedef for const iterator through BeaconSP.
  BEACON_SP_STATUS_MAP m_spStatus;      //!< The status of each Beacon SP in the monitor period.
  bool m_monitoringChannel;             //!< Flag to indicate if we have started monitoring the channel for cluster formation.
  bool m_beaconReceived;                //!< Flag to indicate if we have received beacon during BeaconSP.
  uint8_t m_selectedBeaconSP;           //!< Selected Beacon SP for DMG Transmission.
  Time m_clusterTimeInterval;           //!< The interval between two consectuve Beacon SPs.
  Time m_channelMonitorTime;            //!< The channel monitor time.
  Time m_startedMonitoringChannel;      //!< The time we started monitoring channel for DMG Beacons.
  Time m_clusterBeaconSPDuration;       //!< The duration of the Beacon SP.

  TracedCallback<Mac48Address, uint8_t> m_joinedCluster;  //!< The PCP/AP has joined a cluster.
  /**
   * TracedCallback signature for DTI access period start event.
   *
   * \param clusterID The MAC address of the cluster.
   * \param beaconSP The index of the BeaconSP.
   */
  typedef void (* JoinedClusterCallback)(Mac48Address clusterID, uint8_t index);

  /** A-BFT Access Period Variables **/
  uint8_t m_abftPeriodicity;            //!< The periodicity of the A-BFT in DMG Beacon.
  EventId m_sswFbckEvent;               //!< Event related to sending SSW FBCK.
  /* Ensure only one DMG STA is communicating with us during single A-BFT slot */
  bool m_receivedOneSSW;                //!< Flag to indicate if we received SSW Frame during SSW-Slot in A-BFT period.
  bool m_abftCollision;                 //!< Flad to indicate if we experienced any collision in the current A-BFT slot.
  Mac48Address m_peerAbftStation;       //!< The MAC address of the station we received SSW from.
  uint8_t m_remainingSlots;
  Time m_atiStartTime;                  //!< The start time of ATI Period.

  /** BRP Phase Variables **/
  typedef std::map<Mac48Address, bool> STATION_BRP_MAP;
  STATION_BRP_MAP m_stationBrpMap;      //!< Map to indicate if a station has conducted BRP Phase or not.

  /**
   * Type definition for storing IEs of the associated stations.
   */
  typedef std::map<Mac48Address, WifiInformationElementMap> AssociatedStationsInformation;
  typedef AssociatedStationsInformation::iterator AssociatedStationsInformationI;
  typedef AssociatedStationsInformation::const_iterator AssociatedStationsInformationCI;
  AssociatedStationsInformation m_associatedStationsInfoByAddress;
  std::map<uint16_t, WifiInformationElementMap> m_associatedStationsInfoByAid;

  /** Beacon Interval **/
  TracedCallback<Mac48Address> m_biStarted;                 //!< Trace Callback for starting new Beacon Interval.
  TracedCallback<Mac48Address, Time> m_btiStart;               //!< Trace Callback for starting A-BFT within the BI.
  /**
   * TracedCallback signature for BTI access period start event.
   *
   * \param address The MAC address of the station.
   * \param duration The duration of the BTI period.
   */
  typedef void (* BTIStartedCallback)(Mac48Address address, Time duration);
  TracedCallback<Mac48Address, Time> m_abftStarted;               //!< Trace Callback for starting A-BFT within the BI.
  /**
   * TracedCallback signature for A-BFT access period start event.
   *
   * \param address The MAC address of the station.
   * \param duration The duration of the A-BFT period.
   */
  typedef void (* ABFTStartedCallback)(Mac48Address address, Time duration);


  /** Traffic Stream Allocation **/
  TracedCallback<Mac48Address, DmgTspecElement> m_addTsRequestReceived;   //!< DMG ADDTS Request received.
  /**
   * TracedCallback signature for receiving ADDTS Request.
   *
   * \param address The MAC address of the station.
   * \param element The TSPEC information element.
   */
  typedef void (* AddTsRequestReceivedCallback)(Mac48Address address, DmgTspecElement element);

  /** Dynamic Allocation of Service Period **/
  bool m_initiateDynamicAllocation;                 //!< Flag to indicate whether to commence PP phase at the beginning of the DTI.
  uint32_t m_polledStationsCount;                   //!< Number of DMG STAs to poll.
  uint32_t m_polledStationIndex;                    //!< The index of the current station being polled.
  uint32_t m_grantIndex;                            //!< The index of the current Grant frame.
  Time m_responseOffset;                            //!< Response offset in the current Poll frame.
  Time m_pollFrameTxTime;                           //!< The Tx duration of Poll frame.
  Time m_sprFrameTxTime;                            //!< The Tx duration of SPR frame.
  Time m_grantFrameTxTime;                          //!< The Tx duration of Grant frame.
  std::vector<Mac48Address> m_pollStations;         //!< List of stations to poll during the PP phase.
  AllocationDataList m_sprList;                     //!< List of info received in the SPRs.
  AllocationDataList m_grantList;                   //!< List of allocation info to be assigned during the Grant Period.
  TracedCallback<Mac48Address> m_ppCompleted;       //!< Polling period has ended up.
  TracedCallback<Mac48Address> m_gpCompleted;       //!< Grant period has ended up.
  DynamicAllocationInfoField n_grantDynamicInfo;    //!< The dynamic information of the current Grant frame.

  /* Channel Quality Measurement */
  TracedCallback<Mac48Address, Ptr<DirectionalChannelQualityReportElement> > m_qualityReportReceived;   //!< Received Quality Report
  /**
   * TracedCallback signature for receiving Channel Quality Report.
   *
   * \param address The MAC address of the station.
   * \param element The Directional Quality Report information element.
   */
  typedef void (* QualityReportReceivedCallback)(Mac48Address address, Ptr<DirectionalChannelQualityReportElement> element);

};

} // namespace ns3

#endif /* DMG_AP_WIFI_MAC_H */
