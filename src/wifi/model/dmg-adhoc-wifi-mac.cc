/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/boolean.h"
#include "ns3/double.h"

#include "channel-access-manager.h"
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
  for (EdcaQueues::const_iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->SetAllocationType (CBAP_ALLOCATION);
    }
  /* Initialize Codebook */
  m_codebook->Initialize ();
}

void
DmgAdhocWifiMac::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  DmgWifiMac::DoDispose ();
}

uint16_t
DmgAdhocWifiMac::GetAssociationID (void)
{
  NS_LOG_FUNCTION (this);
  return AID_AP;
}

void
DmgAdhocWifiMac::SetAddress (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  RegularWifiMac::SetAddress (address);
  RegularWifiMac::SetBssid (address);
}

void
DmgAdhocWifiMac::Enqueue (Ptr<Packet> packet, Mac48Address to)
{
  NS_LOG_FUNCTION (this << packet << to);
  if (m_stationManager->IsBrandNew (to))
    {
      m_stationManager->AddAllSupportedMcs (to);
      m_stationManager->AddStationDmgCapabilities (to, GetDmgCapabilities ());
      m_stationManager->RecordDisassociated (to);
      if (m_isEdmgSupported)
        {
          m_stationManager->AddStationEdmgCapabilities (to, GetEdmgCapabilities ());
        }
    }

  WifiMacHeader hdr;

  //If we are not a QoS STA then we definitely want to use AC_BE to
  //transmit the packet. A TID of zero will map to AC_BE (through \c
  //QosUtilsMapTidToAc()), so we use that as our default here.
  uint8_t tid = 0;
  hdr.SetAsDmgPpdu ();
  hdr.SetType (WIFI_MAC_QOSDATA);
  hdr.SetQosAckPolicy (WifiMacHeader::NORMAL_ACK);
  hdr.SetQosNoEosp ();
  hdr.SetQosNoAmsdu ();
  //Transmission of multiple frames in the same TXOP is not
  //supported for now
  hdr.SetQosTxopLimit (0);

  //Fill in the QoS control field in the MAC header
  tid = QosUtilsGetTidForPacket (packet);
  //Any value greater than 7 is invalid and likely indicates that
  //the packet had no QoS tag, so we revert to zero, which will
  //mean that AC_BE is used.
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

  /* We are in DMG Ad-Hoc Mode (Experimental Mode) */
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (m_low->GetAddress ());
  hdr.SetAddr3 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();

  m_edca[QosUtilsMapTidToAc (tid)]->Queue (packet, hdr);
}

void
DmgAdhocWifiMac::SetLinkUpCallback (Callback<void> linkUp)
{
  NS_LOG_FUNCTION (this << &linkUp);
  RegularWifiMac::SetLinkUpCallback (linkUp);

  //The approach taken here is that, from the point of view of a STA
  //in IBSS mode, the link is always up, so we immediately invoke the
  //callback if one is set
  linkUp ();
}

void
DmgAdhocWifiMac::StartBeaconInterval (void)
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("This method should not be called in DmgAdhocWifiMac class");
}

void
DmgAdhocWifiMac::EndBeaconInterval (void)
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("This method should not be called in DmgAdhocWifiMac class");
}

void
DmgAdhocWifiMac::StartBeaconTransmissionInterval (void)
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("This method should not be called in DmgAdhocWifiMac class");
}

void
DmgAdhocWifiMac::StartAssociationBeamformTraining (void)
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("This method should not be called in DmgAdhocWifiMac class");
}

void
DmgAdhocWifiMac::StartAnnouncementTransmissionInterval (void)
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("This method should not be called in DmgAdhocWifiMac class");
}

void
DmgAdhocWifiMac::StartDataTransmissionInterval (void)
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("This method should not be called in DmgAdhocWifiMac class");
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
  m_codebook->SetReceivingInQuasiOmniMode ();
  DmgWifiMac::TxOk (packet, hdr);
}

void
DmgAdhocWifiMac::BrpSetupCompleted (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  NS_FATAL_ERROR ("This method should not be called in DmgAdhocWifiMac class");
}

void
DmgAdhocWifiMac::NotifyBrpPhaseCompleted (void)
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("This method should not be called in DmgAdhocWifiMac class");
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
DmgAdhocWifiMac::AddAntennaConfig (SectorID txSectorID, AntennaID txAntennaID,
                                   SectorID rxSectorID, AntennaID rxAntennaID,
                                   Mac48Address address)
{
  ANTENNA_CONFIGURATION_TX antennaConfigTx = std::make_pair (txAntennaID, txSectorID);
  ANTENNA_CONFIGURATION_RX antennaConfigRx = std::make_pair (rxAntennaID, rxSectorID);
  m_bestAntennaConfig[address] = std::make_tuple (antennaConfigTx, antennaConfigRx, 0.0);
  m_codebook->SetReceivingInDirectionalMode ();
}

void
DmgAdhocWifiMac::AddAntennaConfig (SectorID txSectorID, AntennaID txAntennaID,
                                   Mac48Address address)
{
  ANTENNA_CONFIGURATION_TX antennaConfigTx = std::make_pair (txAntennaID, txSectorID);
  ANTENNA_CONFIGURATION_RX antennaConfigRx = std::make_pair (NO_ANTENNA_CONFIG, NO_ANTENNA_CONFIG);
  m_bestAntennaConfig[address] = std::make_tuple (antennaConfigTx, antennaConfigRx, 0.0);
  m_codebook->SetReceivingInQuasiOmniMode ();
}

void
DmgAdhocWifiMac::Receive (Ptr<WifiMacQueueItem> mpdu)
{
  NS_LOG_FUNCTION (this << *mpdu);

  const WifiMacHeader* hdr = &mpdu->GetHeader ();
  NS_ASSERT (!hdr->IsCtl ());
  Mac48Address from = hdr->GetAddr2 ();
  Mac48Address to = hdr->GetAddr1 ();

  if (m_stationManager->IsBrandNew (to))
    {
      //In ad hoc mode, we assume that every destination supports all
      //the rates we support.
      m_stationManager->AddAllSupportedMcs (to);
      m_stationManager->AddStationDmgCapabilities (to, GetDmgCapabilities ());
      m_stationManager->RecordDisassociated (to);
      if (m_isEdmgSupported)
        {
          m_stationManager->AddStationEdmgCapabilities (to, GetEdmgCapabilities ());
        }
    }
  if (hdr->IsData ())
    {
      if (hdr->IsQosData () && hdr->IsQosAmsdu ())
        {
          NS_LOG_DEBUG ("Received A-MSDU from" << from);
          DeaggregateAmsduAndForward (mpdu);
        }
      else
        {
          ForwardUp (mpdu->GetPacket ()->Copy (), from, to);
        }
      return;
    }

  DmgWifiMac::Receive (mpdu);
}

Ptr<DmgCapabilities>
DmgAdhocWifiMac::GetDmgCapabilities (void) const
{
  Ptr<DmgCapabilities> capabilities = Create<DmgCapabilities> ();
  return capabilities;
}

} // namespace ns3
