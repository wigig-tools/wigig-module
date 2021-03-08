/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2020 IMDEA Networks Institute
 * Author: Nina Grosheva <nina_grosheva@hotmail.com>
 */

#ifndef EDMG_CAPABILITIES_H
#define EDMG_CAPABILITIES_H

#include <stdint.h>
#include "ns3/attribute-helper.h"
#include "ns3/buffer.h"
#include "ns3/mac48-address.h"
#include "ns3/wifi-information-element.h"

namespace ns3 {

/* EDMG Capabilities Subelements */

/**
 * \ingroup wifi
 *
 * Beamforming Capability subelement of EDMG Cababilities Element.
 *
 */
class BeamformingCapabilitySubelement : public WifiInformationElement
{
public:
  BeamformingCapabilitySubelement ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /**
   * The Requested BRP SC Blocks subfield indicates the minimum number of data SC blocks that the STA
   * requests be included in a PPDU carrying a TRN field and transmitted to the STA. The value of this subfield
   * is set to dot11RequestedBRPMinDataLength. If the PPDU is an OFDM mode PPDU, the minimum number
   * of OFDM symbols is calculated as described in 28.9.2.2.4 11ay D5.0.
   * \param reqBlocks
   */
  void SetRequestedBrpScBlocks (uint8_t reqBlocks);
  /**
   * The MU-MIMO Supported subfield indicates if the STA supports the DL MU-MIMO protocol including the
   * MU-MIMO beamforming protocol described in 10.42.10.2.3. This subfield is set to 1 if
   * dot11EDMGMIMOSupport is either muAndSuMimo (2) or reciprocalMuMimoAndSuMimo (3), and is set to 0 otherwise.
   * \param muMimoSupported
   */
  void SetMuMimoSupported (bool muMimoSupported);
  /**
   * The Reciprocal MU-MIMO Supported subfield indicates if the STA supports the reciprocal MU-MIMO
   * protocol specified in 10.42.10.2.3.3.3. This subfield is set to 1 if dot11EDMGMIMOSupport is
   * reciprocalMuMimoAndSuMimo (3), and is set to 0 otherwise. This subfield is reserved if the MU-MIMO
   * Supported field is 0.
   * \param reciprocalMuMimoSupported
   */
  void SetReciprocalMuMimoSupported (bool reciprocalMuMimoSupported);
  /**
   * The SU-MIMO Supported subfield indicates if the STA supports the SU-MIMO protocol including the SU-
   * MIMO beamforming protocol described in 10.42.10.2.2. This subfield is set to 1 if
   * dot11EDMGMIMOSupport is suMimoOnly (1), and is set to 0 otherwise.
   * \param suMimoSupported
   */
  void SetSuMimoSupported (bool suMimoSupported);
  /**
   * The Grant Required subfield indicates if the STA requires reception of a Grant frame to set up a MIMO
   * configuration. This subfield is set to 1 if dot11EDMGBFGrantRequired is true, and is set to 0 otherwise.
   * The Grant Required subfield is reserved if both the MU-MIMO Supported subfield and the SU-MIMO Supported
   * subfield are set to 0.
   * \param isGrantRequired
   */
  void SetGrantRequired (bool isGrantRequired);
  /**
   * The DMG TRN RX Only Capable subfield indicates if the STA is capable of receiving only DMG TRNs as
   * defined in 20.10.2.2.2, even when such TRNs are appended to an EDMG PPDU (see 28.9.2.2.3). This
   * subfield is set to 1 if dot11EDMGBFDMGTRNRXOnlyImplemented is true. Otherwise, this subfield is set to 0.
   * \param dmgTrnRxOnlyCapable
   */
  void SetDmgTrnRxOnlyCapable (bool dmgTrnRxOnlyCapable);
  /**
   * The First Path Training Supported subfield indicates if the STA supports the first path beamforming training
   * procedure defined in 10.42.10.6. This subfield is set to 1 if dot11FirstPathTrainingImplemented is true, and
   * is set to 0 otherwise.
   * \param firstPathTrainingSupported
   */
  void SetFirstPathTrainingSupported (bool firstPathTrainingSupported);
  /**
   * Set Dual Polarization TRN capability Subfield.
   * \param dualPolarizationTrnSupported The Dual Polarization TRN Supported subfield is set to 1 to indicate that
   * the repetition of the same TRN subfield with different polarizations, as specified in 10.42.10.7, is supported
   * by the STA both for transmit and receive. Otherwise, it is set to 0.
   * \param trnPowerDifference The TRN Power Difference subfield indicates the difference, in dB, between the radiated
   * power of consecutive TRN subfields transmitted with the same AWV but with different polarizations. The encoding of
   * the radiated power difference is shown in Table 9-321k.
   */
  void SetDualPolarizationTrnCapability (bool dualPolarizationTrnSupported, int8_t trnPowerDifference);
  /**
   * The MU-MIMO Supported subfield and the Hybrid Beamforming and MU-MIMO Supported subfield indicates if the STA
   * supports the hybrid beamforming protocol during MU-MIMO transmission, including the hybrid beamforming protocol
   * described in 10.42.10.2.4. This subfield is set to 1 if dot11EDMGHybridMUMIMOImplemented is true, and is set to 0
   * otherwise. The Hybrid Beamforming and MU-MIMO Supported subfield is reserved if the MU-MIMO Supported subfield is 0.
   * \param hybridBeamformingAndMuMimoSupported
   */
  void SetHybridBeamformingAndMuMimoSupported (bool hybridBeamformingAndMuMimoSupported);
  /**
   * The SU-MIMO Supported subfield and Hybrid Beamforming and SU-MIMO Supported subfield indicates if the STA supports
   * hybrid beamforming protocol during SU-MIMO transmission, including the hybrid beamforming protocol described in
   * 10.42.10.2.4. This subfield is set to 1 if dot11EDMGHybridSUMIMOImplemented is true, and is set to 0 otherwise.
   * The Hybrid Beamforming and SU-MIMO Supported subfield is reserved if the SU-MIMO Supported subfield is 0.
   * \param hybridBeamformingAndSuMimoSupported
   */
  void SetHybridBeamformingAndSuMimoSupported (bool hybridBeamformingAndSuMimoSupported);
  /**
   * The Largest Ng Supported subfield indicates largest value of N g that the EDMG STA supports for the
   * beamforming feedback matrix (see 9.4.2.282). This subfield is set to the value of dot11EDMGBFGrantLargestNgSupported,
   * i.e., 0 for N g =2, set to 1 for N g =4, and set to 2 for N g =8. Value of 3 is reserved.
   * \param largestNgSupported
   */
  void SetLargestNgSupported (uint8_t largestNgSupported);
  /**
   * The Dynamic Grouping Supported subfield indicates if the EDMG STA supports dynamic grouping. The field is set to 1
   * if dot11EDMGBFDynamicGroupingImplemented is true, and is set to 0 otherwise.
   * \param dynamicGroupingSupported
   */
  void SetDynamicGroupingSupported (bool dynamicGroupingSupported);

  uint8_t GetRequestedBrpScBlocks (void) const;
  bool GetMuMimoSupported (void) const;
  bool GetReciprocalMuMimoSupported (void) const;
  bool GetSuMimoSupported (void) const;
  bool GetGrantRequired (void) const;
  bool GetDmgTrnRxOnlyCapable (void) const;
  bool GetFirstPathTrainingSupported (void) const;
  bool GetDualPolarizationTrnSupported (void) const;
  int8_t GetTrnPowerDifference (void) const;
  bool GetHybridBeamformingAndMuMimoSupported (void) const;
  bool GetHybridBeamformingAndSuMimoSupported (void) const;
  uint8_t GetLargestNgSupported (void) const;
  bool GetDynamicGroupingSupported (void) const;

  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  uint16_t GetSerializedSize () const;


private:
  uint8_t m_reqBrpScBlocks;
  bool m_muMimoSupported;
  bool m_reciprocalMuMimoSupported;
  bool m_suMimoSupported;
  bool m_isGrantRequired;
  bool m_dmgTrnRxOnlyCapable;
  bool m_firstPathTrainingSupported;
  bool m_dualPolarizationTrnSupported;
  uint8_t m_trnPowerDifference;
  bool m_hybridBeamformingAndMuMimoSupported;
  bool m_hybridBeamformingAndSuMimoSupported;
  uint8_t m_largestNgSupported;
  bool m_dynamicGroupingSupported;

};

enum TxRxSubfield {
  TX_AND_RX = 1,
  TX_ONLY = 2,
  RX_ONLY = 3
};

enum PolarizationConfigurationType {
  SINGLE_POLARIZATION = 0,
  POLARIZATION_SWITCH = 1,
  SYNTHESIZABLE_POLARIZATION = 2,
  MIMO_DUAL_POLARIZATION = 3
};

struct PolarizationCapability {
  TxRxSubfield TxRx;
  PolarizationConfigurationType PolarizationConfiguration;
  /**
   * The definition of the Polarization Description subfield depends on the setting of the Polarization
   * Configuration subfield.
   *
   * If the value of the Polarization Configuration subfield is equal to single polarization or MIMO dual
   * polarization, the Polarization Description subfield is set to 0 to indicate linear polarization, is set to 1 to
   * indicate circular polarization and is set to 2 for mixed polarization. Other values are reserved.
   *
   * If the value of the Polarization Configuration subfield is equal to synthesizable polarization, the Polarization
   * Description subfield is set to 0 to indicate linear polarization, is set to 1 to indicate circular polarization, is
   * set to 2 for mixed polarization, and is set to 3 to indicate support for both linear and circular polarization.
   * Other values are reserved.
   *
   * If the value of the Polarization Configuration subfield is equal to polarization switch, the Polarization
   * Description subfield is defined as shown in Figure 9-787at.
   */
  uint8_t PolarizationDescription;
};

typedef std::vector<PolarizationCapability> PolarizationCapabilityList;
typedef PolarizationCapabilityList::iterator PolarizationCapabilityListI;
typedef PolarizationCapabilityList::const_iterator PolarizationCapabilityListCI;

/**
 * \ingroup wifi
 *
 * Antenna Polarization Capability subelement of EDMG Cababilities Element.
 *
 */
class AntennaPolarizationCapabilitySubelement : public WifiInformationElement
{
public:
  AntennaPolarizationCapabilitySubelement ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /**
   * The value of the Number of DMG Antennas subfield plus one defines the combined total number, N, of RX
   * and TX antennas of an EDMG STA.
   * \param numberOfDmgAntennas
   */
  void SetNumberOfDmgAntennas (uint8_t numberOfDmgAntennas);
  /**
   * The Polarization Capability List subfield contains N Polarization Capability subfields.
   * Each Polarization Capability i subfield, 1 ≤ i ≤ N, describes the polarization characteristics of the
   * DMG antenna that is identified by index i.
   * \param capability
   */
  void AddPolarizationCapability (const PolarizationCapability &capability);

  uint8_t GetNumberOfDmgAntennas (void) const;
  PolarizationCapabilityList GetPolarizationCapabilityList (void) const;

  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  uint16_t GetSerializedSize () const;


private:
  uint8_t m_numberOfDmgAntennas;
  PolarizationCapabilityList m_list;

};

enum STBC {
  STBC_NOT_SUPPORTED = 0,
  SINGLE_STREAM_STBC_RX = 1,
  MULTIPLE_STREAMS_STBC_RX = 2
};

/**
 * \ingroup wifi
 *
 * PHY Capabilities subelement of EDMG Capabilities Element.
 *
 */
class PhyCapabilitiesSubelement : public WifiInformationElement
{
public:
  PhyCapabilitiesSubelement ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  void SetPhaseHoppingSupported (bool phaseHoppingSupported);
  void SetOpenLoopPrecodingSupported (bool openLoopPrecodingSupported);
  void SetDcmPi2BPSKSupported (bool dcmPi2BPSKSupported);
  void SetRate78ShortCwPuncturedSupported (bool rate78ShortCwPuncturedSupported);
  void SetRate78ShortCwSuperimposedSupported (bool rate78ShortCwSuperimposedSupported);
  void SetRate78LongCwPuncturedSupported (bool rate78LongCwPuncturedSupported);
  void SetRate78LongCwSuperimposedSupported (bool rate78LongCwSuperimposedSupported);
  void SetScMaxNumberofSuMimoSpatialStreamsSupported (uint8_t scMaxNumberofSuMimoSpatialStreamsSupported);
  void SetOfdmMaxNumberofSuMimoSpatialStreamsSupported (uint8_t ofdmMaxNumberofSuMimoSpatialStreamsSupported);
  void SetNucTxSupported (bool nucTxSupported);
  void SetNucRxSupported (bool nucRxSupported);
  void SetPi28PskSupported (bool pi28PskSupported);
  void SetNumberOfConcurrentRfChains (uint8_t numberOfConcurrentRfChains);
  void SetStbcType (STBC stbcType);
  void SetEdmgAPpdu (bool edmgAPpdu);
  void SetLongCwSupported (bool longCwSupported);

  bool GetPhaseHoppingSupported (void) const;
  bool GetOpenLoopPrecodingSupported (void) const;
  bool GetDcmPi2BPSKSupported (void) const;
  bool GetRate78ShortCwPuncturedSupported (void) const;
  bool GetRate78ShortCwSuperimposedSupported (void) const;
  bool GetRate78LongCwPuncturedSupported (void) const;
  bool GetRate78LongCwSuperimposedSupported (void) const;
  uint8_t GetScMaxNumberofSuMimoSpatialStreamsSupported (void) const;
  uint8_t GetOfdmMaxNumberofSuMimoSpatialStreamsSupported (void) const;
  bool GetNucTxSupported (void) const;
  bool GetNucRxSupported (void) const;
  bool GetPi28PskSupported (void) const;
  uint8_t GetNumberOfConcurrentRfChains (void) const;
  STBC GetStbcType (void) const;
  bool GetEdmgAPpdu (void) const;
  bool GetLongCwSupported (void) const;

  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  uint16_t GetSerializedSize () const;


private:
  bool m_phaseHoppingSupported;
  bool m_openLoopPrecodingSupported;
  bool m_dcmPi2BPSKSupported;
  bool m_rate78ShortCwPuncturedSupported;
  bool m_rate78ShortCwSuperimposedSupported;
  bool m_rate78LongCwPuncturedSupported;
  bool m_rate78LongCwSuperimposedSupported;
  uint8_t m_scMaxNumberofSuMimoSpatialStreamsSupported;
  uint8_t m_ofdmMaxNumberofSuMimoSpatialStreamsSupported;
  bool m_nucTxSupported;
  bool m_nucRxSupported;
  bool m_pi28PskSupported;
  uint8_t m_numberOfConcurrentRfChains;
  STBC m_stbcType;
  bool m_edmgAPpdu;
  bool m_longCwSupported;
};

typedef uint8_t EdmgChannelNumber;

struct ChannelAggregationCombination {
  EdmgChannelNumber AggregatedChannel1;
  EdmgChannelNumber AggregatedChannel2;
};

typedef std::vector<EdmgChannelNumber> EdmgChannelList;
typedef EdmgChannelList::iterator EdmgChannelListI;
typedef EdmgChannelList::const_iterator EdmgChannelListCI;

typedef std::vector<ChannelAggregationCombination> ChannelAggregationCombinationList;
typedef ChannelAggregationCombinationList::iterator EChannelAggregationCombinationListI;
typedef ChannelAggregationCombinationList::const_iterator ChannelAggregationCombinationListCI;

struct EdmgChannelsInformation
{
  uint8_t NumberOfEdmgChannels;
  EdmgChannelList list;
};

struct EdmgAggregatedChannelsInformation
{
  uint8_t NumberOfChannelAggregationCombinations;
  ChannelAggregationCombinationList list;
};

#define EDMG_NUM_CHANNELS       29

/**
 * \ingroup wifi
 *
 * Supported Channels subelement of EDMG Cababilities Element.
 */
class SupportedChannelsSubelement : public WifiInformationElement
{
public:
  SupportedChannelsSubelement ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  void SetNumberOfEdmgChannels (uint8_t numberOfEdmgChannels);
  void AddEdmgChannel (const EdmgChannelNumber &channel);
  void SetNumberOfChannelAggregationCombinations (uint8_t numberOfChannelAggregationCombinations);
  void AddChannelAggregationCombination (const ChannelAggregationCombination &channelCombination);

  uint8_t GetNumberOfEdmgChannels (void) const;
  EdmgChannelList GetEdmgChannelList (void) const;
  uint8_t GetNumberOfChannelAggregationCombinations (void) const;
  ChannelAggregationCombinationList GetChannelAggregationCombinationList (void) const;

  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  uint16_t GetSerializedSize () const;

private:
  /**
   * Add support to all EDMG channels (1-29) as defined in IEEE 802.11ay D5.0 28.3.4 Channelization.
   */
  void SupportAllEdmgChannels (void);

private:
  EdmgChannelsInformation m_channelsInfo;
  EdmgAggregatedChannelsInformation m_aggregatedChannelsInfo;

};

enum smPowerSaveMode {
  STATIC_SM_POWER_SAVE = 0,
  DYNAMIC_SM_POWER_SAVE = 1,
  POWER_SAVE_DISABLED = 3
};

/**
 * \ingroup wifi
 *
 * MAC Capabilities subelement of EDMG Capabilities Element.
 *
 */
class MacCapabilitiesSubelement : public WifiInformationElement
{
public:
  MacCapabilitiesSubelement ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  void SetEdmgMultiTidAggregationSupport (uint8_t edmgMultiTidAggregationSupport);
  void SetEdmgAllAckSupport (bool edmgAllAckSupport);
  void SetSmPowerSave (smPowerSaveMode smPowerSave);
  void SetScheduledRdSupported (bool scheduledRdSupported);

  uint8_t GetEdmgMultiTidAggregationSupport (void) const;
  bool GetEdmgAllAckSupport (void) const;
  smPowerSaveMode GetSmPowerSave (void) const;
  bool GetScheduledRdSupported (void) const;

  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  uint16_t GetSerializedSize () const;


private:
  uint8_t m_edmgMultiTidAggregationSupport;
  bool m_edmgAllAckSupport;
  smPowerSaveMode m_smPowerSave;
  bool m_scheduledRdSupported;
};

typedef uint8_t EdmgCapabilitiesSubelementId;

#define BEAMFORMING_CAPABILITY_SUBELEMENT                       ((EdmgCapabilitiesSubelementId)0)
#define ANTENNA_POLARIZATION_CAPABILITY_SUBELEMENT              ((EdmgCapabilitiesSubelementId)1)
#define PHY_CAPABILITIES_SUBELEMENT                             ((EdmgCapabilitiesSubelementId)2)
#define SUPPORTED_CHANNELS_SUBELEMENT                           ((EdmgCapabilitiesSubelementId)3)
#define MAC_CAPABILITIES_SUBELEMENT                             ((EdmgCapabilitiesSubelementId)4)

/**
 * \ingroup wifi
 *
 * The IEEE 802.11ay EDMG Capabilities Information Element.
 */
class EdmgCapabilities : public WifiInformationElement
{
public:
  EdmgCapabilities ();

  WifiInformationElementId ElementId () const;
  WifiInformationElementId ElementIdExt () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /**
   * Set the Core Capabilities info field in the EDMG Capabilities information element.
   *
   * \param info the Core Capabilities info field in the EDMG Capabilities information element.
   */
  void SetCoreCapabilites (uint64_t info);

  /**
   * Return the Core Capabilities info field in the EDMG Capabilities information element.
   *
   * \return the Core Capabilities info field in the EDMG Capabilities information element.
   */
  uint64_t GetCoreCapabilities (void) const;

  /* Core Capabilities Info fields */
  /**
   * Set the A-MPDU Parameters field in the Core Capabilities Field.
   *
   * \param ampduExponent Using this field we set the maximum length of A-MPDU that the STA can receive.
   * This field is an integer in the range of 0 to 9. The length defined by this field is equal to
   * 2^(13 + Maximum A-MPDU Length Exponent) – 1 octets.
   * \param minimumMpduSpacing the minimum time between the start of adjacent MPDUs within
   * an A-MPDU that the STA can receive, measured at the PHY-SAP.
   * Set to 0 for no restriction
   * Set to 1 for 8 ns
   * Set to 2 for 16 ns
   * Set to 3 for 32 ns
   * Set to 4 for 64 ns
   * Set to 5 for 128 ns
   * Set to 6 for 256 ns
   * Set to 7 for 512 ns
   */
  void SetAmpduParameters (uint8_t ampduExponent, uint8_t minimumMpduSpacing);
  /**
   * Set the TRN parameters field in the Core Capabilities Field.
   * The structure of the TRN parameters field is defined in figure 48 (Draft Standard v4).
   * \param  TP1 indicates that the STA is capable of transmitting an EDMG PPDU with TXVECTOR
   * parameter EDMG_TRN_P equal to 1.
   * \param  TP4 indicates that the STA is capable of transmitting an EDMG PPDU with TXVECTOR
   * parameter EDMG_TRN_P equal to 4.
   * \param  TN2 indicates that the STA is capable of transmitting an EDMG PPDU with TXVECTOR
   * parameter EDMG_TRN_N equal to 2.
   * \param  TN4 indicates that the STA is capable of transmitting an EDMG PPDU with TXVECTOR
   * parameter EDMG_TRN_N equal to 4.
   * \param  TN8 indicates that the STA is capable of transmitting an EDMG PPDU with TXVECTOR
   * parameter EDMG_TRN_N equal to 8.
   * \param  RP1 indicates that the STA is capable of receiving an EDMG PPDU with RXVECTOR
   * parameter EDMG_TRN_P equal to 1.
   * \param  RP4 indicates that the STA is capable of receiving an EDMG PPDU with RXVECTOR
   * parameter EDMG_TRN_P equal to 4.
   * \param  RN2 indicates that the STA is capable of receiving an EDMG PPDU with RXVECTOR
   * parameter EDMG_TRN_N equal to 2.
   * \param  RN4 indicates that the STA is capable of receiving an EDMG PPDU with RXVECTOR
   * parameter EDMG_TRN_N equal to 4.
   * \param  RN8 indicates that the STA is capable of receiving an EDMG PPDU with RXVECTOR
   * parameter EDMG_TRN_N equal to 8.
   * \param shortTrn is set to 1 to indicate that the STA is capable of receiving TRN subfields based on
   * short Golay sequences (see 29.9.2.2.6) in SC mode or short TRN subfields (see 29.9.2.2.7) in OFDM mode,
   * otherwise it is set to 0.
   * \param longTrn is set to 1 to indicate that the STA is capable of receiving TRN subfields based on
   * long Golay sequences (see 29.9.2.2.6) in SC mode or long TRN subfields (see 29.9.2.2.7) in OFDM mode,
   * otherwise it is set to 0.
   */
  void SetTrnParameters (bool TP1, bool TP4, bool TN2, bool TN4, bool TN8, bool RP1, bool RP4,
                         bool RN2, bool RN4, bool RN8, bool shortTrn, bool longTrn);
  /**
   * The Supported MCS Set field indicates which MCSs a DMG STA supports. An MCS is identified by an
   * MCS index, which is represented by an integer in the range of 0 to 31. The interpretation of the MCS index
   * (i.e., the mapping from MCS to data rate) is PHY dependent. For the EDMG PHY, see section 29.5.8 and 29.6.8. The
   * structure of the Supported MCS Set field is defined in Figure 49 (Draft Standard v4).
   * \param maximumScMcs The value of the index of the highest supported receive EDMG SC mode MCS.
   * The mandatory EDMG SC mode MCSs are not impacted by the value of this subfield.
   * \param maximumOfdmMcs the value of the the index of the highest supported receive EDMG OFDM
   * mode MCS. A value of zero indicates that the EDMG OFDM mode is not supported.
   * \param maximumPhyRate the value of the maximum PHY data rate, in units of 100 Mbps, that the STA
   * supports in receive mode, over all supported channel bandwidths and number of spatial streams. This PHY
   * data rate can be lower than the data rate provided by the maximum supported MCS when used with a
   * combination of the largest supported channel bandwidth and the maximum number of supported spatial
   * streams.
   * \param ScMcs6OfdmMcs5Supported it is set to 1 to indicate that MCS 6 of the EDMG SC mode and
   * MCS 5 of the EDMG OFDM mode are supported in SISO. Otherwise, this subfield is set to 0.
   */
  void SetSupportedMCS (uint8_t maximumScMcs, uint8_t maximumOfdmMcs, uint16_t maximumPhyRate,
                        bool ScMcs6OfdmMcs5Supported);

  void AddSubElement (Ptr<WifiInformationElement> elem);

  uint8_t GetAmpduExponent (void) const;
  uint8_t GetAmpduMinimumSpacing (void) const;
  uint32_t GetMaxAmpduLength (void) const;
  uint8_t GetMaximumScMcs (void) const;
  uint8_t GetMaximumOfdmMcs (void) const;
  uint16_t GetMaximumPhyRate (void) const;
  bool GetScMcs6OfdmMcs5Supported (void) const;
  bool GetTP1Supported (void) const;
  bool GetTP4Supported (void) const;
  bool GetTN2Supported (void) const;
  bool GetTN4Supported (void) const;
  bool GetTN8Supported (void) const;
  bool GetRP1Supported (void) const;
  bool GetRP4Supported (void) const;
  bool GetRN2Supported (void) const;
  bool GetRN4Supported (void) const;
  bool GetRN8Supported (void) const;
  bool GetShortTrnSupported (void) const;
  bool GetLongTrnSupported (void) const;

  /**
   * Get a specific SubElement by ID.
   * \param id The ID of the Wifi SubElement.
   * \return
   */
  Ptr<WifiInformationElement> GetSubElement (WifiInformationElementId id);
  /**
   * Get List of SubElement associated with this frame.
   * \return
   */
  WifiInformationSubelementMap GetListOfSubElements (void) const;

  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  uint16_t GetSerializedSize () const;

private:
  /** Core Capabilities Info fields **/

  /* A-MPDU parameters field */
  uint8_t m_ampduExponent;
  uint8_t m_ampduMinimumSpacing;

  /* TRN Parameters field */
  bool m_tp1Supported;
  bool m_tp4Supported;
  bool m_tn2Supported;
  bool m_tn4Supported;
  bool m_tn8Supported;
  bool m_rp1Supported;
  bool m_rp4Supported;
  bool m_rn2Supported;
  bool m_rn4Supported;
  bool m_rn8Supported;
  bool m_shortTrnSupported;
  bool m_longTrnSupported;

  /* Supported MCS field */
  uint8_t m_maximumScMcs;
  uint8_t m_maximumOfdmMcs;
  uint8_t m_maximumPhyRate;
  bool m_scMcs6OfdmMcs5Supported;

  /* List of Subelements associated with this frame */
  WifiInformationSubelementMap m_map;

};

std::ostream &operator << (std::ostream &os, const EdmgCapabilities &edmgcapabilities);
std::istream &operator >> (std::istream &is, EdmgCapabilities &edmgcapabilities);

ATTRIBUTE_HELPER_HEADER (EdmgCapabilities);

typedef std::vector<Ptr<EdmgCapabilities> > EdmgCapabilitiesList;
typedef EdmgCapabilitiesList::const_iterator EdmgCapabilitiesListCI;
typedef EdmgCapabilitiesList::iterator EdmgCapabilitiesListI;

} //namespace ns3

#endif /* EDMG_CAPABILITY_H */
