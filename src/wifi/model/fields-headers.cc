/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#include "ns3/address-utils.h"
#include "ns3/fatal-error.h"
#include "ns3/log.h"

#include "fields-headers.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FieldsHeaders");

/***********************************
 * Sector Sweep (SSW) Field (8.4a.1)
 ***********************************/

NS_OBJECT_ENSURE_REGISTERED (DMG_SSW_Field);

DMG_SSW_Field::DMG_SSW_Field ()
  : m_dir (BeamformingInitiator),
    m_cdown (0),
    m_sid (0),
    m_antenna_id (0),
    m_length (0)
{
  NS_LOG_FUNCTION (this);
}

DMG_SSW_Field::~DMG_SSW_Field ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
DMG_SSW_Field::GetTypeId(void)
{
  NS_LOG_FUNCTION_NOARGS ();
  static TypeId tid = TypeId ("ns3::DMG_SSW_Field")
    .SetParent<Header> ()
    .AddConstructor<DMG_SSW_Field> ()
  ;
  return tid;
}

TypeId
DMG_SSW_Field::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

void
DMG_SSW_Field::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "Direction=" << m_dir
     << ", CDOWN=" << m_cdown
     << ", SID=" << m_sid
     << ", Antenna ID=" << m_antenna_id
     << ", RXSS Length=" << m_length;
}

uint32_t
DMG_SSW_Field::GetSerializedSize () const
{
  NS_LOG_FUNCTION (this);
  return 3;
}

Buffer::Iterator
DMG_SSW_Field::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  uint8_t ssw[3];

  ssw[0] = m_dir & 0x1;
  ssw[0] |= ((m_cdown & 0x7F) << 1);
  ssw[1] = ((m_cdown & 0x180) >> 7);
  ssw[1] |= ((m_sid & 0x3F) << 2);
  ssw[2] = m_antenna_id & 0x3;
  ssw[2] |= ((m_length & 0x3F) << 2);

  start.Write (ssw, 3);

  return start;
}

Buffer::Iterator
DMG_SSW_Field::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  uint8_t ssw[3];

  start.Read (ssw, 3);
  m_dir = static_cast<BeamformingDirection> (ssw[0] & 0x1);
  m_cdown = (static_cast<uint16_t> (ssw[0] & 0xFE) >> 1) | (static_cast<uint16_t> (ssw[1] & 0x03) << 7);
  m_sid = (ssw[1] & 0xFC) >> 2;
  m_antenna_id = ssw[2] & 0x3;
  m_length = (ssw[2] & 0xFC) >> 2;

  return start;
}

void
DMG_SSW_Field::SetDirection (BeamformingDirection dir)
{
  NS_LOG_FUNCTION (this << dir);
  m_dir = dir;
}

void
DMG_SSW_Field::SetCountDown (uint16_t cdown)
{
  NS_ASSERT ((0 <= cdown) && (cdown <= 511));
  NS_LOG_FUNCTION (this << cdown);
  m_cdown = cdown;
}

void
DMG_SSW_Field::SetSectorID (uint8_t sid)
{
  NS_LOG_FUNCTION (this << sid);
  NS_ASSERT ((1 <= sid) && (sid <= 64));
  m_sid = sid - 1;
}

void
DMG_SSW_Field::SetDMGAntennaID (uint8_t antennaId)
{
  NS_LOG_FUNCTION (this << antennaId);
  NS_ASSERT (1 <= antennaId && antennaId <= 4);
  m_antenna_id = antennaId - 1;
}

void
DMG_SSW_Field::SetRXSSLength (uint8_t length)
{
  NS_LOG_FUNCTION (this << length);
  m_length = length;
}

BeamformingDirection
DMG_SSW_Field::GetDirection (void) const
{
  NS_LOG_FUNCTION (this);
  return m_dir;
}

uint16_t
DMG_SSW_Field::GetCountDown (void) const
{
  NS_LOG_FUNCTION (this);
  return m_cdown;
}

uint8_t
DMG_SSW_Field::GetSectorID (void) const
{
  NS_LOG_FUNCTION (this);
  return m_sid + 1;
}

uint8_t
DMG_SSW_Field::GetDMGAntennaID (void) const
{
  NS_LOG_FUNCTION (this);
  return m_antenna_id + 1;
}

uint8_t
DMG_SSW_Field::GetRXSSLength (void) const
{
  NS_LOG_FUNCTION (this);
  return m_length;
}

/*****************************************
 * Dynamic Allocation Info Field (8.4a.2)
 *****************************************/

NS_OBJECT_ENSURE_REGISTERED (DynamicAllocationInfoField);

DynamicAllocationInfoField::DynamicAllocationInfoField ()
  : m_tid (0),
    m_allocationType (SERVICE_PERIOD_ALLOCATION),
    m_sourceAID (0),
    m_destinationAID (0),
    m_allocationDuration (0)
{
  NS_LOG_FUNCTION (this);
}

DynamicAllocationInfoField::~DynamicAllocationInfoField ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
DynamicAllocationInfoField::GetTypeId (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  static TypeId tid = TypeId ("ns3::DynamicAllocationInfoField")
    .SetParent<Header> ()
    .AddConstructor<DynamicAllocationInfoField> ()
    ;
  return tid;
}

TypeId
DynamicAllocationInfoField::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

void
DynamicAllocationInfoField::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
}

uint32_t
DynamicAllocationInfoField::GetSerializedSize () const
{
  NS_LOG_FUNCTION (this);
  return 5;
}

Buffer::Iterator
DynamicAllocationInfoField::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  uint32_t field1 = 0;
  uint8_t field2 = 0;

  field1 |= m_tid & 0xF;
  field1 |= (m_allocationType & 0x7) << 4;
  field1 |= m_sourceAID << 7;
  field1 |= m_destinationAID << 15;
  field1 |= uint32_t (m_allocationDuration & 0x1FF) << 23;

  field2 |= (m_allocationDuration >> 9) & 0x7F;
  field2 |= (m_reserved & 0x1) << 7;

  i.WriteHtolsbU32 (field1);
  i.WriteU8 (field2);

  return i;
}

Buffer::Iterator
DynamicAllocationInfoField::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  uint32_t field1 = i.ReadLsbtohU32 ();
  uint8_t field2 = i.ReadU8 ();

  m_tid = field1 & 0xF;
  m_allocationType = static_cast<AllocationType> ((m_allocationType >> 4) & 0x7);
  m_sourceAID = (field1 >> 7) & 0xFF;
  m_destinationAID = (field1 >> 15) & 0xFF;
  m_allocationDuration = (static_cast<uint16_t>(field1 >> 23) & 0x1FF) |
                         (static_cast<uint16_t>(field2 & 0x7F) << 9);
  m_reserved = (field2 >> 7) & 0x1;

  return i;
}

void
DynamicAllocationInfoField::SetTID (uint8_t tid)
{
  NS_LOG_FUNCTION (this << tid);
  m_tid = tid;
}

void
DynamicAllocationInfoField::SetAllocationType (AllocationType value)
{
  NS_LOG_FUNCTION (this << value);
  m_allocationType = value;
}

void
DynamicAllocationInfoField::SetSourceAID (uint8_t aid)
{
  NS_LOG_FUNCTION (this << aid);
  m_sourceAID = aid;
}

void
DynamicAllocationInfoField::SetDestinationAID (uint8_t aid)
{
  NS_LOG_FUNCTION (this << aid);
  m_destinationAID = aid;
}

void
DynamicAllocationInfoField::SetAllocationDuration (uint16_t duration)
{
  NS_LOG_FUNCTION (this << duration);
  m_allocationDuration = duration;
}

void
DynamicAllocationInfoField::SetReserved (uint8_t reserved)
{
  NS_LOG_FUNCTION (this << reserved);
  m_reserved = reserved;
}

uint8_t
DynamicAllocationInfoField::GetTID (void) const
{
  NS_LOG_FUNCTION (this);
  return m_tid;
}

AllocationType
DynamicAllocationInfoField::GetAllocationType (void) const
{
  NS_LOG_FUNCTION (this);
  return m_allocationType;
}

uint8_t
DynamicAllocationInfoField::GetSourceAID (void) const
{
  NS_LOG_FUNCTION (this);
  return m_sourceAID;
}

uint8_t
DynamicAllocationInfoField::GetDestinationAID (void) const
{
  NS_LOG_FUNCTION (this);
  return m_destinationAID;
}

uint16_t
DynamicAllocationInfoField::GetAllocationDuration (void) const
{
  NS_LOG_FUNCTION (this);
  return m_allocationDuration;
}

uint8_t
DynamicAllocationInfoField::GetReserved (void)
{
  NS_LOG_FUNCTION (this);
  return m_reserved;
}

/*********************************************
 * Sector Sweep Feedback (SSW) Field (8.4a.3)
 *********************************************/

NS_OBJECT_ENSURE_REGISTERED (DMG_SSW_FBCK_Field);

DMG_SSW_FBCK_Field::DMG_SSW_FBCK_Field ()
  : m_sectors (0),
    m_antennas (0),
    m_snr_report (0),
    m_poll_required (false),
    m_reserved (0),
    m_iss (false)
{
  NS_LOG_FUNCTION (this);
}

DMG_SSW_FBCK_Field::~DMG_SSW_FBCK_Field ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
DMG_SSW_FBCK_Field::GetTypeId(void)
{
  NS_LOG_FUNCTION_NOARGS();
  static TypeId tid = TypeId ("ns3::DMG_SSW_FBCK_Field")
    .SetParent<Header> ()
    .AddConstructor<DMG_SSW_FBCK_Field> ()
    ;
  return tid;
}

TypeId
DMG_SSW_FBCK_Field::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

void
DMG_SSW_FBCK_Field::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  if (m_iss)
    {

    }
  else
    {

    }
}

uint32_t
DMG_SSW_FBCK_Field::GetSerializedSize () const
{
  NS_LOG_FUNCTION (this);
  return 3;
}

Buffer::Iterator
DMG_SSW_FBCK_Field::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  uint8_t ssw[3];
  memset (ssw, 0, sizeof (ssw));

  if (m_iss)
    {
      ssw[0] |= m_sectors & 0xFF;
      ssw[1] |= (m_sectors >> 8) & 0x1;
      ssw[1] |= (m_antennas & 0x3) << 1;
      ssw[1] |= (m_snr_report & 0x1F) << 3;
      ssw[2] |= m_poll_required & 0x1;
      ssw[2] |= (m_reserved & 0x7F) << 1;
    }
  else
    {
      ssw[0] |= m_sectors & 0x3F;
      ssw[0] |= (m_antennas & 0x3) << 6;
      ssw[1] |= m_snr_report;
      ssw[2] |= m_poll_required & 0x1;
      ssw[2] |= (m_reserved & 0x7F) << 1;
    }

  i.Write (ssw, 3);

  return i;
}

Buffer::Iterator
DMG_SSW_FBCK_Field::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  uint8_t ssw[3];
  i.Read (ssw, 3);

  if (m_iss)
    {
      m_sectors = (ssw[0] & 0xFF) | ((ssw[1] & 0x1) << 8);
      m_antennas = (ssw[1] >> 1) & 0x3;
      m_snr_report = (ssw[1] >> 3) & 0x1F;
      m_poll_required = ssw[2] & 0x1;
      m_reserved = (ssw[2] >> 1) & 0x7F;
    }
  else
    {
      m_sectors = ssw[0] & 0x3F;
      m_antennas = (ssw[0] >> 6) & 0x3;
      m_snr_report = ssw[1];
      m_poll_required = ssw[2] & 0x1;
      m_reserved = (ssw[2] >> 1) & 0x7F;
    }

  return i;
}

void
DMG_SSW_FBCK_Field::SetSector (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);
  m_sectors = value;
}

void
DMG_SSW_FBCK_Field::SetDMGAntenna (uint8_t value)
{
  NS_LOG_FUNCTION (this << value);
  m_antennas = value;
}

void
DMG_SSW_FBCK_Field::SetSNRReport (uint8_t value)
{
  NS_LOG_FUNCTION (this << value);
  m_snr_report = value;
}

void
DMG_SSW_FBCK_Field::SetPollRequired (bool value)
{
  NS_LOG_FUNCTION (this << value);
  m_poll_required = value;
}

void
DMG_SSW_FBCK_Field::SetReserved (uint8_t value)
{
  NS_LOG_FUNCTION (this << value);
  m_reserved = value;
}

void
DMG_SSW_FBCK_Field::IsPartOfISS (bool value)
{
  NS_LOG_FUNCTION (this << value);
  m_iss = value;
}

uint16_t
DMG_SSW_FBCK_Field::GetSector (void) const
{
  NS_LOG_FUNCTION (this);
  return m_sectors;
}

uint8_t
DMG_SSW_FBCK_Field::GetDMGAntenna (void) const
{
  NS_LOG_FUNCTION (this);
  return m_antennas;
}

bool
DMG_SSW_FBCK_Field::GetPollRequired (void) const
{
  NS_LOG_FUNCTION (this);
  return m_poll_required;
}

uint8_t
DMG_SSW_FBCK_Field::GetReserved (void) const
{
  NS_LOG_FUNCTION (this);
  return m_reserved;
}

/***********************************
 *    BRP Request Field (8.4a.4).
 ***********************************/

NS_OBJECT_ENSURE_REGISTERED (BRP_Request_Field);

BRP_Request_Field::BRP_Request_Field () :
    m_L_RX (0),
    m_TX_TRN_REQ (false),
    m_MID_REQ (false),
    m_BC_REQ (false),
    m_MID_Grant (false),
    m_BC_Grant (false),
    m_Channel_FBCK_CAP (false),
    m_TXSectorID (0),
    m_OtherAID (0),
    m_TXAntennaID (0),
    m_Reserved (0)
{
    NS_LOG_FUNCTION (this);
}

BRP_Request_Field::~BRP_Request_Field ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
BRP_Request_Field::GetTypeId (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  static TypeId tid = TypeId ("ns3::BRP_Request_Field")
    .SetParent<Header> ()
    .AddConstructor<BRP_Request_Field> ()
  ;
  return tid;
}

TypeId
BRP_Request_Field::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

void
BRP_Request_Field::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "L_RX=" << m_L_RX
     << ", TX_TRN_REQ=" << m_TX_TRN_REQ
     << ", MID_REQ=" << m_MID_REQ
     << ", BC_REQ=" << m_BC_REQ
     << ", MID_Grant=" << m_MID_Grant
     << ", BC_Grant=" << m_BC_Grant
     << ", Channel_FBCK_CAP=" << m_Channel_FBCK_CAP
     << ", TXSectorID=" << m_TXSectorID
     << ", OtherAID=" << m_OtherAID
     << ", TXAntennaID=" << m_TXAntennaID
     << ", Reserved=" << m_Reserved;
}

uint32_t
BRP_Request_Field::GetSerializedSize () const
{
  NS_LOG_FUNCTION (this);
  return 4;
}

Buffer::Iterator
BRP_Request_Field::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  uint32_t brp = 0;

  brp |= m_L_RX & 0x1F;
  brp |= ((m_TX_TRN_REQ & 0x1) << 5);
  brp |= ((m_MID_REQ & 0x1) << 6);
  brp |= ((m_BC_REQ & 0x1) << 7);
  brp |= ((m_MID_Grant & 0x1) << 8);
  brp |= ((m_BC_Grant & 0x1) << 9);
  brp |= ((m_Channel_FBCK_CAP & 0x1) << 10);
  brp |= ((m_TXSectorID & 0x3F) << 11);
  brp |= (m_OtherAID << 17);
  brp |= ((m_TXAntennaID & 0x3) << 25);
  brp |= ((m_Reserved & 0x1F) << 27);

  i.WriteHtolsbU32 (brp);

  return i;
}

Buffer::Iterator
BRP_Request_Field::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  uint32_t brp = i.ReadLsbtohU32 ();

  m_L_RX =  brp & 0x1F;
  m_TX_TRN_REQ = (brp >> 5) & 0x1;
  m_MID_REQ = (brp >> 6) & 0x1;
  m_BC_REQ = (brp >> 7) & 0x1;
  m_MID_Grant = (brp >> 8) & 0x1;
  m_BC_Grant = (brp >> 9) & 0x1;
  m_Channel_FBCK_CAP = (brp >> 10) & 0x1;
  m_TXSectorID = (brp >> 11) & 0x3F;
  m_OtherAID = (brp >> 17) & 0xFF;
  m_TXAntennaID = (brp >> 25) & 0x3;
  m_Reserved = (brp >> 27) & 0x1F;

  return i;
}

void
BRP_Request_Field::SetL_RX (uint8_t value)
{
  NS_LOG_FUNCTION (this << &value);
  m_L_RX = value;
}

void
BRP_Request_Field::SetTX_TRN_REQ (bool value)
{
  NS_LOG_FUNCTION (this << &value);
  m_TX_TRN_REQ = value;
}

void BRP_Request_Field::SetMID_REQ (bool value)
{
  NS_LOG_FUNCTION (this << &value);
  m_MID_REQ = value;
}

void
BRP_Request_Field::SetBC_REQ (bool value)
{
  NS_LOG_FUNCTION (this << &value);
  m_BC_REQ = value;
}

void
BRP_Request_Field::SetMID_Grant (bool value)
{
  NS_LOG_FUNCTION (this << &value);
  m_MID_Grant = value;
}

void
BRP_Request_Field::SetBC_Grant (bool value)
{
  NS_LOG_FUNCTION (this << &value);
  m_BC_Grant = value;
}

void
BRP_Request_Field::SetChannel_FBCK_CAP (bool value)
{
  NS_LOG_FUNCTION (this << &value);
  m_Channel_FBCK_CAP = value;
}

void
BRP_Request_Field::SetTXSectorID (uint8_t value)
{
  NS_LOG_FUNCTION (this << &value);
  m_TXSectorID = value;
}

void
BRP_Request_Field::SetOtherAID (uint8_t value)
{
  NS_LOG_FUNCTION (this << &value);
  m_OtherAID = value;
}

void
BRP_Request_Field::SetTXAntennaID (uint8_t value)
{
  NS_LOG_FUNCTION (this << &value);
  m_TXAntennaID = value;
}

void
BRP_Request_Field::SetReserved (uint8_t value)
{
  NS_LOG_FUNCTION (this << &value);
  m_Reserved = value;
}

uint8_t
BRP_Request_Field::GetL_RX (void)
{
  NS_LOG_FUNCTION (this);
  return m_L_RX;
}

bool
BRP_Request_Field::GetTX_TRN_REQ (void)
{
  NS_LOG_FUNCTION (this);
  return m_TX_TRN_REQ;
}

bool
BRP_Request_Field::GetMID_REQ (void)
{
  NS_LOG_FUNCTION (this);
  return m_MID_REQ;
}

bool
BRP_Request_Field::GetBC_REQ (void)
{
  NS_LOG_FUNCTION (this);
  return m_BC_REQ;
}

bool
BRP_Request_Field::GetMID_Grant (void)
{
  NS_LOG_FUNCTION (this);
  return m_MID_Grant;
}

bool
BRP_Request_Field::GetBC_Grant (void)
{
  NS_LOG_FUNCTION (this);
  return m_BC_Grant;
}

bool
BRP_Request_Field::GetChannel_FBCK_CAP (void)
{
  NS_LOG_FUNCTION (this);
  return m_Channel_FBCK_CAP;
}

uint8_t
BRP_Request_Field::GetTXSectorID (void)
{
  NS_LOG_FUNCTION (this);
  return m_TXSectorID;
}

uint8_t
BRP_Request_Field::GetOtherAID (void)
{
  NS_LOG_FUNCTION (this);
  return m_OtherAID;
}

uint8_t
BRP_Request_Field::GetTXAntennaID (void)
{
  NS_LOG_FUNCTION (this);
  return m_TXAntennaID;
}

uint8_t
BRP_Request_Field::GetReserved (void)
{
  NS_LOG_FUNCTION (this);
  return m_Reserved;
}

/***************************************
 * Beamforming Control Field (8.4a.5)
 ***************************************/

NS_OBJECT_ENSURE_REGISTERED (BF_Control_Field);

BF_Control_Field::BF_Control_Field ()
  : m_beamformTraining (false),
    m_isInitiatorTxss (false),
    m_isResponderTxss (false),
    m_sectors (0),
    m_antennas (0),
    m_rxssLength(0),
    m_rxssTXRate (0),
    m_reserved (0)
{
  NS_LOG_FUNCTION(this);
}

BF_Control_Field::~BF_Control_Field ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
BF_Control_Field::GetTypeId (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  static TypeId tid = TypeId ("ns3::BF_Control_Field")
    .SetParent<Header>()
    .AddConstructor<BF_Control_Field> ()
  ;
  return tid;
}

TypeId
BF_Control_Field::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

void
BF_Control_Field::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);

  os << "Beamforming Training=" << m_beamformTraining
     << ", IsInitiatorTXSS=" << m_isInitiatorTxss
     << ", IsResponderTXSS=" << m_isResponderTxss;

  if (m_isInitiatorTxss && m_isResponderTxss)
    {
      os << ", Total Number of Sectors=" << m_sectors
         << ", Number of RX DMG Antennas=" << m_antennas;
    }
  else
    {
      os << ", RXSS Length=" << m_rxssLength
         << ", RXSSTxRate=" << m_rxssTXRate;
    }

  os << ", Reserved=" << m_reserved;
}

uint32_t
BF_Control_Field::GetSerializedSize () const
{
  NS_LOG_FUNCTION (this);
  return 2;
}

Buffer::Iterator
BF_Control_Field::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  uint16_t value = 0;

  /* Common Subfields */
  value |= m_beamformTraining & 0x1;
  value |= ((m_isInitiatorTxss & 0x1) << 1);
  value |= ((m_isResponderTxss & 0x1) << 2);

  if (m_isInitiatorTxss && m_isResponderTxss)
    {
      value |= ((m_sectors & 0x7F) << 3);
      value |= ((m_antennas & 0x3) << 10);
      value |= ((m_reserved & 0xF) << 12);
    }
  else
    {
      value |= ((m_rxssLength & 0x3F) << 3);
      value |= ((m_rxssTXRate & 0x1) << 9);
      value |= ((m_reserved & 0x3F) << 10);
    }

  i.WriteHtolsbU16 (value);

  return i;
}

Buffer::Iterator
BF_Control_Field::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  uint8_t value = i.ReadLsbtohU16 ();

  m_beamformTraining = value & 0x1;
  m_isInitiatorTxss = ((value >> 1) & 0x1);
  m_isResponderTxss = ((value >> 2) & 0x1);

  if (m_isInitiatorTxss && m_isResponderTxss)
    {
      m_sectors |= ((value >> 3) & 0x7F);
      m_antennas |= ((value >> 10) & 0x3);
      m_reserved |= ((value >> 12) & 0xF);
    }
  else
    {
      m_rxssLength |= ((value >> 3) & 0x3F);
      m_rxssTXRate |= ((value >> 9) & 0x1);
      m_reserved |= ((value >> 10) & 0x3F);
    }

  return i;
}

void
BF_Control_Field::SetBeamformTraining (bool value)
{
  NS_LOG_FUNCTION (this << value);
  m_beamformTraining = value;
}

void
BF_Control_Field::SetAsInitiatorTxss (bool value)
{
  NS_LOG_FUNCTION (this << value);
  m_isInitiatorTxss = value;
}

void
BF_Control_Field::SetAsResponderTxss (bool value)
{
  NS_LOG_FUNCTION (this << value);
  m_isResponderTxss = value;
}

void
BF_Control_Field::SetTotalNumberOfSectors (uint8_t sectors)
{
  NS_LOG_FUNCTION (this << sectors);
  m_sectors = sectors;
}

void
BF_Control_Field::SetNumberOfRxDmgAntennas (uint8_t antennas)
{
  NS_LOG_FUNCTION (this << antennas);
  m_antennas = antennas;
}

void
BF_Control_Field::SetRxssLength (uint8_t length)
{
  NS_LOG_FUNCTION (this << length);
  m_rxssLength = length;
}

void
BF_Control_Field::SetRxssTxRate (bool rate)
{
  NS_LOG_FUNCTION (this << rate);
  m_rxssTXRate = rate;
}

bool
BF_Control_Field::IsBeamformTraining (void) const
{
  NS_LOG_FUNCTION (this);
  return m_beamformTraining;
}

bool
BF_Control_Field::IsInitiatorTxss (void) const
{
  NS_LOG_FUNCTION (this);
  return m_isInitiatorTxss;
}

bool
BF_Control_Field::IsResponderTxss (void) const
{
  NS_LOG_FUNCTION (this);
  return m_isResponderTxss;
}

uint8_t
BF_Control_Field::GetTotalNumberOfSectors (void) const
{
  NS_LOG_FUNCTION (this);
  return m_sectors;
}

uint8_t
BF_Control_Field::GetNumberOfRxDmgAntennas (void) const
{
  NS_LOG_FUNCTION (this);
  return m_antennas;
}

uint8_t
BF_Control_Field::GetRxssLength (void) const
{
  NS_LOG_FUNCTION (this);
  return m_rxssLength;
}

bool
BF_Control_Field::GetRxssTxRate (void) const
{
  NS_LOG_FUNCTION (this);
  return m_rxssTXRate;
}

/***************************************
 * Beamformed Link Maintenance (8.4a.6)
 ***************************************/

NS_OBJECT_ENSURE_REGISTERED (BF_Link_Maintenance_Field);

BF_Link_Maintenance_Field::BF_Link_Maintenance_Field ()
  : m_unitIndex (UNIT_32US),
    m_value (0),
    m_isMaster (false)
{
  NS_LOG_FUNCTION (this);
}

BF_Link_Maintenance_Field::~BF_Link_Maintenance_Field ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
BF_Link_Maintenance_Field::GetTypeId (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  static TypeId tid = TypeId ("ns3::BF_Link_Maintenance_Field")
      .SetParent<Header> ()
      .AddConstructor<BF_Link_Maintenance_Field> ()
      ;
  return tid;
}

TypeId
BF_Link_Maintenance_Field::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

void
BF_Link_Maintenance_Field::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "Unit Index=" << m_unitIndex
     << ", Value=" << m_value
     << ", isMaster=" << m_isMaster;
}

uint32_t
BF_Link_Maintenance_Field::GetSerializedSize () const
{
  NS_LOG_FUNCTION (this);
  return 1;
}

Buffer::Iterator
BF_Link_Maintenance_Field::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  uint8_t value = 0;

  value |= static_cast<uint8_t> (m_unitIndex) & 0x1;
  value |= ((m_value & 0x3F) << 1);
  value |= ((m_isMaster & 0x1) << 7);

  i.WriteU8(value);

  return i;
}

Buffer::Iterator
BF_Link_Maintenance_Field::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  uint8_t value = i.ReadU8 ();

  m_unitIndex = static_cast<BeamLinkMaintenanceUnitIndex> (value & 0x1);
  m_value = ((value >> 1) & 0x3F);
  m_isMaster = ((value >> 7) & 0x1);

  return i;
}

void
BF_Link_Maintenance_Field::SetUnitIndex (BeamLinkMaintenanceUnitIndex index)
{
  NS_LOG_FUNCTION (this << index);
  m_unitIndex = index;
}

void
BF_Link_Maintenance_Field::SetMaintenanceValue (uint8_t value)
{
  NS_LOG_FUNCTION (this << value);
  m_value = value;
}

void
BF_Link_Maintenance_Field::SetAsMaster (bool value)
{
  NS_LOG_FUNCTION (this << value);
  m_isMaster = value;
}

BeamLinkMaintenanceUnitIndex
BF_Link_Maintenance_Field::GetUnitIndex (void) const
{
  NS_LOG_FUNCTION (this);
  return m_unitIndex;
}

uint8_t
BF_Link_Maintenance_Field::GetMaintenanceValue (void) const
{
  NS_LOG_FUNCTION (this);
  return m_value;
}

bool
BF_Link_Maintenance_Field::IsMaster (void) const
{
  NS_LOG_FUNCTION (this);
  return m_isMaster;
}

/************************************************
*  DMG Beacon Clustering Control Field (8-34c&d)
*************************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtDMGClusteringControlField);

ExtDMGClusteringControlField::ExtDMGClusteringControlField ()
  : m_discoveryMode (false)
{
}

ExtDMGClusteringControlField::~ExtDMGClusteringControlField ()
{
}

TypeId
ExtDMGClusteringControlField::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ExtDMGClusteringControlField")
    .SetParent<Header> ()
    .AddConstructor<ExtDMGClusteringControlField> ()
  ;
  return tid;
}

TypeId
ExtDMGClusteringControlField::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
ExtDMGClusteringControlField::GetSerializedSize (void) const
{
  return 8;
}

void
ExtDMGClusteringControlField::Print (std::ostream &os) const
{
  if (m_discoveryMode)
    {

    }
  else
    {

    }
}

Buffer::Iterator
ExtDMGClusteringControlField::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  if (m_discoveryMode)
    {
      WriteTo (i, m_responderAddress);
      i.WriteHtolsbU16 (m_reserved);
    }
  else
    {
      uint8_t value = 0;
      i.WriteU8 (m_beaconSpDuration);
      WriteTo (i, m_clusterID);
      value |= m_clusterMemberRole & 0x3;
      value |= (m_clusterMaxMem & 0x1F) << 2;
      value |= (m_reserved << 7);
      i.WriteU8 (value);
    }
  return i;
}

Buffer::Iterator
ExtDMGClusteringControlField::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  if (m_discoveryMode)
    {
      ReadFrom (i, m_responderAddress);
      m_reserved = i.ReadLsbtohU16 ();
    }
  else
    {
      m_beaconSpDuration = i.ReadU8 ();
      ReadFrom (i, m_clusterID);
      uint8_t value = i.ReadU8 ();
      m_clusterMemberRole = value & 0x3;
      m_clusterMaxMem = (value >> 2) & 0x1F;
      m_reserved = (value >> 7) & 0x1;
    }
  return i;
}

void
ExtDMGClusteringControlField::SetDiscoveryMode (bool value)
{
  m_discoveryMode = value;
}

bool
ExtDMGClusteringControlField::GetDiscoveryMode (void) const
{
  return m_discoveryMode;
}

void
ExtDMGClusteringControlField::SetBeaconSpDuration (uint8_t duration)
{
  m_beaconSpDuration = duration;
}

void
ExtDMGClusteringControlField::SetClusterID (Mac48Address clusterID)
{
  m_clusterID = clusterID;
}

void
ExtDMGClusteringControlField::SetClusterMemberRole (ClusterMemberRole role)
{
  m_clusterMemberRole = static_cast<ClusterMemberRole> (role);
}

void
ExtDMGClusteringControlField::SetClusterMaxMem (uint8_t max)
{
  m_clusterMaxMem = max;
}

void
ExtDMGClusteringControlField::SetReserved (uint16_t value)
{
  m_reserved = value;
}

uint8_t
ExtDMGClusteringControlField::GetBeaconSpDuration (void) const
{
  return m_beaconSpDuration;
}

Mac48Address
ExtDMGClusteringControlField::GetClusterID (void) const
{
  return m_clusterID;
}

ClusterMemberRole
ExtDMGClusteringControlField::GetClusterMemberRole (void) const
{
  return static_cast<ClusterMemberRole> (m_clusterMemberRole);
}

uint8_t
ExtDMGClusteringControlField::GetClusterMaxMem (void) const
{
  return m_clusterMaxMem;
}

uint16_t
ExtDMGClusteringControlField::GetReserved (void) const
{
  return m_reserved;
}

void
ExtDMGClusteringControlField::SetABFT_ResponderAddress (Mac48Address address)
{
  m_responderAddress = address;
}

Mac48Address
ExtDMGClusteringControlField::GetABFT_ResponderAddress (void) const
{
  return m_responderAddress;
}

}  // namespace ns3
