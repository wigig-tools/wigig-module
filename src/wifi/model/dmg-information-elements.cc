/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */

#include "ns3/address-utils.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "dmg-information-elements.h"
#include <cmath>

NS_LOG_COMPONENT_DEFINE ("DmgInformationElements");

namespace ns3 {

/***************************************************
*      Measurement Request Element (8.4.2.23)
****************************************************/

MeasurementRequestElement::MeasurementRequestElement ()
  : m_measurementToken (0),
    m_measurementRequestMode (0),
    m_measurementType (BASIC_REQUEST)
{
}

WifiInformationElementId
MeasurementRequestElement::ElementId () const
{
  return IE_MEASUREMENT_REQUEST;
}

uint8_t
MeasurementRequestElement::GetInformationFieldSize () const
{
  return 3;
}

void
MeasurementRequestElement::SetMeasurementToken (uint8_t token)
{
  m_measurementToken = token;
}

void
MeasurementRequestElement::SetMeasurementRequestMode (bool parallel, bool enable, bool request, bool report, bool durationMandatory)
{
  m_measurementRequestMode = 0;
  m_measurementRequestMode |= parallel & 0x1;
  m_measurementRequestMode |= (enable & 0x1) << 1;
  m_measurementRequestMode |= (request & 0x1) << 2;
  m_measurementRequestMode |= (report & 0x1) << 3;
  m_measurementRequestMode |= (durationMandatory & 0x1) << 4;
}

void
MeasurementRequestElement::SetMeasurementType (MeasurementType type)
{
  m_measurementType = type;
}

uint8_t
MeasurementRequestElement::GetMeasurementToken (void) const
{
  return m_measurementToken;
}

bool
MeasurementRequestElement::IsParallelMode (void) const
{
  bool value = m_measurementRequestMode & 0x1;
  return value;
}

bool
MeasurementRequestElement::IsEnableMode (void) const
{
  bool value = (m_measurementRequestMode >> 1) & 0x1;
  return value;
}

bool
MeasurementRequestElement::IsRequestMode (void) const
{
  bool value = (m_measurementRequestMode >> 2) & 0x1;
  return value;
}

bool
MeasurementRequestElement::IsReportMode (void) const
{
  bool value = (m_measurementRequestMode >> 3) & 0x1;
  return value;
}

bool
MeasurementRequestElement::IsDurationMandatory (void) const
{
  bool value = (m_measurementRequestMode >> 4) & 0x1;
  return value;
}

MeasurementType
MeasurementRequestElement::GetMeasurementType (void) const
{
  return m_measurementType;
}

/***************************************************
* Directional Channel Quality Request (8.4.2.23.16)
****************************************************/

DirectionalChannelQualityRequestElement::DirectionalChannelQualityRequestElement ()
  : m_operatingClass (0),
    m_channelNumber (0),
    m_aid (0),
    m_measurementStartTime (0),
    m_measurementDuration (0),
    m_numberOfTimeBlocks (0)
{
}

uint8_t
DirectionalChannelQualityRequestElement::GetInformationFieldSize () const
{
  return MeasurementRequestElement::GetInformationFieldSize () + 16;
}

void
DirectionalChannelQualityRequestElement::SerializeInformationField (Buffer::Iterator start) const
{
  /* Measurement Request Fields */
  start.WriteU8 (m_measurementToken);
  start.WriteU8 (m_measurementRequestMode);
  start.WriteU8 (static_cast<uint8_t> (m_measurementType));
  /* Directional Channel Quality Request Element Fields */
  start.WriteU8 (m_operatingClass);
  start.WriteU8 (m_channelNumber);
  start.WriteU8 (m_aid);
  start.WriteU8 (m_reserved);
  start.WriteU8 (static_cast<uint8_t> (m_measurementMethod));
  start.WriteHtolsbU64 (m_measurementStartTime);
  start.WriteHtolsbU16 (m_measurementDuration);
  start.WriteU8 (m_numberOfTimeBlocks);
}

uint8_t
DirectionalChannelQualityRequestElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;
  /* Measurement Request Fields */
  m_measurementToken = i.ReadU8 ();
  m_measurementRequestMode = i.ReadU8 ();
  m_measurementType = static_cast<MeasurementType> (i.ReadU8 ());
  /* Directional Channel Quality Request Element Fields */
  m_operatingClass = i.ReadU8 ();
  m_channelNumber = i.ReadU8 ();
  m_aid = i.ReadU8 ();
  m_reserved = i.ReadU8 ();
  m_measurementMethod = static_cast<MeasurementMethod> (i.ReadU8 ());
  m_measurementStartTime = i.ReadLsbtohU64 ();
  m_measurementDuration = i.ReadLsbtohU16 ();
  m_numberOfTimeBlocks = i.ReadU8 ();
  return length;
}

void
DirectionalChannelQualityRequestElement::SetOperatingClass (uint8_t oclass)
{
  m_operatingClass = oclass;
}

void
DirectionalChannelQualityRequestElement::SetChannelNumber (uint8_t number)
{
  m_channelNumber = number;
}

void
DirectionalChannelQualityRequestElement::SetAid (uint8_t aid)
{
  m_aid = aid;
}

void
DirectionalChannelQualityRequestElement::SetReservedField (uint8_t field)
{
  m_reserved = field;
}

void
DirectionalChannelQualityRequestElement::SetMeasurementMethod (MeasurementMethod method)
{
  m_measurementMethod = method;
}

void
DirectionalChannelQualityRequestElement::SetMeasurementStartTime (uint64_t startTime)
{
  m_measurementStartTime = startTime;
}

void
DirectionalChannelQualityRequestElement::SetMeasurementDuration (uint16_t duration)
{
  m_measurementDuration = duration;
}

void
DirectionalChannelQualityRequestElement::SetNumberOfTimeBlocks (uint8_t blocks)
{
  m_numberOfTimeBlocks = blocks;
}

uint8_t
DirectionalChannelQualityRequestElement::GetOperatingClass (void) const
{
  return m_operatingClass;
}

uint8_t
DirectionalChannelQualityRequestElement::GetChannelNumber (void) const
{
  return m_channelNumber;
}

uint8_t
DirectionalChannelQualityRequestElement::GetAid (void) const
{
  return m_aid;
}

uint8_t
DirectionalChannelQualityRequestElement::GetReservedField (void) const
{
  return m_reserved;
}

MeasurementMethod
DirectionalChannelQualityRequestElement::GetMeasurementMethod (void) const
{
  return m_measurementMethod;
}

uint64_t
DirectionalChannelQualityRequestElement::GetMeasurementStartTime (void) const
{
  return m_measurementStartTime;
}

uint16_t
DirectionalChannelQualityRequestElement::GetMeasurementDuration (void) const
{
  return m_measurementDuration;
}

uint8_t
DirectionalChannelQualityRequestElement::GetNumberOfTimeBlocks (void) const
{
  return m_numberOfTimeBlocks;
}

ATTRIBUTE_HELPER_CPP (DirectionalChannelQualityRequestElement);

std::ostream &
operator << (std::ostream &os, const DirectionalChannelQualityRequestElement &element)
{
  return os;
}

std::istream &
operator >> (std::istream &is, DirectionalChannelQualityRequestElement &element)
{
  return is;
}

/***************************************************
*      Measurement Report Element (8.4.2.24)
****************************************************/

MeasurementReportElement::MeasurementReportElement ()
  : m_measurementToken (0),
    m_measurementReportMode (0),
    m_measurementType (BASIC_REQUEST)
{
}

WifiInformationElementId
MeasurementReportElement::ElementId () const
{
  return IE_MEASUREMENT_REPORT;
}

uint8_t
MeasurementReportElement::GetInformationFieldSize () const
{
  return 3;
}

void
MeasurementReportElement::SetMeasurementToken (uint8_t token)
{
  m_measurementToken = token;
}

void
MeasurementReportElement::SetMeasurementReportMode (bool late, bool incapable, bool refused)
{
  m_measurementReportMode = 0;
  m_measurementReportMode |= late & 0x1;
  m_measurementReportMode |= (incapable & 0x1) << 1;
  m_measurementReportMode |= (refused & 0x1) << 2;
}

void
MeasurementReportElement::SetMeasurementType (MeasurementType type)
{
  m_measurementType = type;
}

uint8_t
MeasurementReportElement::GetMeasurementToken (void) const
{
  return m_measurementToken;
}

bool
MeasurementReportElement::IsLateMode (void) const
{
  return (m_measurementReportMode & 0x1);
}

bool
MeasurementReportElement::IsIncapableMode (void) const
{
  return ((m_measurementReportMode >> 1) & 0x1);
}

bool
MeasurementReportElement::IsRefusedMode (void) const
{
  return ((m_measurementReportMode >> 2) & 0x1);
}

MeasurementType
MeasurementReportElement::GetMeasurementType (void) const
{
  return m_measurementType;
}

/***************************************************
* Directional Channel Quality Report (8.4.2.24.15)
****************************************************/

DirectionalChannelQualityReportElement::DirectionalChannelQualityReportElement ()
  : m_operatingClass (0),
    m_channelNumber (0),
    m_aid (0),
    m_measurementMethod (0),
    m_measurementStartTime (0),
    m_measurementDuration (0),
    m_numberOfTimeBlocks (0)
{
}

uint8_t
DirectionalChannelQualityReportElement::GetInformationFieldSize () const
{
  return MeasurementReportElement::GetInformationFieldSize () + 16 + m_numberOfTimeBlocks;
}

void
DirectionalChannelQualityReportElement::SerializeInformationField (Buffer::Iterator start) const
{
  /* Measurement Report Fields */
  start.WriteU8 (m_measurementToken);
  start.WriteU8 (m_measurementReportMode);
  start.WriteU8 (static_cast<uint8_t> (m_measurementType));
  /* Directional Channel Quality Request Element Fields */
  start.WriteU8 (m_operatingClass);
  start.WriteU8 (m_channelNumber);
  start.WriteU8 (m_aid);
  start.WriteU8 (m_reserved);
  start.WriteU8 (m_measurementMethod);
  start.WriteHtolsbU64 (m_measurementStartTime);
  start.WriteHtolsbU16 (m_measurementDuration);
  start.WriteU8 (m_numberOfTimeBlocks);
  for (TimeBlockMeasurementListCI it = m_measurementList.begin (); it != m_measurementList.end (); it++)
    {
      start.WriteU8 ((*it));
    }
}

uint8_t
DirectionalChannelQualityReportElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;
  /* Measurement Request Fields */
  m_measurementToken = i.ReadU8 ();
  m_measurementReportMode = i.ReadU8 ();
  m_measurementType = static_cast<MeasurementType> (i.ReadU8 ());
  /* Directional Channel Quality Report Element Fields */
  m_operatingClass = i.ReadU8 ();
  m_channelNumber = i.ReadU8 ();
  m_aid = i.ReadU8 ();
  m_reserved = i.ReadU8 ();
  m_measurementMethod = i.ReadU8 ();
  m_measurementStartTime = i.ReadLsbtohU64 ();
  m_measurementDuration = i.ReadLsbtohU16 ();
  m_numberOfTimeBlocks = i.ReadU8 ();
  TimeBlockMeasurement measurement;
  for (uint8_t j = 0; j < m_numberOfTimeBlocks; j++)
    {
      measurement = i.ReadU8 ();
      m_measurementList.push_back (measurement);
    }
  return length;
}

void
DirectionalChannelQualityReportElement::SetOperatingClass (uint8_t oclass)
{
  m_operatingClass = oclass;
}

void
DirectionalChannelQualityReportElement::SetChannelNumber (uint8_t number)
{
  m_channelNumber = number;
}

void
DirectionalChannelQualityReportElement::SetAid (uint8_t aid)
{
  m_aid = aid;
}

void
DirectionalChannelQualityReportElement::SetReservedField (uint8_t field)
{
  m_reserved = field;
}

void
DirectionalChannelQualityReportElement::SetMeasurementMethod (uint8_t method)
{
  m_measurementMethod = method;
}

void
DirectionalChannelQualityReportElement::SetMeasurementStartTime (uint64_t startTime)
{
  m_measurementStartTime = startTime;
}

void
DirectionalChannelQualityReportElement::SetMeasurementDuration (uint16_t duration)
{
  m_measurementDuration = duration;
}

void
DirectionalChannelQualityReportElement::SetNumberOfTimeBlocks (uint8_t blocks)
{
  m_numberOfTimeBlocks = blocks;
}

void
DirectionalChannelQualityReportElement::AddTimeBlockMeasurement (TimeBlockMeasurement measurement)
{
  m_measurementList.push_back (measurement);
}

uint8_t
DirectionalChannelQualityReportElement::GetOperatingClass (void) const
{
  return m_operatingClass;
}

uint8_t
DirectionalChannelQualityReportElement::GetChannelNumber (void) const
{
  return m_channelNumber;
}

uint8_t
DirectionalChannelQualityReportElement::GetAid (void) const
{
  return m_aid;
}

uint8_t
DirectionalChannelQualityReportElement::GetReservedField (void) const
{
  return m_reserved;
}

uint8_t
DirectionalChannelQualityReportElement::GetMeasurementMethod (void) const
{
  return m_measurementMethod;
}

uint64_t
DirectionalChannelQualityReportElement::GetMeasurementStartTime (void) const
{
  return m_measurementStartTime;
}

uint16_t
DirectionalChannelQualityReportElement::GetMeasurementDuration (void) const
{
  return m_measurementDuration;
}

uint8_t
DirectionalChannelQualityReportElement::GetNumberOfTimeBlocks (void) const
{
  return m_numberOfTimeBlocks;
}

TimeBlockMeasurementList
DirectionalChannelQualityReportElement::GetTimeBlockMeasurementList (void) const
{
  return m_measurementList;
}

ATTRIBUTE_HELPER_CPP (DirectionalChannelQualityReportElement);

std::ostream &
operator << (std::ostream &os, const DirectionalChannelQualityReportElement &element)
{
  return os;
}

std::istream &
operator >> (std::istream &is, DirectionalChannelQualityReportElement &element)
{
  return is;
}

/***************************************************
*             Request Element 8.4.2.51
****************************************************/

RequestElement::RequestElement ()
{
}

WifiInformationElementId
RequestElement::ElementId () const
{
  return IE_REQUEST;
}

uint8_t
RequestElement::GetInformationFieldSize () const
{
  return m_list.size ();
}

void
RequestElement::SerializeInformationField (Buffer::Iterator start) const
{
  for (WifiInformationElementIdList::const_iterator i = m_list.begin (); i != m_list.end (); i++)
    {
      start.WriteU8 (i->first);
    }
}

uint8_t
RequestElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  WifiInformationElementId id;
  while (!start.IsEnd ())
    {
      id = start.ReadU8 (); 
      m_list.push_back (std::make_pair(id,0));

    }
  return length;
}

void
RequestElement::AddRequestElementID (WifiInfoElementId id)
{
  m_list.push_back (id);
}

void
RequestElement::AddListOfRequestedElemetID (WifiInformationElementIdList &list)
{
  m_list = list;
}

WifiInformationElementIdList
RequestElement::GetWifiInformationElementIdList (void) const
{
  return m_list;
}

size_t
RequestElement::GetNumberOfRequestedIEs (void) const
{
  return m_list.size ();
}

ATTRIBUTE_HELPER_CPP (RequestElement);

std::ostream &
operator << (std::ostream &os, const RequestElement &element)
{
  WifiInformationElementIdList list = element.GetWifiInformationElementIdList ();
  for (WifiInformationElementIdList::const_iterator i = list.begin (); i != list.end () - 1; i++)
    {
      os << i->first << "|";
    }
  os << list[list.size () - 1].first;
  return os;
}

std::istream &
operator >> (std::istream &is, RequestElement &element)
{
  WifiInformationElementId c1;
  is >> c1;

  element.AddRequestElementID (std::make_pair(c1,0));

  return is;
}

/***************************************************
*             TSPEC Element (8.4.2.32)
****************************************************/

TspecElement::TspecElement ()
  : m_trafficType (TRAFFIC_TYPE_PERIODIC_TRAFFIC_PATTERN),
    m_tsid (0),
    m_dataDirection (DATA_DIRECTION_UPLINK),
    m_accessPolicy (ACCESS_POLICY_RESERVED),
    m_aggregation (0),
    m_apsd (0),
    m_userPriority (0),
    m_tsInfoAckPolicy (ACK_POLICY_RESERVED),
    m_schedule (0),
    m_nominalMsduSize (0),
    m_maximumMsduSize (0),
    m_minimumServiceInterval (0),
    m_maximumServiceInterval (0),
    m_inactivityInterval (0),
    m_suspensionInterval (0),
    m_serviceStartTime (0),
    m_minimumDateRate (0),
    m_meanDateRate (0),
    m_peakDateRate (0),
    m_burstSize (0),
    m_delayBound (0),
    m_minimumPhyRate (0),
    m_surplusBandwidthAllowance (0),
    m_mediumTime (0),
    m_allocationID (0),
    m_allocationDirection (ALLOCATION_DIRECTION_SOURCE),
    m_amsduSubframe (0),
    m_reliabilityIndex (0)
{
}

WifiInformationElementId
TspecElement::ElementId () const
{
  return IE_TSPEC;
}

uint8_t
TspecElement::GetInformationFieldSize () const
{
  return 57;
}

void
TspecElement::SerializeInformationField (Buffer::Iterator start) const
{
  uint16_t tsInfoField1 = 0;
  uint8_t tsInfoField2;
  tsInfoField1 |= m_trafficType & 0x1;
  tsInfoField1 |= (m_tsid  & 0xF) << 1;
  tsInfoField1 |= (m_dataDirection & 0x3) << 5;
  tsInfoField1 |= (m_accessPolicy & 0x3) << 7;
  tsInfoField1 |= (m_aggregation & 0x1) << 9;
  tsInfoField1 |= (m_apsd & 0x1) << 10;
  tsInfoField1 |= (m_userPriority & 0x7) << 11;
  tsInfoField1 |= (m_tsInfoAckPolicy & 0x3) << 14;
  tsInfoField2 = m_schedule & 0x1;

  start.WriteHtolsbU16 (tsInfoField1);
  start.WriteU8 (tsInfoField2);
  start.WriteHtolsbU16 (m_nominalMsduSize);
  start.WriteHtolsbU16 (m_maximumMsduSize);
  start.WriteHtolsbU32 (m_minimumServiceInterval);
  start.WriteHtolsbU32 (m_maximumServiceInterval);
  start.WriteHtolsbU32 (m_inactivityInterval);
  start.WriteHtolsbU32 (m_suspensionInterval);
  start.WriteHtolsbU32 (m_serviceStartTime);
  start.WriteHtolsbU32 (m_minimumDateRate);
  start.WriteHtolsbU32 (m_meanDateRate);
  start.WriteHtolsbU32 (m_peakDateRate);
  start.WriteHtolsbU32 (m_burstSize);
  start.WriteHtolsbU32 (m_delayBound);
  start.WriteHtolsbU32 (m_minimumPhyRate);
  start.WriteHtolsbU16 (m_surplusBandwidthAllowance);
  start.WriteHtolsbU16 (m_mediumTime);
  start.WriteHtolsbU16 (GetDmgAttributesField ());
}

uint8_t
TspecElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;
  uint16_t tsInfoField1 = 0;
  uint8_t tsInfoField2;
  tsInfoField1 = i.ReadLsbtohU16 ();
  m_trafficType = tsInfoField1 & 0x1;
  m_tsid = (tsInfoField1 >> 1) & 0xF;
  m_dataDirection = (tsInfoField1 >> 5) & 0x3;
  m_accessPolicy = (tsInfoField1 >> 7) & 0x3;
  m_aggregation = (tsInfoField1 >> 9) & 0x1;
  m_apsd = (tsInfoField1 >> 10) & 0x1;
  m_userPriority = (tsInfoField1 >> 11) & 0x7;
  m_tsInfoAckPolicy = (tsInfoField1 >> 14) & 0x3;
  tsInfoField2 = i.ReadU8 ();
  m_schedule = tsInfoField2 & 0x1;

  m_nominalMsduSize = start.ReadLsbtohU16 ();
  m_maximumMsduSize = start.ReadLsbtohU16 ();
  m_minimumServiceInterval = start.ReadLsbtohU32 ();
  m_maximumServiceInterval = start.ReadLsbtohU32 ();
  m_inactivityInterval = start.ReadLsbtohU32 ();
  m_suspensionInterval = start.ReadLsbtohU32 ();
  m_serviceStartTime = start.ReadLsbtohU32 ();
  m_minimumDateRate = start.ReadLsbtohU32 ();
  m_meanDateRate = start.ReadLsbtohU32 ();
  m_peakDateRate = start.ReadLsbtohU32 ();
  m_burstSize = start.ReadLsbtohU32 ();
  m_delayBound = start.ReadLsbtohU32 ();
  m_minimumPhyRate = start.ReadLsbtohU32 ();
  m_surplusBandwidthAllowance = start.ReadLsbtohU16 ();
  m_mediumTime = start.ReadLsbtohU16 ();
  SetDmgAttributesField (start.ReadLsbtohU16 ());

  return length;
}

void
TspecElement::SetDmgAttributesField (uint16_t infieldfo)
{
  m_allocationID = infieldfo & 0xF;
  m_allocationDirection = (infieldfo >> 6) & 0x1;
  m_amsduSubframe = (infieldfo >> 7) & 0x1;
  m_reliabilityIndex = (infieldfo >> 8) & 0x3;
}

uint16_t
TspecElement::GetDmgAttributesField (void) const
{
  uint16_t dmgAttributes = 0;
  dmgAttributes |= m_allocationID & 0xF;
  dmgAttributes |= (m_allocationDirection & 0x1) << 6;
  dmgAttributes |= (m_amsduSubframe & 0x1) << 7;
  dmgAttributes |= (m_reliabilityIndex & 0x3) << 8;
  return dmgAttributes;
}

void
TspecElement::SetTrafficType (TrafficType type)
{
  m_trafficType = static_cast<uint8_t> (type);
}

void
TspecElement::SetTSID (uint8_t id)
{
  m_tsid = id;
}

void
TspecElement::SetDataDirection (DataDirection direction)
{
  m_dataDirection = static_cast<uint8_t> (direction);
}

void
TspecElement::SetAccessPolicy (AccessPolicy policy)
{
  m_accessPolicy = static_cast<uint8_t> (policy);
}

void
TspecElement::SetAggregation (bool enable)
{
  m_aggregation = enable;
}

void
TspecElement::SetAPSD (bool enable)
{
  m_apsd = enable;
}

void
TspecElement::SetUserPriority (uint8_t priority)
{
  m_userPriority = priority;
}

void
TspecElement::SetTSInfoAckPolicy (TSInfoAckPolicy policy)
{
  m_tsInfoAckPolicy = static_cast<uint8_t> (policy);
}

void
TspecElement::SetSchedule (bool schedule)
{
  m_schedule = schedule;
}

void
TspecElement::SetNominalMsduSize (uint16_t size, bool fixed)
{
  m_nominalMsduSize = static_cast<uint8_t> (fixed) << 14;
  m_nominalMsduSize |= size & 0x7FFF;
}

void
TspecElement::SetMaximumMsduSize (uint16_t size)
{
  m_maximumMsduSize = size;
}

void
TspecElement::SetMinimumServiceInterval (uint32_t interval)
{
  m_minimumServiceInterval = interval;
}

void
TspecElement::SetMaximumServiceInterval (uint32_t interval)
{
  m_maximumServiceInterval = interval;
}

void
TspecElement::SetInactivityInterval (uint32_t interval)
{
  m_inactivityInterval = interval;
}

void
TspecElement::SetSuspensionInterval (uint32_t interval)
{
  m_suspensionInterval = interval;
}

void
TspecElement::SetServiceStartTime (uint32_t time)
{
  m_serviceStartTime = time;
}

void
TspecElement::SetMinimumDataRate (uint32_t rate)
{
  m_minimumDateRate = rate;
}

void
TspecElement::SetMeanDataRate (uint32_t rate)
{
  m_meanDateRate = rate;
}

void
TspecElement::SetPeakDataRate (uint32_t rate)
{
  m_peakDateRate = rate;
}

void
TspecElement::SetBurstSize (uint32_t size)
{
  m_burstSize = size;
}

void
TspecElement::SetDelayBound (uint32_t bound)
{
  m_delayBound = bound;
}
void
TspecElement::SetMinimumPhyRate (uint32_t rate)
{
  m_minimumPhyRate = rate;
}

void
TspecElement::SetSurplusBandwidthAllowance (uint16_t allowance)
{
  m_surplusBandwidthAllowance = allowance;
}

void
TspecElement::SetMediumTime (uint16_t time)
{
  m_mediumTime = time;
}

void
TspecElement::SetAllocationID (uint8_t id)
{
  m_allocationID = id;
}

void
TspecElement::SetAllocationDirection (AllocationDirection dircetion)
{
  m_allocationDirection = static_cast<AllocationDirection> (dircetion);
}

void
TspecElement::SetAmsduSubframe (uint8_t amsdu)
{
  m_amsduSubframe = amsdu;
}

void
TspecElement::SetReliabilityIndex (uint8_t index)
{
  m_reliabilityIndex = index;
}

TrafficType
TspecElement::GetTrafficType (void) const
{
  return static_cast<TrafficType> (m_trafficType);
}

uint8_t
TspecElement::GetTSID (void) const
{
  return m_tsid;
}

DataDirection
TspecElement::GetDataDirection (void) const
{
  return static_cast<DataDirection> (m_dataDirection);
}

AccessPolicy
TspecElement::GetAccessPolicy (void) const
{
  return static_cast<AccessPolicy> (m_accessPolicy);
}

bool
TspecElement::GetAggregation (void) const
{
  return m_aggregation;
}

bool
TspecElement::GetAPSD (void) const
{
  return m_apsd;
}

uint8_t
TspecElement::GetUserPriority (void) const
{
  return m_userPriority;
}

TSInfoAckPolicy
TspecElement::GetTSInfoAckPolicy (void) const
{
  return static_cast<TSInfoAckPolicy> (m_tsInfoAckPolicy);
}

bool
TspecElement::GetSchedule (void) const
{
  return m_schedule;
}

uint16_t
TspecElement::GetNominalMsduSize (void) const
{
  return m_nominalMsduSize & 0x3FFF;
}

bool
TspecElement::IsMsduSizeFixed (void) const
{
  bool fixed = static_cast<bool> ((m_nominalMsduSize >> 14) & 0x1);
  return fixed;
}

uint16_t
TspecElement::GetMaximumMsduSize (void) const
{
  return m_maximumMsduSize;
}

uint32_t
TspecElement::GetMinimumServiceInterval (void) const
{
  return m_minimumServiceInterval;
}

uint32_t
TspecElement::GetMaximumServiceInterval (void) const
{
  return m_maximumServiceInterval;
}

uint32_t
TspecElement::GetInactivityInterval (void) const
{
  return m_inactivityInterval;
}

uint32_t
TspecElement::GetSuspensionInterval (void) const
{
  return m_suspensionInterval;
}

uint32_t
TspecElement::GetServiceStartTime (void) const
{
  return m_serviceStartTime;
}

uint32_t
TspecElement::GetMinimumDataRate (void) const
{
  return m_minimumDateRate;
}

uint32_t
TspecElement::GetMeanDataRate (void) const
{
  return m_meanDateRate;
}

uint32_t
TspecElement::GetPeakDataRate (void) const
{
  return m_peakDateRate;
}

uint32_t
TspecElement::GetBurstSize (void) const
{
  return m_burstSize;
}

uint32_t
TspecElement::GetDelayBound (void) const
{
  return m_delayBound;
}

uint32_t
TspecElement::GetMinimumPhyRate (void) const
{
  return m_minimumPhyRate;
}

uint16_t
TspecElement::GetSurplusBandwidthAllowance (void) const
{
  return m_surplusBandwidthAllowance;
}

uint16_t
TspecElement::GetMediumTime (void) const
{
  return m_mediumTime;
}

uint8_t
TspecElement::GetAllocationID (void) const
{
  return m_allocationID;
}

AllocationDirection
TspecElement::GetAllocationDirection (void) const
{
  return static_cast<AllocationDirection> (m_allocationDirection);
}

uint8_t
TspecElement::GetAmsduSubframe (void) const
{
  return m_amsduSubframe;
}

uint8_t
TspecElement::GetReliabilityIndex (void) const
{
  return m_reliabilityIndex;
}

ATTRIBUTE_HELPER_CPP (TspecElement);

std::ostream &
operator << (std::ostream &os, const TspecElement &element)
{
  return os;
}

std::istream &
operator >> (std::istream &is, TspecElement &element)
{
  uint32_t c1;
  is >> c1;
  return is;
}

/***************************************************
*         Traffic Stream (TS) Delay 8.4.2.34
****************************************************/

TsDelayElement::TsDelayElement ()
  :  m_delay (0)
{
}

WifiInformationElementId
TsDelayElement::ElementId () const
{
  return IE_TS_DELAY;
}

uint8_t
TsDelayElement::GetInformationFieldSize () const
{
  return 4;
}

void
TsDelayElement::SerializeInformationField (Buffer::Iterator start) const
{
  start.WriteHtolsbU32 (m_delay);
}

uint8_t
TsDelayElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;
  m_delay = i.ReadLsbtohU32 ();
  return length;
}

void
TsDelayElement::SetDelay (uint32_t value)
{
  m_delay = value;
}

uint32_t
TsDelayElement::GetDelay (void) const
{
  return m_delay;
}

ATTRIBUTE_HELPER_CPP (TsDelayElement);

std::ostream &
operator << (std::ostream &os, const TsDelayElement &element)
{
  os << element.GetDelay ();
  return os;
}

std::istream &
operator >> (std::istream &is, TsDelayElement &element)
{
  uint32_t c1;
  is >> c1;
  element.SetDelay (c1);
  return is;
}

/***************************************************
*         Timeout Interval Element 8.4.2.51
****************************************************/

TimeoutIntervalElement::TimeoutIntervalElement ()
  :  m_timeoutIntervalType (0),
     m_timeoutIntervalValue (0)
{
}

WifiInformationElementId
TimeoutIntervalElement::ElementId () const
{
  return IE_TIMEOUT_INTERVAL;
}

uint8_t
TimeoutIntervalElement::GetInformationFieldSize () const
{
  return 5;
}

void
TimeoutIntervalElement::SerializeInformationField (Buffer::Iterator start) const
{
  start.WriteU8 (m_timeoutIntervalType);
  start.WriteHtolsbU32 (m_timeoutIntervalValue);
}

uint8_t
TimeoutIntervalElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;

  m_timeoutIntervalType = i.ReadU8 ();
  m_timeoutIntervalValue = i.ReadLsbtohU32 ();

  return length;
}

void
TimeoutIntervalElement::SetTimeoutIntervalType (TimeoutIntervalType type)
{
  m_timeoutIntervalType = type;
}

void
TimeoutIntervalElement::SetTimeoutIntervalValue (uint32_t value)
{
  m_timeoutIntervalValue = value;
}

TimeoutIntervalType
TimeoutIntervalElement::GetTimeoutIntervalType () const
{
  return static_cast<TimeoutIntervalType> (m_timeoutIntervalType);
}

uint32_t
TimeoutIntervalElement::GetTimeoutIntervalValue () const
{
  return m_timeoutIntervalValue;
}

ATTRIBUTE_HELPER_CPP (TimeoutIntervalElement);

std::ostream &
operator << (std::ostream &os, const TimeoutIntervalElement &element)
{
  os << element.GetTimeoutIntervalType () << "|" << element.GetTimeoutIntervalValue ();

  return os;
}

std::istream &
operator >> (std::istream &is, TimeoutIntervalElement &element)
{
  uint8_t c1;
  uint32_t c2;
  is >> c1 >> c2;

  element.SetTimeoutIntervalType (static_cast<TimeoutIntervalType> (c1));
  element.SetTimeoutIntervalValue (c2);

  return is;
}

/***************************************************
*           DMG Information 8.4.2.131
****************************************************/

DmgOperationElement::DmgOperationElement ()
  :  m_tddti (0),
     m_pseudo (0),
     m_handover (0),
     m_psRequestSuspensionInterval (0),
     m_minBHIDuration (0),
     m_broadcastSTAInfoDuration (0),
     m_assocRespConfirmTime (0),
     m_minPPDuration (0),
     m_spIdleTimeout (0),
     m_maxLostBeacons (0)
{
}

WifiInformationElementId
DmgOperationElement::ElementId () const
{
  return IE_DMG_OPERATION;
}

uint8_t
DmgOperationElement::GetInformationFieldSize () const
{
  return 10;
}

Buffer::Iterator
DmgOperationElement::Serialize (Buffer::Iterator i) const
{
  return WifiInformationElement::Serialize (i);
}

uint16_t
DmgOperationElement::GetSerializedSize () const
{
  return WifiInformationElement::GetSerializedSize ();
}

void
DmgOperationElement::SerializeInformationField (Buffer::Iterator start) const
{
  start.WriteHtolsbU16 (GetDmgOperationInformation ());
  start.WriteHtolsbU64 (GetDmgBssParameterConfiguration ());
}

uint8_t
DmgOperationElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;
  uint32_t operation = i.ReadLsbtohU16 ();
  uint64_t config = i.ReadLsbtohU64 ();

  SetDmgOperationInformation (operation);
  SetDmgBssParameterConfiguration (config);

  return length;
}

void
DmgOperationElement::SetTDDTI (bool tddti)
{
  m_tddti = tddti;
}

void
DmgOperationElement::SetPseudoStaticAllocations (bool pseudoStatic)
{
  m_pseudo = pseudoStatic;
}

void
DmgOperationElement::SetPcpHandover (bool handover)
{
  m_handover = handover;
}

bool
DmgOperationElement::GetTDDTI () const
{
  return m_tddti;
}

bool
DmgOperationElement::GetPseudoStaticAllocations () const
{
  return m_pseudo;
}

bool
DmgOperationElement::GetPcpHandover () const
{
  return m_handover;
}

void
DmgOperationElement::SetDmgOperationInformation (uint16_t info)
{
  m_tddti = info & 0x1;
  m_pseudo = (info >> 1) & 0x1;
  m_handover = (info >> 2) & 0x1;
}

uint16_t
DmgOperationElement::GetDmgOperationInformation () const
{
  uint16_t val = 0;

  val |= (m_tddti & 0x1);
  val |= (m_pseudo & 0x1) << 1;
  val |= (m_handover & 0x1) <<  2;

  return val;
}

void
DmgOperationElement::SetPSRequestSuspensionInterval (uint8_t interval)
{
  m_psRequestSuspensionInterval = interval;
}

void
DmgOperationElement::SetMinBHIDuration (uint16_t duration)
{
  m_minBHIDuration = duration;
}

void
DmgOperationElement::SetBroadcastSTAInfoDuration (uint8_t duration)
{
  m_broadcastSTAInfoDuration = duration;
}

void
DmgOperationElement::SetAssocRespConfirmTime (uint8_t time)
{
  m_assocRespConfirmTime = time;
}

void
DmgOperationElement::SetMinPPDuration (uint8_t duration)
{
  m_minPPDuration = duration;
}

void
DmgOperationElement::SetSPIdleTimeout (uint8_t timeout)
{
  m_spIdleTimeout = timeout;
}

void
DmgOperationElement::SetMaxLostBeacons (uint8_t max)
{
  m_maxLostBeacons = max;
}

uint8_t
DmgOperationElement::GetPSRequestSuspensionInterval () const
{
  return m_psRequestSuspensionInterval;
}

uint16_t DmgOperationElement::GetMinBHIDuration () const
{
  return m_minBHIDuration;
}

uint8_t
DmgOperationElement::GetBroadcastSTAInfoDuration () const
{
  return m_broadcastSTAInfoDuration;
}

uint8_t
DmgOperationElement::GetAssocRespConfirmTime () const
{
  return m_assocRespConfirmTime;
}

uint8_t
DmgOperationElement::GetMinPPDuration () const
{
  return m_minPPDuration;
}

uint8_t
DmgOperationElement::GetSPIdleTimeout () const
{
  return m_spIdleTimeout;
}

uint8_t
DmgOperationElement::GetMaxLostBeacons () const
{
  return m_maxLostBeacons;
}

void
DmgOperationElement::SetDmgBssParameterConfiguration (uint64_t config)
{
  m_psRequestSuspensionInterval = config & 0xFF;
  m_minBHIDuration = (config >> 8) & 0xFFFF;
  m_broadcastSTAInfoDuration = (config >> 24) & 0xFF;
  m_assocRespConfirmTime = (config >> 32) & 0xFF;
  m_minPPDuration = (config >> 40) & 0xFF;
  m_spIdleTimeout = (config >> 48) & 0xFF;
  m_maxLostBeacons = (config >> 56) & 0xFF;
}

uint64_t
DmgOperationElement::GetDmgBssParameterConfiguration () const
{
  uint64_t val = 0;

  val |= uint64_t (m_psRequestSuspensionInterval);
  val |= uint64_t (m_minBHIDuration) << 8;
  val |= uint64_t (m_broadcastSTAInfoDuration) << 24;
  val |= uint64_t (m_assocRespConfirmTime) << 32;
  val |= uint64_t (m_minPPDuration) << 40;
  val |= uint64_t (m_spIdleTimeout) << 48;
  val |= uint64_t (m_maxLostBeacons) << 56;

  return val;
}

ATTRIBUTE_HELPER_CPP (DmgOperationElement);

std::ostream &
operator << (std::ostream &os, const DmgOperationElement &element)
{
  os <<  element.GetDmgOperationInformation () << "|" << element.GetDmgBssParameterConfiguration ();

  return os;
}

std::istream &
operator >> (std::istream &is, DmgOperationElement &element)
{
  uint16_t c1;
  uint64_t c2;
  is >> c1 >> c2;
  element.SetDmgOperationInformation (c1);
  element.SetDmgBssParameterConfiguration (c2);

  return is;
}

/***************************************************
*       DMG Beam Refinement Element 8.4.2.132
****************************************************/

BeamRefinementElement::BeamRefinementElement ()
  : m_initiator (false),
    m_txTrainResponse (false),
    m_rxTrainResponse (false),
    m_txTrnOk (false),
    m_txssFbckReq (false),
    m_bsFbck (0),
    m_bsFbckAntennaId (0),
    m_snrRequested (false),
    m_channelMeasurementRequested (false),
    m_numberOfTapsRequested (TAPS_1),
    m_sectorIdOrderRequested (false),
    m_snrPresent (false),
    m_channelMeasurementPresent (false),
    m_tapDelayPresent (false),
    m_numberOfTapsPresent (TAPS_1),
    m_numberOfMeasurements (0),
    m_sectorIdOrderPresent (false),
    m_linkType (false),
    m_antennaType (false),
    m_numberOfBeams (0),
    m_midExtension (false),
    m_capabilityRequest (false),
    m_bsFbckMsb (0),
    m_bsFbckAntennaIdMsb (0),
    m_numberOfMeasurementsMsb (0),
    m_edmgExtensionFlag (false),
    m_edmgChannelMeasurementPresent (false),
    m_sswFrameType(DMG_BEACON_FRAME),
    m_dbfFbckReq (false),
    m_channelAggregationRequested (false),
    m_channelAggregationPresent (false),
    m_bfTrainingType (SISO_BF),
    m_edmgDualPolarizationTrnChanneMeasurementPresent (false)
{
}

WifiInformationElementId
BeamRefinementElement::ElementId () const
{
  return IE_DMG_BEAM_REFINEMENT;
}

uint8_t
BeamRefinementElement::GetInformationFieldSize () const
{
  return 8;
}

void
BeamRefinementElement::SerializeInformationField (Buffer::Iterator start) const
{
  uint32_t value1 = 0;
  uint8_t value2 = 0;
  uint16_t value3 = 0;
  uint8_t value4 = 0;

  value1 |= (m_initiator & 0x1);
  value1 |= (m_txTrainResponse & 0x1) << 1;
  value1 |= (m_rxTrainResponse & 0x1) << 2;
  value1 |= (m_txTrnOk & 0x1) << 3;
  value1 |= (m_txssFbckReq & 0x1) << 4;
  value1 |= (m_bsFbck & 0x3F) << 5;
  value1 |= (m_bsFbckAntennaId & 0x3) << 11;
  /* FBCK-REQ */
  value1 |= (m_snrRequested & 0x1) << 13;
  value1 |= (m_channelMeasurementRequested & 0x1) << 14;
  value1 |= (static_cast<uint8_t> (m_numberOfTapsRequested) & 0x3) << 15;
  value1 |= (m_sectorIdOrderRequested & 0x1) << 17;
  /* FBCK-TYPE */
  value1 |= (m_snrPresent & 0x1) << 18;
  value1 |= (m_channelMeasurementPresent & 0x1) << 19;
  value1 |= (m_tapDelayPresent & 0x1) << 20;
  value1 |= (static_cast<uint8_t> (m_numberOfTapsPresent) & 0x3) << 21;
  value1 |= (m_numberOfMeasurements & 0x7F) << 23;
  value1 |= (m_sectorIdOrderPresent & 0x1) << 30;
  value1 |= (m_linkType & 0x1) << 31;
  value2 |= (m_antennaType & 0x1);
  value2 |= (m_numberOfBeams & 0x7) << 1;

  value2 |= (m_midExtension & 0x1) << 4;
  value2 |= (m_capabilityRequest & 0x1) << 5;

  /*802.11ay extension */
  value3 |= (m_bsFbckMsb & 0x1F);
  value3 |= (m_bsFbckAntennaIdMsb & 0x1) << 5;
  value3 |= (m_numberOfMeasurementsMsb & 0xF) << 6;
  value3 |= (m_edmgExtensionFlag & 0x1) << 10;
  value3 |= (m_edmgChannelMeasurementPresent & 0x1) << 11;
  value3 |= (static_cast<uint8_t> (m_sswFrameType) & 0x3) << 12;
  value3 |= (m_dbfFbckReq & 0x1) << 14;
  value3 |= (m_channelAggregationRequested & 0x1) << 15;

  value4 |= (m_channelAggregationPresent & 0x1);
  value4 |= (static_cast<uint8_t> (m_bfTrainingType) & 0x3) << 1;
  value4 |= (m_edmgDualPolarizationTrnChanneMeasurementPresent & 0x1) << 3;

  start.WriteHtolsbU32 (value1);
  start.WriteU8 (value2);
  start.WriteHtolsbU16 (value3);
  start.WriteU8 (value4);
}

uint8_t
BeamRefinementElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;
  uint32_t value1 = i.ReadLsbtohU32 ();
  uint8_t value2 = i.ReadU8 ();
  uint16_t value3 = i.ReadLsbtohU16 ();
  uint8_t value4 = i.ReadU8 ();

  m_initiator = value1 & 0x1;
  m_txTrainResponse = (value1 >> 1) & 0x1;
  m_rxTrainResponse = (value1 >> 2) & 0x1;
  m_txTrnOk = (value1 >> 3) & 0x1;
  m_txssFbckReq = (value1 >> 4) & 0x1;
  m_bsFbck = (value1 >> 5) & 0x3F;
  m_bsFbckAntennaId = (value1 >> 11) & 0x3;
  /* FBCK-REQ */
  m_snrRequested = (value1 >> 13) & 0x1;
  m_channelMeasurementRequested = (value1 >> 14) & 0x1;
  m_numberOfTapsRequested = static_cast<NUMBER_OF_TAPS> ((value1 >> 15) & 0x3);
  m_sectorIdOrderRequested = (value1 >> 17) & 0x1;
  /* FBCK-TYPE */
  m_snrPresent = (value1 >> 18) & 0x1;
  m_channelMeasurementPresent = (value1 >> 19) & 0x1;
  m_tapDelayPresent = (value1 >> 20) & 0x1;
  m_numberOfTapsPresent = static_cast<NUMBER_OF_TAPS> ((value1 >> 21) & 0x3);
  m_numberOfMeasurements = (value1 >> 23) & 0x7F;
  m_sectorIdOrderPresent = (value1 >> 30) & 0x1;
  m_linkType = (value1 >> 31) & 0x1;
  m_antennaType = value2 & 0x1;
  m_numberOfBeams = (value2 >> 1) & 0x7;

  m_midExtension = (value2 >> 4) & 0x1;
  m_capabilityRequest = (value2 >> 5) & 0x1;

  /* 802.11ay Extension Fields */
  m_bsFbckMsb = value3 & 0x1F;
  m_bsFbckAntennaIdMsb = (value3 >> 5) & 0x1;
  m_numberOfMeasurementsMsb = (value3 >> 6) & 0xF;
  m_edmgExtensionFlag = (value3 >> 10) & 0x1;
  m_edmgChannelMeasurementPresent = (value3 >> 11) & 0x1;
  m_sswFrameType = static_cast<SSW_FRAME_TYPE> ((value3 >> 12) & 0x3);
  m_dbfFbckReq = (value3 >> 14) & 0x1;
  m_channelAggregationRequested = (value3 >> 15) & 0x1;

  m_channelAggregationPresent = value4 & 0x1;
  m_bfTrainingType = static_cast<BF_TRAINING_TYPE> ((value4 >> 1) & 0x3);
  m_edmgDualPolarizationTrnChanneMeasurementPresent = (value4 >> 3) & 0x1;

  return length;
}

void
BeamRefinementElement::SetAsBeamRefinementInitiator (bool initiator)
{
  m_initiator = initiator;
}

void
BeamRefinementElement::SetTxTrainResponse (bool response)
{
  m_txTrainResponse = response;
}

void
BeamRefinementElement::SetRxTrainResponse (bool response)
{
  m_rxTrainResponse = response;
}

void
BeamRefinementElement::SetTxTrnOk (bool value)
{
  m_txTrnOk = value;
}

void
BeamRefinementElement::SetTxssFbckReq (bool feedback)
{
  m_txssFbckReq = feedback;
}

void
BeamRefinementElement::SetBsFbck (uint8_t index)
{
  m_bsFbck = index;
}

void
BeamRefinementElement::SetBsFbckAntennaID (uint8_t id)
{
  m_bsFbckAntennaId = id;
}

/* FBCK-REQ field */
void
BeamRefinementElement::SetSnrRequested (bool requested)
{
  m_snrRequested = requested;
}

void
BeamRefinementElement::SetChannelMeasurementRequested (bool requested)
{
  m_channelMeasurementRequested = requested;
}

void
BeamRefinementElement::SetNumberOfTapsRequested (NUMBER_OF_TAPS number)
{
  m_numberOfTapsRequested = number;
}

void
BeamRefinementElement::SetSectorIdOrderRequested (bool present)
{
  m_sectorIdOrderRequested = present;
}

bool
BeamRefinementElement::IsSnrRequested (void) const
{
  return m_snrRequested;
}

bool
BeamRefinementElement::IsChannelMeasurementRequested (void) const
{
  return m_channelMeasurementRequested;
}

NUMBER_OF_TAPS
BeamRefinementElement::GetNumberOfTapsRequested (void) const
{
  return m_numberOfTapsRequested;
}

bool
BeamRefinementElement::IsSectorIdOrderRequested (void) const
{
  return m_sectorIdOrderRequested;
}

/* FBCK-TYPE field */
void
BeamRefinementElement::SetSnrPresent (bool present)
{
  m_snrPresent = present;
}

void
BeamRefinementElement::SetChannelMeasurementPresent (bool present)
{
  m_channelMeasurementPresent = present;
}

void
BeamRefinementElement::SetTapDelayPresent (bool present)
{
  m_tapDelayPresent = present;
}

void
BeamRefinementElement::SetNumberOfTapsPresent (NUMBER_OF_TAPS number)
{
  m_numberOfTapsPresent = number;
}

void
BeamRefinementElement::SetNumberOfMeasurements (uint8_t number)
{
  m_numberOfMeasurements = number;
}

void
BeamRefinementElement::SetSectorIdOrderPresent (bool present)
{
  m_sectorIdOrderPresent = present;
}

void
BeamRefinementElement::SetLinkType (bool linkType)
{
  m_linkType = linkType;
}

void
BeamRefinementElement::SetAntennaType (bool type)
{
  m_antennaType = type;
}

void
BeamRefinementElement::SetNumberOfBeams (uint8_t number)
{
  m_numberOfBeams = number;
}

void
BeamRefinementElement::SetExtendedBsFbck (uint16_t index)
{
  m_bsFbck = index & 0x3F;
  m_bsFbckMsb = (index >> 6) & 0x1F;
}
void
BeamRefinementElement::SetExtendedBsFbckAntennaID (uint8_t id)
{
  m_bsFbckAntennaId = id & 0x3;
  m_bsFbckAntennaIdMsb = (id >> 2) & 0x1;
}

void
BeamRefinementElement::SetExtendedNumberOfMeasurements (uint16_t number)
{
  m_numberOfMeasurements = number & 0x7F;
  m_numberOfMeasurementsMsb = (number >> 7) & 0xF;
}

void
BeamRefinementElement::SetEdmgExtensionFlag (bool edmgFlag)
{
  m_edmgExtensionFlag = edmgFlag;
}


void
BeamRefinementElement::SetEdmgChannelMeasurementPresent ( bool present)
{
  m_edmgChannelMeasurementPresent = present;
}

void
BeamRefinementElement::SetSswFrameType (SSW_FRAME_TYPE type)
{
  m_sswFrameType = type;
}

void
BeamRefinementElement::SetDbfFbckReq (bool req)
{
  m_dbfFbckReq = req;
}

void
BeamRefinementElement::SetChannelAggregationRequested (bool req)
{
  m_channelAggregationRequested = req;
}

void
BeamRefinementElement::SetChannelAggregationPresent (bool present)
{
  m_channelAggregationPresent = present;
}

void
BeamRefinementElement::SetBfTrainingType (BF_TRAINING_TYPE type)
{
  m_bfTrainingType = type;
}

void
BeamRefinementElement::SetEdmgDualPolTrnChMeasurementPresent (bool present)
{
  m_edmgDualPolarizationTrnChanneMeasurementPresent = present;
}

bool
BeamRefinementElement::IsSnrPresent (void) const
{
  return m_snrPresent;
}

bool
BeamRefinementElement::IsChannelMeasurementPresent (void) const
{
  return m_channelMeasurementPresent;
}

bool
BeamRefinementElement::IsTapDelayPresent (void) const
{
  return m_tapDelayPresent;
}

NUMBER_OF_TAPS
BeamRefinementElement::GetNumberOfTapsPresent (void) const
{
  return m_numberOfTapsPresent;
}

uint8_t
BeamRefinementElement::GetNumberOfMeasurements (void) const
{
  return m_numberOfMeasurements;
}

bool
BeamRefinementElement::IsSectorIdOrderPresent (void) const
{
  return m_sectorIdOrderPresent;
}

bool
BeamRefinementElement::GetLinkType (void) const
{
  return m_linkType;
}

bool
BeamRefinementElement::GetAntennaType (void) const
{
  return m_antennaType;
}

uint8_t
BeamRefinementElement::GetNumberOfBeams (void) const
{
  return m_numberOfBeams;
}

void
BeamRefinementElement::SetMidExtension (bool mid)
{
  m_midExtension = mid;
}

void
BeamRefinementElement::SetCapabilityRequest (bool request)
{
  m_capabilityRequest = request;
}

bool
BeamRefinementElement::IsBeamRefinementInitiator (void) const
{
  return m_initiator;
}

bool
BeamRefinementElement::IsTxTrainResponse (void) const
{
  return m_txTrainResponse;
}

bool
BeamRefinementElement::IsRxTrainResponse (void) const
{
  return m_rxTrainResponse;
}

bool
BeamRefinementElement::IsTxTrnOk (void) const
{
  return m_txTrnOk;
}

bool
BeamRefinementElement::IsTxssFbckReq (void) const
{
  return m_txssFbckReq;
}

uint8_t
BeamRefinementElement::GetBsFbck (void) const
{
  return m_bsFbck;
}

uint8_t
BeamRefinementElement::GetBsFbckAntennaID (void) const
{
  return m_bsFbckAntennaId;
}

bool
BeamRefinementElement::IsMidExtension (void) const
{
  return m_midExtension;
}

bool
BeamRefinementElement::IsCapabilityRequest (void) const
{
  return m_capabilityRequest;
}

uint16_t
BeamRefinementElement::GetExtendedBsFbck (void) const
{
  uint16_t bsFbck = 0;
  bsFbck |= (m_bsFbck & 0x3F);
  bsFbck |= (m_bsFbckMsb & 0x1F) << 6;
  return bsFbck;
}

uint8_t
BeamRefinementElement::GetExtendedBsFbckAntennaID (void) const
{
  uint8_t bsFbckAntennaId = 0;
  bsFbckAntennaId |= (m_bsFbckAntennaId & 0x3);
  bsFbckAntennaId |= (m_bsFbckAntennaIdMsb & 0x1) << 2;
  return bsFbckAntennaId;
}

uint16_t
BeamRefinementElement::GetExtendedNumberOfMeasurements (void) const
{
  uint16_t numberOfMeasurments = 0;
  numberOfMeasurments |= (m_numberOfMeasurements & 0x7F);
  numberOfMeasurments |= (m_numberOfMeasurementsMsb & 0xF) << 7;
  return numberOfMeasurments;
}

bool
BeamRefinementElement::GetEdmgExtensionFlag (void) const
{
  return m_edmgExtensionFlag;
}

bool
BeamRefinementElement::IsEdmgChannelMeasurementPresent (void) const
{
  return m_edmgChannelMeasurementPresent;
}

SSW_FRAME_TYPE
BeamRefinementElement::GetSectorSweepFrameType (void) const
{
  return m_sswFrameType;
}

bool
BeamRefinementElement::IsDbFbckReq (void) const
{
  return m_dbfFbckReq;
}

bool
BeamRefinementElement::IsChannelAggregationRequested (void) const
{
  return m_channelAggregationRequested;
}

bool
BeamRefinementElement::IsChannelAggregationPresent (void) const
{
  return m_channelAggregationPresent;
}

BF_TRAINING_TYPE
BeamRefinementElement::GetBfTrainingType (void) const
{
  return m_bfTrainingType;
}

bool
BeamRefinementElement::IsEdmgDualPolTrnChMeasurementPresent (void) const
{
  return m_edmgDualPolarizationTrnChanneMeasurementPresent;
}

ATTRIBUTE_HELPER_CPP (BeamRefinementElement);

std::ostream &
operator << (std::ostream &os, const BeamRefinementElement &element)
{
  return os;
}

std::istream &
operator >> (std::istream &is, BeamRefinementElement &element)
{
  return is;
}

/***************************************************
*           Wakeup Schedule 8.4.2.133
****************************************************/

NS_LOG_COMPONENT_DEFINE ("WakeupScheduleElement");

WakeupScheduleElement::WakeupScheduleElement ()
  :  m_biStarTime (0),
     m_sleepCycle (0),
     m_numberBIs (0)
{
}

WifiInformationElementId
WakeupScheduleElement::ElementId () const
{
  return IE_WAKEUP_SCHEDULE;
}

uint8_t
WakeupScheduleElement::GetInformationFieldSize () const
{
  return 8;
}

Buffer::Iterator
WakeupScheduleElement::Serialize (Buffer::Iterator i) const
{
  return WifiInformationElement::Serialize (i);
}

uint16_t
WakeupScheduleElement::GetSerializedSize () const
{
  return WifiInformationElement::GetSerializedSize ();
}

void
WakeupScheduleElement::SerializeInformationField (Buffer::Iterator start) const
{
  start.WriteHtolsbU32 (m_biStarTime);
  start.WriteHtolsbU16 (m_sleepCycle);
  start.WriteHtolsbU16 (m_numberBIs);
}

uint8_t
WakeupScheduleElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;

  m_biStarTime = i.ReadLsbtohU32 ();
  m_sleepCycle = i.ReadLsbtohU16 ();
  m_numberBIs = i.ReadLsbtohU16 ();

  return length;
}

void
WakeupScheduleElement::SetBiStartTime (uint32_t time)
{
  m_biStarTime = time;
}

void
WakeupScheduleElement::SetSleepCycle (uint16_t cycle)
{
  m_sleepCycle = cycle;
}

void
WakeupScheduleElement::SetNumberOfAwakeDozeBIs (uint16_t number)
{
  m_numberBIs = number;
}

uint32_t
WakeupScheduleElement::GetBiStartTime () const
{
  return m_biStarTime;
}

uint16_t
WakeupScheduleElement::GetSleepCycle () const
{
  return m_sleepCycle;
}

uint16_t
WakeupScheduleElement::GetNumberOfAwakeDozeBIs () const
{
  return m_numberBIs;
}

ATTRIBUTE_HELPER_CPP (WakeupScheduleElement);

std::ostream &
operator << (std::ostream &os, const WakeupScheduleElement &element)
{
  os << element.GetBiStartTime () << "|" << element.GetSleepCycle () << "|" << element.GetNumberOfAwakeDozeBIs ();

  return os;
}

std::istream &
operator >> (std::istream &is, WakeupScheduleElement &element)
{
  uint32_t c1;
  uint16_t c2;
  uint16_t c3;
  is >> c1 >> c2 >> c3;

  element.SetBiStartTime (c1);
  element.SetBiStartTime (c2);
  element.SetBiStartTime (c3);

  return is;
}

/******************************************
*    Allocation Field Format (8-401aa)
*******************************************/

AllocationField::AllocationField ()
  : m_allocationID (0),
    m_allocationType (0),
    m_pseudoStatic (false),
    m_truncatable (false),
    m_extendable (false),
    m_pcpActive (false),
    m_lpScUsed (false),
    m_SourceAid (0),
    m_destinationAid (0),
    m_allocationStart (0),
    m_allocationBlockDuration (0),
    m_numberOfBlocks (0),
    m_allocationBlockPeriod (0),
    m_allocationAnnounced (false)
{
}

void
AllocationField::Print (std::ostream &os) const
{

}

uint32_t
AllocationField::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  return 15;
}

Buffer::Iterator
AllocationField::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;

  i.WriteHtolsbU16 (GetAllocationControl ());
  i = m_bfControl.Serialize (i);
  i.WriteU8 (m_SourceAid);
  i.WriteU8 (m_destinationAid);
  i.WriteHtolsbU32 (m_allocationStart);
  i.WriteHtolsbU16 (m_allocationBlockDuration);
  i.WriteU8 (m_numberOfBlocks);
  i.WriteHtolsbU16 (m_allocationBlockPeriod);

  return i;
}

Buffer::Iterator
AllocationField::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;

  SetAllocationControl (i.ReadLsbtohU16 ());
  i = m_bfControl.Deserialize (i);
  m_SourceAid = i.ReadU8 ();
  m_destinationAid = i.ReadU8 ();
  m_allocationStart = i.ReadLsbtohU32 ();
  m_allocationBlockDuration = i.ReadLsbtohU16 ();
  m_numberOfBlocks = i.ReadU8 ();
  m_allocationBlockPeriod = i.ReadLsbtohU16 ();

  return i;
}

void
AllocationField::SetAllocationID (AllocationID id)
{
  m_allocationID = id;
}

void
AllocationField::SetAllocationType (AllocationType type)
{
  m_allocationType = type;
}

void
AllocationField::SetAsPseudoStatic (bool value)
{
  m_pseudoStatic = value;
}

void
AllocationField::SetAsTruncatable (bool value)
{
  m_truncatable = value;
}

void
AllocationField::SetAsExtendable (bool value)
{
  m_extendable = value;
}

void
AllocationField::SetPcpActive (bool value)
{
  m_pcpActive = value;
}

void
AllocationField::SetLpScUsed (bool value)
{
  m_lpScUsed = value;
}

AllocationID
AllocationField::GetAllocationID (void) const
{
  return m_allocationID;
}

AllocationType
AllocationField::GetAllocationType (void) const
{
  return static_cast<AllocationType> (m_allocationType);
}

bool
AllocationField::IsPseudoStatic (void) const
{
  return m_pseudoStatic;
}

bool
AllocationField::IsTruncatable (void) const
{
  return m_truncatable;
}

bool
AllocationField::IsExtendable (void) const
{
  return m_extendable;
}

bool
AllocationField::IsPcpActive (void) const
{
  return m_pcpActive;
}

bool
AllocationField::IsLpScUsed (void) const
{
  return m_lpScUsed;
}

void
AllocationField::SetAllocationControl (uint16_t ctrl)
{
  m_allocationID = ctrl & 0xF;
  m_allocationType = (ctrl >> 4) & 0x7;
  m_pseudoStatic = (ctrl >> 7) & 0x1;
  m_truncatable = (ctrl >> 8) & 0x1;
  m_extendable = (ctrl >> 9) & 0x1;
  m_pcpActive = (ctrl >> 10) & 0x1;
  m_lpScUsed = (ctrl >> 11) & 0x1;
}

uint16_t
AllocationField::GetAllocationControl (void) const
{
  uint16_t val = 0;
  val |= m_allocationID & 0xF;
  val |= (m_allocationType & 0x7) << 4;
  val |= (m_pseudoStatic & 0x1) << 7;
  val |= (m_truncatable & 0x1) << 8;
  val |= (m_extendable & 0x1) << 9;
  val |= (m_pcpActive & 0x1) << 10;
  val |= (m_lpScUsed & 0x1) << 11;
  return val;
}

void
AllocationField::SetBfControl (BF_Control_Field &field)
{
  m_bfControl = field;
}

void
AllocationField::SetSourceAid (uint8_t aid)
{
  m_SourceAid = aid;
}

void
AllocationField::SetDestinationAid (uint8_t aid)
{
  m_destinationAid = aid;
}

void
AllocationField::SetAllocationStart (uint32_t start)
{
  m_allocationStart = start;
}

void
AllocationField::SetAllocationBlockDuration (uint16_t duration)
{
  if (m_allocationType == SERVICE_PERIOD_ALLOCATION)
    {
      NS_ASSERT (1 <= duration && duration <= 32767);
    }
  else
    {
      NS_ASSERT (1 <= duration && duration <= 65535);
    }
  m_allocationBlockDuration = duration;
}

void
AllocationField::SetNumberOfBlocks (uint8_t number)
{
  m_numberOfBlocks = number;
}

void
AllocationField::SetAllocationBlockPeriod (uint16_t period)
{
  m_allocationBlockPeriod = period;
}

BF_Control_Field
AllocationField::GetBfControl (void) const
{
  return m_bfControl;
}

uint8_t
AllocationField::GetSourceAid (void) const
{
  return m_SourceAid;
}

uint8_t
AllocationField::GetDestinationAid (void) const
{
  return m_destinationAid;
}

uint32_t
AllocationField::GetAllocationStart (void) const
{
  return m_allocationStart;
}

uint16_t
AllocationField::GetAllocationBlockDuration (void) const
{
  return m_allocationBlockDuration;
}

uint8_t
AllocationField::GetNumberOfBlocks (void) const
{
  return m_numberOfBlocks;
}

uint16_t
AllocationField::GetAllocationBlockPeriod (void) const
{
  return m_allocationBlockPeriod;
}

void
AllocationField::SetAllocationAnnounced (void)
{
  m_allocationAnnounced = true;
}

bool
AllocationField::IsAllocationAnnounced (void) const
{
  return m_allocationAnnounced;
}

/***************************************************
*        Extended Schedule Element 8.4.2.134
****************************************************/

ExtendedScheduleElement::ExtendedScheduleElement ()
{
}

WifiInformationElementId
ExtendedScheduleElement::ElementId () const
{
  return IE_EXTENDED_SCHEDULE;
}

uint8_t
ExtendedScheduleElement::GetInformationFieldSize () const
{
  return m_list.size () * 15;
}

void
ExtendedScheduleElement::SerializeInformationField (Buffer::Iterator start) const
{
  for (AllocationFieldList::const_iterator i = m_list.begin (); i != m_list.end () ; i++)
    {
      start = i->Serialize (start);
    }
}

uint8_t
ExtendedScheduleElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  AllocationField field;
  Buffer::Iterator i = start;
  while (i.GetDistanceFrom (start) != length)
    {
      i = field.Deserialize (i);
      m_list.push_back (field);
    }
  return length;
}

void
ExtendedScheduleElement::AddAllocationField (AllocationField &field)
{
  m_list.push_back (field);
}

void
ExtendedScheduleElement::SetAllocationFieldList (const AllocationFieldList &list)
{
  m_list = list;
}

AllocationFieldList
ExtendedScheduleElement::GetAllocationFieldList (void) const
{
  return m_list;
}

ATTRIBUTE_HELPER_CPP (ExtendedScheduleElement);

std::ostream &operator << (std::ostream &os, const ExtendedScheduleElement &element)
{
  return os;
}

std::istream &operator >> (std::istream &is, ExtendedScheduleElement &element)
{
  return is;
}

/***************************************************
*              Next DMG ATI 8.4.2.137
****************************************************/

StaInfoField::StaInfoField ()
  : m_aid (0),
    m_cbap (false),
    m_pp (false)
{
}

void
StaInfoField::Print (std::ostream &os) const
{
}

uint32_t
StaInfoField::GetSerializedSize (void) const
{
  return 2;
}

Buffer::Iterator
StaInfoField::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  uint8_t value = 0;
  value |= m_cbap;
  value |= (m_pp << 1);
  value |= (m_reserved & 0x3F) << 2;

  i.WriteU8 (m_aid);
  i.WriteU8 (value);

  return i;
}

Buffer::Iterator
StaInfoField::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;

  m_aid = i.ReadU8 ();
  uint8_t value = i.ReadU8 ();
  m_cbap = value & 0x1;
  m_pp = (value >> 1) & 0x1;
  m_reserved = (value >> 2) & 0x3F;

  return i;
}

void
StaInfoField::SetAid (uint8_t aid)
{
  m_aid = aid;
}

void
StaInfoField::SetCbap (bool value)
{
  m_cbap = value;
}

void
StaInfoField::SetPollingPhase (bool value)
{
  m_pp = value;
}

void
StaInfoField::SetReserved (uint8_t value)
{
  m_reserved = value;
}

uint8_t
StaInfoField::GetAid (void) const
{
  return m_aid;
}

bool
StaInfoField::GetCbap (void) const
{
  return m_cbap;
}

bool
StaInfoField::GetPollingPhase (void) const
{
  return m_pp;
}

uint8_t
StaInfoField::GetReserved (void) const
{
  return m_reserved;
}

/***************************************************
*        STA Availability element 8.4.2.135
****************************************************/

StaAvailabilityElement::StaAvailabilityElement ()
{
}

WifiInformationElementId
StaAvailabilityElement::ElementId () const
{
  return IE_STA_AVAILABILITY;
}

uint8_t
StaAvailabilityElement::GetInformationFieldSize () const
{
  return m_list.size () * 2;
}

void
StaAvailabilityElement::SerializeInformationField (Buffer::Iterator start) const
{
  for (StaInfoFieldList::const_iterator i = m_list.begin (); i != m_list.end () ; i++)
    {
      start = i->Serialize (start);
    }
}

uint8_t
StaAvailabilityElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  StaInfoField field;
  Buffer::Iterator i = start;
  while (i.GetDistanceFrom (start) != length)
    {
      i = field.Deserialize (i);
      m_list.push_back (field);
    }
  return length;
}

void
StaAvailabilityElement::AddStaInfo (StaInfoField &field)
{
  m_list.push_back (field);
}

void
StaAvailabilityElement::SetStaInfoList (const StaInfoFieldList &list)
{
  m_list = list;
}

StaInfoFieldList
StaAvailabilityElement::GetStaInfoList (void) const
{
  return m_list;
}

StaInfoField
StaAvailabilityElement::GetStaInfoField (void) const
{
  NS_ASSERT (m_list.size () > 0);
  return m_list[0];
}

ATTRIBUTE_HELPER_CPP (StaAvailabilityElement);

std::ostream &
operator << (std::ostream &os, const StaAvailabilityElement &element)
{
  return os;
}

std::istream &
operator >> (std::istream &is, StaAvailabilityElement &element)
{
  return is;
}

/***************************************************
*             DMG Allocation Info Field
****************************************************/

DmgAllocationInfo::DmgAllocationInfo ()
  : m_allocationID (0),
    m_allocationType (SERVICE_PERIOD_ALLOCATION),
    m_allocationFormat (ISOCHRONOUS),
    m_pseudoStatic (false),
    m_truncatable (false),
    m_extendable (false),
    m_lpScUsed (false),
    m_up (0),
    m_destAid (0)
{
}

void
DmgAllocationInfo::Print (std::ostream &os) const
{

}

uint32_t
DmgAllocationInfo::GetSerializedSize (void) const
{
  return 3;
}

Buffer::Iterator
DmgAllocationInfo::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  uint16_t val1 = 0;
  uint8_t val2 = 0;

  val1 |= m_allocationID & 0xF;
  val1 |= (m_allocationType & 0x7) << 4;
  val1 |= (m_allocationFormat & 0x1) << 7;
  val1 |= (m_pseudoStatic & 0x1) << 8;
  val1 |= (m_truncatable & 0x1) << 9;
  val1 |= (m_extendable & 0x1) << 10;
  val1 |= (m_lpScUsed & 0x1) << 11;
  val1 |= (m_up & 0x7) << 12;
  val1 |= (m_destAid & 0x1) << 15;
  val2 |= (m_destAid >> 1);

  i.WriteHtolsbU16 (val1);
  i.WriteU8 (val2);

  return i;
}

Buffer::Iterator
DmgAllocationInfo::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  uint16_t val1 = i.ReadLsbtohU16 ();
  uint8_t val2 = i.ReadU8 ();

  m_allocationID = val1 & 0xF;
  m_allocationType = (val1 >> 4) & 0x7;
  m_allocationFormat = (val1 >> 7) & 0x1;
  m_pseudoStatic = (val1 >> 8) & 0x1;
  m_truncatable = (val1 >> 9) & 0x1;
  m_extendable = (val1 >> 10) & 0x1;
  m_lpScUsed = (val1 >> 11) & 0x1;
  m_up = (val1 >> 12) & 0x7;
  m_destAid = (val1 >> 15) & 0x1;
  m_destAid |= (val2 << 1);

  return i;
}

void
DmgAllocationInfo::SetAllocationID (AllocationID id)
{
  m_allocationID = id;
}

void
DmgAllocationInfo::SetAllocationType (AllocationType type)
{
  m_allocationType = static_cast<AllocationType> (type);
}

void
DmgAllocationInfo::SetAllocationFormat (AllocationFormat format)
{
  m_allocationFormat = static_cast<AllocationFormat> (format);
}

void
DmgAllocationInfo::SetAsPseudoStatic (bool value)
{
  m_pseudoStatic = value;
}

void
DmgAllocationInfo::SetAsTruncatable (bool value)
{
  m_truncatable = value;
}

void
DmgAllocationInfo::SetAsExtendable (bool value)
{
  m_extendable = value;
}

void
DmgAllocationInfo::SetLpScUsed (bool value)
{
  m_lpScUsed = value;
}

void
DmgAllocationInfo::SetUp (uint8_t value)
{
  m_up = value;
}

void
DmgAllocationInfo::SetDestinationAid (uint8_t aid)
{
  m_destAid = aid;
}

AllocationID
DmgAllocationInfo::GetAllocationID (void) const
{
  return m_allocationID;
}

AllocationType
DmgAllocationInfo::GetAllocationType (void) const
{
  return static_cast<AllocationType> (m_allocationType);
}

AllocationFormat
DmgAllocationInfo::GetAllocationFormat (void) const
{
  return static_cast<AllocationFormat> (m_allocationFormat);
}

bool
DmgAllocationInfo::IsPseudoStatic (void) const
{
  return m_pseudoStatic;
}

bool
DmgAllocationInfo::IsTruncatable (void) const
{
  return m_truncatable;
}

bool
DmgAllocationInfo::IsExtendable (void) const
{
  return m_extendable;
}

bool
DmgAllocationInfo::IsLpScUsed (void) const
{
  return m_lpScUsed;
}

uint8_t
DmgAllocationInfo::GetUp (void) const
{
  return m_up;
}

uint8_t
DmgAllocationInfo::GetDestinationAid (void) const
{
  return m_destAid;
}

/***************************************************
*               Constraint Subfield
****************************************************/

ConstraintSubfield::ConstraintSubfield ()
  : m_startTime (0),
    m_duration (0),
    m_period (0)
{
}

uint32_t
ConstraintSubfield::GetSerializedSize (void) const
{
  return 14;
}

Buffer::Iterator
ConstraintSubfield::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteHtolsbU32 (m_startTime);
  i.WriteHtolsbU16 (m_duration);
  i.WriteHtolsbU16 (m_period);
  WriteTo (i, m_address);
  return i;
}

Buffer::Iterator
ConstraintSubfield::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_startTime = i.ReadLsbtohU32 ();
  m_duration = i.ReadLsbtohU16 ();
  m_period = i.ReadLsbtohU16 ();
  ReadFrom (i, m_address);
  return i;
}

void
ConstraintSubfield::SetStartStartTime (uint32_t time)
{
  m_startTime = time;
}

void
ConstraintSubfield::SetDuration (uint16_t duration)
{
  m_duration = duration;
}

void
ConstraintSubfield::SetPeriod (uint16_t period)
{
  m_period = period;
}

void
ConstraintSubfield::SetInterfererAddress (Mac48Address address)
{
  m_address = address;
}

uint32_t
ConstraintSubfield::GetStartStartTime (void) const
{
  return m_startTime;
}

uint16_t
ConstraintSubfield::GetDuration (void) const
{
  return m_duration;
}

uint16_t
ConstraintSubfield::GetPeriod (void) const
{
  return m_period;
}

Mac48Address
ConstraintSubfield::GetInterfererAddress (void) const
{
  return m_address;
}

/***************************************************
*             DMG TSPEC element 8.4.2.136
****************************************************/

DmgTspecElement::DmgTspecElement ()
  : m_allocationPeriod (0),
    m_minAllocation (0),
    m_maxAllocation (0),
    m_minDuration (0)
{
}

WifiInformationElementId
DmgTspecElement::ElementId () const
{
  return IE_DMG_TSPEC;
}

uint8_t
DmgTspecElement::GetInformationFieldSize () const
{
  uint8_t size = 0;
  size += 14 * (1 + m_constraintList.size ());
  return size;
}

void
DmgTspecElement::SerializeInformationField (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i = m_dmgAllocationInfo.Serialize (i);
  i = m_bfControlField.Serialize (i);
  i.WriteHtolsbU16 (m_allocationPeriod);
  i.WriteHtolsbU16 (m_minAllocation);
  i.WriteHtolsbU16 (m_maxAllocation);
  i.WriteHtolsbU16 (m_minDuration);
  i.WriteU8 (m_constraintList.size ());
  for (ConstraintListCI it = m_constraintList.begin (); it != m_constraintList.end (); it++)
    {
      i = it->Serialize (i);
    }
}

uint8_t
DmgTspecElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;
  uint8_t numberOfConstraints;

  i = m_dmgAllocationInfo.Deserialize (i);
  i = m_bfControlField.Deserialize (i);
  m_allocationPeriod = i.ReadLsbtohU16 ();
  m_minAllocation = i.ReadLsbtohU16 ();
  m_maxAllocation = i.ReadLsbtohU16 ();
  m_minDuration = i.ReadLsbtohU16 ();
  numberOfConstraints = i.ReadU8 ();
  while (numberOfConstraints != 0)
    {
      ConstraintSubfield constraint;
      i = constraint.Deserialize (i);
      m_constraintList.push_back (constraint);
      numberOfConstraints--;
    }

  return length;
}

void
DmgTspecElement::SetDmgAllocationInfo (DmgAllocationInfo &info)
{
  m_dmgAllocationInfo = info;
}

void
DmgTspecElement::SetBfControl (BF_Control_Field &ctrl)
{
  m_bfControlField = ctrl;
}

void
DmgTspecElement::SetAllocationPeriod (uint16_t period, bool multiple)
{
  NS_ASSERT (period <= 32767);
  m_allocationPeriod = uint16_t (multiple) << 15;
  m_allocationPeriod |= period;
}

void
DmgTspecElement::SetMinimumAllocation (uint16_t min)
{
  m_minAllocation = min;
}

void
DmgTspecElement::SetMaximumAllocation (uint16_t max)
{
  m_maxAllocation = max;
}

void
DmgTspecElement::SetMinimumDuration (uint16_t duration)
{
  m_minDuration = duration;
}

void
DmgTspecElement::AddTrafficSchedulingConstraint (ConstraintSubfield &constraint)
{
  NS_ASSERT_MSG (m_constraintList.size () != 255, "Cannot add more than 255 contraints");
  m_constraintList.push_back (constraint);
}

DmgAllocationInfo
DmgTspecElement::GetDmgAllocationInfo (void) const
{
  return m_dmgAllocationInfo;
}

BF_Control_Field
DmgTspecElement::GetBfControl (void) const
{
  return m_bfControlField;
}

uint16_t
DmgTspecElement::GetAllocationPeriod (void) const
{
  return (m_allocationPeriod & 0x7FFF);
}

bool
DmgTspecElement::IsAllocationPeriodMultipleBI (void) const
{
  return ((m_allocationPeriod >> 15) & 0x1);
}

uint16_t
DmgTspecElement::GetMinimumAllocation (void) const
{
  return m_minAllocation;
}

uint16_t
DmgTspecElement::GetMaximumAllocation (void) const
{
  return m_maxAllocation;
}

uint16_t
DmgTspecElement::GetMinimumDuration (void) const
{
  return m_minDuration;
}

uint8_t
DmgTspecElement::GetNumberOfConstraints (void) const
{
  return m_constraintList.size ();
}

ConstraintList
DmgTspecElement::GetConstraintList (void) const
{
  return m_constraintList;
}

std::ostream &operator << (std::ostream &os, const DmgTspecElement &element)
{
  return os;
}

std::istream &operator >> (std::istream &is, DmgTspecElement &element)
{
  return is;
}

/***************************************************
*              Next DMG ATI 8.4.2.137
****************************************************/

NextDmgAti::NextDmgAti ()
  : m_startTime (0),
    m_atiDuration (0)
{
}

WifiInformationElementId
NextDmgAti::ElementId () const
{
  return IE_NEXT_DMG_ATI;
}

uint8_t
NextDmgAti::GetInformationFieldSize () const
{
  return 6;
}

Buffer::Iterator
NextDmgAti::Serialize (Buffer::Iterator i) const
{
  return WifiInformationElement::Serialize (i);
}

uint16_t
NextDmgAti::GetSerializedSize () const
{
  return WifiInformationElement::GetSerializedSize ();
}

void
NextDmgAti::SerializeInformationField (Buffer::Iterator start) const
{
  start.WriteHtolsbU32 (m_startTime);
  start.WriteHtolsbU16 (m_atiDuration);
}

uint8_t
NextDmgAti::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;
  uint32_t time = i.ReadLsbtohU32 ();
  uint16_t duration = i.ReadLsbtohU16 ();

  SetStartTime (time);
  SetAtiDuration (duration);

  return length;
}

void
NextDmgAti::SetStartTime (uint32_t time)
{
  m_startTime = time;
}

void
NextDmgAti::SetAtiDuration (uint16_t duration)
{
  m_atiDuration = duration;
}

uint32_t
NextDmgAti::GetStartTime () const
{
  return m_startTime;
}

uint16_t
NextDmgAti::GetAtiDuration () const
{
  return m_atiDuration;
}

ATTRIBUTE_HELPER_CPP (NextDmgAti);

std::ostream &
operator << (std::ostream &os, const NextDmgAti &element)
{
  os <<  element.GetStartTime () << "|" << element.GetAtiDuration ();
  return os;
}

std::istream &
operator >> (std::istream &is, NextDmgAti &element)
{
  uint32_t c1;
  uint16_t c2;
  is >> c1 >> c2;
  element.SetStartTime (c1);
  element.SetAtiDuration (c2);

  return is;
}

/***************************************************
*  Channel Measurement Feedback Element 8.4.2.138
****************************************************/

ChannelMeasurementFeedbackElement::ChannelMeasurementFeedbackElement ()
{
  m_snrListSize = 0;
  m_channelMeasurementSize = 0;
  m_tapComponentsSize = 0;
  m_tapsDelaySize = 0;
  m_sectorIdOrderSize = 0;
}

WifiInformationElementId
ChannelMeasurementFeedbackElement::ElementId () const
{
  return IE_CHANNEL_MEASUREMENT_FEEDBACK;
}

uint8_t
ChannelMeasurementFeedbackElement::GetInformationFieldSize () const
{
  uint8_t size;
  uint16_t numBits = 0;
  numBits += m_snrList.size () * 8;
  if (m_channelMeasurementList.size () != 0)
    {
      numBits += m_channelMeasurementList.size () * m_channelMeasurementList.front ().size() * 16;
    }
  numBits += m_tapDelayList.size () * 8;
  numBits += m_sectorIdOrderList.size() * 8;
  size = std::ceil (numBits/8);
  return size;
}

void
ChannelMeasurementFeedbackElement::SerializeInformationField (Buffer::Iterator start) const
{
  /* Serialize SNR List */
  for (SNR_INT_LIST_ITERATOR iter = m_snrList.begin (); iter != m_snrList.end (); iter++)
    {
      start.WriteU8 (*iter);
    }

  /* Channel Measurement List */
  TAP_COMPONENTS_LIST tapList;
  TAP_COMPONENTS components;
  for (CHANNEL_MEASUREMENT_LIST_ITERATOR iter1 = m_channelMeasurementList.begin ();
       iter1 != m_channelMeasurementList.end (); iter1++)
    {
      tapList = *iter1;
      for (TAP_COMPONENTS_LIST_ITERATOR iter2 = tapList.begin (); iter2 != tapList.end (); iter2++)
        {
          components = *iter2;
          start.WriteU8 (components.first);
          start.WriteU8 (components.second);
        }
    }

  /* Relative Tap Delay List */
  for (TAP_DELAY_LIST_ITERATOR iter = m_tapDelayList.begin (); iter != m_tapDelayList.end (); iter++)
    {
      start.WriteU8 (*iter);
    }

  /* Sector ID Order List */
  SECTOR_ID_ORDER order;
  for (SECTOR_ID_ORDER_LIST_ITERATOR iter = m_sectorIdOrderList.begin (); iter != m_sectorIdOrderList.end (); iter++)
    {
      order = *iter;
      start.WriteU8 (order.first);
      start.WriteU8 (order.second);
    }
}


void
ChannelMeasurementFeedbackElement::AddSnrItem (SNR_INT snr)
{
  m_snrList.push_back (snr);
}

void
ChannelMeasurementFeedbackElement::AddChannelMeasurementItem (TAP_COMPONENTS_LIST taps)
{
  m_channelMeasurementList.push_back (taps);
}

void
ChannelMeasurementFeedbackElement::AddTapDelayItem (TAP_DELAY item)
{
  m_tapDelayList.push_back (item);
}

void
ChannelMeasurementFeedbackElement::AddSectorIdOrder (SECTOR_ID_ORDER order)
{
  m_sectorIdOrderList.push_back (order);
}

void
ChannelMeasurementFeedbackElement::SetSnrListSize (uint16_t size)
{
  m_snrListSize = size;
}

void
ChannelMeasurementFeedbackElement::SetChannelMeasurementSize (uint16_t size)
{
  m_channelMeasurementSize = size;
}

void
ChannelMeasurementFeedbackElement::SetTapComponentsSize (NUMBER_OF_TAPS size)
{
  switch (size)
    {
      case TAPS_1:
        m_tapComponentsSize = 1;
      case TAPS_5:
        m_tapComponentsSize = 5;
      case TAPS_15:
        m_tapComponentsSize = 15;
      case TAPS_63:
        m_tapComponentsSize = 63;
    }
}

void
ChannelMeasurementFeedbackElement::SetTapsDelaySize (NUMBER_OF_TAPS size)
{
  switch (size)
    {
      case TAPS_1:
        m_tapsDelaySize = 1;
      case TAPS_5:
        m_tapsDelaySize = 5;
      case TAPS_15:
        m_tapsDelaySize = 15;
      case TAPS_63:
        m_tapsDelaySize = 63;
    }
}

void
ChannelMeasurementFeedbackElement::SetSectorIdSize (uint16_t size)
{
  m_sectorIdOrderSize = size;
}

SNR_INT_LIST ChannelMeasurementFeedbackElement::GetSnrList(void) const
{
  return m_snrList;
}

CHANNEL_MEASUREMENT_LIST
ChannelMeasurementFeedbackElement::GetChannelMeasurementList (void) const
{
  return m_channelMeasurementList;
}

TAP_DELAY_LIST
ChannelMeasurementFeedbackElement::GetTapDelayList (void) const
{
  return m_tapDelayList;
}

SECTOR_ID_ORDER_LIST
ChannelMeasurementFeedbackElement::GetSectorIdOrderList (void) const
{
  return m_sectorIdOrderList;
}

uint8_t
ChannelMeasurementFeedbackElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  for (int i = 0; i < m_snrListSize; i++)
    {
      SNR_INT snr = start.ReadU8 ();
      m_snrList.push_back (snr);
    }

  for (int i = 0; i < m_channelMeasurementSize; i++)
    {
      TAP_COMPONENTS_LIST tapList;
      for (int j = 0; j < m_tapComponentsSize; j++)
        {
          I_COMPONENT iComp = start.ReadU8 ();
          Q_COMPONENT qComp = start.ReadU8 ();
          tapList.push_back (std::make_pair(iComp,qComp));
        }
      m_channelMeasurementList.push_back (tapList);
    }

  for (int i = 0; i < m_tapsDelaySize; i++)
    {
      TAP_DELAY delay = start.ReadU8 ();
      m_tapDelayList.push_back (delay);
    }

  for (int i = 0; i < m_sectorIdOrderSize; i++)
    {
      uint8_t  value  = start.ReadU8 ();
      SECTOR_ID s = value & 0x3F;
      ANTENNA_ID a = (value >> 6) & 0x3;
      m_sectorIdOrderList.push_back (std::make_pair(s,a));
    }

  
  return length;
}

ATTRIBUTE_HELPER_CPP (ChannelMeasurementFeedbackElement);

std::ostream &
operator << (std::ostream &os, const ChannelMeasurementFeedbackElement &element)
{
  return os;
}

std::istream &
operator >> (std::istream &is, ChannelMeasurementFeedbackElement &element)
{
  return is;
}

/***************************************************
*           Awake Window Element 8.4.2.139
****************************************************/

AwakeWindowElement::AwakeWindowElement ()
  :  m_awakeWindow (0)
{
}

WifiInformationElementId
AwakeWindowElement::ElementId () const
{
  return IE_AWAKE_WINDOW;
}

uint8_t
AwakeWindowElement::GetInformationFieldSize () const
{
  return 2;
}

Buffer::Iterator
AwakeWindowElement::Serialize (Buffer::Iterator i) const
{
  return WifiInformationElement::Serialize (i);
}

uint16_t
AwakeWindowElement::GetSerializedSize () const
{
  return WifiInformationElement::GetSerializedSize ();
}

void
AwakeWindowElement::SerializeInformationField (Buffer::Iterator start) const
{
  start.WriteHtolsbU16 (m_awakeWindow);
}

uint8_t
AwakeWindowElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;
  m_awakeWindow = i.ReadLsbtohU16 ();
  return length;
}

void
AwakeWindowElement::SetAwakeWindow (uint16_t window)
{
  m_awakeWindow = window;
}

uint16_t
AwakeWindowElement::GetAwakeWindow () const
{
  return m_awakeWindow;
}

ATTRIBUTE_HELPER_CPP (AwakeWindowElement);

std::ostream &
operator << (std::ostream &os, const AwakeWindowElement &element)
{
  os <<  element.GetAwakeWindow ();

  return os;
}

std::istream &
operator >> (std::istream &is, AwakeWindowElement &element)
{
  uint16_t c1;
  is >> c1;
  element.SetAwakeWindow (c1);

  return is;
}

/***************************************************
*           Multi-band element 8.4.2.140
****************************************************/

MultiBandElement::MultiBandElement ()
  : m_staRole (ROLE_AP),
    m_staMacAddressPresent (false),
    m_pairWiseCipher (false),
    m_bandID (0),
    m_operatingClass (0),
    m_channelNumber (0),
    m_beaconInterval (0),
    m_tsfOffset (0),
    m_connectionCapability (0),
    m_fstSessionTimeout (0)
{

}

WifiInformationElementId
MultiBandElement::ElementId () const
{
  return IE_MULTI_BAND;
}

uint8_t
MultiBandElement::GetInformationFieldSize () const
{
  uint8_t size = 22;
  if (m_staMacAddressPresent)
    {
      size += 6;
    }
  if (m_pairWiseCipher)
    {
      size += 2;
    }
  return size;
}

void
MultiBandElement::SerializeInformationField (Buffer::Iterator start) const
{
  start.WriteU8 (GetMultiBandControl ());
  start.WriteU8 (m_bandID);
  start.WriteU8 (m_operatingClass);
  start.WriteU8 (m_channelNumber);
  WriteTo (start, m_bssID);
  start.WriteHtolsbU16 (m_beaconInterval);

  start.WriteHtolsbU64 (m_tsfOffset);
  start.WriteU8 (GetConnectionCapability ());
  start.WriteU8 (m_fstSessionTimeout);

  if (m_staMacAddressPresent)
    {
      WriteTo (start, m_staMacAddress);
    }
}

uint8_t
MultiBandElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;

  SetMultiBandControl (i.ReadU8 ());
  m_bandID = i.ReadU8 ();
  m_operatingClass = i.ReadU8 ();
  m_channelNumber = i.ReadU8 ();
  ReadFrom (i, m_bssID);
  m_beaconInterval = i.ReadLsbtohU16 ();

  m_tsfOffset = i.ReadLsbtohU64 ();
  SetConnectionCapability (i.ReadU8 ());
  m_fstSessionTimeout = i.ReadU8 ();

  if (m_staMacAddressPresent)
    ReadFrom (start, m_staMacAddress);

  return length;
}

void
MultiBandElement::SetStaRole (StaRole role)
{
  m_staRole = role;
}

void
MultiBandElement::SetStaMacAddressPresent (bool present)
{
  m_staMacAddressPresent = present;
}

void
MultiBandElement::SetPairwiseCipherSuitePresent (bool present)
{
  m_pairWiseCipher = present;
}

StaRole
MultiBandElement::GetStaRole (void) const
{
  return static_cast<StaRole> (m_staRole);
}

bool
MultiBandElement::IsStaMacAddressPresent (void) const
{
  return m_staMacAddressPresent;
}

bool
MultiBandElement::IsPairwiseCipherSuitePresent (void) const
{
  return m_pairWiseCipher;
}

void
MultiBandElement::SetMultiBandControl (uint8_t control)
{
  m_staRole = control & 0x3;
  m_staMacAddressPresent = (control >> 3) & 0x1;
  m_pairWiseCipher  = (control >> 4) & 0x1;
}

uint8_t
MultiBandElement::GetMultiBandControl () const
{
  uint8_t val = 0;

  val |= m_staRole & 0x3;
  val |= (m_staMacAddressPresent  & 0x1) << 3;
  val |= (m_pairWiseCipher & 0x1) << 4;

  return val;
}

/* Multi-band element Fields */
void
MultiBandElement::SetBandID (BandID id)
{
  m_bandID = id;
}

void
MultiBandElement::SetOperatingClass (uint8_t operating)
{
  m_operatingClass = operating;
}

void
MultiBandElement::SetChannelNumber (uint8_t number)
{
  m_channelNumber = number;
}

void
MultiBandElement::SetBssID (Mac48Address bss)
{
  m_bssID = bss;
}

void
MultiBandElement::SetBeaconInterval (uint16_t interval)
{
  m_beaconInterval = interval;
}

void
MultiBandElement::SetTsfOffset (uint64_t offset)
{
  m_tsfOffset = offset;
}

void
MultiBandElement::SetConnectionCapability (uint8_t capability)
{
  m_connectionCapability = capability;
}

void
MultiBandElement::SetFstSessionTimeout (uint8_t timeout)
{
  m_fstSessionTimeout = timeout;
}

void
MultiBandElement::SetStaMacAddress (Mac48Address address)
{
  m_staMacAddress = address;
}

BandID
MultiBandElement::GetBandID (void) const
{
  return static_cast<BandID> (m_bandID);
}

uint8_t
MultiBandElement::GetOperatingClass (void) const
{
  return m_operatingClass;
}

uint8_t
MultiBandElement::GetChannelNumber (void) const
{
  return m_channelNumber;
}

Mac48Address
MultiBandElement::GetBssID (void) const
{
  return m_bssID;
}

uint16_t
MultiBandElement::GetBeaconInterval (void) const
{
  return m_beaconInterval;
}

uint64_t
MultiBandElement::GetTsfOffset (void) const
{
  return m_tsfOffset;
}

uint8_t
MultiBandElement::GetConnectionCapability () const
{
  return m_connectionCapability;
}

uint8_t
MultiBandElement::GetFstSessionTimeout (void) const
{
  return m_fstSessionTimeout;
}

Mac48Address
MultiBandElement::GetStaMacAddress (void) const
{
  return m_staMacAddress;
}

ATTRIBUTE_HELPER_CPP (MultiBandElement);

std::ostream &
operator << (std::ostream &os, const MultiBandElement &element)
{
  return os;
}

std::istream &
operator >> (std::istream &is, MultiBandElement &element)
{
  uint8_t c1, c2, c3, c4;

  is >> c1 >> c2 >> c3 >> c4;
  element.SetMultiBandControl (c1);
  element.SetBandID (static_cast<BandID> (c2));
  element.SetOperatingClass (c3);
  element.SetChannelNumber (c4);
  //element.SetBssID ();
  return is;
}

/***************************************************
*          Next PCP List Element 8.4.2.142
****************************************************/

NextPcpListElement::NextPcpListElement ()
  : m_token (0)
{
}

WifiInformationElementId
NextPcpListElement::ElementId () const
{
  return IE_NEXT_PCP_LIST;
}

uint8_t
NextPcpListElement::GetInformationFieldSize () const
{
  uint8_t size = 1;
  size += m_list.size ();
  return size;
}

void
NextPcpListElement::SerializeInformationField (Buffer::Iterator start) const
{
  start.WriteU8 (m_token);
  for (NextPcpAidList::const_iterator item = m_list.begin (); item != m_list.end (); item++)
    {
      start.WriteU8 (*item);
    }
}

uint8_t
NextPcpListElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;
  uint8_t value;

  m_token = i.ReadU8 ();
  while (!i.IsEnd ())
    {
      value = i.ReadU8 ();
      m_list.push_back (value);
    }
  return length;
}

void
NextPcpListElement::SetToken (uint8_t token)
{
  m_token = token;
}

void
NextPcpListElement::AddNextPcpAid (uint8_t aid)
{
  m_list.push_back (aid);
}

uint8_t
NextPcpListElement::GetToken () const
{
  return m_token;
}

NextPcpAidList
NextPcpListElement::GetListOfNextPcpAid (void) const
{
  return m_list;
}

ATTRIBUTE_HELPER_CPP (NextPcpListElement);

std::ostream &
operator << (std::ostream &os, const NextPcpListElement &element)
{
  return os;
}

std::istream &
operator >> (std::istream &is, NextPcpListElement &element)
{
  uint8_t c1;
  is >> c1;

  element.SetToken (c1);

  return is;
}

/***************************************************
*          PCP Handover Element 8.4.2.143
****************************************************/

PcpHandoverElement::PcpHandoverElement ()
  : m_remaining (0)
{
}

WifiInformationElementId
PcpHandoverElement::ElementId () const
{
  return IE_PCP_HANDOVER;
}

uint8_t
PcpHandoverElement::GetInformationFieldSize () const
{
  return 13;
}

void
PcpHandoverElement::SerializeInformationField (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  WriteTo (i, m_oldBssID);
  WriteTo (i, m_newPcpAddress);
  start.WriteU8 (m_remaining);
}

uint8_t
PcpHandoverElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;

  ReadFrom (i, m_oldBssID);
  ReadFrom (i, m_newPcpAddress);
  m_remaining = i.ReadU8 ();

  return length;
}

void
PcpHandoverElement::SetOldBssID (Mac48Address id)
{
  m_oldBssID = id;
}

void
PcpHandoverElement::SetNewPcpAddress (Mac48Address address)
{
  m_newPcpAddress = address;
}

void
PcpHandoverElement::SetRemainingBIs (uint8_t number)
{
  m_remaining = number;
}

Mac48Address
PcpHandoverElement::GetOldBssID (void) const
{
  return m_oldBssID;
}

Mac48Address
PcpHandoverElement::GetNewPcpAddress (void) const
{
  return m_newPcpAddress;
}

uint8_t
PcpHandoverElement::PcpHandoverElement::GetRemainingBIs (void) const
{
  return m_remaining;
}

ATTRIBUTE_HELPER_CPP (PcpHandoverElement);

std::ostream &
operator << (std::ostream &os, const PcpHandoverElement &element)
{
  return os;
}

std::istream &
operator >> (std::istream &is, PcpHandoverElement &element)
{
  return is;
}

/***************************************************
*          DMG Link Margin element 8.4.2.144
****************************************************/

LinkMarginElement::LinkMarginElement ()
  : m_activity (NO_CHANGE_PREFFERED),
    m_mcs (),
    m_linkMargin (0),
    m_snr (0),
    m_timestamp (0)
{
}

WifiInformationElementId LinkMarginElement::ElementId () const
{
  return IE_DMG_LINK_MARGIN;
}

uint8_t
LinkMarginElement::GetInformationFieldSize () const
{
  return 8;
}

void
LinkMarginElement::SerializeInformationField (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (static_cast<uint8_t> (m_activity));
  i.WriteU8 (m_mcs);
  i.WriteU8 (m_linkMargin);
  i.WriteU8 (m_snr);
  i.WriteHtolsbU32 (m_timestamp);
}

uint8_t
LinkMarginElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;
  m_activity = static_cast<Activity> (i.ReadU8 ());
  m_mcs = i.ReadU8 ();
  m_linkMargin = i.ReadU8 ();
  m_snr = i.ReadU8 ();
  m_timestamp = i.ReadLsbtohU32 ();
  return length;
}

void
LinkMarginElement::SetActivity (Activity activity)
{
  m_activity = activity;
}

void
LinkMarginElement::SetMcs (uint8_t mcs)
{
  m_mcs = mcs;
}

void
LinkMarginElement::SetLinkMargin (uint8_t margin)
{
  m_linkMargin = margin;
}

void
LinkMarginElement::SetSnr (uint8_t snr)
{
  m_snr = snr;
}

void
LinkMarginElement::SetReferenceTimestamp (uint32_t timestamp)
{
  m_timestamp = timestamp;
}

Activity
LinkMarginElement::GetActivity (void) const
{
  return m_activity;
}

uint8_t
LinkMarginElement::GetMcs (void) const
{
  return m_mcs;
}

uint8_t
LinkMarginElement::GetLinkMargin (void) const
{
  return m_linkMargin;
}

uint8_t
LinkMarginElement::GetSnr (void) const
{
  return m_snr;
}

uint32_t
LinkMarginElement::GetReferenceTimestamp (void) const
{
  return m_timestamp;
}

ATTRIBUTE_HELPER_CPP (LinkMarginElement);

std::ostream &
operator << (std::ostream &os, const LinkMarginElement &element)
{
  return os;
}

std::istream &
operator >> (std::istream &is, LinkMarginElement &element)
{
  return is;
}

/******************************************************
* DMG Link Adaptation Acknowledgment element 8.4.2.145
*******************************************************/

LinkAdaptationAcknowledgment::LinkAdaptationAcknowledgment ()
{
}

WifiInformationElementId LinkAdaptationAcknowledgment::ElementId () const
{
  return IE_DMG_LINK_ADAPTATION_ACKNOWLEDGMENT;
}

uint8_t
LinkAdaptationAcknowledgment::GetInformationFieldSize () const
{
  return 5;
}

void
LinkAdaptationAcknowledgment::SerializeInformationField (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (static_cast<uint8_t> (m_activity));
  i.WriteHtolsbU32 (m_timestamp);
}

uint8_t
LinkAdaptationAcknowledgment::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;
  m_activity = static_cast<Activity> (i.ReadU8 ());
  m_timestamp = i.ReadLsbtohU32 ();
  return length;
}

void
LinkAdaptationAcknowledgment::SetActivity (Activity activity)
{
  m_activity = activity;
}

void
LinkAdaptationAcknowledgment::SetReferenceTimestamp (uint32_t timestamp)
{
  m_timestamp = timestamp;
}

Activity
LinkAdaptationAcknowledgment::GetActivity (void) const
{
  return m_activity;
}

uint32_t
LinkAdaptationAcknowledgment::GetReferenceTimestamp (void) const
{
  return m_timestamp;
}

ATTRIBUTE_HELPER_CPP (LinkAdaptationAcknowledgment);

std::ostream &
operator << (std::ostream &os, const LinkAdaptationAcknowledgment &element)
{
  return os;
}

std::istream &
operator >> (std::istream &is, LinkAdaptationAcknowledgment &element)
{
  return is;
}

/***************************************************
*          Switching Stream Element 8.4.2.146
****************************************************/

SwitchingStreamElement::SwitchingStreamElement ()
  : m_oldBandID (0),
    m_newBandID (0),
    m_nonQosDataFrames (0),
    m_numberOfStreamsSwitching (0)
{

}

WifiInformationElementId
SwitchingStreamElement::ElementId () const
{
  return IE_SWITCHING_STREAM;
}

uint8_t
SwitchingStreamElement::GetInformationFieldSize () const
{
  uint8_t size = 4;
  size += m_numberOfStreamsSwitching * 2;
  return size;
}

void
SwitchingStreamElement::SerializeInformationField (Buffer::Iterator start) const
{
  struct SwitchingParameters parameters;
  uint16_t value;
  start.WriteU8 (m_oldBandID);
  start.WriteU8 (m_newBandID);
  start.WriteU8 (m_nonQosDataFrames);
  start.WriteU8 (m_numberOfStreamsSwitching);
  for (SwitchingParametersList::const_iterator item = m_switchingParametersList.begin (); item != m_switchingParametersList.end (); item++)
    {
      parameters = (struct SwitchingParameters) *item;

      value = parameters.OldBandStreamID.TID & 0xF;
      value |= (parameters.OldBandStreamID.direction & 0x1) << 4;
      value |= (parameters.NewBandStreamID.TID & 0xF) << 5;
      value |= (parameters.NewBandStreamID.direction & 0x1) << 9;
      value |= (parameters.IsNewBandValid & 0x1) << 10;
      value |= (parameters.LltType & 0x1) << 11;
      value |= (parameters.Reserved & 0xF) << 12;

      start.WriteHtolsbU16 (value);
    }
}

uint8_t
SwitchingStreamElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;
  uint8_t counter;
  uint16_t value;
  struct SwitchingParameters parameters;

  m_oldBandID = i.ReadU8 ();
  m_newBandID = i.ReadU8 ();
  m_nonQosDataFrames = i.ReadU8 ();
  m_numberOfStreamsSwitching = i.ReadU8 ();

  /* Read Switching Parameters */
  counter = m_numberOfStreamsSwitching;
  while (counter != 0 )
    {
      value = i.ReadLsbtohU16 ();

      parameters.OldBandStreamID.TID = value & 0xF;
      parameters.OldBandStreamID.direction = (value >> 4) & 0x1;
      parameters.NewBandStreamID.TID = (value >> 5) & 0xF;
      parameters.NewBandStreamID.direction = (value >> 9) & 0x1;
      parameters.IsNewBandValid = (value >> 10) & 0x1;
      parameters.LltType = (value >> 11) & 0x1;
      parameters.Reserved = (value >> 12) & 0xF;

      m_switchingParametersList.push_back (parameters);

      counter--;
    }
  return length;
}

void
SwitchingStreamElement::SetOldBandID (BandID id)
{
  m_oldBandID = id;
}

void
SwitchingStreamElement::SetNewBandID (BandID id)
{
  m_newBandID = id;
}

void
SwitchingStreamElement::SetNonQosDataFrames (uint8_t frames)
{
  m_nonQosDataFrames = frames;
}

void
SwitchingStreamElement::SetNumberOfStreamsSwitching (uint8_t number)
{
  m_numberOfStreamsSwitching = number;
}

BandID
SwitchingStreamElement::GetOldBandID (void) const
{
  return static_cast<BandID> (m_oldBandID);
}

BandID
SwitchingStreamElement::GetNewBandID (void) const
{
  return static_cast<BandID> (m_newBandID);
}

uint8_t
SwitchingStreamElement::GetNonQosDataFrames (void) const
{
  return m_nonQosDataFrames;
}

uint8_t
SwitchingStreamElement::GetNumberOfStreamsSwitching (void) const
{
  return m_numberOfStreamsSwitching;
}

void
SwitchingStreamElement::AddSwitchingParametersField (struct SwitchingParameters &parameters)
{
  m_switchingParametersList.push_back (parameters);
}

SwitchingParametersList
SwitchingStreamElement::GetSwitchingParametersList (void) const
{
  return m_switchingParametersList;
}

ATTRIBUTE_HELPER_CPP (SwitchingStreamElement);

std::ostream &
operator << (std::ostream &os, const SwitchingStreamElement &element)
{
  return os;
}

std::istream &
operator >> (std::istream &is, SwitchingStreamElement &element)
{
  uint8_t c1, c2, c3, c4;
  is >> c1 >> c2 >> c3 >> c4;

  element.SetOldBandID (static_cast<BandID> (c1));
  element.SetNewBandID (static_cast<BandID> (c2));
  element.SetNonQosDataFrames (c3);
  element.SetNumberOfStreamsSwitching (c4);

  return is;
}

/***************************************************
*        Session Transition Element 8.4.2.147
****************************************************/

SessionTransitionElement::SessionTransitionElement ()
  : m_fstsID (0),
    m_sessionType (SessionType_InfrastructureBSS),
    m_switchIntent (0)
{
}

WifiInformationElementId
SessionTransitionElement::ElementId () const
{
  return IE_SESSION_TRANSITION;
}

uint8_t
SessionTransitionElement::GetInformationFieldSize () const
{
  return 11;
}

void
SessionTransitionElement::SerializeInformationField (Buffer::Iterator start) const
{
  start.WriteHtolsbU32 (m_fstsID);
  start.WriteU8 (GetSessionControl ());
  /* New Band Field */
  start.WriteU8 (m_newBand.Band_ID);
  start.WriteU8 (m_newBand.Setup);
  start.WriteU8 (m_newBand.Operation);
  /* Old Band Field */
  start.WriteU8 (m_oldBand.Band_ID);
  start.WriteU8 (m_oldBand.Setup);
  start.WriteU8 (m_oldBand.Operation);
}

uint8_t
SessionTransitionElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;
  uint32_t id = i.ReadLsbtohU32 ();
  uint8_t ctrl = i.ReadU8 ();

  SetFstsID (id);
  SetSessionControl (ctrl);
  Band band;
  /* Set New Band */
  band.Band_ID = i.ReadU8 ();
  band.Operation = i.ReadU8 ();
  band.Setup = i.ReadU8 ();
  SetNewBand (band);
  /* Set Old Band */
  band.Band_ID = i.ReadU8 ();
  band.Operation = i.ReadU8 ();
  band.Setup = i.ReadU8 ();
  SetOldBand (band);

  return length;
}

void
SessionTransitionElement::SetSessionControl (uint8_t ctrl)
{
  m_sessionType = static_cast<SessionType> (ctrl & 0xF);
  m_switchIntent = (ctrl >> 4) & 0x1;
}

uint8_t
SessionTransitionElement::GetSessionControl (void) const
{
  uint8_t val = 0;

  val |= m_sessionType & 0xF;
  val |= (m_switchIntent & 0x1) << 4;

  return val;
}

void
SessionTransitionElement::SetFstsID (uint32_t id)
{
  m_fstsID = id;
}

void
SessionTransitionElement::SetSessionControl (SessionType sessionType, bool switchIntent)
{
  m_sessionType = sessionType;
  m_switchIntent = switchIntent;
}

void
SessionTransitionElement::SetNewBand (Band &newBand)
{
  m_newBand = newBand;
}

void
SessionTransitionElement::SetOldBand (Band &oldBand)
{
  m_oldBand = oldBand;
}

uint32_t
SessionTransitionElement::GetFstsID (void) const
{
  return m_fstsID;
}

SessionType
SessionTransitionElement::GetSessionType (void) const
{
  return m_sessionType;
}

bool
SessionTransitionElement::GetSwitchIntenet (void) const
{
  return m_switchIntent;
}

Band
SessionTransitionElement::GetNewBand (void) const
{
  return m_newBand;
}

Band
SessionTransitionElement::GetOldBand (void) const
{
  return m_oldBand;
}

ATTRIBUTE_HELPER_CPP (SessionTransitionElement);

std::ostream &
operator << (std::ostream &os, const SessionTransitionElement &element)
{
  os <<  element.GetFstsID () << "|" << element.GetSessionControl ();

  return os;
}

std::istream &
operator >> (std::istream &is, SessionTransitionElement &element)
{
  uint32_t c1;
  uint8_t c2;
  is >> c1 >> c2;
  element.SetFstsID (c1);
  element.SetSessionControl (c2);

  return is;
}

/***************************************************
*   Cluster Report Element Field (Figure 8-401ax)
****************************************************/

ClusterReportElement::ClusterReportElement ()
  : m_clusterRequest (false),
    m_clusterReport (false),
    m_schedulePresent (false),
    m_tsconstPresent (false),
    m_ecpacPolicyEnforced (false),
    m_ecpacPolicyPresent (false)
{
}

WifiInformationElementId
ClusterReportElement::ElementId () const
{
  return IE_CLUSTER_REPORT;
}

uint8_t
ClusterReportElement::GetInformationFieldSize () const
{
  uint8_t size = 0;
  size +=1;
  if (m_clusterReport)
    {
      size += 18;
      if (m_ecpacPolicyPresent)
        {

        }
      if (m_schedulePresent)
        {
          size += m_scheduleElement.GetSerializedSize ();
        }
      if (m_tsconstPresent)
        {
          size += 1 + m_constraintList.size () * 14;
        }
    }
  return size;
}

void
ClusterReportElement::SerializeInformationField (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  uint8_t reportControl = 0;

  /** Cluster Report Control **/
  reportControl |= m_clusterRequest & 0x1;
  reportControl |= (m_clusterReport & 0x1) << 1;
  reportControl |= (m_schedulePresent & 0x1) << 2;
  reportControl |= (m_tsconstPresent & 0x1) << 3;
  reportControl |= (m_ecpacPolicyEnforced & 0x1) << 4;
  reportControl |= (m_ecpacPolicyPresent & 0x1) << 5;
  i.WriteU8 (reportControl);

  /* Element Body */
  if (m_clusterReport)
    {
      WriteTo (i, m_bssID);
      i.WriteHtolsbU32 (m_timestamp);
      i = m_clusteringControl.Serialize (i);
      if (m_ecpacPolicyPresent)
        {

        }
      if (m_schedulePresent)
        {
          i = m_scheduleElement.Serialize (i);
        }
      if (m_tsconstPresent)
        {
          i.WriteU8 (m_constraintList.size ());
          for (ConstraintListCI it = m_constraintList.begin (); it != m_constraintList.end (); it++)
            {
              i = it->Serialize (i);
            }
        }
    }
}

uint8_t
ClusterReportElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;
  uint8_t reportControl = 0;

  /** Cluster Report Control **/
  reportControl = i.ReadU8 ();
  m_clusterRequest = reportControl & 0x1;
  m_clusterReport = (reportControl >> 1) & 0x1;
  m_schedulePresent = (reportControl >> 2) & 0x1;
  m_tsconstPresent = (reportControl >> 3) & 0x1;
  m_ecpacPolicyEnforced = (reportControl >> 4) & 0x1;
  m_ecpacPolicyPresent = (reportControl >> 5) & 0x1;

  /* Element Body */
  if (m_clusterReport)
    {
      ReadFrom (i, m_bssID);
      m_timestamp = i.ReadLsbtohU32 ();
      i = m_clusteringControl.Deserialize (i);
      if (m_ecpacPolicyPresent)
        {

        }
      if (m_schedulePresent)
        {
          i = m_scheduleElement.Deserialize (i);
        }
      if (m_tsconstPresent)
        {
          uint8_t numberOfConstraints = i.ReadU8 ();
          while (numberOfConstraints != 0)
            {
              ConstraintSubfield constraint;
              i = constraint.Deserialize (i);
              m_constraintList.push_back (constraint);
              numberOfConstraints--;
            }
        }
    }
  return length;
}

void
ClusterReportElement::SetClusterRequest (bool request)
{
  m_clusterReport = request;
}

void
ClusterReportElement::SetClusterReport (bool report)
{
  m_clusterReport = report;
}

void
ClusterReportElement::SetSchedulePresent (bool present)
{
  m_schedulePresent = present;
}

void
ClusterReportElement::SetTsConstPresent (bool present)
{
  m_tsconstPresent = present;
}

void
ClusterReportElement::SetEcpacPolicyEnforced (bool enforced)
{
  m_ecpacPolicyEnforced = enforced;
}

void
ClusterReportElement::SetEcpacPolicyPresent (bool present)
{
  m_ecpacPolicyPresent = present;
}

void
ClusterReportElement::SetReportedBssID (Mac48Address bssid)
{
  m_bssID = bssid;
}

void
ClusterReportElement::SetReferenceTimestamp (uint32_t timestamp)
{
  m_timestamp = timestamp;
}

void
ClusterReportElement::SetClusteringControl (ExtDMGClusteringControlField &field)
{
  m_clusteringControl = field;
}

void
ClusterReportElement::SetEcpacPolicyElement ()
{

}

void
ClusterReportElement::AddTrafficSchedulingConstraint (ConstraintSubfield &constraint)
{
  NS_ASSERT_MSG (m_constraintList.size () <= 15, "Cannot add more than 15 TSCONST fields");
  m_constraintList.push_back (constraint);
}

bool
ClusterReportElement::GetClusterRequest (void) const
{
  return m_clusterRequest;
}

bool
ClusterReportElement::GetClusterReport (void) const
{
  return m_clusterReport;
}

bool
ClusterReportElement::GetSchedulePresent (void) const
{
  return m_schedulePresent;
}

bool
ClusterReportElement::GetTsConstPresent (void) const
{
  return m_tsconstPresent;
}

bool
ClusterReportElement::GetEcpacPolicyEnforced (void) const
{
  return m_ecpacPolicyEnforced;
}

bool
ClusterReportElement::GetEcpacPolicyPresent (void) const
{
  return m_ecpacPolicyPresent;
}

Mac48Address
ClusterReportElement::GetReportedBssID (void) const
{
  return m_bssID;
}

uint32_t
ClusterReportElement::GetReferenceTimestamp (void) const
{
  return m_timestamp;
}

ExtDMGClusteringControlField
ClusterReportElement::GetClusteringControl (void) const
{
  return m_clusteringControl;
}

void
ClusterReportElement::GetEcpacPolicyElement (void) const
{
}

ExtendedScheduleElement
ClusterReportElement::GetExtendedScheduleElement (void) const
{
  return m_scheduleElement;
}

uint8_t
ClusterReportElement::GetNumberOfContraints (void) const
{
  return m_constraintList.size ();
}

ConstraintList
ClusterReportElement::GetTrafficSchedulingConstraintList (void) const
{
  return m_constraintList;
}

ATTRIBUTE_HELPER_CPP (ClusterReportElement);

std::ostream
&operator << (std::ostream &os, const ClusterReportElement &element)
{
  return os;
}

std::istream
&operator >> (std::istream &is, ClusterReportElement &element)
{
  return is;
}

/***************************************************
*   Relay Capabilities Info field (Figure 8-401ba)
****************************************************/

RelayCapabilitiesInfo::RelayCapabilitiesInfo ()
  : m_supportability (0),
    m_usability (0),
    m_permission (0),
    m_acPower (0),
    m_preference (0),
    m_duplex (0),
    m_cooperation (0)
{
}

void
RelayCapabilitiesInfo::Print (std::ostream &os) const
{

}

uint32_t
RelayCapabilitiesInfo::GetSerializedSize () const
{
  return 2;
}

Buffer::Iterator
RelayCapabilitiesInfo::Serialize (Buffer::Iterator start) const
{
  uint16_t val = 0;
  val |= m_supportability & 0x1;
  val |= (m_usability & 0x1) << 1;
  val |= (m_permission & 0x1) << 2;
  val |= (m_acPower & 0x1) << 3;
  val |= (m_preference & 0x1) << 4;
  val |= (m_duplex & 0x3) << 5;
  val |= (m_cooperation & 0x1) << 7;
  start.WriteHtolsbU16 (val);
  return start;
}

Buffer::Iterator
RelayCapabilitiesInfo::Deserialize (Buffer::Iterator start)
{
  uint16_t info = start.ReadLsbtohU16 ();
  m_supportability = info & 0x1;
  m_usability = (info >> 1) & 0x1;
  m_permission = (info >> 2) & 0x1;
  m_acPower = (info >> 3) & 0x1;
  m_preference = (info >> 4) & 0x1;
  m_duplex = (info >> 5) & 0x3;
  m_cooperation = (info >> 7) & 0x1;
  return start;
}

void
RelayCapabilitiesInfo::SetRelaySupportability (bool value)
{
  m_supportability = value;
}

void
RelayCapabilitiesInfo::SetRelayUsability (bool value)
{
  m_usability = value;
}

void
RelayCapabilitiesInfo::SetRelayPermission (bool value)
{
  m_permission = value;
}

void
RelayCapabilitiesInfo::SetAcPower (bool value)
{
  m_acPower = value;
}

void
RelayCapabilitiesInfo::SetRelayPreference (bool value)
{
  m_preference = value;
}

void
RelayCapabilitiesInfo::SetDuplex (RelayDuplexMode duplex)
{
  m_duplex = duplex;
}

void
RelayCapabilitiesInfo::SetCooperation (bool value)
{
  m_cooperation = value;
}

bool
RelayCapabilitiesInfo::GetRelaySupportability (void) const
{
  return m_supportability;
}

bool
RelayCapabilitiesInfo::GetRelayUsability (void) const
{
  return m_usability;
}

bool
RelayCapabilitiesInfo::GetRelayPermission (void) const
{
  return m_permission;
}

bool
RelayCapabilitiesInfo::GetAcPower (void) const
{
  return m_acPower;
}

bool
RelayCapabilitiesInfo::GetRelayPreference (void) const
{
  return m_preference;
}

RelayDuplexMode
RelayCapabilitiesInfo::GetDuplex (void) const
{
  return static_cast<RelayDuplexMode> (m_duplex);
}

bool
RelayCapabilitiesInfo::GetCooperation (void) const
{
  return m_cooperation;
}

/***************************************************
*       Relay Capabilities Elemenet 8.4.2.150
****************************************************/

RelayCapabilitiesElement::RelayCapabilitiesElement ()
  : m_info ()
{
}

WifiInformationElementId
RelayCapabilitiesElement::ElementId () const
{
  return IE_RELAY_CAPABILITIES;
}

uint8_t
RelayCapabilitiesElement::GetInformationFieldSize () const
{
  return m_info.GetSerializedSize ();
}

void
RelayCapabilitiesElement::SerializeInformationField (Buffer::Iterator start) const
{
  m_info.Serialize (start);
}

uint8_t
RelayCapabilitiesElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;
  i = m_info.Deserialize (i);
  return i.GetDistanceFrom (start);
}

void
RelayCapabilitiesElement::SetRelayCapabilitiesInfo (RelayCapabilitiesInfo &info)
{
  m_info = info;
}

RelayCapabilitiesInfo
RelayCapabilitiesElement::GetRelayCapabilitiesInfo (void) const
{
  return m_info;
}

ATTRIBUTE_HELPER_CPP (RelayCapabilitiesElement);

std::ostream &
operator << (std::ostream &os, const RelayCapabilitiesElement &element)
{
  return os;
}

std::istream &
operator >> (std::istream &is, RelayCapabilitiesElement &element)
{
  return is;
}

/***************************************************
*  Relay Transfer Parameter Set Elemenet 8.4.2.151
****************************************************/

RelayTransferParameterSetElement::RelayTransferParameterSetElement ()
  : m_duplex (false),
    m_cooperation (false),
    m_txMode (false),
    m_changeInterval (0),
    m_sensingTime (0),
    m_firstPeriod (0),
    m_secondPeriod (0)
{
}

WifiInformationElementId
RelayTransferParameterSetElement::ElementId () const
{
  return IE_RELAY_TRANSFER_PARAMETER_SET;
}

uint8_t
RelayTransferParameterSetElement::GetInformationFieldSize () const
{
  return 8;
}

void
RelayTransferParameterSetElement::SerializeInformationField (Buffer::Iterator start) const
{
  start.WriteHtolsbU64 (GetRelayTransferParameter ());
}

uint8_t
RelayTransferParameterSetElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;
  uint64_t parameter = i.ReadLsbtohU64 ();
  SetRelayTransferParameter (parameter);
  return length;
}

void
RelayTransferParameterSetElement::SetRelayTransferParameter (uint64_t value)
{
  m_duplex = value & 0x1;
  m_cooperation = (value >> 1) & 0x1;
  m_txMode  = (value >> 2) & 0x1;
  m_changeInterval = (value >> 8) & 0xFF;
  m_sensingTime = (value >> 16) & 0xFF;
  m_firstPeriod = (value >> 24) & 0xFFFF;
  m_secondPeriod = (value >> 40) & 0xFFFF;
}

uint64_t
RelayTransferParameterSetElement::GetRelayTransferParameter (void) const
{
  uint64_t val = 0;

  val |= m_duplex & 0x1;
  val |= (m_cooperation  & 0x1) << 1;
  val |= (m_txMode & 0x1) << 2;
  val |= m_changeInterval << 8;
  val |= m_sensingTime << 16;
  val |= uint64_t (m_firstPeriod) << 24;
  val |= uint64_t (m_secondPeriod) << 40;

  return val;
}

void
RelayTransferParameterSetElement::SetDuplexMode (bool mode)
{
  m_duplex = mode;
}

void
RelayTransferParameterSetElement::SetCooperationMode (bool mode)
{
  m_cooperation = mode;
}

void
RelayTransferParameterSetElement::SetTxMode (bool mode)
{
  m_txMode = mode;
}

void
RelayTransferParameterSetElement::SetLinkChangeInterval (uint8_t interval)
{
  m_changeInterval = interval;
}

void
RelayTransferParameterSetElement::SetDataSensingTime (uint8_t time)
{
  m_sensingTime = time;
}

void
RelayTransferParameterSetElement::SetFirstPeriod (uint16_t period)
{
  m_firstPeriod = period;
}

void
RelayTransferParameterSetElement::SetSecondPeriod (uint16_t period)
{
  m_secondPeriod = period;
}

bool
RelayTransferParameterSetElement::GetDuplexMode (void) const
{
  return m_duplex;
}

bool
RelayTransferParameterSetElement::GetCooperationMode (void) const
{
  return m_cooperation;
}

bool
RelayTransferParameterSetElement::GetTxMode (void) const
{
  return m_txMode;
}

uint8_t
RelayTransferParameterSetElement::GetLinkChangeInterval (void) const
{
  return m_changeInterval;
}

uint8_t
RelayTransferParameterSetElement::GetDataSensingTime (void) const
{
  return m_sensingTime;
}

uint16_t
RelayTransferParameterSetElement::GetFirstPeriod (void) const
{
  return m_firstPeriod;
}

uint16_t
RelayTransferParameterSetElement::GetSecondPeriod (void) const
{
  return m_secondPeriod;
}

ATTRIBUTE_HELPER_CPP (RelayTransferParameterSetElement);

std::ostream &
operator << (std::ostream &os, const RelayTransferParameterSetElement &element)
{
  return os;
}

std::istream &
operator >> (std::istream &is, RelayTransferParameterSetElement &element)
{
  return is;
}

/***************************************************
*       Quiet Period Request Elemenet 8.4.2.152
****************************************************/

QuietPeriodRequestElement::QuietPeriodRequestElement ()
  : m_token (0),
    m_quietPeriodOffset (0),
    m_quietPeriod (0),
    m_quietDuration (0),
    m_repetitionCount (0)
{
}

WifiInformationElementId
QuietPeriodRequestElement::ElementId () const
{
  return IE_QUIET_PERIOD_REQUEST;
}

uint8_t
QuietPeriodRequestElement::GetInformationFieldSize () const
{
  return 17;
}

void
QuietPeriodRequestElement::SerializeInformationField (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;

  i.WriteHtolsbU16 (m_token);
  i.WriteHtolsbU16 (m_quietPeriodOffset);
  i.WriteHtolsbU32 (m_quietPeriod);
  i.WriteHtolsbU16 (m_quietDuration);
  i.WriteU8 (m_repetitionCount);
  WriteTo (i, m_targetBssID);
}

uint8_t
QuietPeriodRequestElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;

  m_token = i.ReadLsbtohU16 ();
  m_quietPeriodOffset = i.ReadLsbtohU16 ();
  m_quietPeriod = i.ReadLsbtohU32 ();
  m_quietDuration = i.ReadLsbtohU16 ();
  m_repetitionCount = i.ReadU8 ();
  ReadFrom (i, m_targetBssID);

  return length;
}

void
QuietPeriodRequestElement::SetRequestToken (uint16_t token)
{
  m_token = token;
}

void
QuietPeriodRequestElement::SetQuietPeriodOffset (uint16_t offset)
{
  m_quietPeriodOffset = offset;
}

void
QuietPeriodRequestElement::SetQuietPeriod (uint32_t period)
{
  m_quietPeriod = period;
}

void
QuietPeriodRequestElement::SetQuietDuration (uint16_t duration)
{
  m_quietDuration = duration;
}

void
QuietPeriodRequestElement::SetRepetitionCount (uint8_t count)
{
  m_repetitionCount = count;
}

void
QuietPeriodRequestElement::SetTargetBssID (Mac48Address id)
{
  m_targetBssID = id;
}

uint16_t
QuietPeriodRequestElement::GetRequestToken (void) const
{
  return m_token;
}

uint16_t
QuietPeriodRequestElement::GetQuietPeriodOffset (void) const
{
  return m_quietPeriodOffset;
}

uint32_t
QuietPeriodRequestElement::GetQuietPeriod (void) const
{
  return m_quietPeriod;
}

uint16_t
QuietPeriodRequestElement::GetQuietDuration (void) const
{
  return m_quietDuration;
}

uint8_t
QuietPeriodRequestElement::GetRepetitionCount (void) const
{
  return m_repetitionCount;
}

Mac48Address
QuietPeriodRequestElement::GetTargetBssID (void) const
{
  return m_targetBssID;
}

ATTRIBUTE_HELPER_CPP (QuietPeriodRequestElement);

std::ostream &
operator << (std::ostream &os, const QuietPeriodRequestElement &element)
{
  return os;
}

std::istream &
operator >> (std::istream &is, QuietPeriodRequestElement &element)
{
  return is;
}

/***************************************************
*       Quiet Period Response Elemenet 8.4.2.153
****************************************************/

QuietPeriodResponseElement::QuietPeriodResponseElement ()
  : m_token (0),
    m_status (0)
{
}

WifiInformationElementId
QuietPeriodResponseElement::ElementId () const
{
  return IE_QUIET_PERIOD_RESPONSE;
}

uint8_t
QuietPeriodResponseElement::GetInformationFieldSize () const
{
  return 10;
}

void
QuietPeriodResponseElement::SerializeInformationField (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;

  i.WriteHtolsbU16 (m_token);
  WriteTo (i, m_bssID);
  i.WriteHtolsbU16 (m_status);
}

uint8_t
QuietPeriodResponseElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;

  m_token = i.ReadLsbtohU16 ();
  ReadFrom (i, m_bssID);
  m_status = i.ReadLsbtohU16 ();

  return length;
}

void
QuietPeriodResponseElement::SetRequestToken (uint16_t token)
{
  m_token = token;
}

void
QuietPeriodResponseElement::SetBssID (Mac48Address id)
{
  m_bssID = id;
}

void
QuietPeriodResponseElement::SetStatusCode (uint16_t code)
{
  m_status = code;
}

uint16_t
QuietPeriodResponseElement::GetRequestToken (void) const
{
  return m_token;
}

Mac48Address
QuietPeriodResponseElement::GetBssID (void) const
{
  return m_bssID;
}

uint16_t
QuietPeriodResponseElement::GetStatusCode (void) const
{
  return m_status;
}

ATTRIBUTE_HELPER_CPP (QuietPeriodResponseElement);

std::ostream &
operator << (std::ostream &os, const QuietPeriodResponseElement &element)
{
  return os;
}

std::istream &
operator >> (std::istream &is, QuietPeriodResponseElement &element)
{
  return is;
}

/***************************************************
*           ECPAC Policy Element 8.4.2.157
****************************************************/

EcpacPolicyElement::EcpacPolicyElement ()
  : m_bhiEnforced (false),
    m_txssCbapEnforced (false),
    m_protectedPeriodEnforced (false),
    m_timestampOffsetBitmap (0),
    m_txssCbapOffset (0),
    m_txssCbapDuration (0),
    m_txssCbapMaxMem (0)
{
}

WifiInformationElementId
EcpacPolicyElement::ElementId () const
{
  return IE_ECPAC_POLICY;
}

uint8_t
EcpacPolicyElement::GetInformationFieldSize () const
{
  uint8_t size = 11;
  if (m_txssCbapEnforced)
    {
      size += 4;
    }
  return size;
}

void
EcpacPolicyElement::SerializeInformationField (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  uint8_t policyDetail = 0;
  policyDetail |= m_bhiEnforced & 0x1;
  policyDetail |= (m_txssCbapEnforced & 0x1) << 1;
  policyDetail |= m_bhiEnforced & 0x1;
  i.WriteU8 (policyDetail);
  WriteTo (i, m_ccsrID);
  i.WriteHtolsbU32 (m_timestampOffsetBitmap);
  if (m_txssCbapEnforced)
    {
      i.WriteHtolsbU16 (m_txssCbapOffset);
      i.WriteU8 (m_txssCbapDuration);
      i.WriteU8 (m_txssCbapMaxMem);
    }
}

uint8_t
EcpacPolicyElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;
  uint8_t policyDetail =   i.ReadU8 ();
  m_bhiEnforced = (policyDetail & 0x1);
  m_txssCbapEnforced = (policyDetail >> 1) & 0x1;
  m_bhiEnforced = (policyDetail >> 2) & 0x1;
  ReadFrom (i, m_ccsrID);
  m_timestampOffsetBitmap = i.ReadLsbtohU32 ();
  if (m_txssCbapEnforced)
    {
      m_txssCbapOffset = i.ReadLsbtohU16 ();
      m_txssCbapDuration = i.ReadU8 ();
      m_txssCbapMaxMem = i.ReadU8 ();
    }
  return length;
}

void
EcpacPolicyElement::SetBhiEnforced (bool enforced)
{
  m_bhiEnforced = enforced;
}

void
EcpacPolicyElement::SetTxssCbapEnforced (bool enforced)
{
  m_txssCbapEnforced = enforced;
}

void
EcpacPolicyElement::SetProtectedPeriodEnforced (bool enforced)
{
  m_protectedPeriodEnforced = enforced;
}

void
EcpacPolicyElement::SetCCSRID (Mac48Address ccsrID)
{
  m_ccsrID = ccsrID;
}

void
EcpacPolicyElement::SetTimestampOffsetBitmap (uint32_t bitmap)
{
  m_timestampOffsetBitmap = bitmap;
}

void
EcpacPolicyElement::SetTxssCbapOffset (uint16_t offset)
{
  m_txssCbapOffset = offset;
}

void
EcpacPolicyElement::SetTxssCbapDuration (uint8_t duration)
{
  m_txssCbapDuration = duration;
}

void
EcpacPolicyElement::SetTxssCbapMaxMem (uint8_t max)
{
  m_txssCbapMaxMem = max;
}

bool
EcpacPolicyElement::GetBhiEnforced (void) const
{
  return m_bhiEnforced;
}

bool
EcpacPolicyElement::GetTxssCbapEnforced (void) const
{
  return m_txssCbapEnforced;
}

bool
EcpacPolicyElement::GetProtectedPeriodEnforced (void) const
{
  return m_protectedPeriodEnforced;
}

Mac48Address
EcpacPolicyElement::GetCCSRID (void) const
{
  return m_ccsrID;
}

uint32_t
EcpacPolicyElement::GetTimestampOffsetBitmap (void) const
{
  return m_timestampOffsetBitmap;
}

uint16_t
EcpacPolicyElement::GetTxssCbapOffset (void) const
{
  return m_txssCbapOffset;
}

uint8_t
EcpacPolicyElement::GetTxssCbapDuration (void) const
{
  return m_txssCbapDuration;
}

uint8_t
EcpacPolicyElement::GetTxssCbapMaxMem (void) const
{
  return m_txssCbapMaxMem;
}

ATTRIBUTE_HELPER_CPP (EcpacPolicyElement);

std::ostream
&operator << (std::ostream &os, const EcpacPolicyElement &element)
{
  return os;
}

std::istream
&operator >> (std::istream &is, EcpacPolicyElement &element)
{
  return is;
}

/***************************************************
*           EDMG Operation Element 9.4.2.251
****************************************************/

EdmgOperationElement::EdmgOperationElement ()
  : m_primaryChannel (0),
    m_bssAid (0),
    m_rssRetryLimit (0),
    m_rssBackoff (0),
    m_bssOperatingChannels (EDMG_CHANNEL_BTIMAP_CH2),
    m_channelBWConfig (EDMG_BW_216)
{
}

WifiInformationElementId
EdmgOperationElement::ElementId () const
{
  return IE_EXTENSION;
}

WifiInformationElementId
EdmgOperationElement::ElementIdExt () const
{
  return IE_EXTENSION_EDMG_OPERATION;
}

uint8_t
EdmgOperationElement::GetInformationFieldSize () const
{
  return 6;
}

void
EdmgOperationElement::SerializeInformationField (Buffer::Iterator start) const
{
  uint8_t abftParameters = 0;
  abftParameters |= (m_rssRetryLimit & 0x2);
  abftParameters |= (m_rssBackoff & 0x2) << 2;
  start.WriteU8 (m_primaryChannel);
  start.WriteU8 (m_bssAid);
  start.WriteU8 (abftParameters);
  start.WriteU8 (m_bssOperatingChannels);
  start.WriteU8 (m_channelBWConfig);
}

uint8_t
EdmgOperationElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  uint8_t abftParameters ;
  m_primaryChannel = start.ReadU8 ();
  m_bssAid = start.ReadU8 ();
  abftParameters = start.ReadU8 ();
  m_rssRetryLimit = (abftParameters & 0x2);
  m_rssBackoff = (abftParameters >> 2) & 0x2;
  m_bssOperatingChannels = static_cast<EdmgOperatingChannels> (start.ReadU8 ());
  m_channelBWConfig = static_cast<EdmgChannelBwConfiguration> (start.ReadU8 ());
  return length;
}

void
EdmgOperationElement::SetPrimaryChannel (uint8_t ch)
{
  m_primaryChannel = ch;
}

void
EdmgOperationElement::SetBssAid (uint8_t aid)
{
  m_bssAid = aid;
}

void
EdmgOperationElement::SetRssRetryLimit (uint8_t limit)
{
  m_rssRetryLimit = limit;
}

void
EdmgOperationElement::SetRssBackoff (uint8_t backoff)
{
  m_rssBackoff = backoff;
}

void
EdmgOperationElement::SetBssOperatingChannels (EdmgOperatingChannels channels)
{
  m_bssOperatingChannels = channels;
}

void
EdmgOperationElement::SetChannelBWConfiguration (EdmgChannelBwConfiguration bwConfig)
{
  m_channelBWConfig = bwConfig;
}

uint8_t
EdmgOperationElement::GetPrimaryChannel (void) const
{
  return m_primaryChannel;
}

uint8_t
EdmgOperationElement::GetBssAid (void) const
{
  return m_bssAid;
}

uint8_t
EdmgOperationElement::GetRssRetryLimit (void) const
{
  return m_rssRetryLimit;
}

uint8_t
EdmgOperationElement::GetRssBackoff (void) const
{
  return m_rssBackoff;
}

EdmgOperatingChannels
EdmgOperationElement::GetBssOperatingChannels (void) const
{
  return m_bssOperatingChannels;
}

EdmgChannelBwConfiguration
EdmgOperationElement::GetChannelBWConfiguration (void) const
{
  return m_channelBWConfig;
}

bool
EdmgOperationElement::IsChannelBWSupported (EdmgChannelBwConfiguration config) const
{
  return true;
}

ATTRIBUTE_HELPER_CPP (EdmgOperationElement);

std::ostream &operator << (std::ostream &os, const EdmgOperationElement &element)
{
  return os;
}

std::istream &operator >> (std::istream &is, EdmgOperationElement &element)
{
  return is;
}

/***************************************************
*EDMG Channel Measurement Feedback Element 9.4.2.253
****************************************************/

EDMGChannelMeasurementFeedbackElement::EDMGChannelMeasurementFeedbackElement ()
{
  m_sectorIdOrderSize = 0;
  m_brpCdownSize = 0;
  m_tapsDelaySize = 0;
}

WifiInformationElementId
EDMGChannelMeasurementFeedbackElement::ElementId () const
{
  return IE_EXTENSION;
}

WifiInformationElementId
EDMGChannelMeasurementFeedbackElement::ElementIdExt () const
{
  return IE_EXTENSION_EDMG_CHANNEL_MEASUREMENT_FEEDBACK;
}

uint8_t
EDMGChannelMeasurementFeedbackElement::GetInformationFieldSize () const
{
  /* Not accurate Information Field Size. The size of the EDMG Sector ID Order is 17 bits,
   * the size of the BRP CDOWN  is 6 bits and the size of the Tap Delay is 12 bits. However since we can
   * only write on the buffers using bytes, it´s necessary to round up */
  uint8_t size = 1;     // Extension Element ID
  uint16_t numBits = 0;
  numBits += m_edmgSectorIDOrder_List.size () * 24;
  numBits += m_brp_CDOWN_List.size () * 8;
  numBits += m_tap_Delay_List.size () * 16;
  size += std::ceil (numBits/8);
  return size;
}

void
EDMGChannelMeasurementFeedbackElement::SerializeInformationField (Buffer::Iterator start) const
{
  for (EDMGSectorIDOrder_ListCI it = m_edmgSectorIDOrder_List.begin (); it != m_edmgSectorIDOrder_List.end (); it++)
    {
      uint16_t value1 = 0;
      uint8_t value2 = 0;
      value1 |= (it->SectorID & 0x7FF);
      value2 |= (it->TXAntennaID & 0x7);
      value2 |= (it->RXAntennaID & 0x7) << 3;
      start.WriteHtolsbU16 (value1);
      start.WriteU8 (value2);
    }
  for (BRP_CDOWN_LIST_CI it = m_brp_CDOWN_List.begin (); it != m_brp_CDOWN_List.end (); it++)
    {
      start.WriteU8 ((*it));
    }
  for (Tap_Delay_List_CI it = m_tap_Delay_List.begin (); it != m_tap_Delay_List.end (); it++)
    {
      start.WriteHtolsbU16 ((*it));
    }
}

uint8_t
EDMGChannelMeasurementFeedbackElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  for (int i = 0; i < m_sectorIdOrderSize; i++ )
    {
      uint16_t value1 = start.ReadLsbtohU16 ();
      uint8_t value2 = start.ReadU8 ();
      EDMGSectorIDOrder sectorID;
      sectorID.SectorID = value1 & 0x7FF;
      sectorID.TXAntennaID = value2 & 0x7;
      sectorID.RXAntennaID = (value2 >> 3) & 0x7;
      m_edmgSectorIDOrder_List.push_back (sectorID);
    }
  for (int i = 0; i < m_brpCdownSize; i ++ )
    {
      BRP_CDOWN cdown = start.ReadU8 ();
      m_brp_CDOWN_List.push_back (cdown);
    }
  for (int i = 0; i < m_tapsDelaySize; i++)
    {
      Tap_Delay tapDelay = start.ReadLsbtohU16 ();
      m_tap_Delay_List.push_back (tapDelay);
    }

  return length;
}

void
EDMGChannelMeasurementFeedbackElement::Add_EDMG_SectorIDOrder (EDMGSectorIDOrder &order)
{
  m_edmgSectorIDOrder_List.push_back (order);
}

void
EDMGChannelMeasurementFeedbackElement::Add_BRP_CDOWN (BRP_CDOWN brpCdown)
{
  m_brp_CDOWN_List.push_back (brpCdown);
}

void
EDMGChannelMeasurementFeedbackElement::Add_Tap_Delay (Tap_Delay tapDelay)
{
  m_tap_Delay_List.push_back (tapDelay);
}

void
EDMGChannelMeasurementFeedbackElement::SetSectorIdOrderSize (uint16_t number)
{
  m_sectorIdOrderSize = number;
}

void
EDMGChannelMeasurementFeedbackElement::SetBrpCdownSize (uint16_t number)
{
  m_brpCdownSize = number;
}

void
EDMGChannelMeasurementFeedbackElement::SetTapsDelaySize (NUMBER_OF_TAPS number)
{
  switch (number)
    {
      case TAPS_1:
        m_tapsDelaySize = 1;
      case TAPS_5:
        m_tapsDelaySize = 5;
      case TAPS_15:
        m_tapsDelaySize = 15;
      case TAPS_63:
        m_tapsDelaySize = 63;
    }
}

EDMGSectorIDOrder_List
EDMGChannelMeasurementFeedbackElement::Get_EDMG_SectorIDOrder_List (void) const
{
  return m_edmgSectorIDOrder_List;
}

BRP_CDOWN_LIST
EDMGChannelMeasurementFeedbackElement::Get_BRP_CDOWN_List (void) const
{
  return m_brp_CDOWN_List;
}

Tap_Delay_List
EDMGChannelMeasurementFeedbackElement::Get_Tap_Delay_List (void) const
{
  return m_tap_Delay_List;
}

ATTRIBUTE_HELPER_CPP (EDMGChannelMeasurementFeedbackElement);

std::ostream &operator << (std::ostream &os, const EDMGChannelMeasurementFeedbackElement &element)
{
  return os;
}

std::istream &operator >> (std::istream &is, EDMGChannelMeasurementFeedbackElement &element)
{
  return is;
}

/***************************************************
*         EDMG Group ID Set Element 9.4.2.254
****************************************************/

EDMGGroupIDSetElement::EDMGGroupIDSetElement ()
  : m_numEDMGGroups (0)
{
}

WifiInformationElementId
EDMGGroupIDSetElement::ElementId () const
{
  return IE_EXTENSION;
}

WifiInformationElementId
EDMGGroupIDSetElement::ElementIdExt () const
{
  return IE_EXTENSION_EDMG_GROUP_ID_SET;
}

uint8_t
EDMGGroupIDSetElement::GetInformationFieldSize () const
{
  uint8_t size = 0;
  size += 2;  /* 2 Bytes: Extension Element ID + Number of Groups */
  for (EDMGGroupTuplesCI it = m_edmgGroupTuples.begin (); it != m_edmgGroupTuples.end (); it++)
    {
      size += 2 + it->aidList.size ();
    }
  return size;
}

void
EDMGGroupIDSetElement::SerializeInformationField (Buffer::Iterator start) const
{
  uint8_t groupSize;
  start.WriteU8 (m_numEDMGGroups);
  for (EDMGGroupTuplesCI it = m_edmgGroupTuples.begin (); it != m_edmgGroupTuples.end (); it++)
    {
      start.WriteU8 (it->groupID);
      groupSize = it->aidList.size () & 0x1F;
      start.WriteU8 (groupSize);
      for (AID_LIST_CI aidIT = it->aidList.begin (); aidIT != it->aidList.end (); aidIT++)
        {
          start.WriteU8 ((*aidIT));
        }
    }
}

uint8_t
EDMGGroupIDSetElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  uint8_t size = 0;
  m_numEDMGGroups = start.ReadU8 ();
  for (uint8_t i = 0; i < m_numEDMGGroups; i++)
    {
      EDMGGroupTuple tuple;
      tuple.groupID = start.ReadU8 ();
      size = start.ReadU8 () & 0x1F;
      for (uint8_t j = 0; j < size; j++)
        {
          tuple.aidList.push_back (start.ReadU8 ());
        }
      m_edmgGroupTuples.push_back (tuple);
    }
  return length;
}

void
EDMGGroupIDSetElement::SetNumberofEDMGGroups (uint8_t number)
{
  m_numEDMGGroups = number;
}

void
EDMGGroupIDSetElement::AddEDMGGroupTuple (EDMGGroupTuple &tuple)
{
  m_edmgGroupTuples.push_back (tuple);
}

uint8_t
EDMGGroupIDSetElement::GetNumberofEDMGGroups (void) const
{
  return m_numEDMGGroups;
}

EDMGGroupTuples
EDMGGroupIDSetElement::GetEDMGGroupTuples (void) const
{
  return m_edmgGroupTuples;
}

ATTRIBUTE_HELPER_CPP (EDMGGroupIDSetElement);

std::ostream &operator << (std::ostream &os, const EDMGGroupIDSetElement &element)
{
  return os;
}

std::istream &operator >> (std::istream &is, EDMGGroupIDSetElement &element)
{
  return is;
}

/***************************************************
* EDMG Partial Sector Level Sweep Element 9.4.2.257
****************************************************/

EdmgPartialSectorLevelSweep::EdmgPartialSectorLevelSweep ()
  :  m_partialNumberOfSectors (0),
    m_totalNumberOfSectors (0),
    m_partialNumberOfRxAntennas (0),
    m_totalNumberOfRxAntennas (0),
    m_timeToSwitchToFullSectorSweep (0),
    m_agreeToChangeRoles (false),
    m_agreeToPartialSectorSweep (false)
{
}

WifiInformationElementId
EdmgPartialSectorLevelSweep::ElementId () const
{
 return IE_EXTENSION;
}

WifiInformationElementId
EdmgPartialSectorLevelSweep::ElementIdExt () const
{
  return IE_EXTENSION_EDMG_PARTIAL_SECTOR_SWEEP;
}

uint8_t
EdmgPartialSectorLevelSweep::GetInformationFieldSize () const
{
  return 5;
}

void
EdmgPartialSectorLevelSweep::SerializeInformationField (Buffer::Iterator start) const
{
  uint32_t value = 0;
  value |= (m_partialNumberOfSectors & 0x3F);
  value |= (m_totalNumberOfSectors & 0x7FF) << 6;
  value |= (m_partialNumberOfRxAntennas & 0x7) << 17;
  value |= (m_totalNumberOfRxAntennas & 0x7) << 20;
  value |= (m_timeToSwitchToFullSectorSweep & 0xF) << 23;
  value |= (m_agreeToChangeRoles & 0x1) << 27;
  value |= (m_agreeToPartialSectorSweep & 0x1) << 28;
  start.WriteHtolsbU32 (value);
}

uint8_t
EdmgPartialSectorLevelSweep::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  uint32_t value = start.ReadLsbtohU32 ();
  m_partialNumberOfSectors = (value & 0x3F);
  m_totalNumberOfSectors = (value >> 6) & 0x7FF;
  m_partialNumberOfRxAntennas = (value >> 17) & 0x7;
  m_totalNumberOfRxAntennas = (value >> 20) & 0x7;
  m_timeToSwitchToFullSectorSweep = (value >> 23) & 0xF;
  m_agreeToChangeRoles = (value >> 27) & 0x1;
  m_agreeToPartialSectorSweep = (value >> 28) & 0x1;
  return length;
}

void
EdmgPartialSectorLevelSweep::SetPartialNumberOfSectors (uint8_t sectors)
{
  m_partialNumberOfSectors = sectors;
}

void
EdmgPartialSectorLevelSweep::SetTotalNumberOfSectors (uint16_t sectors)
{
  m_totalNumberOfSectors = sectors;
}

void
EdmgPartialSectorLevelSweep::SetPartialNumberOfRxAntennas (uint8_t antennas)
{
  m_partialNumberOfRxAntennas = antennas;
}

void
EdmgPartialSectorLevelSweep::SetTotalNumberOfRxAntennas (uint8_t antennas)
{
  m_totalNumberOfRxAntennas = antennas;
}

void
EdmgPartialSectorLevelSweep::SetTimeToSwitchToFullSectorSweep (uint8_t time)
{
  m_timeToSwitchToFullSectorSweep = time;
}

void
EdmgPartialSectorLevelSweep::SetAgreeToChangeRoles (bool agree)
{
  m_agreeToChangeRoles = agree;
}

void EdmgPartialSectorLevelSweep::SetAgreeToPartialSectorSweep (bool agree)
{
  m_agreeToPartialSectorSweep = agree;
}

uint8_t
EdmgPartialSectorLevelSweep::GetPartialNumberOfSectors (void) const
{
  return m_partialNumberOfSectors;
}

uint16_t
EdmgPartialSectorLevelSweep::GetTotalNumberOfSectors (void) const
{
  return m_totalNumberOfSectors;
}

uint8_t
EdmgPartialSectorLevelSweep::GetPartialNumberOfRxAntennas (void) const
{
  return m_partialNumberOfRxAntennas;
}

uint8_t
EdmgPartialSectorLevelSweep::GetTotalNumberOfRxAntennas (void) const
{
  return m_totalNumberOfRxAntennas;
}

uint8_t
EdmgPartialSectorLevelSweep::GetTimeToSwitchToFullSectorSweep (void) const
{
  return m_timeToSwitchToFullSectorSweep;
}

bool
EdmgPartialSectorLevelSweep::GetAgreeToChangeRoles (void) const
{
  return m_agreeToChangeRoles;
}

bool
EdmgPartialSectorLevelSweep::GetAgreeToPartialSectorSweep (void) const
{
  return m_agreeToPartialSectorSweep;
}

ATTRIBUTE_HELPER_CPP (EdmgPartialSectorLevelSweep);

std::ostream &operator << (std::ostream &os, const EdmgPartialSectorLevelSweep &element)
{
  return os;
}

std::istream &operator >> (std::istream &is, EdmgPartialSectorLevelSweep &element)
{
  return is;
}

/***************************************************
*         MIMO Setup Control Element 9.4.2.258
****************************************************/

MimoSetupControlElement::MimoSetupControlElement ()
  : m_mimoBeamformingType (MU_MIMO_BEAMFORMING),
    m_mimoPhaseType (MIMO_PHASE_RECPIROCAL),
    m_edmgGroupID (0),
    m_groupUserMask (0),
    m_LTxRx (0),
    m_requestedEDMGTRNUnitM (0),
    m_isInitiator (false),
    m_channelMeasurementRequested (false),
    m_numberOfTapsRequested (TAPS_1),
    m_numberOfTXSectorCombinationsRequested (0),
    m_channelAggregationRequested (false)
{
}

WifiInformationElementId
MimoSetupControlElement::ElementId () const
{
  return IE_EXTENSION;
}

WifiInformationElementId
MimoSetupControlElement::ElementIdExt () const
{
  return IE_EXTENSION_MIMO_SETUP_CONTROL;
}

uint8_t
MimoSetupControlElement::GetInformationFieldSize () const
{
  return 10;
}

void
MimoSetupControlElement::SerializeInformationField (Buffer::Iterator start) const
{
  uint64_t val1 = 0;
  uint8_t val2 = 0;
  val1 |= (m_mimoBeamformingType & 0x1);
  val1 |= (m_mimoPhaseType & 0x1) << 1;
  val1 |= (m_edmgGroupID) << 2;
  val1 |= (m_groupUserMask) << 10;
  val1 |= uint64_t (m_LTxRx) << 42;
  val1 |= uint64_t (m_requestedEDMGTRNUnitM & 0xF) << 50;
  val1 |= uint64_t (m_isInitiator & 0x1) << 54;
  val1 |= uint64_t (m_channelMeasurementRequested & 0x1) << 55;
  val1 |= uint64_t (m_numberOfTapsRequested & 0x3) << 56;
  val1 |= uint64_t (m_numberOfTXSectorCombinationsRequested & 0x1F) << 59;
  val2 |= (m_numberOfTXSectorCombinationsRequested >> 5) & 0x1;
  val2 |= (m_channelAggregationRequested & 0x1) << 1;
  start.WriteHtolsbU64 (val1);
  start.WriteU8 (val2);
}

uint8_t
MimoSetupControlElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  uint64_t val1 = start.ReadLsbtohU64 ();
  uint8_t val2 = start.ReadU8 ();
  m_mimoBeamformingType = static_cast<MimoBeamformingType> (val1 & 0x1);
  m_mimoPhaseType = static_cast<MimoPhaseType> ((val1 >> 1) & 0x1);
  m_edmgGroupID = (val1 >> 2 ) & 0xFF;
  m_groupUserMask = (val1 >> 10) & 0xFFFFFFFF;
  m_LTxRx = (val1 >> 42) & 0xFF;
  m_requestedEDMGTRNUnitM = (val1 >> 50) & 0xF;
  m_isInitiator = (val1 >> 54) & 0x1;
  m_channelMeasurementRequested = (val1 >> 55) & 0x1;
  m_numberOfTapsRequested = static_cast<NUMBER_OF_TAPS> ((val1 >> 56) & 0x3);
  m_numberOfTXSectorCombinationsRequested |= (val1 >> 59) & 0x1F;
  m_numberOfTXSectorCombinationsRequested |= (val2 & 0x1) << 5;
  m_channelAggregationRequested = (val2 >> 1) & 0x1;
  return length;
}

void
MimoSetupControlElement::SetMimoBeamformingType (MimoBeamformingType type)
{
  m_mimoBeamformingType = type;
}

void
MimoSetupControlElement::SetMimoPhaseType (MimoPhaseType type)
{
  m_mimoPhaseType = type;
}

void
MimoSetupControlElement::SetEDMGGroupID (uint8_t groupID)
{
  m_edmgGroupID = groupID;
}

void
MimoSetupControlElement::SetGroupUserMask (uint32_t mask)
{
  m_groupUserMask = mask;
}

void
MimoSetupControlElement::SetLTxRx (uint8_t number)
{
  m_LTxRx = number;
}

void
MimoSetupControlElement::SetRequestedEDMGTRNUnitM (uint8_t unit)
{
  m_requestedEDMGTRNUnitM = unit - 1;
}

void
MimoSetupControlElement::SetAsInitiator (bool isInitiator)
{
  m_isInitiator = isInitiator;
}

void
MimoSetupControlElement::SetChannelMeasurementRequested (bool value)
{
  m_channelMeasurementRequested = value;
}

void
MimoSetupControlElement::SetNumberOfTapsRequested (NUMBER_OF_TAPS taps)
{
  m_numberOfTapsRequested = taps;
}

void
MimoSetupControlElement::SetNumberOfTXSectorCombinationsRequested (uint8_t requested)
{
  m_numberOfTXSectorCombinationsRequested = requested;
}

void
MimoSetupControlElement::SetChannelAggregationRequested (bool requested)
{
  m_channelAggregationRequested = requested;
}

MimoBeamformingType
MimoSetupControlElement::GetMimoBeamformingType (void) const
{
  return m_mimoBeamformingType;
}

MimoPhaseType
MimoSetupControlElement::GetMimoPhaseType (void) const
{
  return m_mimoPhaseType;
}

uint8_t
MimoSetupControlElement::GetEDMGGroupID (void) const
{
  return m_edmgGroupID;
}

uint32_t
MimoSetupControlElement::GetGroupUserMask (void) const
{
  return m_groupUserMask;
}

uint8_t
MimoSetupControlElement::GetLTxRx (void) const
{
  return m_LTxRx;
}

uint8_t
MimoSetupControlElement::GetRequestedEDMGTRNUnitM (void) const
{
  return m_requestedEDMGTRNUnitM + 1;
}

bool
MimoSetupControlElement::IsInitiator (void) const
{
  return m_isInitiator;
}

bool
MimoSetupControlElement::IsChannelMeasurementRequested (void) const
{
  return m_channelMeasurementRequested;
}

NUMBER_OF_TAPS
MimoSetupControlElement::GetNumberOfTapsRequested (void) const
{
  return m_numberOfTapsRequested;
}

uint8_t
MimoSetupControlElement::GetNumberOfTXSectorCombinationsRequested (void) const
{
  return m_numberOfTXSectorCombinationsRequested;
}

bool
MimoSetupControlElement::IsChannelAggregationRequested (void) const
{
  return m_channelAggregationRequested;
}

ATTRIBUTE_HELPER_CPP (MimoSetupControlElement);

std::ostream &operator << (std::ostream &os, const MimoSetupControlElement &element)
{
  return os;
}

std::istream &operator >> (std::istream &is, MimoSetupControlElement &element)
{
  return is;
}

/***************************************************
*         MIMO Poll Control Element 9.4.2.259
****************************************************/

MimoPollControlElement::MimoPollControlElement ()
  : m_mimoBeamformingType (SU_MIMO_BEAMFORMING),
    m_pollType (POLL_TRAINING_PACKET),
    m_LTxRx (0),
    m_requestedEDMGTRNUnitM (0),
    m_requestedEDMGTRNUnitP (0),
    m_duration (0)
{
}

WifiInformationElementId
MimoPollControlElement::ElementId () const
{
  return IE_EXTENSION;
}

WifiInformationElementId
MimoPollControlElement::ElementIdExt () const
{
  return IE_EXTENSION_MIMO_POLL_CONTROL;
}

uint8_t
MimoPollControlElement::GetInformationFieldSize () const
{
  return 5;
}

void
MimoPollControlElement::SerializeInformationField (Buffer::Iterator start) const
{
  uint32_t val = 0;
  val |= (m_mimoBeamformingType & 0x1);
  val |= (m_pollType & 0x1) << 1;
  val |= (m_LTxRx) << 2;
  val |= (m_requestedEDMGTRNUnitM & 0xF) << 10;
  val |= (m_requestedEDMGTRNUnitP & 0x3) << 14;
  val |= (m_duration & 0x3FFF) << 16;
  start.WriteHtolsbU32 (val);
}

uint8_t
MimoPollControlElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  uint32_t val = start.ReadLsbtohU32 ();
  m_mimoBeamformingType = static_cast<MimoBeamformingType> (val & 0x1);
  m_pollType = static_cast<PollType> ((val >> 1) & 0x1);
  m_LTxRx = (val >> 2) & 0xFF;
  m_requestedEDMGTRNUnitM = (val >> 10) & 0xF;
  m_requestedEDMGTRNUnitP = (val >> 14) & 0x3;
  m_duration = (val >> 16) & 0x3FFF;
  return length;
}

void
MimoPollControlElement::SetMimoBeamformingType (MimoBeamformingType type)
{
  m_mimoBeamformingType = type;
}

void
MimoPollControlElement::SetPollType (PollType type)
{
  m_pollType = type;
}

void
MimoPollControlElement::SetLTxRx (uint8_t number)
{
  m_LTxRx = number;
}

void
MimoPollControlElement::SetRequestedEDMGTRNUnitM (uint8_t unit)
{
  m_requestedEDMGTRNUnitM = unit;
}

void
MimoPollControlElement::SetRequestedEDMGTRNUnitP (uint8_t unit)
{
  m_requestedEDMGTRNUnitP = unit;
}

void
MimoPollControlElement::SetTrainingDuration (uint16_t duration)
{
  m_duration = duration;
}

MimoBeamformingType
MimoPollControlElement::GetMimoBeamformingType (void) const
{
  return m_mimoBeamformingType;
}

PollType
MimoPollControlElement::GetPollType (void) const
{
  return m_pollType;
}

uint8_t
MimoPollControlElement::GetLTxRx (void) const
{
  return m_LTxRx;
}

uint8_t
MimoPollControlElement::GetRequestedEDMGTRNUnitM (void) const
{
  return m_requestedEDMGTRNUnitM;
}

uint8_t
MimoPollControlElement::GetRequestedEDMGTRNUnitP (void) const
{
  return m_requestedEDMGTRNUnitP;
}

uint16_t
MimoPollControlElement::GetTrainingDuration (void) const
{
  return m_duration;
}

ATTRIBUTE_HELPER_CPP (MimoPollControlElement);

std::ostream &operator << (std::ostream &os, const MimoPollControlElement &element)
{
  return os;
}

std::istream &operator >> (std::istream &is, MimoPollControlElement &element)
{
  return is;
}

/***************************************************
*        MIMO Feedback Control Element 9.4.2.260
****************************************************/

MIMOFeedbackControl::MIMOFeedbackControl ()
  : m_mimoBeamformingType (SU_MIMO_BEAMFORMING),
    m_linkType (false),
    m_comebackDelay (0),
    m_isChannelMeasurementPresent (false),
    m_isTapDelayPresent (false),
    m_numberOfTapsPresent (TAPS_1),
    m_nummberOfTXSectorCombinationsPresent (0),
    m_isPrecodingInformationPresent (false),
    m_ssChannelAggregationPresent (false),
    m_numberOfColumns (0),
    m_numberOfRows (0),
    m_txAntennaMask (0),
    m_numberOfContiguousChannels (0),
    m_grouping (0),
    m_codebookInformationType (false),
    m_feedbackType (false),
    m_numberOfFeedbackMatricesOrTaps (0)
{
}

WifiInformationElementId
MIMOFeedbackControl::ElementId () const
{
  return IE_EXTENSION;
}

WifiInformationElementId
MIMOFeedbackControl::ElementIdExt () const
{
  return IE_EXTENSION_MIMO_FEEDBACK_CONTROL;
}

uint8_t
MIMOFeedbackControl::GetInformationFieldSize () const
{
  //// NINA ////
  return 8;
  //// NINA ////
}

//// NINA ////
void
MIMOFeedbackControl::SerializeInformationField (Buffer::Iterator start) const
{
  uint16_t val1 = 0;
  uint8_t val2 = 0;
  uint32_t val3 = 0;
  val1 |= (m_mimoBeamformingType & 0x1);
  val1 |= (m_linkType & 0x1) << 1;
  val1 |= (m_comebackDelay & 0x7) << 2;
  val1 |= (m_isChannelMeasurementPresent & 0x1) << 5;
  val1 |= (m_isTapDelayPresent & 0x1) << 6;
  val1 |= (m_numberOfTapsPresent & 0x3) << 7;
  val1 |= (m_nummberOfTXSectorCombinationsPresent & 0x3F) << 9;
  val1 |= (m_isPrecodingInformationPresent & 0x1) << 15;

  val2 |= (m_ssChannelAggregationPresent & 0x1);
  val2 |= (m_numberOfTxAntennas & 0x7) << 1;
  val2 |= (m_numberOfRxAntennas & 0x7) << 4;

  val3 |= (m_numberOfColumns & 0x7);
  val3 |= (m_numberOfRows & 0x7) << 3;
  val3 |= (m_txAntennaMask & 0xFF) << 6;
  val3 |= (m_numberOfContiguousChannels & 0x3) << 14;
  val3 |= (m_grouping & 0x3) << 16;
  val3 |= (m_codebookInformationType & 0x1) << 18;
  val3 |= (m_feedbackType & 0x1) << 19;
  val3 |= (m_numberOfFeedbackMatricesOrTaps & 0x3FF) << 20;

  start.WriteHtolsbU16 (val1);
  start.WriteU8 (val2);
  start.WriteHtolsbU32 (val3);
}

uint8_t
MIMOFeedbackControl::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  uint16_t val1 = start.ReadLsbtohU16 ();
  uint8_t val2 = start.ReadU8 ();
  uint32_t val3 = start.ReadLsbtohU32 ();

  m_mimoBeamformingType = static_cast<MimoBeamformingType> (val1 & 0x1);
  m_linkType = (val1 >> 1) & 0x1;
  m_comebackDelay = (val1 >> 2) & 0x7;
  m_isChannelMeasurementPresent = (val1 >> 5) & 0x1;
  m_isTapDelayPresent = (val1 >> 6) & 0x1;
  m_numberOfTapsPresent = static_cast<NUMBER_OF_TAPS> ((val1 >> 7) & 0x3);
  m_nummberOfTXSectorCombinationsPresent = (val1 >> 9) & 0x3F;
  m_isPrecodingInformationPresent = (val1 >> 15) & 0x1;

  m_ssChannelAggregationPresent = (val2 & 0x1);
  m_numberOfTxAntennas = (val2 >> 1) & 0x7;
  m_numberOfRxAntennas = (val2 >> 4) & 0x7;

  m_numberOfColumns = (val3 & 0x7);
  m_numberOfRows = (val3 >> 3) & 0x7;
  m_txAntennaMask = (val3 >> 6) & 0xFF;
  m_numberOfContiguousChannels = (val3 >> 14) & 0x3;
  m_grouping = (val3 >> 16) & 0x3;
  m_codebookInformationType = (val3 >> 18) & 0x1;
  m_feedbackType = (val3 >> 19) & 0x1;
  m_numberOfFeedbackMatricesOrTaps = (val3 >> 20) & 0x3FF;

  return length;
}
//// NINA ////


void
MIMOFeedbackControl::SetMimoBeamformingType (MimoBeamformingType type)
{
  m_mimoBeamformingType = type;
}

void
MIMOFeedbackControl::SetLinkTypeAsInitiator (bool isInitiator)
{
  m_linkType = isInitiator;
}

void
MIMOFeedbackControl::SetComebackDelay (uint8_t delay)
{
  m_comebackDelay = delay;
}

void
MIMOFeedbackControl::SetChannelMeasurementPresent (bool present)
{
  m_isChannelMeasurementPresent = present;
}

void
MIMOFeedbackControl::SetTapDelayPresent (bool present)
{
  m_isTapDelayPresent = present;
}

void
MIMOFeedbackControl::SetNumberOfTapsPresent (NUMBER_OF_TAPS taps)
{
  m_numberOfTapsPresent = taps;
}

void
MIMOFeedbackControl::SetNumberOfTXSectorCombinationsPresent (uint8_t present)
{
  m_nummberOfTXSectorCombinationsPresent = present - 1;
}

void
MIMOFeedbackControl::SetPrecodingInformationPresent (bool present)
{
  m_isPrecodingInformationPresent = present;
}

void
MIMOFeedbackControl::SetChannelAggregationPresent (bool present)
{
  m_isChannelMeasurementPresent = present;
}

//// NINA ////
void
MIMOFeedbackControl::SetNumberOfTxAntennas (uint8_t number)
{
  m_numberOfTxAntennas = number;
}

void
MIMOFeedbackControl::SetNumberOfRxAntennas (uint8_t number)
{
  m_numberOfRxAntennas = number;
}
//// NINA ////


void
MIMOFeedbackControl::SetNumberOfColumns (uint8_t nc)
{
  m_numberOfColumns = nc;
}

void
MIMOFeedbackControl::SetNumberOfRows (uint8_t nr)
{
  m_numberOfRows = nr;
}

void
MIMOFeedbackControl::SetTxAntennaMask (uint8_t mask)
{
  m_txAntennaMask = mask;
}

void
MIMOFeedbackControl::SetNumberOfContiguousChannels (uint8_t ncb)
{
  m_numberOfContiguousChannels = ncb;
}

void
MIMOFeedbackControl::SetGrouping (uint8_t ng)
{
  m_grouping = ng;
}

void
MIMOFeedbackControl::SetCodebookInformationType (bool type)
{
  m_codebookInformationType = type;
}

void
MIMOFeedbackControl::SetFeedbackType (bool type)
{
  m_feedbackType = type;
}

void
MIMOFeedbackControl::SetNumberOfFeedbackMatricesOrTaps (uint16_t number)
{
  m_numberOfFeedbackMatricesOrTaps = number;
}

MimoBeamformingType
MIMOFeedbackControl::SetMimoBeamformingType (void) const
{
  return m_mimoBeamformingType;
}

bool
MIMOFeedbackControl::IsLinkTypeAsInitiator (void) const
{
  return m_linkType;
}

uint8_t
MIMOFeedbackControl::SetComebackDelay (void) const
{
  return m_comebackDelay;
}

bool
MIMOFeedbackControl::IsChannelMeasurementPresent (void) const
{
  return m_isChannelMeasurementPresent;
}

bool
MIMOFeedbackControl::IsTapDelayPresent (void) const
{
  return m_isTapDelayPresent;
}

NUMBER_OF_TAPS
MIMOFeedbackControl::GetNumberOfTapsPresent (void) const
{
  return m_numberOfTapsPresent;
}

uint8_t
MIMOFeedbackControl::GetNumberOfTXSectorCombinationsPresent (void) const
{
  return m_nummberOfTXSectorCombinationsPresent + 1;
}

bool
MIMOFeedbackControl::IsPrecodingInformationPresent (void) const
{
  return m_isPrecodingInformationPresent;
}

bool
MIMOFeedbackControl::IsChannelAggregationPresent (void) const
{
  return m_isChannelMeasurementPresent;
}

//// NINA ////
uint8_t
MIMOFeedbackControl::GetNumberOfTxAntennas (void) const
{
  return m_numberOfTxAntennas;
}

uint8_t
MIMOFeedbackControl::GetNumberOfRxAntennas (void) const
{
  return m_numberOfRxAntennas;
}
//// NINA ////

uint8_t
MIMOFeedbackControl::GetNumberOfColumns (void) const
{
  return m_numberOfColumns;
}

uint8_t
MIMOFeedbackControl::GetNumberOfRows (void) const
{
  return m_numberOfRows;
}

uint8_t
MIMOFeedbackControl::GetTxAntennaMask (void) const
{
  return m_txAntennaMask;
}

uint8_t
MIMOFeedbackControl::GetNumberOfContiguousChannels (void) const
{
  return m_numberOfContiguousChannels;
}

uint8_t
MIMOFeedbackControl::GetGrouping (void) const
{
  return m_grouping;
}

bool
MIMOFeedbackControl::GetCodebookInformationType (void) const
{
  return m_codebookInformationType;
}

bool
MIMOFeedbackControl::GetFeedbackType (void) const
{
  return m_feedbackType;
}

uint16_t
MIMOFeedbackControl::GetNumberOfFeedbackMatricesOrTaps (void) const
{
  return m_numberOfFeedbackMatricesOrTaps;
}

ATTRIBUTE_HELPER_CPP (MIMOFeedbackControl);

std::ostream &operator << (std::ostream &os, const MIMOFeedbackControl &element)
{
  return os;
}

std::istream &operator >> (std::istream &is, MIMOFeedbackControl &element)
{
  return is;
}

/***************************************************
*      MIMO Selection Control Element 9.4.2.261
****************************************************/

MIMOSelectionControlElement::MIMOSelectionControlElement ()
  : m_edmgGroupID (0),
    m_numMUConfigurations (0),
    m_muType (MU_NonReciprocal)
{
}

WifiInformationElementId
MIMOSelectionControlElement::ElementId () const
{
  return IE_EXTENSION;
}

WifiInformationElementId
MIMOSelectionControlElement::ElementIdExt () const
{
  return IE_EXTENSION_MIMO_SELECTION_CONTROL;
}

uint8_t
MIMOSelectionControlElement::GetInformationFieldSize () const
{
  uint8_t length;
  length = 3;
  if (m_muType == MU_NonReciprocal)
    {
      for (auto & config : m_nonReciprocalConfigList)
        {
          length+= 4;
          length+= config.configList.size () * 2;
        }
    }
  else
    {
      for (auto & config : m_reciprocalConfigList)
        {
          length+= 4;
          length+= config.configList.size () * 4;
        }
    }
  return length;
}

void
MIMOSelectionControlElement::SerializeInformationField (Buffer::Iterator start) const
{
  uint8_t val = 0;
  start.WriteU8 (m_edmgGroupID);
  val |= (m_numMUConfigurations & 0x7);
  val |= (m_muType & 0x1) << 3;
  start.WriteU8 (val);
  if (m_muType == MU_NonReciprocal)
    {
      for (auto & config : m_nonReciprocalConfigList)
        {
          start.WriteHtolsbU32 (config.nonReciprocalConfigGroupUserMask);
          for (auto & user : config.configList)
            {
              start.WriteHtolsbU16 (user);
            }
        }
    }
  else
    {
      for (auto & config : m_reciprocalConfigList)
        {
          start.WriteHtolsbU32 (config.reciprocalConfigGroupUserMask);
          for (auto & user : config.configList)
            {
              uint32_t val1 = 0;
              val1 |= (user.ConfigurationAWVFeedbackID & 0x7FF);
              val1 |= (user.ConfigurationBRPCDOWN & 0x3F) << 11;
              val1 |= (user.ConfigurationRXAntennaID & 0x7) << 17;
              start.WriteHtolsbU32 (val1);
            }
        }
    }
}

uint8_t
MIMOSelectionControlElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  m_edmgGroupID = start.ReadU8 ();
  uint8_t val = start.ReadU8 ();
  m_numMUConfigurations = (val & 0x7);
  m_muType = static_cast<MultiUserTransmissionConfigType> ((val >> 3) & 0x1);
  uint8_t endLength = length - 2;
  while (endLength != 0)
    {
      uint32_t groupUserMask = start.ReadLsbtohU32 ();
      uint8_t numUsers = 0;
      for (uint8_t i = 0; i < 32; i++)
        {
          numUsers += (groupUserMask >> i) & 0x1;
        }
      if (m_muType == MU_NonReciprocal)
        {
          NonReciprocalTransmissionConfig config;
          config.nonReciprocalConfigGroupUserMask = groupUserMask;
          for (uint8_t i = 0; i < numUsers; i++)
            {
              config.configList.push_back (start.ReadLsbtohU16 ());
            }
          m_nonReciprocalConfigList.push_back (config);
          endLength-= (4 + numUsers * 2);
        }
      else
        {
          ReciprocalTransmissionConfig config;
          config.reciprocalConfigGroupUserMask = groupUserMask;
          for (uint8_t i = 0; i < numUsers; i++)
            {
              uint32_t val1 = start.ReadLsbtohU32 ();
              ReciprocalConfigData user;
              user.ConfigurationAWVFeedbackID = (val1 & 0x7FF);
              user.ConfigurationBRPCDOWN = (val1 >> 11) & 0x3F;
              user.ConfigurationRXAntennaID = (val1 >> 17) & 0x7;
              config.configList.push_back (user);
            }
          m_reciprocalConfigList.push_back (config);
          endLength-= (4 + numUsers * 4);
        }
    }
  return length;
}

void
MIMOSelectionControlElement::SetEDMGGroupID (uint8_t id)
{
  m_edmgGroupID = id;
}

void
MIMOSelectionControlElement::SetNumberOfMultiUserConfigurations (uint8_t number)
{
  m_numMUConfigurations = number;
}

void
MIMOSelectionControlElement::SetMultiUserTransmissionConfigurationType (MultiUserTransmissionConfigType type)
{
  m_muType = type;
}

void
MIMOSelectionControlElement::AddNonReciprocalMUBFTrainingBasedTransmissionConfig (NonReciprocalTransmissionConfig &config)
{
  m_nonReciprocalConfigList.push_back (config);
}

void
MIMOSelectionControlElement::AddReciprocalMUBFTrainingBasedTransmissionConfig (ReciprocalTransmissionConfig &config)
{
  m_reciprocalConfigList.push_back (config);
}

uint8_t
MIMOSelectionControlElement::GetEDMGGroupID (void) const
{
  return m_edmgGroupID;
}

uint8_t
MIMOSelectionControlElement::GetNumberOfMultiUserConfigurations (void) const
{
  return m_numMUConfigurations;
}

MultiUserTransmissionConfigType
MIMOSelectionControlElement::GetMultiUserTransmissionConfigurationType (void) const
{
  return m_muType;
}

NonReciprocalTransmissionConfigList
MIMOSelectionControlElement::GetNonReciprocalTransmissionConfigList (void) const
{
  return m_nonReciprocalConfigList;
}

ReciprocalTransmissionConfigList
MIMOSelectionControlElement::GetReciprocalTransmissionConfigList (void) const
{
  return m_reciprocalConfigList;
}

ATTRIBUTE_HELPER_CPP (MIMOSelectionControlElement);

std::ostream &operator << (std::ostream &os, const MIMOSelectionControlElement &element)
{
  return os;
}

std::istream &operator >> (std::istream &is, MIMOSelectionControlElement &element)
{
  return is;
}

/***************************************************
*      Digital BF Feedback Element 9.4.2.269
****************************************************/

DigitalBFFeedbackElement::DigitalBFFeedbackElement ()
{
}

WifiInformationElementId
DigitalBFFeedbackElement::ElementId () const
{
  return IE_EXTENSION;
}

WifiInformationElementId
DigitalBFFeedbackElement::ElementIdExt () const
{
  return IE_EXTENSION_DIGITAL_BF_FEEDBACK;
}

uint8_t
DigitalBFFeedbackElement::GetInformationFieldSize () const
{
  return 6;
}

void
DigitalBFFeedbackElement::SerializeInformationField (Buffer::Iterator start) const
{
  /* Iterate over each Digital Beamforming Feedback Matrix*/
  for (DigitalBeamformingFeedbackInformationCI mat = m_digitalFBInfo.begin (); mat != m_digitalFBInfo.end (); mat++)
    {
      /* Iterate over each Digital Feedback Component */
      for (DigitalBeamformingFeedbackMatrixCI it = (*mat).begin (); it != (*mat).end (); it++)
        {
          start.WriteU8 (it->realPart);
          start.WriteU8 (it->imagPart);
        }
    }
}

uint8_t
DigitalBFFeedbackElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  return length;
}

void
DigitalBFFeedbackElement::AddDigitalBeamformingFeedbackMatrix (DigitalBeamformingFeedbackMatrix &matrix)
{
  m_digitalFBInfo.push_back (matrix);
}

void
DigitalBFFeedbackElement::AddDifferentialSubcarrierIndex (DifferentialSubcarrierIndex index)
{
  m_differentialSubcarrierIndexList.push_back (index);
}

void
DigitalBFFeedbackElement::AddRelativeTapDelay (Tap_Delay tapDelay)
{
  m_tapDelay.push_back (tapDelay);
}

void
DigitalBFFeedbackElement::AddDifferentialSNRforSpaceTimeStream (SNR_INT differentialSNR)
{
  m_muExclusiveBeamformingReport.push_back (differentialSNR);
}

DigitalBeamformingFeedbackInformation
DigitalBFFeedbackElement::GetDigitalBeamformingFeedbackInformation (void) const
{
  return m_digitalFBInfo;
}

DifferentialSubcarrierIndexList
DigitalBFFeedbackElement::GetDifferentialSubcarrierIndex (void) const
{
  return m_differentialSubcarrierIndexList;
}

Tap_Delay_List
DigitalBFFeedbackElement::GetTapDelay (void) const
{
  return m_tapDelay;
}

SNR_INT_LIST DigitalBFFeedbackElement::GetMUExclusiveBeamformingReport(void) const
{
  return m_muExclusiveBeamformingReport;
}

ATTRIBUTE_HELPER_CPP (DigitalBFFeedbackElement);

std::ostream &operator << (std::ostream &os, const DigitalBFFeedbackElement &element)
{
  return os;
}

std::istream &operator >> (std::istream &is, DigitalBFFeedbackElement &element)
{
  return is;
}

/***************************************************
*      TDD Slot Structure element (9.4.2.266)
****************************************************/

TDDSlotStructureElement::TDDSlotStructureElement ()
  : m_allocationID (0),
    m_maximumSynchonizationError (0),
    m_peerStaAddress (),
    m_slotStructureStartTime (0),
    m_numberOfTDDIntervals (0),
    m_tddIntervalDuration (0)
{
}

WifiInformationElementId
TDDSlotStructureElement::ElementId () const
{
  return IE_EXTENSION;
}

WifiInformationElementId
TDDSlotStructureElement::ElementIdExt () const
{
  return IE_EXTENSION_TDD_SLOT_STRUCTURE;
}

uint8_t
TDDSlotStructureElement::GetInformationFieldSize () const
{
  uint8_t size = 0;
  size += 18;
  size += 4 * m_slotStructureFieldList.size ();
  return size;
}

void
TDDSlotStructureElement::SerializeInformationField (Buffer::Iterator start) const
{
  uint16_t structureControl = 0;
  structureControl |= m_allocationID & 0xF;
  structureControl |= (m_maximumSynchonizationError & 0xFF) << 4;
  start.WriteHtolsbU16 (structureControl);
  WriteTo (start, m_peerStaAddress);
  start.WriteHtolsbU32 (m_slotStructureStartTime);
  start.WriteHtolsbU16 (m_numberOfTDDIntervals);
  start.WriteHtolsbU16 (m_tddIntervalDuration);
  start.WriteU8 (m_slotStructureFieldList.size () & 0xFF);
  for (SlotStructureFieldListCI it = m_slotStructureFieldList.begin (); it != m_slotStructureFieldList.end (); it++)
    {
      start.WriteHtolsbU16 (it->SlotStart);
      start.WriteHtolsbU16 (it->SlotDuration);
    }
}

uint8_t
TDDSlotStructureElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  uint16_t structureControl = start.ReadLsbtohU16 ();
  m_allocationID = structureControl & 0xF;
  m_maximumSynchonizationError = (structureControl >> 4) & 0xFF;
  ReadFrom (start, m_peerStaAddress);
  m_slotStructureStartTime = start.ReadLsbtohU32 ();
  m_numberOfTDDIntervals = start.ReadLsbtohU16 ();
  m_tddIntervalDuration = start.ReadLsbtohU16 ();
  uint8_t numTDDSlots = start.ReadU8 ();
  for (uint8_t i = 0; i < numTDDSlots; i++)
    {
      SlotStructureField field;
      field.SlotStart = start.ReadLsbtohU16 ();
      field.SlotDuration = start.ReadLsbtohU16 ();
      m_slotStructureFieldList.push_back (field);
    }
  return length;
}

void
TDDSlotStructureElement::SetAllocationID (uint8_t id)
{
  m_allocationID = id;
}

void
TDDSlotStructureElement::SetMaximumTimeSynchonizationError (uint8_t error)
{
  m_maximumSynchonizationError = error;
}

void
TDDSlotStructureElement::SetPeerStaAddress (Mac48Address address)
{
  m_peerStaAddress = address;
}

void
TDDSlotStructureElement::SetSlotStructureStartTime (uint32_t startTime)
{
  m_slotStructureStartTime = startTime;
}

void
TDDSlotStructureElement::SetNumberOfTDDIntervals (uint16_t number)
{
  m_numberOfTDDIntervals = number;
}

void
TDDSlotStructureElement::SetTDDIntervalDuration (uint16_t duration)
{
  m_tddIntervalDuration = duration;
}

void
TDDSlotStructureElement::AddTDDSlotStructure (SlotStructureField &field)
{
  m_slotStructureFieldList.push_back (field);
}

uint8_t
TDDSlotStructureElement::GetAllocationID (void) const
{
  return m_allocationID;
}

uint8_t
TDDSlotStructureElement::GetMaximumTimeSynchonizationError (void) const
{
  return m_maximumSynchonizationError;
}

Mac48Address
TDDSlotStructureElement::GetPeerStaAddress (void) const
{
  return m_peerStaAddress;
}

uint32_t
TDDSlotStructureElement::GetSlotStructureStartTime (void) const
{
  return m_slotStructureStartTime;
}

uint16_t
TDDSlotStructureElement::GetNumberOfTDDIntervals (void) const
{
  return m_numberOfTDDIntervals;
}

uint16_t
TDDSlotStructureElement::GetTDDIntervalDuration (void) const
{
  return m_tddIntervalDuration;
}

SlotStructureFieldList
TDDSlotStructureElement::GetSlotStructure (void) const
{
  return m_slotStructureFieldList;
}

ATTRIBUTE_HELPER_CPP (TDDSlotStructureElement);

std::ostream &operator << (std::ostream &os, const TDDSlotStructureElement &element)
{
   return os;
}

std::istream &operator >> (std::istream &is, TDDSlotStructureElement &element)
{
  return is;
}

/***************************************************
*      TDD Slot Schedule element (9.4.2.267)
****************************************************/

TDDSlotScheduleElement::TDDSlotScheduleElement ()
  : m_allocationID (0),
    m_channelAggregation (false),
    m_bandwidth (0),
    m_maximumSynchonizationError (0),
    m_peerStaAddress (),
    m_slotStructureStartTime (0),
    m_numberOfTDDIntervals (0),
    m_tddSlotScheduleDuration (0)
{
}

WifiInformationElementId
TDDSlotScheduleElement::ElementId () const
{
  return IE_EXTENSION;
}

WifiInformationElementId
TDDSlotScheduleElement::ElementIdExt () const
{
  return IE_EXTENSION_TDD_SLOT_SCHEDULE;
}

uint8_t
TDDSlotScheduleElement::GetInformationFieldSize () const
{
  return 0;
}

void
TDDSlotScheduleElement::SerializeInformationField (Buffer::Iterator start) const
{

}

uint8_t
TDDSlotScheduleElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  return length;
}

void
TDDSlotScheduleElement::SetAllocationID (uint8_t id)
{
  m_allocationID = id;
}

void
TDDSlotScheduleElement::SetChannelAggregation (bool aggregation)
{
  m_channelAggregation = aggregation;
}

void
TDDSlotScheduleElement::SetBandwidth (uint8_t bandwidth)
{
  m_bandwidth = bandwidth;
}

void
TDDSlotScheduleElement::SetPeerStaAddress (Mac48Address address)
{
  m_peerStaAddress = address;
}

void
TDDSlotScheduleElement::SetSlotStructureStartTime (uint32_t startTime)
{
  m_slotStructureStartTime = startTime;
}

void
TDDSlotScheduleElement::SetNumberOfTDDIntervals (uint16_t number)
{
  m_numberOfTDDIntervals = number;
}

void
TDDSlotScheduleElement::SetTDDSlotScheduleDuration (uint16_t duration)
{
  m_tddSlotScheduleDuration = duration;
}

void
TDDSlotScheduleElement::SetBitmapAndAccessTypeSchedule (SlotStructureField &field)
{

}

void
TDDSlotScheduleElement::SetSlotCategorySchedule (SlotStructureField &field)
{

}

uint8_t
TDDSlotScheduleElement::GetAllocationID (void) const
{
  return m_allocationID;
}

bool
TDDSlotScheduleElement::GetChannelAggregation (void) const
{
  return m_channelAggregation;
}

uint8_t
TDDSlotScheduleElement::GetBandwidth (void) const
{
  return m_bandwidth;
}

Mac48Address
TDDSlotScheduleElement::GetPeerStaAddress (void) const
{
  return m_peerStaAddress;
}

uint32_t
TDDSlotScheduleElement::GetSlotStructureStartTime (void) const
{
  return m_slotStructureStartTime;
}

uint16_t
TDDSlotScheduleElement::GetNumberOfTDDIntervals (void) const
{
  return m_numberOfTDDIntervals;
}

uint16_t
TDDSlotScheduleElement::GetTDDSlotScheduleDuration (void) const
{
  return m_tddSlotScheduleDuration;
}

void
TDDSlotScheduleElement::GetBitmapAndAccessTypeSchedule (void) const
{

}

void
TDDSlotScheduleElement::GetSlotCategorySchedule (void) const
{

}

ATTRIBUTE_HELPER_CPP (TDDSlotScheduleElement);

std::ostream &operator << (std::ostream &os, const TDDSlotScheduleElement &element)
{
  return os;
}

std::istream &operator >> (std::istream &is, TDDSlotScheduleElement &element)
{
  return is;
}


/***************************************************
*             TDD Sector Feedback subelement
****************************************************/

TDDSectorFeedbackSubelement::TDDSectorFeedbackSubelement ()
{
}

WifiInformationElementId
TDDSectorFeedbackSubelement::ElementId () const
{
  return TDD_SECTOR_FEEDBACK;
}

uint8_t
TDDSectorFeedbackSubelement::GetInformationFieldSize () const
{
  uint8_t size = 0;
  size += 2;
  for (TxBeamFeedbackCI it = m_txBeamFeedbackList.begin (); it != m_txBeamFeedbackList.end (); it++)
    {
      for (DecodedRxSectorsInformationListCI info = it->DecodedRxSectorsInfoList.begin ();
           info != it->DecodedRxSectorsInfoList.end (); info++)
        {
          size += 3 + it->DecodedRxSectorsInfoList.size () * 4;
        }
    }
  return size;
}

void
TDDSectorFeedbackSubelement::SerializeInformationField (Buffer::Iterator start) const
{
  uint16_t numTxBeams = m_txBeamFeedbackList.size ();
  uint8_t numDecodedTxSectors;
  uint8_t value[3];
  start.WriteHtolsbU16 (numTxBeams);
  for (TxBeamFeedbackCI it = m_txBeamFeedbackList.begin (); it != m_txBeamFeedbackList.end (); it++)
    {
      numDecodedTxSectors = it->DecodedRxSectorsInfoList.size ();
      value[0] = (it->TxSectorID & 0xFF);
      value[1] = (it->TxSectorID >> 8) & 0x1;
      value[1] = (it->TxAntennaID & 0x7) << 1;
      value[1] = (numDecodedTxSectors & 0xF) << 4;
      value[2] = (numDecodedTxSectors >> 4) & 0xF;
      start.Write (value, 3);
      for (DecodedRxSectorsInformationListCI info = it->DecodedRxSectorsInfoList.begin ();
           info != it->DecodedRxSectorsInfoList.end (); info++)
        {
          uint32_t subfield = 0;
          subfield |= (info->DecodedRxSectorID & 0x1FF);
          subfield |= (info->DecodedRxAntennaID & 0x7) << 9;
          subfield |= (info->SnrReport & 0xFF) << 16;
          subfield |= (info->RSSI_Report & 0xFF) << 24;
          start.WriteHtolsbU32 (subfield);
        }
    }
}

uint8_t
TDDSectorFeedbackSubelement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  uint16_t numTxBeams = start.ReadLsbtohU16 ();
  uint8_t value[3];
  for (uint16_t i = 0; i < numTxBeams; i++)
    {
      TxBeamFeedback beamFB;
      uint8_t numDecodedTxSectors = 0;
      start.Read (value, 3);
      beamFB.TxSectorID |= value[0] & 0xFF;
      beamFB.TxSectorID |= (value[1] & 0x1) << 8;
      beamFB.TxAntennaID = (value[1] >> 1) & 0x7;
      numDecodedTxSectors |= (value[1] >> 4) & 0xF;
      numDecodedTxSectors |= (value[2] & 0xF) << 4;
      for (uint16_t j = 0; j < numDecodedTxSectors; j++)
        {
          DecodedRxSectorsInformation decodedInfo;
          uint32_t subfield = start.ReadU8 ();
          decodedInfo.DecodedRxSectorID = (subfield & 0x1FF);
          decodedInfo.DecodedRxAntennaID = (subfield >> 9) & 0x7;
          decodedInfo.SnrReport = (subfield >> 16) & 0xFF;
          decodedInfo.RSSI_Report =  (subfield >> 24) & 0xFF;
          beamFB.DecodedRxSectorsInfoList.push_back (decodedInfo);
        }
      m_txBeamFeedbackList.push_back (beamFB);
    }
  return length;
}

void
TDDSectorFeedbackSubelement::AddTxBeamFeedback (const TxBeamFeedback &feedback)
{
  m_txBeamFeedbackList.push_back (feedback);
}

TxBeamFeedbackList
TDDSectorFeedbackSubelement::GetTxBeamFeedbackList (void) const
{
  return m_txBeamFeedbackList;
}

/***************************************************
*             TDD Sector Setting Subelement
****************************************************/

TDDSectorSettingSubelement::TDDSectorSettingSubelement ()
  : m_sectorRequest (false),
    m_sectorResponse (false),
    m_SectorAcknowledge (false),
    m_switchTimestamp (0),
    m_revertTimestamp (0),
    m_responderRxSectorID (0),
    m_responderRxAntennaID (0),
    m_responderTxSectorID (0),
    m_responderTxAntennaID (0),
    m_initiatorRxSectorID (0),
    m_initiatorRxAntennaID (0),
    m_initiatorTxSectorID (0),
    m_initiatorTxAntennaID (0)
{
}

WifiInformationElementId
TDDSectorSettingSubelement::ElementId () const
{
  return TDD_SECTOR_SETTING;
}

uint8_t
TDDSectorSettingSubelement::GetInformationFieldSize () const
{
  return 23;
}

void
TDDSectorSettingSubelement::SerializeInformationField (Buffer::Iterator start) const
{
  uint8_t sectorSetting = 0;
  uint32_t switchSectors1 = 0;
  uint16_t switchSectors2 = 0;
  sectorSetting |= (m_sectorRequest & 0x1);
  sectorSetting |= (m_sectorResponse & 0x1) << 1;
  sectorSetting |= (m_SectorAcknowledge & 0x1) << 2;
  start.WriteU8 (sectorSetting);
  start.WriteHtolsbU64 (m_switchTimestamp);
  start.WriteHtolsbU64 (m_revertTimestamp);
  switchSectors1 |= (m_responderRxSectorID & 0x1FF);
  switchSectors1 |= (m_responderRxAntennaID & 0x7) << 9;
  switchSectors1 |= (m_responderTxSectorID & 0x1FF) << 12;
  switchSectors1 |= (m_responderTxAntennaID & 0x7) << 21;
  switchSectors1 |= (m_initiatorRxSectorID & 0xFF) << 24;
  switchSectors2 |= ((m_initiatorRxSectorID >> 8) & 0x1);
  switchSectors2 |= (m_initiatorRxAntennaID & 0x7) << 1;
  switchSectors2 |= (m_initiatorTxSectorID & 0x1FF) << 4;
  switchSectors2 |= (m_initiatorTxAntennaID & 0x7) << 13;
  start.WriteHtolsbU32 (switchSectors1);
  start.WriteHtolsbU16 (switchSectors2);
}

uint8_t
TDDSectorSettingSubelement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  uint8_t sectorSetting = 0;
  uint32_t switchSectors1;
  uint16_t switchSectors2;
  sectorSetting = start.ReadU8 ();
  m_switchTimestamp = start.ReadLsbtohU64 ();
  m_revertTimestamp = start.ReadLsbtohU64 ();
  switchSectors1 = start.ReadLsbtohU32 ();
  switchSectors2 = start.ReadLsbtohU16 ();

  m_sectorRequest = (sectorSetting & 0x1);
  m_sectorResponse = (sectorSetting >> 1) & 0x1;
  m_SectorAcknowledge = (sectorSetting >> 2) & 2;

  m_responderRxSectorID = (switchSectors1 & 0x1FF);
  m_responderRxAntennaID = (switchSectors1 >> 9) & 0x7;
  m_responderTxSectorID = (switchSectors1 >> 12) & 0x1FF;
  m_responderTxAntennaID = (switchSectors1 >> 21) & 0x7;
  m_initiatorRxSectorID |= (switchSectors1 >> 24) & 0xFF;
  m_initiatorRxSectorID |= (switchSectors2 & 0x1) << 8;
  m_initiatorRxAntennaID = (switchSectors2 >> 1) & 0x7;
  m_initiatorTxSectorID = (switchSectors2 >> 4) & 0x1FF;
  m_initiatorTxAntennaID = (switchSectors2 >> 13) & 0x7;

  return length;
}

void
TDDSectorSettingSubelement::SetSectorRequest (bool value)
{
  m_sectorRequest = value;
}

void
TDDSectorSettingSubelement::SetSectorResponse (bool value)
{
  m_sectorResponse = value;
}

void
TDDSectorSettingSubelement::SetSectorAcknowledge (bool value)
{
  m_SectorAcknowledge = value;
}

void
TDDSectorSettingSubelement::SetSwitchTimestamp (uint64_t value)
{
  m_switchTimestamp = value;
}

void
TDDSectorSettingSubelement::SetRevertTimestamp (uint64_t value)
{
  m_revertTimestamp = value;
}

/* TDD Switch Sectors Field */
void
TDDSectorSettingSubelement::SetResponderRxSectorID (uint16_t sectorID)
{
  m_responderRxSectorID = sectorID;
}

void
TDDSectorSettingSubelement::SetResponderRxAntennaID (uint8_t antennaID)
{
  m_responderRxAntennaID = antennaID;
}

void
TDDSectorSettingSubelement::SetResponderTxSectorID (uint16_t sectorID)
{
  m_responderTxSectorID = sectorID;
}

void
TDDSectorSettingSubelement::SetResponderTxAntennaID (uint8_t antennaID)
{
  m_responderTxAntennaID = antennaID;
}

void
TDDSectorSettingSubelement::SetInitiatorRxSectorID (uint16_t sectorID)
{
  m_initiatorRxSectorID = sectorID;
}

void
TDDSectorSettingSubelement::SetInitiatorRxAntennaID (uint8_t antennaID)
{
  m_initiatorRxAntennaID = antennaID;
}

void
TDDSectorSettingSubelement::SetInitiatorTxSectorID (uint16_t sectorID)
{
  m_initiatorTxSectorID = sectorID;
}

void
TDDSectorSettingSubelement::SetInitiatorTxAntennaID (uint8_t antennaID)
{
  m_initiatorTxAntennaID = antennaID;
}

bool
TDDSectorSettingSubelement::GetSectorRequest (void) const
{
  return m_sectorRequest;
}

bool
TDDSectorSettingSubelement::GetSectorResponse (void) const
{
  return m_sectorResponse;
}

bool
TDDSectorSettingSubelement::GetSectorAcknowledge (void) const
{
  return m_SectorAcknowledge;
}

uint64_t
TDDSectorSettingSubelement::GetSwitchTimestamp (void) const
{
  return m_switchTimestamp;
}

uint64_t
TDDSectorSettingSubelement::GetRevertTimestamp (void) const
{
  return m_revertTimestamp;
}

uint16_t
TDDSectorSettingSubelement::GetResponderRxSectorID (void) const
{
  return m_responderRxSectorID;
}

uint8_t
TDDSectorSettingSubelement::GetResponderRxAntennaID (void) const
{
  return m_responderRxAntennaID;
}

uint16_t
TDDSectorSettingSubelement::GetResponderTxSectorID (void) const
{
  return m_responderTxSectorID;
}

uint8_t
TDDSectorSettingSubelement::GetResponderTxAntennaID (void) const
{
  return m_responderTxAntennaID;
}

uint16_t
TDDSectorSettingSubelement::GetInitiatorRxSectorID (void) const
{
  return m_initiatorRxSectorID;
}

uint8_t
TDDSectorSettingSubelement::GetInitiatorRxAntennaID (void) const
{
  return m_initiatorRxAntennaID;
}

uint16_t
TDDSectorSettingSubelement::GetInitiatorTxSectorID (void) const
{
  return m_initiatorTxSectorID;
}

uint8_t
TDDSectorSettingSubelement::GetInitiatorTxAntennaID (void) const
{
  return m_initiatorTxAntennaID;
}

/***************************************************
*             TDD Sector Config Subelement
****************************************************/

TDDSectorConfigSubelement::TDDSectorConfigSubelement ()
{
}

WifiInformationElementId
TDDSectorConfigSubelement::ElementId () const
{
  return TDD_SECTOR_CONFIG;
}

uint8_t
TDDSectorConfigSubelement::GetInformationFieldSize () const
{
  return 2 + m_list.size () * 2;
}

void
TDDSectorConfigSubelement::SerializeInformationField (Buffer::Iterator start) const
{
  uint16_t numConfigSectors = m_list.size ();
  start.WriteHtolsbU16 (numConfigSectors);
  for (ConfiguredSectorListCI it = m_list.begin (); it != m_list.end (); it++)
    {
      uint16_t value = 0;
      value |= it->ConfiguredRxSectorID & 0x1FF;
      value |= (it->ConfiguredRxAntennaID & 0x7) << 9;
      start.WriteHtolsbU16 (value);
    }
}

uint8_t
TDDSectorConfigSubelement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  uint16_t numConfigSectors = start.ReadLsbtohU16 ();
  uint16_t value;
  for (uint16_t i = 0; i < numConfigSectors; i++)
    {
      ConfiguredSector config;
      value = start.ReadLsbtohU16 ();
      config.ConfiguredRxSectorID = value & 0x1FF;
      config.ConfiguredRxAntennaID = (value >> 9) & 0x7;
      m_list.push_back (config);
    }
  return length;
}

void
TDDSectorConfigSubelement::AddConfiguredSector (const ConfiguredSector &sector)
{
  m_list.push_back (sector);
}

ConfiguredSectorList
TDDSectorConfigSubelement::GetConfiguredSectorList (void) const
{
  return m_list;
}

/***************************************************
*           TDD Route Element (9.4.2.281)
****************************************************/

TDDRouteElement::TDDRouteElement ()
{
}

WifiInformationElementId
TDDRouteElement::ElementId () const
{
  return IE_EXTENSION;
}

WifiInformationElementId
TDDRouteElement::ElementIdExt () const
{
  return IE_EXTENSION_TDD_ROUTE;
}

uint8_t
TDDRouteElement::GetInformationFieldSize () const
{
  Ptr<WifiInformationElement> element;
  uint32_t size = 1; // Element ID Extension
  for (WifiInformationSubelementMap::const_iterator elem = m_map.begin (); elem != m_map.end (); elem++)
    {
      element = elem->second;
      size += element->GetSerializedSize ();
    }
  return size;
}

void
TDDRouteElement::SerializeInformationField (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  Ptr<WifiInformationElement> element;
  for (WifiInformationSubelementMap::const_iterator elem = m_map.begin (); elem != m_map.end (); elem++)
    {
      element = elem->second;
      i = element->Serialize (i);
    }
}

uint8_t
TDDRouteElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;
  Ptr<WifiInformationElement> element;
  uint8_t id, subelemLen;
  while (!i.IsEnd ())
    {
      i = DeserializeElementID (i, id, subelemLen);
      switch (id)
        {
          case TDD_SECTOR_FEEDBACK:
            {
              element = Create<TDDSectorFeedbackSubelement> ();
              break;
            }
          case TDD_SECTOR_SETTING:
            {
              element = Create<TDDSectorSettingSubelement> ();
              break;
            }
          case TDD_SECTOR_CONFIG:
            {
              element = Create<TDDSectorConfigSubelement> ();
              break;
            }
        default:
          break;
        }
      i = element->DeserializeElementBody (i, subelemLen);
      m_map[id] = element;
    }
  return length;
}

void
TDDRouteElement::AddSubElement (Ptr<WifiInformationElement> elem)
{
  m_map[elem->ElementId ()] = elem;
}

Ptr<WifiInformationElement>
TDDRouteElement::GetSubElement (WifiInformationElementId id)
{
  WifiInformationSubelementMap::const_iterator it = m_map.find (id);
  if (it != m_map.end ())
    {
      return it->second;
    }
  else
    {
      return 0;
    }
}

WifiInformationSubelementMap
TDDRouteElement::GetListOfSubElements (void) const
{
  return m_map;
}

/***************************************************
*      TDD Bandwidth Request Element (9.4.2.270)
****************************************************/

TDDBandwidthRequestElement::TDDBandwidthRequestElement ()
  : m_transmitMCS (0),
    m_txPercentage (0)
{
}

WifiInformationElementId
TDDBandwidthRequestElement::ElementId () const
{
  return IE_EXTENSION;
}

WifiInformationElementId
TDDBandwidthRequestElement::ElementIdExt () const
{
  return IE_EXTENSION_TDD_BANDWIDTH_REQUEST;
}

uint8_t
TDDBandwidthRequestElement::GetInformationFieldSize () const
{
  return 16;
}

void
TDDBandwidthRequestElement::SerializeInformationField (Buffer::Iterator start) const
{
  uint8_t value[3];
  uint8_t numQueueParameters = m_list.size ();
  start.WriteU8 (m_transmitMCS);
  value[0] = (m_txPercentage & 0xFF);
  value[1] = (m_txPercentage >> 8) & 0x3F;
  value[1] = (numQueueParameters & 0x3) << 6;
  value[2] = (numQueueParameters >> 3) & 0x7;
  start.Write (value, 3);
  for (QueueParameterFieldListCI it = m_list.begin (); it != m_list.end (); it++)
    {
      start.WriteU8 (it->TID & 0x1F);
      start.WriteHtolsbU32 (it->QueueSize);
      start.WriteHtolsbU32 (it->TrafficArrivalRate);
    }
}

uint8_t
TDDBandwidthRequestElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  uint8_t value[3];
  uint8_t numQueueParameters = 0;
  m_transmitMCS = start.ReadU8 ();
  start.Read (value, 3);
  m_txPercentage |= (value[0] & 0xFF);
  m_txPercentage |= (value[1] & 0x3F) << 8;
  numQueueParameters |= (value[1] & 0x3);
  numQueueParameters |= (value[2] & 0x7) << 3;
  for (uint8_t i = 0; i < numQueueParameters; i++)
    {
      QueueParameterField field;
      field.TID = start.ReadU8 ();
      field.QueueSize = start.ReadLsbtohU32 ();
      field.TrafficArrivalRate = start.ReadLsbtohU32 ();
      m_list.push_back (field);
    }
  return length;
}

void
TDDBandwidthRequestElement::SetTransmitMCS (uint8_t mcs)
{
  m_transmitMCS = mcs;
}

void
TDDBandwidthRequestElement::SetTxPercentage (uint16_t percentage)
{
  m_txPercentage = percentage;
}

void
TDDBandwidthRequestElement::AddQueueParameter (QueueParameterField &field)
{
  m_list.push_back (field);
}

uint8_t
TDDBandwidthRequestElement::GetTransmitMCS (void) const
{
  return m_transmitMCS;
}

uint16_t
TDDBandwidthRequestElement::GetTxPercentage (void) const
{
  return m_txPercentage;
}

QueueParameterFieldList
TDDBandwidthRequestElement::GetQueueParameterFieldList (void) const
{
  return m_list;
}

/***************************************************
*      TDD Synchronization Element (9.4.2.271)
****************************************************/

TDDSynchronizationElement::TDDSynchronizationElement ()
  : m_priority1 (0),
    m_clockClass (0),
    m_clockAccuracy (0),
    m_offsetScaledLogVariance (0),
    m_priority2 (0),
    m_clockIdentity (0),
    m_timeSource (0),
    m_sycnMode (0)
{
}

WifiInformationElementId
TDDSynchronizationElement::ElementId () const
{
  return IE_EXTENSION;
}

WifiInformationElementId
TDDSynchronizationElement::ElementIdExt () const
{
  return IE_EXTENSION_TDD_SYNCHRONIZATION;
}

uint8_t
TDDSynchronizationElement::GetInformationFieldSize () const
{
  return 16;
}

void
TDDSynchronizationElement::SerializeInformationField (Buffer::Iterator start) const
{
  start.WriteU8 (m_priority1);
  start.WriteU8 (m_clockClass);
  start.WriteU8 (m_clockAccuracy);
  start.WriteHtolsbU16 (m_offsetScaledLogVariance);
  start.WriteU8 (m_priority2);
  start.WriteHtolsbU64 (m_clockIdentity);
  start.WriteU8 (m_timeSource);
  start.WriteU8 (m_sycnMode);
}

uint8_t
TDDSynchronizationElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  m_priority1 = start.ReadU8 ();
  m_clockClass = start.ReadU8 ();
  m_clockAccuracy = start.ReadU8 ();
  m_offsetScaledLogVariance = start.ReadLsbtohU16 ();
  m_priority2 = start.ReadU8 ();
  m_clockIdentity = start.ReadLsbtohU64 ();
  m_timeSource = start.ReadU8 ();
  m_sycnMode = start.ReadU8 ();
  return length;
}

void
TDDSynchronizationElement::SetPriority1 (uint8_t priority)
{
  m_priority1 = priority;
}

void
TDDSynchronizationElement::SetClockClass (uint8_t value)
{
  m_clockClass = value;
}

void
TDDSynchronizationElement::SetClockAccuracy (uint8_t accuracy)
{
  m_clockAccuracy = accuracy;
}

void
TDDSynchronizationElement::SetOffsetScaledLogVariance (uint16_t priority)
{
  m_offsetScaledLogVariance = priority;
}

void
TDDSynchronizationElement::SetPriority2 (uint8_t priority)
{
  m_priority2 = priority;
}

void
TDDSynchronizationElement::SetClockIdentity (uint64_t identity)
{
  m_clockIdentity = identity;
}

void
TDDSynchronizationElement::SetTimeSource (uint8_t source)
{
  m_timeSource = source;
}

void
TDDSynchronizationElement::SetSyncMode (uint8_t mode)
{
  m_sycnMode = mode;
}

uint8_t
TDDSynchronizationElement::GetPriority1 (void) const
{
  return m_priority1;
}

uint8_t
TDDSynchronizationElement::GetClockClass (void) const
{
  return m_clockClass;
}

uint8_t
TDDSynchronizationElement::GetClockAccuracy (void) const
{
  return m_clockAccuracy;
}

uint16_t
TDDSynchronizationElement::GetOffGetScaledLogVariance (void) const
{
  return m_offsetScaledLogVariance;
}

uint8_t
TDDSynchronizationElement::GetPriority2 (void) const
{
  return m_priority2;
}

uint64_t
TDDSynchronizationElement::GetClockIdentity (void) const
{
  return m_clockIdentity;
}

uint8_t
TDDSynchronizationElement::GetTimeSource (void) const
{
  return m_timeSource;
}

uint8_t
TDDSynchronizationElement::GetSyncMode (void) const
{
  return m_sycnMode;
}

/***************************************************
*      EDMG Training Field Schedule Element 9.4.2.256
****************************************************/

EdmgTrainingFieldScheduleElement::EdmgTrainingFieldScheduleElement ()
  : m_nextBtiWithTrn(0),
    m_trnScheduleInterval(0)
{
}

WifiInformationElementId
EdmgTrainingFieldScheduleElement::ElementId () const
{
  return IE_EXTENSION;
}

WifiInformationElementId
EdmgTrainingFieldScheduleElement::ElementIdExt () const
{
  return IE_EXTENSION_EDMG_TRAINING_FIELD_SCHEDULE;
}

uint8_t
EdmgTrainingFieldScheduleElement::GetInformationFieldSize () const
{
  return 3;
}

void
EdmgTrainingFieldScheduleElement::SerializeInformationField (Buffer::Iterator start) const
{
  start.WriteU8(m_nextBtiWithTrn);
  start.WriteU8 (m_trnScheduleInterval);
}

uint8_t
EdmgTrainingFieldScheduleElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;
  m_nextBtiWithTrn = i.ReadU8 ();
  m_trnScheduleInterval = i.ReadU8 ();
  return length;
}

void
EdmgTrainingFieldScheduleElement::SetNextBtiWithTrn (uint8_t nextBti)
{
  m_nextBtiWithTrn = nextBti;
}

void
EdmgTrainingFieldScheduleElement::SetTrnScheduleInterval (uint8_t trnSchedule)
{
  m_trnScheduleInterval = trnSchedule;
}



uint8_t
EdmgTrainingFieldScheduleElement::GetNextBtiWithTrn (void) const
{
  return m_nextBtiWithTrn;
}

uint8_t
EdmgTrainingFieldScheduleElement::GetTrnScheduleInterval (void) const
{
  return m_trnScheduleInterval;
}

ATTRIBUTE_HELPER_CPP (EdmgTrainingFieldScheduleElement);

std::ostream &operator << (std::ostream &os, const EdmgTrainingFieldScheduleElement &element)
{
  return os;
}

std::istream &operator >> (std::istream &is, EdmgTrainingFieldScheduleElement &element)
{
  return is;
}

/***************************************************
*      EDMG BRP Request Element 9.4.2.255
****************************************************/

EdmgBrpRequestElement::EdmgBrpRequestElement ()
  : m_L_RX (0),
    m_L_TX_RX (0),
    m_TXSectorID (0),
    m_reqEDMG_TRN_UnitP (0),
    m_reqEDMG_TRN_UnitM (0),
    m_reqEDMG_TRN_UnitN (0),
    m_BRP_TXSS (false),
    m_TXSS_Initiator (false),
    m_TXSS_Packets (0),
    m_TXSS_Repeat (0),
    m_TXSS_MIMO (false),
    m_BRP_CDOWN (0),
    m_TX_Antenna_Mask (0),
    m_comebackDelay (0),
    m_firstPathTraining (false),
    m_dualPolarizationTrn (false),
    m_digitalBFRequest (false),
    m_feedbackType (SU_MIMO),
    m_ncIndex (0)
    {
    }

WifiInformationElementId
EdmgBrpRequestElement::ElementId () const
{
  return IE_EXTENSION;
}

WifiInformationElementId
EdmgBrpRequestElement::ElementIdExt () const
{
  return IE_EXTENSION_EDMG_BRP_REQUEST;
}

uint8_t
EdmgBrpRequestElement::GetInformationFieldSize () const
{
  return 10;
}

void
EdmgBrpRequestElement::SerializeInformationField (Buffer::Iterator start) const
{
  start.WriteU8(m_L_RX);
  uint32_t val1 = 0;
  uint32_t val2 = 0;

  val1 |= (m_L_TX_RX);
  val1 |= (m_TXSectorID & 0x7FF) << 8;
  val1 |= (m_reqEDMG_TRN_UnitP & 0x3) << 19;
  val1 |= (m_reqEDMG_TRN_UnitM & 0xF) << 21;
  val1 |= (m_reqEDMG_TRN_UnitN & 0x3) << 25;
  val1 |= (m_BRP_TXSS & 0x1) << 27;
  val1 |= (m_TXSS_Initiator & 0x1) << 28;

  val2 |= (m_TXSS_Packets & 0x7);
  val2 |= (m_TXSS_Repeat & 0x7) << 3;
  val2 |= (m_TXSS_MIMO & 0x1) << 6;
  val2 |= (m_BRP_CDOWN & 0x3F) << 7;
  val2 |= (m_TX_Antenna_Mask) << 13;
  val2 |= (m_comebackDelay & 0x7) << 21;
  val2 |= (m_firstPathTraining & 0x1) << 24;
  val2 |= (m_dualPolarizationTrn & 0x1) << 25;
  val2 |= (m_digitalBFRequest & 0x1) << 26;
  val2 |= (m_feedbackType & 0x1) << 27;
  val2 |= (m_ncIndex & 0x3) << 28;

  start.WriteHtolsbU32 (val1);
  start.WriteHtolsbU32 (val2);

}

uint8_t
EdmgBrpRequestElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;
  m_L_RX = i.ReadU8 ();
  uint32_t val1 = i.ReadLsbtohU32 ();
  uint32_t val2 = i.ReadLsbtohU32 ();
  m_L_TX_RX = val1 & 0xFF;
  m_TXSectorID = (val1 >> 8) & 0x7FF;
  m_reqEDMG_TRN_UnitP = (val1 >> 19) & 0x3;
  m_reqEDMG_TRN_UnitM = (val1 >> 21) & 0xF;
  m_reqEDMG_TRN_UnitN = (val1 >> 25) & 0x3;
  m_BRP_TXSS = (val1 >> 27) & 0x1;
  m_TXSS_Initiator = (val1 >> 28) & 0x1;

  m_TXSS_Packets = val2 & 0x7;
  m_TXSS_Repeat = (val2 >> 3) & 0x7;
  m_TXSS_MIMO = (val2 >> 6) & 0x1;
  m_BRP_CDOWN = (val2 >> 7) & 0x3F;
  m_TX_Antenna_Mask = (val2 >> 13) & 0xFF;
  m_comebackDelay = (val2 >> 21) & 0x7;
  m_firstPathTraining = (val2 >> 24) & 0x1;
  m_dualPolarizationTrn = (val2 >> 25) & 0x1;
  m_digitalBFRequest = (val2 >> 26) & 0x1;
  m_feedbackType = static_cast<FbckType> ((val2 >> 27) & 0x1);
  m_ncIndex = (val2 >> 28) & 0x3;
  return length;
}


void
EdmgBrpRequestElement::SetL_RX (uint8_t lRx)
{
  m_L_RX = lRx;
}

void
EdmgBrpRequestElement::SetL_TX_RX (uint8_t lTxRx)
{
  m_L_TX_RX = lTxRx;
}

void
EdmgBrpRequestElement::SetTXSectorID (uint16_t txSectorId)
{
  m_TXSectorID = txSectorId;
}

void
EdmgBrpRequestElement::SetRequestedEDMG_TRN_UnitP (uint8_t edmgTrnUnitP)
{
  if (edmgTrnUnitP <= 2)
    m_reqEDMG_TRN_UnitP = edmgTrnUnitP;
  else if (edmgTrnUnitP == 4)
    m_reqEDMG_TRN_UnitP = 3;
  else
    NS_FATAL_ERROR("Invalid Requested EDMG TRN Unit P value");
}

void
EdmgBrpRequestElement::SetRequestedEDMG_TRN_UnitM (uint8_t edmgTrnUnitM)
{
  m_reqEDMG_TRN_UnitM = edmgTrnUnitM - 1;
}

void
EdmgBrpRequestElement::SetRequestedEDMG_TRN_UnitN (uint8_t edmgTrnUnitN)
{
  if ((edmgTrnUnitN == 3) || (edmgTrnUnitN == 8))
    m_reqEDMG_TRN_UnitN = 2;
  else if ((edmgTrnUnitN == 1) || (edmgTrnUnitN == 2) || (edmgTrnUnitN == 4))
    m_reqEDMG_TRN_UnitN = edmgTrnUnitN -1;
  else
    NS_FATAL_ERROR("Invalid Requested EDMG TRN Unit N value");
}

void
EdmgBrpRequestElement::SetBRP_TXSS (bool brpTxss)
{
  m_BRP_TXSS = brpTxss;
}

void
EdmgBrpRequestElement::SetTXSS_Initiator (bool txssInitiator)
{
  m_TXSS_Initiator = txssInitiator;
}

void
EdmgBrpRequestElement::SetTXSS_Packets (uint8_t txssPackets)
{
  m_TXSS_Packets = txssPackets - 1;
}

void
EdmgBrpRequestElement::SetTXSS_Repeat (uint8_t txssRepeat)
{
  m_TXSS_Repeat = txssRepeat - 1;
}

void
EdmgBrpRequestElement::SetTXSS_MIMO (bool txssMimo)
{
  m_TXSS_MIMO = txssMimo;
}

void
EdmgBrpRequestElement::SetBRP_CDOWN (uint8_t brpCdown)
{
  m_BRP_CDOWN = brpCdown;
}

//// NINA ////
void
EdmgBrpRequestElement::SetTX_Antenna_Mask (std::vector<ANTENNA_ID> txAntennaIds)
{
  m_TX_Antenna_Mask = 0;
  for (std::vector<ANTENNA_ID>::iterator it = txAntennaIds.begin(); it != txAntennaIds.end (); it++)
    {
      uint8_t antennaBitmap = 1 << ((*it) - 1);
      m_TX_Antenna_Mask = m_TX_Antenna_Mask + antennaBitmap;
    }
}
//// NINA ////

void
EdmgBrpRequestElement::SetComebackDelay (uint8_t comebackDelay)
{
  m_comebackDelay = comebackDelay;
}

void
EdmgBrpRequestElement::SetFirstPathTraining (bool firstPathTraining)
{
  m_firstPathTraining = firstPathTraining;
}

void
EdmgBrpRequestElement::SetDualPolarizationTrn (bool dualPolarizationTrn)
{
  m_dualPolarizationTrn = dualPolarizationTrn;
}

void
EdmgBrpRequestElement::SetDigitalBFRequest (bool digitalBfReq)
{
  m_digitalBFRequest = digitalBfReq;
}

void
EdmgBrpRequestElement::SetFeedbackType (FbckType feedbackType)
{
  m_feedbackType = feedbackType;
}

void
EdmgBrpRequestElement::SetNcIndex (uint8_t ncIndex)
{
  ncIndex = ncIndex - 1;
}


uint8_t
EdmgBrpRequestElement::GetL_RX (void) const
{
  return m_L_RX;
}

uint8_t
EdmgBrpRequestElement::GetL_TX_RX (void) const
{
  return m_L_TX_RX;
}
uint16_t
EdmgBrpRequestElement::GetTXSectorID (void) const
{
  return m_TXSectorID;
}
uint8_t
EdmgBrpRequestElement::GetRequestedEDMG_TRN_UnitP (void) const
{
  if (m_reqEDMG_TRN_UnitP <= 2)
    return m_reqEDMG_TRN_UnitP;
  else if (m_reqEDMG_TRN_UnitP == 3)
    return 4;
  else
    NS_FATAL_ERROR("Invalid Requested EDMG TRN Unit P value");
}

uint8_t
EdmgBrpRequestElement::GetRequestedEDMG_TRN_UnitM (void) const
{
  return m_reqEDMG_TRN_UnitM + 1;
}

uint8_t
EdmgBrpRequestElement::GetRequestedEDMG_TRN_UnitN (void) const
{
  if ((m_reqEDMG_TRN_UnitN == 0) || (m_reqEDMG_TRN_UnitN == 1) || (m_reqEDMG_TRN_UnitN == 3))
    return  m_reqEDMG_TRN_UnitN + 1;
  else if (m_reqEDMG_TRN_UnitN == 2)
    {
      if ((m_reqEDMG_TRN_UnitM == 2) || (m_reqEDMG_TRN_UnitM == 5) || (m_reqEDMG_TRN_UnitM == 8)
          || (m_reqEDMG_TRN_UnitM == 11) || (m_reqEDMG_TRN_UnitM == 14) )
        return 3;
      else if ((m_reqEDMG_TRN_UnitM == 7) || (m_reqEDMG_TRN_UnitM == 15))
        return 8;
      else
        NS_FATAL_ERROR("Invalid Requested EDMG TRN Unit N value");
    }
  else
    NS_FATAL_ERROR("Invalid Requested EDMG TRN Unit N value");
}

bool
EdmgBrpRequestElement::GetBRP_TXSS (void) const
{
  return m_BRP_TXSS;
}

bool
EdmgBrpRequestElement::GetTXSS_Initiator (void) const
{
  return m_TXSS_Initiator;
}
uint8_t
EdmgBrpRequestElement::GetTXSS_Packets (void) const

{
  return m_TXSS_Packets + 1;
}

uint8_t
EdmgBrpRequestElement::GetTXSS_Repeat (void) const
{
  return m_TXSS_Repeat + 1;
}

bool
EdmgBrpRequestElement::GetTXSS_MIMO (void) const
{
  return m_TXSS_MIMO;
}

uint8_t
EdmgBrpRequestElement::GetBRP_CDOWN (void) const
{
  return m_BRP_CDOWN;
}

//// NINA ////
std::vector<ANTENNA_ID>
EdmgBrpRequestElement::GetTX_Antenna_Mask (void) const
{
  std::vector<ANTENNA_ID> antennaIds;
  uint8_t dmgAnt;
  for (uint8_t i = 0; i < 8; i++)
    {
      dmgAnt = (m_TX_Antenna_Mask >> i) & 0x1;
      if (dmgAnt == 1)
        {
          antennaIds.push_back (i+1);
        }
    }
  return antennaIds;
}
//// NINA ////

uint8_t
EdmgBrpRequestElement::GetComebackDelay (void) const
{
  return m_comebackDelay;
}

bool
EdmgBrpRequestElement::GetFirstPathTraining (void) const
{
  return m_firstPathTraining;
}

bool
EdmgBrpRequestElement::GetDualPolarizationTrn (void) const
{
  return m_dualPolarizationTrn;
}

bool
EdmgBrpRequestElement::GetDigitalBFRequest (void) const
{
  return m_digitalBFRequest;
}

FbckType
EdmgBrpRequestElement::GetFeedbackType (void) const
{
  return m_feedbackType;
}

uint8_t
EdmgBrpRequestElement::GetNcIndex (void) const
{
  return m_ncIndex + 1;
}

ATTRIBUTE_HELPER_CPP (EdmgBrpRequestElement);

std::ostream &operator << (std::ostream &os, const EdmgBrpRequestElement &element)
{
  return os;
}

std::istream &operator >> (std::istream &is, EdmgBrpRequestElement &element)
{
  return is;
}

} //namespace ns3
