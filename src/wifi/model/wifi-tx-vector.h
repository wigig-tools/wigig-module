/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 CTTC
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
 * Authors: Nicola Baldo <nbaldo@cttc.es>
 *          Ghada Badawy <gbadawy@gmail.com>
 *          Hany Assasa <hany.assasa@gmail.com>
 */

#ifndef WIFI_TX_VECTOR_H
#define WIFI_TX_VECTOR_H

#include "ns3/mac48-address.h"
#include "ns3/nstime.h"
#include "wifi-mode.h"
#include "wifi-preamble.h"
#include "wifi-phy-standard.h"
#include "wigig-data-types.h"

namespace ns3 {

/**
 * The EDMG SC Guard Interval Length.
 */
enum GuardIntervalLength {
  GI_SHORT  = 0,
  GI_NORMAL = 1,
  GI_LONG   = 2,
};

/**
 * EDMG TRN Sequence Length.
 */
enum TRN_SEQ_LENGTH {
  TRN_SEQ_LENGTH_NORMAL = 0,
  TRN_SEQ_LENGTH_LONG   = 1,
  TRN_SEQ_LENGTH_SHORT  = 2,
};

enum rxPattern {
  QUASI_OMNI,
  DIRECTIONAL
};

/**
 * This class mimics the TXVECTOR which is to be
 * passed to the PHY in order to define the parameters which are to be
 * used for a transmission. See IEEE 802.11-2016 16.2.5 "Transmit PHY",
 * and also 8.3.4.1 "PHY SAP peer-to-peer service primitive
 * parameters".
 *
 * If this class is constructed with the constructor that takes no
 * arguments, then the client must explicitly set the mode and
 * transmit power level parameters before using them.  Default
 * member initializers are provided for the other parameters, to
 * conform to a non-MIMO/long guard configuration, although these
 * may also be explicitly set after object construction.
 *
 * When used in a infrastructure context, WifiTxVector values should be
 * drawn from WifiRemoteStationManager parameters since rate adaptation
 * is responsible for picking the mode, number of streams, etc., but in
 * the case in which there is no such manager (e.g. mesh), the client
 * still needs to initialize at least the mode and transmit power level
 * appropriately.
 *
 * \note the above reference is valid for the DSSS PHY only (clause
 * 16). TXVECTOR is defined also for the other PHYs, however they
 * don't include the TXPWRLVL explicitly in the TXVECTOR. This is
 * somewhat strange, since all PHYs actually have a
 * PMD_TXPWRLVL.request primitive. We decide to include the power
 * level in WifiTxVector for all PHYs, since it serves better our
 * purposes, and furthermore it seems close to the way real devices
 * work (e.g., madwifi).
 */
class WifiTxVector
{
public:
  WifiTxVector ();
  /**
   * Create a TXVECTOR with the given parameters.
   *
   * \param mode WifiMode
   * \param powerLevel transmission power level
   * \param preamble preamble type
   * \param channelWidth the channel width in MHz
   * \param aggregation enable or disable MPDU aggregation
   */
  WifiTxVector (WifiMode mode,
                uint8_t powerLevel,
                WifiPreamble preamble,
                uint16_t channelWidth,
                bool aggregation);
  /**
   * Create a TXVECTOR with the given parameters.
   *
   * \param mode WifiMode
   * \param powerLevel transmission power level
   * \param preamble preamble type
   * \param guardInterval the guard interval duration in nanoseconds
   * \param nTx the number of TX antennas
   * \param nss the number of spatial STBC streams (NSS)
   * \param ness the number of extension spatial streams (NESS)
   * \param channelWidth the channel width in MHz
   * \param aggregation enable or disable MPDU aggregation
   * \param stbc enable or disable STBC
   * \param bssColor the BSS color
   */
  WifiTxVector (WifiMode mode,
                uint8_t powerLevel,
                WifiPreamble preamble,
                uint16_t guardInterval,
                uint8_t nTx,
                uint8_t nss,
                uint8_t ness,
                uint16_t channelWidth,
                bool aggregation,
                bool stbc,
                uint8_t bssColor = 0);
  /**
   * \returns whether mode has been initialized
   */
  bool GetModeInitialized (void) const;
  /**
   * \returns the selected payload transmission mode
   */
  WifiMode GetMode (void) const;
  /**
  * Sets the selected payload transmission mode
  *
  * \param mode the payload WifiMode
  */
  void SetMode (WifiMode mode);
  /**
   * \returns the transmission power level
   */
  uint8_t GetTxPowerLevel (void) const;
  /**
   * Sets the selected transmission power level
   *
   * \param powerlevel the transmission power level
   */
  void SetTxPowerLevel (uint8_t powerlevel);
  /**
   * \returns the preamble type
   */
  WifiPreamble GetPreambleType (void) const;
  /**
   * Sets the preamble type
   *
   * \param preamble the preamble type
   */
  void SetPreambleType (WifiPreamble preamble);
  /**
   * \returns the channel width (in MHz)
   */
  uint16_t GetChannelWidth (void) const;
  /**
   * Sets the selected channelWidth (in MHz)
   *
   * \param channelWidth the channel width (in MHz)
   */
  void SetChannelWidth (uint16_t channelWidth);
  /**
   * \returns the guard interval duration (in nanoseconds)
   */
  uint16_t GetGuardInterval (void) const;
  /**
  * Sets the guard interval duration (in nanoseconds)
  *
  * \param guardInterval the guard interval duration (in nanoseconds)
  */
  void SetGuardInterval (uint16_t guardInterval);
  /**
   * \returns the number of TX antennas
   */
  uint8_t GetNTx (void) const;
  /**
   * Sets the number of TX antennas
   *
   * \param nTx the number of TX antennas
   */
  void SetNTx (uint8_t nTx);
  /**
   * \returns the number of spatial streams
   */
  uint8_t GetNss (void) const;
  /**
   * Sets the number of Nss refer to IEEE 802.11n Table 20-28 for explanation and range
   *
   * \param nss the number of spatial streams
   */
  void SetNss (uint8_t nss);
  /**
   * \returns the number of extended spatial streams
   */
  uint8_t GetNess (void) const;
  /**
   * Sets the Ness number refer to IEEE 802.11n Table 20-6 for explanation
   *
   * \param ness the number of extended spatial streams
   */
  void SetNess (uint8_t ness);
  /**
   * Checks whether the PSDU contains A-MPDU.
   *  \returns true if this PSDU has A-MPDU aggregation,
   *           false otherwise.
   */
  bool IsAggregation (void) const;
  /**
   * Sets if PSDU contains A-MPDU.
   *
   * \param aggregation whether the PSDU contains A-MPDU or not.
   */
  void SetAggregation (bool aggregation);
  /**
   * Check if STBC is used or not
   *
   * \returns true if STBC is used,
   *           false otherwise
   */
  bool IsStbc (void) const;
  /**
   * Sets if STBC is being used
   *
   * \param stbc enable or disable STBC
   */
  void SetStbc (bool stbc);
  /**
   * Set the BSS color
   * \param color the BSS color
   */
  void SetBssColor (uint8_t color);
  /**
   * Get the BSS color
   * \return the BSS color
   */
  uint8_t GetBssColor (void) const;
  /**
   * The standard disallows certain combinations of WifiMode, number of
   * spatial streams, and channel widths.  This method can be used to
   * check whether this WifiTxVector contains an invalid combination.
   *
   * \return true if the WifiTxVector parameters are allowed by the standard
   */
  bool IsValid (void) const;

  //// WIGIG ////
  /** IEEE 802.11ad DMG Tx Vector **/

  /**
   * Set BRP Packet Type.
   * \param type The type of BRP packet.
   */
  void SetPacketType (PacketType type);
  /**
   * Get BRP Packet Type.
   * \return The type of BRP packet.
   */
  PacketType GetPacketType (void) const;
  /**
   * Set the length of te training field.
   * \param length The length of the training field.
   */
  void SetTrainngFieldLength (uint8_t length);
  /**
   * Get the length of te training field.
   * \return The length of te training field.
   */
  uint8_t GetTrainngFieldLength (void) const;
  /**
   * Request Beam Tracking.
   */
  void RequestBeamTracking (void);
  /**
   * \return True if Beam Tracking requested, otherwise false.
   */
  bool IsBeamTrackingRequested (void) const;
  /**
   * In the TXVECTOR, LAST_RSSI indicates the received power level of
   * the last packet with a valid PHY header that was received a SIFS period
   * before transmission of the current packet; otherwise, it is 0.
   *
   * In the RXVECTOR, LAST_RSSI indicates the value of the LAST_RSSI
   * field from the PCLP header of the received packet. Valid values are
   * integers in the range of 0 to 15:
   * — Values of 2 to 14 represent power levels (–71+value×2) dBm.
   * — A value of 15 represents power greater than or equal to –42 dBm.
   * — A value of 1 represents power less than or equal to –68 dBm.
   * — A value of 0 indicates that the previous packet was not received a
   * SIFS period before the current transmission.
   *
   * \param length The length of the training field.
   */
  void SetLastRssi (uint8_t level);
  /**
   * Get the level of Last RSSI.
   * \return The level of Last RSSI.
   */
  uint8_t GetLastRssi (void) const;

public:
  uint8_t remainingTrnUnits;
  uint8_t remainingTrnSubfields;
  uint8_t remainingTSubfields;
  uint8_t remainingPSubfields;
  uint8_t repeatSameAWVSubfield;
  uint8_t repeatSameAWVUnit;
  Time edmgTrnSubfieldDuration;

  /** IEEE 802.11ay EDMG Tx Vector **/

  /**
   * Indicates the number of space-time streams. Value is an integer in the
   * range 1 to 8 for an SU PPDU. For an MU PPDU, values are integers in the
   * range 1 to 2 per user in the TXVECTOR, and 0 to 2 per user in the RXVECTOR.
   * The sum of NUM_STS over all users is in the range of 1 to 8.
   * \param num
   */
  void Set_NUM_STS (uint8_t num);
  uint8_t Get_NUM_STS (void) const;
  /**
   * Indicates the number of EDMG CEF Fields transmitted when using SC mode.
   * Depends on the number of space-time streams as specified in 29.12.3.3.
   */
  uint8_t Get_SC_EDMG_CEF (void) const;
  /**
   * Indicates the number of EDMG CEF Fields transmitted when using OFDM mode.
   * Depends on the number of space-time streams as specified in 29.12.3.3.
   */
  uint8_t Get_OFDM_EDMG_CEF (void) const;
  /**
   * Indicates the number of users with nonzero space-time streams.
   * Integer: range 1 to 8 in case of 1 space-time stream per user,
   * range 1 to 4 in case of 2 space-time streams per user.
   * \param num
   */
  void SetNumUsers (uint8_t num);
  uint8_t GetNumUsers (void) const;
  /**
   * Indicates the length of the guard interval.
   * Enumerated Type:
   * - ShortGI
   * - NormalGI
   * - LongGI
   * \param gi
   */
  void SetGaurdIntervalType (GuardIntervalLength gi);
  GuardIntervalLength GetGaurdIntervalType (void) const;
  /**
   * Set the primary 2.16 GHz channel number.
   * \param primaryCh The pimrary 2.16 GHz channel number.
   */
  void SetPrimaryChannelNumber (uint8_t primaryCh);
  /**
   * Get the primary 2.16 channel number.
   * \return The primary 2.16 channel number.
   */
  uint8_t GetPrimaryChannelNumber (void) const;
  /**
   * Set channels on which the PPDU is transmitted and the value of the BW field in the EDMG Header-A.
   * In the RXVECTOR, indicates the value of the BW field in the EDMG Header-A of a received PPDU.
   * This parameter is a bitmap. Valid values for this parameter and the CHANNEL_AGGREGATION parameter
   *  are indicated in Table 28-21, Table 28-22 and Table 28-23.
   * \param bandwdith
   */
  void SetChBandwidth (EDMG_CHANNEL_CONFIG bandwdith);
  /**
   * \return The current EDMG channel bandwidth.
   */
  uint8_t GetChBandwidth (void) const;
  /**
   * Set channel configuration for the current transmission.
   * \param primaryCh The primary 2.16 GHz channel number.
   * \param bw The channels over which the PPDU is transmitted.
   */
  void SetChannelConfiguration (uint8_t primaryCh, uint8_t bw);
  /**
   * \return the current transmit mask corresponding to the current channel configuration.
   */
  EDMG_TRANSMIT_MASK GetTransmitMask (void) const;
  /**
   * Get the Number of contiguous 2.16 GHz channels, NCB = 1 for 2.16 GHz
   * and 2.16+2.16 GHz, NCB = 2 for 4.32 GHz and 4.32+4.32 GHz, N CB = 3 for
   * 6.48 GHz, and NCB = 4 for 8.64 GHz channel
   * \return The number of continous channels.
   */
  uint8_t GetNCB (void) const;
  /**
   * Indicate whether Channel Aggregation is used or not.
   * \param chAggregation
   */
  void SetChannelAggregation (bool chAggregation);
  /**
   * True if Channel Aggregation is used and false otherwise.
   * \return if channel aggregation is used.
   */
  bool GetChannelAggregation (void) const;
  /**
   * Set the number of TX Chains used for transmission of the packet.
   * \param nTxChains
   */
  void SetNumberOfTxChains (uint8_t nTxChains);
  /**
   * Return the number of tx chains used for transmission of the packet.
   * \return  number of active tx chains.
   */
  uint8_t GetNumberOfTxChains (void) const;
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
   * Set the number of EDMG TRN units in the training field.
   * \param length The number of EDMG TRN units.
   */
  void SetEDMGTrainingFieldLength (uint8_t length);
  /**
   * Get the number of EDMG TRN units in the training field.
   * \return The number of EDMG TRN units.
   */
  uint8_t GetEDMGTrainingFieldLength (void) const;
  /**
   * Indicates the number of TRN subfields at the beginning of a TRN-Unit
   * which are transmitted with the same AWV.
   * \param number
   */
  void Set_EDMG_TRN_P (uint8_t number);
  /**
   * Returns the number of TRN subfields at the beginning of a TRN-Unit
   * which are transmitted with the same AWV as defined in 29.9.2.2.3
   * \param number
   */
  uint8_t Get_EDMG_TRN_P (void) const;
  /**
   * If EDMG_PACKET_TYPE is TRN-T-PACKET or TRN-R/T-PACKET, indicates the
   * number of TRN subfields in a TRN-Unit that may be used for transmit
   * training, as defined in 29.9.2.2.
   * The parameter is reserved if TRN-LEN is 0. The parameter is reserved
   * if EDMG_PACKET_TYPE is TRN-R-PACKET.
   * \param number
   */
  void Set_EDMG_TRN_M (uint8_t number);
  /**
   * Returns the number of TRN Subfields used for transmit training as defined in 29.9.2.2.3.
   * \param number
   */
  uint8_t Get_EDMG_TRN_M (void) const;
  /**
   * Indicates the number of consecutive TRN subfields within the EDMG
   * TRN-Unit M of a TRN-Unit which are transmitted using the same AWV.
   * \param number
   */
  void Set_EDMG_TRN_N (uint8_t number);
  /**
   * Returns the number of consecutive TRN subfields within the EDMG-Unit M
   * which are transmitted using the same AWV as defined in 29.9.2.2.3
   * \param number
   */
  uint8_t Get_EDMG_TRN_N (void) const;
  /**
   * Indicates the length of the Golay sequence to be used to transmit the
   * TRN subfields present in the TRN field of the PPDU. Enumerated Type:
   * - Normal: The Golay sequence has a length of 128×NCB.
   * - Long: The Golay sequence has a length of 256× NCB.
   * - Short: The Golay sequence has a length of 64× NCB.
   * NCB represents the integer number of contiguous 2.16 GHz channels over
   * which the TRN subfield is transmitted and 1 ≤ NCB ≤ 4.
   * \param number
   */
  void Set_TRN_SEQ_LEN (TRN_SEQ_LENGTH number);
  /**
   * Returns the length of the Golay sequence to be used to transmit the
   * TRN subfields present in the TRN field of the PPDU.
   * \param number
   */
  TRN_SEQ_LENGTH Get_TRN_SEQ_LEN (void) const;
  /**
   * If EDMG_PACKET_TYPE is TRN-T-PACKET or TRN-R/T-PACKET, returns the number of
   * TRN subfields repeated at the start of the TRN field with the same AWV as the
   * rest of the packet and used as a transional period before the training.
   * Can have the values 1,2 or 4 depending on the value of m_TrnSeqLen so that
   * the duration of transmission of the T subfields remains the same.
   */
  uint8_t Get_TRN_T (void) const;
  /**
   * If EDMG_PACKET_TYPE is TRN-R/T-PACKET, indicates the number of
   * TRN units repeated with the same AWV in order to perform RX training
   * at the responder.
   * \param number
   */
  void Set_RxPerTxUnits (uint8_t number);
  /**
   * Returns the number of TRN units repeated with the same AWV in order
   * to perform RX training at the responder as defined in 29.9.2.2.3.
   */
  uint8_t Get_RxPerTxUnits (void) const;
  /**
   * Set the MAC address of the sender of the packet.
   */
  void SetSender (Mac48Address sender);
  /**
   * Returns the MAC address of the sender of the packet.
  */
  Mac48Address GetSender (void) const;
  /**
   * Whether the transmitted packet is a DMG beacon or not.
   */
  void SetDMGBeacon (bool beacon);
  /**
   * Returns whether the transmitted packet is a DMG beacon or not.
  */
  bool IsDMGBeacon (void) const;
  /**
   * Indicates the receive antenna pattern to be used
   * when measuring TRN-Units present in a received PPDU.
  */
  void SetTrnRxPattern (rxPattern trnRxPattern);
  /**
   * Returns the receive antenna pattern to be used
   * when measuring TRN-Units present in a received PPDU.
  */
  rxPattern GetTrnRxPattern (void) const;
  /**
   * Sets the BRP CDOWN value of the packet in EDMG BRP Packets
   */
  void SetBrpCdown (uint8_t brpCdown);
  /**
   * Returns the BRP CDOWN value in EDMG BRP Packets.
  */
  uint8_t GetBrpCdown (void) const;
  /**
   * Sets the flag for presence of a control trailer in the packet.
   */
  void SetControlTrailerPresent (bool flag);
  /**
   * Returns whether a control trailer is present in the packet.
  */
  bool IsControlTrailerPresent (void) const;
  //// WIGIG ////

private:
  void DoInitialize(void);

  WifiMode m_mode;               /**< The DATARATE parameter in Table 15-4.
                                 It is the value that will be passed
                                 to PMD_RATE.request */
  uint8_t  m_txPowerLevel;       /**< The TXPWR_LEVEL parameter in Table 15-4.
                                 It is the value that will be passed
                                 to PMD_TXPWRLVL.request */
  WifiPreamble m_preamble;       /**< preamble */
  uint16_t m_channelWidth;       /**< channel width in MHz */
  uint16_t m_guardInterval;      /**< guard interval duration in nanoseconds */
  uint8_t  m_nTx;                /**< number of TX antennas */
  uint8_t  m_nss;                /**< number of spatial streams */
  uint8_t  m_ness;               /**< number of spatial streams in beamforming */
  bool     m_aggregation;        /**< Flag whether the PSDU contains A-MPDU. */
  bool     m_stbc;               /**< STBC used or not */
  uint8_t  m_bssColor;           /**< BSS color */

  bool     m_modeInitialized;         /**< Internal initialization flag */
  bool     m_txPowerLevelInitialized; /**< Internal initialization flag */

  //// WIGIG ////
  /** IEEE 802.11ad DMG Tx Vector **/
  PacketType  m_packetType;               //!< BRP-RX, BRP-TX or BRP-RX/TX packet.
  uint8_t     m_traingFieldLength;        //!< The length of the training field (Number of TRN-Units).
  bool        m_beamTrackingRequest;      //!< Flag to indicate the need for beam tracking.
  uint8_t     m_lastRssi;                 //!< Last Received Signal Strength Indicator.

  /** IEEE 802.11ay EDMG Tx Vector **/

  /* EDMG Header-A */
  uint8_t m_numSts;                       //!< The number of space-time streams.
  uint8_t m_numUsers;
  GuardIntervalLength m_guardIntervalType;
  uint8_t m_chBandwidth;
  uint8_t m_primaryChannel;               //!< The primary 2.16 GHz channel.
  uint8_t m_NCB;                          //!< Number of bonded channels.
  EDMG_TRANSMIT_MASK m_mask;              //!< The EDMG transmit mask (Represent the numfer of channels).
  bool m_chAggregation;                   //!< Flag to indicate whether channel aggregation is used or not.
  uint8_t m_nTxChains;                    //!< The number of Tx Chains used for the transmission of the packet.
  bool m_shortLongLDCP;                   //!< Flag to indicate whether the LDCP codewords have a short or long length.
  uint8_t m_edmgTrnLength;                //!< The length of the EDMG training field (Number of EDMG TRN-Units).
  uint8_t m_edmgTrnP;                     //!< Number of TRN Subfields repeated at the start of a unit with the same AWV.
  uint8_t m_edmgTrnM;                     //!< In BRP-TX and BRP-RX/TX packets the number of TRN Subfields that can be used for training.
  uint8_t m_edmgTrnN;                     //!< In BRP-TX packets the number of TRN Subfields in a unit transmitted with the same AWV.
  TRN_SEQ_LENGTH m_TrnSeqLen;             //!< Length of the Golay Sequence used in the TRN Subfields.
  uint8_t m_rxPerTxUnits;                 //!< In BRP-RX/TX packets the number of times a TRN unit is repeated for RX training at the responder.
  rxPattern m_trnRxPattern;               //!< Indicates the receive antenna pattern to be used when measuring TRN-Units present in a received PPDU.

  /* Helper values needed for PHY processing of TRN fields */
  Mac48Address m_sender;                  //!< MAC address of the sender of the packet.
  bool m_isDMGBeacon;                     //!< Flag that specifies whether the transmitted packet is a DMG beacon or not.
  uint8_t m_brpCdown;                     //!< BRP CDOWN value of the packet
  //// WIGIG ////
  bool m_isControlTrailerPresent;           //!< Flag that specifies if a control trailer is added at the end of the packet

};

/**
 * Serialize WifiTxVector to the given ostream.
 *
 * \param os the output stream
 * \param v the WifiTxVector to stringify
 *
 * \return ouput stream
 */
std::ostream & operator << (std::ostream & os,const WifiTxVector &v);

} //namespace ns3

#endif /* WIFI_TX_VECTOR_H */
