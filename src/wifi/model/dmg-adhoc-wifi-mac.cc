/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/boolean.h"
#include "ns3/double.h"

#include "dcf-manager.h"
#include "dmg-capabilities.h"
#include "dmg-adhoc-wifi-mac.h"
#include "ext-headers.h"
#include "mac-low.h"
#include "msdu-aggregator.h"
#include "wifi-mac-header.h"
#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DmgAdhocWifiMac");

NS_OBJECT_ENSURE_REGISTERED (DmgAdhocWifiMac);

TypeId
DmgAdhocWifiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DmgAdhocWifiMac")
    .SetParent<DmgWifiMac> ()
    .AddConstructor<DmgAdhocWifiMac> ()
  ;
  return tid;
}

DmgAdhocWifiMac::DmgAdhocWifiMac ()
{
  NS_LOG_FUNCTION (this);
  // Let the lower layers know that we are acting as a non-AP DMG STA in an infrastructure BSS.
  SetTypeOfStation (DMG_ADHOC);
}

DmgAdhocWifiMac::~DmgAdhocWifiMac ()
{
  NS_LOG_FUNCTION(this);
}

void
DmgAdhocWifiMac::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  DmgWifiMac::DoInitialize ();
}

void
DmgAdhocWifiMac::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  DmgWifiMac::DoDispose ();
}

void
DmgAdhocWifiMac::SetAddress (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  //In an IBSS, the BSSID is supposed to be generated per Section
  //11.1.3 of IEEE 802.11. We don't currently do this - instead we
  //make an IBSS STA a bit like an AP, with the BSSID for frames
  //transmitted by each STA set to that STA's address.
  //
  //This is why we're overriding this method.
  RegularWifiMac::SetAddress (address);
  RegularWifiMac::SetBssid (address);
}

void
DmgAdhocWifiMac::Enqueue (Ptr<const Packet> packet, Mac48Address to)
{
  NS_LOG_FUNCTION (this << packet << to);
  if (m_stationManager->IsBrandNew (to))
    {
      //In ad hoc mode, we assume that every destination supports all
      //the rates we support.
      m_stationManager->AddAllSupportedMcs (to);
      m_stationManager->AddStationDmgCapabilities (to, GetDmgCapabilities ());
      m_stationManager->AddAllSupportedModes (to);
      m_stationManager->RecordDisassociated (to);
    }

  WifiMacHeader hdr;

  // If we are not a QoS AP then we definitely want to use AC_BE to
  // transmit the packet. A TID of zero will map to AC_BE (through \c
  // QosUtilsMapTidToAc()), so we use that as our default here.
  uint8_t tid = 0;

  // For now, an AP that supports QoS does not support non-QoS
  // associations, and vice versa. In future the AP model should
  // support simultaneously associated QoS and non-QoS STAs, at which
  // point there will need to be per-association QoS state maintained
  // by the association state machine, and consulted here.

  /* The QoS Data and QoS Null subtypes are the only Data subtypes transmitted by a DMG STA. */
  hdr.SetType (WIFI_MAC_QOSDATA);
  hdr.SetQosAckPolicy (WifiMacHeader::NORMAL_ACK);
  hdr.SetQosNoEosp ();
  hdr.SetQosNoAmsdu ();
  // Transmission of multiple frames in the same TXOP is not
  // supported for now.
  hdr.SetQosTxopLimit (0);
  // Fill in the QoS control field in the MAC header
  tid = QosUtilsGetTidForPacket (packet);
  // Any value greater than 7 is invalid and likely indicates that
  // the packet had no QoS tag, so we revert to zero, which'll
  // mean that AC_BE is used.
  if (tid > 7)
    {
      tid = 0;
    }
  hdr.SetQosTid (tid);
  /* DMG QoS Control*/
  hdr.SetQosRdGrant (m_supportRdp);
  /* The HT Control field is not present in frames transmitted by a DMG STA.
   * The presence of the HT Control field is determined by the Order
   * subfield of the Frame Control field, as specified in 8.2.4.1.10.*/
  hdr.SetNoOrder ();

  // Sanity check that the TID is valid
  NS_ASSERT (tid < 8);

  /* We are in DMG Ad-Hoc Mode (Experimental Mode) */
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (m_low->GetAddress ());
  hdr.SetAddr3 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();

  m_edca[QosUtilsMapTidToAc (tid)]->Queue (packet, hdr);
}

void
DmgAdhocWifiMac::StartBeaconInterval (void)
{
  NS_LOG_FUNCTION (this);
}

void
DmgAdhocWifiMac::EndBeaconInterval (void)
{
  NS_LOG_FUNCTION (this);
}

void
DmgAdhocWifiMac::StartBeaconTransmissionInterval (void)
{
  NS_LOG_FUNCTION (this);
}

void
DmgAdhocWifiMac::StartAssociationBeamformTraining (void)
{
  NS_LOG_FUNCTION (this);
}

void
DmgAdhocWifiMac::StartAnnouncementTransmissionInterval (void)
{
  NS_LOG_FUNCTION (this);
}

void
DmgAdhocWifiMac::StartDataTransmissionInterval (void)
{
  NS_LOG_FUNCTION (this);
}

void
DmgAdhocWifiMac::FrameTxOk (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this);
}

void
DmgAdhocWifiMac::TxOk (Ptr<const Packet> packet, const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this);
  /* After transmission we stay in quasi-omni mode since we do not know which station will transmit to us */
  m_phy->GetDirectionalAntenna ()->SetInOmniReceivingMode ();
  DmgWifiMac::TxOk (packet, hdr);
}

void
DmgAdhocWifiMac::BrpSetupCompleted (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
}

void
DmgAdhocWifiMac::NotifyBrpPhaseCompleted (void)
{
  NS_LOG_FUNCTION (this);
}

Ptr<MultiBandElement>
DmgAdhocWifiMac::GetMultiBandElement (void) const
{
  Ptr<MultiBandElement> multiband = Create<MultiBandElement> ();
  multiband->SetStaRole (ROLE_NON_PCP_NON_AP);
  multiband->SetStaMacAddressPresent (false); /* The same MAC address is used across all the bands */
  multiband->SetBandID (Band_4_9GHz);
  multiband->SetOperatingClass (18);          /* Europe */
  multiband->SetChannelNumber (1);
  multiband->SetBssID (GetBssid ());
  multiband->SetConnectionCapability (1);     /* AP */
  multiband->SetFstSessionTimeout (1);
  return multiband;
}

void
DmgAdhocWifiMac::AddAntennaConfig (SECTOR_ID txSectorID, ANTENNA_ID txAntennaID,
                                   SECTOR_ID rxSectorID, ANTENNA_ID rxAntennaID,
                                   Mac48Address address)
{
  ANTENNA_CONFIGURATION_TX antennaConfigTx = std::make_pair (txSectorID, txAntennaID);
  ANTENNA_CONFIGURATION_RX antennaConfigRx = std::make_pair (rxSectorID, rxAntennaID);
  m_bestAntennaConfig[address] = std::make_pair (antennaConfigTx, antennaConfigRx);
}

void
DmgAdhocWifiMac::Receive (Ptr<Packet> packet, const WifiMacHeader *hdr)
{
  NS_LOG_FUNCTION (this << packet << hdr);
  NS_ASSERT (!hdr->IsCtl ());
  Mac48Address from = hdr->GetAddr2 ();
  Mac48Address to = hdr->GetAddr1 ();
  if (m_stationManager->IsBrandNew (to))
    {
      //In ad hoc mode, we assume that every destination supports all
      //the rates we support.
      m_stationManager->AddAllSupportedMcs (to);
      m_stationManager->AddStationDmgCapabilities (to, GetDmgCapabilities ());
      m_stationManager->AddAllSupportedModes (to);
      m_stationManager->RecordDisassociated (to);
    }
  if (hdr->IsData ())
    {
      if (hdr->IsData ())
        {
          if (hdr->IsQosData () && hdr->IsQosAmsdu ())
            {
              NS_LOG_DEBUG ("Received A-MSDU from" << from);
              DeaggregateAmsduAndForward (packet, hdr);
            }
          else
            {
              ForwardUp (packet, from, to);
            }
          return;
        }
    }

  DmgWifiMac::Receive (packet, hdr);
}

Ptr<DmgCapabilities>
DmgAdhocWifiMac::GetDmgCapabilities (void) const
{
  Ptr<DmgCapabilities> capabilities = Create<DmgCapabilities> ();
  return capabilities;
}

} // namespace ns3
