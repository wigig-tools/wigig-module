/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
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
 * Author: Hany Assasa <Hany.assasa@gmail.com>
 */
#ifndef DMG_WIFI_MAC_H
#define DMG_WIFI_MAC_H

#include "regular-wifi-mac.h"
#include "dmg-ati-dca.h"
#include "dmg-capabilities.h"

namespace ns3 {

#define dot11MaxBFTime                  10
#define dot11BFTXSSTime                 10
#define dot11MaximalSectorScan          10
#define dot11ABFTRTXSSSwitch            10
#define dot11RSSRetryLimit              10
#define dot11RSSBackoff                 10
#define dot11BFRetryLimit               10
#define dot11BeamLinkMaintenanceTime    10
#define dot11AntennaSwitchingTime       10
#define dot11ChanMeasFBCKNtaps          10
// Frames duration precalculated using MCS0 and reported in nanoseconds
#define sswTxTime     NanoSeconds (4291) + NanoSeconds (4654) + NanoSeconds (5964)
#define sswFbckTxTime NanoSeconds (4291) + NanoSeconds (4654) + NanoSeconds (9310)
#define sswAckTxTime  NanoSeconds (4291) + NanoSeconds (4654) + NanoSeconds (9310)

class BeamRefinementElement;

typedef enum {
  CHANNEL_ACCESS_BTI = 0,
  CHANNEL_ACCESS_A_BFT,
  CHANNEL_ACCESS_ATI,
  CHANNEL_ACCESS_BHI,
  CHANNEL_ACCESS_DTI
} ChannelAccessPeriod;

typedef uint8_t SECTOR_ID;                    /* Typedef for Sector ID */
typedef uint8_t ANTENNA_ID;                   /* Typedef for Antenna ID */

/**
 * \brief Wi-Fi DMG
 * \ingroup wifi
 *
 * Abstract class for DMG support.
 */
class DmgWifiMac : public RegularWifiMac
{
public:
  static TypeId GetTypeId (void);

  DmgWifiMac ();
  virtual ~DmgWifiMac ();

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

  void SetDMGAntennaCount (uint8_t antennas);
  void SetDMGSectorCount (uint8_t sectors);

  uint8_t GetDMGAntennaCount (void) const;
  uint8_t GetDMGSectorCount (void) const;

  /**
   * \param sbifs the value of Short Beamforming Interframe Space (SBIFS)
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
   * Beam Refinement Protocol Interframe Spacing (BRPIFS).
   */
  void SetBrpifs (Time brpifs);

  Time GetSbifs (void) const;
  Time GetMbifs (void) const;
  Time GetLbifs (void) const;
  Time GetBrpifs (void) const;

  /**
   * Set whether PCP Handover is supported or not.
   * \param value
   */
  void SetPcpHandoverSupport (bool value);
  bool GetPcpHandoverSupport (void) const;
  /**
   * Map received SNR value to a specific address and TX antenna configuration (The Tx of the peer station).
   * \param address The address of the receiver.
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
   * \param snr The received Signal to Noise Ration in dBm.
   */
  void MapRxSnr (Mac48Address address, SECTOR_ID sectorID, ANTENNA_ID antennaID, double snr);
  /**
   * Tear down relay operation by either RDS or source REDS.
   * \param to The address of the REDS or RDS.
   * \param apAddress The address of the PCP/AP.
   * \param destinationAddress The address of the REDS.
   * \param source_aid The source AID of the REDS.
   * \param destination_aid The destination AID of the REDS.
   * \param relay_aid The source AID of the RDS.
   */
  void TeardownRelay (Mac48Address to, Mac48Address apAddress, Mac48Address destinationAddress,
                      uint16_t source_aid, uint16_t destination_aid, uint16_t relay_aid);
  /**
   * Send Information Request
   * \param to The address of the receiving node.
   * \param request Pointer to the Request Element.
   */
  void SendInformationRequest (Mac48Address to, ExtInformationRequest &requestHdr);
  /**
   * Send Information Rsponse
   * \param to
   * \param responseHdr
   */
  void SendInformationRsponse (Mac48Address to, ExtInformationResponse &responseHdr);
  /**
   * Send Channel Measurement Request to specific DMG STA.
   * \param to The MAC address of the destination STA.
   * \param token The dialog token.
   */
  void SendChannelMeasurementRequest (Mac48Address to, uint8_t token);

  /* Temporary */
  void ChangeActiveTxSector (Mac48Address address);
  void ChangeActiveRxSector (Mac48Address address);

  /* Temporary since we do not have TDMA */
  void StayInOmniReceiveMode (void);

protected:
  /**
   * Return the DMG capability of the current STA.
   *
   * \return the DMG capability that we support
   */
  virtual Ptr<DmgCapabilities> GetDmgCapabilities (void) const = 0;
  /**
   * Get DMG Relay Capabilities support by this DMG STA.
   * \return
   */
  Ptr<RelayCapabilitiesElement> GetRelayCapabilities (void) const;
  /**
   * GetRelayTransferParameterSet
   * \return
   */
  Ptr<RelayTransferParameterSetElement> GetRelayTransferParameterSet (void) const;
  /**
   * SendRelaySearchRequest
   * \param token
   * \param destinationAid
   */
  void SendRelaySearchRequest (uint8_t token, uint16_t destinationAid);
  /**
   * SendRelaySearchResponse
   * \param to
   * \param token
   */
  void SendRelaySearchResponse (Mac48Address to, uint8_t token);
  /**
   * SendChannelMeasurementReport
   * \param to
   * \param token
   * \param List of channel measurement information between sending station and other stations.
   */
  void SendChannelMeasurementReport (Mac48Address to, uint8_t token, ChannelMeasurementInfoList &measurementList);
  /**
   * SetupRls
   * \param to
   * \param token
   * \param source_aid
   * \param relay_aid
   * \param destination_aid
   */
  void SetupRls (Mac48Address to, uint8_t token, uint16_t source_aid, uint16_t relay_aid, uint16_t destination_aid);
  /**
   * SendRlsResponse
   * \param to
   * \param token
   */
  void SendRlsResponse (Mac48Address to, uint8_t token);
  /**
   * SendRlsAnnouncment
   * \param to
   * \param destination_aid
   * \param relay_aid
   * \param source_aid
   */
  void SendRlsAnnouncment (Mac48Address to, uint16_t destination_aid, uint16_t relay_aid, uint16_t source_aid);
  /**
   * SendRelayTeardown
   * \param to
   * \param source_aid
   * \param destination_aid
   * \param relay_aid
   */
  void SendRelayTeardown (Mac48Address to, uint16_t source_aid, uint16_t destination_aid, uint16_t relay_aid);

  void SendSswFbckAfterRss (Mac48Address receiver);
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
  virtual void NotifyBrpPhaseCompleted (void) = 0;

  /* Typedefs for Recording SNR Value per Antenna Configuration */
  typedef double SNR;                                                   /* Typedef SNR */
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

  virtual void DoDispose (void);
  virtual void DoInitialize (void);
  virtual void Configure80211ad (void);
  /**
   * This method can be called to accept a received ADDBA Request. An
   * ADDBA Response will be constructed and queued for transmission.
   *
   * \param reqHdr a pointer to the received ADDBA Request header.
   * \param originator the MAC address of the originator.
   */
  virtual void SendAddBaResponse (const MgtAddBaRequestHeader *reqHdr,
                                  Mac48Address originator);

  virtual void ConfigureAggregation (void);
  virtual void EnableAggregation (void);
  virtual void DisableAggregation (void);

  /**
   * Start Beacon Transmission Interval (BTI)
   */
  virtual void StartBeaconTransmissionInterval (void) = 0;
  /**
   * Start Association Beamform Training (A-BFT)
   */
  virtual void StartAssociationBeamformTraining (void) = 0;
  /**
   * Start Announcement Transmission Interval (ATI)
   */
  virtual void StartAnnouncementTransmissionInterval (void) = 0;
  /**
   * Start Data Transmission Interval (DTI)
   */
  virtual void StartDataTransmissionInterval (void) = 0;
  /**
   * Start contention based access period (CBAP)
   * \param contentionDuration The duration of the contention period.
   */
  void StartContentionPeriod (Time contentionDuration);
  /**
   * Start service  period (SP)
   * \param length The length of the allocation period.
   * \param isBeamforming Inidicate whether the SP is for beamforming training.
   */
  void StartServicePeriod (Time length, Mac48Address peerStation, bool isSource);
  /**
   * End service period
   */
  void EndServicePeriod ();
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

protected:
  STATION_SNR_PAIR_MAP m_stationSnrMap;           //!< Map between stations and their SNR Table.
  STATION_ANTENNA_CONFIG_MAP m_bestAntennaConfig; //!< Map between remote stations and the best antenna configuration.
  ANTENNA_CONFIGURATION m_feedbackAntennaConfig;  //!< Temporary variable to save the best antenna config;

  Time m_sbifs;                         //!< Short Beamformnig Interframe Space.
  Time m_mbifs;                         //!< Medium Beamformnig Interframe Space.
  Time m_lbifs;                         //!< Long Beamformnig Interframe Space.
  Time m_brpifs;                        //!< Beam Refinement Protocol Interframe Spacing.
  ChannelAccessPeriod m_accessPeriod;   //!< The type of the current channel access period.
  Time m_btiDuration;                   //!< The length of the Beacon Transmission Interval (BTI).
  Time m_atiDuration;                   //!< The length of the ATI period.
  Time m_btiStarted;                    //!< The start time of the BTI Period.
  Time m_beaconInterval;		//!< Interval between beacons.
  bool m_supportRdp;                    //!< Flag to indicate whether we support RDP.
  TracedCallback<Mac48Address, ChannelAccessPeriod, SECTOR_ID, ANTENNA_ID> m_slsCompleted;  //!< Trace callback for SLS completion.

  /* A-BFT Variables */
  Time m_abftDuration;                  //!< The duration of the A-BFT access period.
  uint8_t m_ssSlotsPerABFT;             //!< Number of Sector Sweep Slots Per A-BFT.
  uint8_t m_ssFramesPerSlot;            //!< Number of SSW Frames per Sector Sweep Slot.

  uint8_t m_sectorId;                   //!< Current Sector ID.
  uint8_t m_antennaId;                  //!< Current Antenna ID.
  uint16_t m_totalSectors;              //!< Total number of sectors remaining to cover.
  bool m_pcpHandoverSupport;            //!< Flat to indicate if we support PCP Handover.
  std::map<Mac48Address, bool> m_sectorFeedbackSent;  //!< Map to indicate whether we sent SSW Feedback to a station or not.

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
  RelayCapableStaList m_rdsList;                //!< List of Relay STAs (RDS).
  bool m_redsActivated;                         //!< DMG Station supports REDS.
  bool m_rdsActivated;                          //!< DMG Station supports RDS.
  bool m_rdsOperational;                        //!< DMG Station supports is operation in RDS.
  Mac48Address m_selectedRelayAddress;          //!< Address of the selected RDS.
  Mac48Address m_srcRedsAddress;                //!< Address of the Source REDS.
  bool m_relayMode;                             //!< Flag to indicate we are in relay mode.
  TracedCallback<Mac48Address> m_rlsCompleted;  //!< Flag to indicate we are completed RLS setup.

  /* Information Request/Response */
  typedef std::vector<Ptr<WifiInformationElement> > WifiInformationElementList;
  typedef std::pair<Ptr<DmgCapabilities>, WifiInformationElementList> StationInformation;
  typedef std::map<Mac48Address, StationInformation> InformationMap;
  typedef InformationMap::iterator InformationMapIterator;
  InformationMap m_informationMap;

  /* DMG Parameteres */
  bool m_isCbapOnly;                    //!< Flag to indicate whether the DTI is allocated to CBAP.
  bool m_isCbapSource;                  //!< Flag to indicate that PCP/AP has higher priority for transmission.

  /* Access Period Allocations */
  Time m_allocationStarted;             //!< The time we initiated the allocation.
  Time m_currentAllocationLength;       //!< The length of the current allocation period in MicroSeconds.
  AllocationType m_currentAllocation;   //!< The current access period allocation.
  std::list<Mac48Address> m_spStations; //!< List of stations to transmit to in SP.
  uint8_t m_allocationID;               //!< The ID couter for allocations.
  AllocationFieldList m_allocationList; //!< List of access periods allocation in DTI.

  /* Service Period Channel Access */
  Ptr<ServicePeriod> m_sp;              //!< Pointer to service period channel access pbject.

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
