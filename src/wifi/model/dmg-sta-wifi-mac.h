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
   * \param missed the number of beacons which must be missed
   * before a new association sequence is started.
   */
  void SetMaxMissedBeacons (uint32_t missed);
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
   * Initiate Beamforming
   * \param address
   * \param isTxss Indicate whether the RSS is TxSS or RxSS.
   * \param duration The allocation time of the RSS period.
   */
  void InitiateBeamforming (Mac48Address address, bool isTxss, Time duration);
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
   * Discover all the available relays in BSS.
   * \param The address of the station to establish RLS with.
   */
  void DoRelayDiscovery (Mac48Address stationAddress);
  /**
   * Initiate Relay Link Switching Type Procedure.
   * \param rdsAddress The address of the RDS station.
   */
  void InitiateLinkSwitchingTypeProcedure (Mac48Address rdsAddress);

  Ptr<MultiBandElement> GetMultiBandElement (void) const;
  /**
   * Get Association Identifier (AID)
   * \return
   */
  uint16_t GetAssociationID (void);

  /* Temporary Function to store AID mapping */
  void MapAidToMacAddress (uint16_t aid, Mac48Address address);

protected:
  virtual void DoDispose (void);
  virtual void DoInitialize (void);

  void StartBeaconTransmissionInterval (void);
  void StartAssociationBeamformTraining (void);
  void StartAnnouncementTransmissionInterval (void);
  void StartDataTransmissionInterval (void);
  void FrameTxOk (const WifiMacHeader &hdr);
  void BrpSetupCompleted (Mac48Address address);
  virtual void NotifyBrpPhaseCompleted (void);
  /**
   * Set header addresses
   * \param destAddress
   * \param hdr
   */
  void SetHeaderAddresses (Mac48Address destAddress, WifiMacHeader &hdr);

private:
  /**
   * The current MAC state of the STA.
   */
  enum MacState
  {
    ASSOCIATED,
    WAIT_PROBE_RESP,
    WAIT_ASSOC_RESP,
    BEACON_MISSED,
    REFUSED
  };

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
   * Set the current MAC state.
   *
   * \param value the new state
   */
  void SetState (enum MacState value);
  /**
   * Return the DMG capability of the current STA.
   *
   * \return the DMG capability that we support
   */
  Ptr<DmgCapabilities> GetDmgCapabilities (void) const;

  void SendSprFrame (Mac48Address to);
  /**
   * Start Initiator Sector Sweep (ISS) Phase.
   * \param stationAddress The address of the station.
   * \param isTxss Indicate whether the RSS is TxSS or RxSS.
   * \param duration The allocation time of the ISS period.
   */
  void StartInitiatorSectorSweep (Mac48Address address, bool isTxss, Time duration);
  /**
   * Start Responder Sector Sweep (RSS) Phase.
   * \param stationAddress The address of the station.
   * \param isTxss Indicate whether the RSS is TxSS or RxSS.
   * \param duration The allocation time of the RSS period.
   */
  void StartResponderSectorSweep (Mac48Address stationAddress, bool isTxss, Time duration);
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

  Time GetRemainingAllocationTime (void) const;

private:
  enum MacState m_state;
  Time m_probeRequestTimeout;
  Time m_assocRequestTimeout;
  EventId m_probeRequestEvent;
  EventId m_assocRequestEvent;
  EventId m_beaconWatchdog;
  Time m_beaconWatchdogEnd;
  uint32_t m_maxMissedBeacons;
  bool m_activeProbing;

  uint16_t m_aid;   /* AID of the STA */

  TracedCallback<Mac48Address> m_assocLogger;
  TracedCallback<Mac48Address> m_deAssocLogger;

  EventId   m_abftEvent;                        //!< Association Beamforming training Event.
  bool      m_atiPresent;                       //!< Flag to indicate if ATI period is present in the current BI.
  uint8_t   m_nBI;                              //!< Flag to indicate the number of intervals to allocate A-BFT.

  Ptr<DcaTxop> m_beaconDca;                     //!< Dedicated DcaTxop for beacons.

  /* BTI Beamforming */
  bool m_receivedDmgBeacon;
  uint8_t m_failedRssAttemps;                   //!< Counter for not receiving SSW-Feedback */
  bool    m_isResponderTXSS;                    //!< Flag to indicate whether the A-BFT Period is TXSS or RXSS.
  uint8_t m_slotIndex;                          //!< The index of the selected slot in the A-BFT period.
  EventId m_sswFbckTimeout;                     //!< Timeout Event for receiving SSW FBCK Frame.
  bool    m_scheduledPeriodAfterAbft;           //!< Flag to indicate that we have scheduled a period after A-BFT.
  Ptr<UniformRandomVariable> a_bftSlot;         //!< Random variable for A-BFT slot.
  bool    m_isIssInitiator;                     //!< Flag to indicate that we are ISS.
  EventId m_rssEvent;                           //!< Event related to scheduling
  uint8_t m_remainingSlotsPerABFT;              //!< Remaining Slots in the current A-BFT.
  Time m_sswFbckDuration;                       //!< The duration in the SSW-FBCK Field.

  /* DMG Relay Support Variables */
  bool m_relayInitiator;                //!< Flag to indicate whether this STA established Relay Procedure.
  bool m_waitingDestinationRedsReports; //!< Flag to indicate that we are waiting for destination REDS report.
  Mac48Address m_dstRedsAddress;        //!< The Mac address of the destination REDS.
  uint16_t m_dstRedsAid;
  uint16_t m_selectedRelayAid;
  TracedCallback<Mac48Address> m_channelReportReceived;  //!< Trace callback for SLS completion

  /* Data Forwarding */
  typedef std::list<Mac48Address> DataForwardingMap; /* Destination Address */
  typedef DataForwardingMap::iterator DataForwardingMapIterator;
  DataForwardingMap m_dataForwardingMap;

  /* AID to MAC Address mapping */
  typedef std::map<uint16_t, Mac48Address> AID_MAP;
  typedef std::map<Mac48Address, uint16_t> MAC_MAP;
  AID_MAP m_aidMap;
  MAC_MAP m_macMap;

};

} // namespace ns3

#endif /* STA_WIFI_MAC_H */
