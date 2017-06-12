/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#ifndef EXT_HEADERS_H
#define EXT_HEADERS_H

#include "ns3/header.h"
#include "ns3/mac48-address.h"

#include "common-header.h"
#include "dmg-capabilities.h"
#include "dmg-information-elements.h"
#include "fields-headers.h"
#include "ssid.h"

namespace ns3 {

/**********************************************************
*  Channel Measurement Info field format (Figure 8-502h)
**********************************************************/

/**
 * \ingroup wifi
 * Implement the header for Channel Measurement Info field format (Figure 8-502h)
 */
class ExtChannelMeasurementInfo : public SimpleRefCount<ExtChannelMeasurementInfo>
{
public:
  ExtChannelMeasurementInfo ();
  ~ExtChannelMeasurementInfo ();

  void Print (std::ostream &os) const;
  uint32_t GetSerializedSize (void) const;
  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  Buffer::Iterator Deserialize (Buffer::Iterator start);

  /**
   * Set the AID of the STA toward which the reporting STA measures link.
   * \param aid
   */
  void SetPeerStaAid (uint16_t aid);
  /**
   * The SNR subfield indicates the SNR measured in the link toward the STA corresponding to Peer STA AID.
   * This field is encoded as 8-bit twos complement value of 4x(SNR-19), where SNR is measured in dB. This
   * covers from –13 dB to 50.75 dB in 0.25 dB steps.
   * \param
   */
  void SetSnr (uint8_t snr);
  /**
   * The Internal Angle subfield indicates the angle between directions toward the other STAs involved in the
   * relay operation. This covers from 0 degree to 180 degree in 2 degree steps. This subfield uses the degree of
   * the direction from the sector that the feedbacks indicates has highest quality during TXSS if SLS phase of
   * BF is performed and RXSS is not included.
   * \param angle
   */
  void SetInternalAngle (uint8_t angle);
  /**
   * The Recommend subfield indicates whether the responding STA recommends the relay operation based on
   * the channel measurement with the Peer STA. This subfield is set to 1 when the relay operation is
   * recommended and otherwise is set to 0.
   * \param value
   */
  void SetRecommendSubField (bool value);
  void SetReserved (uint8_t reserved);

  uint16_t GetPeerStaAid (void) const;
  uint8_t GetSnr (void) const;
  uint8_t GetInternalAngle (void) const;
  bool GetRecommendSubField (void) const;
  uint8_t GetReserved (void) const;

private:
  uint16_t m_aid;               //!< Peer STA AID.
  uint8_t m_snr;
  uint8_t m_angle;
  bool m_recommend;
  uint8_t m_reserved;

};

enum BSSType
{
  Reserved = 0,
  IBSS = 1,
  PBSS = 2,
  InfrastructureBSS = 3
};

/******************************************
*	DMG Parameters Field (8.4.1.46)
*******************************************/

/**
 * \ingroup wifi
 * Implement the header for DMG Parameters Field.
 */
class ExtDMGParameters : public ObjectBase
{
public:
  ExtDMGParameters ();
  ~ExtDMGParameters ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  void Print (std::ostream &os) const;
  uint32_t GetSerializedSize (void) const;
  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  Buffer::Iterator Deserialize (Buffer::Iterator start);

  /**
   * Set the Basic Service Set (BSS) Type.
   *
   * \param duration The duration of the beacon frame
   */
  void Set_BSS_Type (enum BSSType type);
  /**
   * The CBAP Only subfield indicates the type of link access provided by the STA sending the DMG
   * Beacon frame in the data transfer interval (DTI) (see 9.33.2) of the beacon interval. The CBAP Only
   * subfield is set to 1 when the entirety of the DTI portion of the beacon interval is allocated as a
   * CBAP. The CBAP Only subfield is set to 0 when the allocation of the DTI portion of the beacon
   * interval is provided through the Extended Schedule element.
   * \param value
   */
  void Set_CBAP_Only (bool value);
  /**
   * The CBAP Source subfield is valid only if the CBAP Only subfield is 1. The CBAP Source subfield
   * is set to 1 to indicate that the PCP/AP has higher priority to initiate transmissions during the CBAP
   * than non-PCP/non-AP STAs. The CBAP Source subfield is set to 0 otherwise.
   * \param value
   */
  void Set_CBAP_Source (bool value);
  void Set_DMG_Privacy (bool value);
  /**
   * The ECPAC Policy Enforced subfield is set to 1 to indicate that medium access policies specific to
   * the centralized PCP/AP cluster are required as defined in 9.34.3.4. The ECPAC Policy Enforced
   * subfield is set to 0 to indicate that medium access policies specific to the centralized PCP/AP cluster
   * are not required.
   * \param value
   */
  void Set_ECPAC_Policy_Enforced (bool value);
  void Set_Reserved (uint8_t value);

  /**
  * Return the Basic Service Set (BSS) Type.
  *
  * \return BSSType
  */
  enum BSSType Get_BSS_Type (void) const;
  bool Get_CBAP_Only (void) const;
  bool Get_CBAP_Source (void) const;
  bool Get_DMG_Privacy (void) const;
  bool Get_ECPAC_Policy_Enforced (void) const;
  uint8_t Get_Reserved (void) const;

private:
  enum BSSType m_bssType;	    //!< BSS Type.
  bool m_cbapOnly;		    //!< CBAP Only.
  bool m_cbapSource;		    //!< CBAP Source.
  bool m_dmgPrivacy;		    //!< DMG Privacy.
  bool m_ecpacPolicyEnforced;	    //!< ECPAC Policy Enforced.
  uint8_t m_reserved;

};

/******************************************
*   Beacon Interval Control Field (8-34b)
*******************************************/

/**
 * \ingroup wifi
 * Implement the header for DMG Beacon Interval Control Field.
 */
class ExtDMGBeaconIntervalCtrlField : public ObjectBase
{
public:
  ExtDMGBeaconIntervalCtrlField ();
  ~ExtDMGBeaconIntervalCtrlField ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  void Print (std::ostream &os) const;
  uint32_t GetSerializedSize (void) const;
  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  Buffer::Iterator Deserialize (Buffer::Iterator start);

  /**
   * Set the duration of the beacon frame.
   * \param duration The duration of the beacon frame
   */
  void SetCCPresent (bool value);
  /**
   * The Discovery Mode field is set to 1 if the STA is generating the DMG Beacon following the procedure
   * described in 10.1.3.2b. Otherwise, this field is set to 0.
   * \param value
   */
  void SetDiscoveryMode (bool value);
  /**
   * The Next Beacon field indicates the number of beacon intervals following the current beacon interval during
   * which the DMG Beacon is not be present.
   * \param value
   */
  void SetNextBeacon (uint8_t value);
  /**
   * The ATI Present field is set to 1 to indicate that the announcement transmission interval (ATI) is present in
   * the current beacon interval. Otherwise, the ATI is not present.
   * \param value
   */
  void SetATIPresent (bool value);
  /**
   * The A-BFT Length field specifies the size of the A-BFT following the BTI, and is defined in units of a
   * sector sweep slot (9.35.5). The value of this field is in the range of 1 to 8, with the value being
   * equal to the bit representation plus 1.
   * \param length The length of A-BFT period
   */
  void SetABFT_Length (uint8_t length);
  /**
   * The FSS field specifies the number of SSW frames allowed per sector sweep slot (9.35.5). The value of this
   * field is in the range of 1 to 16, with the value being equal to the bit representation plus 1.
   * \param number The number of SSW frames per sector sweep slot.
   */
  void SetFSS (uint8_t number);
  /**
   * The IsResponderTXSS field is set to 1 to indicate the A-BFT following the BTI is used for responder
   * transmit sector sweep (TXSS). This field is set to 0 to indicate responder receive sector sweep (RXSS).
   * When this field is set to 0, the FSS field specifies the length of a complete receive sector sweep by the STA
   * sending the DMG Beacon frame.
   * \param value
   */
  void SetIsResponderTXSS (bool value);
  /**
   * The Next A-BFT field indicates the number of beacon intervals during which the A-BFT is not be present.
   * A value of 0 indicates that the A-BFT immediately follows this BTI.
   * \param value
   */
  void SetNextABFT (uint8_t value);
  /**
   * The Fragmented TXSS field is set to 1 to indicate the TXSS is a fragmented sector sweep, and is set to 0 to
   * indicate the TXSS is a complete sector sweep.
   * \param value
   */
  void SetFragmentedTXSS (bool value);
  /**
   * The TXSS Span field indicates the number of beacon intervals it takes for the STA sending the DMG
   * Beacon frame to complete the TXSS phase. This field is always greater than or equal to one.
   * \param value
   */
  void SetTXSS_Span (uint8_t value);
  /**
   * The N BIs A-BFT field indicates the interval, in number of beacon intervals, at which the STA sending
   * the DMG Beacon frame allocates an A-BFT. A value of 1 indicates that every beacon interval contains an A-BFT.
   * \param value
   */
  void SetN_BI (uint8_t value);
  /**
   * The A-BFT Count field indicates the number of A-BFTs since the STA sending the DMG Beacon frame last
   * switched RX DMG antennas for an A-BFT. A value of 0 indicates that the DMG antenna used in the
   * forthcoming A-BFT differs from the DMG antenna used in the last A-BFT. This field is reserved if the value
   * of the Number of RX DMG Antennas field within the STA’s DMG Capabilities element is 1.
   * \param value
   */
  void SetABFT_Count (uint8_t value);
  /**
   * The N A-BFT in Ant field indicates how many A-BFTs the STA sending the DMG Beacon frame receives
   * from each DMG antenna in the DMG antenna receive rotation. This field is reserved if the value of the
   * Number of RX DMG Antennas field within the STA’s DMG Capabilities element is 1.
   * \param value
   */
  void SetN_ABFT_Ant (uint8_t value);
  /**
   * The PCP Association Ready field is set to 1 to indicate that the PCP is ready to receive Association Request
   * frames from non-PCP STAs and is set to 0 otherwise. This field is reserved when transmitted by a non-PCP STA.
   * \param value
   */
  void SetPCPAssoicationReady (bool value);

  bool IsCCPresent (void) const;
  bool IsDiscoveryMode (void) const;
  uint8_t GetNextBeacon (void) const;
  bool IsATIPresent (void) const;
  uint8_t GetABFT_Length (void) const;
  uint8_t GetFSS (void) const;
  bool IsResponderTXSS (void) const;
  uint8_t GetNextABFT (void) const;
  bool GetFragmentedTXSS (void) const;
  uint8_t GetTXSS_Span (void) const;
  uint8_t GetN_BI (void) const;
  uint8_t GetABFT_Count (void) const;
  uint8_t GetN_ABFT_Ant (void) const;
  bool GetPCPAssoicationReady (void) const;

private:
  bool m_ccPresent;
  bool m_discoveryMode;
  uint8_t m_nextBeacon;
  bool m_ATI_Present;
  uint8_t m_ABFT_Length;
  uint8_t m_FSS;
  bool m_isResponderTXSS;
  uint8_t m_next_ABFT;
  bool m_fragmentedTXSS;
  uint8_t m_TXSS_Span;
  uint8_t m_N_BI;
  uint8_t m_ABFT_Count;
  uint8_t m_N_ABFT_Ant;
  bool m_pcpAssociationReady;

};

/******************************************
*	     DMG Beacon (8.3.4.1)
*******************************************/

/**
 * \ingroup wifi
 * Implement the header for DMG Beacon.
 */
class ExtDMGBeacon : public Header, public MgtFrame
{
public:
  ExtDMGBeacon ();
  ~ExtDMGBeacon ();

  /**
  * Set the Basic Service Set Identifier (BSSID).
  *
  * \param ssid BSSID
  */
  void SetBSSID (Mac48Address bssid);
  /**
  * Set the Timestamp in the DMG Beacon frame body.
  *
  * \param timestamp The timestamp.
  */
  void SetTimestamp (uint64_t timestamp);
  /**
  * Set Sector Sweep Information Field in the DMG Beacon frame body.
  *
  * \param ssw The sector sweep information field.
  */
  void SetSSWField (DMG_SSW_Field &ssw);
  /**
  * Set the DMG Beacon Interval.
  *
  * \param interval The DMG Beacon Interval.
  */
  void SetBeaconIntervalUs (uint64_t interval);
  /**
  * Set Beacon Interval Control Field in the DMG Beacon frame body.
  *
  * \param ssw The Beacon Interval Control field.
  */
  void SetBeaconIntervalControlField (ExtDMGBeaconIntervalCtrlField &ctrl);
  /**
  * Set DMG Parameters Field in the DMG Beacon frame body.
  *
  * \param parameters The DMG Parameters field.
  */
  void SetBeaconIntervalControlField (ExtDMGParameters &parameters);
  /**
  * Set DMG Parameters Field in the DMG Beacon frame body.
  *
  * \param The DMG Parameters field.
  */
  void SetDMGParameters (ExtDMGParameters &parameters);
  /**
  * Set DMG Cluster Control Field
  *
  * \param The DMG Cluster Control Field.
  */
  void SetClusterControlField (ExtDMGClusteringControlField &cluster);
  /**
  * Set the Service Set Identifier (SSID)
  *
  * \param ssid SSID.
  */
  void SetSsid (Ssid ssid);

  /**
  * Return the Service Set Identifier (SSID).
  *
  * \return SSID
  */
  Mac48Address GetBSSID (void) const;
  /**
  * Get the Timestamp in the DMG Beacon frame body.
  *
  * \return The timestamp.
  */
  uint64_t GetTimestamp (void) const;
  /**
  * Get Sector Sweep Information Field in the DMG Beacon frame body.
  *
  * \return The sector sweep information field.
  */
  DMG_SSW_Field GetSSWField (void) const;
  /**
  * Get the DMG Beacon Interval.
  *
  * \return The DMG Beacon Interval.
  */
  uint64_t GetBeaconIntervalUs (void) const;
  /**
  * Get Beacon Interval Control Field in the DMG Beacon frame body.
  *
  * \return The Beacon Interval Control field.
  */
  ExtDMGBeaconIntervalCtrlField GetBeaconIntervalControlField (void) const;
  /**
  * Get DMG Parameters Field in the DMG Beacon frame body.
  *
  * \return The DMG Parameters field.
  */
  ExtDMGParameters GetDMGParameters (void) const;
  /**
  * Get DMG Cluster Control Field
  *
  * \return The DMG Cluster Control Field.
  */
  ExtDMGClusteringControlField GetClusterControlField (void) const;
  /**
   * Return the Service Set Identifier (SSID).
   *
   * \return SSID
   */
  Ssid GetSsid (void) const;

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

private:
  Mac48Address m_bssid;                                   //!< Basic Service Set ID (SSID).
  uint64_t m_timestamp;                                   //!< Timestamp.
  DMG_SSW_Field m_ssw;                                    //!< Sector Sweep Field.
  uint64_t m_beaconInterval;                              //!< Beacon Interval.
  ExtDMGBeaconIntervalCtrlField m_beaconIntervalCtrl;     //!< Beacon Interval Control.
  ExtDMGParameters m_dmgParameters;                       //!< DMG Parameters.
  ExtDMGClusteringControlField m_cluster;                 //!< Cluster Control Field.
  Ssid m_ssid;                                            //!< Service set ID (SSID)

};

} // namespace ns3

#endif /* EXT_HEADERS_H */
