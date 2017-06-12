/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#include "ns3/address-utils.h"
#include "ns3/fatal-error.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

#include "ext-headers.h"
#include "mgt-headers.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ExtHeaders");

/**********************************************************
*  Channel Measurement Info field format (Figure 8-502h)
**********************************************************/

ExtChannelMeasurementInfo::ExtChannelMeasurementInfo ()
  : m_aid (0),
    m_snr (0),
    m_angle (0),
    m_recommend (0),
    m_reserved (0)
{
}

ExtChannelMeasurementInfo::~ExtChannelMeasurementInfo ()
{

}

uint32_t
ExtChannelMeasurementInfo::GetSerializedSize (void) const
{
  return 4;
}

void
ExtChannelMeasurementInfo::Print (std::ostream &os) const
{

}

Buffer::Iterator
ExtChannelMeasurementInfo::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  uint8_t buffer = 0;

  start.WriteU8 (m_aid);
  start.WriteU8 (m_snr);
  buffer |= m_angle & 0x7F;
  buffer |= (m_recommend & 0x1) << 7;
  start.WriteU8 (buffer);
  start.WriteU8 (m_reserved);

  return start;
}

Buffer::Iterator
ExtChannelMeasurementInfo::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  uint8_t buffer;

  m_aid = start.ReadU8 ();
  m_snr = start.ReadU8 ();
  buffer = start.ReadU8 ();
  m_angle = buffer & 0x7F;
  m_recommend = (buffer >> 1) & 0x1;
  m_reserved = start.ReadU8 ();

  return start;
}

void
ExtChannelMeasurementInfo::SetPeerStaAid (uint16_t aid)
{
  NS_LOG_FUNCTION (this << aid);
  m_aid = aid;
}

void
ExtChannelMeasurementInfo::SetSnr (uint8_t snr)
{
  NS_LOG_FUNCTION (this << snr);
  m_snr = snr;
}

void
ExtChannelMeasurementInfo::SetInternalAngle (uint8_t angle)
{
  NS_LOG_FUNCTION (this << angle);
  m_angle = angle;
}

void
ExtChannelMeasurementInfo::SetRecommendSubField (bool value)
{
  NS_LOG_FUNCTION (this << value);
  m_recommend = value;
}

void
ExtChannelMeasurementInfo::SetReserved (uint8_t reserved)
{
  NS_LOG_FUNCTION (this << reserved);
  m_reserved = reserved;
}

uint16_t
ExtChannelMeasurementInfo::GetPeerStaAid (void) const
{
  NS_LOG_FUNCTION (this);
  return m_aid;
}

uint8_t
ExtChannelMeasurementInfo::GetSnr (void) const
{
  NS_LOG_FUNCTION (this);
  return m_snr;
}

uint8_t
ExtChannelMeasurementInfo::GetInternalAngle (void) const
{
  NS_LOG_FUNCTION (this);
  return m_angle;
}

bool
ExtChannelMeasurementInfo::GetRecommendSubField (void) const
{
  NS_LOG_FUNCTION (this);
  return m_recommend;
}

uint8_t
ExtChannelMeasurementInfo::GetReserved (void) const
{
  NS_LOG_FUNCTION (this);
  return m_reserved;
}

/******************************************
*	DMG Parameters Field (8.4.1.46)
*******************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtDMGParameters);

ExtDMGParameters::ExtDMGParameters ()
{

}

ExtDMGParameters::~ExtDMGParameters ()
{

}

TypeId
ExtDMGParameters::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ExtDMGParameters")
    .SetParent <Header> ()
    .AddConstructor <ExtDMGParameters> ()
    ;
  return tid;
}

TypeId
ExtDMGParameters::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
ExtDMGParameters::GetSerializedSize (void) const
{
  return 1; // 1 bytes length.
}

void
ExtDMGParameters::Print (std::ostream &os) const
{
}

Buffer::Iterator
ExtDMGParameters::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  uint8_t buffer = 0;

  buffer |= m_bssType & 0x3;
  buffer |= ((m_cbapOnly & 0x1) << 2);
  buffer |= ((m_cbapSource & 0x1) << 3);
  buffer |= ((m_dmgPrivacy & 0x1) << 4);
  buffer |= ((m_ecpacPolicyEnforced & 0x1) << 5);
  buffer |= ((m_reserved & 0x3) << 6);

  start.WriteU8 (buffer);
  return start;
}

Buffer::Iterator
ExtDMGParameters::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  uint8_t buffer = start.ReadU8 ();

  m_bssType = static_cast<enum BSSType> (buffer & 0x3);
  m_cbapOnly = (buffer >> 2) & 0x1;
  m_cbapSource = (buffer >> 3) & 0x1;
  m_dmgPrivacy = (buffer >> 4) & 0x1;
  m_ecpacPolicyEnforced = (buffer >> 5) & 0x1;
  m_reserved = (buffer >> 6) & 0x3;

  return start;
}

/**
* Set the duration of the beacon frame.
*
* \param duration The duration of the beacon frame
*/
void
ExtDMGParameters::Set_BSS_Type (enum BSSType type)
{
  NS_LOG_FUNCTION (this << type);
  m_bssType = type;
}

void
ExtDMGParameters::Set_CBAP_Only (bool value)
{
  NS_LOG_FUNCTION (this << value);
  m_cbapOnly = value;
}

void
ExtDMGParameters::Set_CBAP_Source (bool value)
{
    NS_LOG_FUNCTION (this << value);
    m_cbapSource = value;
}

void
ExtDMGParameters::Set_DMG_Privacy (bool value)
{
    NS_LOG_FUNCTION (this << value);
    m_dmgPrivacy = value;
}

void
ExtDMGParameters::Set_ECPAC_Policy_Enforced (bool value)
{
  NS_LOG_FUNCTION (this << value);
  m_ecpacPolicyEnforced = value;
}

void
ExtDMGParameters::Set_Reserved (uint8_t value)
{
  NS_LOG_FUNCTION (this << value);
  m_reserved = value;
}

/**
* Return the Basic Service Set (BSS) Type.
*
* \return BSSType
*/
enum BSSType
ExtDMGParameters::Get_BSS_Type (void) const
{
  NS_LOG_FUNCTION (this);
  return m_bssType;
}

bool
ExtDMGParameters::Get_CBAP_Only (void) const
{
  NS_LOG_FUNCTION (this);
  return m_cbapOnly;
}

bool
ExtDMGParameters::Get_CBAP_Source (void) const
{
  NS_LOG_FUNCTION (this);
  return m_cbapSource;
}

bool
ExtDMGParameters::Get_DMG_Privacy (void) const
{
  NS_LOG_FUNCTION (this);
  return m_dmgPrivacy;
}

bool
ExtDMGParameters::Get_ECPAC_Policy_Enforced (void) const
{
  NS_LOG_FUNCTION (this);
  return m_ecpacPolicyEnforced;
}

uint8_t
ExtDMGParameters::Get_Reserved (void) const
{
  NS_LOG_FUNCTION (this);
  return m_reserved;
}

/******************************************
*   Beacon Interval Control Field (8-34b)
*******************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtDMGBeaconIntervalCtrlField);

ExtDMGBeaconIntervalCtrlField::ExtDMGBeaconIntervalCtrlField ()
  : m_ccPresent (false),
    m_discoveryMode (false),
    m_nextBeacon (1),
    m_ATI_Present (true),
    m_ABFT_Length (0),
    m_FSS (0),
    m_isResponderTXSS (false),
    m_next_ABFT (0),
    m_fragmentedTXSS (false),
    m_TXSS_Span (0),
    m_N_BI (0),
    m_ABFT_Count (0),
    m_N_ABFT_Ant (0),
    m_pcpAssociationReady (false)
{
}

ExtDMGBeaconIntervalCtrlField::~ExtDMGBeaconIntervalCtrlField ()
{
}

TypeId
ExtDMGBeaconIntervalCtrlField::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ExtDMGBeaconIntervalCtrlField")
    .SetParent<Header> ()
    .AddConstructor<ExtDMGBeaconIntervalCtrlField> ()
  ;
  return tid;
}

TypeId
ExtDMGBeaconIntervalCtrlField::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
ExtDMGBeaconIntervalCtrlField::GetSerializedSize (void) const
{
  return 6; // 6 bytes length.
}

void
ExtDMGBeaconIntervalCtrlField::Print (std::ostream &os) const
{

}

Buffer::Iterator
ExtDMGBeaconIntervalCtrlField::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  uint32_t ctrl1 = 0;
  uint16_t ctrl2 = 0;

  ctrl1 |= m_ccPresent & 0x1;
  ctrl1 |= ((m_discoveryMode & 0x1) << 1);
  ctrl1 |= ((m_nextBeacon & 0xF) << 2);
  ctrl1 |= ((m_ATI_Present & 0x1) << 6);
  ctrl1 |= ((m_ABFT_Length & 0x7) << 7);
  ctrl1 |= ((m_FSS & 0xF) << 10);
  ctrl1 |= ((m_isResponderTXSS & 0x1) << 14);
  ctrl1 |= ((m_next_ABFT & 0xF) << 15);
  ctrl1 |= ((m_fragmentedTXSS & 0x1) << 19);
  ctrl1 |= ((m_TXSS_Span & 0x3F) << 20);
  ctrl1 |= ((m_N_BI & 0xF) << 27);
  ctrl1 |= ((m_ABFT_Count & 0x1) << 31);

  ctrl2 |= (m_ABFT_Count >> 1) & 0x1F;
  ctrl2 |= ((m_N_ABFT_Ant & 0x3F) << 5);
  ctrl2 |= ((m_pcpAssociationReady & 0x1) << 11);

  start.WriteHtolsbU32 (ctrl1);
  start.WriteHtolsbU16 (ctrl2);

  return start;
}

Buffer::Iterator
ExtDMGBeaconIntervalCtrlField::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  uint32_t ctrl1 = start.ReadLsbtohU32();
  uint16_t ctrl2 = start.ReadLsbtohU16();

  m_ccPresent = ctrl1 & 0x1;
  m_discoveryMode = (ctrl1 >> 1) & 0x1;
  m_nextBeacon = (ctrl1 >> 2) & 0xF;
  m_ATI_Present = (ctrl1 >> 6) & 0x1;
  m_ABFT_Length = (ctrl1 >> 7) & 0x7;
  m_FSS = (ctrl1 >> 10) & 0xF;
  m_isResponderTXSS = (ctrl1 >> 14) & 0x1;
  m_next_ABFT = (ctrl1 >> 15) & 0xF;
  m_fragmentedTXSS = (ctrl1 >> 19) & 0x1;
  m_TXSS_Span = (ctrl1 >> 20) & 0x3F;
  m_N_BI = (ctrl1 >> 27) & 0xF;
  m_ABFT_Count =  ((ctrl1 >> 31) & 0x1) | ((ctrl2 << 1) & 0x3E);
  m_N_ABFT_Ant = (ctrl2 >> 5) & 0x3F;
  m_pcpAssociationReady = (ctrl2 >> 11) & 0x1;

  return start;
}

void
ExtDMGBeaconIntervalCtrlField::SetCCPresent (bool value)
{
  m_ccPresent = value;
}

void
ExtDMGBeaconIntervalCtrlField::SetDiscoveryMode (bool value)
{
  m_discoveryMode = value;
}

void
ExtDMGBeaconIntervalCtrlField::SetNextBeacon (uint8_t value)
{
  NS_ASSERT ((value >= 0) && (value <= 15));
  m_nextBeacon = value;
}

void
ExtDMGBeaconIntervalCtrlField::SetATIPresent (bool value)
{
  m_ATI_Present = value;
}

void
ExtDMGBeaconIntervalCtrlField::SetABFT_Length (uint8_t length)
{
  NS_ASSERT ((1 <= length) && (length <= 8));
  m_ABFT_Length = length - 1;
}

void
ExtDMGBeaconIntervalCtrlField::SetFSS (uint8_t number)
{
  NS_ASSERT ((1 <= number) && (number <= 16));
  m_FSS = number - 1;
}

void
ExtDMGBeaconIntervalCtrlField::SetIsResponderTXSS (bool value)
{
  m_isResponderTXSS = value;
}

void
ExtDMGBeaconIntervalCtrlField::SetNextABFT (uint8_t value)
{
  m_next_ABFT = value;
}

void
ExtDMGBeaconIntervalCtrlField::SetFragmentedTXSS (bool value)
{
  m_fragmentedTXSS = value;
}

void
ExtDMGBeaconIntervalCtrlField::SetTXSS_Span (uint8_t value)
{
  m_TXSS_Span = value;
}

void
ExtDMGBeaconIntervalCtrlField::SetN_BI (uint8_t value)
{
  m_N_BI = value;
}

void
ExtDMGBeaconIntervalCtrlField::SetABFT_Count (uint8_t value)
{
  m_ABFT_Count = value;
}

void
ExtDMGBeaconIntervalCtrlField::SetN_ABFT_Ant (uint8_t value)
{
  m_N_ABFT_Ant = value;
}

void
ExtDMGBeaconIntervalCtrlField::SetPCPAssoicationReady (bool value)
{
  m_pcpAssociationReady = value;
}

bool
ExtDMGBeaconIntervalCtrlField::IsCCPresent (void) const
{
  return m_ccPresent;
}

bool
ExtDMGBeaconIntervalCtrlField::IsDiscoveryMode (void) const
{

  return m_discoveryMode;
}

uint8_t
ExtDMGBeaconIntervalCtrlField::GetNextBeacon (void) const
{
  return m_nextBeacon;
}

bool
ExtDMGBeaconIntervalCtrlField::IsATIPresent (void) const
{
  return m_ATI_Present;
}

uint8_t
ExtDMGBeaconIntervalCtrlField::GetABFT_Length (void) const
{
  return m_ABFT_Length + 1;
}

uint8_t
ExtDMGBeaconIntervalCtrlField::GetFSS (void) const
{
  return m_FSS + 1;
}

bool
ExtDMGBeaconIntervalCtrlField::IsResponderTXSS (void) const
{
  return m_isResponderTXSS;
}

uint8_t
ExtDMGBeaconIntervalCtrlField::GetNextABFT (void) const
{
  return m_next_ABFT;
}

bool
ExtDMGBeaconIntervalCtrlField::GetFragmentedTXSS (void) const
{
  return m_fragmentedTXSS;
}

uint8_t
ExtDMGBeaconIntervalCtrlField::GetTXSS_Span (void) const
{
  return m_TXSS_Span;
}

uint8_t
ExtDMGBeaconIntervalCtrlField::GetN_BI (void) const
{
  return m_N_BI;
}

uint8_t
ExtDMGBeaconIntervalCtrlField::GetABFT_Count (void) const
{
  return m_ABFT_Count;
}

uint8_t
ExtDMGBeaconIntervalCtrlField::GetN_ABFT_Ant (void) const
{
  return m_N_ABFT_Ant;
}

bool
ExtDMGBeaconIntervalCtrlField::GetPCPAssoicationReady (void) const
{
  return m_pcpAssociationReady;
}

/******************************************
*	   DMG Beacon (8.3.4.1)
*******************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtDMGBeacon);

ExtDMGBeacon::ExtDMGBeacon ()
  : m_timestamp (0),
    m_beaconInterval (0)
{
}

ExtDMGBeacon::~ExtDMGBeacon ()
{
}

TypeId
ExtDMGBeacon::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ExtDMGBeacon")
    .SetParent<Header> ()
    .AddConstructor<ExtDMGBeacon> ()
  ;
  return tid;
}

TypeId
ExtDMGBeacon::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
ExtDMGBeacon::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 8;                                          // Timestamp (See 8.4.1.10)
  size += m_ssw.GetSerializedSize ();                 // Sector Sweep (See 8.4a.1)
  size += 2;                                          // Beacon Interval (See 8.4.1.3)
  size += m_beaconIntervalCtrl.GetSerializedSize ();  // Beacon Interval Control (See 8.4.1.3)
  size += m_dmgParameters.GetSerializedSize ();       // DMG Parameters (See 8.4.1.46)
  if (m_beaconIntervalCtrl.IsCCPresent ())            // Cluster Control Information
    {
      size += m_cluster.GetSerializedSize ();
    }
  size += m_ssid.GetSerializedSize ();
  size += GetInformationElementsSerializedSize ();
  return size;
}

void
ExtDMGBeacon::Print (std::ostream &os) const
{

}

void
ExtDMGBeacon::Serialize (Buffer::Iterator start) const
{
  /* Fixed Parameters */
  // 1. Timestamp.
  // 2. Sector Sweep.
  // 3. Beacon Interval.
  // 4. Beacon Interval Control.
  // 5. DMG Parameters.
  /* Other Information Elements */
  Buffer::Iterator i = start;

  i.WriteHtolsbU64 (Simulator::Now ().GetMicroSeconds ());
  i = m_ssw.Serialize (i);
  i.WriteHtolsbU16 (m_beaconInterval / 1024);
  i = m_beaconIntervalCtrl.Serialize (i);
  i = m_dmgParameters.Serialize (i);
  if (m_beaconIntervalCtrl.IsCCPresent ())
    {
      i = m_cluster.Serialize (i);
    }
  i = m_ssid.Serialize (i);
  i = SerializeInformationElements (i);
}

uint32_t
ExtDMGBeacon::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  m_timestamp = i.ReadLsbtohU64 ();
  i = m_ssw.Deserialize (i);
  m_beaconInterval = i.ReadLsbtohU16 ();
  m_beaconInterval *= 1024;
  i = m_beaconIntervalCtrl.Deserialize (i);
  i = m_dmgParameters.Deserialize (i);
  if (m_beaconIntervalCtrl.IsCCPresent ())
    {
      i = m_cluster.Deserialize (i);
    }
  i = m_ssid.Deserialize (i);
  i = DeserializeInformationElements (i);

  return i.GetDistanceFrom (start);
}

void
ExtDMGBeacon::SetBSSID (Mac48Address bssid)
{
  m_bssid = bssid;
}

void
ExtDMGBeacon::SetTimestamp (uint64_t timestamp)
{
  m_timestamp = timestamp;
}

void
ExtDMGBeacon::SetSSWField (DMG_SSW_Field &ssw)
{
  m_ssw = ssw;
}

void
ExtDMGBeacon::SetBeaconIntervalUs (uint64_t interval)
{
  NS_LOG_FUNCTION (this << interval);
  m_beaconInterval = interval;
}

void
ExtDMGBeacon::SetBeaconIntervalControlField (ExtDMGBeaconIntervalCtrlField &ctrl)
{
  NS_LOG_FUNCTION (this << &ctrl);
  m_beaconIntervalCtrl = ctrl;
}

void
ExtDMGBeacon::SetBeaconIntervalControlField (ExtDMGParameters &parameters)
{
  NS_LOG_FUNCTION (this << &parameters);
  m_dmgParameters = parameters;
}

void
ExtDMGBeacon::SetDMGParameters (ExtDMGParameters &parameters)
{
  NS_LOG_FUNCTION (this << &parameters);
  m_dmgParameters = parameters;
}

void
ExtDMGBeacon::SetClusterControlField (ExtDMGClusteringControlField &cluster)
{
  NS_LOG_FUNCTION (this << &cluster);
  m_cluster = cluster;
}

void
ExtDMGBeacon::SetSsid (Ssid ssid)
{
  m_ssid = ssid;
}

Mac48Address
ExtDMGBeacon::GetBSSID (void) const
{
  NS_LOG_FUNCTION (this);
  return m_bssid;
}

uint64_t
ExtDMGBeacon::GetTimestamp (void) const
{
  NS_LOG_FUNCTION (this);
  return m_timestamp;
}

DMG_SSW_Field
ExtDMGBeacon::GetSSWField (void) const
{
  NS_LOG_FUNCTION (this);
  return m_ssw;
}

uint64_t
ExtDMGBeacon::GetBeaconIntervalUs (void) const
{
  NS_LOG_FUNCTION (this);
  return m_beaconInterval;
}

ExtDMGBeaconIntervalCtrlField
ExtDMGBeacon::GetBeaconIntervalControlField (void) const
{
  NS_LOG_FUNCTION (this);
  return m_beaconIntervalCtrl;
}

ExtDMGParameters
ExtDMGBeacon::GetDMGParameters (void) const
{
  NS_LOG_FUNCTION (this);
  return m_dmgParameters;
}

ExtDMGClusteringControlField
ExtDMGBeacon::GetClusterControlField (void) const
{
  NS_LOG_FUNCTION (this);
  return m_cluster;
}

Ssid
ExtDMGBeacon::GetSsid (void) const
{
  return m_ssid;
}

} // namespace ns3
