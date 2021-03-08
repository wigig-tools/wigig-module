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

#ifndef CTRL_HEADERS_H
#define CTRL_HEADERS_H

#include "ns3/header.h"
#include "block-ack-type.h"
#include "fields-headers.h"

namespace ns3 {

/**
 * \ingroup wifi
 * \brief Headers for BlockAckRequest.
 *
 *  802.11n standard includes three types of BlockAck:
 *    - Basic BlockAck (unique type in 802.11e)
 *    - Compressed BlockAck
 *    - Multi-TID BlockAck
 *  For now only basic BlockAck and compressed BlockAck
 *  are supported.
 *  Basic BlockAck is also default variant.
 */
class CtrlBAckRequestHeader : public Header
{
public:
  CtrlBAckRequestHeader ();
  ~CtrlBAckRequestHeader ();
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  TypeId GetInstanceTypeId (void) const;
  void Print (std::ostream &os) const;
  uint32_t GetSerializedSize (void) const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);

  /**
   * Enable or disable HT immediate Ack.
   *
   * \param immediateAck enable or disable HT immediate Ack
   */
  void SetHtImmediateAck (bool immediateAck);
  /**
   * Set the block ack type.
   *
   * \param type the BA type
   */
  void SetType (BlockAckType type);
  /**
   * Set Traffic ID (TID).
   *
   * \param tid the TID
   */
  void SetTidInfo (uint8_t tid);
  /**
   * Set the starting sequence number from the given
   * raw sequence control field.
   *
   * \param seq the raw sequence control
   */
  void SetStartingSequence (uint16_t seq);

  /**
   * Check if the current Ack Policy is immediate.
   *
   * \return true if the current Ack Policy is immediate,
   *         false otherwise
   */
  bool MustSendHtImmediateAck (void) const;
  /**
   * Return the Block Ack type ID.
   *
   * \return the BA type
   */
  BlockAckType GetType (void) const;
  /**
   * Return the Traffic ID (TID).
   *
   * \return TID
   */
  uint8_t GetTidInfo (void) const;
  /**
   * Return the starting sequence number.
   *
   * \return the starting sequence number
   */
  uint16_t GetStartingSequence (void) const;
  /**
   * Check if the current Ack Policy is Basic Block Ack
   * (i.e. not multi-TID nor compressed).
   *
   * \return true if the current Ack Policy is Basic Block Ack,
   *         false otherwise
   */
  bool IsBasic (void) const;
  /**
   * Check if the current Ack Policy is Compressed Block Ack
   * and not multi-TID.
   *
   * \return true if the current Ack Policy is Compressed Block Ack,
   *         false otherwise
   */
  bool IsCompressed (void) const;
  /**
   * Check if the current Ack Policy is Extended Compressed Block Ack.
   *
   * \return true if the current Ack Policy is Extended Compressed Block Ack,
   *         false otherwise
   */
  bool IsExtendedCompressed (void) const;
  /**
   * Check if the current Ack Policy has Multi-TID Block Ack.
   *
   * \return true if the current Ack Policy has Multi-TID Block Ack,
   *         false otherwise
   */
  bool IsMultiTid (void) const;

  /**
   * Return the starting sequence control.
   *
   * \return the starting sequence control
   */
  uint16_t GetStartingSequenceControl (void) const;


private:
  /**
   * Set the starting sequence control with the given
   * sequence control value
   *
   * \param seqControl the sequence control value
   */
  void SetStartingSequenceControl (uint16_t seqControl);
  /**
   * Return the Block Ack control.
   *
   * \return the Block Ack control
   */
  uint16_t GetBarControl (void) const;
  /**
   * Set the Block Ack control.
   *
   * \param bar the BAR control value
   */
  void SetBarControl (uint16_t bar);

  /**
   * The LSB bit of the BAR control field is used only for the
   * HT (High Throughput) delayed block ack configuration.
   * For now only non HT immediate BlockAck is implemented so this field
   * is here only for a future implementation of HT delayed variant.
   */
  bool m_barAckPolicy;    ///< BAR Ack Policy
  BlockAckType m_baType;  ///< BA type
  uint16_t m_tidInfo;     ///< TID info
  uint16_t m_startingSeq; ///< starting sequence number
};

enum EDMG_COMPRESSED_BLOCK_ACK_BITMAP_SIZE {
  EDMG_COMPRESSED_BLOCK_ACK_BITMAP_64   = 1,
  EDMG_COMPRESSED_BLOCK_ACK_BITMAP_128  = 2,
  EDMG_COMPRESSED_BLOCK_ACK_BITMAP_256  = 4,
  EDMG_COMPRESSED_BLOCK_ACK_BITMAP_512  = 8,
  EDMG_COMPRESSED_BLOCK_ACK_BITMAP_1024 = 16,
};

/**
 * \ingroup wifi
 * \brief Headers for BlockAck response.
 *
 *  802.11n standard includes three types of BlockAck:
 *    - Basic BlockAck (unique type in 802.11e)
 *    - Compressed BlockAck
 *    - Multi-TID BlockAck
 *  For now only basic BlockAck and compressed BlockAck
 *  are supported.
 *  Basic BlockAck is also default variant.
 */
class CtrlBAckResponseHeader : public Header
{
public:
  CtrlBAckResponseHeader ();
  ~CtrlBAckResponseHeader ();
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  TypeId GetInstanceTypeId (void) const;
  void Print (std::ostream &os) const;
  uint32_t GetSerializedSize (void) const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);

  /**
   * Enable or disable HT immediate Ack.
   *
   * \param immediateAck enable or disable HT immediate Ack
   */
  void SetHtImmediateAck (bool immediateAck);
  /**
   * Set the block ack type.
   *
   * \param type the BA type
   */
  void SetType (BlockAckType type);
  /**
   * Set Traffic ID (TID).
   *
   * \param tid the TID
   */
  void SetTidInfo (uint8_t tid);
  /**
   * Set the starting sequence number from the given
   * raw sequence control field.
   *
   * \param seq the raw sequence control
   */
  void SetStartingSequence (uint16_t seq);

  /**
   * Check if the current Ack Policy is immediate.
   *
   * \return true if the current Ack Policy is immediate,
   *         false otherwise
   */
  bool MustSendHtImmediateAck (void) const;
  /**
   * Return the block ack type ID.
   *
   * \return type
   */
  BlockAckType GetType (void) const;
  /**
   * Return the Traffic ID (TID).
   *
   * \return TID
   */
  uint8_t GetTidInfo (void) const;
  /**
   * Return the starting sequence number.
   *
   * \return the starting sequence number
   */
  uint16_t GetStartingSequence (void) const;
  /**
   * Check if the current BA policy is Basic Block Ack.
   *
   * \return true if the current BA policy is Basic Block Ack,
   *         false otherwise
   */
  bool IsBasic (void) const;
  /**
   * Check if the current BA policy is Compressed Block Ack.
   *
   * \return true if the current BA policy is Compressed Block Ack,
   *         false otherwise
   */
  bool IsCompressed (void) const;
  /**
   * Check if the current BA policy is Extended Compressed Block Ack.
   *
   * \return true if the current BA policy is Extended Compressed Block Ack,
   *         false otherwise
   */
  bool IsExtendedCompressed (void) const;
  /**
   * Check if the current BA policy is Multi-TID Block Ack.
   *
   * \return true if the current BA policy is Multi-TID Block Ack,
   *         false otherwise
   */
  bool IsMultiTid (void) const;
  /**
   * Check if the current BA policy is EDMG compressed block ACK.
   *
   * \return true if the current BA policy is EDMG compressed block ACK,
   *         false otherwise
   */
  bool IsEdmgCompressed (void) const;

  /**
   * Set the bitmap that the packet with the given sequence
   * number was received.
   *
   * \param seq the sequence number
   */
  void SetReceivedPacket (uint16_t seq);
  /**
   * Set the bitmap that the packet with the given sequence
   * number and fragment number was received.
   *
   * \param seq the sequence number
   * \param frag the fragment number
   */
  void SetReceivedFragment (uint16_t seq, uint8_t frag);
  /**
   * Check if the packet with the given sequence number
   * was acknowledged in this BlockAck response.
   *
   * \param seq the sequence number
   * \return true if the packet with the given sequence number
   *         was ACKed in this BlockAck response, false otherwise
   */
  bool IsPacketReceived (uint16_t seq) const;
  /**
   * Check if the packet with the given sequence number
   * and fragment number was acknowledged in this BlockAck response.
   *
   * \param seq the sequence number
   * \param frag the fragment number
   * \return true if the packet with the given sequence number
   *         and sequence number was acknowledged in this BlockAck response,
   *         false otherwise
   */
  bool IsFragmentReceived (uint16_t seq, uint8_t frag) const;

  /**
   * Return the starting sequence control.
   *
   * \return the starting sequence control
   */
  uint16_t GetStartingSequenceControl (void) const;
  /**
   * Set the starting sequence control with the given
   * sequence control value
   *
   * \param seqControl the raw sequence control value
   */
  void SetStartingSequenceControl (uint16_t seqControl);
  /**
   * Return the bitmap from the BlockAck response header.
   *
   * \param size
   */
  void SetCompresssedBlockAckSize (EDMG_COMPRESSED_BLOCK_ACK_BITMAP_SIZE size);
  /**
   * Set receive buffer capability.
   *
   * \param capability
   */
  void SetReceiveBufferCapability (uint8_t capability);
  /**
   * Get receive buffer capability.
   *
   * \return capability
   */
  uint8_t GetReceiveBufferCapability (void) const;
  /**
   * Return the bitmap from the BlockAck response header.
   *
   * \return the bitmap from the BlockAck response header
   */
  const uint16_t* GetBitmap (void) const;
  /**
   * Return the compressed bitmap from the BlockAck response header.
   *
   * \return the compressed bitmap from the BlockAck response header
   */
  uint64_t GetCompressedBitmap (void) const;
  /**
   * Return the extended compressed bitmap from the BlockAck response header.
   *
   * \return the extended compressed bitmap from the BlockAck response header
   */
  const uint64_t* GetExtendedCompressedBitmap (void) const;
  /**
   * Return the EDMG compressed bitmap from the block ACK response header.
   *
   * \return the EDMG compressed bitmap from the block ACK response header
   */
  const uint64_t* GetEdmgCompressedBitmap (void) const;

  /**
   * Reset the bitmap to 0.
   */
  void ResetBitmap (void);


private:
  /**
   * Return the Block Ack control.
   *
   * \return the Block Ack control
   */
  uint16_t GetBaControl (void) const;
  /**
   * Set the Block Ack control.
   *
   * \param ba the BA control to set
   */
  void SetBaControl (uint16_t ba);

  /**
   * Serialize bitmap to the given buffer.
   *
   * \param start the iterator
   * \return Buffer::Iterator to the next available buffer
   */
  Buffer::Iterator SerializeBitmap (Buffer::Iterator start) const;
  /**
   * Deserialize bitmap from the given buffer.
   *
   * \param start the iterator
   * \return Buffer::Iterator to the next available buffer
   */
  Buffer::Iterator DeserializeBitmap (Buffer::Iterator start);

  /**
   * This function is used to correctly index in both bitmap
   * and compressed bitmap, one bit or one block of 16 bits respectively.
   *
   * for more details see 7.2.1.8 in IEEE 802.11n/D4.00
   *
   * \param seq the sequence number
   *
   * \return If we are using basic block ack, return value represents index of
   * block of 16 bits for packet having sequence number equals to <i>seq</i>.
   * If we are using compressed block ack, return value represents bit
   * to set to 1 in the compressed bitmap to indicate that packet having
   * sequence number equals to <i>seq</i> was correctly received.
   */
  uint16_t IndexInBitmap (uint16_t seq) const;

  /**
   * Checks if sequence number <i>seq</i> can be acknowledged in the bitmap.
   *
   * \param seq the sequence number
   *
   * \return true if the sequence number is concerned by the bitmap
   */
  bool IsInBitmap (uint16_t seq) const;

  /**
   * The LSB bit of the BA control field is used only for the
   * HT (High Throughput) delayed block ack configuration.
   * For now only non HT immediate block ack is implemented so this field
   * is here only for a future implementation of HT delayed variant.
   */
  bool m_baAckPolicy;     ///< BA Ack Policy
  BlockAckType m_baType;  ///< BA type
  uint16_t m_tidInfo;     ///< TID info
  uint16_t m_startingSeq; ///< starting sequence number

  union
  {
    uint16_t m_bitmap[64]; ///< the basic BlockAck bitmap
    uint64_t m_compressedBitmap; ///< the compressed BlockAck bitmap
    uint64_t m_extendedCompressedBitmap[4]; ///< the extended compressed BlockAck bitmap
    //// WIGIG ////
    uint64_t m_edmgCompressedBitmap[16]; ///< the EDMG compressed block ack bitmap
    //// WIGIG ////
  } bitmap; ///< bitmap union type

  //// WIGIG ////
  EDMG_COMPRESSED_BLOCK_ACK_BITMAP_SIZE m_edmgCompressedBlockAckSize;
  uint8_t m_rbufcapValue; ///< Receive buffer capacity.
  //// WIGIG ////
};


/*************************
 *  Poll Frame (8.3.1.11)
 *************************/

/**
 * \ingroup wifi
 * \brief Header for Poll Frame.
 */
class CtrlDmgPoll : public Header
{
public:
  CtrlDmgPoll ();
  ~CtrlDmgPoll ();
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /**
   * Set the offset in units of 1 microseconds.
   *
   * \param value The offset in units of 1 microseconds.
   */
   void SetResponseOffset (uint16_t value);

  /**
   * Return the offset in units of 1 microseconds.
   *
   * \return the offset in units of 1 microseconds.
   */
  uint16_t GetResponseOffset (void) const;

private:
  uint16_t m_responseOffset;

};

/***********************************************
 * Service Period Request (SPR) Frame (8.3.1.12)
 ***********************************************/

/**
 * \ingroup wifi
 * \brief Header for Service Period Request (SPR) Frame.
 */
class CtrlDMG_SPR : public Header
{
public:
  CtrlDMG_SPR ();
  ~CtrlDMG_SPR ();
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /**
   * Set the Dynamic Allocation Information Field.
   *
   * \param value The Dynamic Allocation Information Field.
   */
  void SetDynamicAllocationInfo (DynamicAllocationInfoField field);
  /**
   * Set the offset in units of 1 microseconds.
   *
   * \param value The offset in units of 1 microseconds.
   */
  void SetBFControl (BF_Control_Field value);

  /**
   * Return the offset in units of 1 microseconds.
   *
   * \return the offset in units of 1 microseconds.
   */
  DynamicAllocationInfoField GetDynamicAllocationInfo (void) const;
  /**
   * Return the offset in units of 1 microseconds.
   *
   * \return the offset in units of 1 microseconds.
   */
  BF_Control_Field GetBFControl (void) const;

private:
  DynamicAllocationInfoField m_dynamic;
  BF_Control_Field m_bfControl;

};

/*************************
 * Grant Frame (8.3.1.13)
 *************************/

/**
 * \ingroup wifi
 * \brief Header for Grant Frame.
 */
class CtrlDMG_Grant : public CtrlDMG_SPR
{
public:
  CtrlDMG_Grant ();
  ~CtrlDMG_Grant ();
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

};

/********************************************
 * DMG Denial to Send (DTS) Frame (8.3.1.15)
 ********************************************/

/**
 * \ingroup wifi
 * \brief Header for Denial to Send (DTS) Frame.
 */
class CtrlDMG_DTS: public Header
{
public:
  CtrlDMG_DTS ();
  ~CtrlDMG_DTS ();
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /**
   * Set the Dynamic Allocation Information Field.
   *
   * \param value The Dynamic Allocation Information Field.
   */
  void SetNAV_SA (Mac48Address value);
  /**
   * Set the offset in units of 1 microseconds.
   *
   * \param value The offset in units of 1 microseconds.
   */
  void SetNAV_DA (Mac48Address value);

  /**
   * Return the offset in units of 1 microseconds.
   *
   * \return the offset in units of 1 microseconds.
   */
  Mac48Address GetNAV_SA (void) const;
  /**
   * Return the offset in units of 1 microseconds.
   *
   * \return the offset in units of 1 microseconds.
   */
  Mac48Address GetNAV_DA (void) const;

private:
    Mac48Address m_navSA;
    Mac48Address m_navDA;

};

/****************************************
 *  Sector Sweep (SSW) Frame (8.3.1.16)
 ****************************************/

/**
 * \ingroup wifi
 * \brief Header for Sector Sweep (SSW) Frame.
 */
class CtrlDMG_SSW : public Header
{
public:
  CtrlDMG_SSW ();
  ~CtrlDMG_SSW ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  void SetSswField (DMG_SSW_Field &field);
  void SetSswFeedbackField (DMG_SSW_FBCK_Field &field);
  DMG_SSW_Field GetSswField (void) const;
  DMG_SSW_FBCK_Field GetSswFeedbackField (void) const;

private:
  DMG_SSW_Field m_ssw;
  DMG_SSW_FBCK_Field m_sswFeedback;

};

/*********************************************************
 *  Sector Sweep Feedback (SSW-Feedback) Frame (8.3.1.17)
 *********************************************************/

/**
 * \ingroup wifi
 * \brief Header for Sector Sweep Feedback (SSW-Feedback) Frame.
 */
class CtrlDMG_SSW_FBCK : public Header
{
public:
  CtrlDMG_SSW_FBCK ();
  virtual ~CtrlDMG_SSW_FBCK ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  void SetSswFeedbackField (DMG_SSW_FBCK_Field &field);
  void SetBrpRequestField (BRP_Request_Field &field);
  void SetBfLinkMaintenanceField (BF_Link_Maintenance_Field &field);

  DMG_SSW_FBCK_Field GetSswFeedbackField (void) const;
  BRP_Request_Field GetBrpRequestField (void) const;
  BF_Link_Maintenance_Field GetBfLinkMaintenanceField (void) const;

private:
  DMG_SSW_FBCK_Field m_sswFeedback;
  BRP_Request_Field m_brpRequest;
  BF_Link_Maintenance_Field m_linkMaintenance;

};

/**********************************************
 * Sector Sweep ACK (SSW-ACK) Frame (8.3.1.18)
 **********************************************/

/**
 * \ingroup wifi
 * \brief Header for Sector Sweep ACK (SSW-ACK) Frame.
 */
class CtrlDMG_SSW_ACK : public CtrlDMG_SSW_FBCK
{
public:
  CtrlDMG_SSW_ACK (void);
  virtual ~CtrlDMG_SSW_ACK (void);
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

};

/******************************
 *  Grant ACK Frame (8.3.1.19)
 ******************************/

/**
 * \ingroup wifi
 * \brief Header for Grant ACK Frame.
 * The Grant ACK frame is sent only in CBAPs as a response to the reception of a Grant frame
 * that has the Beamforming Training field equal to 1.
 */
class CtrlGrantAck : public Header
{
public:
  CtrlGrantAck ();
  ~CtrlGrantAck ();
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

private:
  uint8_t m_reserved[5];
  BF_Control_Field m_bfControl;

};

/*********************************************
 *  TDD Beamforming frame format (9.3.1.24.1)
 *********************************************/

enum TDD_Beamforming_Frame_Type {
  TDD_SSW = 0,
  TDD_SSW_Feedback = 1,
  TDD_SSW_ACK = 2,
};

enum TDD_BEAMFORMING_PROCEDURE {
  TDD_BEAMFORMING_INDIVIDUAL = 0,
  TDD_BEAMFORMING_GROUP = 1,
  TDD_BEAMFORMING_MEASUREMENT = 2,
};

/**
 * \ingroup wifi
 * \brief Header for TDD Beamforming Frame.
 */
class TDD_Beamforming : public Header
{
public:
  TDD_Beamforming ();
  ~TDD_Beamforming ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /** TDD Beamforming Control field **/
  void SetGroup_Beamforming (bool value);
  void SetBeam_Measurement (bool value);
  void SetBeamformingFrameType (TDD_Beamforming_Frame_Type type);
  /**
   * The End of Training subfield is set as follows:
   *  A. The End of Training subfield is set to 1 in a TDD SSW frame to indicate that the initiator intends to
   *  end the TDD individual beamforming training or the TDD beam measurement after the transmission
   *  of the remaining TDD SSW frames with the current Sector ID; this subfield is set to 0 otherwise.
   *  B. The End of Training subfield is set to 1 in a TDD SSW Feedback frame sent as part of a TDD
   *  individual beamforming training if the TDD SSW Feedback frame is sent in response to a TDD SSW
   *  frame in which its End of Training subfield was set to 1; this subfield is set to 0 otherwise.
   *  C. The End of Training subfield is set to 1 in a TDD SSW Ack frame to indicate that the TDD individual
   *  beamforming training has completed; otherwise, this subfield is set to 0.
   * For TDD group BF, the End of Training subfield is reserved.
   * \param value
   */
  void SetEndOfTraining (bool value);

  bool GetGroup_Beamforming (void) const;
  bool GetBeam_Measurement (void) const;
  TDD_Beamforming_Frame_Type GetBeamformingFrameType (void) const;
  bool GetEndOfTraining (void) const;

  TDD_BEAMFORMING_PROCEDURE GetBeamformingProcedure (Mac48Address receiver) const;

protected:
  bool m_groupBeamforming;
  bool m_beamMeasurement;
  TDD_Beamforming_Frame_Type m_beamformingFrameType;
  bool m_endOfTraining;

};

/*********************************************
 * TDD Sector Sweep (SSW) format (9.3.1.24.2)
 *********************************************/

/**
 * \ingroup wifi
 * \brief Header for TDD Beamforming Frame of a TDD SSW frame.
 */
class TDD_Beamforming_SSW : public TDD_Beamforming
{
public:
  TDD_Beamforming_SSW ();
  ~TDD_Beamforming_SSW ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /* TDD Beamforming Information field format (TDD individual BF) */
  void SetTxSectorID (uint16_t sectorID);
  void SetTxAntennaID (uint8_t antennaID);
  void SetCountIndex (uint8_t index);
  void SetBeamformingTimeUnit (uint8_t unit);
  void SetTransmitPeriod (uint8_t period);
  void SetResponderFeedbackOffset (uint16_t offset);
  void SetInitiatorAckOffset (uint16_t offset);
  void SetNumberOfRequestedFeedback (uint8_t feedback);

  uint16_t GetTxSectorID (void) const;
  uint8_t GetTxAntennaID (void) const;
  uint8_t GetCountIndex (void) const;
  uint8_t GetBeamformingTimeUnit (void) const;
  uint8_t GetTransmitPeriod (void) const;
  uint16_t GetResponderFeedbackOffset (void) const;
  uint16_t GetInitiatorAckOffset (void) const;
  uint8_t GetNumberOfRequestedFeedback (void) const;

  /* TDD Beamforming Information field format (TDD beam measurement) */
  void SetTDDSlotCDOWN (uint16_t cdown);
  void SetFeedbackRequested (bool feedback);

  uint16_t GetTDDSlotCDOWN (void) const;
  bool GetFeedbackRequested (void) const;

private:
  /* TDD Beamforming Information field format (TDD individual BF) */
  uint16_t m_sectorID;
  uint8_t m_antennaID;
  uint8_t m_countIndex;
  uint8_t m_beamformingTimeUnit;
  uint8_t m_transmitPeriod;
  uint16_t m_responderFeedbackOffset;
  uint16_t m_initiatorAckOffset;
  uint8_t m_numRequestedFeedback;

  /* TDD Beamforming Information field format (TDD beam measurement) */
  uint16_t m_tddSlotCDOWN;
  bool m_feedbackRequested;

};

/*********************************************
 *     TDD SSW Feedback format (9.3.1.24.3)
 *********************************************/

/**
 * \ingroup wifi
 * \brief Header for TDD Beamforming Frame.
 */
class TDD_Beamforming_SSW_FEEDBACK : public TDD_Beamforming
{
public:
  TDD_Beamforming_SSW_FEEDBACK ();
  ~TDD_Beamforming_SSW_FEEDBACK ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /**
   * The TX Sector ID subfield is set to indicate the sector through which the TDD SSW Feedback frame is transmitted.
   * \param sectorID
   */
  void SetTxSectorID (uint16_t sectorID);
  /**
   * The TX Antenna ID subfield indicates the DMG antenna ID through which the TDD SSW Feedback frame is transmitted.
   * \param antennaID
   */
  void SetTxAntennaID (uint8_t antennaID);
  /**
   * The Decoded TX Sector ID subfield contains the value of the TX Sector ID subfield from the TDD SSW
   * frame that the feedback frame is sent in response to and that the TDD SSW frame was received from the
   * initiator with the best quality.
   * \param sectorID
   */
  void SetDecodedTxSectorID (uint16_t sectorID);
  /**
   * The Decoded TX Antenna ID subfield contains the value of the TX Antenna ID subfield from the TDD SSW
   * frame that the feedback frame is sent in response to and that was received with the best quality.
   * \param antennaID
   */
  void SetDecodedTxAntennaID (uint8_t antennaID);
  /**
   * The SNR Report subfield is set to the value of the SNR achieved while decoding the TDD SSW frame
   * received with the best quality and which is indicated in the Decoded TX Sector ID subfield. The value of the
   * SNR Report subfield is an unsigned integer referenced to a level of –8 dB. Each step is 0.25 dB. SNR values
   * less than or equal to –8 dB are represented as 0. SNR values greater than or equal to 55.75 dB are represented
   * as 0xFF.
   * \param snr
   */
  void SetSnrReport (uint8_t snr);
  /**
   * The Feedback Count Index subfield is counter indicating the index of the TDD SSW Feedback frame
   * transmission during a TDD slot. Value 0 is used in the first transmitted TDD SSW Feedback frame and this
   * subfield value is increased by 1 for each subsequent transmitted frame.
   * \param index
   */
  void SetFeedbackCountIndex (uint8_t index);

  uint16_t GetTxSectorID (void) const;
  uint8_t GetTxAntennaID (void) const;
  uint16_t GetDecodedTxSectorID (void) const;
  uint8_t GetDecodedTxAntennaID (void) const;
  uint8_t GetSnrReport (void) const;
  uint8_t GetFeedbackCountIndex (void) const;

private:
  uint16_t m_sectorID;
  uint8_t m_antennaID;
  uint16_t m_decodedSectorID;
  uint8_t m_decodedAntennaID;
  uint8_t m_snrReport;
  uint8_t m_feedbackCountIndex;

};

/*********************************************
 *     TDD SSW ACK format (9.3.1.24.4)
 *********************************************/

/**
 * \ingroup wifi
 * \brief Header for TDD Beamforming SSW ACK Frame.
 */
class TDD_Beamforming_SSW_ACK : public TDD_Beamforming
{
public:
  TDD_Beamforming_SSW_ACK ();
  ~TDD_Beamforming_SSW_ACK ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /**
   * The Decoded TX Sector ID subfield contains the value of the TX Sector ID subfield from the TDD SSW
   * Feedback frame that was received from the responder.
   * \param sectorID
   */
  void SetDecodedTxSectorID (uint16_t sectorID);
  /**
   * The Decoded TX Antenna ID subfield contains the value of the TX Antenna ID subfield from the TDD SSW
   * Feedback frame that was received from the responder.
   * \param antennaID
   */
  void SetDecodedTxAntennaID (uint8_t antennaID);
  /**
   * The Count Index subfield indicates the index of the TDD Beamforming frame transmitted by the initiator
   * within a TDD slot, with the subfield set to 0 for the first frame transmission and increased by one for each
   * successive frame transmission within a TDD slot.
   * \param index
   */
  void SetCountIndex (uint8_t index);
  /**
   * The Transmit Period subfield indicates the interval, in units of BTUs, between successive TDD SSW
   * transmissions with the same Count Index subfield value in different TDD slots.
   * \param period
   */
  void SetTransmitPeriod (uint8_t period);
  /**
   * The SNR Report subfield is set to the value of the SNR achieved while decoding the TDD SSW Feedback
   * frame. The value of the SNR Report subfield is an unsigned integer referenced to a level of –8 dB. Each step
   * is 0.25 dB. SNR values less than or equal to –8 dB are represented as 0. SNR values greater than or equal to
   * 55.75 dB are represented as 0xFF.
   * \param snr
   */
  void SetSnrReport (uint8_t snr);
  /**
   * The Initiator Transmit Offset subfield indicates the offset, in units of BTUs, beginning immediately after the
   * end of the TDD SSW Ack frame, to the TDD slot in which the initiator is expected to transmit an additional
   * frame (e.g., an Announce frame) to the responder. When the Initiator Transmit Offset subfield is set to 0, no
   * time offset indication is specified by the initiator.
   * \param offset
   */
  void SetInitiatorTransmitOffset (uint16_t offset);
  /**
   * The Responder Transmit Offset subfield indicates the offset, in units of BTUs, beginning immediately after
   * the TDD SSW Ack frame, to the TDD slot in which the responder is expected to respond to frames sent by
   * the initiator. When the Responder Transmit Offset subfield is set to 0, no time offset indication is specified
   * by the initiator.
   * \param offset
   */
  void SetResponderTransmitOffset (uint8_t offset);
  /**
   * The Ack Count Index subfield indicates the number of the TDD SSW Ack frames that have been sent before
   * the current TDD SSW Ack frame within the same TDD slot. The Ack Count Index subfield is set to 0 if no
   * TDD SSW Ack frame is transmitted before the current TDD SSW Ack frame in the same TDD slot, and
   * increases by one for each transmission of a TDD SSW Ack frame within the same TDD slot.
   * \param count
   */
  void SetAckCountIndex (uint8_t count);

  uint16_t GetDecodedTxSectorID (void) const;
  uint8_t GetDecodedTxAntennaID (void) const;
  uint8_t GetCountIndex (void) const;
  uint8_t GetTransmitPeriod (void) const;
  uint8_t GetSnrReport (void) const;
  uint16_t GetInitiatorTransmitOffGet (void) const;
  uint8_t GetResponderTransmitOffGet (void) const;
  uint8_t GetAckCountIndex (void) const;

private:
  uint16_t m_sectorID;
  uint8_t m_antennaID;
  uint8_t m_countIndex;
  uint8_t m_transmitPeriod;
  uint8_t m_snrReport;
  uint16_t m_InitiatorTransmitOffset;
  uint8_t m_responderTransmitOffset;
  uint8_t m_ackCountIndex;

};

} // namespace ns3

#endif /* CTRL_HEADERS_H */
