/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#ifndef DMG_WIFI_MAC_H
#define DMG_WIFI_MAC_H

#include "regular-wifi-mac.h"
#include "dmg-ati-dca.h"
#include "dmg-sls-dca.h"
#include "dmg-capabilities.h"
#include "codebook.h"

namespace ns3 {

#define dot11MaxBFTime                  10    /* Maximum Beamforming Time (in units of beacon interval) */
#define dot11BFTXSSTime                 10    /* Timeout until the initiator restarts ISS (in Milliseconds)." */
#define dot11MaximalSectorScan          10
#define dot11ABFTRTXSSSwitch            10
#define dot11BFRetryLimit               10    /* Beamforming Retry Limit */
/* Frames duration precalculated using MCS0 and reported in nanoseconds */
/**
  * SSW Frame Size = 26 Bytes
  * Ncw = 2, Ldpcw = 160, Ldplcw = 160, dencodedSymbols  = 328
  */
#define sswTxTime     MicroSeconds (16)//(NanoSeconds (4291) + NanoSeconds (4654) + NanoSeconds (5964))
#define sswFbckTxTime MicroSeconds (19)//(NanoSeconds (4291) + NanoSeconds (4654) + NanoSeconds (9310))
#define sswAckTxTime  MicroSeconds (19)//(NanoSeconds (4291) + NanoSeconds (4654) + NanoSeconds (9310))
#define aAirPropagationTime NanoSeconds (100)
#define aTSFResolution MicroSeconds (1)
#define GUARD_TIME MicroSeconds (10)
// Timeout Values
#define MBIFS MicroSeconds (3)
#define SSW_ACK_TIMEOUT 2 * (sswAckTxTime + aAirPropagationTime + MBIFS)
// Size of different DMG Control Frames in Bytes 802.11ad
#define POLL_FRAME_SIZE             22
#define SPR_FRAME_SIZE              27
#define GRANT_FRAME_SIZE            27
// Association Identifiers
#define AID_AP                      0
#define AID_BROADCAST               255
// Antenna Configuration
#define NO_ANTENNA_CONFIG           255
// Allocation of SPs and CBAPs
#define BROADCAST_CBAP              0
#define MAX_SP_BLOCK_DURATION       32767
#define MAX_CBAP_BLOCK_DURATION     65535
#define MAX_NUM_BLOCKS              255

enum ChannelAccessPeriod {
  CHANNEL_ACCESS_BTI = 0,
  CHANNEL_ACCESS_ABFT,
  CHANNEL_ACCESS_ATI,
  CHANNEL_ACCESS_BHI,
  CHANNEL_ACCESS_DTI
};

/** Type definitions **/
typedef std::pair<DynamicAllocationInfoField, BF_Control_Field> AllocationData; /* Typedef for dynamic allocation of SPs */
typedef std::list<AllocationData> AllocationDataList;
typedef AllocationDataList::const_iterator AllocationDataListCI;
typedef std::pair<SectorID, AntennaID>        ANTENNA_CONFIGURATION;            /* Typedef for antenna Config (SectorID, AntennaID) */

class BeamRefinementElement;

enum SLS_INITIATOR_STATE_MACHINE {
  INITIATOR_IDLE = 0,
  INITIATOR_SECTOR_SELECTOR = 1,
  INITIATOR_SSW_ACK = 2,
  INITIATOR_TXSS_PHASE_COMPELTED = 3,
};

enum SLS_RESPONDER_STATE_MACHINE {
  RESPONDER_IDLE = 0,
  RESPONDER_SECTOR_SELECTOR = 1,
  RESPONDER_SSW_FBCK = 2,
  RESPONDER_TXSS_PHASE_PRECOMPLETE = 3,
  RESPONDER_TXSS_PHASE_COMPELTED = 4,
};

enum BRP_TRAINING_TYPE {
  BRP_TRN_T = 0,
  BRP_TRN_R = 1,
  BRP_TRN_T_R = 2,
};

/**
 * \brief IEEE 802.11ad MAC.
 * \ingroup wifi
 *
 * Abstract class for Directional Multi Gigabit (DMG) support.
 */
class DmgWifiMac : public RegularWifiMac
{
public:
  static TypeId GetTypeId (void);

  DmgWifiMac ();
  virtual ~DmgWifiMac ();

  /**
   * Get Association Identifier (AID).
   * \return The AID of the station.
   */
  virtual uint16_t GetAssociationID (void) = 0;
  /**
   * \param address the current address of this MAC layer.
   */
  virtual void SetAddress (Mac48Address address);
  /**
  * \param packet the packet to send.
  * \param to the address to which the packet should be sent.
  *
  * The packet should be enqueued in a tx queue, and should be
  * dequeued as soon as the channel access function determines that
  * access is granted to this MAC.
  */
  virtual void Enqueue (Ptr<const Packet> packet, Mac48Address to) = 0;
  /**
   * \param stationManager the station manager attached to this MAC.
   */
  virtual void SetWifiRemoteStationManager(Ptr<WifiRemoteStationManager> stationManager);
  /**
   * Steer the directional antenna towards specific station for transmission and set the reception in omni-mode.
   * \param address The MAC address of the peer station.
   */
  void SteerTxAntennaToward (Mac48Address address);
  /**
   * Steer the directional antenna towards specific station for transmission and reception.
   * \param address The MAC address of the peer station.
   */
  void SteerAntennaToward (Mac48Address address);
  /**
   * Print SNR Table for each station that we did sectcor level sweeping with.
   */
  void PrintSnrTable (void);
  /**
   * Print Beam Refinement Measurements for each device.
   */
  void PrintBeamRefinementMeasurements (void);
  /**
   * Calculate the duration of a single sweep based on the number of sectors.
   * \param sectors The number of sectors.
   * \return The total duration required for completing a single sweep.
   */
  Time CalculateSectorSweepDuration (uint8_t sectors);
  /**
   * Calculate the duration of a sector sweep based on the number of sectors and antennas.
   * \param peerAntennas The total number of antennas in the peer device.
   * \param myAntennas The total number of antennas in the current device.
   * \param sectors The number of sectors.
   * \return The total duration required for completing a single sweep.
   */
  Time CalculateSectorSweepDuration (uint8_t peerAntennas, uint8_t myAntennas, uint8_t sectors);
  /**
   * Calculate the duration of the sector sweep of the current station.
   * \return The total duration required for completing a single sweep.
   */
  Time CalculateMySectorSweepDuration (void);
  /**
   * Calculate the duration of the Beamforming Training (SLS Only).
   * \param initiatorSectors The number of Tx/Rx sectors in the initiator station.
   * \param responderSectors The number of Tx/Rx sectors in the responder station.
   * \return The total duration required for completing beamforming training.
   */
  Time CalculateBeamformingTrainingDuration (uint8_t initiatorSectors, uint8_t responderSectors);
  /**
   * Store the DMG capabilities of a DMG STA.
   * \param wifiMac Pointer to the DMG STA.
   */
  void StorePeerDmgCapabilities (Ptr<DmgWifiMac> wifiMac);
  /**
   * Retreive the DMG Capbilities of a peer DMG station.
   * \param stationAddress The MAC address of the peer station.
   * \return DMG Capbilities of the Peer station if exists otherwise null.
   */
  Ptr<DmgCapabilities> GetPeerStationDmgCapabilities (Mac48Address stationAddress) const;
  /**
   * Calculate the duration of the Beamforming Training (SLS Only) based on the capabilities of the responder.
   * \param responderAddress The MAC address of the responder.
   * \param isInitiatorTxss Indicate whether the initiator is TxSS or RxSS.
   * \param isResponderTxss Indicate whether the respodner is TxSS or RxSS.
   * \return The total duration required for completing beamforming training.
   */
  Time ComputeBeamformingAllocationSize (Mac48Address responderAddress, bool isInitiatorTxss, bool isResponderTxss);
  /**
   * Initiate Beamforming with a particular DMG STA.
   * \param peerAid The AID of the peer DMG STA.
   * \param peerAddress The address of the DMG STA to perform beamforming training with.
   * \param isInitiator Indicate whether we are the Beamforming Initiator.
   * \param isInitiatorTxss Indicate whether the initiator is TxSS or RxSS.
   * \param isResponderTxss Indicate whether the respodner is TxSS or RxSS.
   * \param length The length of the Beamforming Service Period.
   */
  void StartBeamformingTraining (uint8_t peerAid, Mac48Address peerAddress,
                                 bool isInitiator, bool isInitiatorTxss, bool isResponderTxss,
                                 Time length);

  /**
   * Initiate TxSS in CBAP allocation.
   * \param peerAddress The address of the DMG STA to perform beamforming training with.
   */
  void InitiateTxssCbap (Mac48Address peerAddress);
  /**
   * Set Codebook
   * \param codebook The codebook for antennas and sectors patterns.
   */
  void SetCodebook (Ptr<Codebook> codebook);
  /**
   * Get Codebook
   * \return A pointer to the current codebook.
   */
  Ptr<Codebook> GetCodebook (void) const;
  /**
   * Initiate BRP transaction with a peer sation in ATI access period.
   * \param receiver The address of the peer station.
   * \param L_RX The compressed number of TRN-R subfields requested by the transmitting STA as part of beam refinement.
   * \param TX_TRN_REQ To indicate that the STA needs transmit training as part of beam refinement.
   */
  void InitiateBrpTransaction (Mac48Address receiver, uint8_t L_RX, bool TX_TRN_REQ);
  /**
   * Initiate BRP transaction with a peer sation after an SLS phase.
   * \param receiver The address of the peer station.
   * \param requestField The BRP Request Field from the SLS phase.
   */
//  void InitiateBrpTransaction (Mac48Address receiver, BRP_Request_Field &requestField);

  /* Temporary Function to store AID mapping */
  void MapAidToMacAddress (uint16_t aid, Mac48Address address);
  /**
   * Get Antenna Configuration
   * \param stationAddress The MAC address of the DMG STA.
   * \param isTxConfiguration
   * \return
   */
  ANTENNA_CONFIGURATION GetAntennaConfiguration (const Mac48Address stationAddress, bool isTxConfiguration) const;

protected:
  friend class MacLow;

  virtual void DoDispose (void);
  virtual void DoInitialize (void);
  virtual void Configure80211ad (void);

  /**
   * Start TxSS Transmit opportunity (TxOP).
   * \param peerAddress The address of the DMG STA to perform beamforming training with.
   * \param isInitiator Indicate whether we are the TxSS Initiator.
   */
  void StartTxssTxop (Mac48Address peerAddress, bool isInitiator);
  /**
   * Start Sector Sweep Feedback Transmit opportunity (TxOP).
   * \param peerAddress The address of the DMG STA to transmit SSW-FBCK.
   */
  void StartSswFeedbackTxop (Mac48Address peerAddress);
  /**
   * TxSS TxOP is granted.
   * \param peerAddress The address of the DMG STA to perform beamforming training with.
   * \param feedback Whether to transmit feedback or SSW frames.
   */
  void TxssTxopGranted (Mac48Address peerAddress, bool feedback);
  /**
   * Start Beamforming Initiator Phase.
   */
  void StartBeamformingInitiatorPhase (void);
  /**
   * Start Beamforming Responder Phase.
   * \address The MAC address of the peer station.
   */
  void StartBeamformingResponderPhase (Mac48Address address);
  /**
   * Terminate service period.
   */
  void EndServicePeriod (void);
  /**
   * Set whether PCP Handover is supported or not.
   * \param value Set to true if PCP handover is enabled otherwise it is set to false.
   */
  void SetPcpHandoverSupport (bool value);
  /**
   * \return Whether the PCP handover is supported or not.
   */
  bool GetPcpHandoverSupport (void) const;

  /**
   * Send Information Request frame.
   * \param to The MAC address of the receiving station.
   * \param request Pointer to the Request Element.
   */
  void SendInformationRequest (Mac48Address to, ExtInformationRequest &requestHdr);
  /**
   * Send Information Response to the station that requested the information.
   * \param to The MAC address of the station requested the information.
   * \param responseHdr Pointer to the Response Element.
   */
  void SendInformationResponse (Mac48Address to, ExtInformationResponse &responseHdr);
  /**
   * Return the DMG capability of the current STA.
   * \return the DMG capability that we support
   */
  virtual Ptr<DmgCapabilities> GetDmgCapabilities (void) const = 0;

  /**
   * \param sbifs the value of Short Beamforming Interframe Space (SBIFS).
   */
  void SetSbifs (Time sbifs);
  /**
   * \param mbifs the value of Medium Beamforming Interframe Space (MBIFS).
   */
  void SetMbifs (Time mbifs);
  /**
   * \param lbifs the value of Large Beamforming Interframe Space (LBIFS).
   */
  void SetLbifs (Time lbifs);
  /**
   * \param brpifs the value of Beam Refinement Protocol Interframe Spacing (BRPIFS).
   */
  void SetBrpifs (Time brpifs);
  /**
   * \return The value of of Short Beamforming Interframe Space (SBIFS)
   */
  Time GetSbifs (void) const;
  /**
   * \return The the value of Medium Beamforming Interframe Space (MBIFS).
   */
  Time GetMbifs (void) const;
  /**
   * \return The value of Large Beamforming Interframe Space (LBIFS).
   */
  Time GetLbifs (void) const;
  /**
   * \return The value of Beam Refinement Protocol Interframe Spacing (BRPIFS).
   */
  Time GetBrpifs (void) const;
  /**
   * Get sector sweep duration either at the initiator or at the responder.
   * \param sectors The number of sectors in the duration.
   * \return The total duration required to complete sector sweep.
   */
  Time GetSectorSweepDuration (uint8_t sectors);
  /**
   * Get sector sweep slot time in A-BFT access period.
   * \param fss The number of sectors in a slot.
   * \return The duration of a single slot in A-BFT access period.
   */
  Time GetSectorSweepSlotTime (uint8_t fss);
  /**
   * Map received SNR value to a specific address and TX antenna configuration (The Tx of the peer station).
   * \param address The MAC address of the receiver.
   * \param sectorID The ID of the transmitting sector.
   * \param antennaID The ID of the transmitting antenna.
   * \param snr The received Signal to Noise Ration in dBm.
   */
  void MapTxSnr (Mac48Address address, SectorID sectorID, AntennaID antennaID, double snr);
  /**
   * Map received SNR value to a specific address and RX antenna configuration (The Rx of the calling station).
   * \param address The address of the receiver.
   * \param sectorID The ID of the receiving sector.
   * \param antennaID The ID of the receiving antenna.
   * \param snr The received Signal to Noise Ration in dB.
   */
  void MapRxSnr (Mac48Address address, SectorID sectorID, AntennaID antennaID, double snr);
  /**
   * Get the remaining time for the current allocation period.
   * \return The remaining time for the current allocation period.
   */
  Time GetRemainingAllocationTime (void) const;
  /**
   * Get the remaining time for the current sector sweep.
   * \return The remaining time for the current sector sweep.
   */
  Time GetRemainingSectorSweepTime (void) const;
  /**
   * Transmit control frame using either CBAP or SP access period.
   * \param packet
   * \param hdr
   * \param duration The duration in the SSW field.
   */
  void TransmitControlFrame (Ptr<const Packet> packet, WifiMacHeader &hdr, Time duration);
  /**
   * Transmit control frame using SP access period.
   * \param packet
   * \param hdr
   * \param duration The duration in the SSW field.
   */
  void TransmitControlFrameImmediately (Ptr<const Packet> packet, WifiMacHeader &hdr, Time duration);
  /**
   * \brief Receive Sector Sweep Frame.
   * \param packet Pointer to the sector sweep packet.
   * \param hdr The MAC header of the packet.
   */
  void ReceiveSectorSweepFrame (Ptr<Packet> packet, const WifiMacHeader *hdr);
  /**
   * Record Beamformed Link Maintenance Value
   * \param field The BF Link Maintenance Field in SSW-FBCK/ACK.
   */
  void RecordBeamformedLinkMaintenanceValue (BF_Link_Maintenance_Field field);
  /**
   * Switch to the next Quasi-omni pattern in the codebook
   * \param switchTime The time when to switch to the next Quasi-Omni Pattern.
   */
  void SwitchQuasiOmniPattern (Time switchTime);
  /**
   * Restart Initiator Sector Sweep (ISS) Phase.
   * \param stationAddress The MAC address of the peer station we want to perform ISS with.
   */
  void RestartInitiatorSectorSweep (Mac48Address stationAddress);
  /**
   * Start Transmit Sector Sweep (TxSS) with specific station.
   * \param address The MAC address of the peer DMG STA.
   * \param direction Indicate whether we are initiator or responder.
   */
  void StartTransmitSectorSweep (Mac48Address address, BeamformingDirection direction);
  /**
   * Start Receive Sector Sweep (RxSS) with specific station.
   * \param address The MAC address of the peer DMG STA.
   * \param direction Indicate whether we are beamforming initiator or responder.
   */
  void StartReceiveSectorSweep (Mac48Address address, BeamformingDirection direction);
  /**
   * Send Receive Sector Sweep Frame as Initiator.
   * \param address The MAC address of the respodner.
   * \param count the remaining number of sectors.
   * \param direction Indicate whether we are beamforming initiator or responder.
   */
  void SendReceiveSectorSweepFrame (Mac48Address address, uint16_t count, BeamformingDirection direction);
  /**
   * Send a Transmit Sector Sweep Frame as an Initiator.
   * \param address The MAC address of the respodner.
   */
  void SendInitiatorTransmitSectorSweepFrame (Mac48Address address);
  /**
   * Send a Tansmit Sector Sweep Frame as a Responder.
   * \param address The MAC address of the initiator.
   */
  void SendRespodnerTransmitSectorSweepFrame (Mac48Address address);
  /**
   * Send SSW FBCK Frame for SLS phase in a scheduled service period or after SSW-Slot in A-BFT.
   * \param receiver The MAC address of the responding station.
   * \param duration The duration in the MAC header.
   */
  void SendSswFbckFrame (Mac48Address receiver, Time duration);
  /**
   * Re-Send SSW FBCK Frame for SLS phase in a scheduled service period.
   */
  void ResendSswFbckFrame (void);
  /**
   * Send SSW ACK Frame for SLS phase in a scheduled service period.
   * \param receiver The MAC address of the peer DMG STA.
   * \param sswFbckDuration The duration in the SSW-FBCK Field.
   */
  void SendSswAckFrame (Mac48Address receiver, Time sswFbckDuration);
  /**
   * The BRP setup subphase is used to set up beam refinement transaction.
   * \param type The type of BRP (Either Transmit, Receive, or both).
   * \param address The MAC address of the receiving station (Responder's MAC Address).
   */
  void InitiateBrpSetupSubphase (BRP_TRAINING_TYPE type, Mac48Address receiver);
  /**
   * Send BRP Frame without TRN Field to a specific station.
   * \param receiver The MAC address of the receiving station.
   * \param requestField The BRP Request Field.
   * \param element The Beam Refinement Element.
   */
  void SendEmptyBrpFrame (Mac48Address receiver, BRP_Request_Field &requestField, BeamRefinementElement &element);
  /**
   * Send BRP Frame with TRN Field to a specific station.
   * \param receiver The MAC address of the receiving station.
   * \param requestField The BRP Request Field.
   * \param element The Beam Refinement Element.
   */
  void SendBrpFrame (Mac48Address receiver, BRP_Request_Field &requestField, BeamRefinementElement &element,
                     bool requestBeamRefinement, PacketType packetType, uint8_t trainingFieldLength);

  /**
   * This function is called by dervied call to notify that BRP Phase has completed.
   */
  virtual void NotifyBrpPhaseCompleted (void) = 0;

  /* Typedefs for Recording SNR Value per Antenna Configuration */
  typedef double SNR;                                                   /* Typedef for SNR value. */
  typedef std::map<ANTENNA_CONFIGURATION, SNR>  SNR_MAP;                /* Typedef for Map between Antenna Config and SNR. */
  typedef SNR_MAP                               SNR_MAP_TX;             /* Typedef for SNR TX for each antenna configuration. */
  typedef SNR_MAP                               SNR_MAP_RX;             /* Typedef for SNR RX for each antenna configuration. */
  typedef std::pair<SNR_MAP_TX, SNR_MAP_RX>     SNR_PAIR;               /* Typedef for SNR RX for each antenna configuration. */
  typedef std::map<Mac48Address, SNR_PAIR>      STATION_SNR_PAIR_MAP;   /* Typedef for Map between stations and their SNR Table. */
  typedef STATION_SNR_PAIR_MAP::iterator        STATION_SNR_PAIR_MAP_I; /* Typedef for iterator over SNR MAPPING Table. */
  typedef STATION_SNR_PAIR_MAP::const_iterator  STATION_SNR_PAIR_MAP_CI;/* Typedef for const iterator over SNR MAPPING Table. */

  /* Typedefs for Recording Best Antenna Configuration per Station */
  typedef ANTENNA_CONFIGURATION ANTENNA_CONFIGURATION_TX;               /* Typedef for best TX antenna configuration. */
  typedef ANTENNA_CONFIGURATION ANTENNA_CONFIGURATION_RX;               /* Typedef for best RX antenna configuration. */
  typedef std::pair<ANTENNA_CONFIGURATION_TX, ANTENNA_CONFIGURATION_RX> BEST_ANTENNA_CONFIGURATION;
  typedef std::map<Mac48Address, BEST_ANTENNA_CONFIGURATION>            STATION_ANTENNA_CONFIG_MAP;
  typedef STATION_ANTENNA_CONFIG_MAP::iterator                          STATION_ANTENNA_CONFIG_MAP_I;
  typedef STATION_ANTENNA_CONFIG_MAP::const_iterator                    STATION_ANTENNA_CONFIG_MAP_CI;

  /**
   * Update Best Tx antenna configuration towards specific station.
   * \param stationAddress The MAC address of the peer station.
   * \param antennaConfigTx Best transmit antenna configuration.
   */
  void UpdateBestTxAntennaConfiguration (const Mac48Address stationAddress, ANTENNA_CONFIGURATION_TX antennaConfigTx);
  /**
   * Update Best Rx antenna configuration towards specific station.
   * \param stationAddress The MAC address of the peer station.
   * \param antennaConfigRx Best receive antenna configuration.
   */
  void UpdateBestRxAntennaConfiguration (const Mac48Address stationAddress, ANTENNA_CONFIGURATION_RX antennaConfigRx);
  /**
   * Update Best Rx antenna configuration towards specific station.
   * \param stationAddress The MAC address of the peer station.
   * \param antennaConfigTx Best transmit antenna configuration.
   * \param antennaConfigRx Best receive antenna configuration.
   */
  void UpdateBestAntennaConfiguration (const Mac48Address stationAddress,
                                       ANTENNA_CONFIGURATION_TX txConfig, ANTENNA_CONFIGURATION_RX rxConfig);
  /**
   * Print SNR for either Tx/Rx Antenna Configurations.
   * \param snrMap The SNR Map
   */
  void PrintSnrConfiguration (SNR_MAP snrMap);
  /**
   * Obtain antenna configuration for the highest received SNR to feed it back
   * \param stationAddress The MAC address of the station.
   * \param isTxConfiguration Is the Antenna Tx Configuration we are searching for or not.
   */
  ANTENNA_CONFIGURATION GetBestAntennaConfiguration (const Mac48Address stationAddress, bool isTxConfiguration);
  /**
   * Obtain antenna configuration for the highest received SNR to feed it back
   * \param stationAddress The MAC address of the station.
   * \param isTxConfiguration Is the Antenna Tx Configuration we are searching for or not.
   * \param maxSnr The SNR value corresponding to the BEst Antenna Configuration.
   */
  ANTENNA_CONFIGURATION GetBestAntennaConfiguration (const Mac48Address stationAddress, bool isTxConfiguration, double &maxSnr);
  /**
   * Get Relay Capabilities Informaion for this DMG STA.
   * \return
   */
  RelayCapabilitiesInfo GetRelayCapabilitiesInfo (void) const;
  /**
   * Get DMG Relay Capabilities Element supported by this DMG STA.
   * \return The DMG Relay Capabilities Information Element.
   */
  Ptr<RelayCapabilitiesElement> GetRelayCapabilitiesElement (void) const;
  /**
   * Send Relay Search Response.
   * \param to
   * \param token The dialog token.
   */
  void SendRelaySearchResponse (Mac48Address to, uint8_t token);
  /**
   * Start Beacon Interval (BI)
   */
  virtual void StartBeaconInterval (void) = 0;
  /**
   * End Beacon Interval (BI)
   */
  virtual void EndBeaconInterval (void) = 0;
  /**
   * Start Beacon Transmission Interval (BTI)
   */
  virtual void StartBeaconTransmissionInterval (void) = 0;
  /**
   * Start Association Beamform Training (A-BFT).
   */
  virtual void StartAssociationBeamformTraining (void) = 0;
  /**
   * Start Announcement Transmission Interval (ATI).
   */
  virtual void StartAnnouncementTransmissionInterval (void) = 0;
  /**
   * Start Data Transmission Interval (DTI).
   */
  virtual void StartDataTransmissionInterval (void) = 0;
  /**
   * Get current access period in the current beacon interval
   * \return The type of the access period (BTI/A-BFT/ATI or DTI).
   */
  ChannelAccessPeriod GetCurrentAccessPeriod (void) const;
  /**
   * Get allocation type for the current period.
   * \return The type of the current allocation (SP or CBAP).
   */
  AllocationType GetCurrentAllocation (void) const;
  /**
   * Start contention based access period (CBAP).
   * \param contentionDuration The duration of the contention period.
   */
  void StartContentionPeriod (AllocationID allocationID, Time contentionDuration);
  /**
   * End Contention Period.
   */
  void EndContentionPeriod (void);
  /**
   *BeamLink Maintenance Timeout.
   */
  virtual void BeamLinkMaintenanceTimeout (void);
  /**
   * Schedule the service period (SP) allocation blocks at the start of DTI.
   * \param blocks The number of time blocks making up the allocation.
   * \param spStart
   * \param spLength
   * \param spPeriod
   * \param length The length of the allocation period.
   * \param peerAid The AID of the peer station in this service period.
   * \param peerStation The MAC Address of the peer station in this service period.
   * \param isSource Whether we are the initiator of the service period.
   */
  void ScheduleServicePeriod (uint8_t blocks, Time spStart, Time spLength, Time spPeriod,
                              AllocationID allocationID, uint8_t peerAid, Mac48Address peerAddress, bool isSource);
  /**
   * Start service period (SP) allocation period.
   * \param length The length of the allocation period.
   * \param peerAid The AID of the peer station in this service period.
   * \param peerStation The MAC Address of the peer station in this service period.
   * \param isSource Whether we are the initiator of the service period.
   */
  void StartServicePeriod (AllocationID allocationID, Time length, uint8_t peerAid, Mac48Address peerStation, bool isSource);
  /**
   * Resume transmission for the current service period.
   */
  void ResumeServicePeriodTransmission (void);
  /**
   * Suspend ongoing transmission for the current service period.
   */
  void SuspendServicePeriodTransmission (void);
  /**
   * Add new data forwarding entry.
   * \param nextHopAddress The MAC Address of the next hop.
   */
  void AddForwardingEntry (Mac48Address nextHopAddress);
  /**
   * This function is excuted upon the transmission of frame.
   * \param hdr The header of the transmitted frame.
   */
  virtual void FrameTxOk (const WifiMacHeader &hdr) = 0;
  virtual void Receive (Ptr<Packet> packet, const WifiMacHeader *hdr);
  virtual void BrpSetupCompleted (Mac48Address address) = 0;
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
   * Get frame Duration in microseconds.
   * \param duration The duration of the frame in nanoseconds.
   * \return The duration of the frame in microseconds.
   */
  Time GetFrameDurationInMicroSeconds (Time duration) const;
  /**
   * Get transmission time duration for SPR frame.
   * \return The transmission time duration for SPR frame.
   */
  Time GetSprFrameDuration (void) const;
  /**
   * Add range of MCSs to support in transmission/reception towards particular station.
   * \param address The MAC address of the peer station
   * \param firstMcs The first MCS index to support.
   * \param lastMcs The last MCS index to support.
   */
  void AddMcsSupport (Mac48Address address, uint32_t initialMcs, uint32_t lastMcs);

protected:
  STATION_SNR_PAIR_MAP m_stationSnrMap;           //!< Map between stations and their SNR Table.
  STATION_ANTENNA_CONFIG_MAP m_bestAntennaConfig; //!< Map between remote stations and the best antenna configuration.
  ANTENNA_CONFIGURATION m_feedbackAntennaConfig;  //!< Temporary variable to save the best antenna config of the peer station.

  /* PHY Layer Information */
  Time m_sbifs;                         //!< Short Beamforming Interframe Space.
  Time m_mbifs;                         //!< Medium Beamforming Interframe Space.
  Time m_lbifs;                         //!< Long Beamfornnig Interframe Space.
  Time m_brpifs;                        //!< Beam Refinement Protocol Interframe Spacing.
  bool m_supportOFDM;                   //!< Flag to indicate whether we support OFDM PHY layer.
  bool m_supportLpSc;                   //!< Flag to indicate whether we support LP-SC PHY layer.
  uint8_t m_maxScTxMcs;                 //!< The maximum supported MCS for transmission by SC PHY.
  uint8_t m_maxScRxMcs;                 //!< The maximum supported MCS for reception by SC PHY.
  uint8_t m_maxOfdmTxMcs;               //!< The maximum supported MCS for transmission by OFDM PHY.
  uint8_t m_maxOfdmRxMcs;               //!< The maximum supported MCS for reception by OFDM PHY.

  /* Channel Access Period */
  ChannelAccessPeriod m_accessPeriod;               //!< The type of the current channel access period.
  Time m_atiDuration;                               //!< The length of the ATI period.
  Time m_biStartTime;                               //!< The start time of the BI Interval.
  Time m_beaconInterval;                            //!< Interval between beacons.
  bool m_atiPresent;                                //!< Flag to indicate if ATI period is present in the current BI.
  TracedCallback<Mac48Address, Time> m_dtiStarted;  //!< Trace Callback for starting new DTI access period.
  /**
   * TracedCallback signature for DTI access period start event.
   *
   * \param address The MAC address of the station.
   * \param duration The duration of the DTI period.
   */
  typedef void (* DtiStartedCallback)(Mac48Address address, Time duration);

  /** BTI Access Period Variables **/
  uint8_t m_nextBeacon;                 //!< The number of BIs following the current beacon interval during which the DMG Beacon is not be present.
  uint8_t m_btiPeriodicity;             //!< The periodicity of the BTI in BI.
  Time m_dtiDuration;                   //!< The duration of the DTI access period.
  Time m_dtiStartTime;                  //!< The start time of the DTI access period.

  /** A-BFT Access Period Variables **/
  Time m_abftDuration;                  //!< The duration of the A-BFT access period.
  uint8_t m_ssSlotsPerABFT;             //!< Number of Sector Sweep Slots Per A-BFT.
  uint8_t m_ssFramesPerSlot;            //!< Number of SSW Frames per Sector Sweep Slot.
  uint8_t m_nextAbft;                   //!< The value of the next A-BFT in DMG Beacon.

  uint16_t m_totalSectors;              //!< Total number of sectors remaining to cover.
  bool m_pcpHandoverSupport;            //!< Flag to indicate if we support PCP Handover.
  bool m_sectorFeedbackSchedulled;      //!< Flag to indicate if we schedulled SSW Feedback.

  enum ABFT_STATE {
    WAIT_BEAMFORMING_TRAINING = 0,
    BEAMFORMING_TRAINING_COMPLETED = 1,
    BEAMFORMING_TRAINING_PARTIALLY_COMPLETED = 2,
    FAILED_BEAMFORMING_TRAINING = 3,
    START_RSS_BACKOFF_STATE = 4,
    IN_RSS_BACKOFF_STATE = 5,
  };
  ABFT_STATE m_abftState;                             //!< Beamforming Training State in A-BFT.

  /** ATI Period Variables **/
  Ptr<DmgAtiDca> m_dmgAtiDca;                         //!< Dedicated DcaTxop for ATI.

  /** SLS Variables  **/
  Ptr<DmgSlsDca> m_dmgSlsDca;                         //!< Dedicated DcaTxop for SLS.

  /* BRP Phase Variables */
  std::map<Mac48Address, bool> m_isBrpResponder;      //!< Map to indicate whether we are BRP Initiator or responder.
  std::map<Mac48Address, bool> m_isBrpSetupCompleted; //!< Map to indicate whether BRP Setup is completed with station.
  std::map<Mac48Address, bool> m_raisedBrpSetupCompleted;
  bool m_recordTrnSnrValues;                          //!< Flag to indicate if we should record reported SNR Values by TRN Fields.
  bool m_requestedBrpTraining;                        //!< Flag to indicate whether BRP Training has been performed.
  bool m_executeBRPinATI;                             //!< Flag to indicate if we execute BRP in ATI phase.

  bool m_executeBRPafterSLS;                          //!< Flag to indicate we execute BRP phase after SLS.
  BRP_TRAINING_TYPE m_brpType;                        //!< The type of the BRP phase to be executed.

  /* DMG Relay Variables */
  RelayDuplexMode m_relayDuplexMode;            //!< The duplex mode of the relay.
  bool m_relayMode;                             //!< Flag to indicate we are in relay mode.
  bool m_redsActivated;                         //!< DMG Station supports REDS.
  bool m_rdsActivated;                          //!< DMG Station supports RDS.
  RelayCapableStaList m_rdsList;                //!< List of Relay STAs (RDS) in the currernt DMG BSS.
  TracedCallback<Mac48Address> m_rlsCompleted;  //!< Flag to indicate we are completed RLS setup.

  /* Information Request/Response */
  typedef std::vector<Ptr<WifiInformationElement> > WifiInformationElementList;
  typedef std::pair<Ptr<DmgCapabilities>, WifiInformationElementMap> StationInformation;
  typedef std::map<Mac48Address, StationInformation> InformationMap;
  typedef InformationMap::iterator InformationMapIterator;
  typedef InformationMap::const_iterator InformationMapCI;
  InformationMap m_informationMap;              //!< List of information element for assoicated stations.

  /* DMG Parameteres */
  bool m_isCbapOnly;                            //!< Flag to indicate whether the DTI is allocated to CBAP.
  bool m_isCbapSource;                          //!< Flag to indicate that PCP/AP has higher priority for transmission.
  bool m_supportRdp;                            //!< Flag to indicate whether we support RDP.

  /* Access Period Allocations */
  AllocationFieldList m_allocationList;         //!< List of access period allocations in DTI.

  /* Service Period Channel Access */
  AllocationID m_currentAllocationID;           //!< The ID of the current allocation.
  AllocationType m_currentAllocation;           //!< The current access period allocation.
  Time m_allocationStarted;                     //!< The time we initiated the allocation.
  Time m_currentAllocationLength;               //!< The length of the current allocation period in MicroSeconds.
  uint8_t m_peerStationAid;                     //!< The AID of the peer DMG STA in the current SP.
  Mac48Address m_peerStationAddress;            //!< The MAC address of the peer DMG STA in the current SP.
  Time m_suspendedPeriodDuration;               //!< The remaining duration of the suspended SP.
  bool m_spSource;                              //!< Flag to indicate if we are the source of the SP.

  /* Beamforming Variables */
  Ptr<Codebook> m_codebook;                     //!< Pointer to the beamforming codebook.
  uint8_t m_bfRetryTimes;                       //!< Counter for Beamforming retry times.
  EventId m_restartISSEvent;                    //!< Event related to restarting ISS.
  EventId m_sswAckTimeoutEvent;                 //!< Event related to not receiving SSW ACK.
  bool m_isBeamformingInitiator;                //!< Flag to indicate whether we initaited BF.
  bool m_isInitiatorTXSS;                       //!< Flag to indicate whether the initiator is TxSS or RxSS.
  bool m_isResponderTXSS;                       //!< Flag to indicate whether the responder is TxSS or RxSS.
  uint8_t m_peerSectors;                        //!< The total number of the sectors in the peer station.
  uint8_t m_peerAntennas;                       //!< The total number of the antennas in the peer station.
  Time m_sectorSweepStarted;
  Time m_sectorSweepDuration;
  EventId m_rssEvent;                           //!< Event related to scheduling RSS.
  /**
   * Trace callback for SLS phase completion.
   * \param Mac48Address The MAC address of the peer station.
   * \param ChannelAccessPeriod The current Channel access period BTI or DTI.
   * \param BeamformingDirection Initiator or Responder.
   * \param isBeamformingTxss Flag to indicate if the initiator is TxSS or RxSS.
   * \param isBeamformingRxss Flag to indicate if the responder is TxSS or RxSS.
   * \param sectorID The ID of the selected sector.
   * \param antennaID The ID of the selected antenna.
   */
  TracedCallback<Mac48Address, ChannelAccessPeriod, BeamformingDirection, bool, bool, SectorID, AntennaID> m_slsCompleted;
  SLS_INITIATOR_STATE_MACHINE m_slsInitiatorStateMachine;
  /**
   * Trace callback for BRP phase completion.
   * \param Mac48Address The MAC address of the peer station.
   * \param BeamRefinementType The type of the beam refinement (BRP-Tx or BRP-Rx).
   * \param AntennaID The ID of the selected antenna.
   * \param SectorID The ID of the selected sector.
   * \param AWV_ID The ID of the selected custom AWV.
   */
  TracedCallback<Mac48Address, BeamRefinementType, AntennaID, SectorID, AWV_ID> m_brpCompleted;

  /** Link Maintenance Variabeles **/
  BeamLinkMaintenanceUnitIndex m_beamlinkMaintenanceUnit;   //!< Link maintenance Unit according to std 802.11ad-2012.
  uint8_t m_beamlinkMaintenanceValue;                       //!< Link maintenance timer value in MicroSeconds.
  uint32_t dot11BeamLinkMaintenanceTime;                    //!< Link maintenance timer in MicroSeconds.

  struct BeamLinkMaintenanceInfo {
    uint32_t negotiatedValue;                               //!< Negotiated link maintenance value.
    uint32_t beamLinkMaintenanceTime;
  };                                                        //!< Typedef for Link Maintenance Information.

  typedef std::map<uint8_t, BeamLinkMaintenanceInfo> BeamLinkMaintenanceTable;
  typedef BeamLinkMaintenanceTable::iterator BeamLinkMaintenanceTableI;
  BeamLinkMaintenanceTable m_beamLinkMaintenanceTable;
  EventId m_beamLinkMaintenanceTimeout;         //!< Event ID related to the timer assoicated to the current beamformed link.
  bool m_currentLinkMaintained;                 //!< Flag to indicate whether the current beamformed link is maintained.
  BeamLinkMaintenanceInfo m_currentBeamLinkMaintenanceInfo;

  /**
   * TracedCallback signature for BeamLink Maintenance Timer expiration.
   *
   * \param aid The AID of the peer station.
   * \param address The MAC address of the peer station.
   * \param transmissionLink The new transmission link.
   */
  typedef void (* BeamLinkMaintenanceTimerExpired)(uint8_t aid, Mac48Address address, Time timeLeft);
  TracedCallback<uint8_t, Mac48Address, Time> m_beamLinkMaintenanceTimerExpired;

  /* Beam Refinement variables */
  typedef std::vector<double> TRN2SNR;                  //!< Typedef for vector of SNR values for TRN Subfields.
  typedef TRN2SNR::const_iterator TRN2SNR_CI;
  typedef std::map<Mac48Address, TRN2SNR> TRN2SNR_MAP;  //!< Typedef for map of TRN2SNR per station.
  typedef TRN2SNR_MAP::const_iterator TRN2SNR_MAP_CI;
  TRN2SNR m_trn2Snr;                                    //!< Variable to store SNR per TRN subfield for ongoing beam refinement phase or beam tracking.
  TRN2SNR_MAP m_trn2snrMap;                             //!< Variable to store SNR vector for TRN Subfields per device.

  /* AID to MAC Address mapping */
  typedef std::map<uint16_t, Mac48Address> AID_MAP;
  typedef std::map<Mac48Address, uint16_t> MAC_MAP;
  typedef AID_MAP::const_iterator AID_MAP_CI;
  typedef MAC_MAP::const_iterator MAC_MAP_CI;
  AID_MAP m_aidMap;                                     //!< Map AID to MAC Address.
  MAC_MAP m_macMap;                                     //!< Map MAC Address to AID.

  /* Data Forwarding Table */
  typedef struct {
    Mac48Address nextHopAddress;
    bool isCbapPeriod;
  } AccessPeriodInformation;

  typedef std::map<Mac48Address, AccessPeriodInformation> DataForwardingTable;
  typedef DataForwardingTable::iterator DataForwardingTableIterator;
  typedef DataForwardingTable::const_iterator DataForwardingTableCI;
  DataForwardingTable m_dataForwardingTable;

  /**
   * TracedCallback signature for service period initiation/termination.
   *
   * \param srcAddress The MAC address of the source station.
   * \param dstAddress The MAC address of the destination station.
   */
  typedef void (* ServicePeriodCallback)(Mac48Address srcAddress, Mac48Address dstAddress);
  TracedCallback<Mac48Address, Mac48Address> m_servicePeriodStartedCallback;
  TracedCallback<Mac48Address, Mac48Address> m_servicePeriodEndedCallback;

  /* Association Traces */
  typedef void (* AssociationCallback)(Mac48Address address, uint16_t);
  TracedCallback<Mac48Address, uint16_t> m_assocLogger;
  TracedCallback<Mac48Address> m_deAssocLogger;

private:
  /**
   * This function is called upon transmission of a 802.11 Management frame.
   * \param hdr The header of the management frame.
   */
  void ManagementTxOk (const WifiMacHeader &hdr);
  /**
   * Report SNR Value, this is a callback to be hooked with DmgWifiPhy class.
   * \param antennaID The ID of the phased antenna array.
   * \param sectorID The ID of the sector within the phased antenna array.
   * \param trnUnitsRemaining The number of remaining TRN Units.
   * \param subfieldsRemaining The number of remaining TRN Subfields within the TRN Unit.
   * \param snr The measured SNR over specific TRN.
   * \param isTxTrn Flag to indicate if we are receiving TRN-Tx or TRN-RX Subfields.
   */
  void ReportSnrValue (AntennaID antennaID, SectorID sectorID,
                       uint8_t trnUnitsRemaining, uint8_t subfieldsRemaining,
                       double snr, bool isTxTrn);

  Mac48Address m_peerStation;     /* The address of the station we are waiting BRP Response from */
  uint8_t m_dialogToken;          /* The token of the current dialog */

};

} // namespace ns3

#endif /* DMG_WIFI_MAC_H */
