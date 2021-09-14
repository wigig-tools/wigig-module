/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2020 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#ifndef DMG_STA_WIFI_MAC_H
#define DMG_STA_WIFI_MAC_H

#include "dmg-wifi-mac.h"

#include "ns3/event-id.h"
#include "ns3/packet.h"
#include "ns3/traced-callback.h"

#include "supported-rates.h"
#include "amsdu-subframe-header.h"

#include "ns3/traced-value.h"
#include "mgt-headers.h"

namespace ns3  {

class UniformRandomVariable;

/**
 * \ingroup wifi
 *
 * Struct to hold information regarding DMG beacons observed.
 */
struct DmgApInfo
{
  Mac48Address m_bssid;               //!< BSSID.
  Mac48Address m_apAddr;              //!< DMG PCP/AP MAC address.
  double m_snr;                       //!< SNR in linear scale.
  bool m_activeProbing;               //!< Flag whether active probing is used or not.
  ExtDMGBeacon m_beacon;              //!< DMG Beacon header.
  MgtProbeResponseHeader m_probeResp; //!< Probe Response header.
};

typedef std::vector<DmgApInfo> DmgApInfoList;       //!< List of DMG PCP/APs.
typedef DmgApInfoList::iterator DmgApInfoList_I;    //!< Type definition for iterator over DMG PCP/APs.

enum TransmissionLink {
  DIRECT_LINK = 0,
  RELAY_LINK = 1
};

enum STA_ROLE {
  SOURCE_STA = 0,
  DESTINATION_STA = 1,
  RELAY_STA = 2
};

struct RELAY_LINK_INFO {
  bool relayForwardingActivated;              //!< Flag to indicate if a relay link has been aactivated.
  bool relayLinkEstablished;                  //!< Flag to indicate if a relay link has been established.
  bool rdsDuplexMode;                         //!< The duplex mode of the RDS.
  bool waitingDestinationRedsReports;         //!< Flag to indicate that we are waiting for the destination REDS report.
  bool switchTransmissionLink;                //!< Flag to indicate that we want to change the current transmission link.
  bool tearDownRelayLink;

  uint8_t relayLinkChangeInterval;            //!< Relay Link Change Interval reported by the source REDS (MicroSeconds).
  uint8_t relayDataSensingTime;               //!< Relay Data Sensing Time reported by the source REDS (MicroSeconds).
  uint16_t relayFirstPeriod;                  //!< Relay First Period reported by the source REDS (MicroSeconds).
  uint16_t relaySecondPeriod;                 //!< Relay Second Period reported by the source REDS (MicroSeconds).

  /* Identifiers of the relay protected service period */
  uint16_t srcRedsAid;                        //!< The AID of the source REDS.
  uint16_t dstRedsAid;                        //!< The AID of the destination REDS.
  uint16_t selectedRelayAid;                  //!< The AID of the selected RDS.
  Mac48Address srcRedsAddress;                //!< The MAC address of the Source REDS.
  Mac48Address dstRedsAddress;                //!< The MAC address of the destination REDS.
  Mac48Address selectedRelayAddress;          //!< The MAC address of the selected RDS.
  TransmissionLink transmissionLink;          //!< Current transmission link for the service period.

  RelayCapabilitiesInfo rdsCapabilitiesInfo;            //!< The relay capabilities Information for the RDS.
  RelayCapabilitiesInfo dstRedsCapabilitiesInfo;        //!< The relay capabilities Information for the destination REDS.
  ChannelMeasurementInfoList channelMeasurementList;    //!< The channel measurement list between the source REDS and the RDS.
};

/**
 * Callback signature for reporting channel measurements for relay selection.
 *
 * \param address The MAC address of the station.
 * \param duration The duration of the DTI period.
 */
typedef Callback<uint8_t, ChannelMeasurementInfoList, ChannelMeasurementInfoList, Mac48Address &> ChannelMeasurementCallback;
typedef std::pair<uint8_t, uint8_t> SERVICE_PERIOD_PAIR;      //!< Typedef to identify source and destination DMG STA for SP.
typedef SERVICE_PERIOD_PAIR REDS_PAIR;                        //!< Typedef to identify source and destination REDS protected by an RDS.

#define dot11RSSRetryLimit              8
#define dot11RSSBackoff                 8
#define aMaxABFTAccessPeriod            2

typedef Callback<DynamicAllocationInfoField, Mac48Address, BF_Control_Field &> ServicePeriodRequestCallback;

/**
 * \ingroup wifi
 *
 * The Wifi MAC high model for a non-DMG PCP/AP STA in a DMG BSS.
 */
class DmgStaWifiMac : public DmgWifiMac
{
public:
  static TypeId GetTypeId (void);

  DmgStaWifiMac ();
  virtual ~DmgStaWifiMac ();
  /**
   * \param stationManager the station manager attached to this MAC.
   */
  virtual void SetWifiRemoteStationManager(Ptr<WifiRemoteStationManager> stationManager);
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
   * \param lost The number of beacons which must be lost
   * before a new association sequence is started.
   */
  void SetMaxLostBeacons (uint32_t lost);
  /**
   * \param timeout
   *
   * If no probe response is received within the specified
   * timeout, the station sends a new probe request.
   */
  void SetProbeRequestTimeout (Time timeout);
  /**
   * \param timeout
   *
   * If no association response is received within the specified
   * timeout, the station sends a new association request.
   */
  void SetAssocRequestTimeout (Time timeout);
  /**
   * Start an active association sequence immediately.
   */
  void StartActiveAssociation (void);
  /**
   * Request Information regarding station capabilities.
   * \param stationAddress The address of the station to obtain its capabilities.
   * \param list A list of the requested element IDs.
   */
  void RequestInformation (Mac48Address stationAddress, WifiInformationElementIdList &list);
  /**
   * Request Information regarding station DMG capabilities and relay capabilities.
   * \param stationAddress The address of the station to obtain its capabilities.
   */
  void RequestRelayInformation (Mac48Address stationAddress);
  /**
   * Discover all the available relays in DMG BSS. This method is called by the source REDS.
   * \param stationAddress The MAC address of the station to establish relay link with (Destination REDS).
   */
  void StartRelayDiscovery (Mac48Address stationAddress);
  /**
   * Initiate Relay Link Switching Type Procedure with the selected RDS in the RDS Selection procedure.
   */
  void StartRlsProcedure (void);
  /**
   * Tear down relay operation by either the RDS or the source REDS.
   * \param sourceAid The AID of the source REDS.
   * \param destinationAid The AID of the destination REDS.
   * \param relayAid The AID of the RDS.
   */
  void TeardownRelay (uint16_t sourceAid, uint16_t destinationAid, uint16_t relayAid);
  /**
   * Tear down relay operation by either the RDS or the source REDS.
   * \param sourceAid The AID of the source REDS.
   * \param destinationAid The AID of the destination REDS.
   * \param relayAid The AID of the RDS.
   */
  void TeardownRelayImmediately (uint16_t sourceAid, uint16_t destinationAid, uint16_t relayAid);
  /**
   * Send Channel Measurement Request to specific DMG STA.
   * \param to The MAC address of the destination STA.
   * \param token The dialog token.
   */
  void SendChannelMeasurementRequest (Mac48Address to, uint8_t token);
  /**
   * Register relay selector function for custom relay selection.
   * \param callback
   */
  void RegisterRelaySelectorFunction (ChannelMeasurementCallback callback);
  /**
   * Switch transmission link for a specific service period to an alternative path.
   * \param srcAid The AID of the source DMG STA.
   * \param dstAid The AID of the destination DMG STA.
   */
  void SwitchTransmissionLink (uint8_t srcAid, uint8_t dstAid);
  /**
   * Get Association Identifier (AID).
   * \return The AID of the station.
   */
  virtual uint16_t GetAssociationID (void);
  /**
   * Create or modify DMG allocation for the transmission of frames between DMG STA
   * that are members of a PBSS or a DMG infrastructure.
   * \param elem The DMG TSPEC information element.
   */
  void CreateAllocation (DmgTspecElement elem);
  /**
   * Delete existing DMG Allocation.
   * \param reason The reason for deleting the DMG Allocation.
   * \param allocationInfo Information about the allocation to be deleted.
   */
  void DeleteAllocation (uint16_t reason, DmgAllocationInfo &allocationInfo);
  /**
   * Register service period request function for custom resource request.
   * \param callback
   */
  void RegisterSPRequestFunction (ServicePeriodRequestCallback callback);
  /**
   * Communicate in service period (temporary solution for Dynamic Allocation).
   * \param peerAddress The MAC address of the peer station.
   */
  void CommunicateInServicePeriod (Mac48Address peerAddress);
  /**
   * Return whether we are associated with a DMG AP.
   *
   * \return true if we are associated with a DMG AP, false otherwise
   */
  bool IsAssociated (void) const;
  /**
   * Initiate TXSS in CBAP allocation using TxOP.
   * \param peerAddress The address of the DMG STA to perform beamforming training with.
   */
  virtual void Perform_TXSS_TXOP (Mac48Address peerAddress);

protected:
  friend class MultiBandNetDevice;

  virtual void DoDispose (void);
  virtual void DoInitialize (void);
  void StartBeaconInterval (void);
  void EndBeaconInterval (void);
  void StartBeaconTransmissionInterval (void);
  void StartAssociationBeamformTraining (void);
  void StartAnnouncementTransmissionInterval (void);
  void StartDataTransmissionInterval (void);
  void FrameTxOk (const WifiMacHeader &hdr);
  void BrpSetupCompleted (Mac48Address address);
  virtual void NotifyBrpPhaseCompleted (void);

  /**
   * Resume any Pending TXSS requests in the current CBAP allocation.
   */
  virtual void ResumePendingTXSS (void);
  /**
   * End Association Beamform Training (A-BFT) period.
   */
  void EndAssociationBeamformTraining (void);
  /**
   * StartChannelQualityMeasurement
   * \param element
   */
  void StartChannelQualityMeasurement (Ptr<DirectionalChannelQualityRequestElement> element);
  /**
   * ReportChannelQualityMeasurement
   * \param list
   */
  void ReportChannelQualityMeasurement (TimeBlockMeasurementList list);
  /**
   * Send Directional Channel Quality Report.
   * \param element The Directional Channel Quality Report Information Element.
   */
  void SendDirectionalChannelQualityReport (Ptr<DirectionalChannelQualityReportElement> element);
  /**
   * Forward an Action Frame to the specified destination.
   * \param to
   * \param actionHdr
   * \param actionBody
   */
  void ForwardActionFrame (Mac48Address to, WifiActionHeader &actionHdr, Header &actionBody);
  /**
   * Get Relay Transfer Parameter Set associated with this DMG STA.
   * \return The relay transfer parameter set.
   */
  Ptr<RelayTransferParameterSetElement> GetRelayTransferParameterSet (void) const;
  /**
   * Initiate Relay Link Switch (RLS) procedure.
   * \param to The MAC address of the RDS.
   * \param token The dialog token.
   * \param sourceAid The AID of the source REDS.
   * \param relayAid The AID of the selected RDS.
   * \param destinationAid The AID of the desination REDS.
   */
  void SendRlsRequest (Mac48Address to, uint8_t token, uint16_t sourceAid, uint16_t relayAid, uint16_t destinationAid);
  /**
   * Send RLS Response to the specified Station.
   * \param to The MAC address of the station.
   * \param token The dialog token.
   * \param destinationStatus The status of the destination REDS.
   * \param relayStatus The status of the RDS.
   */
  void SendRlsResponse (Mac48Address to, uint8_t token, uint16_t destinationStatus, uint16_t relayStatus);
  /**
   * Send RLS Announcment to the specified Station.
   * \param to The MAC address of the PCP/AP.
   * \param destinationAid The AID of the desination REDS.
   * \param relayAid The AID of the selected RDS.
   * \param sourceAid The AID of the source REDS.
   */
  void SendRlsAnnouncment (Mac48Address to, uint16_t destinationAid, uint16_t relayAid, uint16_t sourceAid);
  /**
   * Send Relay Teardown frame to the specified Station.
   * \param to
   * \param sourceAid The AID of the source REDS.
   * \param destinationAid The AID of the desination REDS.
   * \param relayAid The AID of the selected RDS.
   */
  void SendRelayTeardown (Mac48Address to, uint16_t sourceAid, uint16_t destinationAid, uint16_t relayAid);
  /**
   * Remove Relay Entry
   * \param sourceAid The AID of the source REDS.
   * \param destinationAid The AID of the desination REDS.
   */
  void RemoveRelayEntry (uint16_t sourceAid, uint16_t destinationAid);

  Ptr<MultiBandElement> GetMultiBandElement (void) const;
  /**
   * Set the current MAC state.
   *
   * \param value the new state
   */
  void SetState (MacState value);
  /**
   * Get STA Availability Element.
   * \return Pointer to the STA availabiltiy element.
   */
  Ptr<StaAvailabilityElement> GetStaAvailabilityElement (void) const;
  /**
   * Forward data frame to another DMG STA. This function is called during HD-DF relay operation ode.
   * \param hdr The MAC Header.
   * \param packet
   * \param destAddress The MAC address of the receiving station.
   */
  void ForwardDataFrame (WifiMacHeader hdr, Ptr<Packet> packet, Mac48Address destAddress);
  /**
   * Do Association Beamforming Training in the A-BFT period.
   * \param currentSlotIndex The index of the  current SSW slot in A-BFT.
   */
  void DoAssociationBeamformingTraining (uint8_t currentSlotIndex);
  /**
   * Send Relay Search Request.
   * \param token The dialog token.
   * \param destinationAid The AID of the destination DMG STA.
   */
  void SendRelaySearchRequest (uint8_t token, uint16_t destinationAid);
  /**
   * Send Channel Measurement Report.
   * \param to The address of the DMG STA which sent the measurement request.
   * \param token The token dialog.
   * \param List of channel measurement information between sending station and other stations.
   */
  void SendChannelMeasurementReport (Mac48Address to, uint8_t token, ChannelMeasurementInfoList &measurementList);
  /**
   * Schedule all the allocation blocks.
   * \param field The allocation field information..
   * \param role The role of the STA in the relay period.
   */
  void ScheduleAllocationBlocks (AllocationField &field, STA_ROLE role);
  /**
   * Schedule an allocation block protected by relay operation mode.
   * \param id The allocation identifier.
   * \param role The role of the STA in the relay period.
   */
  void InitiateAllocationPeriod (AllocationID id, uint8_t srcAid, uint8_t dstAid, Time spLength, STA_ROLE role);
  /**
   * Initiate and schedule periods related to relay operation.
   * \param info Information regarding relay link.
   */
  void InitiateRelayPeriods (RELAY_LINK_INFO &info);
  /**
   * End Relay Period.
   * \param pair The pair of REDS to which we establsihed the relay period.
   */
  void EndRelayPeriods (REDS_PAIR &pair);
  /**
   * This function is called upon the expiration of RelayLinkChangeIntervalTimeout.
   * \param offset
   */
  void RelayLinkChangeIntervalTimeout (void);
  /**
   * Check if there is an available time for a given period in the current allocated Service Period.
   * \param periodDuration The duration of the period.
   * \return True if the period can exist in the current service period.
   */
  bool CheckTimeAvailabilityForPeriod (Time servicePeriodDuration, Time partialDuration);
  /**
   * Start Full Duplex Relay operation.
   * \param allocationID The ID of the current allocation.
   * \param servicePeriodLength The length of the service period for which we are using the relay.
   * \param involved Flag to indicate if we are involved in relay forwarding.
   */
  void StartFullDuplexRelay (AllocationID allocationID, Time length, uint8_t peerAid, Mac48Address peerAddress, bool isSource);
  /**
   * Start Half Duplex Relay operation.
   * \param allocationID The ID of the current allocation.
   * \param servicePeriodLength The length of the service period for which we are using the relay.
   * \param involved Flag to indicate if we are involved in relay forwarding.
   */
  void StartHalfDuplexRelay (AllocationID allocationID, Time servicePeriodLength, bool involved);

  void StartRelayFirstPeriodAfterSwitching (void);
  /**
   * Start the First Period related to the HD-DF Relay operation mode.
   */
  void StartRelayFirstPeriod (void);
  /**
   * Start the Second Period related to the HD-DF Relay operation mode.
   */
  void StartRelaySecondPeriod (void);
  /**
   * Suspend transmission in the current period related to the HD-DF Relay operation mode.
   */
  void SuspendRelayPeriod (void);
  /**
   * Relay First Period Timeout.
   */
  void RelayFirstPeriodTimeout (void);
  /**
   * Relay Second Period Timeout.
   */
  void RelaySecondPeriodTimeout (void);
  /**
   * MissedAck
   * \param hdr
   */
  void MissedAck (const WifiMacHeader &hdr);
  /**
   * This function is called upon the expiration of Relay Data Sensing Timeout.
   */
  void RelayDataSensingTimeout (void);
  /**
   * Switch to Relay Opertional Mode. This method is called by the RDS.
   */
  void SwitchToRelayOpertionalMode (void);
  /**
   * Relay Operation Timeout.
   */
  void RelayOperationTimeout (void);
  /**
   * The packet we sent was successfully received by the receiver
   * (i.e. we received an ACK from the receiver).  If the packet
   * was an association response to the receiver, we record that
   * the receiver is now associated with us.
   *
   * \param hdr the header of the packet that we successfully sent
   */
  virtual void TxOk (Ptr<const Packet> packet, const WifiMacHeader &hdr);
  /**
   * BeamLink Maintenance Timeout.
   */
  virtual void BeamLinkMaintenanceTimeout (void);
  /**
   * Return the DMG capability of the current STA.
   *
   * \return the DMG capability that we support
   */
  Ptr<DmgCapabilities> GetDmgCapabilities (void) const;

private:
  /**
   * Enable or disable active probing.
   *
   * \param enable enable or disable active probing
   */
  void SetActiveProbing (bool enable);
  /**
   * Return whether active probing is enabled.
   *
   * \return true if active probing is enabled, false otherwise
   */
  bool GetActiveProbing (void) const;
  /**
   * Handle a received packet.
   *
   * \param mpdu the received MPDU
   */
  void Receive (Ptr<WifiMacQueueItem> mpdu);

  /**
   * Update list of candidate DMG PCP/APs to associate. The list should contain DmgApInfo sorted from
   * best to worst SNR, with no duplicate.
   *
   * \param newApInfo the new DmgApInfo to be inserted
   */
  void UpdateCandidateApList (DmgApInfo &newApInfo);
  /**
   * Forward a probe request packet to the non-QoS Txop. The standard is not clear on the correct
   * queue for management frames if QoS is supported. We always use the non-QoS Txop.
   */
  void SendProbeRequest (void);
  /**
   * Forward an association or reassociation request packet to the non-QoS Txop.
   * The standard is not clear on the correct queue for management frames if QoS is supported.
   * We always use the non-QoS Txop.
   * \param isReassoc flag whether it is a reassociation request
   */
  void SendAssociationRequest (bool isReassoc);
  /**
   * Try to ensure that we are associated with an DMG PCP/AP by taking an appropriate action
   * depending on the current association status.
   */
  void TryToEnsureAssociated (void);
  /**
   * This method is called after the association timeout occurred. We switch the state to
   * WAIT_ASSOC_RESP and re-send an association request.
   */
  void AssocRequestTimeout (void);
  /**
   * Return whether we have completed beamforming training with an AP.
   *
   * \return true if we have completed beamforming training with an AP, false otherwise
   */
  bool IsBeamformedTrained (void) const;
  /**
   * Return whether we are waiting for an association response from an AP.
   *
   * \return true if we are waiting for an association response from an AP, false otherwise
   */
  bool IsWaitAssocResp (void) const;
  /**
   * This method is called after we have not received a beacon from the AP
   */
  void MissedBeacons (void);
  /**
   * Restarts the beacon timer.
   *
   * \param delay the delay before the watchdog fires
   */
  void RestartBeaconWatchdog (Time delay);
  /**
   * Failed to perform RSS in A-BFT period.
   */
  void FailedRssAttempt (void);
  /**
   * Missed SSW-Feedback from PCP/AP during A-BFT.
   */
  void MissedSswFeedback (void);
  /**
   * Send Service Period Request (SPR) Frame.
   * \param to The MAC address of the PCP/AP.
   * \param duration The duration till the end of the polling period.
   * \param info The dynamic allocation information to be requested.
   * \param bfField The beamforming training field.
   */
  void SendSprFrame (Mac48Address to, Time duration, DynamicAllocationInfoField &info, BF_Control_Field &bfField);
  /**
   * Start A-BFT Sector Sweep Slot.
   */
  void StartSectorSweepSlot (void);
  /**
   * Start Responder Sector Sweep (RSS) Phase during A-BFT access period.
   * \param stationAddress The address of the station.
   */
  void StartAbftResponderSectorSweep (Mac48Address address);
  /**
   * Start the scanning process which trigger active or passive scanning based on the
   * active probing flag.
   */
  void StartScanning (void);
  /**
   * This method is called after wait beacon timeout or wait probe request timeout has
   * occurred. This will trigger association process from beacons or probe responses
   * gathered while scanning.
   */
  void ScanningTimeout (void);
  /**
   * Map the Tx and RX config during the reception of a TRN field to the SNR measured at the reception of the
   * field and store them to determine the optimal antenna configuration.
   * A callback to be hooked with the DmgWifiPhy class.
   * \param antennaId the ID of the antenna used for receiving the TRN field
   * \param sectorId the ID of the sector used for receiving the TRN field
   * \param awvId the ID of the AWV used for receiving the TRN field
   * \param trnUnitsRemaining The number of remaining TRN Units.
   * \param subfieldsRemaining The number of remaining TRN Subfields within the TRN Unit.
   * \param snr The measured SNR over specific TRN.
   */
  void RegisterBeaconSnr (AntennaID antennaId, SectorID sectorId, AWV_ID awvId, uint8_t trnUnitsRemaining, uint8_t subfieldsRemaining, double snr, Mac48Address apAddress);
  /**
   * Start the Group Beamforming Procedure. To be called at the start of a DTI if an STA supports the 802.11ay protocol
   * and there were TRN fields appended to the beacons transmitted in the current BI. Determines the optimal
   * antenna configurations according to the measured SNR. Updates the best RX or TX and RX configs for the station
   * and if there is a change in the best transmit config for the AP sends an unsolicited Information Response.
   */
  void StartGroupBeamformingTraining (void);
  /**
   * Updates the SNR table for the AP (m_stationSnrMap) according to the newest SNR values measured when receiving the
   * TRN fields appended to the beacons. For each TX config of the AP, the SNR from the best Rx config is taken.
   */
  void UpdateSnrTable (Mac48Address apAddress);
  /**
   * Checks if there has been a change in the best Tx config determined for the AP from the previous training.
   * \param newTxConfig The optimal Tx config determined during the last training
   * \return true if there has been a change in configuration, false otherwise
   */
  bool DetectChangeInConfiguration (ANTENNA_CONFIGURATION_COMBINATION newTxConfig);

private:
  /** Association Variables and Trace Sources **/
  uint16_t m_aid;                               //!< AID of the STA.
  Time m_waitBeaconTimeout;                     //!< wait beacon timeout
  Time m_probeRequestTimeout;                   //!< probe request timeout.
  Time m_assocRequestTimeout;                   //!< Association request timeout
  EventId m_waitBeaconEvent;                    //!< Wait Event for DMG Beacon Reception.
  EventId m_probeRequestEvent;                  //!< Event for Probe request.
  EventId m_assocRequestEvent;                  //!< Event for assoication request.
  EventId m_beaconWatchdog;                     //!< Event for  watch dog timer.
  Time m_beaconWatchdogEnd;                     //!< Watch dog timer end time.
  uint32_t m_maxMissedBeacons;                  //!< Maximum number of beacons we are allowed to miss.
  bool m_activeProbing;                         //!< Flag to indicate whether to perform active probing.
  bool m_activeScanning;                        //!< Flag to indicate whether to perform active network scanning.
  TracedValue<Time> m_beaconArrival;            //!< Time in queue.
  std::vector<DmgApInfo> m_candidateAps;        //!< list of candidate APs to associate to.
  // Note: std::multiset<ApInfo> might be a candidate container to implement
  // this sorted list, but we are using a std::vector because we want to sort
  // based on SNR but find duplicates based on BSSID, and in practice this
  // candidate vector should not be too large.

  /* Data transmission variables  */
  bool m_moreData;                              //! More data field in the last received Data Frame to indicate that the STA
                                                //! has MSDUs or A-MSDUs buffered for transmission to the frame’s recipient
                                                //! during the current SP or TXOP.

  /** BI Parameters **/
  EventId m_abftEvent;                          //!< Event for schedulling Association Beamforming Training channel access period.
  uint8_t m_nBI;                                //!< Flag to indicate the number of intervals to allocate A-BFT.
  uint8_t m_TXSSSpan;                           //!< The number of BIs to cover all the sectors.
  uint8_t m_remainingBIs;                       //!< The number of remaining BIs to cover the TXSS of the AP.
  bool m_completedFragmenteBI;                  //!< Flag to indicate if we have completed BI.
  Time m_nextDti;                               //!< The relative starting time of the next DTI.
  EventId m_dtiStartEvent;                      //!< Event for schedulling DTI channel access period.

  /** BTI Beamforming **/
  bool m_receivedDmgBeacon;
  Ptr<UniformRandomVariable> m_abftSlot;        //!< Random variable for selecting A-BFT slot.
  uint8_t m_remainingSlotsPerABFT;              //!< Remaining Slots in the current A-BFT.
  uint8_t m_currentSlotIndex;                   //!< Current SSW Slot in A-BFT.
  uint8_t m_selectedSlotIndex;                  //!< Selected SSW slot in A-BFT..

  uint32_t m_failedRssAttemptsCounter;          //!< Counter for Failed RSS Attempts during A-BFT.
  uint32_t m_rssAttemptsLimit;                  //!< Maximum Failed RSS Attempts during A-BFT.
  uint32_t m_rssBackoffRemaining;               //!< Remaining BTIs for the RSS in A-BFT.
  uint32_t m_rssBackoffLimit;                   //!< Maximum RSS Backoff value.
  Ptr<UniformRandomVariable> m_rssBackoffVariable;//!< Random variable for the RSS Backoff value.
  bool m_staAvailabilityElement;                //!< Flag to indicate whether we include STA Availability element in DMG Beacon.
  bool m_immediateAbft;                         //!< Flag to indicate if we start A-BFT after receiving fragmented A-BFT.

  /* DMG Relay Support Variables */
  bool m_relayAckRequest;                       //!< Request Relay ACK.
  bool m_relayMode;                             //!< Flag to indicate if we are in relay mode (For RDS).
  bool m_rdsDuplexMode;                         //!< The duplex mode of the RDS.
  uint8_t m_relayLinkChangeInterval;            //!< Relay Link Change Interval reported by the source REDS (MicroSeconds).
  uint8_t m_relayDataSensingTime;               //!< Relay Data Sensing Time reported by the source REDS (MicroSeconds).
  uint16_t m_relayFirstPeriod;                  //!< Relay First Period reported by the source REDS (MicroSeconds).
  uint16_t m_relaySecondPeriod;                 //!< Relay Second Period reported by the source REDS (MicroSeconds).

  TracedCallback<Mac48Address> m_channelReportReceived;  //!< Trace callback for Channel Measurement completion.
  EventId m_linkChangeInterval;                 //!< Event ID for the link change interval.
  EventId m_firstPeriod;                        //!< Event ID for the first period.
  EventId m_secondPeriod;                       //!< Event ID for the second period.
  bool m_relayDataExchanged;                    //!< Flag to indicate a data has been exchanged during the relay operation mode.
  bool m_relayReceivedData;                     //!< Flag to indicate if the RDS has received frame in the HD-DF operation mode.
  bool m_periodProtected;                       //!< Flag to indicate if the current SP allocation is protected by relay operation.

  ChannelMeasurementInfoList m_channelMeasurementList;    //!< The channel measurement list between the source REDS and the RDS.
  ChannelMeasurementCallback m_channelMeasurementCallback;

  typedef std::map<REDS_PAIR, RELAY_LINK_INFO> RELAY_LINK_MAP;  //!< Typedef to identify relay link information for pair of REDS.
  typedef RELAY_LINK_MAP::iterator RELAY_LINK_MAP_ITERATOR;     //!< Typedef for Ralay Link Map Iterator.
  RELAY_LINK_MAP m_relayLinkMap;                                //!< List to store information related to relay links.
  RELAY_LINK_INFO m_relayLinkInfo;                              //!< Information about the relay link being established.

  /**
   * TracedCallback signature for transmission link change event.
   *
   * \param address The MAC address of the station.
   * \param transmissionLink The new transmission link.
   */
  typedef void (* TransmissionLinkChangedCallback)(Mac48Address address, TransmissionLink link);
  TracedCallback<Mac48Address, TransmissionLink> m_transmissionLinkChanged;

  /**
   * TracedCallback signature for .
   *
   * \param address The MAC address of the station.
   * \param transmissionLink The new transmission link.
   */
  typedef void (* RdsDiscoveryCompletedCallback)(RelayCapableStaList rdsList);
  TracedCallback<RelayCapableStaList> m_rdsDiscoveryCompletedCallback;

  /** Dynamic Allocation of Service Period **/
  bool m_pollingPhase;                            //!< Flag to indicate if we participate in the polling phase.
  SERVICE_PERIOD_PAIR m_currentServicePeriod;
  ServicePeriodRequestCallback m_servicePeriodRequestCallback;

  /* Spatial Sharing and Interference Mitigation */
  bool m_supportSpsh;                           //!< Flag to indicate whether we support Spatial Sharing and Interference Mitigation.
  Ptr<DirectionalChannelQualityRequestElement> m_reqElem;

  /** DMG BSS peer and service discovery **/
  TracedCallback<Mac48Address> m_informationReceived;
  bool m_groupTraining;                         //!< Flag to indicate whether we are currently performing beamforming training using TRN fields in Beacons.
  bool m_groupBeamformingFailed;                //!< Flag to indicate whether the previous beamforming training failed (the information response was not received by the AP).
  TracedCallback<Mac48Address> m_updateFailed;
};

} // namespace ns3

#endif /* DMG_STA_WIFI_MAC_H */
