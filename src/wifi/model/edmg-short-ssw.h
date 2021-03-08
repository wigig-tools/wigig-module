/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#ifndef EDMG_SHORT_SSW_H
#define EDMG_SHORT_SSW_H

#include "ns3/header.h"

namespace ns3 {

class Time;

enum BeamformingPacketType {
  SHORT_SSW = 0,
  RESERVED = 1,
};

enum TransmissionDirection {
  BEAMFORMING_INITIATOR = 0,
  BEAMFORMING_RESPONDER = 1,
};

enum AddressingMode {
  INDIVIDUAL_ADRESS = 0,
  GROUP_ADDRESS = 1,
};

/**
 * \ingroup wifi
 * Implement the header for Short SSW (29.9.1).
 */
class ShortSSW : public Header
{
public:
  ShortSSW ();
  ~ShortSSW ();

  /**
   * \Set the type of the packet.
   * \param type
   */
  void SetPacketType (BeamformingPacketType type);
  /**
   * Set the direction of the transmission.
   * \param direction
   */
  void SetDirection (TransmissionDirection direction);
  /**
   * Set addressing mode
   * \param mode If set to 0, this indicates that the Destination AID field contains an individual address.
   * Otherwise, the Destination AID field contains a group address. In case of an individual address, the
   * SISO beamforming training is used. Otherwise, the MU-MIMO beamforming training is used.
   */
  void SetAddressingMode (AddressingMode mode);
  /**
   * Set Source AID
   * \param aid
   */
  void SetSourceAID (uint8_t aid);
  /**
   * SetDestinationAID
   * \param aid
   */
  void SetDestinationAID (uint8_t aid);
  void SetCDOWN (uint16_t cdown);
  void SetRFChainID (uint8_t id);
  void SetShortScrambledBSSID (uint16_t bssid);
  void SetAsUnassociated (bool value);
  void SetSisoFbckDuration (Time duration);
  void SetShortSSWFeedback (uint16_t feedback);

  BeamformingPacketType GetPacketType (void) const;
  TransmissionDirection GetDirection (void) const;
  AddressingMode GetAddressingMode (void) const;
  uint8_t GetSourceAID (void) const;
  uint8_t GetDestinationAID (void) const;
  uint16_t GetCDOWN (void) const;
  uint8_t GetRFChainID (void) const;
  uint16_t GetShortScrambledBSSID (void) const;
  bool GetUnassociated (void) const;
  Time GetSisoFbckDuration (void) const;
  uint16_t GetShortSSWFeedback (void) const;

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

private:
  BeamformingPacketType m_packetType;            //!< Beamforming packet type.
  TransmissionDirection m_transmissionDirection; //!< Transmission Direction.
  AddressingMode m_addressingMode;               //!< Addressing mode.
  uint8_t m_sourceAID;                           //!< Source AID.
  uint8_t m_destinationAID;                      //!< Destination AID.
  uint16_t m_cdown;                              //!< CDOWN.
  uint8_t m_rfChainID;
  uint16_t m_bssID;
  bool m_unassociated;
  uint16_t m_sisoFbckDuration;
  uint16_t m_feedback;

};

} // namespace ns3

#endif /* EDMG_SHORT_SSW_H */
