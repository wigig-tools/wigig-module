/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 MIRKO BANCHI
 * Copyright (c) 2015 HANY ASSASA
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
 * Authors: Mirko Banchi <mk.banchi@gmail.com>
 *          Hany Assasa <hany.assasa@gmail.com>
 */
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ctrl-headers.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CtrlHeaders");

/***********************************
 *       Block ack request
 ***********************************/

NS_OBJECT_ENSURE_REGISTERED (CtrlBAckRequestHeader);

CtrlBAckRequestHeader::CtrlBAckRequestHeader ()
  : m_barAckPolicy (false),
    m_multiTid (false),
    m_compressed (false)
{
}

CtrlBAckRequestHeader::~CtrlBAckRequestHeader ()
{
}

TypeId
CtrlBAckRequestHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CtrlBAckRequestHeader")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<CtrlBAckRequestHeader> ()
  ;
  return tid;
}

TypeId
CtrlBAckRequestHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
CtrlBAckRequestHeader::Print (std::ostream &os) const
{
  os << "TID_INFO=" << m_tidInfo << ", StartingSeq=" << std::hex << m_startingSeq << std::dec;
}

uint32_t
CtrlBAckRequestHeader::GetSerializedSize () const
{
  uint32_t size = 0;
  size += 2; //Bar control
  if (!m_multiTid)
    {
      size += 2; //Starting sequence control
    }
  else
    {
      if (m_compressed)
        {
          size += (2 + 2) * (m_tidInfo + 1);  //Multi-tid block ack
        }
      else
        {
          NS_FATAL_ERROR ("Reserved configuration.");
        }
    }
  return size;
}

void
CtrlBAckRequestHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteHtolsbU16 (GetBarControl ());
  if (!m_multiTid)
    {
      i.WriteHtolsbU16 (GetStartingSequenceControl ());
    }
  else
    {
      if (m_compressed)
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        }
      else
        {
          NS_FATAL_ERROR ("Reserved configuration.");
        }
    }
}

uint32_t
CtrlBAckRequestHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  SetBarControl (i.ReadLsbtohU16 ());
  if (!m_multiTid)
    {
      SetStartingSequenceControl (i.ReadLsbtohU16 ());
    }
  else
    {
      if (m_compressed)
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        }
      else
        {
          NS_FATAL_ERROR ("Reserved configuration.");
        }
    }
  return i.GetDistanceFrom (start);
}

uint16_t
CtrlBAckRequestHeader::GetBarControl (void) const
{
  uint16_t res = 0;
  if (m_barAckPolicy)
    {
      res |= 0x1;
    }
  if (m_multiTid)
    {
      res |= (0x1 << 1);
    }
  if (m_compressed)
    {
      res |= (0x1 << 2);
    }
  res |= (m_tidInfo << 12) & (0xf << 12);
  return res;
}

void
CtrlBAckRequestHeader::SetBarControl (uint16_t bar)
{
  m_barAckPolicy = ((bar & 0x01) == 1) ? true : false;
  m_multiTid = (((bar >> 1) & 0x01) == 1) ? true : false;
  m_compressed = (((bar >> 2) & 0x01) == 1) ? true : false;
  m_tidInfo = (bar >> 12) & 0x0f;
}

uint16_t
CtrlBAckRequestHeader::GetStartingSequenceControl (void) const
{
  return (m_startingSeq << 4) & 0xfff0;
}

void
CtrlBAckRequestHeader::SetStartingSequenceControl (uint16_t seqControl)
{
  m_startingSeq = (seqControl >> 4) & 0x0fff;
}

void
CtrlBAckRequestHeader::SetHtImmediateAck (bool immediateAck)
{
  m_barAckPolicy = immediateAck;
}

void
CtrlBAckRequestHeader::SetType (BlockAckType type)
{
  switch (type)
    {
    case BASIC_BLOCK_ACK:
      m_multiTid = false;
      m_compressed = false;
      break;
    case COMPRESSED_BLOCK_ACK:
      m_multiTid = false;
      m_compressed = true;
      break;
    case MULTI_TID_BLOCK_ACK:
      m_multiTid = true;
      m_compressed = true;
      break;
    default:
      NS_FATAL_ERROR ("Invalid variant type");
      break;
    }
}

void
CtrlBAckRequestHeader::SetTidInfo (uint8_t tid)
{
  m_tidInfo = static_cast<uint16_t> (tid);
}

void
CtrlBAckRequestHeader::SetStartingSequence (uint16_t seq)
{
  m_startingSeq = seq;
}

bool
CtrlBAckRequestHeader::MustSendHtImmediateAck (void) const
{
  return m_barAckPolicy;
}

uint8_t
CtrlBAckRequestHeader::GetTidInfo (void) const
{
  uint8_t tid = static_cast<uint8_t> (m_tidInfo);
  return tid;
}

uint16_t
CtrlBAckRequestHeader::GetStartingSequence (void) const
{
  return m_startingSeq;
}

bool
CtrlBAckRequestHeader::IsBasic (void) const
{
  return (!m_multiTid && !m_compressed) ? true : false;
}

bool
CtrlBAckRequestHeader::IsCompressed (void) const
{
  return (!m_multiTid && m_compressed) ? true : false;
}

bool
CtrlBAckRequestHeader::IsMultiTid (void) const
{
  return (m_multiTid && m_compressed) ? true : false;
}


/***********************************
 *       Block ack response
 ***********************************/

NS_OBJECT_ENSURE_REGISTERED (CtrlBAckResponseHeader);

CtrlBAckResponseHeader::CtrlBAckResponseHeader ()
  : m_baAckPolicy (false),
    m_multiTid (false),
    m_compressed (false)
{
  memset (&bitmap, 0, sizeof (bitmap));
}

CtrlBAckResponseHeader::~CtrlBAckResponseHeader ()
{
}

TypeId
CtrlBAckResponseHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CtrlBAckResponseHeader")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<CtrlBAckResponseHeader> ()
  ;
  return tid;
}

TypeId
CtrlBAckResponseHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
CtrlBAckResponseHeader::Print (std::ostream &os) const
{
  os << "TID_INFO=" << m_tidInfo << ", StartingSeq=" << std::hex << m_startingSeq << std::dec;
}

uint32_t
CtrlBAckResponseHeader::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 2; //Bar control
  if (!m_multiTid)
    {
      if (!m_compressed)
        {
          size += (2 + 128); //Basic block ack
        }
      else
        {
          size += (2 + 8); //Compressed block ack
        }
    }
  else
    {
      if (m_compressed)
        {
          size += (2 + 2 + 8) * (m_tidInfo + 1); //Multi-tid block ack
        }
      else
        {
          NS_FATAL_ERROR ("Reserved configuration.");
        }
    }
  return size;
}

void
CtrlBAckResponseHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteHtolsbU16 (GetBaControl ());
  if (!m_multiTid)
    {
      i.WriteHtolsbU16 (GetStartingSequenceControl ());
      i = SerializeBitmap (i);
    }
  else
    {
      if (m_compressed)
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        }
      else
        {
          NS_FATAL_ERROR ("Reserved configuration.");
        }
    }
}

uint32_t
CtrlBAckResponseHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  SetBaControl (i.ReadLsbtohU16 ());
  if (!m_multiTid)
    {
      SetStartingSequenceControl (i.ReadLsbtohU16 ());
      i = DeserializeBitmap (i);
    }
  else
    {
      if (m_compressed)
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        }
      else
        {
          NS_FATAL_ERROR ("Reserved configuration.");
        }
    }
  return i.GetDistanceFrom (start);
}

void
CtrlBAckResponseHeader::SetHtImmediateAck (bool immediateAck)
{
  m_baAckPolicy = immediateAck;
}

void
CtrlBAckResponseHeader::SetType (BlockAckType type)
{
  switch (type)
    {
    case BASIC_BLOCK_ACK:
      m_multiTid = false;
      m_compressed = false;
      break;
    case COMPRESSED_BLOCK_ACK:
      m_multiTid = false;
      m_compressed = true;
      break;
    case MULTI_TID_BLOCK_ACK:
      m_multiTid = true;
      m_compressed = true;
      break;
    default:
      NS_FATAL_ERROR ("Invalid variant type");
      break;
    }
}

void
CtrlBAckResponseHeader::SetTidInfo (uint8_t tid)
{
  m_tidInfo = static_cast<uint16_t> (tid);
}

void
CtrlBAckResponseHeader::SetStartingSequence (uint16_t seq)
{
  m_startingSeq = seq;
}

bool
CtrlBAckResponseHeader::MustSendHtImmediateAck (void) const
{
  return (m_baAckPolicy) ? true : false;
}

uint8_t
CtrlBAckResponseHeader::GetTidInfo (void) const
{
  uint8_t tid = static_cast<uint8_t> (m_tidInfo);
  return tid;
}

uint16_t
CtrlBAckResponseHeader::GetStartingSequence (void) const
{
  return m_startingSeq;
}

bool
CtrlBAckResponseHeader::IsBasic (void) const
{
  return (!m_multiTid && !m_compressed) ? true : false;
}

bool
CtrlBAckResponseHeader::IsCompressed (void) const
{
  return (!m_multiTid && m_compressed) ? true : false;
}

bool
CtrlBAckResponseHeader::IsMultiTid (void) const
{
  return (m_multiTid && m_compressed) ? true : false;
}

uint16_t
CtrlBAckResponseHeader::GetBaControl (void) const
{
  uint16_t res = 0;
  if (m_baAckPolicy)
    {
      res |= 0x1;
    }
  if (m_multiTid)
    {
      res |= (0x1 << 1);
    }
  if (m_compressed)
    {
      res |= (0x1 << 2);
    }
  res |= (m_tidInfo << 12) & (0xf << 12);
  return res;
}

void
CtrlBAckResponseHeader::SetBaControl (uint16_t ba)
{
  m_baAckPolicy = ((ba & 0x01) == 1) ? true : false;
  m_multiTid = (((ba >> 1) & 0x01) == 1) ? true : false;
  m_compressed = (((ba >> 2) & 0x01) == 1) ? true : false;
  m_tidInfo = (ba >> 12) & 0x0f;
}

uint16_t
CtrlBAckResponseHeader::GetStartingSequenceControl (void) const
{
  return (m_startingSeq << 4) & 0xfff0;
}

void
CtrlBAckResponseHeader::SetStartingSequenceControl (uint16_t seqControl)
{
  m_startingSeq = (seqControl >> 4) & 0x0fff;
}

Buffer::Iterator
CtrlBAckResponseHeader::SerializeBitmap (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  if (!m_multiTid)
    {
      if (!m_compressed)
        {
          for (uint32_t j = 0; j < 64; j++)
            {
              i.WriteHtolsbU16 (bitmap.m_bitmap[j]);
            }
        }
      else
        {
          i.WriteHtolsbU64 (bitmap.m_compressedBitmap);
        }
    }
  else
    {
      if (m_compressed)
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        }
      else
        {
          NS_FATAL_ERROR ("Reserved configuration.");
        }
    }
  return i;
}

Buffer::Iterator
CtrlBAckResponseHeader::DeserializeBitmap (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  if (!m_multiTid)
    {
      if (!m_compressed)
        {
          for (uint32_t j = 0; j < 64; j++)
            {
              bitmap.m_bitmap[j] = i.ReadLsbtohU16 ();
            }
        }
      else
        {
          bitmap.m_compressedBitmap = i.ReadLsbtohU64 ();
        }
    }
  else
    {
      if (m_compressed)
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        }
      else
        {
          NS_FATAL_ERROR ("Reserved configuration.");
        }
    }
  return i;
}

void
CtrlBAckResponseHeader::SetReceivedPacket (uint16_t seq)
{
  if (!IsInBitmap (seq))
    {
      return;
    }
  if (!m_multiTid)
    {
      if (!m_compressed)
        {
          /* To set correctly basic block ack bitmap we need fragment number too.
             So if it's not specified, we consider packet not fragmented. */
          bitmap.m_bitmap[IndexInBitmap (seq)] |= 0x0001;
        }
      else
        {
          bitmap.m_compressedBitmap |= (uint64_t (0x0000000000000001) << IndexInBitmap (seq));
        }
    }
  else
    {
      if (m_compressed)
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        }
      else
        {
          NS_FATAL_ERROR ("Reserved configuration.");
        }
    }
}

void
CtrlBAckResponseHeader::SetReceivedFragment (uint16_t seq, uint8_t frag)
{
  NS_ASSERT (frag < 16);
  if (!IsInBitmap (seq))
    {
      return;
    }
  if (!m_multiTid)
    {
      if (!m_compressed)
        {
          bitmap.m_bitmap[IndexInBitmap (seq)] |= (0x0001 << frag);
        }
      else
        {
          /* We can ignore this...compressed block ack doesn't support
             acknowledgement of single fragments */
        }
    }
  else
    {
      if (m_compressed)
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        }
      else
        {
          NS_FATAL_ERROR ("Reserved configuration.");
        }
    }
}

bool
CtrlBAckResponseHeader::IsPacketReceived (uint16_t seq) const
{
  if (!IsInBitmap (seq))
    {
      return false;
    }
  if (!m_multiTid)
    {
      if (!m_compressed)
        {
          /*It's impossible to say if an entire packet was correctly received. */
          return false;
        }
      else
        {
          uint64_t mask = uint64_t (0x0000000000000001);
          return (((bitmap.m_compressedBitmap >> IndexInBitmap (seq)) & mask) == 1) ? true : false;
        }
    }
  else
    {
      if (m_compressed)
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        }
      else
        {
          NS_FATAL_ERROR ("Reserved configuration.");
        }
    }
  return false;
}

bool
CtrlBAckResponseHeader::IsFragmentReceived (uint16_t seq, uint8_t frag) const
{
  NS_ASSERT (frag < 16);
  if (!IsInBitmap (seq))
    {
      return false;
    }
  if (!m_multiTid)
    {
      if (!m_compressed)
        {
          return ((bitmap.m_bitmap[IndexInBitmap (seq)] & (0x0001 << frag)) != 0x0000) ? true : false;
        }
      else
        {
          /* Although this could make no sense, if packet with sequence number
             equal to <i>seq</i> was correctly received, also all of its fragments
             were correctly received. */
          uint64_t mask = uint64_t (0x0000000000000001);
          return (((bitmap.m_compressedBitmap >> IndexInBitmap (seq)) & mask) == 1) ? true : false;
        }
    }
  else
    {
      if (m_compressed)
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        }
      else
        {
          NS_FATAL_ERROR ("Reserved configuration.");
        }
    }
  return false;
}

uint8_t
CtrlBAckResponseHeader::IndexInBitmap (uint16_t seq) const
{
  uint8_t index;
  if (seq >= m_startingSeq)
    {
      index = seq - m_startingSeq;
    }
  else
    {
      index = 4096 - m_startingSeq + seq;
    }
  NS_ASSERT (index <= 63);
  return index;
}

bool
CtrlBAckResponseHeader::IsInBitmap (uint16_t seq) const
{
  return (seq - m_startingSeq + 4096) % 4096 < 64;
}

const uint16_t*
CtrlBAckResponseHeader::GetBitmap (void) const
{
  return bitmap.m_bitmap;
}

uint64_t
CtrlBAckResponseHeader::GetCompressedBitmap (void) const
{
  return bitmap.m_compressedBitmap;
}

void
CtrlBAckResponseHeader::ResetBitmap (void)
{
  memset (&bitmap, 0, sizeof (bitmap));
}

/*************************
 *  Poll Frame (8.3.1.11)
 *************************/

NS_OBJECT_ENSURE_REGISTERED (CtrlDmgPoll);

CtrlDmgPoll::CtrlDmgPoll ()
    : m_responseOffset (0)
{
  NS_LOG_FUNCTION (this);
}

CtrlDmgPoll::~CtrlDmgPoll ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
CtrlDmgPoll::GetTypeId (void)
{
  NS_LOG_FUNCTION_NOARGS();
  static TypeId tid = TypeId ("ns3::CtrlDmgPoll")
    .SetParent<Header> ()
    .AddConstructor<CtrlDmgPoll> ()
    ;
  return tid;
}

TypeId
CtrlDmgPoll::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION(this);
  return GetTypeId();
}

void
CtrlDmgPoll::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION(this << &os);
  os << "Response Offset=" << m_responseOffset;
}

uint32_t
CtrlDmgPoll::GetSerializedSize () const
{
  NS_LOG_FUNCTION (this);
  return 2;  // Response Offset Field.
}

void
CtrlDmgPoll::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  i.WriteHtolsbU16 (m_responseOffset);
}

uint32_t
CtrlDmgPoll::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  m_responseOffset = i.ReadLsbtohU16 ();
  return i.GetDistanceFrom (start);
}

void
CtrlDmgPoll::SetResponseOffset (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);
  m_responseOffset = value;
}

uint16_t
CtrlDmgPoll::GetResponseOffset (void) const
{
  NS_LOG_FUNCTION (this);
  return m_responseOffset;
}

/*************************************************
 *  Service Period Request (SPR) Frame (8.3.1.12)
 *************************************************/

NS_OBJECT_ENSURE_REGISTERED (CtrlDMG_SPR);

CtrlDMG_SPR::CtrlDMG_SPR ()
{
  NS_LOG_FUNCTION (this);
}

CtrlDMG_SPR::~CtrlDMG_SPR ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
CtrlDMG_SPR::GetTypeId(void)
{
  NS_LOG_FUNCTION_NOARGS ();
  static TypeId tid = TypeId ("ns3::CtrlDMG_SPR")
    .SetParent<Header> ()
    .AddConstructor<CtrlDMG_SPR> ()
    ;
  return tid;
}

TypeId
CtrlDMG_SPR::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

void
CtrlDMG_SPR::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  m_dynamic.Print (os);
  m_bfControl.Print (os);
}

uint32_t
CtrlDMG_SPR::GetSerializedSize () const
{
  NS_LOG_FUNCTION (this);
  return 7;  // Dynamic Allocation Info Field + BF Control.
}

void
CtrlDMG_SPR::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;

  i = m_dynamic.Serialize (i);
  i = m_bfControl.Serialize (i);
}

uint32_t
CtrlDMG_SPR::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;

  i = m_dynamic.Deserialize (i);
  i = m_bfControl.Deserialize (i);

  return i.GetDistanceFrom (start);
}

void
CtrlDMG_SPR::SetDynamicAllocationInfo (DynamicAllocationInfoField field)
{
  m_dynamic = field;
}

void
CtrlDMG_SPR::SetBFControl (BF_Control_Field value)
{
  m_bfControl = value;
}

DynamicAllocationInfoField
CtrlDMG_SPR::CtrlDMG_SPR::GetDynamicAllocationInfo (void) const
{
  return m_dynamic;
}

BF_Control_Field
CtrlDMG_SPR::GetBFControl (void) const
{
  return m_bfControl;
}

/*************************
 * Grant Frame (8.3.1.13)
 *************************/

NS_OBJECT_ENSURE_REGISTERED (CtrlDMG_Grant);

CtrlDMG_Grant::CtrlDMG_Grant ()
{
  NS_LOG_FUNCTION(this);
}

CtrlDMG_Grant::~CtrlDMG_Grant ()
{
  NS_LOG_FUNCTION(this);
}

TypeId
CtrlDMG_Grant::GetTypeId (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  static TypeId tid = TypeId ("ns3::CtrlDMG_Grant")
      .SetParent<Header>()
      .AddConstructor<CtrlDMG_Grant> ()
      ;
  return tid;
}

TypeId
CtrlDMG_Grant::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

/********************************************
 * DMG Denial to Send (DTS) Frame (8.3.1.15)
 ********************************************/

NS_OBJECT_ENSURE_REGISTERED (CtrlDMG_DTS);

CtrlDMG_DTS::CtrlDMG_DTS ()
{
  NS_LOG_FUNCTION (this);
}

CtrlDMG_DTS::~CtrlDMG_DTS ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
CtrlDMG_DTS::GetTypeId (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  static TypeId tid = TypeId ("ns3::CtrlDMG_DTS")
    .SetParent<Header> ()
    .AddConstructor<CtrlDMG_DTS> ()
    ;
  return tid;
}

TypeId
CtrlDMG_DTS::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

void
CtrlDMG_DTS::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
}

uint32_t
CtrlDMG_DTS::GetSerializedSize () const
{
  NS_LOG_FUNCTION (this);
  return 12;  // NAV-SA + NAV-DA
}

void
CtrlDMG_DTS::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;

  WriteTo (i, m_navSA);
  WriteTo (i, m_navDA);
}

uint32_t
CtrlDMG_DTS::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;

  ReadFrom (i, m_navSA);
  ReadFrom (i, m_navDA);

  return i.GetDistanceFrom (start);
}

/****************************************
 *  Sector Sweep (SSW) Frame (8.3.1.16)
 ****************************************/

NS_OBJECT_ENSURE_REGISTERED (CtrlDMG_SSW);

CtrlDMG_SSW::CtrlDMG_SSW ()
{
  NS_LOG_FUNCTION (this);
}

CtrlDMG_SSW::~CtrlDMG_SSW ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
CtrlDMG_SSW::GetTypeId (void)
{
  NS_LOG_FUNCTION_NOARGS();
  static TypeId tid = TypeId ("ns3::CtrlDMG_SSW")
    .SetParent<Header> ()
    .AddConstructor<CtrlDMG_SSW> ()
    ;
  return tid;
}

TypeId
CtrlDMG_SSW::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

void CtrlDMG_SSW::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  m_ssw.Print (os);
  m_sswFeedback.Print (os);
}

uint32_t
CtrlDMG_SSW::GetSerializedSize () const
{
  NS_LOG_FUNCTION (this);
  return 6;  // SSW Field + SSW Feedback Field.
}

void
CtrlDMG_SSW::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;

  i = m_ssw.Serialize (i);
  i = m_sswFeedback.Serialize (i);
}

uint32_t
CtrlDMG_SSW::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;

  i = m_ssw.Deserialize (i);
  i = m_sswFeedback.Deserialize (i);

  return i.GetDistanceFrom (start);
}

void
CtrlDMG_SSW::SetSswField (DMG_SSW_Field &field)
{
  m_ssw = field;
}

void
CtrlDMG_SSW::SetSswFeedbackField (DMG_SSW_FBCK_Field &field)
{
  m_sswFeedback = field;
}

DMG_SSW_Field
CtrlDMG_SSW::GetSswField (void) const
{
  return m_ssw;
}

DMG_SSW_FBCK_Field
CtrlDMG_SSW::GetSswFeedbackField (void) const
{
  return m_sswFeedback;
}

/*********************************************************
 *  Sector Sweep Feedback (SSW-Feedback) Frame (8.3.1.17)
 *********************************************************/

NS_OBJECT_ENSURE_REGISTERED (CtrlDMG_SSW_FBCK);

CtrlDMG_SSW_FBCK::CtrlDMG_SSW_FBCK ()
{
  NS_LOG_FUNCTION (this);
}

CtrlDMG_SSW_FBCK::~CtrlDMG_SSW_FBCK ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
CtrlDMG_SSW_FBCK::GetTypeId (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  static TypeId tid = TypeId ("ns3::CtrlDMG_SSW_FBCK")
    .SetParent<Header>()
    .AddConstructor<CtrlDMG_SSW_FBCK> ()
    ;
  return tid;
}

TypeId
CtrlDMG_SSW_FBCK::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

void
CtrlDMG_SSW_FBCK::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  m_sswFeedback.Print (os);
  m_brpRequest.Print (os);
  m_linkMaintenance.Print (os);
}

uint32_t
CtrlDMG_SSW_FBCK::GetSerializedSize () const
{
  NS_LOG_FUNCTION (this);
  return 8;  // SSW Feedback Field + BRP Request + Beamformed Link Maintenance.
}

void
CtrlDMG_SSW_FBCK::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;

  i = m_sswFeedback.Serialize (i);
  i = m_brpRequest.Serialize (i);
  i = m_linkMaintenance.Serialize (i);
}

uint32_t
CtrlDMG_SSW_FBCK::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION(this << &start);
  Buffer::Iterator i = start;

  i = m_sswFeedback.Deserialize (i);
  i = m_brpRequest.Deserialize (i);
  i = m_linkMaintenance.Deserialize (i);

  return i.GetDistanceFrom (start);
}

void
CtrlDMG_SSW_FBCK::SetSswFeedbackField (DMG_SSW_FBCK_Field &field)
{
  m_sswFeedback = field;
}

void
CtrlDMG_SSW_FBCK::SetBrpRequestField (BRP_Request_Field &field)
{
  m_brpRequest = field;
}

void
CtrlDMG_SSW_FBCK::SetBfLinkMaintenanceField (BF_Link_Maintenance_Field &field)
{
  m_linkMaintenance = field;
}

DMG_SSW_FBCK_Field
CtrlDMG_SSW_FBCK::GetSswFeedbackField (void) const
{
  return m_sswFeedback;
}

BRP_Request_Field
CtrlDMG_SSW_FBCK::GetBrpRequestField (void) const
{
  return m_brpRequest;
}

BF_Link_Maintenance_Field
CtrlDMG_SSW_FBCK::GetBfLinkMaintenanceField (void) const
{
  return m_linkMaintenance;
}

/**********************************************
 * Sector Sweep ACK (SSW-ACK) Frame (8.3.1.18)
 **********************************************/

NS_OBJECT_ENSURE_REGISTERED (CtrlDMG_SSW_ACK);

CtrlDMG_SSW_ACK::CtrlDMG_SSW_ACK ()
{
  NS_LOG_FUNCTION (this);
}

CtrlDMG_SSW_ACK::~CtrlDMG_SSW_ACK ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
CtrlDMG_SSW_ACK::GetTypeId(void)
{
  NS_LOG_FUNCTION_NOARGS ();
  static TypeId tid = TypeId ("ns3::CtrlDMG_SSW_ACK")
    .SetParent<Header> ()
    .AddConstructor<CtrlDMG_SSW_ACK> ()
  ;
  return tid;
}

TypeId
CtrlDMG_SSW_ACK::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

/*******************************
 *  Grant ACK Frame (8.3.1.19)
 *******************************/

NS_OBJECT_ENSURE_REGISTERED (CtrlGrantAck);

CtrlGrantAck::CtrlGrantAck ()
{
  NS_LOG_FUNCTION (this);
  memset (m_reserved, 0, sizeof (uint8_t) * 5);
}

CtrlGrantAck::~CtrlGrantAck ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
CtrlGrantAck::GetTypeId (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  static TypeId tid = TypeId ("ns3::CtrlGrantAck")
    .SetParent<Header> ()
    .AddConstructor<CtrlGrantAck> ()
    ;
  return tid;
}

TypeId
CtrlGrantAck::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

void
CtrlGrantAck::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  m_bfControl.Print (os);
}

uint32_t
CtrlGrantAck::GetSerializedSize () const
{
  NS_LOG_FUNCTION (this);
  return 7;  // Reserved + BF Control.
}

void
CtrlGrantAck::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;

  i.Write (m_reserved, 5);
  i = m_bfControl.Serialize (i);
}

uint32_t
CtrlGrantAck::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;

  i.Read (m_reserved, 5);
  i = m_bfControl.Deserialize (i);

  return i.GetDistanceFrom (start);
}

}  // namespace ns3
