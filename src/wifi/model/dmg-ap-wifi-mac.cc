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
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/boolean.h"

#include "amsdu-subframe-header.h"
#include "dcf-manager.h"
#include "dmg-ap-wifi-mac.h"
#include "ext-headers.h"
#include "mac-low.h"
#include "mac-rx-middle.h"
#include "mac-tx-middle.h"
#include "msdu-aggregator.h"
#include "qos-tag.h"
#include "wifi-phy.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DmgApWifiMac");

NS_OBJECT_ENSURE_REGISTERED (DmgApWifiMac);

#define TU                      1024        /* Time Unit defined in 802.11 std */
#define aMaxBIDuration          TU * 1024   /* Maximum BI Duration Defined in 802.11ad */
#define aMinSSSlotsPerABFT      1           /* Minimum Number of Sector Sweep Slots Per A-BFT */
#define aSSFramesPerSlot        8           /* Number of SSW Frames per Sector Sweep Slot */
#define aDMGPPMinListeningTime  150         /* The minimum time between two adjacent SPs with the same source or destination AIDs*/

TypeId
DmgApWifiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DmgApWifiMac")
    .SetParent<DmgWifiMac> ()
    .SetGroupName ("Wifi")
    .AddConstructor<DmgApWifiMac> ()
    .AddAttribute ("BeaconInterval", "The interval between two Target Beacon Transmission Times (TBTTs).",
                   TimeValue (MicroSeconds (aMaxBIDuration)),
                   MakeTimeAccessor (&DmgApWifiMac::GetBeaconInterval,
                                     &DmgApWifiMac::SetBeaconInterval),
                   MakeTimeChecker ())
    .AddAttribute ("ATIPresent", "The BI period contains ATI access period.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&DmgApWifiMac::m_atiPresent),
                   MakeBooleanChecker ())
     .AddAttribute ("BeaconTransmissionInterval", "The duration of the BTI period.",
                    TimeValue (MicroSeconds (400)),
                    MakeTimeAccessor (&DmgApWifiMac::GetBeaconTransmissionInterval,
                                      &DmgApWifiMac::SetBeaconTransmissionInterval),
                   MakeTimeChecker ())
     .AddAttribute ("SSSlotsPerABFT", "Number of Sector Sweep Slots Per A-BFT.",
                    UintegerValue (aMinSSSlotsPerABFT),
                    MakeUintegerAccessor (&DmgApWifiMac::m_ssSlotsPerABFT),
                    MakeUintegerChecker<uint8_t> (1, 8))
     .AddAttribute ("SSFramesPerSlot", "Number of SSW Frames per Sector Sweep Slot.",
                    UintegerValue (aSSFramesPerSlot),
                    MakeUintegerAccessor (&DmgApWifiMac::m_ssFramesPerSlot),
                    MakeUintegerChecker<uint8_t> (1, 16))
     .AddAttribute ("IsResponderTxss", "Indicates whether the A-BFT period is TxSS or RxSS",
                    BooleanValue (true),
                    MakeBooleanAccessor (&DmgApWifiMac::m_isResponderTXSS),
                    MakeBooleanChecker ())

      /* DMG Parameters */
     .AddAttribute ("CBAPSource", "Indicates that PCP/AP has a higher priority for transmission in CBAP",
                    BooleanValue (false),
                    MakeBooleanAccessor (&DmgApWifiMac::m_isCbapSource),
                    MakeBooleanChecker ())

      /* DMG Related Attributes */
      /* Dot11DMGBeamformingConfigEntry fom Annex C MIB */
//    .AddAttribute ("dot11MaxBFTime", "Maximum Beamforming Time (in units of beacon interval).",
//                   UintegerValue (4),
//                   MakeUintegerAccessor (&YansWifiPhy::m_nTxPower),
//                   MakeUintegerChecker<uint32_t> ())
//    .AddAttribute ("dot11BFTXSSTime", "Timeout until the initiator restarts ISS (in Milliseconds).",
//                   UintegerValue (100),
//                   MakeUintegerAccessor (&YansWifiPhy::m_nTxPower),
//                   MakeUintegerChecker<uint32_t> ())
//    .AddAttribute ("dot11MaximalSectorScan", "Maximal Sector Scan (in units of beacon interval).",
//                   UintegerValue (8),
//                   MakeUintegerAccessor (&YansWifiPhy::m_nTxPower),
//                   MakeUintegerChecker<uint32_t> ())
//    .AddAttribute ("dot11ABFTRTXSSSwitch", "A-BFT Transmit Sector Sweep Switch (in units of beacon interval).",
//                   UintegerValue (4),
//                   MakeUintegerAccessor (&YansWifiPhy::m_nTxPower),
//                   MakeUintegerChecker<uint32_t> ())
//    .AddAttribute ("dot11RSSRetryLimit", "Responder Sector Sweep Retry Limit.",
//                   UintegerValue (8),
//                   MakeUintegerAccessor (&YansWifiPhy::m_nTxPower),
//                   MakeUintegerChecker<uint32_t> ())
//    .AddAttribute ("dot11RSSBackoff", "Responder Sector Sweep Backoff.",
//                   UintegerValue (8),
//                   MakeUintegerAccessor (&YansWifiPhy::m_nTxPower),
//                   MakeUintegerChecker<uint32_t> ())
//    .AddAttribute ("dot11BFRetryLimit", "Beamforming Retry Limit.",
//                   UintegerValue (8),
//                   MakeUintegerAccessor (&YansWifiPhy::m_nTxPower),
//                   MakeUintegerChecker<uint32_t> ())
//    .AddAttribute ("dot11BeamLinkMaintenanceTime",
//                   "dot11BeamLinkMaintenanceTime = BeamLink_Maintenance_Unit * BeamLink_Maintenance_Value."
//                   "Otherwise, the dot11BeamLinkMaintenanceTime is left undefined."
//                   "An undefined value of the dot11BeamLinkMaintenanceTime indicates"
//                   "that the STA does not participate in beamformed link maintenance.",
//                   UintegerValue (0),
//                   MakeUintegerAccessor (&YansWifiPhy::m_nTxPower),
//                   MakeUintegerChecker<uint32_t> ())
    /* Dot11DMGBeamformingConfigEntry fom Annex C MIB */
  ;
  return tid;
}

DmgApWifiMac::DmgApWifiMac ()
{
  NS_LOG_FUNCTION (this);

  /* DMG Beacon DCF Manager */
  m_beaconDca = CreateObject<DmgBeaconDca> ();
  m_beaconDca->SetAifsn (0);
  m_beaconDca->SetMinCw (0);
  m_beaconDca->SetMaxCw (0);
  m_beaconDca->SetWifiMac (this);
  m_beaconDca->SetLow (m_low);
  m_beaconDca->SetManager (m_dcfManager);
  m_beaconDca->SetTxOkNoAckCallback (MakeCallback (&DmgApWifiMac::FrameTxOk, this));

  /* Constant Values */
  m_abftPeriodicity = 0;
  m_nextAbft = m_abftPeriodicity;
  m_receivedOneSSW = false;
  m_allocationID = 0;

  // Let the lower layers know that we are acting as an AP.
  SetTypeOfStation (DMG_AP);
}

DmgApWifiMac::~DmgApWifiMac ()
{
  NS_LOG_FUNCTION (this);
}

void 
DmgApWifiMac::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_beaconDca = 0;
  m_beaconEvent.Cancel ();
  DmgWifiMac::DoDispose ();
}

void 
DmgApWifiMac::SetAddress (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  // As an AP, our MAC address is also the BSSID. Hence we are
  // overriding this function and setting both in our parent class.
  RegularWifiMac::SetAddress (address);
  RegularWifiMac::SetBssid (address);
}

Time 
DmgApWifiMac::GetBeaconInterval(void) const
{
  NS_LOG_FUNCTION (this);
  return m_beaconInterval;
}

void
DmgApWifiMac::SetBeaconTransmissionInterval (Time interval)
{
  NS_LOG_FUNCTION (this);
  m_btiDuration = interval;
}

Time
DmgApWifiMac::GetBeaconTransmissionInterval (void) const
{
  NS_LOG_FUNCTION (this);
  return m_btiDuration;
}

void 
DmgApWifiMac::SetWifiRemoteStationManager (Ptr<WifiRemoteStationManager> stationManager)
{
  NS_LOG_FUNCTION (this << stationManager);
  m_beaconDca->SetWifiRemoteStationManager (stationManager);
  DmgWifiMac::SetWifiRemoteStationManager (stationManager);
}

void 
DmgApWifiMac::SetLinkUpCallback (Callback<void> linkUp)
{
  NS_LOG_FUNCTION (this << &linkUp);
  RegularWifiMac::SetLinkUpCallback (linkUp);

  // The approach taken here is that, from the point of view of an AP,
  // the link is always up, so we immediately invoke the callback if
  // one is set.
  linkUp ();
}

void 
DmgApWifiMac::SetBeaconInterval (Time interval)
{
  NS_LOG_FUNCTION (this << interval);
  if ((interval.GetMicroSeconds () % 1024) != 0)
    {
      NS_LOG_WARN ("beacon interval should be multiple of 1024us (802.11 time unit), see IEEE Std. 802.11-2012");
    }
  m_beaconInterval = interval;
}

int64_t 
DmgApWifiMac::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  return 1;
}

void 
DmgApWifiMac::ForwardDown (Ptr<const Packet> packet, Mac48Address from, Mac48Address to)
{
  NS_LOG_FUNCTION (this << packet << from << to);
  // If we are not a QoS AP then we definitely want to use AC_BE to
  // transmit the packet. A TID of zero will map to AC_BE (through \c
  // QosUtilsMapTidToAc()), so we use that as our default here.
  uint8_t tid = 0;

   // If we are a QoS AP then we attempt to get a TID for this packet
  if (m_qosSupported)
    {
      tid = QosUtilsGetTidForPacket (packet);
      // Any value greater than 7 is invalid and likely indicates that
      // the packet had no QoS tag, so we revert to zero, which'll
      // mean that AC_BE is used.
      if (tid > 7)
        {
          tid = 0;
        }
    }

  ForwardDown (packet, from, to, tid);
}

void 
DmgApWifiMac::ForwardDown (Ptr<const Packet> packet, Mac48Address from,
Mac48Address to, uint8_t tid)
{
  NS_LOG_FUNCTION (this << packet << from << to << static_cast<uint32_t> (tid));
  WifiMacHeader hdr;

  // For now, an AP that supports QoS does not support non-QoS
  // associations, and vice versa. In future the AP model should
  // support simultaneously associated QoS and non-QoS STAs, at which
  // point there will need to be per-association QoS state maintained
  // by the association state machine, and consulted here.
  hdr.SetType (WIFI_MAC_QOSDATA);
  hdr.SetQosAckPolicy (WifiMacHeader::NORMAL_ACK);
  hdr.SetQosNoEosp ();
  hdr.SetQosNoAmsdu ();
  // Transmission of multiple frames in the same TXOP is not
  // supported for now
  hdr.SetQosTxopLimit (0);
  // Fill in the QoS control field in the MAC header
  hdr.SetQosTid (tid);
  hdr.SetNoOrder ();

  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (from);
  hdr.SetDsFrom ();
  hdr.SetDsNotTo ();

  // Sanity check that the TID is valid
  NS_ASSERT (tid < 8);
  m_edca[QosUtilsMapTidToAc (tid)]->Queue (packet, hdr);
}

void DmgApWifiMac::Enqueue (Ptr<const Packet> packet, Mac48Address to, Mac48Address from)
{
  NS_LOG_FUNCTION (this << packet << to << from);
  if (to.IsBroadcast () || m_stationManager->IsAssociated (to))
    {
      ForwardDown (packet, from, to);
    }
}

void 
DmgApWifiMac::Enqueue (Ptr<const Packet> packet, Mac48Address to)
{
  NS_LOG_FUNCTION (this << packet << to);
  // We're sending this packet with a from address that is our own. We
  // get that address from the lower MAC and make use of the
  // from-spoofing Enqueue() method to avoid duplicated code.
  Enqueue (packet, to, m_low->GetAddress ());
}

bool 
DmgApWifiMac::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

void 
DmgApWifiMac::SendProbeResp (Mac48Address to)
{
  NS_LOG_FUNCTION (this << to);
  WifiMacHeader hdr;
  hdr.SetProbeResp ();
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  Ptr<Packet> packet = Create<Packet> ();
  MgtProbeResponseHeader probe;
  probe.SetSsid (GetSsid ());
  probe.SetBeaconIntervalUs (m_beaconInterval.GetMicroSeconds ());
  packet->AddHeader (probe);

  // The standard is not clear on the correct queue for management
  // frames if we are a QoS AP. The approach taken here is to always
  // use the DCF for these regardless of whether we have a QoS
  // association or not.
  m_dca->Queue (packet, hdr);
}

void
DmgApWifiMac::SendAssocResp (Mac48Address to, bool success)
{
  NS_LOG_FUNCTION (this << to << success);
  WifiMacHeader hdr;
  hdr.SetAssocResp ();
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  Ptr<Packet> packet = Create<Packet> ();
  MgtAssocResponseHeader assoc;
  StatusCode code;
  if (success)
    {
      code.SetSuccess ();
      m_aidCounter++;
      assoc.SetAid (m_aidCounter);
    }
  else
    {
      code.SetFailure ();
      assoc.SetAid (0);
    }

  assoc.SetStatusCode (code);
  assoc.AddWifiInformationElement (GetDmgCapabilities ());
  packet->AddHeader (assoc);

  /* For now, we assume one station that talks to the DMG AP */
  ANTENNA_CONFIGURATION_TX antennaConfigTx = m_bestAntennaConfig[to].first;
  ANTENNA_CONFIGURATION_TX antennaConfigRx = m_bestAntennaConfig[to].second;
  m_phy->GetDirectionalAntenna ()->SetCurrentTxSectorID (antennaConfigTx.first);
  m_phy->GetDirectionalAntenna ()->SetCurrentTxAntennaID (antennaConfigTx.second);
  if (antennaConfigRx.first != 0)
    {
      m_phy->GetDirectionalAntenna ()->SetCurrentRxSectorID (antennaConfigRx.first);
      m_phy->GetDirectionalAntenna ()->SetCurrentRxAntennaID (antennaConfigRx.second);
    }

  //The standard is not clear on the correct queue for management
  //frames if we are a QoS AP. The approach taken here is to always
  //use the DCF for these regardless of whether we have a QoS
  //association or not.
  m_dca->Queue (packet, hdr);
}

Ptr<DmgCapabilities>
DmgApWifiMac::GetDmgCapabilities (void) const
{
  Ptr<DmgCapabilities> capabilities = Create<DmgCapabilities> ();
  capabilities->SetStaAddress (GetAddress ()); /* STA MAC Adress*/
  capabilities->SetAID (0);

  /* DMG STA Capability Information Field */
  capabilities->SetReverseDirection (m_supportRdp);
  capabilities->SetHigherLayerTimerSynchronization (false);
  capabilities->SetNumberOfRxDmgAntennas (1);  /* Hardcoded Now */
  capabilities->SetNumberOfSectors (128);      /* Hardcoded Now */
  capabilities->SetRxssLength (128);           /* Hardcoded Now */
  capabilities->SetAmpduParameters (5, 0);     /* Hardcoded Now (Maximum A-MPDU + No restriction) */
  capabilities->SetSupportedMCS (12, 24, 12 ,24, false, true); /* LP SC is not supported yet */
  capabilities->SetAppduSupported (false);     /* Currently A-PPDU Agregatio is not supported*/

  /* DMG PCP/AP Capability Information Field */
  capabilities->SetTDDTI (true);
  capabilities->SetPseudoStaticAllocations (true);
  capabilities->SetMaxAssociatedStaNumber (10);
  capabilities->SetPowerSource (true); /* Not battery powered */
  capabilities->SetPcpForwarding (true);

  return capabilities;
}

Ptr<DmgOperationElement>
DmgApWifiMac::GetDmgOperationElement (void) const
{
  Ptr<DmgOperationElement> operation = Create<DmgOperationElement> ();
  /* DMG Operation Information */
  operation->SetTDDTI (true);
  operation->SetPseudoStaticAllocations (true);
  operation->SetPcpHandover (m_pcpHandoverSupport);
  /* DMG BSS Parameter Configuration */
  Time bhiDuration = m_btiDuration + m_abftDuration + m_atiDuration + 2 * GetMbifs ();
  operation->SetMinBHIDuration (static_cast<uint16_t> (bhiDuration.GetMicroSeconds ()));
  operation->SetMaxLostBeacons (10);
  return operation;
}

Ptr<NextDmgAti>
DmgApWifiMac::GetNextDmgAtiElement (void) const
{
  Ptr<NextDmgAti> ati = Create<NextDmgAti> ();
  Time atiStart = m_btiDuration + GetMbifs () + m_abftDuration;
  ati->SetStartTime (static_cast<uint32_t> (atiStart.GetMicroSeconds ()));
  ati->SetAtiDuration (static_cast<uint16_t> (m_atiDuration.GetMicroSeconds ()));  /* Microseconds*/
  return ati;
}

Ptr<ExtendedScheduleElement>
DmgApWifiMac::GetExtendedScheduleElement (void) const
{
  Ptr<ExtendedScheduleElement> scheduleElement = Create<ExtendedScheduleElement> ();
  scheduleElement->SetAllocationFieldList (m_allocationList);
  return scheduleElement;
}

void
DmgApWifiMac::CleanupAllocations (void)
{
  NS_LOG_FUNCTION (this);
  AllocationField allocation;
  for(AllocationFieldList::iterator iter = m_allocationList.begin (); iter != m_allocationList.end ();)
    {
      allocation = (*iter);
      if (!allocation.IsPseudoStatic ())
        {
          iter = m_allocationList.erase (iter);
        }
      else
        {
          ++iter;
        }
    }
}

void
DmgApWifiMac::AddAllocationPeriod (AllocationType allocationType, bool staticAllocation,
                                   uint8_t sourceAid, uint8_t destAid,
                                   uint32_t allocationStart, uint16_t blockDuration)
{
  NS_LOG_FUNCTION (this);
  AllocationField field;
  if ((allocationType == CBAP_ALLOCATION) && (sourceAid == 255) && (destAid == 255))
    {
      field.SetAllocationID (0);
    }
  else
    {
      field.SetAllocationID (++m_allocationID);
    }
  field.SetAllocationType (allocationType);
  field.SetAsPseudoStatic (staticAllocation);
  field.SetSourceAid (sourceAid);
  field.SetDestinationAid (destAid);
  field.SetAllocationStart (allocationStart);
  field.SetAllocationBlockDuration (blockDuration);
  field.SetNumberOfBlocks (1);
  /**
   * When scheduling two adjacent SPs, the PCP/AP should allocate the SPs separated by at least
   * aDMGPPMinListeningTime if one or more of the source or destination DMG STAs participate in both SPs.
   */
  m_allocationList.push_back (field);
}

void
DmgApWifiMac::AllocateBeamformingServicePeriod (uint8_t sourceAid, uint8_t destAid,
                                                uint32_t allocationStart, bool isTxss)
{
  NS_LOG_FUNCTION (this << sourceAid << destAid << allocationStart << isTxss);
  AllocationField field;
  field.SetAllocationType (SERVICE_PERIOD_ALLOCATION);
  field.SetAsPseudoStatic (false);
  field.SetSourceAid (sourceAid);
  field.SetDestinationAid (destAid);
  field.SetAllocationStart (allocationStart);
  field.SetAllocationBlockDuration (400);     // Microseconds
  field.SetNumberOfBlocks (1);

  BF_Control_Field bfField;
  bfField.SetBeamformTraining (true);
  bfField.SetAsInitiatorTxss (isTxss);
  bfField.SetAsResponderTxss (isTxss);
  bfField.SetRxssLength (m_phy->GetDirectionalAntenna ()->GetNumberOfSectors ());

  field.SetBfControl (bfField);
  m_allocationList.push_back (field);
}

void
DmgApWifiMac::SendOneDMGBeacon (uint8_t sectorID, uint8_t antennaID, uint16_t count)
{
  NS_LOG_FUNCTION (this);
  WifiMacHeader hdr;
  hdr.SetDMGBeacon ();                /* Set frame type to DMG beacon i.e. Change Frame Control Format. */
  hdr.SetAddr1 (GetBssid ());         /* BSSID */
  hdr.SetNoMoreFragments ();
  hdr.SetNoRetry ();

  ExtDMGBeacon beacon;

  /* Timestamp */
  m_btiRemaining = GetBTIRemainingTime ();
  m_beaconTransmitted = Simulator::Now ();

  /* Sector Sweep Field */
  DMG_SSW_Field ssw;
  ssw.SetDirection (BeamformingInitiator);
  ssw.SetCountDown (count);
  ssw.SetSectorID (sectorID);
  ssw.SetDMGAntennaID (antennaID);
  beacon.SetSSWField (ssw);

  /* Beacon Interval */
  beacon.SetBeaconIntervalUs (m_beaconInterval.GetMicroSeconds ());

  /* Beacon Interval Control Field */
  ExtDMGBeaconIntervalCtrlField ctrl;
  ctrl.SetCCPresent (false);              /* Cluster control is not supported now*/
  ctrl.SetDiscoveryMode (false);
  ctrl.SetNextBeacon (0);
  /* Signal the presence of an ATI interval */
  m_isCbapOnly = (m_allocationList.size () == 0);
  if (m_isCbapOnly)
    {
      /* For CBAP DTI, the ATI is not present */
      ctrl.SetATIPresent (true);
    }
  else
    {
      /* Todo: depends on user's configuration */
      ctrl.SetATIPresent (m_atiPresent);
    }
  ctrl.SetABFT_Length (m_ssSlotsPerABFT); /* Length of the following A-BFT*/
  ctrl.SetFSS (m_ssFramesPerSlot);
  ctrl.SetIsResponderTXSS (m_isResponderTXSS);
  ctrl.SetNextABFT (m_nextAbft);
  ctrl.SetFragmentedTXSS (false);         /* To fragment the initiator TXSS over multiple consecutive BTIs, do not support */
  ctrl.SetTXSS_Span (1);                  /* Currently, we finish TXSS Phase in one BI */
  ctrl.SetN_BI (1);
  ctrl.SetABFT_Count (10);
  ctrl.SetN_ABFT_Ant (0);
  ctrl.SetPCPAssoicationReady (false);
  beacon.SetBeaconIntervalControlField (ctrl);

  /* DMG Parameters*/
  ExtDMGParameters parameters;
  parameters.Set_BSS_Type (InfrastructureBSS);  // An AP sets the BSS Type subfield to 3 within transmitted DMG Beacon,
  parameters.Set_CBAP_Only (m_isCbapOnly);
  parameters.Set_CBAP_Source (m_isCbapSource);
  parameters.Set_DMG_Privacy (false);
  parameters.Set_ECPAC_Policy_Enforced (false);
  beacon.SetDMGParameters (parameters);

  /* Cluster Control Field */
  if (ctrl.IsCCPresent ())
    {
      ExtDMGClusteringControlField cluster;
      cluster.SetDiscoveryMode (ctrl.IsDiscoveryMode ());
      beacon.SetClusterControlField (cluster);
    }

  /* DMG Capabilities Information Element */
  beacon.AddWifiInformationElement (GetDmgCapabilities ());
  /* DMG Operation Element */
  beacon.AddWifiInformationElement (GetDmgOperationElement ());
  /* Next DMG ATI Information Element */
  beacon.AddWifiInformationElement (GetNextDmgAtiElement ());
  /* Multi-band Information Element */
  beacon.AddWifiInformationElement (GetMultiBandElement ());
  /* Add Relay Capability Element */
  beacon.AddWifiInformationElement (GetRelayCapabilities ());
  /* Extended Schedule Element */
  beacon.AddWifiInformationElement (GetExtendedScheduleElement ());

  /* Set Antenna Sector in the PHY Layer */
  m_phy->GetDirectionalAntenna ()->SetCurrentTxSectorID (sectorID);
  m_phy->GetDirectionalAntenna ()->SetCurrentTxAntennaID (antennaID);

  /* The DMG beacon has it's own special queue, so we load it in there */
  m_beaconDca->TransmitDmgBeacon (beacon, hdr);
}

Time
DmgApWifiMac::GetBTIRemainingTime (void) const
{
  return m_btiRemaining - (Simulator::Now () - m_beaconTransmitted);
}

void
DmgApWifiMac::FrameTxOk (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this);

  if (hdr.IsDMGBeacon ())
    {
      m_btiRemaining = GetBTIRemainingTime ();
      m_beaconTransmitted = Simulator::Now ();
      /* Check whether we start a new access phase or schedule new DMG Beacon */
      if (m_totalSectors == 0)
        {
          if (m_nextAbft != 0)
            {
              /* Following the end of a BTI, the PCP/AP shall decrement the value of the Next A-BFT field by one provided
               * it is not equal to zero and shall announce this value in the next BTI.*/
              m_nextAbft--;

              Simulator::Schedule (m_btiRemaining + m_mbifs, &DmgApWifiMac::StartAnnouncementTransmissionInterval, this);
            }
          else
            {
              /* The PCP/AP may increase the Next A-BFT field value following
               *  a BTI in which the Next A-BFT field was equal to zero. */
              m_nextAbft = m_abftPeriodicity;

              /* The PCP/AP shall allocate an A-BFT period MBIFS time following the end of a BTI that
               * included a DMG Beacon frame transmission with Next A-BFT equal to 0.*/
              Simulator::Schedule (m_btiRemaining + m_mbifs, &DmgApWifiMac::StartAssociationBeamformTraining, this);

              /* Check the type of RSS in A-BFT */
              if (m_isResponderTXSS)
                {
                  /* Set the antenna as Quasi-omni receiving antenna */
                  m_phy->GetDirectionalAntenna ()->SetInOmniReceivingMode ();
                }
              else
                {
                  /* Set the antenna as directional receiving antenna */
                  m_phy->GetDirectionalAntenna ()->SetInDirectionalReceivingMode ();
                }
            }

          /* Cleanup non-static allocations */
          CleanupAllocations ();
        }
      else
        {
          if (m_sectorId < m_phy->GetDirectionalAntenna ()->GetNumberOfSectors ())
            {
              m_sectorId++;
            }
          else if ((m_sectorId == m_phy->GetDirectionalAntenna ()->GetNumberOfSectors ()) &&
                   (m_antennaId < m_phy->GetDirectionalAntenna ()->GetNumberOfAntennas ()))
            {
              m_sectorId = 1;
              m_antennaId++;
            }

          m_totalSectors--;
          NS_LOG_INFO ("Sending DMG Beacon " << Simulator::Now () << " with "
                       << uint32_t (m_sectorId) << " " << uint32_t (m_antennaId));
          if (m_sectorId == 1)
            {
              /* The PCP/AP shall not change DMG Antennas within a BTI */
              /* LBIFS is switching DMG Antenna */
              m_beaconEvent = Simulator::Schedule (m_lbifs, &DmgApWifiMac::SendOneDMGBeacon, this,
                                                   m_sectorId, m_antennaId, m_totalSectors);
            }
          else
            {
              /* SBIFS is switching Sector */
              m_beaconEvent = Simulator::Schedule (m_sbifs, &DmgApWifiMac::SendOneDMGBeacon, this,
                                                   m_sectorId, m_antennaId, m_totalSectors);
            }
        }
    }
}

void
DmgApWifiMac::StartBeaconTransmissionInterval (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("DMG AP Starting BTI at " << Simulator::Now ());
  /* Re-initialize variables */
  m_sectorFeedbackSent.clear ();

  /* Disable Channel Access by EDCAs */
  m_dcfManager->DisableChannelAccess ();
  m_sectorFeedbackSent.clear ();

  /* Start DMG Beaconing */
  m_sectorId = 1;
  m_antennaId = 1;
  m_totalSectors = m_phy->GetDirectionalAntenna ()->GetNumberOfSectors () * m_phy->GetDirectionalAntenna ()->GetNumberOfAntennas () - 1;

  /* Timing variables */
  m_btiStarted = Simulator::Now ();
  m_beaconTransmitted = Simulator::Now ();
  m_btiRemaining = m_btiDuration;
  NS_LOG_INFO ("Sending DMG Beacon " << Simulator::Now () << " with " <<
               uint32_t (m_sectorId) << " " << uint32_t (m_antennaId));

  m_beaconEvent = Simulator::ScheduleNow (&DmgApWifiMac::SendOneDMGBeacon, this,
                                          m_antennaId, m_sectorId, m_totalSectors);
}

void
DmgApWifiMac::StartAssociationBeamformTraining (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("DMG AP Starting A-BFT at " << Simulator::Now ());
  /* Schedule next period */
  if (m_atiPresent)
    {
      Simulator::Schedule (m_abftDuration, &DmgApWifiMac::StartAnnouncementTransmissionInterval, this);
    }
  else
    {
      Simulator::Schedule (m_abftDuration, &DmgApWifiMac::StartDataTransmissionInterval, this);
    }
  /* Schedule the beginning of the first A-BFT Slot */
  m_remainingSlots = m_ssSlotsPerABFT;
  Simulator::ScheduleNow (&DmgApWifiMac::StartSectorSweepSlot, this);
}

void
DmgApWifiMac::StartSectorSweepSlot (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("DMG AP Starting A-BFT SSW Slot [" << m_ssFramesPerSlot - m_remainingSlots << "] at " << Simulator::Now ());
  m_receivedOneSSW = false;
  m_remainingSlots--;
  /* Schedule the beginning of the next A-BFT Slot */
  if (m_remainingSlots > 0)
    {
      Simulator::Schedule (NanoSeconds (m_low->GetSectorSweepSlotTime (m_ssFramesPerSlot)),
                           &DmgApWifiMac::StartSectorSweepSlot, this);
    }
}

/**
 * During the ATI STAs shall not transmit frames that are not request or response frames.
 * Request and response frames transmitted during the ATI shall be one of the following:
 * 1. A frame of type Management
 * 2. An ACK frame
 * 3. A Grant, Poll, RTS or DMG CTS frame when transmitted as a request frame
 * 4. An SPR or DMG CTS frame when transmitted as a response frame
 * 5. A frame of type Data only as part of an authentication exchange to reach a RSNA security association
 * 6. The Announce frame is designed to be used primarily during the ATI and can perform functions of a DMG Beacon frame.
 */
void
DmgApWifiMac::StartAnnouncementTransmissionInterval (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("DMG AP Starting ATI at " << Simulator::Now ());
  /* Schedule DTI Period Starting Time */
  Simulator::Schedule (m_atiDuration, &DmgApWifiMac::StartDataTransmissionInterval, this);
  /* Initiate BRP Setup Subphase, currently ATI is used for BRP Setup + Training */
  m_dmgAtiDca->InitiateTransmission (m_atiDuration);
  DoBrpSetupSubphase ();
}

void
DmgApWifiMac::BrpSetupCompleted (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  /* Initiate BRP Transaction */
  InitiateBrpTransaction (address);
}

void
DmgApWifiMac::DoBrpSetupSubphase (void)
{
  NS_LOG_FUNCTION (this);
  for (STATION_BRP_MAP::iterator iter = m_stationBrpMap.begin (); iter != m_stationBrpMap.end (); iter++)
    {
      if (iter->second == true)
        {
          /* Request for receive beam training with each sation */
          InitiateBrpSetupSubphase (iter->first);
          iter->second = false;
          return;
        }
    }
}

void
DmgApWifiMac::NotifyBrpPhaseCompleted (void)
{
  NS_LOG_FUNCTION (this);
  DoBrpSetupSubphase ();
}

void
DmgApWifiMac::StartDataTransmissionInterval (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Starting DTI at " << Simulator::Now ());
//  m_phy->GetDirectionalAntenna ()->SetInOmniReceivingMode ();

  /* Schedule the beginning of next BHI interval */
  Time nextBeaconInterval = m_beaconInterval - (Simulator::Now () - m_btiStarted);
  Simulator::Schedule (nextBeaconInterval, &DmgApWifiMac::StartBeaconTransmissionInterval, this);
  NS_LOG_DEBUG ("Next Beacon Interval will start at " << Simulator::Now () + nextBeaconInterval);

  /* Start CBAPs and SPs */
  if (m_isCbapOnly)
    {
      NS_LOG_INFO ("CBAP allocation only in DTI");
      Simulator::ScheduleNow (&DmgApWifiMac::StartContentionPeriod, this, nextBeaconInterval);
    }
  else
    {
//      for (AllocationFieldList::iterator iter = m_allocationList.begin (); iter != m_allocationList.end (); iter++)
//        {
//          if ((iter->GetAllocationType () == AllocationType.SERVICE_PERIOD_ALLOCATION) && (iter->GetSourceAid () == m_aid))
//            {
//              Simulator::Schedule (i->GetAllocationStart(), &m_sp->DoNotifyAccessGranted (), this);
//            }
//          else if ((i->GetAllocationType() == AllocationType.CBAP_ALLOCATION) && (i->GetSourceAid() == m_aid))
//            {
//              Simulator::Schedule (i->GetAllocationStart(), &StartContentionPeriod (), this);
//            }
//        }
    }
}

/* Dynamic Allocattion of service periods */
void
DmgApWifiMac::StartPollingPhase (void)
{
  NS_LOG_FUNCTION (this);
  for (AssociatedStationsInfoByAddress::const_iterator iter = m_associatedStationsInfoByAddress.begin ();
       iter != m_associatedStationsInfoByAddress.end (); iter++)
    {
      SendPollFrame (iter->first);
    }
}

/**
 * Service Period (SP) Functions
 */
void
DmgApWifiMac::SendPollFrame (Mac48Address to)
{
  NS_LOG_FUNCTION (this << to);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_CTL_DMG_POLL);
  hdr.SetAddr1 (to);            //RA Field (MAC Address of the STA being polled)
  hdr.SetAddr2 (GetAddress ()); //TA Field (MAC Address of the PCP or AP)
  hdr.SetDsNotFrom ();
  hdr.SetDsTo ();
  hdr.SetNoOrder ();

  Ptr<Packet> packet = Create<Packet> ();
  CtrlDmgPoll poll;
  poll.SetResponseOffset (0);
  packet->AddHeader (poll);

  m_dmgAtiDca->Queue (packet, hdr);
}

void
DmgApWifiMac::SendGrantFrame (Mac48Address to)
{
  NS_LOG_FUNCTION (this << to);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_CTL_DMG_GRANT);
  hdr.SetAddr1 (to);            //RA Field (MAC Address of the STA receiving SP Grant)
  hdr.SetAddr2 (GetAddress ()); //TA Field (MAC Address of the STA that has transmited the Grant frame)
  hdr.SetDsNotFrom ();
  hdr.SetDsTo ();
  hdr.SetNoOrder ();

  Ptr<Packet> packet = Create<Packet> ();
  CtrlDMG_Grant grant;
  packet->AddHeader (grant);

  m_dmgAtiDca->Queue (packet, hdr);
}

/**
 * Announce Frame
 */
void
DmgApWifiMac::SendAnnounceFrame (Mac48Address to)
{
  NS_LOG_FUNCTION (this << to);
  WifiMacHeader hdr;
  hdr.SetActionNoAck ();
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  ExtAnnounceFrame announceHdr;
  announceHdr.SetBeaconInterval (m_beaconInterval.GetInteger ());

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.unprotectedAction = WifiActionHeader::UNPROTECTED_DMG_ANNOUNCE;
  actionHdr.SetAction (WifiActionHeader::UNPROTECTED_DMG, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (announceHdr);
  packet->AddHeader (actionHdr);

  m_dmgAtiDca->Queue (packet, hdr);
}

void 
DmgApWifiMac::TxOk (Ptr<const Packet> packet, const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this);
  DmgWifiMac::TxOk (packet, hdr);
  /* For asociation */
  if (hdr.IsAssocResp() && m_stationManager->IsWaitAssocTxOk (hdr.GetAddr1 ()))
    {
      NS_LOG_DEBUG ("associated with sta=" << hdr.GetAddr1 ());
      m_stationManager->RecordGotAssocTxOk (hdr.GetAddr1 ());
    }
}

void 
DmgApWifiMac::TxFailed (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this);
  RegularWifiMac::TxFailed (hdr);

  if (hdr.IsAssocResp () && m_stationManager->IsWaitAssocTxOk (hdr.GetAddr1 ()))
    {
      NS_LOG_DEBUG ("assoc failed with sta=" << hdr.GetAddr1 ());
      m_stationManager->RecordGotAssocTxFailed (hdr.GetAddr1 ());
    }
}

Ptr<MultiBandElement>
DmgApWifiMac::GetMultiBandElement (void) const
{
  Ptr<MultiBandElement> multiband = Create<MultiBandElement> ();
  multiband->SetStaRole (ROLE_AP);
  multiband->SetStaMacAddressPresent (false); /* The same MAC address is used across all the bands */
  multiband->SetBandID (Band_4_9GHz);
  multiband->SetOperatingClass (18);          /* Europe */
  multiband->SetChannelNumber (1);
  multiband->SetBssID (GetAddress ());
  multiband->SetConnectionCapability (1);     /* AP */
  multiband->SetFstSessionTimeout (1);
  return multiband;
}

void 
DmgApWifiMac::Receive (Ptr<Packet> packet, const WifiMacHeader *hdr)
{
  NS_LOG_FUNCTION(this << packet << hdr);
  Mac48Address from = hdr->GetAddr2 ();

  if (hdr->IsData ())
    {
      Mac48Address bssid = hdr->GetAddr1 ();
      if (!hdr->IsFromDs ()
          && hdr->IsToDs ()
          && bssid == GetAddress ()
          && m_stationManager->IsAssociated (from))
        {
          Mac48Address to = hdr->GetAddr3 ();
          if (to == GetAddress ())
            {
              NS_LOG_DEBUG ("frame for me from=" << from);
              if (hdr->IsQosData())
                {
                  if (hdr->IsQosAmsdu())
                    {
                      NS_LOG_DEBUG ("Received A-MSDU from=" << from << ", size=" << packet->GetSize ());
                      DeaggregateAmsduAndForward (packet, hdr);
                      packet = 0;
                    }
                  else
                    {
                      ForwardUp(packet, from, bssid);
                    }
                }
              else
                {
                  ForwardUp(packet, from, bssid);
                }
            }
          else if (to.IsGroup() || m_stationManager->IsAssociated(to))
            {
              NS_LOG_DEBUG ("forwarding frame from=" << from << ", to=" << to);
              Ptr<Packet> copy = packet->Copy ();

              // If the frame we are forwarding is of type QoS Data,
              // then we need to preserve the UP in the QoS control
              // header...
              if (hdr->IsQosData ())
                {
                  ForwardDown (packet, from, to, hdr->GetQosTid ());
                }
              else
                {
                  ForwardDown (packet, from, to);
                }
              ForwardUp (copy, from, to);
            }
          else
            {
              ForwardUp (packet, from, to);
            }
        }
      else if (hdr->IsFromDs() && hdr->IsToDs())
        {
          // this is an AP-to-AP frame
          // we ignore for now.
          NotifyRxDrop(packet);
        }
      else
        {
          // we can ignore these frames since
          // they are not targeted at the AP
          NotifyRxDrop (packet);
        }
      return;
    }
  else if (hdr->IsSSW ())
    {
      NS_LOG_INFO ("Received SSW frame from=" << hdr->GetAddr2 ());

      if (!m_receivedOneSSW)
        {
          m_receivedOneSSW = true;
          m_peerAbftStation = hdr->GetAddr2 ();
        }

      if (m_receivedOneSSW && m_peerAbftStation == hdr->GetAddr2 ())
        {
          CtrlDMG_SSW sswFrame;
          packet->RemoveHeader (sswFrame);

          DMG_SSW_Field ssw = sswFrame.GetSswField ();
          /* Map the antenna configuration for the frames received by SLS of the DMG-STA */
          MapTxSnr (hdr->GetAddr2 (), ssw.GetSectorID (), ssw.GetDMGAntennaID (), m_stationManager->GetRxSnr ());

          /* If we receive one SSW Frame at least, then we schedule SSW-FBCK */
          if (!m_sectorFeedbackSent[hdr->GetAddr2 ()])
            {
              m_sectorFeedbackSent[hdr->GetAddr2 ()] = true;

              /* Set the best TX antenna configuration reported by the SSW-FBCK Field */
              DMG_SSW_FBCK_Field sswFeedback = sswFrame.GetSswFeedbackField ();
              sswFeedback.IsPartOfISS (false);

              /* The Sector Sweep Frame contains feedback about the the best Tx Sector in the DMG-AP with the sending DMG-STA */
              ANTENNA_CONFIGURATION_TX antennaConfigTx = std::make_pair (sswFeedback.GetSector (), sswFeedback.GetDMGAntenna ());
              ANTENNA_CONFIGURATION_RX antennaConfigRx = std::make_pair (0, 0);
              m_bestAntennaConfig[hdr->GetAddr2 ()] = std::make_pair (antennaConfigTx, antennaConfigRx);

              NS_LOG_INFO ("Best TX Antenna Sector Config by this DMG AP to DMG STA=" << hdr->GetAddr2 ()
                           << ": SectorID=" << uint32_t (antennaConfigTx.first)
                           << ", AntennaID=" << uint32_t (antennaConfigTx.second));

              /* Raise an event that we selected the best sector to the DMG STA */
              m_slsCompleted (hdr->GetAddr2 (), CHANNEL_ACCESS_BHI, antennaConfigTx.first, antennaConfigTx.second);

              /* Indicate this DMG-STA as waiting for Beam Refinement Phase */
              m_stationBrpMap[hdr->GetAddr2 ()] = true;

              Time sswFbckTime = m_low->GetSectorSweepDuration (ssw.GetCountDown ()) + m_mbifs;
              NS_LOG_INFO ("Scheduled SSW-FBCK Frame to " << hdr->GetAddr2 () << " at " << Simulator::Now () + sswFbckTime);
              Simulator::Schedule (sswFbckTime, &DmgApWifiMac::SendSswFbckAfterRss, this, hdr->GetAddr2 ());
            }
        }

      return;
    }
  else if (hdr->IsSprFrame ())
    {
      NS_LOG_INFO ("Received SPR frame from=" << hdr->GetAddr2 ());

    }
  else if (hdr->IsMgt ())
    {
      if (hdr->IsProbeReq ())
        {
          NS_ASSERT(hdr->GetAddr1 ().IsBroadcast ());
          SendProbeResp (from);
          return;
        }
      else if (hdr->GetAddr1 () == GetAddress())
        {
          if (hdr->IsAssocReq ())
            {
              // First, verify that the the station's supported
              // rate set is compatible with our Basic Rate set
              MgtAssocRequestHeader assocReq;
              packet->RemoveHeader (assocReq);
              bool problem = false;
              if (m_dmgSupported)
                {
                  //check that the DMG STA supports all MCSs in Basic MCS Set
                }
              if (problem)
                {
                  //One of the Basic Rate set mode is not
                  //supported by the station. So, we return an assoc
                  //response with an error status.
                  SendAssocResp (hdr->GetAddr2 (), false);
                }
              else
                {
                  m_stationManager->RecordWaitAssocTxOk (from);

                  /* Send assocication response with success status. */
                  SendAssocResp (hdr->GetAddr2 (), true);

                  /* Record DMG STA Information */
                  WifiInformationElementMap infoMap = assocReq.GetListOfInformationElement ();
                  /* Change AID of the station */
                  Ptr<DmgCapabilities> capabilities = StaticCast<DmgCapabilities> (infoMap[IE_DMG_CAPABILITIES]);
                  capabilities->SetAID (m_aidCounter & 0xFF);

                  m_associatedStationsInfoByAddress[hdr->GetAddr2 ()] = infoMap;
                  m_associatedStationsInfoByAid[m_aidCounter] = infoMap;

                  /* Check Relay Capabilities */
                  Ptr<RelayCapabilitiesElement> relayElement =
                      DynamicCast<RelayCapabilitiesElement> (assocReq.GetInformationElement (IE_RELAY_CAPABILITIES));
                  /* Check if the STA supports RDS */
                  if (relayElement->GetRelayCapabilitiesInfo ().GetRelaySupportability ())
                    {
                      ExtRelayCapableStaInfo *info = new ExtRelayCapableStaInfo;
                      info->SetStaAid (m_aidCounter);
                      m_rdsList.push_back (info);
                      NS_LOG_DEBUG ("Station=" << hdr->GetAddr2 () << " with AID=" << m_aidCounter
                                    << " supports RDS operation");
                    }
                }
              return;
            }
          else if (hdr->IsDisassociation ())
            {
              m_stationManager->RecordDisassociated (from);
              return;
            }
          /* Received Action Frame */
          else if (hdr->IsAction ())
            {
              WifiActionHeader actionHdr;
              packet->RemoveHeader (actionHdr);
              switch (actionHdr.GetCategory ())
                {
                case WifiActionHeader::DMG:
                  switch (actionHdr.GetAction ().dmgAction)
                    {
                    case WifiActionHeader::DMG_RELAY_SEARCH_REQUEST:
                      {
                        ExtRelaySearchRequestHeader requestHdr;
                        packet->RemoveHeader (requestHdr);
                        /* Received Relay Search Request, reply back with the list of RDS */
                        SendRelaySearchResponse (hdr->GetAddr2 (), requestHdr.GetDialogToken ());

                        /* Send Unsolicited Relay Search Response to the destination */
                        Ptr<DmgCapabilities> dmgCapabilities = StaticCast<DmgCapabilities>
                            (m_associatedStationsInfoByAid[requestHdr.GetDestinationRedsAid ()][IE_DMG_CAPABILITIES]);
                        SendRelaySearchResponse (dmgCapabilities->GetStaAddress (), requestHdr.GetDialogToken ());

                         /* Get Source REDS DMG Capabilities */
                        Ptr<DmgCapabilities> srcDmgCapabilities = StaticCast<DmgCapabilities>
                            (m_associatedStationsInfoByAddress[hdr->GetAddr2 ()][IE_DMG_CAPABILITIES]);

                        /* The PCP/AP should schedule two SPs for each RDS in the response */
                        Ptr<ExtRelayCapableStaInfo> relayInfo;
                        for (RelayCapableStaList::const_iterator iter = m_rdsList.begin (); iter != m_rdsList.end (); iter++)
                          {
                            relayInfo = StaticCast<ExtRelayCapableStaInfo> (*iter);

                            /* First SP between the source REDS and the RDS */
                            AllocateBeamformingServicePeriod (srcDmgCapabilities->GetAID (),
                                                              relayInfo->GetStaAid (), 0, true);
                            /* Second SP between the source REDS and the Destination REDS */
                            AllocateBeamformingServicePeriod (relayInfo->GetStaAid (),
                                                              requestHdr.GetDestinationRedsAid (), 500, true);
                          }

                        return;
                      }
                    case WifiActionHeader::DMG_RLS_ANNOUNCEMENT:
                      {
                        ExtRlsAnnouncment announcementHdr;
                        packet->RemoveHeader (announcementHdr);
                        NS_LOG_INFO ("A relay Link is established between: " <<
                                     "Source REDS AID=" << announcementHdr.GetSourceAid () <<
                                     ", RDS AID=" << announcementHdr.GetRelayAid () <<
                                     ", Destination REDS AID=" << announcementHdr.GetDestinationAid ());
                        return;
                      }
                    case WifiActionHeader::DMG_INFORMATION_REQUEST:
                      {
                        ExtInformationRequest requestHdr;
                        packet->RemoveHeader (requestHdr);

                        Mac48Address subjectAddress = requestHdr.GetSubjectAddress ();
                        ExtInformationResponse responseHdr;
                        responseHdr.SetSubjectAddress (subjectAddress);
                        /* Obtain Subject Station DMG Capabilities */
                        Ptr<DmgCapabilities> dmgCapabilities =
                            StaticCast<DmgCapabilities> (m_associatedStationsInfoByAddress[subjectAddress][IE_DMG_CAPABILITIES]);
                        responseHdr.AddDmgCapabilitiesElement (dmgCapabilities);
                        /* Request Element */
                        responseHdr.SetRequestInformationElement (requestHdr.GetRequestInformationElement ());

                        SendInformationRsponse (hdr->GetAddr2 (), responseHdr);

                        /* Check request Elements */
                        return;
                      }
                    default:
                      packet->AddHeader (actionHdr);
                      DmgWifiMac::Receive (packet, hdr);
                      return;
                    }

                default:
                  packet->AddHeader (actionHdr);
                  DmgWifiMac::Receive (packet, hdr);
                  return;
                }
            }
          else if (hdr->IsActionNoAck ())
            {
              DmgWifiMac::Receive (packet, hdr);
              return;
            }
        }
      return;
    }
  DmgWifiMac::Receive (packet, hdr);
}

void 
DmgApWifiMac::DeaggregateAmsduAndForward (Ptr<Packet> aggregatedPacket, const WifiMacHeader *hdr)
{
  NS_LOG_FUNCTION (this << aggregatedPacket << hdr);
  MsduAggregator::DeaggregatedMsdus packets =
  MsduAggregator::Deaggregate (aggregatedPacket);

  for (MsduAggregator::DeaggregatedMsdusCI i = packets.begin (); i != packets.end (); ++i)
  {
    if ((*i).second.GetDestinationAddr () == GetAddress ())
      {
        ForwardUp ((*i).first, (*i).second.GetSourceAddr (), (*i).second.GetDestinationAddr ());
      }
    else
      {
        Mac48Address from = (*i).second.GetSourceAddr ();
        Mac48Address to = (*i).second.GetDestinationAddr ();
        NS_LOG_DEBUG ("forwarding QoS frame from=" << from << ", to=" << to);
        ForwardDown ((*i).first, from, to, hdr->GetQosTid ());
      }
  }
}

void 
DmgApWifiMac::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  m_beaconDca->Initialize ();
  m_beaconEvent.Cancel ();

  /* Calculate A-BFT Duration (Constant during the entire simulation) */
  m_abftDuration = NanoSeconds (m_ssSlotsPerABFT * m_low->GetSectorSweepSlotTime (m_ssFramesPerSlot));
  m_abftDuration = MicroSeconds (ceil ((double) m_abftDuration.GetNanoSeconds () / 1000));

  NS_LOG_DEBUG ("Starting DMG Access Point " << GetAddress () << " at time " << Simulator::Now ());
  StartBeaconTransmissionInterval ();

  DmgWifiMac::DoInitialize ();
}

} // namespace ns3
