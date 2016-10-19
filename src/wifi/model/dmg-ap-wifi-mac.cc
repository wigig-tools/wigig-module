/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
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
#include "wifi-phy.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DmgApWifiMac");

NS_OBJECT_ENSURE_REGISTERED (DmgApWifiMac);

TypeId
DmgApWifiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DmgApWifiMac")
    .SetParent<DmgWifiMac> ()
    .SetGroupName ("Wifi")
    .AddConstructor<DmgApWifiMac> ()

    /* DMG Beacon Control Interval */
    .AddAttribute ("BeaconInterval", "The interval between two Target Beacon Transmission Times (TBTTs).",
                   TimeValue (MicroSeconds (aMaxBIDuration)),
                   MakeTimeAccessor (&DmgApWifiMac::GetBeaconInterval,
                                     &DmgApWifiMac::SetBeaconInterval),
                   MakeTimeChecker (MicroSeconds (TU), MicroSeconds (aMaxBIDuration)))
    .AddAttribute ("EnableBeaconRandomization",
                   "Whether the DMG AP shall change the sequence of directions through which a DMG Beacon frame"
                   "is transmitted after it has transmitted a DMG Beacon frame through each direction in the"
                   "current sequence of directions.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&DmgApWifiMac::m_beaconRandomization),
                   MakeBooleanChecker ())
    .AddAttribute ("NextBeacon", "The number of beacon intervals following the current beacon interval during"
                   "which the DMG Beacon is not be present.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&DmgApWifiMac::m_nextBeacon),
                   MakeUintegerChecker<uint8_t> (0, 15))
    .AddAttribute ("BeaconTransmissionInterval", "The duration of the BTI period.",
                   TimeValue (MicroSeconds (400)),
                   MakeTimeAccessor (&DmgApWifiMac::GetBeaconTransmissionInterval,
                                     &DmgApWifiMac::SetBeaconTransmissionInterval),
                   MakeTimeChecker ())
    .AddAttribute ("NextABFT", "The number of beacon intervals during which the A-BFT is not be present.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&DmgApWifiMac::m_abftPeriodicity),
                   MakeUintegerChecker<uint8_t> (0, 15))
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
    .AddAttribute ("ATIPresent", "The BI period contains ATI access period.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&DmgApWifiMac::m_atiPresent),
                   MakeBooleanChecker ())
    .AddAttribute ("ATIDuration", "The duration of the ATI Period.",
                    TimeValue (MicroSeconds (500)),
                    MakeTimeAccessor (&DmgApWifiMac::m_atiDuration),
                    MakeTimeChecker ())

    /* DMG Parameters */
    .AddAttribute ("CBAPSource", "Indicates that PCP/AP has a higher priority for transmission in CBAP",
                   BooleanValue (false),
                   MakeBooleanAccessor (&DmgApWifiMac::m_isCbapSource),
                   MakeBooleanChecker ())

      .AddTraceSource ("BIStarted", "A new Beacon Interval has started.",
                       MakeTraceSourceAccessor (&DmgApWifiMac::m_biStarted),
                       "ns3::Mac48Address::TracedCallback")
      .AddTraceSource ("DTIStarted", "The Data Transmission Interval access period started.",
                       MakeTraceSourceAccessor (&DmgApWifiMac::m_dtiStarted),
                       "ns3::DmgApWifiMac::DtiStartedTracedCallback")
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
  m_receivedOneSSW = false;
  m_aidCounter = 0;
  m_btiPeriodicity = 0;
  m_nextAbft = m_abftPeriodicity;

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

  hdr.SetType (WIFI_MAC_QOSDATA);
  hdr.SetQosAckPolicy (WifiMacHeader::NORMAL_ACK);
  hdr.SetQosNoEosp ();
  hdr.SetQosNoAmsdu ();
  hdr.SetQosTxopLimit (0);
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
      m_aidCounter++;
      code.SetSuccess ();
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
  SteerAntennaToward (to);
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
  capabilities->SetMaxAssociatedStaNumber (254);
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

uint32_t
DmgApWifiMac::AllocateCbapPeriod (bool staticAllocation,
                                  uint32_t allocationStart, uint16_t blockDuration)
{
  NS_LOG_FUNCTION (this << staticAllocation << allocationStart << blockDuration);
  AddAllocationPeriod (0, CBAP_ALLOCATION, staticAllocation, AID_BROADCAST, AID_BROADCAST, allocationStart, blockDuration);
  return (allocationStart + blockDuration);
}

uint32_t
DmgApWifiMac::AddAllocationPeriod (AllocationID allocationID,
                                   AllocationType allocationType,
                                   bool staticAllocation,
                                   uint8_t sourceAid, uint8_t destAid,
                                   uint32_t allocationStart, uint16_t blockDuration)
{
  NS_LOG_FUNCTION (this << allocationID << allocationType << staticAllocation << sourceAid << destAid);
  AllocationField field;
  /* Allocation Control Field */
  field.SetAllocationID (allocationID);
  field.SetAllocationType (allocationType);
  field.SetAsPseudoStatic (staticAllocation);
  /* Allocation Field */
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

  return (allocationStart + blockDuration);
}

uint32_t
DmgApWifiMac::AllocateBeamformingServicePeriod (uint8_t sourceAid, uint8_t destAid,
                                                uint32_t allocationStart, bool isTxss)
{
  NS_LOG_FUNCTION (this << sourceAid << destAid << allocationStart << isTxss);
  AllocationField field;
  /* Allocation Control Field */
  field.SetAllocationType (SERVICE_PERIOD_ALLOCATION);
  field.SetAsPseudoStatic (false);
  /* Allocation Field */
  field.SetSourceAid (sourceAid);
  field.SetDestinationAid (destAid);
  field.SetAllocationStart (allocationStart);
  field.SetAllocationBlockDuration (2000);     // Microseconds
  field.SetNumberOfBlocks (1);

  BF_Control_Field bfField;
  bfField.SetBeamformTraining (true);
  bfField.SetAsInitiatorTxss (isTxss);
  bfField.SetAsResponderTxss (isTxss);
  bfField.SetRxssLength (m_phy->GetDirectionalAntenna ()->GetNumberOfSectors ());

  field.SetBfControl (bfField);
  m_allocationList.push_back (field);

  return (allocationStart + 600);
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
  ctrl.SetCCPresent (false);
  ctrl.SetDiscoveryMode (false);          /* Discovery Mode = 0 when transmitted by PCP/AP */
  ctrl.SetNextBeacon (m_nextBeacon);
  /* Signal the presence of an ATI interval */
  m_isCbapOnly = (m_allocationList.size () == 0);
//  if (m_isCbapOnly)
//    {
      /* For CBAP DTI, the ATI is not present */
//      ctrl.SetATIPresent (true);
//    }
//  else
//    {
      ctrl.SetATIPresent (m_atiPresent);
//    }
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

  /* Service Set Identifier Information Element */
  beacon.SetSsid (GetSsid ());
  /* DMG Capabilities Information Element */
  beacon.AddWifiInformationElement (GetDmgCapabilities ());
  /* DMG Operation Element */
  beacon.AddWifiInformationElement (GetDmgOperationElement ());
  /* Next DMG ATI Information Element */
  beacon.AddWifiInformationElement (GetNextDmgAtiElement ());
  /* Multi-band Information Element */
  beacon.AddWifiInformationElement (GetMultiBandElement ());
  /* Add Relay Capability Element */
  beacon.AddWifiInformationElement (GetRelayCapabilitiesElement ());
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
              if (m_atiPresent)
                {
                  Simulator::Schedule (m_btiRemaining + m_mbifs, &DmgApWifiMac::StartAnnouncementTransmissionInterval, this);
                }
              else
                {
                  Simulator::Schedule (m_btiRemaining + m_mbifs, &DmgApWifiMac::StartDataTransmissionInterval, this);
                }
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
                  /* Set the antenna in Quasi-omni receiving mode */
                  m_phy->GetDirectionalAntenna ()->SetInOmniReceivingMode ();
                }
              else
                {
                  /* Set the antenna in directional receiving mode */
                  m_phy->GetDirectionalAntenna ()->SetInDirectionalReceivingMode ();
                }
            }

          /* Cleanup non-static allocations */
          CleanupAllocations ();
        }
      else
        {
          m_antennaConfigurationIndex++;
          if (m_beaconRandomization && (m_antennaConfigurationIndex == m_antennaConfigurationTable.size ()))
            {
              m_antennaConfigurationIndex = 0;
            }
          m_totalSectors--;

          ANTENNA_CONFIGURATION config = m_antennaConfigurationTable[m_antennaConfigurationIndex];
          NS_LOG_INFO ("Sending DMG Beacon " << Simulator::Now () << " with "
                       << uint32_t (config.first) << " " << uint32_t (config.second));

          if (config.first == 1)
            {
              /* The PCP/AP shall not change DMG Antennas within a BTI */
              /* LBIFS is switching DMG Antenna */
              m_beaconEvent = Simulator::Schedule (m_lbifs, &DmgApWifiMac::SendOneDMGBeacon, this,
                                                   config.first, config.second, m_totalSectors);
            }
          else
            {
              /* SBIFS is switching Sector */
              m_beaconEvent = Simulator::Schedule (m_sbifs, &DmgApWifiMac::SendOneDMGBeacon, this,
                                                   config.first, config.second, m_totalSectors);
            }
        }
    }
}

void
DmgApWifiMac::StartBeaconInterval (void)
{
  NS_LOG_FUNCTION (this << "DMG AP Starting BI at " << Simulator::Now ());

  /* Invoke callback */
  m_biStarted (GetAddress ());

  /* Disable Channel Access by CBAP */
  EndContentionPeriod ();

  /* Timing variables */
  m_biStartTime = Simulator::Now ();

  if (m_btiPeriodicity == 0)
    {
      m_btiPeriodicity = m_nextBeacon;
      StartBeaconTransmissionInterval ();
    }
  else
    {
      /* We will not have BTI access period during this BI */
      m_btiPeriodicity--;
      if (m_atiPresent)
        {
          StartAnnouncementTransmissionInterval ();
          NS_LOG_DEBUG ("ATI for Station:" << GetAddress () << " is scheduled at " << Simulator::Now ());
        }
      else
        {
          StartDataTransmissionInterval ();
          NS_LOG_DEBUG ("DTI for Station:" << GetAddress () << " is scheduled at " << Simulator::Now ());
        }
    }
}

void
DmgApWifiMac::StartBeaconTransmissionInterval (void)
{
  NS_LOG_FUNCTION (this << "DMG AP Starting BTI at " << Simulator::Now ());
  m_accessPeriod = CHANNEL_ACCESS_BTI;

  /* Re-initialize variables */
  m_sectorFeedbackSent.clear ();

  /* Start DMG Beaconing */
  m_totalSectors = m_antennaConfigurationTable.size () - 1;
  if (m_beaconRandomization)
    {
      if (m_antennaConfigurationOffset == m_antennaConfigurationTable.size ())
        {
          m_antennaConfigurationOffset = 0;
        }
      m_antennaConfigurationIndex = m_antennaConfigurationOffset;
      m_antennaConfigurationOffset++;
    }
  else
    {
      m_antennaConfigurationIndex = 0;
    }
  ANTENNA_CONFIGURATION config = m_antennaConfigurationTable[m_antennaConfigurationIndex];

  /* Timing variables */
  m_beaconTransmitted = Simulator::Now ();
  m_btiRemaining = m_btiDuration;
  NS_LOG_INFO ("Sending DMG Beacon " << Simulator::Now () << " with " <<
               uint32_t (config.first) << " " << uint32_t (config.second));

  m_beaconEvent = Simulator::ScheduleNow (&DmgApWifiMac::SendOneDMGBeacon, this,  config.first, config.second, m_totalSectors);
}

void
DmgApWifiMac::StartAssociationBeamformTraining (void)
{
  NS_LOG_FUNCTION (this << "DMG AP Starting A-BFT at " << Simulator::Now ());
  m_accessPeriod = CHANNEL_ACCESS_ABFT;

  /* Schedule next period */
  if (m_atiPresent)
    {
      Simulator::Schedule (m_abftDuration + m_mbifs, &DmgApWifiMac::StartAnnouncementTransmissionInterval, this);
    }
  else
    {
      Simulator::Schedule (m_abftDuration + m_mbifs, &DmgApWifiMac::StartDataTransmissionInterval, this);
    }

  /* Schedule the beginning of the first A-BFT Slot */
  m_remainingSlots = m_ssSlotsPerABFT;
  Simulator::ScheduleNow (&DmgApWifiMac::StartSectorSweepSlot, this);
}

void
DmgApWifiMac::StartSectorSweepSlot (void)
{
  NS_LOG_FUNCTION (this << "DMG AP Starting A-BFT SSW Slot [" << m_ssSlotsPerABFT - m_remainingSlots << "] at " << Simulator::Now ());
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
  NS_LOG_FUNCTION (this << "DMG AP Starting ATI at " << Simulator::Now ());
  m_accessPeriod = CHANNEL_ACCESS_ATI;
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
  NS_LOG_INFO ("DMG AP Starting DTI at " << Simulator::Now ());
  m_accessPeriod = CHANNEL_ACCESS_DTI;

  /* Schedule the beginning of next BHI interval */
  Time nextBeaconInterval = m_beaconInterval - (Simulator::Now () - m_biStartTime);
  m_dtiStarted (GetAddress (), nextBeaconInterval);
  Simulator::Schedule (nextBeaconInterval, &DmgApWifiMac::StartBeaconInterval, this);
  NS_LOG_DEBUG ("Next Beacon Interval will start at " << Simulator::Now () + nextBeaconInterval);

  /* Start CBAPs and SPs */
  if (m_isCbapOnly)
    {
      NS_LOG_INFO ("CBAP allocation only in DTI");
      m_phy->GetDirectionalAntenna ()->SetInOmniReceivingMode ();
      Simulator::ScheduleNow (&DmgApWifiMac::StartContentionPeriod, this, BROADCAST_CBAP, nextBeaconInterval);
    }
  else
    {
      AllocationField field;
      for (AllocationFieldList::iterator iter = m_allocationList.begin (); iter != m_allocationList.end (); iter++)
        {
          field = (*iter);
          if (field.GetAllocationType () == SERVICE_PERIOD_ALLOCATION)
            {
              Time spStart = MicroSeconds (field.GetAllocationStart ());
              Time servicePeriodLength = MicroSeconds (field.GetAllocationBlockDuration ());

              if ((field.GetSourceAid () == AID_AP) && (!field.GetBfControl ().IsBeamformTraining ()))
                {
                  uint8_t destAid = field.GetDestinationAid ();
                  Mac48Address destAddress = m_aidMap[destAid];
                  Simulator::Schedule (spStart, &DmgApWifiMac::StartServicePeriod,
                                       this, field.GetAllocationID (), servicePeriodLength, destAid, destAddress, true);
                  Simulator::Schedule (spStart + servicePeriodLength, &DmgApWifiMac::EndServicePeriod, this);
                }
              else if ((field.GetSourceAid () == AID_BROADCAST) && (field.GetDestinationAid () == AID_BROADCAST))
                {
                  /* The PCP/AP may create SPs in its beacon interval with the source and destination AID
                   * subfields within an Allocation field set to 255 to prevent transmissions during
                   * specific periods in the beacon interval. This period can used for Dynamic Allocation
                   * of service periods (Polling) */
                  NS_LOG_INFO ("No transmission is allowed from " << field.GetAllocationStart () <<
                               " till " << field.GetAllocationBlockDuration ());
                }
              else if ((field.GetDestinationAid () == AID_AP) || (field.GetDestinationAid () == AID_BROADCAST))
                {
                  /* The STA identified by the Destination AID field in the Extended Schedule element
                   * should be in the receive state for the duration of the SP in order to receive
                   * transmissions from the source DMG STA. */
                  uint8_t sourceAid = field.GetSourceAid ();
                  Mac48Address sourceAddress = m_aidMap[sourceAid];
                  Simulator::Schedule (spStart, &DmgApWifiMac::StartServicePeriod, this,
                                       field.GetAllocationID (), servicePeriodLength, sourceAid, sourceAddress, true);
                }
            }
          else if ((field.GetAllocationType () == CBAP_ALLOCATION) &&
                  ((field.GetSourceAid () == AID_BROADCAST) || (field.GetSourceAid () == AID_AP) || (field.GetDestinationAid () == AID_AP)))

            {
              Time cbapEnd = MicroSeconds (field.GetAllocationStart ()) + MicroSeconds (field.GetAllocationBlockDuration ());
              /* Schedule two events for the beginning of the relay mode */
              Simulator::Schedule (MicroSeconds (field.GetAllocationStart ()), &DmgApWifiMac::StartContentionPeriod, this,
                                   field.GetAllocationID (), MicroSeconds (field.GetAllocationBlockDuration ()));
              Simulator::Schedule (cbapEnd, &DmgApWifiMac::EndContentionPeriod, this);
            }
        }
    }
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
          MapTxSnr (from, ssw.GetSectorID (), ssw.GetDMGAntennaID (), m_stationManager->GetRxSnr ());

          /* If we receive one SSW Frame at least, then we schedule SSW-FBCK */
          if (!m_sectorFeedbackSent[from])
            {
              m_sectorFeedbackSent[from] = true;

              /* Set the best TX antenna configuration reported by the SSW-FBCK Field */
              DMG_SSW_FBCK_Field sswFeedback = sswFrame.GetSswFeedbackField ();
              sswFeedback.IsPartOfISS (false);

              /* The Sector Sweep Frame contains feedback about the the best Tx Sector in the DMG-AP with the sending DMG-STA */
              ANTENNA_CONFIGURATION_TX antennaConfigTx = std::make_pair (sswFeedback.GetSector (), sswFeedback.GetDMGAntenna ());
              ANTENNA_CONFIGURATION_RX antennaConfigRx = std::make_pair (NO_ANTENNA_CONFIG, NO_ANTENNA_CONFIG);
              m_bestAntennaConfig[hdr->GetAddr2 ()] = std::make_pair (antennaConfigTx, antennaConfigRx);

              NS_LOG_INFO ("Best TX Antenna Sector Config by this DMG AP to DMG STA=" << from
                           << ": SectorID=" << uint32_t (antennaConfigTx.first)
                           << ", AntennaID=" << uint32_t (antennaConfigTx.second));

              /* Raise an event that we selected the best sector to the DMG STA */
              m_slsCompleted (from, CHANNEL_ACCESS_BHI, antennaConfigTx.first, antennaConfigTx.second);

              /* Indicate this DMG-STA as waiting for Beam Refinement Phase */
              m_stationBrpMap[from] = true;

              Time sswFbckTime = m_low->GetSectorSweepDuration (ssw.GetCountDown ()) + m_mbifs;
              NS_LOG_INFO ("Scheduled SSW-FBCK Frame to " << hdr->GetAddr2 () << " at " << Simulator::Now () + sswFbckTime);
              Simulator::Schedule (sswFbckTime, &DmgApWifiMac::SendSswFbckAfterRss, this, from);
            }
        }

      return;
    }
  else if (hdr->IsMgt ())
    {
      if (hdr->IsProbeReq ())
        {
          NS_ASSERT(hdr->GetAddr1 ().IsBroadcast ());
          SendProbeResp (from);
          return;
        }
      else if (hdr->GetAddr1 () == GetAddress ())
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

                  /* Set AID of the station */
                  Ptr<DmgCapabilities> capabilities = StaticCast<DmgCapabilities> (infoMap[IE_DMG_CAPABILITIES]);
                  capabilities->SetAID (m_aidCounter & 0xFF);
                  m_associatedStationsInfoByAddress[from] = infoMap;
                  m_associatedStationsInfoByAid[m_aidCounter] = infoMap;
                  MapAidToMacAddress (m_aidCounter, hdr->GetAddr2 ());

                  /** Check Relay Capabilities **/
                  Ptr<RelayCapabilitiesElement> relayElement =
                      DynamicCast<RelayCapabilitiesElement> (assocReq.GetInformationElement (IE_RELAY_CAPABILITIES));

                  /* Check if the DMG STA supports RDS */
                  if (relayElement->GetRelayCapabilitiesInfo ().GetRelaySupportability ())
                    {
                      m_rdsList[m_aidCounter] = relayElement->GetRelayCapabilitiesInfo ();
                      NS_LOG_DEBUG ("Station=" << from << " with AID=" << m_aidCounter << " supports RDS operation");
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

                        /* Received Relay Search Request, reply back with the list of RDSs */
                        SendRelaySearchResponse (from, requestHdr.GetDialogToken ());

                        /* Send Unsolicited Relay Search Response to the destination */
                        Ptr<DmgCapabilities> dmgCapabilities = StaticCast<DmgCapabilities>
                            (m_associatedStationsInfoByAid[requestHdr.GetDestinationRedsAid ()][IE_DMG_CAPABILITIES]);
                        SendRelaySearchResponse (dmgCapabilities->GetStaAddress (), requestHdr.GetDialogToken ());

                         /* Get Source REDS DMG Capabilities */
                        Ptr<DmgCapabilities> srcDmgCapabilities = StaticCast<DmgCapabilities>
                            (m_associatedStationsInfoByAddress[hdr->GetAddr2 ()][IE_DMG_CAPABILITIES]);

                        /* The PCP/AP should schedule two SPs for each RDS in the response */
                        uint32_t allocationStart = 0;
                        for (RelayCapableStaList::const_iterator iter = m_rdsList.begin (); iter != m_rdsList.end (); iter++)
                          {
                            /* First SP between the source REDS and the RDS */
                            allocationStart = AllocateBeamformingServicePeriod (srcDmgCapabilities->GetAID (),
                                                                                iter->first, allocationStart, true);
                            /* Second SP between the source REDS and the Destination REDS */
                            allocationStart = AllocateBeamformingServicePeriod (iter->first, requestHdr.GetDestinationRedsAid (),
                                                                                allocationStart, true);
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
                    case WifiActionHeader::DMG_RLS_TEARDOWN:
                      {
                        ExtRlsTearDown header;
                        packet->RemoveHeader (header);
                        return;
                      }
                    case WifiActionHeader::DMG_INFORMATION_REQUEST:
                      {
                        ExtInformationRequest requestHdr;
                        packet->RemoveHeader (requestHdr);
                        NS_LOG_INFO ("Received Information Request Frame from " << from);
                        Mac48Address subjectAddress = requestHdr.GetSubjectAddress ();

                        ExtInformationResponse responseHdr;
                        responseHdr.SetSubjectAddress (subjectAddress);
                        /* Obtain Subject Station DMG Capabilities */
                        Ptr<DmgCapabilities> dmgCapabilities =
                            StaticCast<DmgCapabilities> (m_associatedStationsInfoByAddress[subjectAddress][IE_DMG_CAPABILITIES]);
                        responseHdr.AddDmgCapabilitiesElement (dmgCapabilities);
                        /* Request Element */
                        Ptr<RequestElement> requestElement = requestHdr.GetRequestInformationElement ();
                        WifiInformationElementIdList elementList =  requestElement->GetWifiInformationElementIdList ();
                        responseHdr.SetRequestInformationElement (requestElement);
                        for (WifiInformationElementIdList::const_iterator iter = elementList.begin ();
                             iter != elementList.end (); iter++)
                          {
                            responseHdr.AddWifiInformationElement (m_associatedStationsInfoByAddress[subjectAddress][*iter]);
                          }
                        /* Send Information Resposne Frame */
                        SendInformationResponse (from, responseHdr);
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

  /* Generate Antenna Configuration Table */
  m_antennaConfigurationOffset = 0;
  for (uint8_t i = 1; i <= m_phy->GetDirectionalAntenna ()->GetNumberOfAntennas (); i++)
    {
      for (uint8_t j = 1; j <= m_phy->GetDirectionalAntenna ()->GetNumberOfSectors (); j++)
        {
          m_antennaConfigurationTable.push_back (std::make_pair (j, i));
        }
    }

  /* Start Beacon Interval */
  NS_LOG_DEBUG ("Starting DMG Access Point " << GetAddress () << " at time " << Simulator::Now ());
  StartBeaconInterval ();

  DmgWifiMac::DoInitialize ();
}

} // namespace ns3
