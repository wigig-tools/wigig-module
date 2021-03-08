/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"

#include "edmg-short-ssw.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ShortSSW");

NS_OBJECT_ENSURE_REGISTERED (ShortSSW);

ShortSSW::ShortSSW ()
  : m_packetType (SHORT_SSW),
    m_transmissionDirection (BEAMFORMING_INITIATOR),
    m_addressingMode (),
    m_sourceAID (0),
    m_destinationAID (0),
    m_cdown (0),
    m_rfChainID (0),
    m_bssID (0),
    m_unassociated (false),
    m_sisoFbckDuration (0),
    m_feedback (0)
{
  NS_LOG_FUNCTION (this);
}

ShortSSW::~ShortSSW ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
ShortSSW::GetTypeId (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  static TypeId tid = TypeId ("ns3::ShortSSW")
    .SetParent<Header> ()
    .AddConstructor<ShortSSW> ()
    ;
  return tid;
}

TypeId
ShortSSW::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

void ShortSSW::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
}

uint32_t
ShortSSW::GetSerializedSize () const
{
  NS_LOG_FUNCTION (this);
  return 6;
}

void
ShortSSW::Serialize (Buffer::Iterator start) const
{
  /* Note: The order of the elements is different from the one in the standard in order to be able to have the correct size */
  NS_LOG_FUNCTION (this << &start);
  uint32_t value1 = 0;
  uint16_t value2 = 0;
  value1 |= (m_packetType & 0x1);
  value1 |= (m_transmissionDirection & 0x1) << 1;
  value1 |= (m_addressingMode & 0x1) << 2;
  value1 |= (m_cdown & 0x7FF) << 3;
  value1 |= (m_rfChainID & 0x7) << 14 ;

  if ((m_transmissionDirection == BEAMFORMING_INITIATOR) && (m_addressingMode == INDIVIDUAL_ADRESS))
    {
      value1 |= (m_bssID & 0x3FF) << 17;
      value1 |= (m_unassociated & 0x1) << 27;
    }
  else if ((m_transmissionDirection == BEAMFORMING_INITIATOR) && (m_addressingMode == GROUP_ADDRESS))
    {
      value2 |= (m_sisoFbckDuration & 0x3FF) << 17;
    }
  else if (m_transmissionDirection == BEAMFORMING_RESPONDER)
    {
      value2 |= (m_feedback & 0x7FF) << 17;
    }
  start.WriteHtolsbU32 (value1);
  start.WriteU8 (m_sourceAID);
  start.WriteU8 (m_destinationAID);
}

uint32_t
ShortSSW::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  uint32_t value1 = 0;
  value1 = i.ReadLsbtohU32 ();
  m_packetType = static_cast<BeamformingPacketType> (value1 & 0x1);
  m_transmissionDirection = static_cast<TransmissionDirection> ((value1 >> 1) & 0x1);
  m_addressingMode = static_cast<AddressingMode> ((value1 >> 2) & 0x1);
  m_cdown = (value1 >> 3) & 0x7FF;
  m_rfChainID = (value1 >> 14) & 0x7;
  if (m_transmissionDirection == BEAMFORMING_INITIATOR)
    {
      if (m_addressingMode == INDIVIDUAL_ADRESS)
        {
          m_bssID = (value1 >> 17) & 0x3FF;
          m_unassociated = (value1 >> 27) & 0x1;
        }
      else
        {
          m_sisoFbckDuration = (value1 >> 17) & 0x3FF;
        }
    }
  else
    {
      m_feedback = (value1 >> 17) & 0x7FF;
    }
  m_sourceAID = i.ReadU8 ();
  m_destinationAID = i.ReadU8 ();
  return i.GetDistanceFrom (start);
}

//void
//ShortSSW::GenerateShortScrambledBSSID (Mac48Address bssid)
//{

//}

void
ShortSSW::SetPacketType (BeamformingPacketType type)
{
  m_packetType = type;
}

void
ShortSSW::SetDirection (TransmissionDirection direction)
{
  m_transmissionDirection = direction;
}

void
ShortSSW::SetAddressingMode (AddressingMode mode)
{
  m_addressingMode = mode;
}

void
ShortSSW::SetSourceAID (uint8_t aid)
{
  m_sourceAID = aid;
}

void
ShortSSW::SetDestinationAID (uint8_t aid)
{
  m_destinationAID = aid;
}

void
ShortSSW::SetCDOWN (uint16_t cdown)
{
  m_cdown = cdown;
}

void
ShortSSW::SetRFChainID (uint8_t id)
{
  m_rfChainID = id - 1;
}

void
ShortSSW::SetShortScrambledBSSID (uint16_t bssid)
{
  m_bssID = bssid;
}

void
ShortSSW::SetAsUnassociated (bool value)
{
  m_unassociated = value;
}

void
ShortSSW::SetSisoFbckDuration (Time duration)
{
  int64_t duration_us = static_cast<int64_t> (ceil (static_cast<double> (duration.GetNanoSeconds ()) / 1000));
  NS_ASSERT (duration_us >= 0 && duration_us <= 0x2ff);
  m_sisoFbckDuration = duration_us;
}

void
ShortSSW::SetShortSSWFeedback (uint16_t feedback)
{
  m_feedback = feedback;
}

BeamformingPacketType
ShortSSW::GetPacketType (void) const
{
  return m_packetType;
}

TransmissionDirection
ShortSSW::GetDirection (void) const
{
  return m_transmissionDirection;
}

AddressingMode
ShortSSW::GetAddressingMode (void) const
{
  return m_addressingMode;
}

uint8_t
ShortSSW::GetSourceAID (void) const
{
  return m_sourceAID;
}

uint8_t
ShortSSW::GetDestinationAID (void) const
{
  return m_destinationAID;
}

uint16_t
ShortSSW::GetCDOWN (void) const
{
  return m_cdown;
}

uint8_t
ShortSSW::GetRFChainID (void) const
{
  return m_rfChainID + 1;
}

uint16_t
ShortSSW::GetShortScrambledBSSID (void) const
{
  return m_bssID;
}

bool
ShortSSW::GetUnassociated (void) const
{
  return m_unassociated;
}

Time
ShortSSW::GetSisoFbckDuration (void) const
{
  return MicroSeconds (m_sisoFbckDuration);
}

uint16_t
ShortSSW::GetShortSSWFeedback (void) const
{
  return m_feedback;
}

} // namespace ns3
