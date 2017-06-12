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
    .AddAttribute ("StartBeaconing", "The amount of delay to add before the beginning of DMG Beaconing.",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&DmgApWifiMac::m_beaconingDelay),
                   MakeTimeChecker ())
    .AddAttribute ("AllowBeaconing", "Allow PCP/AP to start Beaconing upon initialization.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&DmgApWifiMac::m_allowBeaconing),
                   MakeBooleanChecker ())
    .AddAttribute ("BeaconInterval", "The interval between two Target Beacon Transmission Times (TBTTs).",
                   TimeValue (aMaxBIDuration),
                   MakeTimeAccessor (&DmgApWifiMac::GetBeaconInterval,
                                     &DmgApWifiMac::SetBeaconInterval),
                   MakeTimeChecker (TU, aMaxBIDuration))
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

    .AddAttribute ("OperationElement", "Whether to include DMG Operation Element in DMG Beacons.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&DmgApWifiMac::m_announceOperationElement),
                   MakeBooleanChecker ())
    .AddAttribute ("ScheduleElement", "Whether to include Extended Schedule Element in DMG Beacons.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&DmgApWifiMac::m_scheduleElement),
                   MakeBooleanChecker ())
    .AddAttribute ("ATIPresent", "The BI period contains ATI access period.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&DmgApWifiMac::m_atiPresent),
                   MakeBooleanChecker ())
    .AddAttribute ("ATIDuration", "The duration of the ATI Period.",
                   TimeValue (MicroSeconds (0)),
                   MakeTimeAccessor (&DmgApWifiMac::m_atiDuration),
                   MakeTimeChecker ())

    /* DMG PCP/AP Clustering */
    .AddAttribute ("EnableDecentralizedClustering", "Enable/Disable decentralized clustering.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&DmgApWifiMac::m_enableDecentralizedClustering),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableCentralizedClustering", "Enable/Disable centralized clustering.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&DmgApWifiMac::m_enableCentralizedClustering),
                   MakeBooleanChecker ())
    .AddAttribute ("ClusterMaxMem", "The maximum number of PCPs and/or APs, including the S-PCP/S-AP.",
                   UintegerValue (2),
                   MakeUintegerAccessor (&DmgApWifiMac::m_clusterMaxMem),
                   MakeUintegerChecker<uint8_t> (2, 8))
    .AddAttribute ("BeaconSPDuration", "The size of a Beacon SP used for PCP/AP clustering in microseconds.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&DmgApWifiMac::m_beaconSPDuration),
                   MakeUintegerChecker<uint8_t> (0, 255))
    .AddAttribute ("ClusterRole", "The role of the PCP/AP in the cluster.",
                   EnumValue (NOT_PARTICIPATING),
                   MakeEnumAccessor (&DmgApWifiMac::m_clusterRole),
                   MakeEnumChecker (SYNC_PCP_AP, "S-PCP/S-AP",
                                    NOT_PARTICIPATING, "NotParticipating",
                                    PARTICIPATING, "Participating"))
    .AddAttribute ("ChannelMonitorDuration", "The amount of time to spend monitoring a channel for activities.",
                   TimeValue (Seconds (aMinChannelTime)),
                   MakeTimeAccessor (&DmgApWifiMac::m_channelMonitorTime),
                   MakeTimeChecker ())

    /* DMG Parameters */
    .AddAttribute ("CBAPSource", "Indicates that PCP/AP has a higher priority for transmission in CBAP",
                   BooleanValue (false),
                   MakeBooleanAccessor (&DmgApWifiMac::m_isCbapSource),
                   MakeBooleanChecker ())

    /* Beacon Interval Traces */
    .AddTraceSource ("BIStarted", "A new Beacon Interval has started.",
                     MakeTraceSourceAccessor (&DmgApWifiMac::m_biStarted),
                     "ns3::Mac48Address::TracedCallback")
    .AddTraceSource ("DTIStarted", "The Data Transmission Interval access period started.",
                     MakeTraceSourceAccessor (&DmgApWifiMac::m_dtiStarted),
                     "ns3::DmgApWifiMac::DtiStartedTracedCallback")

    /* DMG PCP/AP Clustering */
    .AddTraceSource ("JoinedCluster", "The PCP/AP joined a cluster.",
                     MakeTraceSourceAccessor (&DmgApWifiMac::m_joinedCluster),
                     "ns3::DmgApWifiMac::JoinedClusterTracedCallback")

    /* Dynamic Allocation Traces */
    .AddTraceSource ("PPCompleted", "The Polling Period has ended.",
                     MakeTraceSourceAccessor (&DmgApWifiMac::m_ppCompleted),
                     "ns3::Mac48Address::TracedCallback")
    .AddTraceSource ("GPCompleted", "The Grant Period has ended.",
                     MakeTraceSourceAccessor (&DmgApWifiMac::m_gpCompleted),
                     "ns3::Mac48Address::TracedCallback")

    /* Spatial Sharing */
    .AddTraceSource ("ChannelQualityReportReceived", "The PCP/AP received Directional Channel Quality Report.",
                     MakeTraceSourceAccessor (&DmgApWifiMac::m_qualityReportReceived),
                     "ns3::DmgApWifiMac::QualityReportReceivedTracedCallback")

    /* DMG TS Traces */
    .AddTraceSource ("ADDTSReceived", "The PCP/AP received DMG ADDTS Request.",
                     MakeTraceSourceAccessor (&DmgApWifiMac::m_addTsRequestReceived),
                     "ns3::DmgApWifiMac::AddTsRequestReceivedTracedCallback")
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
  m_initiateDynamicAllocation = false;
  m_monitoringChannel = false;

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
  capabilities->SetSupportedMCS (m_maxScRxMcs, m_maxOfdmRxMcs, m_maxScTxMcs, m_maxOfdmTxMcs, m_supportLpSc, true); /* LP SC is not supported yet */
  capabilities->SetAppduSupported (false);     /* Currently A-PPDU Agregatio is not supported*/

  /* DMG PCP/AP Capability Information Field */
  capabilities->SetTDDTI (true);
  capabilities->SetPseudoStaticAllocations (true);
  capabilities->SetMaxAssociatedStaNumber (254);
  capabilities->SetPowerSource (true); /* Not battery powered */
  capabilities->SetPcpForwarding (true);
  capabilities->SetDecentralizedClustering (m_enableDecentralizedClustering);
  capabilities->SetCentralizedClustering (m_enableCentralizedClustering);

  return capabilities;
}

Ptr<DmgOperationElement>
DmgApWifiMac::GetDmgOperationElement (void) const
{
  Ptr<DmgOperationElement> operation = Create<DmgOperationElement> ();
  /* DMG Operation Information */
  operation->SetTDDTI (true);         /* We are able to provide time division channel access */
  operation->SetPseudoStaticAllocations (true);
  operation->SetPcpHandover (m_pcpHandoverSupport);
  /* DMG BSS Parameter Configuration */
  operation->SetMinBHIDuration (static_cast<uint16_t> (GetBHIDuration ().GetMicroSeconds ()));
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
DmgApWifiMac::AllocateCbapPeriod (bool staticAllocation, uint32_t allocationStart, uint16_t blockDuration)
{
  NS_LOG_FUNCTION (this << staticAllocation << allocationStart << blockDuration);
  AllocateSingleContiguousBlock (0, CBAP_ALLOCATION, staticAllocation, AID_BROADCAST, AID_BROADCAST, allocationStart, blockDuration);
  return (allocationStart + blockDuration);
}

uint32_t
DmgApWifiMac::AllocateSingleContiguousBlock (AllocationID allocationID,
                                             AllocationType allocationType,
                                             bool staticAllocation,
                                             uint8_t sourceAid, uint8_t destAid,
                                             uint32_t allocationStart, uint16_t blockDuration)
{
  NS_LOG_FUNCTION (this);
  return (AddAllocationPeriod (allocationID, allocationType, staticAllocation, sourceAid, destAid,
                               allocationStart, blockDuration, 0, 1));
}

uint32_t
DmgApWifiMac::AllocateMultipleContiguousBlocks (AllocationID allocationID,
                                                AllocationType allocationType,
                                                bool staticAllocation,
                                                uint8_t sourceAid, uint8_t destAid,
                                                uint32_t allocationStart,
                                                uint16_t blockDuration, uint8_t blocks)
{
  NS_LOG_FUNCTION (this);
  AddAllocationPeriod (allocationID, allocationType, staticAllocation, sourceAid, destAid,
                       allocationStart, blockDuration, 0, blocks);
  return (allocationStart + blockDuration * blocks);
}

void
DmgApWifiMac::AllocateDTIAsServicePeriod (AllocationID id, uint8_t srcAid, uint8_t dstAid)
{
  NS_LOG_FUNCTION (this << uint32_t (id) << uint32_t (srcAid) << uint32_t (dstAid));
  uint16_t spDuration = floor (GetDTIDuration ().GetMicroSeconds () / MAX_NUM_BLOCKS);
  AddAllocationPeriod (id, SERVICE_PERIOD_ALLOCATION, true, srcAid, dstAid,
                       0, spDuration, 0, MAX_NUM_BLOCKS);
}

uint32_t
DmgApWifiMac::AddAllocationPeriod (AllocationID id,
                                   AllocationType allocationType,
                                   bool staticAllocation,
                                   uint8_t sourceAid, uint8_t destAid,
                                   uint32_t allocationStart, uint16_t blockDuration,
                                   uint16_t blockPeriod, uint8_t blocks)
{
  NS_LOG_FUNCTION (this << uint32_t (id) << allocationType << staticAllocation
                   << uint32_t (sourceAid) << uint32_t (destAid)
                   << allocationStart << blockDuration << blockPeriod << uint32_t (blocks));
  AllocationField field;
  /* Allocation Control Field */
  field.SetAllocationID (id);
  field.SetAllocationType (allocationType);
  field.SetAsPseudoStatic (staticAllocation);
  /* Allocation Field */
  field.SetSourceAid (sourceAid);
  field.SetDestinationAid (destAid);
  field.SetAllocationStart (allocationStart);
  field.SetAllocationBlockDuration (blockDuration);
  field.SetAllocationBlockPeriod (blockPeriod);
  field.SetNumberOfBlocks (blocks);
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
  NS_LOG_FUNCTION (this << uint32_t (sourceAid) << uint32_t (destAid) << allocationStart << isTxss);
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
  bfField.SetRxssLength (GetNumberOfSectors ());

  field.SetBfControl (bfField);
  m_allocationList.push_back (field);

  return (allocationStart + 2200);
}

void
DmgApWifiMac::ModifyAllocation (AllocationID id, uint8_t srcAid, uint8_t dstAid, uint32_t newStartTime, uint16_t newDuration)
{
  NS_LOG_FUNCTION (this << uint32_t (id) << uint32_t (srcAid) << uint32_t (dstAid) << newStartTime << newDuration);
  for (AllocationFieldList::iterator iter = m_allocationList.begin (); iter != m_allocationList.end (); iter++)
    {
      if (((*iter).GetAllocationID () == id) &&
          ((*iter).GetSourceAid () == srcAid) && ((*iter).GetDestinationAid () == dstAid))
        {
          (*iter).SetAllocationStart (newStartTime);
          (*iter).SetAllocationBlockDuration (newDuration);
          break;
        }
    }
}

AllocationFieldList
DmgApWifiMac::GetAllocationList (void) const
{
  return m_allocationList;
}

void
DmgApWifiMac::SendOneDMGBeacon (uint8_t sectorID, uint8_t antennaID, uint16_t count)
{
  NS_LOG_FUNCTION (this << uint32_t (sectorID) << uint32_t (antennaID) << count);
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
  ctrl.SetCCPresent (m_enableCentralizedClustering || m_enableDecentralizedClustering);
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
  parameters.Set_ECPAC_Policy_Enforced (false); // Decentralized clustering
  beacon.SetDMGParameters (parameters);

  /* Cluster Control Field */
  if (ctrl.IsCCPresent ())
    {
      ExtDMGClusteringControlField cluster;
      cluster.SetDiscoveryMode (ctrl.IsDiscoveryMode ());
      cluster.SetBeaconSpDuration (m_beaconSPDuration);
      cluster.SetClusterMaxMem (m_clusterMaxMem);
      cluster.SetClusterMemberRole (m_clusterRole);
      if (m_clusterRole == SYNC_PCP_AP)
        {
          m_ClusterID = GetAddress ();
        }
      cluster.SetClusterID (m_ClusterID);
      beacon.SetClusterControlField (cluster);
    }

  /* Service Set Identifier Information Element */
  beacon.SetSsid (GetSsid ());

  /* DMG Capabilities Information Element */
  if (m_announceDmgCapabilities)
    {
      beacon.AddWifiInformationElement (GetDmgCapabilities ());
    }
  /* DMG Operation Element */
  if (m_announceOperationElement)
    {
      beacon.AddWifiInformationElement (GetDmgOperationElement ());
    }
  /* Next DMG ATI Information Element */
  if (m_atiPresent)
    {
      beacon.AddWifiInformationElement (GetNextDmgAtiElement ());
    }
  /* Multi-band Information Element */
  if (m_supportMultiBand)
    {
      beacon.AddWifiInformationElement (GetMultiBandElement ());
    }
  /* Add Relay Capability Element */
  if (m_redsActivated || m_rdsActivated)
    {
      beacon.AddWifiInformationElement (GetRelayCapabilitiesElement ());
    }
  /* Extended Schedule Element */
  if (m_scheduleElement)
    {
      beacon.AddWifiInformationElement (GetExtendedScheduleElement ());
    }

  /* Set Antenna Sector in the PHY Layer */
  m_phy->GetDirectionalAntenna ()->SetCurrentTxSectorID (sectorID);
  m_phy->GetDirectionalAntenna ()->SetCurrentTxAntennaID (antennaID);

  /* The DMG beacon has it's own special queue, so we load it in there */
  m_beaconDca->TransmitDmgBeacon (beacon, hdr);
}

void
DmgApWifiMac::SendDmgAddTsResponse (Mac48Address to, StatusCode code, TsDelayElement &delayElem, DmgTspecElement &elem)
{
  NS_LOG_FUNCTION (this);
  WifiMacHeader hdr;
  hdr.SetAction ();
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetBssid ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  DmgAddTSResponseFrame frame;
  frame.SetStatusCode (code);
  frame.SetTsDelay (delayElem);
  frame.SetDmgTspecElement (elem);

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.qos = WifiActionHeader::ADDTS_RESPONSE;
  actionHdr.SetAction (WifiActionHeader::QOS, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (frame);
  packet->AddHeader (actionHdr);

  m_dca->Queue (packet, hdr);
}

AllocationDataList
DmgApWifiMac::GetSprList (void) const
{
  return m_sprList;
}

void
DmgApWifiMac::AddGrantData (AllocationData info)
{
  m_grantList.push_back (info);
}

uint8_t
DmgApWifiMac::GetStationAid (Mac48Address address) const
{
  MAC_MAP_CI it = m_macMap.find (address);
  if (it != m_macMap.end ())
    {
      return it->second;
    }
  else
    {
      return -1;
    }
}

Mac48Address
DmgApWifiMac::GetStationAddress (uint8_t aid) const
{
  AID_MAP_CI it = m_aidMap.find (aid);
  if (it != m_aidMap.end ())
    {
      return it->second;
    }
  else
    {
      return Mac48Address ();
    }
}

Time
DmgApWifiMac::GetBHIDuration (void) const
{
  return m_btiDuration + m_abftDuration + m_atiDuration + 2 * GetMbifs ();
}

Time
DmgApWifiMac::GetDTIDuration (void) const
{
  return m_beaconInterval - GetBHIDuration ();
}

Time
DmgApWifiMac::GetDTIRemainingTime (void) const
{
  return GetDTIDuration () - (Simulator::Now () - m_dtiStartTime);
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
  else if (hdr.IsPollFrame ())
    {
      /**
       * The PCP/AP expects an SPR frame in response to each transmitted Poll frame
       * so steer PCP/AP receive antenna towards it
       */
      Simulator::Schedule (m_responseOffset, &DmgApWifiMac::SteerAntennaToward, this, hdr.GetAddr1 ());

      /* Schedule next poll frame */
      m_polledStationIndex++;
      if (m_polledStationIndex < m_polledStationsCount)
        {
          Simulator::Schedule (GetSbifs (), &DmgApWifiMac::SendPollFrame, this, m_pollStations[m_polledStationIndex]);
        }
    }
  else if (hdr.IsGrantFrame ())
    {
      /* Special case when the grant is for an allocation with the PCP/AP */
      if ((n_grantDynamicInfo.GetSourceAID () == AID_AP) || (n_grantDynamicInfo.GetDestinationAID () == AID_AP))
        {
          /* Initiate AP Service Period */
          uint8_t peerAid;
          bool isSource = false;
          if (n_grantDynamicInfo.GetSourceAID () == AID_AP)
            {
              /* The PCP/AP is the initiator in the allocated SP */
              isSource = true;
              peerAid = n_grantDynamicInfo.GetDestinationAID ();
            }
          else
            {
              /* The PCP/AP is the responder in the allocated SP */
              peerAid = n_grantDynamicInfo.GetSourceAID ();
            }
          Simulator::Schedule (2 * GetSifs (), &DmgApWifiMac::StartServicePeriod, this,
                               0, MicroSeconds (n_grantDynamicInfo.GetAllocationDuration ()),
                               peerAid, m_aidMap[peerAid], isSource);
        }
    }
  else if (hdr.IsSSW_FBCK ())
    {
      ANTENNA_CONFIGURATION antennaConfig;
      Mac48Address address = hdr.GetAddr1 ();
      if (m_receivedOneSSW && m_isResponderTXSS)
        {
          antennaConfig = m_bestAntennaConfig[address].first;
        }
      else
        {
          antennaConfig.first = NO_ANTENNA_CONFIG;
          antennaConfig.second = NO_ANTENNA_CONFIG;
        }
      /* Raise an event that we selected the best sector to the DMG STA */
      m_slsCompleted (address, CHANNEL_ACCESS_BHI, antennaConfig.first, antennaConfig.second);
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

  /* Initiate BTI in DMG Beacon DCA */
  m_beaconDca->InitiateBTI ();

  /* Timing variables */
  m_beaconTransmitted = Simulator::Now ();
  m_btiRemaining = m_btiDuration;
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
      Simulator::Schedule (GetSectorSweepSlotTime (m_ssFramesPerSlot), &DmgApWifiMac::StartSectorSweepSlot, this);
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
  m_dtiStartTime = Simulator::Now ();
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
              Time spLength = MicroSeconds (field.GetAllocationBlockDuration ());
              Time spPeriod = MicroSeconds (field.GetAllocationBlockPeriod ());
              if ((field.GetSourceAid () == AID_AP) && (!field.GetBfControl ().IsBeamformTraining ()))
                {
                  uint8_t destAid = field.GetDestinationAid ();
                  Mac48Address destAddress = m_aidMap[destAid];
                  ScheduleServicePeriod (field.GetNumberOfBlocks (), spStart, spLength, spPeriod,
                                         field.GetAllocationID (), destAid, destAddress, true);
                }
              else if ((field.GetSourceAid () == AID_BROADCAST) && (field.GetDestinationAid () == AID_BROADCAST))
                {
                  /* The PCP/AP may create SPs in its beacon interval with the source and destination AID
                   * subfields within an Allocation field set to 255 to prevent transmissions during
                   * specific periods in the beacon interval. This period can used for Dynamic Allocation
                   * of service periods (Polling) */
                  if (m_initiateDynamicAllocation)
                    {
                      Simulator::Schedule (spStart, &DmgApWifiMac::InitiatePollingPeriod, this, spLength);
                    }
                  else
                    {
                      NS_LOG_INFO ("No transmission is allowed from " << field.GetAllocationStart () <<
                                   " till " << field.GetAllocationBlockDuration ());
                    }
                }
              else if ((field.GetDestinationAid () == AID_AP) || (field.GetDestinationAid () == AID_BROADCAST))
                {
                  uint8_t sourceAid = field.GetSourceAid ();
                  Mac48Address sourceAddress = m_aidMap[sourceAid];
                  ScheduleServicePeriod (field.GetNumberOfBlocks (), spStart, spLength, spPeriod,
                                         field.GetAllocationID (), sourceAid, sourceAddress, false);
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
 * Dynamic Allocation of Service Periods Functions
 */
void
DmgApWifiMac::InitiateDynamicAllocation (void)
{
  NS_LOG_FUNCTION (this);
  m_polledStationsCount = m_pollStations.size ();
  if (m_polledStationsCount > 0)
    {
      Time ppDuration;  /* The duration of the polling period to be allocated in DTI */
      m_initiateDynamicAllocation = true;
      m_pollFrameTxTime = GetFrameDurationInMicroSeconds (m_phy->CalculateTxDuration (POLL_FRAME_SIZE,
                                                                                      m_stationManager->GetDmgLowestScVector (), 0));
      m_sprFrameTxTime = GetSprFrameDuration ();
      m_grantFrameTxTime = GetFrameDurationInMicroSeconds (m_phy->CalculateTxDuration (GRANT_FRAME_SIZE,
                                                                                       m_stationManager->GetDmgLowestScVector (), 0));
      ppDuration = GetPollingPeriodDuration (m_polledStationsCount);
      /* Allocate SP for the Polling phase as indicated in 9.33.7.2 */
      AllocateSingleContiguousBlock (1, SERVICE_PERIOD_ALLOCATION, true,
                                     AID_BROADCAST, AID_BROADCAST, 0, ppDuration.GetMicroSeconds ());
    }
  else
    {
      NS_LOG_INFO ("No station is available for dynamic allocation.");
    }
}

Time
DmgApWifiMac::GetPollingPeriodDuration (uint8_t polledStationsCount)
{
  NS_LOG_FUNCTION (this);
  return GetPollingPeriodDuration (m_pollFrameTxTime, m_sprFrameTxTime, polledStationsCount);
}

Time
DmgApWifiMac::GetPollingPeriodDuration (Time pollFrameTxTime, Time sprFrameTxTime, uint8_t polledStationsCount)
{
  NS_LOG_FUNCTION (this);
  Time ppDuration;
  ppDuration = (pollFrameTxTime + sprFrameTxTime) * polledStationsCount;
  ppDuration += GetSbifs () * (polledStationsCount - 1) + GetSifs () * polledStationsCount;
  return ppDuration;
}

void
DmgApWifiMac::InitiatePollingPeriod (Time ppLength)
{
  NS_LOG_FUNCTION (this << ppLength);
  m_currentAllocation = SERVICE_PERIOD_ALLOCATION;
  /* Start Polling Period for dynamic allocation of a SP */
  Simulator::ScheduleNow (&DmgApWifiMac::StartPollingPeriod, this);
  /* Schedule the end of Polling Period */
  Simulator::Schedule (ppLength, &DmgApWifiMac::PollingPeriodCompleted, this);
}

void
DmgApWifiMac::StartPollingPeriod (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Starting Polling Period for " << m_polledStationsCount << " DMG STA(s)");
  m_polledStationIndex = 0;
  Simulator::ScheduleNow (&DmgApWifiMac::SendPollFrame, this, m_pollStations[m_polledStationIndex]);
}

void
DmgApWifiMac::PollingPeriodCompleted (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Polling Period is Completed");
  m_ppCompleted (GetAddress ());
  /* Schedule the start of Grant Period */
  if (m_grantList.size () > 0)
    {
      Simulator::Schedule (GetSifs (), &DmgApWifiMac::StartGrantPeriod, this);
    }
}

void
DmgApWifiMac::StartGrantPeriod (void)
{
  NS_LOG_FUNCTION (this << m_grantList.size ());
  m_grantIndex = 0;
  SendGrantFrames ();
}

void
DmgApWifiMac::SendGrantFrames (void)
{
  NS_LOG_FUNCTION (this);
  AllocationData data = m_grantList.front ();
  BF_Control_Field bf = data.second;
  n_grantDynamicInfo = data.first;
  Time hdrDuration = MicroSeconds (n_grantDynamicInfo.GetAllocationDuration ()) + 2 * GetSifs ();
  Time nextGrantPeriod = hdrDuration; /* Next Grant period start time*/
  if ((n_grantDynamicInfo.GetSourceAID () == AID_AP) || (n_grantDynamicInfo.GetDestinationAID () == AID_AP))
    {
      Mac48Address peerAddress = m_pollStations[m_grantIndex];

      /* If the communication is with the AP then send one Grant frame only */
      nextGrantPeriod += m_grantFrameTxTime;
      Simulator::ScheduleNow (&DmgApWifiMac::SendGrantFrame, this,
                              peerAddress, hdrDuration, n_grantDynamicInfo, bf);
    }
  else
    {
      /* If the communication is between two DMG STAs then sends two Grant frames */
      /* The Dynamic Allocation Info field within Grant frames transmitted as part of the same GP shall be the same */
      Mac48Address dstAddress = m_aidMap[n_grantDynamicInfo.GetDestinationAID ()];
      Mac48Address srcAddress = m_aidMap[n_grantDynamicInfo.GetSourceAID ()];

      /* Send the second grant frame to the source station */
      Simulator::Schedule (GetSbifs () + m_grantFrameTxTime, &DmgApWifiMac::SendGrantFrame, this,
                           srcAddress, hdrDuration, n_grantDynamicInfo, bf);

      /* Send the first grant frame to the destination station */
      hdrDuration += m_grantFrameTxTime + GetSbifs ();
      Simulator::ScheduleNow (&DmgApWifiMac::SendGrantFrame, this,
                              dstAddress, hdrDuration, n_grantDynamicInfo, bf);

      nextGrantPeriod += m_grantFrameTxTime * 2 + GetSbifs ();
    }

  /* Schedule next Grant Period if there is any */
  m_grantIndex++;
  m_grantList.pop_front ();
  if (m_grantList.size () > 0)
    {
      Simulator::Schedule (nextGrantPeriod, &DmgApWifiMac::SendGrantFrames, this);
    }
  else
    {
      m_sprList.clear ();
      Simulator::Schedule (nextGrantPeriod, &DmgApWifiMac::GrantPeriodCompleted, this);
    }
}

void
DmgApWifiMac::GrantPeriodCompleted (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Grant Period is Completed");
  m_gpCompleted (GetAddress ());
}

Time
DmgApWifiMac::GetOffsetOfSprTransmission (uint32_t index)
{
  NS_LOG_FUNCTION (this << index);
  Time offsetOfSpr;
  if (index == 0)
    {
      offsetOfSpr = GetSifs ();
    }
  else
    {
      offsetOfSpr = index * (m_sprFrameTxTime) + (index + 1) * GetSifs ();
    }
  return offsetOfSpr;
}

Time
DmgApWifiMac::GetDurationOfPollTransmission (void)
{
  NS_LOG_FUNCTION (this);
  Time duration;
  if (m_polledStationIndex < m_polledStationsCount)
    {
      duration = (m_polledStationsCount - (m_polledStationIndex + 1)) * (m_pollFrameTxTime + GetSbifs ());
    }
  else
    {
      duration = aTSFResolution;
    }
  return duration;
}

Time
DmgApWifiMac::GetResponseOffset (void)
{
  /** Response Offset(i) = Duration_of_Poll_transmission(i,n) + Offset_of_SPR_transmission(j) **/
  return (GetDurationOfPollTransmission () + GetOffsetOfSprTransmission (m_polledStationIndex));
}

Time
DmgApWifiMac::GetPollFrameDuration (void)
{
  /** Duration(i) = Duration_of_Poll_transmission(i,n) + Offset_of_SPR_transmission(m) + ceiling(TXTIME(SPR(m)), aTSFResolution) **/
  return (GetDurationOfPollTransmission () + GetOffsetOfSprTransmission (m_polledStationsCount - 1) + m_sprFrameTxTime);
}

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
  hdr.SetDuration (GetPollFrameDuration ());

  Ptr<Packet> packet = Create<Packet> ();
  CtrlDmgPoll poll;
  m_responseOffset = GetResponseOffset ();
  poll.SetResponseOffset (m_responseOffset.GetMicroSeconds ());
  packet->AddHeader (poll);

  /* Transmit control frames directly without DCA + DCF Manager */
  SteerAntennaToward (to);
  TransmitControlFrameImmediately (packet, hdr);
}

void
DmgApWifiMac::SendGrantFrame (Mac48Address to, Time duration, DynamicAllocationInfoField &info, BF_Control_Field &bf)
{
  NS_LOG_FUNCTION (this << to << duration);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_CTL_DMG_GRANT);
  hdr.SetAddr1 (to);            //RA Field (MAC Address of the STA receiving SP Grant)
  hdr.SetAddr2 (GetAddress ()); //TA Field (MAC Address of the STA that has transmited the Grant frame)
  hdr.SetDsNotFrom ();
  hdr.SetDsTo ();
  hdr.SetNoOrder ();
  hdr.SetDuration (duration);

  Ptr<Packet> packet = Create<Packet> ();
  CtrlDMG_Grant grant;
  grant.SetDynamicAllocationInfo (info);
  grant.SetBFControl (bf);
  packet->AddHeader (grant);

  /* Transmit control frames directly without DCA + DCF Manager */
  SteerAntennaToward (to);
  TransmitControlFrameImmediately (packet, hdr);
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
  announceHdr.SetBeaconInterval (m_beaconInterval.GetMicroSeconds ());

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.unprotectedAction = WifiActionHeader::UNPROTECTED_DMG_ANNOUNCE;
  actionHdr.SetAction (WifiActionHeader::UNPROTECTED_DMG, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (announceHdr);
  packet->AddHeader (actionHdr);

  m_dmgAtiDca->Queue (packet, hdr);
}

/* Spatial Sharing and interference assessment Functions */

void
DmgApWifiMac::SendDirectionalChannelQualityRequest (Mac48Address to, uint16_t numOfRepts,
                                                    Ptr<DirectionalChannelQualityRequestElement> element)
{
  NS_LOG_FUNCTION (this << to);
  WifiMacHeader hdr;
  hdr.SetAction ();
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  RadioMeasurementRequest requestHdr;
  requestHdr.SetDialogToken (0);
  requestHdr.SetNumberOfRepetitions (numOfRepts);
  requestHdr.AddMeasurementRequestElement (element);

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.radioMeasurementAction = WifiActionHeader::RADIO_MEASUREMENT_REQUEST;
  actionHdr.SetAction (WifiActionHeader::RADIO_MEASUREMENT, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (requestHdr);
  packet->AddHeader (actionHdr);

  m_dca->Queue (packet, hdr);
}

void
DmgApWifiMac::TxOk (Ptr<const Packet> packet, const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this);
  if (m_currentAllocation == CBAP_ALLOCATION)
    {
      /* After transmitting a packet successfully, the PCP/AP returns to the quasi-omni receive mode */
      m_phy->GetDirectionalAntenna ()->SetInOmniReceivingMode ();
    }
  /* For association */
  if (hdr.IsAssocResp() && m_stationManager->IsWaitAssocTxOk (hdr.GetAddr1 ()))
    {
      NS_LOG_DEBUG ("associated with sta=" << hdr.GetAddr1 ());
      m_stationManager->RecordGotAssocTxOk (hdr.GetAddr1 ());
    }
  DmgWifiMac::TxOk (packet, hdr);
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
  multiband->SetChannelNumber (m_phy->GetChannelNumber ());
  multiband->SetBssID (GetAddress ());
  multiband->SetBeaconInterval (m_beaconInterval.GetMicroSeconds ());
  multiband->SetConnectionCapability (1);     /* AP */
  multiband->SetFstSessionTimeout (m_fstTimeout);
  return multiband;
}

/* Decentralized Clustering */
void
DmgApWifiMac::StartMonitoringBeaconSP (uint8_t beaconSPIndex)
{
  NS_LOG_FUNCTION (this << beaconSPIndex);
  m_beaconReceived = false;

  if (beaconSPIndex == m_clusterMaxMem - 1)
    {
      Time remainingMonitoringTime = m_channelMonitorTime - (Simulator::Now () - m_startedMonitoringChannel);
      if (remainingMonitoringTime > Seconds (0))
        {
          Time clusterTimeOffset;
          for (uint8_t n = 1; n < m_clusterMaxMem; n++)
            {
              clusterTimeOffset =  m_clusterTimeInterval * (n + 1);
              /* Schedule the beginning of BeaconSP */
              Simulator::Schedule (clusterTimeOffset, &DmgApWifiMac::StartMonitoringBeaconSP, this, n);
              Simulator::Schedule (clusterTimeOffset + m_clusterBeaconSPDuration, &DmgApWifiMac::EndMonitoringBeaconSP, this, n);
            }
        }
    }
}

void
DmgApWifiMac::EndMonitoringBeaconSP (uint8_t beaconSPIndex)
{
  NS_LOG_FUNCTION (this << beaconSPIndex << m_beaconReceived);
  if (!m_spStatus[beaconSPIndex])
    {
      m_spStatus[beaconSPIndex] = m_beaconReceived;
    }
}

void
DmgApWifiMac::EndChannelMonitoring (Mac48Address clusterID)
{
  NS_LOG_FUNCTION (this << clusterID);
  m_monitoringChannel = false;
  /* Search for empty BeaconSP */
  for (BEACON_SP_STATUS_MAPCI it = m_spStatus.begin (); it != m_spStatus.end (); it++)
    {
      if (it->second == false)
        {
          /* Join the cluster */
          NS_LOG_INFO ("DMG PCP/AP " << GetAddress () << " Joined ClusterID=" << clusterID
                       << ", Sending DMG Beacons in [" << m_selectedBeaconSP << "] BeaconSP");
          m_ClusterID = clusterID;
          m_clusterRole = PARTICIPATING;
          m_selectedBeaconSP = it->first;
          m_joinedCluster (m_ClusterID, m_selectedBeaconSP);
          break;
        }
    }
}

void
DmgApWifiMac::StartSynBeaconInterval (void)
{
  NS_LOG_FUNCTION (this);
  if (m_clusterRole == PARTICIPATING)
    {
      /* We joined cluster so we start DMG Beaconning in the specified BeaconSP */
      Simulator::Schedule (m_clusterTimeInterval * m_selectedBeaconSP, &DmgApWifiMac::StartBeaconInterval, this);
    }
  else
    {
      /* We keep schedulling SYN Beacon Interval until we find an empty BeaconSP and join cluster */
      Simulator::Schedule (m_beaconInterval, &DmgApWifiMac::StartSynBeaconInterval, this);
    }
}

void
DmgApWifiMac::Receive (Ptr<Packet> packet, const WifiMacHeader *hdr)
{
  NS_LOG_FUNCTION (this << packet << hdr);
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

      /* Check if we have received SSW frame during the current SSW-Slot */
      if (!m_receivedOneSSW)
        {
          m_receivedOneSSW = true;
          m_peerAbftStation = hdr->GetAddr2 ();
        }

      if (m_receivedOneSSW && (m_peerAbftStation == hdr->GetAddr2 ()) && m_isResponderTXSS)
        {
          CtrlDMG_SSW sswFrame;
          packet->RemoveHeader (sswFrame);

          DMG_SSW_Field ssw = sswFrame.GetSswField ();
          /* Map the antenna Tx configuration for the frame received by SLS of the DMG-STA */
          MapTxSnr (from, ssw.GetSectorID (), ssw.GetDMGAntennaID (), m_stationManager->GetRxSnr ());

          /* If we receive one SSW Frame at least, then we schedule SSW-FBCK frame */
          if (!m_sectorFeedbackSent[from])
            {
              m_sectorFeedbackSent[from] = true;

              /* Record the best TX antenna configuration reported by the SSW-FBCK Field */
              DMG_SSW_FBCK_Field sswFeedback = sswFrame.GetSswFeedbackField ();
              sswFeedback.IsPartOfISS (false);

              /* The Sector Sweep Frame contains feedback about the the best Tx Sector in the DMG-AP with the sending DMG-STA */
              ANTENNA_CONFIGURATION_TX antennaConfigTx = std::make_pair (sswFeedback.GetSector (), sswFeedback.GetDMGAntenna ());
              ANTENNA_CONFIGURATION_RX antennaConfigRx = std::make_pair (NO_ANTENNA_CONFIG, NO_ANTENNA_CONFIG);
              m_bestAntennaConfig[hdr->GetAddr2 ()] = std::make_pair (antennaConfigTx, antennaConfigRx);

              NS_LOG_INFO ("Best TX Antenna Sector Config by this DMG AP to DMG STA=" << from
                           << ": SectorID=" << uint32_t (antennaConfigTx.first)
                           << ", AntennaID=" << uint32_t (antennaConfigTx.second));

              /* Indicate this DMG-STA as waiting for Beam Refinement Phase */
              m_stationBrpMap[from] = true;

              Time sswFbckTime = GetSectorSweepDuration (ssw.GetCountDown ()) + GetMbifs ();
              NS_LOG_INFO ("Scheduled SSW-FBCK Frame to " << hdr->GetAddr2 () << " at " << Simulator::Now () + sswFbckTime);
              Simulator::Schedule (sswFbckTime, &DmgApWifiMac::SendSswFbckAfterRss, this, from);
            }
        }

      return;
    }
  else if (hdr->IsSprFrame ())
    {
      NS_LOG_INFO ("Received SPR frame from=" << from);

      CtrlDMG_SPR spr;
      packet->RemoveHeader (spr);

      /* Add the dynamic allocation info field in the SPR frame to the list */
      m_sprList.push_back (std::make_pair (spr.GetDynamicAllocationInfo (), spr.GetBFControl ()));

      return;
    }
  else if (hdr->IsDMGBeacon ())
    {
      if ((!m_monitoringChannel) && m_enableDecentralizedClustering && (m_clusterRole == NOT_PARTICIPATING))
        {
          NS_LOG_LOGIC ("Received DMG Beacon frame with BSSID=" << hdr->GetAddr1 ());

          ExtDMGBeacon beacon;
          packet->RemoveHeader (beacon);

          ExtDMGBeaconIntervalCtrlField beaconInterval = beacon.GetBeaconIntervalControlField ();
          /* Cluster Control Field */
          if (beaconInterval.IsCCPresent ())
            {
              ExtDMGParameters parameters = beacon.GetDMGParameters ();
              ExtDMGClusteringControlField cluster = beacon.GetClusterControlField ();
              cluster.SetDiscoveryMode (beaconInterval.IsDiscoveryMode ());
              if ((!parameters.Get_ECPAC_Policy_Enforced ()) && (cluster.GetClusterMemberRole () == SYNC_PCP_AP))
                {
                  /**
                   * A decentralized clustering enabled PCP/AP that receives a DMG Beacon frame with the ECPAC Policy
                   * Enforced subfield in the DMG Parameters field set to 0 from an S-PCP/S-AP on the channel the PCP/AP
                   * selects to establish a BSS shall monitor the channel for DMG Beacon transmissions during each Beacon SP
                   * for an interval of length at least aMinChannelTime.
                   */

                  /* Get the beginning of the S-PCP/S-AP */
                  ExtDMGBeaconIntervalCtrlField bi = beacon.GetBeaconIntervalControlField ();
                  Time abftDuration = bi.GetABFT_Length () * GetSectorSweepSlotTime (bi.GetFSS ());

                  /* DMG Operation Element */
                  Ptr<DmgOperationElement> operationElement
                      = StaticCast<DmgOperationElement> (beacon.GetInformationElement (IE_DMG_OPERATION));

                  /* Next DMG ATI Element */
                  Ptr<NextDmgAti> atiElement = StaticCast<NextDmgAti> (beacon.GetInformationElement (IE_NEXT_DMG_ATI));
                  Time atiDuration = MicroSeconds (atiElement->GetAtiDuration ());
                  Time btiDuration = MicroSeconds (operationElement->GetMinBHIDuration ()) - abftDuration - atiDuration - 2 * GetMbifs ();
                  m_biStartTime = MicroSeconds (beacon.GetTimestamp ()) + hdr->GetDuration () - btiDuration;
                  m_beaconInterval = MicroSeconds (beacon.GetBeaconIntervalUs ());

                  /* Schedule Beacon SPs */
                  Time clusterTimeOffset;
                  m_monitoringChannel = true;
                  m_clusterMaxMem = cluster.GetClusterMaxMem ();
                  m_clusterBeaconSPDuration = MicroSeconds (uint32_t (cluster.GetBeaconSpDuration ()) * 8);
                  m_clusterTimeInterval = (m_beaconInterval/m_clusterMaxMem);
                  m_startedMonitoringChannel = Simulator::Now ();

                  /* The first Beacon SP ia reserved for S-PCP/S-AP */
                  Time timeShift = (Simulator::Now () - m_biStartTime);
                  m_spStatus[0] = true;
                  for (uint8_t n = 1; n < m_clusterMaxMem; n++)
                    {
                      m_spStatus[n] = false;
                      clusterTimeOffset =  m_clusterTimeInterval * n - timeShift;
                      /* Schedule the beginning of BeaconSP */
                      Simulator::Schedule (clusterTimeOffset, &DmgApWifiMac::StartMonitoringBeaconSP, this, n);
                      Simulator::Schedule (clusterTimeOffset + m_clusterBeaconSPDuration, &DmgApWifiMac::EndMonitoringBeaconSP, this, n);
                    }

                  /* Schedule the beginning on the next BI of the S-PCP/S-AP */
                  Simulator::Schedule (m_beaconInterval - timeShift, &DmgApWifiMac::StartSynBeaconInterval, this);

                  /* Schedule the end of the monitoring period */
                  Simulator::Schedule (m_channelMonitorTime, &DmgApWifiMac::EndChannelMonitoring, this, cluster.GetClusterID ());
                }
            }
        }
      else if (m_monitoringChannel)
        {
          NS_LOG_LOGIC ("Received DMG Beacon frame during monitoring period with BSSID=" << hdr->GetAddr1 ());
          m_beaconReceived = true;
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

                  /** Record DMG STA Capabilities **/
                  /* Set AID of the station */
                  Ptr<DmgCapabilities> capabilities = StaticCast<DmgCapabilities> (infoMap[IE_DMG_CAPABILITIES]);
                  capabilities->SetAID (m_aidCounter & 0xFF);
                  m_associatedStationsInfoByAddress[from] = infoMap;
                  m_associatedStationsInfoByAid[m_aidCounter] = infoMap;
                  MapAidToMacAddress (m_aidCounter, hdr->GetAddr2 ());

                  /* Record MCS1-4 as mandatory modes for data communication */
                  AddMcsSupport (from, 1, 4);
                  /* Record SC MCS range */
                  AddMcsSupport (from, 5, capabilities->GetMaximumScTxMcs ());
                  /* Record OFDM MCS range */
                  if (capabilities->GetMaximumOfdmTxMcs () != 0)
                    {
                      AddMcsSupport (from, 13, capabilities->GetMaximumOfdmTxMcs ());
                    }
                  /* Record DMG Capabilities */
                  m_stationManager->AddStationDmgCapabilities (hdr->GetAddr2 (), capabilities);

                  /** Check Relay Capabilities **/
                  Ptr<RelayCapabilitiesElement> relayElement =
                      DynamicCast<RelayCapabilitiesElement> (assocReq.GetInformationElement (IE_RELAY_CAPABILITIES));

                  /* Check if the DMG STA supports RDS */
                  if ((relayElement != 0) && (relayElement->GetRelayCapabilitiesInfo ().GetRelaySupportability ()))
                    {
                      m_rdsList[m_aidCounter] = relayElement->GetRelayCapabilitiesInfo ();
                      NS_LOG_DEBUG ("Station=" << from << " with AID=" << m_aidCounter << " supports RDS operation");
                    }

                  /* Check if the DMG STA can participate in polling phase */
                  Ptr<StaAvailabilityElement> availabilityElement =
                      DynamicCast<StaAvailabilityElement> (assocReq.GetInformationElement (IE_STA_AVAILABILITY));
                  if (availabilityElement)
                    {
                      StaInfoField field = availabilityElement->GetStaInfoField ();
                      if (field.GetPollingPhase ())
                        {
                          m_pollStations.push_back (from);
                        }
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
                case WifiActionHeader::RADIO_MEASUREMENT:
                  switch (actionHdr.GetAction ().radioMeasurementAction)
                    {
                    case WifiActionHeader::RADIO_MEASUREMENT_REPORT:
                      {
                        RadioMeasurementReport reportHdr;
                        packet->RemoveHeader (reportHdr);
                        Ptr<DirectionalChannelQualityReportElement> elem =
                            DynamicCast<DirectionalChannelQualityReportElement> (reportHdr.GetListOfMeasurementReportElement ().at (0));

                        /* Report back the received measurements report */
                        m_qualityReportReceived (from, elem);

                        return;
                      }
                    default:
                      NS_FATAL_ERROR ("Unsupported Action frame received");
                      return;
                    }

                case WifiActionHeader::QOS:
                  switch (actionHdr.GetAction ().qos)
                    {
                    case WifiActionHeader::ADDTS_REQUEST:
                      {
                        DmgAddTSRequestFrame frame;
                        packet->RemoveHeader (frame);
                        /* Callback to the user, so can take decision */
                        m_addTsRequestReceived (hdr->GetAddr2 (), frame.GetDmgTspec ());
                        return;
                      }
                    case WifiActionHeader::DELTS:
                      {
                        DelTsFrame frame;
                        packet->RemoveHeader (frame);
                        /* Search for the allocation */
                        DmgAllocationInfo info = frame.GetDmgAllocationInfo ();
                        AllocationField allocation;
                        for(AllocationFieldList::iterator iter = m_allocationList.begin (); iter != m_allocationList.end ();)
                          {
                            allocation = (*iter);
                            if ((allocation.GetAllocationID () == info.GetAllocationID ()) &&
                                (allocation.GetSourceAid () == GetStationAid (hdr->GetAddr2 ())) &&
                                (allocation.GetDestinationAid () == info.GetDestinationAid ()))
                              {
                                iter = m_allocationList.erase (iter);
                                break;
                              }
                            else
                              {
                                ++iter;
                              }
                          }
                        return;
                      }
                    default:
                      packet->AddHeader (actionHdr);
                      DmgWifiMac::Receive (packet, hdr);
                      return;
                    }

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
                        NS_LOG_INFO ("A relay Link is teared down between: " <<
                                     "Source REDS AID=" << header.GetSourceAid () <<
                                     ", RDS AID=" << header.GetRelayAid () <<
                                     ", Destination REDS AID=" << header.GetDestinationAid ());
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
  m_abftDuration = m_ssSlotsPerABFT * GetSectorSweepSlotTime (m_ssFramesPerSlot);

  /* Generate Antenna Configuration Table */
  m_antennaConfigurationOffset = 0;
  for (uint8_t i = 1; i <= GetNumberOfAntennas (); i++)
    {
      for (uint8_t j = 1; j <= GetNumberOfSectors (); j++)
        {
          m_antennaConfigurationTable.push_back (std::make_pair (j, i));
        }
    }

  /* Initialize Upper layers */
  DmgWifiMac::DoInitialize ();

  /* Start Beacon Interval */
  if (m_allowBeaconing)
    {
      NS_LOG_DEBUG ("Starting DMG Access Point " << GetAddress () << " at time " << Simulator::Now ());
      Simulator::Schedule (m_beaconingDelay, &DmgApWifiMac::StartBeaconInterval, this);
    }
}

} // namespace ns3
