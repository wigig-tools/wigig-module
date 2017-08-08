/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#ifndef DMG_WIFI_MAC_H
#define DMG_WIFI_MAC_H

#include "regular-wifi-mac.h"
#include "dmg-ati-dca.h"
#include "dmg-capabilities.h"
#include "service-period.h"

namespace ns3 {

#define dot11MaxBFTime                  10
#define dot11BFTXSSTime                 10
#define dot11MaximalSectorScan          10
#define dot11ABFTRTXSSSwitch            10
#define dot11BFRetryLimit               10
/* Frames duration precalculated using MCS0 and reported in nanoseconds */
/**
  * SSW Frame Size = 26 Bytes
  * Ncw = 2, Ldpcw = 160, Ldplcw = 160, dencodedSymbols  = 328
  */
#define sswTxTime     NanoSeconds (4291) + NanoSeconds (4654) + NanoSeconds (5964)
#define sswFbckTxTime NanoSeconds (4291) + NanoSeconds (4654) + NanoSeconds (9310)
#define sswAckTxTime  NanoSeconds (4291) + NanoSeconds (4654) + NanoSeconds (9310)
#define aAirPropagationTime NanoSeconds (100)
#define aTSFResolution MicroSeconds (1)
#define GUARD_TIME MicroSeconds (10)
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

typedef enum {
  CHANNEL_ACCESS_BTI = 0,
  CHANNEL_ACCESS_ABFT,
  CHANNEL_ACCESS_ATI,
  CHANNEL_ACCESS_BHI,
  CHANNEL_ACCESS_DTI
} ChannelAccessPeriod;

/** Type definitions **/
typedef uint8_t SECTOR_ID;                    /* Typedef for Sector ID */
typedef uint8_t ANTENNA_ID;                   /* Typedef for Antenna ID */
typedef std::pair<DynamicAllocationInfoField, BF_Control_Field> AllocationData; /* Typedef for dynamic allocation of SPs*/
typedef std::list<AllocationData> AllocationDataList;
typedef AllocationDataList::const_iterator AllocationDataListCI;

class BeamRefinementElement;

/**
 * \brief Wi-Fi DMG
 * \ingroup wifi
 *
 * Abstract class for Directional Multi Gigabit support.
 */
class DmgWifiMac : public RegularWifiMac
{
public:
  static TypeId GetTypeId (void);

  DmgWifiMac ();
  virtual ~DmgWifiMac ();

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
   * Print SNR Table for each station that we did beamforming training with.
   */
  void PrintSnrTable (void);
  /**
   * Calculate the duration of the Beamforming Training (SLS Only).
   * \param peerSectors The number of sectors in the peer station.
   * \return The total duration required for completing beamforming training.
   */
  Time CalculateBeamformingTrainingDuration (uint8_t peerSectors);
  /**
   * Get the total number of sectors
   * \return The total number of sectors
   */
  uint8_t GetNumberOfSectors (void) const;
  /**
   * Get number of phased antenna arrays.
   * \return The total number of phased antenna arrays.
   */
  uint8_t GetNumberOfAntennas (void) const;
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
   * Terminate service period.
   */
  void EndServicePeriod (void);
  /**
   * Get SP Queue.
   * \return Pointer to Queue of the service period access scheme.
   */
  Ptr<WifiMacQueue> GetSPQueue (void) const;

  /* Temporary Function to store AID mapping */
  void MapAidToMacAddress (uint16_t aid, Mac48Address address);
  Ptr<ServicePeriod> GetServicePeriod () const;

protected:
  friend class MacLow;

  virtual void DoDispose (void);
  virtual void DoInitialize (void);
  virtual void Configure80211ad (void);
  virtual void ConfigureAggregation (void);
  virtual void EnableAggregation (void);
  virtual void DisableAggregation (void);

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
  void MapTxSnr (Mac48Address address, SECTOR_ID sectorID, ANTENNA_ID antennaID, double snr);
  /**
   * Map received SNR value to a specific address and RX antenna configuration (The Rx of the calling station).
   * \param address The address of the receiver.
   * \param sectorID The ID of the receiving sector.
   * \param antennaID The ID of the receiving antenna.
   * \param snr The received Signal to Noise Ration in dB.
   */
  void MapRxSnr (Mac48Address address, SECTOR_ID sectorID, ANTENNA_ID antennaID, double snr);
  /**
   * Get the remaining time for the current allocation period.
   * \return The remaining time for the current allocation period.
   */
  Time GetRemainingAllocationTime (void) const;
  /**
   * Transmit control frame immediately.
   * \param packet
   * \param hdr
   */
  void TransmitControlFrameImmediately (Ptr<const Packet> packet, WifiMacHeader &hdr);
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
   * Send Transmit Sector Sweep Frame as Initiator.
   * \param address The MAC address of the respodner.
   * \param sectorID The ID of the current transmitting sector.
   * \param antennaID the ID of the current transmitting antenna.
   * \param count the remaining number of sectors.
   */
  void SendInitiatorTransmitSectorSweepFrame (Mac48Address address, uint8_t sectorID, uint8_t antennaID, uint16_t count);
  /**
   * Send Tansmit Sector Sweep Frame as Responder.
   * \param address The MAC address of the initiator.
   * \param sectorID The ID of the current transmitting sector.
   * \param antennaID the ID of the current transmitting antenna.
   * \param count the remaining number of sectors.
   */
  void SendRespodnerTransmitSectorSweepFrame (Mac48Address address, uint8_t sectorID, uint8_t antennaID, uint16_t count);
  /**
   * Send SSW FBCK Frame for SLS phase in a scheduled service period or after SSW-Slot in A-BFT.
   * \param receiver The MAC address of the responding station.
   * \param duration The duration in the MAC header.
   */
  void SendSswFbckFrame (Mac48Address receiver, Time duration);
  /**
   * Send SSW ACK Frame for SLS phase in a scheduled service period.
   * \param receiver The MAC address of the peer DMG STA.
   * \param sswFbckDuration The duration in the SSW-FBCK Field.
   */
  void SendSswAckFrame (Mac48Address receiver, Time sswFbckDuration);
  /**
   * The BRP setup subphase is used to set up beam refinement transactions.
   * \param address The MAC address of the receiving station.
   */
  void InitiateBrpSetupSubphase (Mac48Address receiver);
  /**
   * Send BRP Frame to a specific station.
   * \param receiver The MAC address of the receiving station.
   * \param requestField The BRP Request Field.
   * \param element The Beam Refinement Element.
   */
  void SendBrpFrame (Mac48Address receiver, BRP_Request_Field &requestField, BeamRefinementElement &element);
  /**
   * Send BRP Frame to a specific station.
   * \param receiver The MAC address of the receiving station.
   * \param requestField The BRP Request Field.
   * \param element The Beam Refinement Element.
   */
  void SendBrpFrame (Mac48Address receiver, BRP_Request_Field &requestField, BeamRefinementElement &element,
                     bool requestBeamRefinement, PacketType packetType, uint8_t trainingFieldLength);
  /**
   * Initiate BRP Transaction with a peer sation.
   * \param receiver The address of the peer station.
   */
  void InitiateBrpTransaction (Mac48Address receiver);
  /**
   * This function is called by dervied call to notify that BRP Phase has completed.
   */
  virtual void NotifyBrpPhaseCompleted (void) = 0;

  /* Typedefs for Recording SNR Value per Antenna Configuration */
  typedef double SNR;                                                   /* Typedef for SNR */
  typedef std::pair<SECTOR_ID, ANTENNA_ID>      ANTENNA_CONFIGURATION;  /* Typedef for antenna Config (SectorID, AntennaID) */
  typedef std::map<ANTENNA_CONFIGURATION, SNR>  SNR_MAP;                /* Typedef for Map between Antenna Config and SNR */
  typedef SNR_MAP                               SNR_MAP_TX;             /* Typedef for SNR TX for each antenna configuration */
  typedef SNR_MAP                               SNR_MAP_RX;             /* Typedef for SNR RX for each antenna configuration */
  typedef std::pair<SNR_MAP_TX, SNR_MAP_RX>     SNR_PAIR;               /* Typedef for SNR RX for each antenna configuration */
  typedef std::map<Mac48Address, SNR_PAIR>      STATION_SNR_PAIR_MAP;   /* Typedef for Map between stations and their SNR Table. */

  /* Typedefs for Recording Best Antenna Configuration per Station */
  typedef ANTENNA_CONFIGURATION ANTENNA_CONFIGURATION_TX;  /* Typedef for best TX antenna Config */
  typedef ANTENNA_CONFIGURATION ANTENNA_CONFIGURATION_RX;  /* Typedef for best RX antenna Config */
  typedef std::pair<ANTENNA_CONFIGURATION_TX, ANTENNA_CONFIGURATION_RX> BEST_ANTENNA_CONFIGURATION;
  typedef std::map<Mac48Address, BEST_ANTENNA_CONFIGURATION>            STATION_ANTENNA_CONFIG_MAP;

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
   * This method can be called to accept a received ADDBA Request. An
   * ADDBA Response will be constructed and queued for transmission.
   *
   * \param reqHdr a pointer to the received ADDBA Request header.
   * \param originator the MAC address of the originator.
   */
  virtual void SendAddBaResponse (const MgtAddBaRequestHeader *reqHdr,
                                  Mac48Address originator);
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
  Time m_sbifs;                         //!< Short Beamformnig Interframe Space.
  Time m_mbifs;                         //!< Medium Beamformnig Interframe Space.
  Time m_lbifs;                         //!< Long Beamformnig Interframe Space.
  Time m_brpifs;                        //!< Beam Refinement Protocol Interframe Spacing.
  bool m_supportOFDM;                   //!< Flag to indicate whether we support OFDM PHY layer.
  bool m_supportLpSc;                   //!< Flag to indicate whether we support LP-SC PHY layer.
  uint8_t m_maxScTxMcs;                 //!< The maximum supported MCS for transmission by SC PHY.
  uint8_t m_maxScRxMcs;                 //!< The maximum supported MCS for reception by SC PHY.
  uint8_t m_maxOfdmTxMcs;               //!< The maximum supported MCS for transmission by OFDM PHY.
  uint8_t m_maxOfdmRxMcs;               //!< The maximum supported MCS for reception by OFDM PHY.

  /* Channel Access Period */
  ChannelAccessPeriod m_accessPeriod;   //!< The type of the current channel access period.
  Time m_btiDuration;                   //!< The length of the Beacon Transmission Interval (BTI).
  Time m_atiDuration;                   //!< The length of the ATI period.
  Time m_biStartTime;                   //!< The start time of the BI Interval.
  Time m_beaconInterval;		//!< Interval between beacons.
  bool m_supportRdp;                    //!< Flag to indicate whether we support RDP.
  TracedCallback<Mac48Address, ChannelAccessPeriod, SECTOR_ID, ANTENNA_ID> m_slsCompleted;  //!< Trace callback for SLS completion.
  bool m_atiPresent;                    //!< Flag to indicate if ATI period is present in the current BI.

  /** BTI Access Period Variables **/
  uint8_t m_nextBeacon;                 //!< The number of BIs following the current beacon interval during which the DMG Beacon is not be present.
  uint8_t m_btiPeriodicity;             //!< The periodicity of the BTI in BI.

  /** A-BFT Access Period Variables **/
  Time m_abftDuration;                  //!< The duration of the A-BFT access period.
  uint8_t m_ssSlotsPerABFT;             //!< Number of Sector Sweep Slots Per A-BFT.
  uint8_t m_ssFramesPerSlot;            //!< Number of SSW Frames per Sector Sweep Slot.
  uint8_t m_nextAbft;                   //!< The value of the next A-BFT in DMG Beacon.

  uint8_t m_sectorId;                   //!< Current Sector ID.
  uint8_t m_antennaId;                  //!< Current Antenna ID.
  uint16_t m_totalSectors;              //!< Total number of sectors remaining to cover.
  bool m_pcpHandoverSupport;            //!< Flag to indicate if we support PCP Handover.
  bool m_sectorFeedbackSchedulled;      //!< Flag to indicate if we schedulled SSW Feedback.

  /** ATI Period Variables **/
  Ptr<DmgAtiDca> m_dmgAtiDca;           //!< Dedicated DcaTxop for ATI.

  /* BRP Phase Variables */
  std::map<Mac48Address, bool> m_isBrpResponder;      //!< Map to indicate whether we are BRP Initiator or responder.
  std::map<Mac48Address, bool> m_isBrpSetupCompleted; //!< Map to indicate whether BRP Setup is completed with station.
  std::map<Mac48Address, bool> m_raisedBrpSetupCompleted;
  bool m_recordTrnSnrValues;                          //!< Flag to indicate if we should record reported SNR Values by TRN Fields.
  bool m_requestedBrpTraining;                        //!< Flag to indicate whether BRP Training has been performed.

  uint8_t m_antennaCount;
  uint8_t m_sectorCount;

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
  InformationMap m_informationMap;

  /* DMG Parameteres */
  bool m_isCbapOnly;                            //!< Flag to indicate whether the DTI is allocated to CBAP.
  bool m_isCbapSource;                          //!< Flag to indicate that PCP/AP has higher priority for transmission.
  bool m_announceDmgCapabilities;               //!< Flag to indicate that we announce DMG Capabilities.

  /* Access Period Allocations */
  AllocationID m_currentAllocationID;           //!< The ID of the current allocation.
  AllocationType m_currentAllocation;           //!< The current access period allocation.
  Time m_allocationStarted;                     //!< The time we initiated the allocation.
  Time m_currentAllocationLength;               //!< The length of the current allocation period in MicroSeconds.
  uint8_t m_peerStationAid;                     //!< The AID of the peer DMG STA in the current SP.
  Mac48Address m_peerStationAddress;            //!< The MAC address of the peer DMG STA in the current SP.
  Time m_suspendedPeriodDuration;               //!< The remaining duration of the suspended SP.
  bool m_spSource;                              //!< Flag to indicate if we are the source of the SP.
  bool m_beamformingTxss;                       //!< Flag to inidicate if we perform TxSS during the beamforming service period.
  AllocationFieldList m_allocationList;         //!< List of access period allocations in DTI.

  /* Service Period Channel Access */
  Ptr<ServicePeriod> m_sp;                      //!< Pointer to current service period channel access pbject.

  /* Beamforming Variables */
  bool m_isBeamformingInitiator;                //!< Flag to indicate whether we initaited BF.
  bool m_isResponderTXSS;                       //!< Flag to indicate whether the responder is TxSS or RxSS.
  EventId m_rssEvent;                           //!< Event related to scheduling RSS.

  /** Link Maintenance **/
  BeamLinkMaintenanceUnitIndex m_beamlinkMaintenanceUnit;   //!< Link maintenance Unit according to std 802.11ad-2012.
  uint8_t m_beamlinkMaintenanceValue;                       //!< Link maintenance timer value in MicroSeconds.
  uint32_t dot11BeamLinkMaintenanceTime;                    //!< Link maintenance timer in MicroSeconds.

  typedef struct {
    uint32_t negotiatedValue;
    uint32_t beamLinkMaintenanceTime;
  } BeamLinkMaintenanceInfo;

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

private:
  /**
   * This function is called upon transmission of a 802.11 Management frame.
   * \param hdr The header of the management frame.
   */
  void ManagementTxOk (const WifiMacHeader &hdr);
  /**
   * Report SNR Value, this is a callback to be hooked with the WifiPhy.
   * \param sectorID
   * \param antennaID
   * \param snr
   */
  void ReportSnrValue (SECTOR_ID sectorID, ANTENNA_ID antennaID, uint8_t fieldsRemaining, double snr, bool isTxTrn);

  Mac48Address m_peerStation;     /* The address of the station we are waiting BRP Response from */
  uint8_t m_dialogToken;

};

} // namespace ns3

#endif /* DMG_WIFI_MAC_H */
