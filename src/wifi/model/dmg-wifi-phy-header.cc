/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
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
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */

#include "dmg-wifi-phy-header.h"

namespace ns3 {

//////////////////// IEEE 802.11ad DMG Control PHY Header ////////////////////

NS_OBJECT_ENSURE_REGISTERED (DmgControlHeader);

DmgControlHeader::DmgControlHeader ()
  : m_length (14),
    m_packetType (TRN_T),
    m_trainingLength (0),
    m_scramblerInitializationBits (0)
{
}

DmgControlHeader::~DmgControlHeader ()
{
}

TypeId
DmgControlHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DmgControlHeader")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<DmgControlHeader> ()
  ;
  return tid;
}

TypeId
DmgControlHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
DmgControlHeader::Print (std::ostream &os) const
{
  os << "LENGTH=" << m_length
     << " PACKET_TYPE=" << m_packetType
     << " TRAINING_LENGTH=" << m_trainingLength;
}

uint32_t
DmgControlHeader::GetSerializedSize (void) const
{
  return 5;
}

void
DmgControlHeader::SetLength (uint16_t length, bool isShortSSW)
{
  if (isShortSSW)
    NS_ASSERT_MSG (length == 6, "PSDU Size for Short SSW packets should be 6 octets.");
  else
    NS_ASSERT_MSG ((length >= 14) && (length <= 1023), "PSDU size should be between 14 and 1023 octets.");
  m_length = length;
}

uint16_t
DmgControlHeader::GetLength (void) const
{
  return m_length;
}

void
DmgControlHeader::SetPacketType (PacketType type)
{
  m_packetType = type;
}

PacketType
DmgControlHeader::GetPacketType (void) const
{
  return m_packetType;
}

void
DmgControlHeader::SetTrainingLength (uint16_t length)
{
  NS_ASSERT_MSG (length <= 16, "The maxium number of TRN-Units is 16.");
  m_trainingLength = length;
}

uint16_t
DmgControlHeader::GetTrainingLength (void) const
{
  return m_trainingLength;
}

void
DmgControlHeader::SetControlTrailerPresent (bool flag)
{
  if (flag)
    m_scramblerInitializationBits = 0;
  else
    m_scramblerInitializationBits = 1;
}

bool
DmgControlHeader::IsControlTrailerPresent (void) const
{
  if (m_scramblerInitializationBits == 0)
    return true;
  else
    return false;
}

void
DmgControlHeader::Serialize (Buffer::Iterator start) const
{
  uint16_t value1 = 0;
  value1 = (m_scramblerInitializationBits & 0x3) << 1;
  value1 = (m_length & 0x3FF) << 5;
  value1 = (m_packetType & 0x1) << 15;
  start.WriteU16 (value1);
  start.WriteU8 (m_trainingLength & 0x1F);
  start.WriteU16 (0);
}

uint32_t
DmgControlHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint16_t value1 = i.ReadU16 ();
  m_scramblerInitializationBits = (value1 >> 1) & 0x3;
  m_length = (value1 >> 5) & 0x3FF;
  m_packetType = static_cast<PacketType> ((value1 >> 15) & 0x1);
  m_trainingLength = (i.ReadU8 () & 0x1F);
  i.ReadU16 ();
  return i.GetDistanceFrom (start);
}

//////////////////// IEEE 802.11ad DMG OFDM PHY Header ////////////////////

NS_OBJECT_ENSURE_REGISTERED (DmgOfdmHeader);

DmgOfdmHeader::DmgOfdmHeader ()
  : m_baseMcs (1),
    m_length (1),
    m_packetType (TRN_T),
    m_trainingLength (0),
    m_aggregation (false),
    m_beamTrackingRequest (false),
    m_lastRSSI (0)
{
}

DmgOfdmHeader::~DmgOfdmHeader ()
{
}

TypeId
DmgOfdmHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DmgOfdmHeader")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<DmgOfdmHeader> ()
  ;
  return tid;
}

TypeId
DmgOfdmHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
DmgOfdmHeader::Print (std::ostream &os) const
{
  os << "BASE_MCS=" << +m_baseMcs
     << " LENGTH=" << m_length
     << " PACKET_TYPE=" << m_packetType
     << " TRAINING_LENGTH=" << m_trainingLength
     << " AGGREGATION=" << m_aggregation
     << " BEAM_TRACKING_REQUEST=" << m_beamTrackingRequest
     << " LAST_RSSI=" << +m_lastRSSI;
}

uint32_t
DmgOfdmHeader::GetSerializedSize (void) const
{
  return 8;
}

void
DmgOfdmHeader::SetBaseMcs (uint8_t mcs)
{
  m_baseMcs = mcs;
}

uint8_t
DmgOfdmHeader::GetBaseMcs (void) const
{
  return m_baseMcs;
}

void
DmgOfdmHeader::SetLength (uint32_t length)
{
  m_length = length;
}

uint32_t
DmgOfdmHeader::GetLength (void) const
{
  return m_length;
}

void
DmgOfdmHeader::SetPacketType (PacketType type)
{
  m_packetType = type;
}

PacketType
DmgOfdmHeader::GetPacketType (void) const
{
  return m_packetType;
}

void
DmgOfdmHeader::SetTrainingLength (uint16_t length)
{
  m_trainingLength = length;
}

uint16_t
DmgOfdmHeader::GetTrainingLength (void) const
{
  return m_trainingLength;
}

void
DmgOfdmHeader::SetAggregation (bool aggregation)
{
  m_aggregation = aggregation;
}

bool
DmgOfdmHeader::GetAggregation (void) const
{
  return m_aggregation;
}

void
DmgOfdmHeader::SetBeamTrackingRequest (bool request)
{
  m_beamTrackingRequest = request;
}

bool
DmgOfdmHeader::GetBeamTrackingRequest (void) const
{
  return m_beamTrackingRequest;
}

void
DmgOfdmHeader::SetLastRSSI (uint8_t rssi)
{
  m_lastRSSI = rssi;
}

uint8_t
DmgOfdmHeader::GetLastRSSI (void) const
{
  return m_lastRSSI;
}

void
DmgOfdmHeader::Serialize (Buffer::Iterator start) const
{
  uint32_t bytes1 = (m_baseMcs & 0x1F) << 7;
  bytes1 |= ((m_length & 0x3FFFF) << 12);
  start.WriteU32 (bytes1);
  uint16_t bytes2 = (m_packetType & 0x1);
  bytes2 |= ((m_trainingLength & 0x1F) << 1);
  bytes2 |= ((m_aggregation & 0x1) << 6);
  bytes2 |= ((m_beamTrackingRequest & 0x1) << 7);
  bytes2 |= ((m_lastRSSI & 0xF) << 10);
  start.WriteU16 (bytes2);
  start.WriteU16 (0);
}

uint32_t
DmgOfdmHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint32_t bytes1 = i.ReadU32 ();
  m_baseMcs = (bytes1 >> 7) & 0x1F;
  m_length = (bytes1 >> 12) & 0x3FFFF;
  uint16_t bytes2 = i.ReadU32 ();
  m_packetType = static_cast<PacketType> (bytes2 & 0x1);
  m_trainingLength = (bytes2 >> 1) & 0x1F;
  m_aggregation = (bytes2 >> 6) & 0x1;
  m_beamTrackingRequest = (bytes2 >> 7) & 0x1;
  m_lastRSSI = (bytes2 >> 10) & 0xF;
  start.ReadU16 ();
  return i.GetDistanceFrom (start);
}

//////////////////// IEEE 802.11ad DMG SC PHY Header ////////////////////

NS_OBJECT_ENSURE_REGISTERED (DmgScHeader);

DmgScHeader::DmgScHeader ()
  : m_extendedScMcsIndication (false)
{
}

DmgScHeader::~DmgScHeader ()
{
}

TypeId
DmgScHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DmgScHeader")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<DmgScHeader> ()
  ;
  return tid;
}

TypeId
DmgScHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
DmgScHeader::Print (std::ostream &os) const
{
  os << " EXTENDED_SC_MCS_INDICATION=" << m_extendedScMcsIndication;
}

uint32_t
DmgScHeader::GetSerializedSize (void) const
{
  return 6;
}

void
DmgScHeader::Serialize (Buffer::Iterator start) const
{
  uint32_t bytes1 = (m_baseMcs & 0x1F) << 7;
  bytes1 |= ((m_length & 0x3FFFF) << 12);
  start.WriteU32 (bytes1);
  uint16_t bytes2 = (m_packetType & 0x1);
  bytes2 |= ((m_trainingLength & 0x1F) << 1);
  bytes2 |= ((m_aggregation & 0x1) << 6);
  bytes2 |= ((m_beamTrackingRequest & 0x1) << 7);
  bytes2 |= ((m_lastRSSI & 0xF) << 10);
  bytes2 |= ((m_extendedScMcsIndication & 0xF) << 14);
  start.WriteU16 (bytes2);
  start.WriteU16 (0);
}

uint32_t
DmgScHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint32_t bytes1 = i.ReadU32 ();
  m_baseMcs = (bytes1 >> 7) & 0x1F;
  m_length = (bytes1 >> 12) & 0x3FFFF;
  uint16_t bytes2 = i.ReadU32 ();
  m_packetType = static_cast<PacketType> (bytes2 & 0x1);
  m_trainingLength = (bytes2 >> 1) & 0x1F;
  m_aggregation = (bytes2 >> 6) & 0x1;
  m_beamTrackingRequest = (bytes2 >> 7) & 0x1;
  m_lastRSSI = (bytes2 >> 10) & 0xF;
  m_extendedScMcsIndication = (bytes2 >> 14) & 0xF;
  start.ReadU16 ();
  return i.GetDistanceFrom (start);
}

void
DmgScHeader::SetExtendedScMcsIndication (bool extended)
{
  m_extendedScMcsIndication = extended;
}

bool
DmgScHeader::GetExtendedScMcsIndication (void) const
{
  return m_extendedScMcsIndication;
}

//////////////////// IEEE 802.11ay EDMG Control PHY Header ////////////////////

NS_OBJECT_ENSURE_REGISTERED (EdmgControlHeaderA);

EdmgControlHeaderA::EdmgControlHeaderA ()
  : m_bw (2),
    m_length (14),
    m_edmgTrnLength (0),
    m_rxPerTxUnits (0),
    m_edmgTrnUnitP (0),
    m_edmgTrnUnitM (0),
    m_edmgTrnUnitN (0),
    m_trnSeqLen (TRN_SEQ_LENGTH_NORMAL),
    m_numberOfTxChains (1)
{
}

EdmgControlHeaderA::~EdmgControlHeaderA ()
{
}

TypeId
EdmgControlHeaderA::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EdmgControlHeaderA")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<EdmgControlHeaderA> ()
  ;
  return tid;
}

TypeId
EdmgControlHeaderA::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
EdmgControlHeaderA::Print (std::ostream &os) const
{
  os << " BW=" << m_bw
     << " LENGTH=" << m_length
     << " EDMG TRN LENGTH=" << m_edmgTrnLength
     << " RX TRN UNITS PER EACH TX TRN UNIT=" << m_rxPerTxUnits
     << " EDMG TRN UNIT P" << m_edmgTrnUnitP
     << " EDMG TRN UNIT M" << m_edmgTrnUnitM
     << " EDMG TRN UNIT N" << m_edmgTrnUnitN
     << " TRN Sequence Length" << m_trnSeqLen
     << " Number of Tx Chains" << m_numberOfTxChains;
}

uint32_t
EdmgControlHeaderA::GetSerializedSize (void) const
{
  return 9;
}

void
EdmgControlHeaderA::SetBw (uint8_t bw)
{
  m_bw = bw;
}

uint8_t
EdmgControlHeaderA::GetBw (void) const
{
  return m_bw;
}

void
EdmgControlHeaderA::SetPrimaryChannelNumber (uint8_t chNumber)
{
  m_primaryChannelNumber = chNumber;
}

uint8_t
EdmgControlHeaderA::GetPrimaryChannelNumber (void) const
{
  return m_primaryChannelNumber;
}

void
EdmgControlHeaderA::SetLength (uint32_t length, bool isShortSSW )
{
  if (isShortSSW)
    NS_ASSERT_MSG (length == 6, "PSDU Size for Short SSW packets should be 6 octets.");
  else
    NS_ASSERT_MSG ((length >= 14) && (length <= 1023), "PSDU size should be between 14 and 1023 octets.");
  m_length = length;
}

uint32_t
EdmgControlHeaderA::GetLength(void) const
{
  return m_length;
}

void
EdmgControlHeaderA::SetEdmgTrnLength (uint8_t length)
{
  m_edmgTrnLength = length;
}

uint8_t
EdmgControlHeaderA::GetEdmgTrnLength (void) const
{
  return m_edmgTrnLength;
}

void
EdmgControlHeaderA::SetRxPerTxUnits (uint8_t number)
{
  m_rxPerTxUnits = number;
}

uint8_t
EdmgControlHeaderA::GetRxPerTxUnits (void) const
{
  return m_rxPerTxUnits;
}

void
EdmgControlHeaderA::SetEdmgTrnUnitP (uint8_t number)
{
  if (number <= 2)
    m_edmgTrnUnitP = number;
  else if (number == 4)
    m_edmgTrnUnitP = 3;
  else
    NS_FATAL_ERROR ("Invalid EDMG TRN Unit P value");
}

uint8_t
EdmgControlHeaderA::GetEdmgTrnUnitP (void) const
{
  if (m_edmgTrnUnitP <= 2)
    return m_edmgTrnUnitP;
  else if (m_edmgTrnUnitP == 3)
    return 4;
  else
    NS_FATAL_ERROR ("Invalid EDMG TRN Unit P value");
}

void
EdmgControlHeaderA::SetEdmgTrnUnitM (uint8_t number)
{
  m_edmgTrnUnitM = number - 1;
}

uint8_t
EdmgControlHeaderA::GetEdmgTrnUnitM (void) const
{
  return m_edmgTrnUnitM + 1;
}

void
EdmgControlHeaderA::SetEdmgTrnUnitN (uint8_t number)
{
  if ((number == 3) || (number == 8))
    m_edmgTrnUnitN = 2;
  else if ((number == 1) || (number == 2) || (number == 4))
    m_edmgTrnUnitN = number -1;
  else
    NS_FATAL_ERROR("Invalid EDMG TRN Unit N value");
}

uint8_t
EdmgControlHeaderA::GetEdmgTrnUnitN (void) const
{
  if ((m_edmgTrnUnitN == 0) || (m_edmgTrnUnitN == 1) || (m_edmgTrnUnitN == 3))
    return  m_edmgTrnUnitN + 1;
  else if (m_edmgTrnUnitN == 2)
    {
      if ((m_edmgTrnUnitM == 2) || (m_edmgTrnUnitM == 5) || (m_edmgTrnUnitM == 8)
          || (m_edmgTrnUnitM == 11) || (m_edmgTrnUnitM == 14) )
        return 3;
      else if ((m_edmgTrnUnitM == 7) || (m_edmgTrnUnitM == 15))
        return 8;
      else
        NS_FATAL_ERROR ("Invalid EDMG TRN Unit N value");
    }
  else
    NS_FATAL_ERROR ("Invalid EDMG TRN Unit N value");
}

void
EdmgControlHeaderA::SetTrnSequenceLength (TRN_SEQ_LENGTH length)
{
  m_trnSeqLen = length;
}

TRN_SEQ_LENGTH
EdmgControlHeaderA::GetTrnSequenceLength (void) const
{
  return m_trnSeqLen;
}

void
EdmgControlHeaderA::SetNumberOfTxChains (uint8_t number)
{
  m_numberOfTxChains = number - 1;
}

uint8_t
EdmgControlHeaderA::GetNumberOfTxChains (void) const
{
  return m_numberOfTxChains + 1;
}

void
EdmgControlHeaderA::Serialize (Buffer::Iterator start) const
{
  // TODO
  //  uint32_t value1 = 0;
  //  uint16_t value2 = 0;
  //  value1 |= m_bw;
  //  value1 |= (m_primaryChannelNumber & 0x7) << 8;
  //  value1 |= (m_length & 0x3FF) << 11;
  //  value1 |= (m_edmgTrnLength) << 21;
  //  value1 |= (m_rxPerTxUnits & 0x7) << 29;
  //  value2 |= (m_rxPerTxUnits >> 3) & 0x1F;
  //  value2 |= (m_edmgTrnUnitP & 0x3) << 5;
  //  value2 |= ((m_edmgTrnUnitM & 0xF) << 7);
  //  value2 |= ((m_edmgTrnUnitN & 0x3) << 11);
  //  value2 |= ((m_trnSeqLen & 0x3) << 13);
  //  start.WriteU32 (value1);
  //  start.WriteU16 (value2);
  start.WriteU8 (m_bw);
  uint16_t value = (m_length & 0x3FF) << 3;
  start.WriteHtolsbU16 (value);
  start.WriteU8 (m_edmgTrnLength);
  start.WriteU8 (m_rxPerTxUnits);
  uint8_t value1 = 0;
  value1 |= (m_edmgTrnUnitP & 0x3);
  value1 |= ((m_edmgTrnUnitM & 0xF) << 2);
  value1 |= ((m_edmgTrnUnitN & 0x3) << 6);
  start.WriteU8 (value1);
  uint8_t value2 = 0;
  value2 |= (m_trnSeqLen & 0x3);
  value2 |= ((m_numberOfTxChains & 0x7) << 4);
  start.WriteU8 (value2);
  start.WriteU16 (0);
}

uint32_t
EdmgControlHeaderA::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_bw = i.ReadU8 ();
  uint16_t value = i.ReadLsbtohU16 ();
  m_length = ((value >> 3) & 0x3FF);
  m_edmgTrnLength = i.ReadU8 ();
  m_rxPerTxUnits = i.ReadU8 ();
  uint8_t value1 = i.ReadU8 ();
  m_edmgTrnUnitP = (value1 & 0x3);
  m_edmgTrnUnitM = ((value1 >> 2) & 0xF);
  m_edmgTrnUnitN = ((value1 >> 6) & 0x3);
  uint8_t value2 = i.ReadU8 ();
  m_trnSeqLen = static_cast<TRN_SEQ_LENGTH> (value2 & 0x3);
  m_numberOfTxChains = ((value2 >> 4) & 0x7);
  i.ReadLsbtohU16 ();
  return i.GetDistanceFrom (start);
}

//////////////////// IEEE 802.11ay SU EDMG PHY Header ////////////////////

NS_OBJECT_ENSURE_REGISTERED (EdmgSuHeaderA);

EdmgSuHeaderA::EdmgSuHeaderA ()
  : m_suMuPpdu (false),
    m_chAggregation (false),
    m_beamformed (false),
    m_shortLongLDCP (false),
    m_stbcApplied (false),
    m_numberOfSS (1),
    m_baseMcs (1)
{
}

EdmgSuHeaderA::~EdmgSuHeaderA ()
{
}

TypeId
EdmgSuHeaderA::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EdmgSuHeaderA")
    .SetParent<EdmgControlHeaderA> ()
    .SetGroupName ("Wifi")
    .AddConstructor<EdmgSuHeaderA> ()
  ;
  return tid;
}

TypeId
EdmgSuHeaderA::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
EdmgSuHeaderA::Print (std::ostream &os) const
{
  os << " SU/MU PPDU: " << m_suMuPpdu
     << ", CH_AGGREGATION: " << m_chAggregation
     << ", BW: " << m_bw
     << ", PRIMARY CHANNEL NUMBER: " << m_primaryChannelNumber
     << ", BEAMFORMED: " << m_beamformed
     << ", SHORT/LONG LDCP: " << m_shortLongLDCP
     << ", STCB APPLIED: " << m_stbcApplied
     << ", LENGTH:" << m_length
     << ", EDMG TRN LENGTH: " << m_edmgTrnLength
     << ", RX TRN UNITS PER EACH TX TRN UNIT: " << m_rxPerTxUnits
     << ", EDMG TRN UNIT P: " << m_edmgTrnUnitP
     << ", EDMG TRN UNIT M: " << m_edmgTrnUnitM
     << ", EDMG TRN UNIT N: " << m_edmgTrnUnitN
     << ", TRN Sequence Length: " << m_trnSeqLen
     << ", NUM TX CHAINS: " << m_numberOfTxChains
     << ", BASE MCS: " << m_baseMcs;
}

uint32_t
EdmgSuHeaderA::GetSerializedSize (void) const
{
  return 16;
}

void
EdmgSuHeaderA::SetSuMuPpdu (bool suMuPpdu)
{
   NS_ASSERT_MSG (suMuPpdu == false, "This header should be used only for SU PPDUs");
   m_suMuPpdu = suMuPpdu;
}

bool
EdmgSuHeaderA::GetSuMuPpdu (void) const
{
  return m_suMuPpdu;
}

void
EdmgSuHeaderA::SetChAggregation (bool chAggregation)
{
   m_chAggregation = chAggregation;
}

bool
EdmgSuHeaderA::GetChAggregation (void) const
{
  return m_chAggregation;
}

void
EdmgSuHeaderA::SetBeamformed (bool beamformed)
{
   m_beamformed = beamformed;
}

bool
EdmgSuHeaderA::GetBeamformed (void) const
{
  return m_beamformed;
}

void
EdmgSuHeaderA::SetLdcpCwLength (bool cwLength)
{
  m_shortLongLDCP = cwLength;
}

bool
EdmgSuHeaderA::GetLdcpCwLength (void) const
{
  return m_shortLongLDCP;
}

void
EdmgSuHeaderA::SetStbcApplied (bool stcb)
{
  m_stbcApplied = stcb;
}

bool
EdmgSuHeaderA::GetStbcApplied (void) const
{
  return m_stbcApplied;
}

void
EdmgSuHeaderA::SetLength (uint32_t length)
{
  NS_ASSERT_MSG (length <= 4194303, "PSDU size should be smaller than 4194303 octets.");
  m_length = length;
}

uint32_t
EdmgSuHeaderA::GetLength(void) const
{
  return m_length;
}

void
EdmgSuHeaderA::SetNumberOfSS (uint8_t number)
{
  NS_ASSERT_MSG ((number >= 1) && (number <= 8), "Number of SS should be between 1 and 8.");
  m_numberOfSS = number - 1;
}

uint8_t
EdmgSuHeaderA::GetNumberOfSS (void) const
{
  return m_numberOfSS + 1;
}

void
EdmgSuHeaderA::SetBaseMcs (uint8_t mcs)
{
  m_baseMcs = mcs;
}

uint8_t
EdmgSuHeaderA::GetBaseMcs (void) const
{
  return m_baseMcs;
}

void
EdmgSuHeaderA::Serialize (Buffer::Iterator start) const
{
  uint16_t value1 = 0;
  uint32_t value2 = 0;
  value1 |= (m_suMuPpdu & 0x1);
  value1 |= ((m_chAggregation & 0x1) << 1);
  value1 |= ((m_bw) << 2);
  value1 |= ((m_primaryChannelNumber & 0x3) << 10);
  value1 |= ((m_beamformed & 0x1) << 13);
  value1 |= ((m_shortLongLDCP & 0x1) << 14);
  value1 |= ((m_stbcApplied & 0x1) << 15);
  value2 |= (m_length & 0x3FFFFF);
  value2 |= ((m_numberOfSS & 0x7) << 22);
  value2 |= ((m_baseMcs & 0x1F) << 25);
  start.WriteHtolsbU16 (value1);
  start.WriteHtolsbU32 (value2);
  start.WriteHtolsbU16 (0);
  start.WriteU8 (m_edmgTrnLength);
  start.WriteU8 (m_rxPerTxUnits);

  uint16_t value3 = 0;
  uint16_t value4 = 0;

  value3 |= (m_edmgTrnUnitP & 0x3);
  value3 |= ((m_edmgTrnUnitM & 0xF) << 2);
  value3 |= ((m_edmgTrnUnitN & 0x3) << 6);
  value3 |= ((m_trnSeqLen & 0x3) << 8);
  value4 |= ((m_numberOfTxChains & 0x7) << 2);
  start.WriteHtolsbU16 (value3);
  start.WriteHtolsbU16 (value4);
  start.WriteHtolsbU16 (0);
}

uint32_t
EdmgSuHeaderA::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint16_t value = i.ReadLsbtohU16 ();
  uint32_t value1 = i.ReadLsbtohU32 ();
  m_suMuPpdu = (value & 0x1);
  m_chAggregation = ((value >> 1) & 0x1);
  m_bw = ((value >> 2) & 0xFF);
  m_primaryChannelNumber = ((value >> 10) & 0x3);
  m_beamformed = ((value >> 13) & 0x1);
  m_shortLongLDCP = ((value >> 14) & 0x1);
  m_stbcApplied = ((value >> 15) & 0x1);
  m_length = (value1 & 0x3FFFFF);
  m_numberOfSS = ((value >> 22) & 0x7);
  m_baseMcs = ((value >> 25) & 0x1F);

  i.ReadLsbtohU16 ();
  m_edmgTrnLength = i.ReadU8 ();
  m_rxPerTxUnits = i.ReadU8 ();

  uint16_t value2 = i.ReadLsbtohU16 ();
  uint16_t value3 = i.ReadLsbtohU16 ();
  m_edmgTrnUnitP = (value2 & 0x3);
  m_edmgTrnUnitM = ((value2 >> 2) & 0xF);
  m_edmgTrnUnitN = ((value2 >> 6) & 0x3);
  m_trnSeqLen = static_cast<TRN_SEQ_LENGTH> ((value2 >> 8) & 0x3);
  m_numberOfTxChains = ((value3 >> 2) & 0x7);
  i.ReadLsbtohU16 ();

  return i.GetDistanceFrom (start);
}

NS_OBJECT_ENSURE_REGISTERED (EdmgMuHeaderA);

//////////////////// IEEE 802.11ay MU EDMG PHY Header ////////////////////

EdmgMuHeaderA::EdmgMuHeaderA ()
  : m_suMuPpdu (false),
    m_chAggregation (false)
{
  /* Add empty SS descriptors */
  for (uint8_t i = 0; i < 8; i++)
    m_ssDescriptorSetList.push_back (SSDescriptorSet ());
  m_currentDescriptorIt = m_ssDescriptorSetList.begin ();
}

EdmgMuHeaderA::~EdmgMuHeaderA ()
{
}

TypeId
EdmgMuHeaderA::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EdmgMuHeaderA")
    .SetParent<EdmgControlHeaderA> ()
    .SetGroupName ("Wifi")
    .AddConstructor<EdmgMuHeaderA> ()
  ;
  return tid;
}

TypeId
EdmgMuHeaderA::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
EdmgMuHeaderA::Print (std::ostream &os) const
{
  os << " CH_BANDWIDTH=" << m_bw
     << " EDMG TRN LENGTH=" << m_edmgTrnLength
     << " RX TRN UNITS PER EACH TX TRN UNIT=" << m_rxPerTxUnits
     << " EDMG TRN UNIT P" << m_edmgTrnUnitP
     << " EDMG TRN UNIT M" << m_edmgTrnUnitM
     << " EDMG TRN UNIT N" << m_edmgTrnUnitN
     << " TRN Sequence Length" << m_trnSeqLen
     << " SU/MU PPDU" << m_suMuPpdu
     << " CH_AGGREGATION" << m_chAggregation;
}

uint32_t
EdmgMuHeaderA::GetSerializedSize (void) const
{
  return 16;
}

void
EdmgMuHeaderA::SetSuMuPpdu (bool suMuPpdu)
{
   NS_ASSERT_MSG (suMuPpdu == true, "This header should be used only for MU PPDUs");
   m_suMuPpdu = suMuPpdu;
}

bool
EdmgMuHeaderA::GetSuMuPpdu (void) const
{
  return m_suMuPpdu;
}

void
EdmgMuHeaderA::SetChAggregation (bool chAggregation)
{
   m_chAggregation = chAggregation;
}

bool
EdmgMuHeaderA::GetChAggregation (void) const
{
  return m_chAggregation;
}

void
EdmgMuHeaderA::AddSSDescriptorSet (uint8_t aid, uint8_t numberOfSs)
{
  NS_ASSERT_MSG (m_currentDescriptorIt != m_ssDescriptorSetList.end (), "The maximum number of users is 8");
  m_currentDescriptorIt->AID = aid;
  m_currentDescriptorIt->NumberOfSS = numberOfSs;
  m_currentDescriptorIt++;
}

SSDescriptorSetList
EdmgMuHeaderA::GetSSDescriptorSetList (void) const
{
  return m_ssDescriptorSetList;
}

void
EdmgMuHeaderA::Serialize (Buffer::Iterator start) const
{
  uint16_t value1 = 0;
  value1 |= (m_suMuPpdu & 0x1);
  value1 |= ((m_chAggregation & 0x1) << 1);
  value1 |= ((m_bw) << 2);
  start.WriteHtolsbU16 (value1);

  for (SSDescriptorSetListCI it = m_ssDescriptorSetList.begin (); it != m_ssDescriptorSetList.end (); it++)
    {
      start.WriteU8 (it->AID);
    }
  uint8_t value2 = 0;
  for (uint8_t i = 0; i < 8; i++)
    {
      value2 |= (((m_ssDescriptorSetList[i].NumberOfSS - 1) & 0x1) << i);
    }
  start.WriteU8 (value2);
  start.WriteU8 (m_edmgTrnLength);
  start.WriteU8 (m_rxPerTxUnits);

  uint16_t value3 = 0;
  value3 |= (m_edmgTrnUnitP & 0x3);
  value3 |= ((m_edmgTrnUnitM & 0xF) << 2);
  value3 |= ((m_edmgTrnUnitN & 0x3) << 6);
  value3 |= ((m_trnSeqLen & 0x3) << 8);
  start.WriteHtolsbU16 (value3);
  start.WriteU8 (0);
}

uint32_t
EdmgMuHeaderA::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint16_t value = i.ReadLsbtohU16 ();
  m_suMuPpdu = (value & 0x1);
  m_chAggregation = ((value >> 1) & 0x1);
  m_bw = ((value >> 2) & 0xFF);

  for (uint8_t j = 0; j < 8; j++)
    {
      SSDescriptorSet set;
      set.AID = i.ReadU8 ();
      m_ssDescriptorSetList.push_back (set);
    }

  uint8_t value1 = i.ReadU8 ();
  for (uint8_t j = 0; j < 8; j++)
    {
      uint8_t numberOfSS = ((value1 >> j) & 0x1);
      m_ssDescriptorSetList[j].NumberOfSS = numberOfSS + 1;
    }
  m_edmgTrnLength = i.ReadU8 ();
  m_rxPerTxUnits = i.ReadU8 ();

  uint16_t value2 = i.ReadLsbtohU16 ();
  m_edmgTrnUnitP = (value2 & 0x3);
  m_edmgTrnUnitM = ((value2 >> 2) & 0xF);
  m_edmgTrnUnitN = ((value2 >> 6) & 0x3);
  m_trnSeqLen = static_cast<TRN_SEQ_LENGTH> ((value2 >> 8) & 0x3);
  i.ReadU8 ();

  return i.GetDistanceFrom (start);
}

} //namespace ns3
