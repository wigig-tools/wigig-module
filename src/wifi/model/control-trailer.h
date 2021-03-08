/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2020 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#ifndef CONTROL_TRAILER
#define CONTROL_TRAILER

#include "ns3/header.h"

namespace ns3 {

enum CT_FORMAT_TYPE {
  CT_TYPE_CTS_DTS = 0,
  CT_TYPE_GRANT_RTS_CTS2SELF = 1,
  CT_TYPE_SPR = 2,
  CT_TYPE_SSW_FEEDBACK = 3,
  CT_TYPE_BLOCK = 3,
  CT_TYPE_ACK = 3,
};

/**
 * Structure for reporting measured stream SNR and RSSI values.
 */
struct StreamMeasurement {
  uint8_t SNR;
  uint8_t RSSI;
};

typedef std::vector<StreamMeasurement> StreamMeasurementList;
typedef StreamMeasurementList::iterator StreamMeasurementListI;
typedef StreamMeasurementList::const_iterator StreamMeasurementListCI;

/**
 * \ingroup wifi
 * Implement the header for Control trailer (802.11ay Draft 5.0 Table 77).
 */
class ControlTrailer : public Header
{
public:
  ControlTrailer ();
  ~ControlTrailer ();

  /**
   * Indicates the format of the control trailer. This field takes the value defined
   * in Table 28-35 for CT_TYPE = CTS_DTS.
   * \param type
   */
  void SetControlTrailerFormatType (CT_FORMAT_TYPE type);
  /**
   * Set to 0 if the parameter is equal to NOT_AGGREGATE. Set to 1 if the parameter
   * is equal to AGGREGATE.
   * \param aggregation
   */
  void SetChannelAggregation (bool aggregation);
  /**
   * A bitmap as indicated by the CH_BANDWIDTH parameter in the TXVECTOR that indicates
   * the 2.16 GHz channel(s) over which the PPDU is transmitted on. If a bit is set to 1,
   * it indicates that the corresponding channel is used for the PPDU transmission;
   * otherwise if the bit is set to 0, the channel is not used. Bit 0 corresponds to
   * channel 1, bit 1 corresponds to channel 2, and so on.
   * \param bw
   */
  void SetBW (uint8_t bw);
  /**
   * Contains the 3 LSBs of the primary channel number of the BSS minus one.
   * \param num Primary channel number.
   */
  void SetPrimaryChannelNumber (uint8_t num);
  /**
   * Set to 0 to indicate that the following transmission from this STA is
   * performed with a single antenna. Set to 1 to indicate that the following
   * transmission from this STA is performed with multiple antennas.
   * \param mimo
   */
  void SetAsMimoTransmission (bool mimo);
  /**
   * Set to 0 to indicate SU-MIMO, and set to 1 to indicate MU-MIMO. Reserved
   * when the SISO/MIMO field is set to 0.
   * \param mu True if MU-MIMO, otherwise false.
   */
  void SetAsMuMimoTransmission (bool mu);
  /**
   * This field indicates the group of STAs that will be involved in the following
   * MU-MIMO transmission. Reserved when the SU/MU MIMO field is set to 0.
   * \param id
   */
  void SetEdmgGroupID (uint8_t id);
  /**
   * Indicates the TX sector combination and the corresponding RX AWVs to be used
   * in the following SU-MIMO transmission. Reserved if the SISO/MIMO field is 0
   * or the SU/MU MIMO field is 1.
   * \param idx
   */
  void SetTxSectorCombinationIdx (uint8_t idx);
  /**
   * Set to 0 toindicate that the following transmission from this STA is not hybrid
   * beamforming training. Set to 1 to indicate that the following transmission
   * from this STA is hybrid beamforming training. Reserved when the SISO/MIMO field is 0.
   * \param hbf
   */
  void SetHBF (bool hbf);

  /** Field specific when Control trailer definition when CT_TYPE is GRANT_RTS_CTS2self **/
  /**
   * Set to 1 to indicate that the MU-MIMO transmission configuration was obtained
   * from the reciprocal MU-MIMO BF training; set to 0 to indicate that the MU-MIMO
   * transmission configuration was obtained from the non-reciprocal MU-MIMO BF
   * training. Reserved if the SISO/MIMO field is 0 or the SU/MU MIMO field is 0.
   * \param type
   */
  void SetMuMimoTransmissionConfigType (uint8_t type);
  /**
   * Indicates the MU-MIMO transmission configuration to be used in the following
   * MU-MIMO transmission. Reserved if the SISO/MIMO field is 0 or the SU/MU MIMO
   * field is 0.
   * \param idx
   */
  void SetMuMimoConfigIdx (uint8_t idx);
  /**
   * This field is prepended to the Total Number of Sectors subfield in the BF
   * Control field to form a single 11 bits field indicating the total number of
   * sectors the initiator or the responder uses during an SLS. This field is
   * reserved and set to 0 when the PPDU does not carry a Grant or Grant Ack
   * frame with the Beamforming Training field equal to 1
   * \param msb
   */
  void SetTotalNumberOfSectorsMSB (uint8_t msb);
  /**
   * This field is prepended to the Number of RX DMG Antennas subfield in the
   * BF Control field to form a single 3 bits field indicating the total number of
   * repetitions of the TXSS that the initiator or the responder uses during the
   * SLS. This field is reserved and set to 0 when the PPDU does not carry a
   * Grant or Grant Ack frame with the Beamforming Training field equal to 1.
   * \param msb
   */
  void SetNumberOfRxDMGAntennasMSB (uint8_t msb);

  /** Specifics fields when Control trailer definition when CT_TYPE is SPR **/

  /**
   * Indicates whether the value in the BW subfield represents a channel width or a channel number.
   * \param isChannelNumber
   */
  void SetAsChannelNumber (bool isChannelNumber);
  /**
   * This field indicates the total number of sectors the initiator or the unsolicited RSS
   * responder uses during an SLS. This field is reserved and set to 0 when the PPDU does
   * not carry an SPR frame with the Beamforming Training field equal to 1.
   * \param num
   */
  void SetTotalNumberOfSectors (uint16_t num);
  /**
   * This field indicates the total number of repetitions of the TXSS that the responder
   * uses during the SLS. This field is reserved and set to 0 when the PPDU does not carry
   * an SPR frame with the Beamforming Training field equal to 1.
   * \param num
   */
  void SetNumberOfRXDMGAntennas (uint8_t num);

  /** Specifics fields when Control trailer definition when CT_TYPE is SSW_FEEDBACK, BLOCK_ACK or ACK **/
  /**
   * Add Stream Measurement (SNR + RSSI). At least one stream must be added and a maximum
   * of 8 streams measurements.
   * \param measurement The measurement struct that includes SNR + RSSI.
   */
  void AddStreamMeasurement (StreamMeasurement &measurement);

  CT_FORMAT_TYPE GetControlTrailerFormatType (void) const;
  bool IsAggregateChannel (void) const;
  uint8_t GetBW (void) const;
  uint8_t GetPrimaryChannelNumber (void) const;
  bool IsMimoTransmission (void) const;
  bool IsMuMimoTransmission (void) const;
  uint8_t GetEdmgGroupID (void) const;
  uint8_t GetTxSectorCombinationIdx (void) const;
  bool GetHBF (void) const;
  uint8_t GetMuMimoTransmissionConfigType (void) const;
  uint8_t GetMuMimoConfigIdx (void) const;
  uint8_t GetTotalNumberOfSectorsMSB (void) const;
  uint8_t GetNumberOfRxDMGAntennasMSB (void) const;
  bool IsChannelNumber (void) const;
  uint16_t GetTotalNumberOfSectors (void) const;
  uint8_t GetNumberOfRXDMGAntennas (void) const;
  /**
   * Get SNR and RSSI measurements for a particular stream.
   * \param streamIndex The index of the stream (1 <= index <= 8).
   * \return The measurement structure corresponding to that stream.
   */
  StreamMeasurement GetStreamMeasurement (uint8_t streamIndex);

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

private:
  CT_FORMAT_TYPE m_ctFormatType;
  bool m_aggregateChannel;
  uint8_t m_bw;
  uint8_t m_primaryChannelNumber;
  bool m_mimoTransmission;
  bool m_muMimoTransmission;
  uint8_t m_txSectorCombinationIdx;
  uint8_t m_edmgGroupID;
  bool m_hbf;
  uint8_t m_muMimoTransmissionConfigType;
  uint8_t m_muMimoConfigIdx;
  uint8_t m_totalNumberOfSectorsMSB;
  uint8_t m_numberOfRxDMGAntennasMSB;
  bool m_isChannelNumber;
  uint16_t m_totalNumberOfSectors;
  uint8_t m_numberOfRXDMGAntennas;
  StreamMeasurementList m_streamMeasurementList;

};

} // namespace ns3

#endif /* CONTROL_TRAILER */

