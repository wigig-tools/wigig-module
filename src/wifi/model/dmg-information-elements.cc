/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */

#include "ns3/address-utils.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "dmg-information-elements.h"

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
      start.WriteU8 (*i);
    }
}

uint8_t
RequestElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  WifiInformationElementId id;
  while (!start.IsEnd ())
    {
      id = start.ReadU8 ();
      m_list.push_back (id);
    }
  return length;
}

void
RequestElement::AddRequestElementID (WifiInformationElementId id)
{
  m_list.push_back (id);
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
      os << *i << "|";
    }
  os << list[list.size () - 1];
  return os;
}

std::istream &
operator >> (std::istream &is, RequestElement &element)
{
  WifiInformationElementId c1;
  is >> c1;

  element.AddRequestElementID (c1);

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
TimeoutIntervalElement::SetTimeoutIntervalType (enum TimeoutIntervalType type)
{
  m_timeoutIntervalType = type;
}

void
TimeoutIntervalElement::SetTimeoutIntervalValue (uint32_t value)
{
  m_timeoutIntervalValue = value;
}

enum TimeoutIntervalType
TimeoutIntervalElement::GetTimeoutIntervalType () const
{
  return static_cast<enum TimeoutIntervalType> (m_timeoutIntervalType);
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

  element.SetTimeoutIntervalType (static_cast<enum TimeoutIntervalType> (c1));
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
    m_numberOfTapsRequested (0),
    m_sectorIdOrderRequested (false),
    m_snrPresent (false),
    m_channelMeasurementPresent (false),
    m_tapDelayPresent (false),
    m_numberOfTapsPresent (0),
    m_numberOfMeasurements (0),
    m_sectorIdOrderPresent (false),
    m_numberOfBeams (0),
    m_midExtension (false),
    m_capabilityRequest (false)
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
  return 5;
}

void
BeamRefinementElement::SerializeInformationField (Buffer::Iterator start) const
{
  uint32_t value1 = 0;
  uint8_t value2 = 0;

  value1 |= m_initiator & 0x1;
  value1 |= (m_txTrainResponse & 0x1) << 1;
  value1 |= (m_rxTrainResponse & 0x1) << 2;
  value1 |= (m_txTrnOk & 0x1) << 3;
  value1 |= (m_txssFbckReq & 0x1) << 4;
  value1 |= (m_bsFbck & 0x3F) << 5;
  value1 |= (m_bsFbckAntennaId & 0x3) << 11;
  /* FBCK-REQ */
  value1 |= (m_snrRequested & 0x1) << 13;
  value1 |= (m_channelMeasurementRequested & 0x1) << 14;
  value1 |= (m_numberOfTapsRequested & 0x3) << 15;
  value1 |= (m_sectorIdOrderRequested & 0x1) << 17;
  /* FBCK-TYPE */
  value1 |= (m_snrPresent & 0x1) << 18;
  value1 |= (m_channelMeasurementPresent & 0x1) << 19;
  value1 |= (m_tapDelayPresent & 0x1) << 20;
  value1 |= (m_numberOfTapsPresent & 0x3) << 21;
  value1 |= (m_numberOfMeasurements & 0x7F) << 23;
  value1 |= (m_sectorIdOrderPresent & 0x1) << 30;
  value1 |= (m_numberOfBeams & 0x1) << 31;
  value2 |= (m_numberOfBeams >> 1) & 0xF;

  value2 |= (m_midExtension & 0x1) << 4;
  value2 |= (m_capabilityRequest & 0x1) << 5;

  start.WriteHtolsbU32 (value1);
  start.WriteU8 (value2);
}

uint8_t
BeamRefinementElement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;
  uint32_t value1 = i.ReadLsbtohU32 ();
  uint8_t value2 = i.ReadU8 ();

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
  m_numberOfTapsRequested = (value1 >> 15) & 0x3;
  m_sectorIdOrderRequested = (value1 >> 17) & 0x1;
  /* FBCK-TYPE */
  m_snrPresent = (value1 >> 18) & 0x1;
  m_channelMeasurementPresent = (value1 >> 19) & 0x1;
  m_tapDelayPresent = (value1 >> 20) & 0x1;
  m_numberOfTapsPresent = (value1 >> 21) & 0x3;
  m_numberOfMeasurements = (value1 >> 23) & 0x7F;
  m_sectorIdOrderPresent = (value1 >> 30) & 0x1;
  m_numberOfBeams = (value1 >> 31) & 0x01;
  m_numberOfBeams |= (value2 << 1) & 0x1E;

  m_midExtension = (value2 >> 4) & 0x1;
  m_capabilityRequest = (value2 >> 5) & 0x1;

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
BeamRefinementElement::SetBsFbck (uint8_t feedback)
{
  m_bsFbck = feedback;
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
BeamRefinementElement::SetNumberOfTapsRequested (uint8_t number)
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

uint8_t
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
BeamRefinementElement::SetNumberOfTapsPresent (uint8_t number)
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
BeamRefinementElement::SetNumberOfBeams (uint8_t number)
{
  m_numberOfBeams = number;
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

uint8_t
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
}

WifiInformationElementId
ChannelMeasurementFeedbackElement::ElementId () const
{
  return IE_CHANNEL_MEASUREMENT_FEEDBACK;
}

uint8_t
ChannelMeasurementFeedbackElement::GetInformationFieldSize () const
{
  return 6;
}

void
ChannelMeasurementFeedbackElement::SerializeInformationField (Buffer::Iterator start) const
{
  /* Serialize SNR List */
  for (SNR_LIST_ITERATOR iter = m_snrList.begin (); iter != m_snrList.end (); iter++)
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
ChannelMeasurementFeedbackElement::AddSnrItem (SNR snr)
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

SNR_LIST
ChannelMeasurementFeedbackElement::GetSnrList (void) const
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
MultiBandElement::SetStaRole (enum StaRole role)
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

enum StaRole
MultiBandElement::GetStaRole (void) const
{
  return static_cast<enum StaRole> (m_staRole);
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

  uint8_t policyDetail =   i.ReadU8 ();;
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

} //namespace ns3
