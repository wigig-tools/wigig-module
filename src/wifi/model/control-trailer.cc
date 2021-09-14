/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2020 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "control-trailer.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ControlTrailer");

NS_OBJECT_ENSURE_REGISTERED (ControlTrailer);

TypeId
ControlTrailer::GetTypeId (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  static TypeId tid = TypeId ("ns3::ControlTrailer")
    .SetParent<Header> ()
    .AddConstructor<ControlTrailer> ()
    ;
  return tid;
}

TypeId
ControlTrailer::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

ControlTrailer::ControlTrailer ()
  : m_ctFormatType (CT_TYPE_CTS_DTS),
    m_aggregateChannel (false),
    m_bw (0),
    m_primaryChannelNumber (0),
    m_mimoTransmission (false),
    m_muMimoTransmission (false),
    m_txSectorCombinationIdx (0),
    m_edmgGroupID (0),
    m_hbf (false),
    m_muMimoTransmissionConfigType (0),
    m_muMimoConfigIdx (0),
    m_totalNumberOfSectorsMSB (0),
    m_numberOfRxDMGAntennasMSB (0),
    m_isChannelNumber (false),
    m_totalNumberOfSectors (0),
    m_numberOfRXDMGAntennas (0)
{
}

ControlTrailer::~ControlTrailer ()
{
}

void
ControlTrailer::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
}

uint32_t
ControlTrailer::GetSerializedSize (void) const
{
  return 18;
}

void
ControlTrailer::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  if (m_ctFormatType == CT_TYPE_CTS_DTS)
    {
      uint64_t value = 0;
      uint8_t paddingBytes[10] = {0};
      value |= m_ctFormatType & 0xF;
      value |= (m_aggregateChannel & 0x1) << 4;
      value |= m_bw << 5;
      value |= (m_primaryChannelNumber & 0x7) << 13;
      value |= (m_mimoTransmission & 0x1) << 16;
      value |= (m_muMimoTransmission & 0x1) << 17;
      value |= m_edmgGroupID << 18;
      value |= (m_txSectorCombinationIdx & 0x2F) << 26;
      value |= uint64_t (m_hbf & 0x1) << 32;
      start.WriteHtolsbU64 (value);
      start.Write (paddingBytes, 10);
    }
  else if (m_ctFormatType == CT_TYPE_GRANT_RTS_CTS2SELF)
    {
      uint64_t value = 0;
      uint8_t paddingBytes[10] = {0};
      value |= m_ctFormatType & 0xF;
      value |= (m_aggregateChannel & 0x1) << 4;
      value |= m_bw << 5;
      value |= m_primaryChannelNumber << 13;
      value |= (m_mimoTransmission & 0x1) << 16;
      value |= (m_muMimoTransmission & 0x1) << 17;
      value |= (m_txSectorCombinationIdx & 0x2F) << 18;
      value |= m_edmgGroupID << 24;
      value |= uint64_t (m_muMimoTransmissionConfigType & 0x1) << 32;
      value |= uint64_t (m_muMimoConfigIdx & 0x7) << 33;
      value |= uint64_t (m_totalNumberOfSectorsMSB & 0xF) << 36;
      value |= uint64_t (m_numberOfRxDMGAntennasMSB & 0x1) << 40;
      value |= uint64_t (m_hbf & 0x1) << 41;
      start.WriteHtolsbU64 (value);
      start.Write (paddingBytes, 10);
    }
  else if (m_ctFormatType == CT_TYPE_SPR)
    {
      uint32_t value = 0;
      uint8_t paddingBytes[14] = {0};
      value |= m_ctFormatType & 0xF;
      value |= (m_aggregateChannel & 0x1) << 4;
      value |= m_bw << 5;
      value |= m_primaryChannelNumber << 13;
      value |= (m_isChannelNumber & 0x1) << 13;
      value |= (m_totalNumberOfSectors & 0x7FF) << 17;
      value |= (m_totalNumberOfSectors & 0x7) << 28;
      start.WriteHtolsbU32 (value);
      start.Write (paddingBytes, 14);
    }
  else
    {
      NS_ASSERT_MSG (m_streamMeasurementList.size () >= 1, "At least a single stream's measurement must be reported");
      uint64_t value = 0;
      uint8_t paddingBytes[10] = {0};
      value |= m_ctFormatType & 0xF;
      value |= (m_streamMeasurementList.size () & 0x7) << 4;
      uint8_t offset = 7;
      for (StreamMeasurementListCI it = m_streamMeasurementList.begin () + 1; it != m_streamMeasurementList.end (); it++)
        {
          value |= (it->SNR & 0xF) << offset;
          value |= (it->RSSI & 0x7) << (offset + 4);
          offset += 7;
        }
      start.WriteHtolsbU64 (value);
      start.Write (paddingBytes, 10);
    }
}

uint32_t
ControlTrailer::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint8_t firstByte = i.ReadU8 ();
  m_ctFormatType = static_cast<CT_FORMAT_TYPE> (firstByte & 0xF);
  if (m_ctFormatType == CT_TYPE_CTS_DTS)
    {
      uint64_t value = i.ReadLsbtohU64 ();
      m_aggregateChannel = (firstByte >> 4) & 0x1;
      m_bw |= (firstByte >> 5) & 0x7;
      m_bw |= (value & 0x1F) << 3;
      m_primaryChannelNumber = (value >> 5) & 0x7;
      m_mimoTransmission = (value >> 8) & 0x1;
      m_muMimoTransmission = (value >> 9) & 0x1;
      m_edmgGroupID = (value >> 10) & 0xFF;
      m_txSectorCombinationIdx = (value >> 18) & 0x2F;
      m_hbf = (value >> 24) & 0x1;
    }
  else if (m_ctFormatType == CT_TYPE_GRANT_RTS_CTS2SELF)
    {
      uint64_t value = i.ReadLsbtohU64 ();
      m_aggregateChannel = (firstByte >> 4) & 0x1;
      m_bw |= (firstByte >> 5) & 0x7;
      m_bw |= (value & 0x1F) << 3;
      m_primaryChannelNumber = (value >> 5) & 0x7;
      m_mimoTransmission = (value >> 8) & 0x1;
      m_muMimoTransmission = (value >> 9) & 0x1;
      m_txSectorCombinationIdx = (value >> 10) & 0x2F;
      m_edmgGroupID = (value >> 16) & 0xFF;
      m_muMimoTransmissionConfigType = (value >> 24) & 0x1;
      m_muMimoConfigIdx = (value >> 25) & 0x7;
      m_totalNumberOfSectorsMSB = (value >> 28) & 0xF;
      m_numberOfRxDMGAntennasMSB = (value >> 32) & 0x1;
      m_hbf = (value >> 33) & 0x1;
    }
  else if (m_ctFormatType == CT_TYPE_SPR)
    {
      uint32_t value = i.ReadLsbtohU32 ();
      m_aggregateChannel = (firstByte >> 4) & 0x1;
      m_bw |= (firstByte >> 5) & 0x7;
      m_bw |= (value & 0x1F) << 3;
      m_primaryChannelNumber = (value >> 5) & 0x7;
      m_isChannelNumber = (value >> 8) << 1;
      m_totalNumberOfSectors = (value >> 9) & 0x7FF;
      m_totalNumberOfSectors = (value >> 20) & 0x7;
    }
  else
    {
      NS_ASSERT_MSG (m_streamMeasurementList.size () >= 1, "At least a single stream's measurement must be reported");
      uint64_t value = i.ReadLsbtohU64 ();
      uint8_t numStreams = (value >> 4) & 0x7;
      uint8_t offset = 7;
      for (uint8_t i = 0; i < numStreams; i++)
        {
          StreamMeasurement stream;
          stream.SNR = (value >> offset) & 0xF;
          stream.RSSI = (value >> (offset + 4)) & 0x7;
          offset += 7;
          m_streamMeasurementList.push_back (stream);
        }
    }
  return i.GetDistanceFrom (start);
}

void
ControlTrailer::SetControlTrailerFormatType (CT_FORMAT_TYPE type)
{
  m_ctFormatType = type;
}

void
ControlTrailer::SetChannelAggregation (bool aggregation)
{
  m_aggregateChannel = aggregation;
}

void
ControlTrailer::SetBW (uint8_t bw)
{
  m_bw = bw;
}

void
ControlTrailer::SetPrimaryChannelNumber (uint8_t num)
{
  m_primaryChannelNumber = num;
}

void
ControlTrailer::SetAsMimoTransmission (bool mimo)
{
  m_mimoTransmission = mimo;
}

void
ControlTrailer::SetAsMuMimoTransmission (bool mu)
{
  m_muMimoTransmission = mu;
}

void
ControlTrailer::SetTxSectorCombinationIdx (uint8_t idx)
{
  m_txSectorCombinationIdx = idx;
}

void
ControlTrailer::SetEdmgGroupID (uint8_t id)
{
  m_edmgGroupID = id;
}

void
ControlTrailer::SetMuMimoTransmissionConfigType (uint8_t type)
{
  m_muMimoTransmissionConfigType = type;
}

void
ControlTrailer::SetMuMimoConfigIdx (uint8_t idx)
{
  m_muMimoConfigIdx = idx;
}

void
ControlTrailer::SetTotalNumberOfSectorsMSB (uint8_t msb)
{
  m_totalNumberOfSectorsMSB = msb;
}

void
ControlTrailer::SetNumberOfRxDMGAntennasMSB (uint8_t msb)
{
  m_numberOfRxDMGAntennasMSB = msb;
}

void
ControlTrailer::SetAsChannelNumber (bool isChannelNumber)
{
  m_isChannelNumber = isChannelNumber;
}

void
ControlTrailer::SetTotalNumberOfSectors (uint16_t num)
{
  m_totalNumberOfSectors = num;
}

void
ControlTrailer::SetNumberOfRXDMGAntennas (uint8_t num)
{
  m_numberOfRXDMGAntennas = num;
}

void
ControlTrailer::AddStreamMeasurement (StreamMeasurement &measurement)
{
  NS_ASSERT_MSG (m_streamMeasurementList.size () <= 8, "The maximum number of streams is 8");
  m_streamMeasurementList.push_back (measurement);
}

CT_FORMAT_TYPE
ControlTrailer::GetControlTrailerFormatType (void) const
{
  return m_ctFormatType;
}

bool
ControlTrailer::IsAggregateChannel (void) const
{
  return m_aggregateChannel;
}

uint8_t
ControlTrailer::GetBW (void) const
{
  return m_bw;
}

uint8_t
ControlTrailer::GetPrimaryChannelNumber (void) const
{
  return m_primaryChannelNumber;
}

bool
ControlTrailer::IsMimoTransmission (void) const
{
  return m_mimoTransmission;
}

bool
ControlTrailer::IsMuMimoTransmission (void) const
{
  return m_muMimoTransmission;
}

uint8_t
ControlTrailer::GetTxSectorCombinationIdx (void) const
{
  return m_txSectorCombinationIdx;
}

bool
ControlTrailer::GetHBF (void) const
{
  return m_hbf;
}

uint8_t
ControlTrailer::GetEdmgGroupID (void) const
{
  return m_edmgGroupID;
}

uint8_t
ControlTrailer::GetMuMimoTransmissionConfigType (void) const
{
  return m_muMimoTransmissionConfigType;
}

uint8_t
ControlTrailer::GetMuMimoConfigIdx (void) const
{
  return m_muMimoConfigIdx;
}

uint8_t
ControlTrailer::GetTotalNumberOfSectorsMSB (void) const
{
  return m_totalNumberOfSectorsMSB;
}

uint8_t
ControlTrailer::GetNumberOfRxDMGAntennasMSB (void) const
{
  return m_numberOfRxDMGAntennasMSB;
}

bool
ControlTrailer::IsChannelNumber (void) const
{
  return m_isChannelNumber;
}

uint16_t
ControlTrailer::GetTotalNumberOfSectors (void) const
{
  return m_totalNumberOfSectors;
}

uint8_t
ControlTrailer::GetNumberOfRXDMGAntennas (void) const
{
  return m_numberOfRXDMGAntennas;
}

StreamMeasurement
ControlTrailer::GetStreamMeasurement (uint8_t streamIndex)
{
  NS_ASSERT_MSG (((1 <= streamIndex) && (streamIndex <= 8)), "Stream Index must be between 1 and 8");
  return m_streamMeasurementList.at (streamIndex);
}

} // namespace ns3
