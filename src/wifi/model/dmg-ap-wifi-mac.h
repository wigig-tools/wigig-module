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
#ifndef DMG_AP_WIFI_MAC_H
#define DMG_AP_WIFI_MAC_H

#include "ns3/random-variable-stream.h"

#include "amsdu-subframe-header.h"
#include "dmg-beacon-dca.h"
#include "dmg-wifi-mac.h"

namespace ns3 {

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
  virtual void Enqueue (Ptr<const Packet> packet, Mac48Address to);
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
  virtual void Enqueue (Ptr<const Packet> packet, Mac48Address to, Mac48Address from);
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
   * \param interval the interval.
   */
  void SetBeaconTransmissionInterval (Time interval);
  /**
   * \return the interval between two beacon transmissions.
   */
  Time GetBeaconTransmissionInterval (void) const;
  /**
   * Assign a fixed random variable stream number to the random variables
   * used by this model. Return the number of streams (possibly zero) that
   * have been assigned.
   *
   * \param stream first stream index to use
   * \return the number of stream indices assigned by this model
   */
  int64_t AssignStreams (int64_t stream);

  void SetDMGAntennaCount (uint8_t antennas);
  void SetDMGSectorCount (uint8_t sectors);

  uint8_t GetDMGAntennaCount (void) const;
  uint8_t GetDMGSectorCount (void) const;

  Time GetBTIRemainingTime (void) const;
  Ptr<MultiBandElement> GetMultiBandElement (void) const;
  /**
   * Add an allocation period to be announced in DMG Beacon or Announce Frame.
   * \param allocationType The type of the allocation (CBAP or SP).
   * \param staticAllocation Is the allocation static.
   * \param sourceAid The AID of the source DMG STA.
   * \param destAid The AID of the destination DMG STA.
   * \param allocationStart The start time of the allocation relative to the beginning of DTI.
   * \param blockDuration The duration of the allocation period.
   */
  void AddAllocationPeriod (AllocationType allocationType, bool staticAllocation,
                            uint8_t sourceAid, uint8_t destAid,
                            uint32_t allocationStart, uint16_t blockDuration);
  /**
   * AllocateBeamformingServicePeriod
   * \param sourceAid The AID of the source DMG STA.
   * \param destAid The AID of the destination DMG STA.
   * \param allocationStart The start time of the allocation relative to the beginning of DTI.
   * \param isTxss Is the Beamforming TxSS or RxSS.
   */
  void AllocateBeamformingServicePeriod (uint8_t sourceAid, uint8_t destAid,
                                         uint32_t allocationStart, bool isTxss);

protected:
  virtual void DoDispose (void);
  virtual void DoInitialize (void);

  void StartBeaconTransmissionInterval (void);
  void StartAssociationBeamformTraining (void);
  /**
   * Start A-BFT Sector Sweep Slot.
   */
  void StartSectorSweepSlot (void);
  void StartAnnouncementTransmissionInterval (void);
  void StartDataTransmissionInterval (void);
  void FrameTxOk (const WifiMacHeader &hdr);
  /**
   * Establish BRP Setup Subphase
   */
  void DoBrpSetupSubphase (void);
  virtual void BrpSetupCompleted (Mac48Address address);
  virtual void NotifyBrpPhaseCompleted (void);

private:
  virtual void Receive (Ptr<Packet> packet, const WifiMacHeader *hdr);
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
  virtual void TxFailed(const WifiMacHeader &hdr);
  /**
   * This method is called to de-aggregate an A-MSDU and forward the
   * constituent packets up the stack. We override the WifiMac version
   * here because, as an AP, we also need to think about redistributing
   * to other associated STAs.
   *
   * \param aggregatedPacket the Packet containing the A-MSDU.
   * \param hdr a pointer to the MAC header for \c aggregatedPacket.
   */
  virtual void DeaggregateAmsduAndForward (Ptr<Packet> aggregatedPacket, const WifiMacHeader *hdr);
  /**
   * Forward the packet down to DCF/EDCAF (enqueue the packet). This method
   * is a wrapper for ForwardDown with traffic id.
   *
   * \param packet the packet we are forwarding to DCF/EDCAF
   * \param from the address to be used for Address 3 field in the header
   * \param to the address to be used for Address 1 field in the header
   */
  void ForwardDown (Ptr<const Packet> packet, Mac48Address from, Mac48Address to);
  /**
   * Forward the packet down to DCF/EDCAF (enqueue the packet).
   *
   * \param packet the packet we are forwarding to DCF/EDCAF
   * \param from the address to be used for Address 3 field in the header
   * \param to the address to be used for Address 1 field in the header
   * \param tid the traffic id for the packet
   */
  void ForwardDown (Ptr<const Packet> packet, Mac48Address from, Mac48Address to, uint8_t tid);
  /**
   * Forward a probe response packet to the DCF. The standard is not clear on the correct
   * queue for management frames if QoS is supported. We always use the DCF.
   *
   * \param to the address of the STA we are sending a probe response to
   */
  void SendProbeResp (Mac48Address to);
  /**
   * Forward an association response packet to the DCF. The standard is not clear on the correct
   * queue for management frames if QoS is supported. We always use the DCF.
   *
   * \param to the address of the STA we are sending an association response to
   * \param success indicates whether the association was successful or not
   */
  void SendAssocResp (Mac48Address to, bool success);

  /**
   * Start Polling Phase.
   */
  void StartPollingPhase (void);
  /**
   * Send Poll Frame
   * \param to
   */
  void SendPollFrame (Mac48Address to);
  /**
   * Send Grant Frame.
   * \param to
   */
  void SendGrantFrame (Mac48Address to);
  /**
   * Send Announce Frame
   * \param to
   */
  void SendAnnounceFrame (Mac48Address to);

  /**
   * Return the DMG capability of the current AP.
   *
   * \return the DMG capability that we support
   */
  Ptr<DmgCapabilities> GetDmgCapabilities (void) const;
  /**
   * GetDmgOperationElement
   * \return
   */
  Ptr<DmgOperationElement> GetDmgOperationElement (void) const;
  /**
   * Get Next DMG ATI Element Information Element.
   * \return
   */
  Ptr<NextDmgAti> GetNextDmgAtiElement (void) const;
  /**
   * Get Extended Schedule Element.
   * \return
   */
  Ptr<ExtendedScheduleElement> GetExtendedScheduleElement (void) const;
  /**
   * Cleanup non-static allocations.
   */
  void CleanupAllocations (void);

  /**
   * Send One DMG Beacon Frame with the provided arguments.
   * \param antennaID The ID of the current ID.
   * \param sectorID The ID of the current sector in the antenna.
   * \param count Number of remaining DMG Beacons till the end of BTI.
   */
  void SendOneDMGBeacon (uint8_t sectorID, uint8_t antennaID, uint16_t count);

  /* BTI Period Variables */
  Ptr<DmgBeaconDca> m_beaconDca;        //!< Dedicated DcaTxop for beacons.
  EventId m_beaconEvent;		//!< Event to generate one beacon.

  bool m_atiPresent;                    //!< Flag to indicate whether ATI is present or not.
  DcfManager *m_beaconDcfManager;       //!< DCF manager (access to channel)
  Time m_btiRemaining;                  //!< Remaining Time to the end of the current BTI.
  Time m_beaconTransmitted;             //!< The time at which we transmitted DMG Beacon.

  /** A-BFT Access Period Variables **/
  uint8_t m_nextAbft;                   //!< The value of the next A-BFT in DMG Beacon.
  uint8_t m_abftPeriodicity;            //!< The periodicity of the A-BFT in DMG Beacon.
  /* Ensure only one DMG STA is communicating with us during A-BFT slot */
  bool m_receivedOneSSW;
  Mac48Address m_peerAbftStation;
  bool m_isResponderTXSS;
  uint8_t m_remainingSlots;
  Time m_atiStartTime;                  //!< The start time of ATI Period.

  /** BRP Phase Variables **/
  typedef std::map<Mac48Address, bool> STATION_BRP_MAP;
  STATION_BRP_MAP m_stationBrpMap;      //!< Map to indicate if a station has conducted BRP Phase or not.
  uint16_t m_aidCounter;                //!< Association Identifier.

  typedef std::map<Mac48Address, WifiInformationElementMap> AssociatedStationsInfoByAddress;
  AssociatedStationsInfoByAddress m_associatedStationsInfoByAddress;
  std::map<uint16_t, WifiInformationElementMap> m_associatedStationsInfoByAid;

};

} // namespace ns3

#endif /* DMG_AP_WIFI_MAC_H */
