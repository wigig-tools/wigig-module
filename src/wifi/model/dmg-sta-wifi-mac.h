/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
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

namespace ns3  {

class MgtAddBaRequestHeader;
class UniformRandomVariable;

typedef enum {
  DIRECT_LINK = 0,
  RELAY_LINK = 1
} TransmissionLink;

typedef enum {
  SOURCE_STA = 0,
  DESTINATION_STA = 1,
  RELAY_STA = 2
} STA_ROLE;

typedef struct {
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
} RELAY_LINK_INFO;

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
 * The Wifi MAC high model for a non-AP STA in a BSS.
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
  virtual void Enqueue (Ptr<const Packet> packet, Mac48Address to);
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
   * Initiate Beamforming with a particular DMG STA.
   * \param peerAid The AID of the peer DMG STA.
   * \param peerAddress The address of the DMG STA to perform beamforming training with.
   * \param isInitiator Indicate whether we are the Beamforming Initiator.
   * \param isTxss Indicate whether the RSS is TxSS or RxSS.
   * \param length The length of the Beamforming Service Period.
   */
  void StartBeamformingServicePeriod (uint8_t peerAid, Mac48Address peerAddress, bool isInitiator, bool isTxss, Time length);
  /**
   * Start an active association sequence immediately.
   */
  void StartActiveAssociation (void);
  /**
   * Request Information regarding station capabilities.
   * \param stationAddress The address of the station to obtain its capabilities.
   */
  void RequestInformation (Mac48Address stationAddress);
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
  uint16_t GetAssociationID (void);
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

protected:
  friend class MultiBandNetDevice;

  virtual void DoDispose (void);
  virtual void DoInitialize (void);
  void StartBeaconInterval (void);
  void StartBeaconTransmissionInterval (void);
  void StartAssociationBeamformTraining (void);
  void StartAnnouncementTransmissionInterval (void);
  void StartDataTransmissionInterval (void);
  void FrameTxOk (const WifiMacHeader &hdr);
  void BrpSetupCompleted (Mac48Address address);
  virtual void NotifyBrpPhaseCompleted (void);

  void StartChannelQualityMeasurement (Ptr<DirectionalChannelQualityRequestElement> element);
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
  void SetState (enum MacState value);
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
   */
  void DoAssociationBeamformingTraining (void);
  /**
   * Add new data forwarding entry.
   * \param nextHopAddress The MAC Address of the next hop.
   */
  void AddForwardingEntry (Mac48Address nextHopAddress);
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
   * \param id The allocation identifier.
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
   *BeamLink Maintenance Timeout.
   */
  virtual void BeamLinkMaintenanceTimeout (void);

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
  virtual void Receive (Ptr<Packet> packet, const WifiMacHeader *hdr);

  /**
   * Forward a probe request packet to the DCF. The standard is not clear on the correct
   * queue for management frames if QoS is supported. We always use the DCF.
   */
  void SendProbeRequest (void);
  /**
   * Forward an association request packet to the DCF. The standard is not clear on the correct
   * queue for management frames if QoS is supported. We always use the DCF.
   */
  void SendAssociationRequest (void);
  /**
   * Try to ensure that we are associated with an AP by taking an appropriate action
   * depending on the current association status.
   */
  void TryToEnsureAssociated (void);
  /**
   * This method is called after the association timeout occurred. We switch the state to
   * WAIT_ASSOC_RESP and re-send an association request.
   */
  void AssocRequestTimeout (void);
  /**
   * This method is called after the probe request timeout occurred. We switch the state to
   * WAIT_PROBE_RESP and re-send a probe request.
   */
  void ProbeRequestTimeout (void);
  /**
   * Return whether we are associated with an AP.
   *
   * \return true if we are associated with an AP, false otherwise
   */
  bool IsAssociated (void) const;
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
   * Missed SSW-Feedback from PCP/AP during A-BFT.
   */
  void MissedSswFeedback (void);
  /**
   * Return the DMG capability of the current STA.
   *
   * \return the DMG capability that we support
   */
  Ptr<DmgCapabilities> GetDmgCapabilities (void) const;
  /**
   * Send Service Period Request (SPR) Frame.
   * \param to The MAC address of the PCP/AP.
   * \param duration The duration till the end of the polling period.
   * \param info The dynamic allocation information to be requested.
   * \param bfField The beamforming training field.
   */
  void SendSprFrame (Mac48Address to, Time duration, DynamicAllocationInfoField &info, BF_Control_Field &bfField);
  /**
   * Start Initiator Sector Sweep (ISS) Phase.
   * \param stationAddress The address of the station.
   * \param isTxss Indicate whether the RSS is TxSS or RxSS.
   */
  void StartInitiatorSectorSweep (Mac48Address address, bool isTxss);
  /**
   * Start Generic Responder Sector Sweep (RSS) Phase.
   * \param stationAddress The address of the station.
   * \param isTxss Indicate whether the RSS is TxSS or RxSS.
   */
  void StartResponderSectorSweep (Mac48Address stationAddress, bool isTxss);
  /**
   * Start Responder Sector Sweep (RSS) Phase during A-BFT access period.
   * \param stationAddress The address of the station.
   * \param isTxss Indicate whether the RSS is TxSS or RxSS.
   */
  void StartAbftResponderSectorSweep (Mac48Address address, bool isTxss);
  /**
   * Start Transmit Sector Sweep (TxSS) with specific station.
   * \param address The MAC address of the peer DMG STA.
   * \param direction Indicate whether we are initiator or responder.
   */
  void StartTransmitSectorSweep (Mac48Address address, BeamformingDirection direction);
  /**
   * Start Receive Sector Sweep (RxSS) with specific station.
   * \param address The MAC address of the peer DMG STA.
   * \param direction Indicate whether we are initiator or responder.
   */
  void StartReceiveSectorSweep (Mac48Address address, BeamformingDirection direction);
  /**
   * Send ISS Sector Sweep Frame
   * \param address
   * \param direction
   * \param sectorID
   * \param antennaID
   * \param count
   */
  void SendIssSectorSweepFrame (Mac48Address address, BeamformingDirection direction,
                                uint8_t sectorID, uint8_t antennaID,  uint16_t count);
  /**
   * Send RSS Sector Sweep Frame
   * \param address
   * \param direction
   * \param sectorID
   * \param antennaID
   * \param count
   */
  void SendRssSectorSweepFrame (Mac48Address address, BeamformingDirection direction,
                                uint8_t sectorID, uint8_t antennaID,  uint16_t count);
  /**
   * Send SSW Frame.
   * \param sectorID The ID of the current transmitting sector.
   * \param antennaID the ID of the current transmitting antenna.
   * \param count the remaining number of sectors.
   */
  void SendSectorSweepFrame (Mac48Address address, BeamformingDirection direction,
                             uint8_t sectorID, uint8_t antennaID, uint16_t count);
  /**
   * Send SSW FBCK Frame for SLS phase in a scheduled service period.
   * \param receiver The MAC address of the peer DMG STA.
   */
  void SendSswFbckFrame (Mac48Address receiver);
  /**
   * Send SSW ACK Frame for SLS phase in a scheduled service period.
   * \param receiver The MAC address of the peer DMG STA.
   */
  void SendSswAckFrame (Mac48Address receiver);
  /**
   * Record Beamformed Link Maintenance Value
   * \param field The BF Link Maintenance Field in SSW-FBCK/ACK.
   */
  void RecordBeamformedLinkMaintenanceValue (BF_Link_Maintenance_Field field);

private:
  Time m_probeRequestTimeout;
  Time m_assocRequestTimeout;
  EventId m_probeRequestEvent;
  EventId m_assocRequestEvent;
  EventId m_beaconWatchdog;
  Time m_beaconWatchdogEnd;                     //! Watch dog timer end time/
  uint32_t m_maxLostBeacons;                    //! Maximum number of beacons we are allowed to miss.
  bool m_activeProbing;                         //! Flag to indicate whether to perform active probing.
  uint16_t m_aid;                               //! AID of the STA.

  TracedCallback<Mac48Address> m_assocLogger;
  TracedCallback<Mac48Address> m_deAssocLogger;

  bool m_moreData;                              //! More data field in the last received Data Frame to indicate that the STA
                                                //! has MSDUs or A-MSDUs buffered for transmission to the frame’s recipient
                                                //! during the current SP or TXOP.

  /* BI Parameters */
  EventId   m_abftEvent;                        //!< Association Beamforming training Event.
  uint8_t   m_nBI;                              //!< Flag to indicate the number of intervals to allocate A-BFT.

  /* BTI Beamforming */
  bool m_receivedDmgBeacon;
  bool    m_isResponderTXSS;                    //!< Flag to indicate whether the A-BFT Period is TXSS or RXSS.
  uint8_t m_slotIndex;                          //!< The index of the selected slot in the A-BFT period.
  EventId m_sswFbckTimeout;                     //!< Timeout Event for receiving SSW FBCK Frame.
  Ptr<UniformRandomVariable> a_bftSlot;         //!< Random variable for A-BFT slot.
  bool    m_isIssInitiator;                     //!< Flag to indicate that we are ISS.
  EventId m_rssEvent;                           //!< Event related to scheduling RSS.
  uint8_t m_remainingSlotsPerABFT;              //!< Remaining Slots in the current A-BFT.
  uint8_t m_slotOffset;                         //!< Variable used to print the correct slot selected.
  Time m_sswFbckDuration;                       //!< The duration in the SSW-FBCK Field.

  uint32_t m_failedRssAttemptsCounter;          //!< Counter for Failed RSS Attempts during A-BFT.
  uint32_t m_rssAttemptsLimit;                  //!< Maximum Failed RSS Attempts during A-BFT.
  uint32_t m_rssBackoffRemaining;               //!< Remaining BTIs for the RSS in A-BFT.
  uint32_t m_rssBackoffLimit;                   //!< Maximum RSS Backoff value.
  Ptr<UniformRandomVariable> m_rssBackoffVariable;//!< Random variable for the RSS Backoff value.
  bool m_staAvailabilityElement;                //!< Flag to indicate whether we include STA Availability element in DMG Beacon.

  /* DMG Relay Support Variables */
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

  /* Dynamic Allocation of Service Period */
  bool m_pollingPhase;                            //!< Flag to indicate if we participate in the polling phase.
  SERVICE_PERIOD_PAIR m_currentServicePeriod;
  ServicePeriodRequestCallback m_servicePeriodRequestCallback;

  /* Data Forwarding Table */
  typedef struct {
    Mac48Address nextHopAddress;
    bool isCbapPeriod;
  } AccessPeriodInformation;

  typedef std::map<Mac48Address, AccessPeriodInformation> DataForwardingTable;
  typedef DataForwardingTable::iterator DataForwardingTableIterator;
  typedef DataForwardingTable::const_iterator DataForwardingTableCI;
  DataForwardingTable m_dataForwardingTable;

  /* Spatial Sharing and Interference Mitigation */
  bool m_supportSpsh;                           //!< Flag to indicate whether we support Spatial Sharing and Interference Mitigation.
  Ptr<DirectionalChannelQualityRequestElement> m_reqElem;

};

} // namespace ns3

#endif /* DMG_STA_WIFI_MAC_H */
