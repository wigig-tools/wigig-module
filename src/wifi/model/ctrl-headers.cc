/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 MIRKO BANCHI
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
    m_baType (BASIC_BLOCK_ACK)
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
  switch (m_baType)
    {
      case BASIC_BLOCK_ACK:
      case COMPRESSED_BLOCK_ACK:
      case EXTENDED_COMPRESSED_BLOCK_ACK:
        size += 2;
        break;
      case MULTI_TID_BLOCK_ACK:
        size += (2 + 2) * (m_tidInfo + 1);
        break;
      default:
        NS_FATAL_ERROR ("Invalid BA type");
        break;
    }
  return size;
}

void
CtrlBAckRequestHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteHtolsbU16 (GetBarControl ());
  switch (m_baType)
    {
      case BASIC_BLOCK_ACK:
      case COMPRESSED_BLOCK_ACK:
      case EXTENDED_COMPRESSED_BLOCK_ACK:
        i.WriteHtolsbU16 (GetStartingSequenceControl ());
        break;
      case MULTI_TID_BLOCK_ACK:
        NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        break;
      default:
        NS_FATAL_ERROR ("Invalid BA type");
        break;
    }
}

uint32_t
CtrlBAckRequestHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  SetBarControl (i.ReadLsbtohU16 ());
  switch (m_baType)
    {
      case BASIC_BLOCK_ACK:
      case COMPRESSED_BLOCK_ACK:
      case EXTENDED_COMPRESSED_BLOCK_ACK:
        SetStartingSequenceControl (i.ReadLsbtohU16 ());
        break;
      case MULTI_TID_BLOCK_ACK:
        NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        break;
      default:
        NS_FATAL_ERROR ("Invalid BA type");
        break;
    }
  return i.GetDistanceFrom (start);
}

uint16_t
CtrlBAckRequestHeader::GetBarControl (void) const
{
  uint16_t res = 0;
  switch (m_baType)
    {
      case BASIC_BLOCK_ACK:
        break;
      case COMPRESSED_BLOCK_ACK:
        res |= (0x02 << 1);
        break;
      case EXTENDED_COMPRESSED_BLOCK_ACK:
        res |= (0x01 << 1);
        break;
      case MULTI_TID_BLOCK_ACK:
        res |= (0x03 << 1);
        break;
      default:
        NS_FATAL_ERROR ("Invalid BA type");
        break;
    }
  res |= (m_tidInfo << 12) & (0xf << 12);
  return res;
}

void
CtrlBAckRequestHeader::SetBarControl (uint16_t bar)
{
  m_barAckPolicy = ((bar & 0x01) == 1) ? true : false;
  if (((bar >> 1) & 0x0f) == 0x03)
    {
      m_baType = MULTI_TID_BLOCK_ACK;
    }
  else if (((bar >> 1) & 0x0f) == 0x01)
    {
      m_baType = EXTENDED_COMPRESSED_BLOCK_ACK;
    }
  else if (((bar >> 1) & 0x0f) == 0x02)
    {
      m_baType = COMPRESSED_BLOCK_ACK;
    }
  else
    {
      m_baType = BASIC_BLOCK_ACK;
    }
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
  m_baType = type;
}

BlockAckType
CtrlBAckRequestHeader::GetType (void) const
{
  return m_baType;
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
  return (m_baType == BASIC_BLOCK_ACK) ? true : false;
}

bool
CtrlBAckRequestHeader::IsCompressed (void) const
{
  return (m_baType == COMPRESSED_BLOCK_ACK) ? true : false;
}

bool
CtrlBAckRequestHeader::IsExtendedCompressed (void) const
{
  return (m_baType == EXTENDED_COMPRESSED_BLOCK_ACK) ? true : false;
}

bool
CtrlBAckRequestHeader::IsMultiTid (void) const
{
  return (m_baType == MULTI_TID_BLOCK_ACK) ? true : false;
}


/***********************************
 *       Block ack response
 ***********************************/

NS_OBJECT_ENSURE_REGISTERED (CtrlBAckResponseHeader);

CtrlBAckResponseHeader::CtrlBAckResponseHeader ()
  : m_baAckPolicy (false),
    m_baType (BASIC_BLOCK_ACK),
    //// WIGIG ////
    m_edmgCompressedBlockAckSize (EDMG_COMPRESSED_BLOCK_ACK_BITMAP_1024)
    //// WIGIG ////
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
  switch (m_baType)
    {
      case BASIC_BLOCK_ACK:
        size += (2 + 128);
        break;
      case COMPRESSED_BLOCK_ACK:
        size += (2 + 8);
        break;
      case EXTENDED_COMPRESSED_BLOCK_ACK:
        size += (2 + 32);
        break;
      case MULTI_TID_BLOCK_ACK:
        size += (2 + 2 + 8) * (m_tidInfo + 1); //Multi-TID block ack
        break;
      //// WIGIG ////
      case EDMG_COMPRESSED_BLOCK_ACK:
        size += (2 + 128 + 1); // Consider that the compressed BlockAckBitmap is 1024 bits
        break;
      //// WIGIG ////
      default:
        NS_FATAL_ERROR ("Invalid BA type");
        break;
    }
  return size;
}

void
CtrlBAckResponseHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteHtolsbU16 (GetBaControl ());
  switch (m_baType)
    {
      case BASIC_BLOCK_ACK:
      case COMPRESSED_BLOCK_ACK:
      case EXTENDED_COMPRESSED_BLOCK_ACK:
        i.WriteHtolsbU16 (GetStartingSequenceControl ());
        i = SerializeBitmap (i);
        break;
      //// WIGIG ////
      case EDMG_COMPRESSED_BLOCK_ACK:
        i.WriteHtolsbU16 (GetStartingSequenceControl ());
        i = SerializeBitmap (i);
        i.WriteU8 (m_rbufcapValue);
        break;
      //// WIGIG ////
      case MULTI_TID_BLOCK_ACK:
        NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        break;
      default:
        NS_FATAL_ERROR ("Invalid BA type");
        break;
    }
}

uint32_t
CtrlBAckResponseHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  SetBaControl (i.ReadLsbtohU16 ());
  switch (m_baType)
    {
      case BASIC_BLOCK_ACK:
      case COMPRESSED_BLOCK_ACK:
      case EXTENDED_COMPRESSED_BLOCK_ACK:
        SetStartingSequenceControl (i.ReadLsbtohU16 ());
        i = DeserializeBitmap (i);
        break;
      //// WIGIG ////
      case EDMG_COMPRESSED_BLOCK_ACK:
        SetStartingSequenceControl (i.ReadLsbtohU16 ());
        i = DeserializeBitmap (i);
        m_rbufcapValue = i.ReadU8 ();
        break;
      //// WIGIG ////
      case MULTI_TID_BLOCK_ACK:
        NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        break;
      default:
        NS_FATAL_ERROR ("Invalid BA type");
        break;
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
  m_baType = type;
}

BlockAckType
CtrlBAckResponseHeader::GetType (void) const
{
  return m_baType;
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
  return (m_baType == BASIC_BLOCK_ACK) ? true : false;
}

bool
CtrlBAckResponseHeader::IsCompressed (void) const
{
  return (m_baType == COMPRESSED_BLOCK_ACK) ? true : false;
}

bool
CtrlBAckResponseHeader::IsExtendedCompressed (void) const
{
  return (m_baType == EXTENDED_COMPRESSED_BLOCK_ACK) ? true : false;
}

bool
CtrlBAckResponseHeader::IsMultiTid (void) const
{
  return (m_baType == MULTI_TID_BLOCK_ACK) ? true : false;
}
//// WIGIG ////
bool
CtrlBAckResponseHeader::IsEdmgCompressed (void) const
{
  return (m_baType == EDMG_COMPRESSED_BLOCK_ACK) ? true : false;
}
//// WIGIG ////

uint16_t
CtrlBAckResponseHeader::GetBaControl (void) const
{
  uint16_t res = 0;
  if (m_baAckPolicy)
    {
      res |= 0x1;
    }
  switch (m_baType)
    {
      case BASIC_BLOCK_ACK:
        break;
      case COMPRESSED_BLOCK_ACK:
        res |= (0x02 << 1);
        break;
      case EXTENDED_COMPRESSED_BLOCK_ACK:
        res |= (0x01 << 1);
        break;
      case MULTI_TID_BLOCK_ACK:
        res |= (0x03 << 1);
        break;
      //// WIGIG ////
      case EDMG_COMPRESSED_BLOCK_ACK:
        res |= (0x08 << 1);
        break;
      //// WIGIG ////
      default:
        NS_FATAL_ERROR ("Invalid BA type");
        break;
    }
  res |= (m_tidInfo << 12) & (0xf << 12);
  return res;
}

void
CtrlBAckResponseHeader::SetBaControl (uint16_t ba)
{
  m_baAckPolicy = ((ba & 0x01) == 1) ? true : false;
  if (((ba >> 1) & 0x0f) == 0x03)
    {
      m_baType = MULTI_TID_BLOCK_ACK;
    }
  else if (((ba >> 1) & 0x0f) == 0x01)
    {
      m_baType = EXTENDED_COMPRESSED_BLOCK_ACK;
    }
  else if (((ba >> 1) & 0x0f) == 0x02)
    {
      m_baType = COMPRESSED_BLOCK_ACK;
    }
  //// WIGIG ////
  else if (((ba >> 1) & 0x0f) == 0x08)
    {
      m_baType = EDMG_COMPRESSED_BLOCK_ACK;
    }
  //// WIGIG ////
  else
    {
      m_baType = BASIC_BLOCK_ACK;
    }
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
//// WIGIG ////
void
CtrlBAckResponseHeader::SetCompresssedBlockAckSize (EDMG_COMPRESSED_BLOCK_ACK_BITMAP_SIZE size)
{
  m_edmgCompressedBlockAckSize = size;
}

void
CtrlBAckResponseHeader::SetReceiveBufferCapability (uint8_t capability)
{
  m_rbufcapValue = capability;
}

uint8_t
CtrlBAckResponseHeader::GetReceiveBufferCapability (void) const
{
  return m_rbufcapValue;
}
//// WIGIG ////

Buffer::Iterator
CtrlBAckResponseHeader::SerializeBitmap (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  switch (m_baType)
    {
      case BASIC_BLOCK_ACK:
          for (uint8_t j = 0; j < 64; j++)
            {
              i.WriteHtolsbU16 (bitmap.m_bitmap[j]);
            }
          break;
      case COMPRESSED_BLOCK_ACK:
          i.WriteHtolsbU64 (bitmap.m_compressedBitmap);
          break;
      case EXTENDED_COMPRESSED_BLOCK_ACK:
          i.WriteHtolsbU64 (bitmap.m_extendedCompressedBitmap[0]);
          i.WriteHtolsbU64 (bitmap.m_extendedCompressedBitmap[1]);
          i.WriteHtolsbU64 (bitmap.m_extendedCompressedBitmap[2]);
          i.WriteHtolsbU64 (bitmap.m_extendedCompressedBitmap[3]);
          break;
      //// WIGIG ////
      case EDMG_COMPRESSED_BLOCK_ACK:
          for (uint8_t j = 0; j < m_edmgCompressedBlockAckSize; j++)
            {
              i.WriteHtolsbU64 (bitmap.m_edmgCompressedBitmap[j]);
            }
          break;
      //// WIGIG ////
      case MULTI_TID_BLOCK_ACK:
        NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        break;
      default:
        NS_FATAL_ERROR ("Invalid BA type");
        break;
    }
  return i;
}

Buffer::Iterator
CtrlBAckResponseHeader::DeserializeBitmap (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  switch (m_baType)
    {
      case BASIC_BLOCK_ACK:
          for (uint8_t j = 0; j < 64; j++)
            {
              bitmap.m_bitmap[j] = i.ReadLsbtohU16 ();
            }
          break;
      case COMPRESSED_BLOCK_ACK:
          bitmap.m_compressedBitmap = i.ReadLsbtohU64 ();
          break;
      case EXTENDED_COMPRESSED_BLOCK_ACK:
          bitmap.m_extendedCompressedBitmap[0] = i.ReadLsbtohU64 ();
          bitmap.m_extendedCompressedBitmap[1] = i.ReadLsbtohU64 ();
          bitmap.m_extendedCompressedBitmap[2] = i.ReadLsbtohU64 ();
          bitmap.m_extendedCompressedBitmap[3] = i.ReadLsbtohU64 ();
          break;
      //// WIGIG ////
      case EDMG_COMPRESSED_BLOCK_ACK:
          for (uint8_t j = 0; j < m_edmgCompressedBlockAckSize; j++)
            {
              bitmap.m_edmgCompressedBitmap[j] = i.ReadLsbtohU64 ();
            }
          break;
      //// WIGIG ////
      case MULTI_TID_BLOCK_ACK:
        NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        break;
      default:
        NS_FATAL_ERROR ("Invalid BA type");
        break;
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
  switch (m_baType)
    {
      case BASIC_BLOCK_ACK:
        {
          /* To set correctly basic block ack bitmap we need fragment number too.
             So if it's not specified, we consider packet not fragmented. */
          bitmap.m_bitmap[IndexInBitmap (seq)] |= 0x0001;
          break;
        }
      case COMPRESSED_BLOCK_ACK:
        {
          bitmap.m_compressedBitmap |= (uint64_t (0x0000000000000001) << IndexInBitmap (seq));
          break;
        }
      case EXTENDED_COMPRESSED_BLOCK_ACK:
        {
          uint16_t index = IndexInBitmap (seq);
          bitmap.m_extendedCompressedBitmap[index/64] |= (uint64_t (0x0000000000000001) << index);
          break;
        }
      //// WIGIG ////
      case EDMG_COMPRESSED_BLOCK_ACK:
        {
          uint16_t index = IndexInBitmap (seq);
          bitmap.m_edmgCompressedBitmap[index/64] |= (uint64_t (0x0000000000000001) << index);
          break;
        }
      //// WIGIG ////
      case MULTI_TID_BLOCK_ACK:
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
          break;
        }
      default:
        {
          NS_FATAL_ERROR ("Invalid BA type");
          break;
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
  switch (m_baType)
    {
      case BASIC_BLOCK_ACK:
        bitmap.m_bitmap[IndexInBitmap (seq)] |= (0x0001 << frag);
        break;
      case COMPRESSED_BLOCK_ACK:
      case EXTENDED_COMPRESSED_BLOCK_ACK:
      //// WIGIG //// /* WIGIG: To check */
      case EDMG_COMPRESSED_BLOCK_ACK:
      //// WIGIG ////
        /* We can ignore this...compressed block ack doesn't support
           acknowledgment of single fragments */
        break;
      case MULTI_TID_BLOCK_ACK:
        NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        break;
      default:
        NS_FATAL_ERROR ("Invalid BA type");
        break;
    }
}

bool
CtrlBAckResponseHeader::IsPacketReceived (uint16_t seq) const
{
  if (!IsInBitmap (seq))
    {
      return false;
    }
  switch (m_baType)
    {
      case BASIC_BLOCK_ACK:
        {
          /*It's impossible to say if an entire packet was correctly received. */
          return false;
        }
      case COMPRESSED_BLOCK_ACK:
        {
          /* Although this could make no sense, if packet with sequence number
             equal to <i>seq</i> was correctly received, also all of its fragments
             were correctly received. */
          uint64_t mask = uint64_t (0x0000000000000001);
          return (((bitmap.m_compressedBitmap >> IndexInBitmap (seq)) & mask) == 1) ? true : false;
        }
      case EXTENDED_COMPRESSED_BLOCK_ACK:
        {
          uint64_t mask = uint64_t (0x0000000000000001);
          uint16_t index = IndexInBitmap (seq);
          return (((bitmap.m_extendedCompressedBitmap[index/64] >> index) & mask) == 1) ? true : false;
        }
      //// WIGIG ////
      case EDMG_COMPRESSED_BLOCK_ACK:
        {
          uint64_t mask = uint64_t (0x0000000000000001);
          uint16_t index = IndexInBitmap (seq);
          return (((bitmap.m_edmgCompressedBitmap[index/64] >> index) & mask) == 1) ? true : false;
        }
      //// WIGIG ////
      case MULTI_TID_BLOCK_ACK:
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
          break;
        }
      default:
        {
          NS_FATAL_ERROR ("Invalid BA type");
          break;
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
  switch (m_baType)
    {
      case BASIC_BLOCK_ACK:
        {
          return ((bitmap.m_bitmap[IndexInBitmap (seq)] & (0x0001 << frag)) != 0x0000) ? true : false;
        }
      case COMPRESSED_BLOCK_ACK:
        {
          /* Although this could make no sense, if packet with sequence number
             equal to <i>seq</i> was correctly received, also all of its fragments
             were correctly received. */
          uint64_t mask = uint64_t (0x0000000000000001);
          return (((bitmap.m_compressedBitmap >> IndexInBitmap (seq)) & mask) == 1) ? true : false;
        }
      case EXTENDED_COMPRESSED_BLOCK_ACK:
        {
          uint64_t mask = uint64_t (0x0000000000000001);
          uint16_t index = IndexInBitmap (seq);
          return (((bitmap.m_extendedCompressedBitmap[index/64] >> index) & mask) == 1) ? true : false;
        }
      //// WIGIG ////
      case EDMG_COMPRESSED_BLOCK_ACK:
      //// WIGIG ////
      case MULTI_TID_BLOCK_ACK:
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
          break;
        }
      default:
        {
          NS_FATAL_ERROR ("Invalid BA type");
          break;
        }
    }
  return false;
}

uint16_t
CtrlBAckResponseHeader::IndexInBitmap (uint16_t seq) const
{
  uint16_t index;
  if (seq >= m_startingSeq)
    {
      index = seq - m_startingSeq;
    }
  else
    {
      index = 4096 - m_startingSeq + seq;
    }
  //// WIGIG ////
  if (m_baType == EDMG_COMPRESSED_BLOCK_ACK)
    {
      NS_ASSERT (index <= 1023);
    }
  //// WIGIG ////
  else if (m_baType == EXTENDED_COMPRESSED_BLOCK_ACK)
    {
      NS_ASSERT (index <= 255);
    }
  else
    {
      NS_ASSERT (index <= 63);
    }
  return index;
}

bool
CtrlBAckResponseHeader::IsInBitmap (uint16_t seq) const
{
  //// WIGIG ////
  if (m_baType == EDMG_COMPRESSED_BLOCK_ACK)
    {
      return (seq - m_startingSeq + 4096) % 4096 < 1024;
    }
  //// WIGIG ////
  else if (m_baType == EXTENDED_COMPRESSED_BLOCK_ACK)
    {
      return (seq - m_startingSeq + 4096) % 4096 < 256;
    }
  else
    {
      return (seq - m_startingSeq + 4096) % 4096 < 64;
    }
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

const uint64_t*
CtrlBAckResponseHeader::GetExtendedCompressedBitmap (void) const
{
  return bitmap.m_extendedCompressedBitmap;
}

const uint64_t*
CtrlBAckResponseHeader::GetEdmgCompressedBitmap (void) const
{
  return bitmap.m_edmgCompressedBitmap;
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
  NS_LOG_FUNCTION_NOARGS ();
  static TypeId tid = TypeId ("ns3::CtrlDmgPoll")
    .SetParent<Header> ()
    .AddConstructor<CtrlDmgPoll> ()
    ;
  return tid;
}

TypeId
CtrlDmgPoll::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

void
CtrlDmgPoll::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
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
CtrlDMG_SPR::GetTypeId (void)
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
  NS_LOG_FUNCTION_NOARGS ();
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
CtrlDMG_SSW_ACK::GetTypeId (void)
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

/***********************************************
 *   TDD Beamforming frame format (9.3.1.24.1)
 ***********************************************/

NS_OBJECT_ENSURE_REGISTERED (TDD_Beamforming);

TDD_Beamforming::TDD_Beamforming ()
{
  NS_LOG_FUNCTION (this);
}

TDD_Beamforming::~TDD_Beamforming ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
TDD_Beamforming::GetTypeId (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  static TypeId tid = TypeId ("ns3::TDD_Beamforming")
    .SetParent<Header> ()
    .AddConstructor<TDD_Beamforming> ()
    ;
  return tid;
}

TypeId
TDD_Beamforming::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

void TDD_Beamforming::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
}

uint32_t
TDD_Beamforming::GetSerializedSize () const
{
  NS_LOG_FUNCTION (this);
  return 1;
}

void
TDD_Beamforming::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  uint8_t controlField = 0;
  controlField |= m_groupBeamforming & 0x1;
  controlField |= (m_beamMeasurement & 0x1) << 1;
  controlField |= (m_beamformingFrameType & 0x3) << 2;
  controlField |= (m_endOfTraining & 0x1) << 4;
  i.WriteU8 (controlField);
}

uint32_t
TDD_Beamforming::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  uint8_t controlField = i.ReadU8 ();
  m_groupBeamforming = controlField & 0x1;
  m_beamMeasurement = (controlField >> 1) & 0x1;
  m_beamformingFrameType = static_cast<TDD_Beamforming_Frame_Type> ((controlField << 2) & 0x3);
  m_endOfTraining = (controlField >> 4) & 0x1;
  return i.GetDistanceFrom (start);
}

void
TDD_Beamforming::SetGroup_Beamforming (bool value)
{
  m_groupBeamforming = value;
}

void
TDD_Beamforming::SetBeam_Measurement (bool value)
{
  m_beamMeasurement = value;
}

void
TDD_Beamforming::SetBeamformingFrameType (TDD_Beamforming_Frame_Type type)
{
  m_beamformingFrameType = type;
}

void
TDD_Beamforming::SetEndOfTraining (bool value)
{
  m_endOfTraining = value;
}

bool
TDD_Beamforming::GetGroup_Beamforming (void) const
{
  return m_groupBeamforming;
}

bool
TDD_Beamforming::GetBeam_Measurement (void) const
{
  return m_beamMeasurement;
}

TDD_Beamforming_Frame_Type
TDD_Beamforming::GetBeamformingFrameType (void) const
{
  return m_beamformingFrameType;
}

bool
TDD_Beamforming::GetEndOfTraining (void) const
{
  return m_endOfTraining;
}

TDD_BEAMFORMING_PROCEDURE
TDD_Beamforming::GetBeamformingProcedure (Mac48Address receiver) const
{
  return TDD_BEAMFORMING_INDIVIDUAL;
}

/*********************************************
 * TDD Sector Sweep (SSW) format (9.3.1.24.2)
 *********************************************/

NS_OBJECT_ENSURE_REGISTERED (TDD_Beamforming_SSW);

TDD_Beamforming_SSW::TDD_Beamforming_SSW ()
{
  NS_LOG_FUNCTION (this);
}

TDD_Beamforming_SSW::~TDD_Beamforming_SSW ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
TDD_Beamforming_SSW::GetTypeId (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  static TypeId tid = TypeId ("ns3::TDD_Beamforming_SSW")
    .SetParent<TDD_Beamforming> ()
    .AddConstructor<TDD_Beamforming_SSW> ()
    ;
  return tid;
}

TypeId
TDD_Beamforming_SSW::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

void TDD_Beamforming_SSW::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
}

uint32_t
TDD_Beamforming_SSW::GetSerializedSize () const
{
  NS_LOG_FUNCTION (this);
  return 6;
}

void
TDD_Beamforming_SSW::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
//  Buffer::Iterator i = start;
}

uint32_t
TDD_Beamforming_SSW::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  return i.GetDistanceFrom (start);
}

void
TDD_Beamforming_SSW::SetTxSectorID (uint16_t sectorID)
{
  m_sectorID = sectorID;
}

void
TDD_Beamforming_SSW::SetTxAntennaID (uint8_t antennaID)
{
  m_antennaID = antennaID;
}

void
TDD_Beamforming_SSW::SetCountIndex (uint8_t index)
{
  m_countIndex = index;
}

void
TDD_Beamforming_SSW::SetBeamformingTimeUnit (uint8_t unit)
{
  m_beamformingTimeUnit = unit;
}

void
TDD_Beamforming_SSW::SetTransmitPeriod (uint8_t period)
{
  m_transmitPeriod = period;
}

void
TDD_Beamforming_SSW::SetResponderFeedbackOffset (uint16_t offset)
{
  m_responderFeedbackOffset = offset;
}

void
TDD_Beamforming_SSW::SetInitiatorAckOffset (uint16_t offset)
{
  m_initiatorAckOffset = offset;
}

void
TDD_Beamforming_SSW::SetNumberOfRequestedFeedback (uint8_t feedback)
{
  m_numRequestedFeedback = feedback;
}


void
TDD_Beamforming_SSW::SetTDDSlotCDOWN (uint16_t cdown)
{
  m_tddSlotCDOWN = cdown;
}

void
TDD_Beamforming_SSW::SetFeedbackRequested (bool feedback)
{
  m_feedbackRequested = feedback;
}

uint16_t
TDD_Beamforming_SSW::GetTxSectorID (void) const
{
  return m_sectorID;
}

uint8_t
TDD_Beamforming_SSW::GetTxAntennaID (void) const
{
  return m_antennaID;
}

uint8_t
TDD_Beamforming_SSW::GetCountIndex (void) const
{
  return m_countIndex;
}

uint8_t
TDD_Beamforming_SSW::GetBeamformingTimeUnit (void) const
{
  return m_beamformingTimeUnit;
}

uint8_t
TDD_Beamforming_SSW::GetTransmitPeriod (void) const
{
  return m_transmitPeriod;
}

uint16_t
TDD_Beamforming_SSW::GetResponderFeedbackOffset (void) const
{
  return m_responderFeedbackOffset;
}

uint16_t
TDD_Beamforming_SSW::GetInitiatorAckOffset (void) const
{
  return m_initiatorAckOffset;
}

uint8_t
TDD_Beamforming_SSW::GetNumberOfRequestedFeedback (void) const
{
  return m_numRequestedFeedback;
}

uint16_t
TDD_Beamforming_SSW::GetTDDSlotCDOWN (void) const
{
  return m_tddSlotCDOWN;
}

bool
TDD_Beamforming_SSW::GetFeedbackRequested (void) const
{
  return m_feedbackRequested;
}

/*********************************************
 *     TDD SSW Feedback format (9.3.1.24.3)
 *********************************************/

NS_OBJECT_ENSURE_REGISTERED (TDD_Beamforming_SSW_FEEDBACK);

TDD_Beamforming_SSW_FEEDBACK::TDD_Beamforming_SSW_FEEDBACK ()
{
  NS_LOG_FUNCTION (this);
}

TDD_Beamforming_SSW_FEEDBACK::~TDD_Beamforming_SSW_FEEDBACK ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
TDD_Beamforming_SSW_FEEDBACK::GetTypeId (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  static TypeId tid = TypeId ("ns3::TDD_Beamforming_SSW_FEEDBACK")
    .SetParent<TDD_Beamforming> ()
    .AddConstructor<TDD_Beamforming_SSW_FEEDBACK> ()
    ;
  return tid;
}

TypeId
TDD_Beamforming_SSW_FEEDBACK::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

void TDD_Beamforming_SSW_FEEDBACK::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
}

uint32_t
TDD_Beamforming_SSW_FEEDBACK::GetSerializedSize () const
{
  NS_LOG_FUNCTION (this);
  return TDD_Beamforming::GetSerializedSize () + 6;
}

void
TDD_Beamforming_SSW_FEEDBACK::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  TDD_Beamforming::Serialize (i);
  uint32_t value1 = 0;
  uint16_t value2 = 0;
  value1 |= (m_sectorID & 0x1FF);
  value1 |= (m_antennaID & 0x7) << 9;
  value1 |= (m_decodedSectorID & 0x1FF) << 12;
  value1 |= (m_decodedAntennaID & 0x7) << 21;
  value1 |= (m_snrReport & 0xFF) << 24;
  value2 |= (m_feedbackCountIndex & 0x33);
  i.WriteHtolsbU32 (value1);
  i.WriteHtolsbU16 (value2);
}

uint32_t
TDD_Beamforming_SSW_FEEDBACK::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
//  i += TDD_Beamforming::Deserialize (i);
  uint32_t value1 = i.ReadLsbtohU32 ();
  uint16_t value2 = i.ReadLsbtohU16 ();
  m_sectorID = (value1 & 0x1FF);
  m_antennaID = (value1 >> 9) & 0x7;
  m_decodedSectorID = (value1 >> 12) & 0x1FF;
  m_decodedAntennaID = (value1 >> 21) & 0x7;
  m_snrReport = (value1 >> 24) & 0xFF;
  m_feedbackCountIndex = (value2 & 0x33);
  return i.GetDistanceFrom (start);
}

void
TDD_Beamforming_SSW_FEEDBACK::SetTxSectorID (uint16_t sectorID)
{
  m_sectorID = sectorID;
}

void
TDD_Beamforming_SSW_FEEDBACK::SetTxAntennaID (uint8_t antennaID)
{
  m_antennaID = antennaID;
}

void
TDD_Beamforming_SSW_FEEDBACK::SetDecodedTxSectorID (uint16_t sectorID)
{
  m_decodedSectorID = sectorID;
}

void
TDD_Beamforming_SSW_FEEDBACK::SetDecodedTxAntennaID (uint8_t antennaID)
{
  m_decodedAntennaID = antennaID;
}

void
TDD_Beamforming_SSW_FEEDBACK::SetSnrReport (uint8_t snr)
{
  m_snrReport = snr;
}

void
TDD_Beamforming_SSW_FEEDBACK::SetFeedbackCountIndex (uint8_t index)
{
  m_feedbackCountIndex = index;
}

uint16_t
TDD_Beamforming_SSW_FEEDBACK::GetTxSectorID (void) const
{
  return m_sectorID;
}

uint8_t
TDD_Beamforming_SSW_FEEDBACK::GetTxAntennaID (void) const
{
  return m_antennaID;
}

uint16_t
TDD_Beamforming_SSW_FEEDBACK::GetDecodedTxSectorID (void) const
{
  return m_decodedSectorID;
}

uint8_t
TDD_Beamforming_SSW_FEEDBACK::GetDecodedTxAntennaID (void) const
{
  return m_decodedAntennaID;
}

uint8_t
TDD_Beamforming_SSW_FEEDBACK::GetSnrReport (void) const
{
  return m_snrReport;
}

uint8_t
TDD_Beamforming_SSW_FEEDBACK::GetFeedbackCountIndex (void) const
{
  return m_feedbackCountIndex;
}

/*********************************************
 *      TDD SSW ACK format (9.3.1.24.3)
 *********************************************/

NS_OBJECT_ENSURE_REGISTERED (TDD_Beamforming_SSW_ACK);

TDD_Beamforming_SSW_ACK::TDD_Beamforming_SSW_ACK ()
{
  NS_LOG_FUNCTION (this);
}

TDD_Beamforming_SSW_ACK::~TDD_Beamforming_SSW_ACK ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
TDD_Beamforming_SSW_ACK::GetTypeId (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  static TypeId tid = TypeId ("ns3::TDD_Beamforming_SSW_ACK")
    .SetParent<TDD_Beamforming> ()
    .AddConstructor<TDD_Beamforming_SSW_ACK> ()
  ;
  return tid;
}

TypeId
TDD_Beamforming_SSW_ACK::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

void TDD_Beamforming_SSW_ACK::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
}

uint32_t
TDD_Beamforming_SSW_ACK::GetSerializedSize () const
{
  NS_LOG_FUNCTION (this);
  return 6;
}

void
TDD_Beamforming_SSW_ACK::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
//  Buffer::Iterator i = start;
  if (m_beamformingFrameType == TDD_SSW)
    {

    }
  else if (m_beamformingFrameType == TDD_SSW_Feedback)
    {

    }
}

uint32_t
TDD_Beamforming_SSW_ACK::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;

  return i.GetDistanceFrom (start);
}

void
TDD_Beamforming_SSW_ACK::SetDecodedTxSectorID (uint16_t sectorID)
{
  m_sectorID = sectorID;
}

void
TDD_Beamforming_SSW_ACK::SetDecodedTxAntennaID (uint8_t antennaID)
{
  m_antennaID = antennaID;
}

void
TDD_Beamforming_SSW_ACK::SetCountIndex (uint8_t index)
{
  m_countIndex = index;
}

void
TDD_Beamforming_SSW_ACK::SetTransmitPeriod (uint8_t period)
{
  m_transmitPeriod = period;
}

void
TDD_Beamforming_SSW_ACK::SetSnrReport (uint8_t snr)
{
  m_snrReport = snr;
}

void
TDD_Beamforming_SSW_ACK::SetInitiatorTransmitOffset (uint16_t offset)
{
  m_InitiatorTransmitOffset = offset;
}

void
TDD_Beamforming_SSW_ACK::SetResponderTransmitOffset (uint8_t offset)
{
  m_responderTransmitOffset = offset;
}

void
TDD_Beamforming_SSW_ACK::SetAckCountIndex (uint8_t count)
{
  m_ackCountIndex = count;
}

uint16_t
TDD_Beamforming_SSW_ACK::GetDecodedTxSectorID (void) const
{
  return m_sectorID;
}

uint8_t
TDD_Beamforming_SSW_ACK::GetDecodedTxAntennaID (void) const
{
  return m_antennaID;
}

uint8_t
TDD_Beamforming_SSW_ACK::GetCountIndex (void) const
{
  return m_countIndex;
}

uint8_t
TDD_Beamforming_SSW_ACK::GetTransmitPeriod (void) const
{
  return m_transmitPeriod;
}

uint8_t
TDD_Beamforming_SSW_ACK::GetSnrReport (void) const
{
  return m_snrReport;
}

uint16_t
TDD_Beamforming_SSW_ACK::GetInitiatorTransmitOffGet (void) const
{
  return m_InitiatorTransmitOffset;
}

uint8_t
TDD_Beamforming_SSW_ACK::GetResponderTransmitOffGet (void) const
{
  return m_responderTransmitOffset;
}

uint8_t
TDD_Beamforming_SSW_ACK::GetAckCountIndex (void) const
{
  return m_ackCountIndex;
}

}  // namespace ns3
