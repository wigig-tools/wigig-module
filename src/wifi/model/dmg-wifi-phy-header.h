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

#ifndef DMG_WIFI_PHY_HEADER_H
#define DMG_WIFI_PHY_HEADER_H

#include "ns3/header.h"
#include "wifi-phy-standard.h"
#include "wifi-tx-vector.h"

namespace ns3 {

/**
 * \ingroup wifi
 *
 * Implements the IEEE 802.11ad DMG Control PHY header.
  * See section 20.4.3.2 in IEEE 802.11-2016.
 */
class DmgControlHeader : public Header
{
public:
  DmgControlHeader ();
  virtual ~DmgControlHeader ();

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
   * Set the number of data octets in the PSDU.
   *
   * \param length the number of data octets in the PSDU.
   *        Range 14–1023 (in bytes).
   * \param isShortSSW Flag that specifies if the PSDU is a Short SSW packet.
   *        Short SSW packets are allowed to have a length of 6 bytes.
   */
  void SetLength (uint16_t length, bool isShortSSW = false);
  /**
   * Get the number of data octets in the PSDU.
   *
   * \return the number of data octets in the PSDU.
   */
  uint16_t GetLength (void) const;
  /**
   * — Packet Type = 0 (BRP-RX packet, see 20.10.2.2.3), indicates
   *   either a PPDU whose data part is followed by one or more
   *   TRN subfields (when the Beam Tracking Request field is 0 or
   *   in DMG control mode), or a PPDU that contains a request for
   *   TRN subfields to be appended to a future response PPDU
   *   (when the Beam Tracking Request field is 1).
   * — Packet Type = 1 (BRP-TX packet, see 20.10.2.2.3), indicates a
   *   PPDU whose data part is followed by one or more TRN sub-
   *   fields. The transmitter may change AWV at the beginning of
   *   each TRN subfield.
   * The field is reserved when the Training Length field is 0.
   *
   * \param type
   */
  void SetPacketType (PacketType type);
  /**
   * Get packet type.
   *
   * \return The type of the TRN subfields appended to the packet.
   */
  PacketType GetPacketType (void) const;
  /**
   * Set the length of the training field (The number of TRN units).
   *
   * \param length The length of the training field.
   */
  void SetTrainingLength (uint16_t length);
  /**
   * Get the length of the training field (The number of TRN units).
   *
   * \return the length of the training field.
   */
  uint16_t GetTrainingLength (void) const;
  /**
   * Set the scrambler bits to indicate the presence of a control trailer (true) or EDMG-Header-A (false).
   * Only used when the standard is 802.11ay
   * \param flag Whether control trailer is present in the packet.
   */
  void SetControlTrailerPresent (bool flag);
  /**
   * Whether a control trailer or an EDMG-Header-A is present in the packet based on the scrambler initialization bits.
   * Only used when the standard is 802.11ay
   * \return whether a control trailer is present in the packet.
   */
  bool IsControlTrailerPresent (void) const;


private:
  uint16_t m_length;            //!< The length of the PSDU in bytes.
  PacketType m_packetType;      //!< The type of the TRN subfields.
  uint16_t m_trainingLength;    //!< The number of the TRN units.
  uint8_t m_scramblerInitializationBits; //!< First two bits from the initial scrambler state

};

/**
 * \ingroup wifi
 *
 * Implements the IEEE 802.11ad DMG OFDM PHY header.
  * See section 20.5.3.1 in IEEE 802.11-2016.
 */
class DmgOfdmHeader : public Header
{
public:
  DmgOfdmHeader ();
  virtual ~DmgOfdmHeader ();

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
   * Set modulation and coding scheme based on Table 20-19 in IEEE 802.11-2016
   *
   * \param mcs The modulation and coding scheme.
   */
  void SetBaseMcs (uint8_t mcs);
  /**
   * Get modulation and coding scheme.
   *
   * \return The base modulation and coding scheme.
   */
  uint8_t GetBaseMcs (void) const;
  /**
   * If the Extended SC MCS Indication field is 0, indicates the number
   * of data octets in the PSDU; range 1–262 143.
   *
   * If the Extended SC MCS Indication field is 1, the Length field value
   * is set to Base_Length1 |_(Base_Length2 - N)/4_| , where N
   * is the number of data octets in the PSDU, and Base_Length1 and
   * Base_Length2 are computed according to Table 20-18. The number
   * of data octets in the PSDU shall not exceed 262 143.
   *
   * \param length the number of data octets in the PSDU.
   */
  void SetLength (uint32_t length);
  /**
   * Get the number of data octets in the PSDU.
   *
   * \return the number of data octets in the PSDU.
   */
  uint32_t GetLength (void) const;
  /**
   * — Packet Type = 0 (BRP-RX packet, see 20.10.2.2.3), indicates
   *   either a PPDU whose data part is followed by one or more
   *   TRN subfields (when the Beam Tracking Request field is 0 or
   *   in DMG control mode), or a PPDU that contains a request for
   *   TRN subfields to be appended to a future response PPDU
   *   (when the Beam Tracking Request field is 1).
   * — Packet Type = 1 (BRP-TX packet, see 20.10.2.2.3), indicates a
   *   PPDU whose data part is followed by one or more TRN sub-
   *   fields. The transmitter may change AWV at the beginning of
   *   each TRN subfield.
   * The field is reserved when the Training Length field is 0.
   *
   * \param type
   */
  void SetPacketType (PacketType type);
  /**
   * Get packet type.
   *
   * \return The type of the TRN subfields appended to the packet.
   */
  PacketType GetPacketType (void) const;
  /**
   * Set the length of the training field (The number of TRN units).
   *
   * \param length The length of the training field.
   */
  void SetTrainingLength (uint16_t length);
  /**
   * Get the length of the training field (The number of TRN units).
   *
   * \return the length of the training field.
   */
  uint16_t GetTrainingLength (void) const;
  /**
   * Set to 1 to indicate that the PPDU in the data portion of the packet
   * contains an A-MPDU; otherwise, set to 0.
   *
   * \param aggregation
   */
  void SetAggregation (bool aggregation);
  /**
   * Get aggregation field in DMG SC Header.
   *
   * \return True if the PPDU in the data portion of the packet contains
   * an A-MPDU; otherwise false.
   */
  bool GetAggregation (void) const;
  /**
   * Set to 1 to indicate the need for beam tracking (10.38.7); otherwise,
   * set to 0. The Beam Tracking Request field is reserved when the Training
   * Length field is 0.
   *
   * \param request
   */
  void SetBeamTrackingRequest (bool request);
  /**
   * Get BeamTrackingRequest field in DMG SC Header.
   *
   * \return True if beam tracking is requested; otherwise false.
   */
  bool GetBeamTrackingRequest (void) const;
  /**
   * Contains a copy of the parameter LAST_RSSI from the TXVECTOR.
   * The value is an unsigned integer:
   * - Values 2 to 14 represent power levels (–71 + value×2) dBm.
   * - A value of 15 represents a power greater than or equal to –42 dBm.
   * - A value of 1 represents a power less than or equal to –68 dBm.
   * - A value of 0 indicates that the previous packet was not received a SIFS
   *  before the current transmission.
   *
   * \param rssi
   */
  void SetLastRSSI (uint8_t rssi);
  /**
   * Get LastRSSI field  in DMG SC Header.
   *
   * \return
   */
  uint8_t GetLastRSSI (void) const;

protected:
  uint8_t m_baseMcs;                //!< Modulation and coding scheme.
  uint32_t m_length;                //!< The length of the PSDU in bytes.
  PacketType m_packetType;          //!< The type of the TRN subfields.
  uint16_t m_trainingLength;        //!< The number of the TRN units.
  bool m_aggregation;               //!< Flag to indicate if the PSDU contains A-MPDU.
  bool m_beamTrackingRequest;       //!< Flag to indicate is beam tracking is requested.
  uint8_t m_lastRSSI;               //!< Last RSSI value.

};

/**
 * \ingroup wifi
 *
 * Implements the IEEE 802.11ad DMG SC PHY header.
  * See section 20.6.3.1 in IEEE 802.11-2016.
 */
class DmgScHeader : public DmgOfdmHeader
{
public:
  DmgScHeader ();
  virtual ~DmgScHeader ();

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
   * The Extended SC MCS Indication field combined with the Base MCS field
   * indicates the MCS.
   * The Extended SC MCS Indication field indicates whether the Length field
   * shall be calculated according to Table 20-18.
   *
   * \param extended
   */
  void SetExtendedScMcsIndication (bool extended);
  /**
   * Get Extended SC MCS Indication field in DMG SC Header.
   *
   * \return
   */
  bool GetExtendedScMcsIndication (void) const;

private:
  bool m_extendedScMcsIndication;   //!< Flad to indicate if we are using extended SC MCS values.

};

/**
 * IEEE 802.11ay, the selected channel for MPDU transmssion
 */
enum CH_BANDWIDTH_NUM {
  CH_1 = 1,
  CH_2 = 2,
  CH_3 = 4,
  CH_4 = 8,
  CH_5 = 16,
  CH_6 = 32,
  CH_7 = 64,
  CH_8 = 128
};

/**
 * \ingroup wifi
 *
 * Implements the IEEE 802.11ay EDMG Control PHY Header-A.
 * See section 28.3.3.3.2.2 in IEEE Draft P802.11ay - D5.0.
 */
class EdmgControlHeaderA : public Header
{
public:
  EdmgControlHeaderA ();
  virtual ~EdmgControlHeaderA ();

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
   * A bitmap as indicated by the CH_BANDWIDTH parameter in the TXVECTOR that indicates the 2.16 GHz channel(s)
   * over which the PPDU is transmitted on. If a bit is set to 1, it indicates that the corresponding
   * channel is used for the PPDU transmission; otherwise if the bit is set to 0, the channel is not used.
   * Bit 0 corresponds to channel 1, bit 1 corresponds to channel 2, and so on.
   * Receives a vector of all of the channels that the packet is being transmitted on and creates the bitmap value.
   * \param channels
   */
  void SetBw (uint8_t bw);
  /**
   * Returns all the channels over which the PPDU is transmitted on.
   * \return All the channels over which the PPDU is transmitted on.
   */
  uint8_t GetBw (void) const;
  /**
   * Set the primary 2.16 GHz channel number of the BSS.
   * \param chNumber The primary 2.16 GHz channel number of the BSS minus one.
   * It corresponds to TXVECTOR parameter PRIMARY_CHANNEL.
   */
  void SetPrimaryChannelNumber (uint8_t chNumber);
  /**
   * Get the primary 2.16 GHz channel number of the BSS.
   * \return The primary 2.16 GHz channel number of the BSS minus one.
   */
  uint8_t GetPrimaryChannelNumber (void) const;
  /**
   * Set the number of data octets in the PSDU.
   *
   * \param length the number of data octets in the PSDU.
   *        Range 14–1023 (in bytes).
   */
  void SetLength (uint32_t length, bool isShortSSW = false);
  /**
   * Get the number of data octets in the PSDU.
   *
   * \return the number of data octets in the PSDU.
   */
  uint32_t GetLength (void) const;
  /**
   * Set the number of EDMG TRN units in the training field.
   * \param length The number of EDMG TRN units.
   */
  void SetEdmgTrnLength (uint8_t length);
  /**
   * Get the number of EDMG TRN units in the training field.
   * \return The number of EDMG TRN units.
   */
  uint8_t GetEdmgTrnLength (void) const;
  /**
   * If the packet contains TRN-RX/TX Units, indicates the number of
   * TRN units repeated with the same AWV per each TX TRN Unit in order to perform RX training
   * at the responder.
   * \param number RX TRN Units per each TX TRN Unit
   */
  void SetRxPerTxUnits (uint8_t number);
  /**
   * Returns the number of TRN units repeated with the same AWV in order
   * to perform RX training at the responder as defined in 29.9.2.2.3.
   * \return RX TRN Units per each TX TRN Unit
   */
  uint8_t GetRxPerTxUnits (void) const;
  /**
   * Indicates the number of TRN subfields at the beginning of a TRN-Unit
   * which are transmitted with the same AWV.
   * \param number Edmg TRN Unit P
   */
  void SetEdmgTrnUnitP (uint8_t number);
  /**
   * Returns the number of TRN subfields at the beginning of a TRN-Unit
   * which are transmitted with the same AWV as defined in 29.9.2.2.3
   * \return Edmg TRN Unit P
   */
  uint8_t GetEdmgTrnUnitP (void) const;
  /**
   * In packets with TRN-TX or TRN-RX/TX Units, indicates the
   * number of TRN subfields in a TRN-Unit that may be used for transmit
   * training, as defined in 29.9.2.2.
   * The parameter is reserved if TRN-LEN is 0 or if the packet contains TRN-R Units.
   * \param Edmg TRN Unit M
   */
  void SetEdmgTrnUnitM (uint8_t number);
  /**
   * Returns the number of TRN Subfields used for transmit training as defined in 29.9.2.2.3.
   * \return Edmg TRN Unit M
   */
  uint8_t GetEdmgTrnUnitM (void) const;
  /**
   * Indicates the number of consecutive TRN subfields within the EDMGHeader
   * TRN-Unit M of a TRN-Unit which are transmitted using the same AWV.
   * \param Edmg TRN Unit N
   */
  void SetEdmgTrnUnitN (uint8_t number);
  /**
   * Returns the number of consecutive TRN subfields within the EDMG-Unit M
   * which are transmitted using the same AWV as defined in 29.9.2.2.3
   * \return Edmg TRN Unit N
   */
  uint8_t GetEdmgTrnUnitN (void) const;
  /**
   * Indicates the length of the Golay sequence to be used to transmit the
   * TRN subfields present in the TRN field of the PPDU. Enumerated Type:
   * - Normal: The Golay sequence has a length of 128×NCB.
   * - Long: The Golay sequence has a length of 256× NCB.
   * - Short: The Golay sequence has a length of 64× NCB.
   * NCB represents the integer number of contiguous 2.16 GHz channels over
   * which the TRN subfield is transmitted and 1 ≤ NCB ≤ 4.
   * \param length TRN Sequence Length
   */
  void SetTrnSequenceLength (TRN_SEQ_LENGTH length);
  /**
    * Returns the length of the Golay sequence to be used to transmit the
    * TRN subfields present in the TRN field of the PPDU.
    * \return TRN Sequence Length
    */
  TRN_SEQ_LENGTH GetTrnSequenceLength (void) const;
  /**
    * The value of this field plus 1 indicates the number of transmit chains used in the
    * transmission of the PPDU. The value of the field plus 1 also indicates the
    * total number of orthogonal sequences in a TRN field (see 28.9.2.2.5). This field is
    * reserved when the EDMG TRN Length field is 0, or when the EDMG Beam Tracking Request
    * field is 1 and the packet is an EDMG BRP-RX PPDU.
    * \param number Number of Tx Chains
    */
  void SetNumberOfTxChains (uint8_t number);
  /**
    * Returns the number of consecutive TRN subfields within the EDMG-Unit M
    * which are transmitted using the same AWV as defined in 29.9.2.2.3
    * \return Number Of Tx Chains
    */
  uint8_t GetNumberOfTxChains (void) const;

protected:
  uint8_t m_bw;                   //!< A bitmap that indicates the 2.16 GHz channel(s) over which the PPDU is transmitted on.
  uint8_t m_primaryChannelNumber; //!< The primary 2.16 GHz channel number of the BSS.
  uint32_t m_length;              //!< The length of the PSDU in bytes.
  uint8_t m_edmgTrnLength;        //!< The length of the EDMG training field (Number of EDMG TRN-Units).
  uint8_t m_rxPerTxUnits;         //!< In BRP-RX/TX packets the number of times a TRN unit is repeated for RX training at the responder.
  uint8_t m_edmgTrnUnitP;         //!< Number of TRN Subfields repeated at the start of a unit with the same AWV.
  uint8_t m_edmgTrnUnitM;         //!< In BRP-TX and BRP-RX/TX packets the number of TRN Subfields that can be used for training.
  uint8_t m_edmgTrnUnitN;         //!< In BRP-TX packets the number of TRN Subfields in a unit transmitted with the same AWV.
  TRN_SEQ_LENGTH m_trnSeqLen;     //!< Length of the Golay Sequence used in the TRN Subfields.
  uint8_t m_numberOfTxChains;     //!< Number of transmit chains used in the transmission of the PPDU.

};

/**
 * \ingroup wifi
 *
 * Implements the IEEE 802.11ay EDMG SU PHY Header-A.
 * See section 28.3.3.3.2.3 in IEEE Draft P802.11ay - D5.0.
 */
class EdmgSuHeaderA : public EdmgControlHeaderA
{
public:
  EdmgSuHeaderA ();
  virtual ~EdmgSuHeaderA ();

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
   * Indicates whether the packet is a SU (0) or MU (1) PPDU. Always has to be 1 (MU PPDU) in this header.
   * \param suMuPpdu
   */
  void SetSuMuPpdu (bool suMuPpdu);
  /**
   * Indicates whether the packet is a SU or MU PPDU. Always has to be 1 (MU PPDU) in this header.
   * \return SU or MU PPDU
   */
  bool GetSuMuPpdu (void) const;
  /**
   * Set to 1 to indicate that channel aggregation is used for transmission of the packet.
   * \param chAggregation
   */
  void SetChAggregation (bool chAggregation);
  /**
   *  Whether channel aggregation is used for transmission of the packet.
   * \return Is channel aggregation used.
   */
  bool GetChAggregation (void) const;
  /**
   * Set to 1 to indicate that digital beamforming is applied.
   * \param beamformed
   */
  void SetBeamformed (bool beamformed);
  /**
   * Indicates whether digital beamforming is applied.
   * \return beamformed
   */
  bool GetBeamformed (void) const;
  /**
   * Set the LDCP codeword length. Set to 0 for LDPC
   * codeword of length 672, 624, 504, or 468. Set to 1 for LDPC codeword of
   * length 1344, 1248, 1008, or 936.
   * \param cwlength Short or Long LDCP CW length.
   */
  void SetLdcpCwLength (bool cwLength);
  /**
   * Get the LDCP codeword length. 0 means LDPC
   * codeword of length 672, 624, 504, or 468. 1 means LDPC codeword of
   * length 1344, 1248, 1008, or 936.
   * \return Short or Long LDCP CW length.
   */
  bool GetLdcpCwLength (void) const;
  /**
   * Set to 1, indicates that STBC was applied at the transmitter. Otherwise, set to 0.
   * \param stcb
   */
  void SetStbcApplied (bool stcb);
  /**
   * Indicates whether STBC was applied at the transmitter.
   * \return STCB Applied or not
   */
  bool GetStbcApplied (void) const;
  /**
   * Set the number of data octets in the PSDU.
   *
   * \param length the number of data octets in the PSDU.
   *        Range 1-4194303 (in bytes).
   */
  void SetLength (uint32_t length);
  /**
   * Get the number of data octets in the PSDU.
   *
   * \return the number of data octets in the PSDU.
   */
  uint32_t GetLength (void) const;
  /**
   * The value of this field plus one indicates the number of SSs transmitted in the PPDU.
   * \param number The number of SS (1-8).
   */
  void SetNumberOfSS (uint8_t number);
  /**
   * Get the number of SSs transmitted in the PPDU.
   *
   * \return the number of SSs transmitted in the PPDU.
   */
  uint8_t GetNumberOfSS (void) const;
  /**
   * Set modulation and coding scheme based on Table 20-19 in IEEE 802.11-2016
   *
   * \param mcs The modulation and coding scheme.
   */
  void SetBaseMcs (uint8_t mcs);
  /**
   * Get modulation and coding scheme.
   *
   * \return The base modulation and coding scheme.
   */
  uint8_t GetBaseMcs (void) const;

private:
  bool m_suMuPpdu;            //!< Flag that specifies if the packet is a SU or MU PPDU (has to be 0 always).
  bool m_chAggregation;       //!< Flag that indicates if channel aggregation is used for transmission of the packet.
  bool m_beamformed;          //!< Flag that indicates if digital beamforming is applied.
  bool m_shortLongLDCP;       //!< Flag to indicate whether the LDCP codewords have a short or long length.
  bool m_stbcApplied;         //!< Flag to indicate if STBC is applied or not.
  uint8_t m_numberOfSS;       //!< Number of spatial streams transmitted in the PPDU.
  uint8_t m_baseMcs;          //!< Base modulation and coding Scheme.

};

struct SSDescriptorSet {
  SSDescriptorSet ()
    : AID (0),
      NumberOfSS (0)
  {}
  uint8_t AID;
  uint8_t NumberOfSS;
};

typedef std::vector<SSDescriptorSet> SSDescriptorSetList;
typedef SSDescriptorSetList::iterator SSDescriptorSetListI;
typedef SSDescriptorSetList::const_iterator SSDescriptorSetListCI;

/**
 * \ingroup wifi
 *
 * Implements the IEEE 802.11ay EDMG MU PHY Header-A.
 * See section 28.3.3.3.2.3 in IEEE Draft P802.11ay - D5.0.
 */
class EdmgMuHeaderA : public EdmgControlHeaderA
{
public:
  EdmgMuHeaderA ();
  virtual ~EdmgMuHeaderA ();

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
   * Indicates whether the packet is a SU (0) or MU (1) PPDU. Always has to be 0 (SU PPDU) in this header.
   * \param suMuPpdu
   */
  void SetSuMuPpdu (bool suMuPpdu);
  /**
   * Indicates whether the packet is a SU or MU PPDU. Always has to be 0 (SU PPDU) in this header.
   * \return SU or MU PPDU
   */
  bool GetSuMuPpdu (void) const;
  /**
   * Set to 1 to indicate that channel aggregation is used for transmission of the packet.
   * \param chAggregation
   */
  void SetChAggregation (bool chAggregation);
  /**
   *  Whether channel aggregation is used for transmission of the packet.
   * \return Is channel aggregation used.
   */
  bool GetChAggregation (void) const;
  /**
   *  Adds a SS descriptor set.
   * \param aid the AID of a STA addressed by an MPDU contained within the MU PPDU.
   * \param numberOfSs number of SSs transmitted to the STA indicated by the AID subfield.
   */
  void AddSSDescriptorSet (uint8_t aid, uint8_t numberOfSs);
  /**
   *  Get the list of the SS descriptor sets for all the SS contained in the packet.
   * \return SS Descriptor set list
   */
  SSDescriptorSetList GetSSDescriptorSetList (void) const;

private:
  bool m_suMuPpdu;                            //!< Flag that specifies if the packet is a SU or MU PPDU (has to be 0 always).
  bool m_chAggregation;                       //!< Flag that indicates if channel aggregation is used for transmission of the packet.
  SSDescriptorSetList m_ssDescriptorSetList;  //!< List that contains the SS descriptor Sets for all the SS contained in the packet.
  SSDescriptorSetListI m_currentDescriptorIt; //!< Iterator to the current SS descritpor.

};

} //namespace ns3

#endif /* DMG_WIFI_PHY_HEADER_H */
