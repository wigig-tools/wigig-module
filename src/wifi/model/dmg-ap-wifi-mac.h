/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@hotmail.com>
 */
#ifndef DMG_AP_WIFI_MAC_H
#define DMG_AP_WIFI_MAC_H

#include "ns3/random-variable-stream.h"

#include "amsdu-subframe-header.h"
#include "dmg-beacon-dca.h"
#include "dmg-wifi-mac.h"

namespace ns3 {

#define TU                      1024            /* Time Unit defined in 802.11 std */
#define aMaxBIDuration          TU * 1024       /* Maximum BI Duration Defined in 802.11ad */
#define aMinChannelTime         aMaxBIDuration  /* Minimum Channel Time for Clustering */
#define aMinSSSlotsPerABFT      1               /* Minimum Number of Sector Sweep Slots Per A-BFT */
#define aSSFramesPerSlot        8               /* Number of SSW Frames per Sector Sweep Slot */
#define aDMGPPMinListeningTime  150             /* The minimum time between two adjacent SPs with the same source or destination AIDs*/

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
   * Allocate CBAP period to be announced in DMG Beacon or Announce Frame.
   * \param staticAllocation Is the allocation static.
   * \param allocationStart The start time of the allocation relative to the beginning of DTI.
   * \param blockDuration The duration of the allocation period.
   * \return The start of the next allocation period.
   */
  uint32_t AllocateCbapPeriod (bool staticAllocation, uint32_t allocationStart, uint16_t blockDuration);
  /**
   * Add a new allocation period to be announced in DMG Beacon or Announce Frame.
   * \param allocationID The unique identifier o
   * \param allocationType The type of the allocation (CBAP or SP).
   * \param staticAllocation Is the allocation static.
   * \param sourceAid The AID of the source DMG STA.
   * \param destAid The AID of the destination DMG STA.
   * \param allocationStart The start time of the allocation relative to the beginning of DTI.
   * \param blockDuration The duration of the allocation period.
   * \return The start of the next allocation period.
   */
  uint32_t AddAllocationPeriod (AllocationID allocationID,
                                AllocationType allocationType, bool staticAllocation,
                                uint8_t sourceAid, uint8_t destAid,
                                uint32_t allocationStart, uint16_t blockDuration);
  /**
   * AllocateBeamformingServicePeriod
   * \param sourceAid The AID of the source DMG STA.
   * \param destAid The AID of the destination DMG STA.
   * \param allocationStart The start time of the allocation relative to the beginning of DTI.
   * \param isTxss Is the Beamforming TxSS or RxSS.
   * \return The start of the next allocation period.
   */
  uint32_t AllocateBeamformingServicePeriod (uint8_t sourceAid, uint8_t destAid,
                                             uint32_t allocationStart, bool isTxss);

protected:
  friend class DmgBeaconDca;

  Time GetBTIRemainingTime (void) const;

private:
  virtual void DoDispose (void);
  virtual void DoInitialize (void);

  void StartBeaconInterval (void);
  void StartBeaconTransmissionInterval (void);
  void StartAssociationBeamformTraining (void);
  void StartAnnouncementTransmissionInterval (void);
  void StartDataTransmissionInterval (void);
  void FrameTxOk (const WifiMacHeader &hdr);
  virtual void BrpSetupCompleted (Mac48Address address);
  virtual void NotifyBrpPhaseCompleted (void);
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
  virtual void TxFailed (const WifiMacHeader &hdr);
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
   * \param to the MAC address of the STA we are sending a probe response to.
   */
  void SendProbeResp (Mac48Address to);
  /**
   * Forward an association response packet to the DCF. The standard is not clear on the correct
   * queue for management frames if QoS is supported. We always use the DCF.
   *
   * \param to the MAC address of the STA we are sending an association response to.
   * \param success indicates whether the association was successful or not.
   */
  void SendAssocResp (Mac48Address to, bool success);
  /**
   * Send Announce Frame
   * \param to The MAC address of the DMG STA.
   */
  void SendAnnounceFrame (Mac48Address to);
  /**
   * Return the DMG capability of the current AP.
   * \return the DMG capability that we support
   */
  Ptr<DmgCapabilities> GetDmgCapabilities (void) const;
  /**
   * Get DMG Operation Element.
   * \return Pointer to the DMG Operation element.
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
   * Cleanup non-static allocations. This is method is called after the transmission of the last DMG Beacon.
   */
  void CleanupAllocations (void);
  /**
   * Send One DMG Beacon Frame with the provided arguments.
   * \param antennaID The ID of the current Antenna.
   * \param sectorID The ID of the current sector in the antenna.
   * \param count Number of remaining DMG Beacons till the end of BTI.
   */
  void SendOneDMGBeacon (uint8_t sectorID, uint8_t antennaID, uint16_t count);

  /** BTI Period Variables **/
  Ptr<DmgBeaconDca> m_beaconDca;        //!< Dedicated DcaTxop for beacons.
  EventId m_beaconEvent;		//!< Event to generate one beacon.
  DcfManager *m_beaconDcfManager;       //!< DCF manager (access to channel)
  Time m_btiRemaining;                  //!< Remaining Time to the end of the current BTI.
  Time m_beaconTransmitted;             //!< The time at which we transmitted DMG Beacon.
  std::vector<ANTENNA_CONFIGURATION> m_antennaConfigurationTable; //! Table with the current antenna configuration.
  uint32_t m_antennaConfigurationIndex; //! Index of the current antenna configuration.
  uint32_t m_antennaConfigurationOffset;//! The first antenna configuration to start BTI with.
  bool m_beaconRandomization;           //!< Flag to indicate whether we want to randomize selection of DMG Beacon at each BI.

  /** A-BFT Access Period Variables **/
  bool m_isResponderTXSS;               //!< Flag to indicate if RSS in A-BFT is TxSS or RxSS.
  uint8_t m_abftPeriodicity;            //!< The periodicity of the A-BFT in DMG Beacon.
  /* Ensure only one DMG STA is communicating with us during single A-BFT slot */
  bool m_receivedOneSSW;                //!< Flag to indicate if we received SSW Frame during SSW-Slot in A-BFT.
  Mac48Address m_peerAbftStation;       //!< The MAC address of the station we received SSW from.
  uint8_t m_remainingSlots;
  Time m_atiStartTime;                  //!< The start time of ATI Period.

  /** BRP Phase Variables **/
  typedef std::map<Mac48Address, bool> STATION_BRP_MAP;
  STATION_BRP_MAP m_stationBrpMap;      //!< Map to indicate if a station has conducted BRP Phase or not.
  uint16_t m_aidCounter;                //!< Association Identifier.

  /**
   * Type definition for storing IEs of the associated stations.
   */
  typedef std::map<Mac48Address, WifiInformationElementMap> AssociatedStationsInfoByAddress;
  AssociatedStationsInfoByAddress m_associatedStationsInfoByAddress;
  std::map<uint16_t, WifiInformationElementMap> m_associatedStationsInfoByAid;

  /**
   * TracedCallback signature for DTI access period start event.
   *
   * \param address The MAC address of the station.
   * \param duration The duration of the DTI period.
   */
  typedef void (* DtiStartedCallback)(Mac48Address address, Time duration);

  TracedCallback<Mac48Address> m_biStarted;         //!< New BI Started has started.
  TracedCallback<Mac48Address, Time> m_dtiStarted;  //!< DTI Started has started.

};

} // namespace ns3

#endif /* DMG_AP_WIFI_MAC_H */
