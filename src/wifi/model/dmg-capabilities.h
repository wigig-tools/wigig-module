/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */

#ifndef DMG_CAPABILITIES_H
#define DMG_CAPABILITIES_H

#include <stdint.h>
#include "ns3/attribute-helper.h"
#include "ns3/buffer.h"
#include "ns3/mac48-address.h"
#include "ns3/wifi-information-element.h"

namespace ns3 {

/**
 * \ingroup wifi
 *
 * The IEEE 802.11ad DMG Capabilities
 */
class DmgCapabilities : public WifiInformationElement
{
public:
  DmgCapabilities ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /**
   * Set the STA Address Info field in the DMG Capabilities information element.
   *
   * \param address the STA Address Info field in the DMG Capabilities information element
   */
  void SetStaAddress (Mac48Address address);
  /**
   * Set the AID Info field in the DMG Capabilities information element.
   *
   * \param aid the AID Info field in the DMG Capabilities information element
   */
  void SetAID (uint8_t aid);
  /**
   * Set the DMG STA Capability Info field in the DMG Capabilities information element.
   *
   * \param info the DMG STA Capability Info field in the DMG Capabilities information element
   */
  void SetDmgStaCapabilityInfo (uint64_t info);
  /**
   * Set the DMG PCP/AP Capability Info field in the DMG Capabilities information element.
   *
   * \param info the DMG PCP/AP Capability Info field in the DMG Capabilities information element
   */
  void SetDmgPcpApCapabilityInfo (uint16_t info);

  /**
   * Return the STA Address Info field in the DMG Capabilities information element.
   *
   * \return the STA Address Info field in the DMG Capabilities information element
   */
  Mac48Address GetStaAddress () const;
  /**
   * Return the AID Info field in the DMG Capabilities information element.
   *
   * \return the AID Info field in the DMG Capabilities information element
   */
  uint8_t GetAID () const ;
  /**
   * Return the DMG STA Capability Info field in the DMG Capabilities information element.
   *
   * \return the DMG STA Capability Info field in the DMG Capabilities information element
   */
  uint64_t GetDmgStaCapabilityInfo () const;
  /**
   * Return the DMG PCP/AP Capability Info field in the DMG Capabilities information element.
   *
   * \return the DMG PCP/AP Capability Info field in the DMG Capabilities information element
   */
  uint16_t GetDmgPcpApCapabilityInfo () const;

  /* DMG STA Capability Info fields */

  /**
   * Set the Reverse Direction field if the STA supports RD as defined in 9.25.
   *
   * \param value true if the STA supports RD otherwise false
   */
  void SetReverseDirection (bool value);
  /**
   * Set the Higher Layer Timer Synchronization field if the STA supports Higher Layer Timer
   * Synchronization as defined in 10.23.5 and is set to 0 otherwise.
   *
   * \param value true if the STA supports Higher Layer Timer Synchronization, otherwise false
   */
  void SetHigherLayerTimerSynchronization (bool value);
  void SetTPC (bool value);
  void SetSPSH (bool value);
  /**
   * Set the Number of RX DMG Antennas of receive DMG antennas of the STA.
   * The value of this field is in the range of 1 to 4, with the value being equal to the bit representation plus 1.
   * \param number The number of RX DMG Antennas of receive DMG antennas of the STA
   */
  void SetNumberOfRxDmgAntennas (uint8_t number);
  /**
   * The Fast Link Adaptation field is set to 1 to indicate that the STA supports the fast link adaptation procedure
   * described in 9.37.3. Otherwise, it is set to 0.
   * \param value
   */
  void SetFastLinkAdaption (bool value);
  /**
   * The Total Number of Sectors field indicates the total number of transmit sectors the STA uses in a transmit
   * sector sweep combined over all DMG antennas. The value of this field is in the range of 1 to 128, with the
   * value being equal to the bit representation plus 1
   * \param number the total number of transmit sectors the STA uses
   */
  void SetNumberOfSectors (uint8_t number);
  /**
   * The value represented by the RXSS Length field specifies the total number of receive sectors combined over
   * all receive DMG antennas of the STA. The value represented by this field is in the range of 2 to 128 and is
   * given by (RXSS Length+1)×2. The maximum number of SSW frames transmitted during an RXSS is equal
   * to the value of (RXSS Length+1)×2 times the total number of transmit DMG antennas of the peer device.
   * \param length the total number of receive sectors combined over all receive DMG antennas of the STA
   */
  void SetRxssLength (uint8_t length);
  /**
   * The DMG Antenna Reciprocity field is set to 1 to indicate that the best transmit DMG antenna of the STA is
   * the same as the best receive DMG antenna of the STA and vice versa. Otherwise, this field is set to 0.
   * \param reciprocity
   */
  void SetDmgAntennaReciprocity (bool reciprocity);
  /**
   * Set the A-MPDU Parameters field.
   *
   * \param maximumLength the maximum length of A-MPDU that the STA can receive.
   * This field is an integer in the range of 0 to 5. The length defined by this field is equal to
   * 2^(13 + Maximum A-MPDU Length Exponent) – 1 octets.
   * \param minimumMpduSpacing the minimum time between the start of adjacent MPDUs within
   * an A-MPDU that the STA can receive, measured at the PHY-SAP.
   * Set to 0 for no restriction
   * Set to 1 for 16 ns
   * Set to 2 for 32 ns
   * Set to 3 for 64 ns
   * Set to 4 for 128 ns
   * Set to 5 for 256 ns
   * Set to 6 for 512 ns
   * Set to 7 for 1024 ns
   */
  void SetAmpduParameters (uint8_t maximumLength, uint8_t minimumMpduSpacing);
  /**
   * Set the BA Flow Control field to 1 if the STA supports BA with flow control as
   * defined in 9.36 and is set to 0 otherwise.
   *
   * \param value true if the STA supports BA with flow control, otherwise false
   */
  void SetBaFlowControl (bool value);
  /**
   * The Supported MCS Set field indicates which MCSs a DMG STA supports. An MCS is identified by an
   * MCS index, which is represented by an integer in the range of 0 to 31. The interpretation of the MCS index
   * (i.e., the mapping from MCS to data rate) is PHY dependent. For the DMG PHY, see Clause 21. The
   * structure of the Supported MCS Set field is defined in Figure 8-401q.
   * \param maximumScRxMcs The value of the maximum MCS index the STA supports for reception of single-carrier
   * frames. Values 0-3 of this field are reserved. Possible values for this subfield are shown in Table 21-18
   * \param maximumOfdmRxMcs the value of the maximum MCS index the STA supports for reception of OFDM frames.
   * If this field is set to 0, it indicates that the STA does not support reception of OFDM frames.
   * Values 1-17 of this field are reserved. Possible values for this subfield are described in 21.5.3.1.2.
   * \param maximumScTxMcs the value of the maximum MCS index the STA supports for transmission of single-carrier
   *  frames. Values 0-3 of this field are reserved. Possible values for this subfield are shown in Table 21-18.
   * \param maximumOfdmTxMcs the value of the maximum MCS index the STA supports for transmission of OFDM frames.
   * If this field is set to 0, it indicates that the STA does not support transmission of OFDM frames. Values 1-17
   * of this field are reserved. Possible values for this subfield are described 21.5.3.1.2.
   * \param lowPower it indicates that the STA supports the DMG low- power SC PHY mode (21.7). Otherwise, it is set
   *  to 0. If the STA supports the low-power SC PHY, it supports all low-power SC PHY MCSs indicated in Table 21-22.
   * \param codeRate13_16 It is set to 1 to indicate that the STA supports rate 13/16 and is set to 0 otherwise.
   * If this field is 0, MCSs with 13/16 code rate specified in Table 21-14 and Table 21-18 are not supported
   * regardless of the value in Maximum SC/OFDM Tx/Rx MCS subfields.
   */
  void SetSupportedMCS (uint8_t maximumScRxMcs, uint8_t maximumOfdmRxMcs, uint8_t maximumScTxMcs,
                        uint8_t maximumOfdmTxMcs, bool lowPower, bool codeRate13_16);
  /**
   * Set the DTP Supported field to 1 to indicate that the STA supports DTP as described in 9.38 and
   * 21.5.3.2.4.6.3. Otherwise, it is set to 0.
   * \param value true if the STA supports DTP, otherwise false
   */
  void SetDtpSupported (bool value);
  /**
   * Aet the A-PPDU Supported field to 1 to indicate that the STA supports A-PPDU aggregation as described
   * in 9.13a. Otherwise, it is set to 0.
   * \param value true if the STA supports A-PPDU aggregation, otherwise false
   */
  void SetAppduSupported (bool value);
  /**
   * Set the Heartbeat field to 1 to indicate that the STA expects to receive a frame from the PCP/AP during
   * the ATI (9.33.3) and expects to receive a frame with the DMG Control modulation from a source DMG STA
   * at the beginning of an SP (9.33.6.2) or TXOP (9.19.2). Otherwise, it is set to 0.
   * \param value
   */
  void SetHeartbeat (bool value);
  /**
   * Set Supports Other_AID field to 1 to indicate that the STA sets its AWV configuration according to
   * the Other_AID subfield in the BRP Request field during the BRP procedure as described in 9.35.6.4.4 and
   * 21.10.2.2.6, if the value of the Other_AID subfield is different from zero. Otherwise, this field is set to 0.
   * \param value
   */
  void SetSupportsOtherAid (bool value);
  /**
   * Set the Antenna Pattern Reciprocity field to 1 to indicate that the transmit antenna pattern associated with
   * an AWV is the same as the receive antenna pattern for the same AWV. Otherwise, this field is set to 0.
   * \param value
   */
  void SetAntennaPatternReciprocity (bool value);
  void SetHeartbeatElapsedIndication (uint8_t indication);
  /**
   * Set the Grant ACK Supported field to 1 to indicate that the STA is capable of responding to a Grant frame
   * with a Grant ACK frame. Otherwise, this field is set to 0.
   * \param value
   */
  void SetGrantAckSupported (bool value);
  /**
   * The RXSSTxRate Supported field is set to 1 to indicate that the STA can perform an RXSS with SSW
   * frames transmitted at MCS 1 of the DMG SC modulation class. Otherwise, it is set to 0.
   * \param value
   */
  void SetRXSSTxRateSupported (bool value);

  bool GetReverseDirection (void) const;
  bool GetHigherLayerTimerSynchronization (void) const;
  bool GetTPC (void) const;
  bool GetSPSH (void) const;
  uint8_t GetNumberOfRxDmgAntennas (void) const;
  bool GetFastLinkAdaption (void) const;
  uint8_t GetNumberOfSectors (void) const;
  uint8_t GetRxssLength (void) const;
  bool GetDmgAntennaReciprocity (void) const;
  uint8_t GetAmpduMaximumLength (void) const;
  uint8_t GetAmpduMinimumSpacing (void) const;
  bool GetBaFlowControl (void) const;
  uint8_t GetMaximumScRxMcs (void) const;
  uint8_t GetMaximumOfdmRxMcs (void) const;
  uint8_t GetMaximumScTxMcs (void) const;
  uint8_t GetMaximumOfdmTxMcs (void) const;
  bool GetLowPowerScSupported (void) const;
  bool GetCodeRate13_16Suppoted (void) const;
  bool GetDtpSupported (void) const;
  bool GetAppduSupported (void) const;
  bool GetHeartbeat (void) const;
  bool GetSupportsOtherAid (void) const;
  bool GetAntennaPatternReciprocity (void) const;
  uint8_t GetHeartbeatElapsedIndication (void) const;
  bool GetGrantAckSupported (void) const;
  bool GetRXSSTxRateSupported (void) const;

  /***** DMG PCP/AP Capability Info fields ******/

  /**
   * The TDDTI (time division data transfer interval) field is set to 1 if the STA, while operating as a
   * PCP/AP, is capable of providing channel access as defined in 9.33.6 and 10.4 and is set to 0 otherwise.
   * \param tddti
   */
  void SetTDDTI (bool tddti);
  /**
   * The Pseudo-static Allocations field is set to 1 if the STA, while operating as a PCP/AP, is capable of
   * providing pseudo-static allocations as defined in 9.33.6.4 and is set to 0 otherwise. The Pseudo-static
   * Allocations field is set to 1 only if the TDDTI field in the DMG PCP/AP Capability Information field is set
   * to 1. The Pseudo-static Allocations field is reserved if the TDDTI field in the DMG PCP/AP Capability
   * Information field is set to 0.
   * \param pseudoStatic
   */
  void SetPseudoStaticAllocations (bool pseudoStatic);
  /**
   * The PCP Handover field is set to 1 if the STA, while operating as a PCP, is capable of performing a PCP
   * Handover as defined in 10.28.2 and is set to 0 if the STA does not support PCP Handover.
   * \param handover
   */
  void SetPcpHandover (bool handover);
  /**
   * The MAX Associated STA Number field indicates the maximum number of STAs that the STA can perform association
   * with if operating as a PCP/AP. The value of this field includes the STAs, if any, that are colocated with
   * the PCP/AP.
   * \param max The maximum number of STAs that the STA can perform association with if operating as a PCP/AP.
   */
  void SetMaxAssociatedStaNumber (uint8_t max);
  /**
   * \param powerSource The Power Source field is set to 0 if the STA is battery powered and is set to 1 otherwise.
   */
  void SetPowerSource (bool powerSource);
  /**
   * The Decentralized PCP/AP Clustering field is set to 1 if the STA, when operating as a PCP/AP, is capable of
   * performing Decentralized PCP/AP clustering and is set to 0 otherwise.
   * \param decentralized
   */
  void SetDecentralizedClustering (bool decentralized);
  /**
   * The PCP Forwarding field is set to 1 if the STA, while operating as a PCP, is capable of forwarding frames
   * it receives from a non-PCP STA and destined to another non-PCP STA in the PBSS. This field is set to 0 otherwise.
   * \param forwarding
   */
  void SetPcpForwarding (bool forwarding);
  /**
   * The Centralized PCP/AP Clustering field is set to 1 if the STA, when operating as a PCP/AP, is capable of
   * performing centralized PCP/AP clustering and is set to 0 otherwise. A PCP/AP that is incapable of
   * performing centralized PCP/AP clustering is subject to requirements as described in 9.34.2.2.
   * \param centralized
   */
  void SetCentralizedClustering (bool centralized);

  bool GetTDDTI (void) const;
  bool GetPseudoStaticAllocations (void) const;
  bool GetPcpHandover (void) const;
  uint8_t GetMaxAssociatedStaNumber (void) const;
  bool GetPowerSource (void) const;
  bool GetDecentralizedClustering (void) const;
  bool GetPcpForwarding (void) const;
  bool GetCentralizedClustering (void) const;

  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  uint16_t GetSerializedSize () const;

private:
  Mac48Address m_staAddress;
  uint8_t m_aid;

  /** DMG STA Capability Info fields **/
  bool m_reverseDirection;
  bool m_higherLayerTimerSynchronziation;
  bool m_tpc;
  bool m_spsh;
  uint8_t m_RxDmgAntennas;
  bool m_fastLinkAdaption;
  uint8_t m_sectorsNumber;
  uint8_t m_rxssLength;
  bool m_dmgAntennaReciprocity;
  uint8_t m_ampduMaximumLength;
  uint8_t m_ampduMinimumSpacing;
  bool m_baFlowControl;
  /* Supported MCS Set field format */
  uint8_t m_maximumScRxMcs;
  uint8_t m_maximumOfdmRxMcs;
  uint8_t m_maximumScTxMcs;
  uint8_t m_maximumOfdmTxMcs;
  bool m_lowPower;
  bool m_codeRate13_16;

  bool m_dtpSupported;
  bool m_appduSupported;
  bool m_heartbeat;
  bool m_supportsOtherAid;
  bool m_antennaPatternReciprocity;
  uint8_t m_heartbeatElapsedIndication;
  bool m_GrantAckSupported;
  bool m_RxssTxRateSupported;

  /** DMG PCP/AP Capability Info fields **/
  bool m_tddti;
  bool m_pseudoStaticAllocations;
  bool m_pcpHandover;
  uint8_t m_maxAssoicatedStaNumber;
  bool m_powerSource;
  bool m_decentralizedClustering;
  bool m_pcpForwarding;
  bool m_centralizedClustering;

};

std::ostream &operator << (std::ostream &os, const DmgCapabilities &dmgcapabilities);
std::istream &operator >> (std::istream &is, DmgCapabilities &dmgcapabilities);

ATTRIBUTE_HELPER_HEADER (DmgCapabilities)

typedef std::vector<Ptr<DmgCapabilities> > DmgCapabilitiesList;

} //namespace ns3

#endif /* DMG_CAPABILITY_H */
