/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2020 IMDEA Networks Institute
 * Author: Nina Grosheva <nina_grosheva@hotmail.com>
 */

#include "ns3/address-utils.h"
#include "ns3/assert.h"
#include "ns3/log.h"

#include "edmg-capabilities.h"

NS_LOG_COMPONENT_DEFINE ("EdmgCapabilities");

namespace ns3 {

/********* EDMG Capabilities Subelements *********/

/** Beamforming Capability Subelement **/

BeamformingCapabilitySubelement::BeamformingCapabilitySubelement ()
  : m_reqBrpScBlocks (0),
    m_muMimoSupported (false),
    m_reciprocalMuMimoSupported (false),
    m_suMimoSupported (false),
    m_isGrantRequired (false),
    m_dmgTrnRxOnlyCapable (false),
    m_firstPathTrainingSupported (false),
    m_dualPolarizationTrnSupported (false),
    m_trnPowerDifference (0),
    m_hybridBeamformingAndMuMimoSupported (false),
    m_hybridBeamformingAndSuMimoSupported (false),
    m_largestNgSupported (0),
    m_dynamicGroupingSupported (false)
{
}

WifiInformationElementId
BeamformingCapabilitySubelement::ElementId () const
{
  return BEAMFORMING_CAPABILITY_SUBELEMENT;
}

uint8_t
BeamformingCapabilitySubelement::GetInformationFieldSize () const
{
  //we should not be here if dmg is not supported
  // Add optional Subelements
  return 4;
}

Buffer::Iterator
BeamformingCapabilitySubelement::Serialize (Buffer::Iterator i) const
{
  return WifiInformationElement::Serialize (i);
}

uint16_t
BeamformingCapabilitySubelement::GetSerializedSize () const
{
  return WifiInformationElement::GetSerializedSize ();
}

void
BeamformingCapabilitySubelement::SerializeInformationField (Buffer::Iterator start) const
{
  uint32_t InfoField1 = 0;
  InfoField1 |= m_reqBrpScBlocks & 0x1F;
  InfoField1 |= (m_muMimoSupported & 0x1) << 5;
  InfoField1 |= (m_reciprocalMuMimoSupported & 0x1) << 6;
  InfoField1 |= (m_suMimoSupported & 0x1) << 7;
  InfoField1 |= (m_isGrantRequired & 0x1) << 8;
  InfoField1 |= (m_dmgTrnRxOnlyCapable & 0x1) << 9;
  InfoField1 |= (m_firstPathTrainingSupported & 0x1) << 10;
  InfoField1 |= (m_dualPolarizationTrnSupported & 0x1) << 11;
  InfoField1 |= (m_trnPowerDifference & 0x7) << 12;
  InfoField1 |= (m_hybridBeamformingAndMuMimoSupported & 0x1) << 15;
  InfoField1 |= (m_hybridBeamformingAndSuMimoSupported & 0x1) << 16;
  InfoField1 |= (m_largestNgSupported & 0x3) << 17;
  InfoField1 |= (m_dynamicGroupingSupported & 0x1) << 19;
  start.WriteHtolsbU32 (InfoField1);
}

uint8_t
BeamformingCapabilitySubelement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;
  uint32_t InfoField1 = i.ReadLsbtohU32 ();

  m_reqBrpScBlocks                        = InfoField1 & 0x1F;
  m_muMimoSupported                       = (InfoField1 >> 5) & 0x1;
  m_reciprocalMuMimoSupported             = (InfoField1 >> 6) & 0x1;
  m_suMimoSupported                       = (InfoField1 >> 7) & 0x1;
  m_isGrantRequired                       = (InfoField1 >> 8) & 0x1;
  m_dmgTrnRxOnlyCapable                   = (InfoField1 >> 9) & 0x1;
  m_firstPathTrainingSupported            = (InfoField1 >> 10) & 0x1;
  m_dualPolarizationTrnSupported          = (InfoField1 >> 11) & 0x1;
  m_trnPowerDifference                    = (InfoField1 >> 12) & 0x7;
  m_hybridBeamformingAndMuMimoSupported   = (InfoField1 >> 15) & 0x1;
  m_hybridBeamformingAndSuMimoSupported   = (InfoField1 >> 16) & 0x1;
  m_largestNgSupported                    = (InfoField1 >> 17) & 0x3;
  m_dynamicGroupingSupported              = (InfoField1 >> 19) & 0x1;
  return length;
}

void
BeamformingCapabilitySubelement::SetRequestedBrpScBlocks (uint8_t reqBlocks)
{
  m_reqBrpScBlocks = reqBlocks;
}

void
BeamformingCapabilitySubelement::SetMuMimoSupported (bool muMimoSupported)
{
  m_muMimoSupported = muMimoSupported;
}

void
BeamformingCapabilitySubelement::SetReciprocalMuMimoSupported (bool reciprocalMuMimoSupported)
{
  m_reciprocalMuMimoSupported = reciprocalMuMimoSupported;
}

void
BeamformingCapabilitySubelement::SetSuMimoSupported (bool suMimoSupported)
{
  m_suMimoSupported = suMimoSupported;
}

void
BeamformingCapabilitySubelement::SetGrantRequired (bool isGrantRequired)
{
  m_isGrantRequired = isGrantRequired;
}

void
BeamformingCapabilitySubelement::SetDmgTrnRxOnlyCapable (bool dmgTrnRxOnlyCapable)
{
  m_dmgTrnRxOnlyCapable = dmgTrnRxOnlyCapable;
}

void
BeamformingCapabilitySubelement::SetFirstPathTrainingSupported (bool firstPathTrainingSupported)
{
  m_firstPathTrainingSupported = firstPathTrainingSupported;
}

void
BeamformingCapabilitySubelement::SetDualPolarizationTrnCapability (bool dualPolarizationTrnSupported, int8_t trnPowerDifference)
{
  m_dualPolarizationTrnSupported = dualPolarizationTrnSupported;
  switch (trnPowerDifference)
    {
    case 0:
      m_trnPowerDifference = 0;
      break;
    case 1:
      m_trnPowerDifference = 1;
       break;
    case 2:
      m_trnPowerDifference = 2;
       break;
    case -1:
      m_trnPowerDifference = 5;
      break;
    case -2:
      m_trnPowerDifference = 6;
      break;
    case -3:
      m_trnPowerDifference = 7;
      break;
    default:
      if (trnPowerDifference >= 3)
        {
          m_trnPowerDifference = 3;
        }
      else if (trnPowerDifference <= -4)
        {
          m_trnPowerDifference = 4;
        }
      break;
    }
}

void
BeamformingCapabilitySubelement::SetHybridBeamformingAndMuMimoSupported (bool hybridBeamformingAndMuMimoSupported)
{
  m_hybridBeamformingAndMuMimoSupported = hybridBeamformingAndMuMimoSupported;
}

void
BeamformingCapabilitySubelement::SetHybridBeamformingAndSuMimoSupported (bool hybridBeamformingAndSuMimoSupported)
{
  m_hybridBeamformingAndSuMimoSupported = hybridBeamformingAndSuMimoSupported;
}

void
BeamformingCapabilitySubelement::SetLargestNgSupported (uint8_t largestNgSupported)
{
  switch (largestNgSupported)
    {
    case 2:
      m_largestNgSupported = 0;
      break;
    case 4:
      m_largestNgSupported = 1;
      break;
    case 8:
      m_largestNgSupported = 2;
      break;
    default:
      NS_FATAL_ERROR ("Invalid Ng Value");
      break;
    }
}

void
BeamformingCapabilitySubelement::SetDynamicGroupingSupported (bool dynamicGroupingSupported)
{
  m_dynamicGroupingSupported = dynamicGroupingSupported;
}

uint8_t
BeamformingCapabilitySubelement::GetRequestedBrpScBlocks (void) const
{
  return m_reqBrpScBlocks;
}

bool
BeamformingCapabilitySubelement::GetMuMimoSupported (void) const
{
  return m_muMimoSupported;
}

bool
BeamformingCapabilitySubelement::GetReciprocalMuMimoSupported (void) const
{
  return m_reciprocalMuMimoSupported;
}

bool
BeamformingCapabilitySubelement::GetSuMimoSupported (void) const
{
  return m_suMimoSupported;
}

bool
BeamformingCapabilitySubelement::GetGrantRequired (void) const
{
  return m_isGrantRequired;
}

bool
BeamformingCapabilitySubelement::GetDmgTrnRxOnlyCapable (void) const
{
  return m_dmgTrnRxOnlyCapable;
}

bool
BeamformingCapabilitySubelement::GetFirstPathTrainingSupported (void) const
{
  return m_firstPathTrainingSupported;
}

bool
BeamformingCapabilitySubelement::GetDualPolarizationTrnSupported (void) const
{
  return m_dualPolarizationTrnSupported;
}

int8_t
BeamformingCapabilitySubelement::GetTrnPowerDifference (void) const
{
  switch (m_trnPowerDifference)
    {
    case 0:
      return 0;
      break;
    case 1:
      return 1;
      break;
    case 2:
      return 2;
      break;
    case 3:
      return 3;
      break;
    case 5:
      return -1;
      break;
    case 6:
      return -2;
      break;
    case 7:
      return -3;
      break;
    case 4:
      return -4;
      break;
    default:
      NS_FATAL_ERROR ("Wrong TRN Power Difference");
      break;
    }
}

bool
BeamformingCapabilitySubelement::GetHybridBeamformingAndMuMimoSupported (void) const
{
  return m_hybridBeamformingAndMuMimoSupported;
}

bool
BeamformingCapabilitySubelement::GetHybridBeamformingAndSuMimoSupported (void) const
{
  return m_hybridBeamformingAndSuMimoSupported;
}

uint8_t
BeamformingCapabilitySubelement::GetLargestNgSupported (void) const
{
  switch (m_largestNgSupported)
    {
    case 0:
      return 2;
      break;
    case 1:
      return 4;
      break;
    case 2:
      return 8;
      break;
    default:
      NS_FATAL_ERROR ("Invalid Ng Value");
      break;
    }
}

bool
BeamformingCapabilitySubelement::GetDynamicGroupingSupported (void) const
{
  return m_dynamicGroupingSupported;
}

/** Antenna Polarization Capability Subelement -*/

AntennaPolarizationCapabilitySubelement::AntennaPolarizationCapabilitySubelement ()
{
}

WifiInformationElementId
AntennaPolarizationCapabilitySubelement::ElementId () const
{
  return ANTENNA_POLARIZATION_CAPABILITY_SUBELEMENT;
}

uint8_t
AntennaPolarizationCapabilitySubelement::GetInformationFieldSize () const
{
  return 1 + (m_numberOfDmgAntennas + 1) * 2;
}

Buffer::Iterator
AntennaPolarizationCapabilitySubelement::Serialize (Buffer::Iterator i) const
{
  return WifiInformationElement::Serialize (i);
}

uint16_t
AntennaPolarizationCapabilitySubelement::GetSerializedSize () const
{
  return WifiInformationElement::GetSerializedSize ();
}

void
AntennaPolarizationCapabilitySubelement::SerializeInformationField (Buffer::Iterator start) const
{
  start.WriteU8 (m_numberOfDmgAntennas);
  for (PolarizationCapabilityListCI it = m_list.begin (); it != m_list.end (); it++)
    {
      uint16_t value = 0;
      value |= static_cast<uint8_t> (it->TxRx) & 0x3;
      value |= (static_cast<uint8_t> (it->PolarizationConfiguration) & 0x3) << 2;
      value |= (it->PolarizationDescription & 0x7F) << 4;
      start.WriteHtolsbU16 (value);
    }
}

uint8_t
AntennaPolarizationCapabilitySubelement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  m_numberOfDmgAntennas = start.ReadU8 ();
  uint16_t value;
  for (uint16_t i = 0; i < (m_numberOfDmgAntennas + 1); i++)
    {
      PolarizationCapability capability;
      value = start.ReadLsbtohU16 ();
      capability.TxRx = static_cast<TxRxSubfield> (value & 0x3);
      capability.PolarizationConfiguration = static_cast<PolarizationConfigurationType> ((value >> 2) & 0x3);
      capability.PolarizationDescription = (value >> 4) & 0x3F;
      m_list.push_back (capability);
    }
  return length;
}

void
AntennaPolarizationCapabilitySubelement::SetNumberOfDmgAntennas (uint8_t numberOfDmgAntennas)
{
  m_numberOfDmgAntennas = numberOfDmgAntennas;
}

void
AntennaPolarizationCapabilitySubelement::AddPolarizationCapability (const PolarizationCapability &capability)
{
  m_list.push_back (capability);
}

uint8_t
AntennaPolarizationCapabilitySubelement::GetNumberOfDmgAntennas (void) const
{
  return m_numberOfDmgAntennas;
}

PolarizationCapabilityList
AntennaPolarizationCapabilitySubelement::GetPolarizationCapabilityList (void) const
{
  return m_list;
}

/** PHY Capabilities Subelement **/

PhyCapabilitiesSubelement::PhyCapabilitiesSubelement ()
  : m_phaseHoppingSupported (false),
    m_openLoopPrecodingSupported (false),
    m_dcmPi2BPSKSupported (false),
    m_rate78ShortCwPuncturedSupported (false),
    m_rate78ShortCwSuperimposedSupported (false),
    m_rate78LongCwPuncturedSupported (false),
    m_rate78LongCwSuperimposedSupported (false),
    m_scMaxNumberofSuMimoSpatialStreamsSupported (0),
    m_ofdmMaxNumberofSuMimoSpatialStreamsSupported (0),
    m_nucTxSupported (false),
    m_nucRxSupported (false),
    m_pi28PskSupported (false),
    m_numberOfConcurrentRfChains (1),
    m_stbcType (STBC_NOT_SUPPORTED),
    m_edmgAPpdu (false),
    m_longCwSupported (false)
{
}

WifiInformationElementId
PhyCapabilitiesSubelement::ElementId () const
{
  return PHY_CAPABILITIES_SUBELEMENT;
}

uint8_t
PhyCapabilitiesSubelement::GetInformationFieldSize () const
{
  //we should not be here if dmg is not supported
  // Add optional Subelements
  return 3;
}

Buffer::Iterator
PhyCapabilitiesSubelement::Serialize (Buffer::Iterator i) const
{
  return WifiInformationElement::Serialize (i);
}

uint16_t
PhyCapabilitiesSubelement::GetSerializedSize () const
{
  return WifiInformationElement::GetSerializedSize ();
}

void
PhyCapabilitiesSubelement::SerializeInformationField (Buffer::Iterator start) const
{
  uint16_t InfoField1 = 0;
  uint8_t InfoField2 = 0;
  InfoField1 |= m_phaseHoppingSupported & 0x1;
  InfoField1 |= (m_openLoopPrecodingSupported & 0x1) << 1;
  InfoField1 |= (m_dcmPi2BPSKSupported & 0x1) << 2;
  InfoField1 |= (m_rate78ShortCwPuncturedSupported & 0x1) << 3;
  InfoField1 |= (m_rate78ShortCwSuperimposedSupported & 0x1) << 4;
  InfoField1 |= (m_rate78LongCwPuncturedSupported & 0x1) << 5;
  InfoField1 |= (m_rate78LongCwSuperimposedSupported & 0x1) << 6;
  InfoField1 |= (m_scMaxNumberofSuMimoSpatialStreamsSupported & 0x7) << 7;
  InfoField1 |= (m_ofdmMaxNumberofSuMimoSpatialStreamsSupported & 0x7) << 10;
  InfoField1 |= (m_nucTxSupported & 0x1) << 13;
  InfoField1 |= (m_nucRxSupported & 0x1) << 14;
  InfoField1 |= (m_pi28PskSupported & 0x1) << 15;
  InfoField2 |= m_numberOfConcurrentRfChains & 0x7;
  InfoField2 |= (m_stbcType & 0x3) << 3;
  InfoField2 |= (m_edmgAPpdu & 0x1) << 5;
  InfoField2 |= (m_longCwSupported & 0x1) << 6;
  start.WriteHtolsbU16 (InfoField1);
  start.WriteU8 (InfoField1);
}

uint8_t
PhyCapabilitiesSubelement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;
  uint16_t InfoField1 = i.ReadLsbtohU16 ();
  uint8_t InfoField2 = i.ReadU8 ();

  m_phaseHoppingSupported                        = InfoField1 & 0x1;
  m_openLoopPrecodingSupported                   = (InfoField1 >> 1) & 0x1;
  m_dcmPi2BPSKSupported                          = (InfoField1 >> 2) & 0x1;
  m_rate78ShortCwPuncturedSupported              = (InfoField1 >> 3) & 0x1;
  m_rate78ShortCwSuperimposedSupported           = (InfoField1 >> 4) & 0x1;
  m_rate78LongCwPuncturedSupported               = (InfoField1 >> 5) & 0x1;
  m_rate78LongCwSuperimposedSupported            = (InfoField1 >> 6) & 0x1;
  m_scMaxNumberofSuMimoSpatialStreamsSupported   = (InfoField1 >> 7) & 0x7;
  m_ofdmMaxNumberofSuMimoSpatialStreamsSupported = (InfoField1 >> 10) & 0x7;
  m_nucTxSupported                               = (InfoField1 >> 13) & 0x1;
  m_nucRxSupported                               = (InfoField1 >> 14) & 0x1;
  m_pi28PskSupported                             = (InfoField1 >> 15) & 0x1;
  m_numberOfConcurrentRfChains                   = InfoField2 & 0x7;
  m_stbcType                                     = static_cast<STBC> ((InfoField2 >> 3) & 0x3);
  m_edmgAPpdu                                    = (InfoField2 >> 5) & 0x1;
  m_longCwSupported                              = (InfoField2 >> 6) & 0x1;
  return length;
}

void
PhyCapabilitiesSubelement::SetPhaseHoppingSupported (bool phaseHoppingSupported)
{
  m_phaseHoppingSupported = phaseHoppingSupported;
}

void
PhyCapabilitiesSubelement::SetOpenLoopPrecodingSupported (bool openLoopPrecodingSupported)
{
  m_openLoopPrecodingSupported = openLoopPrecodingSupported;
}

void
PhyCapabilitiesSubelement::SetDcmPi2BPSKSupported (bool dcmPi2BPSKSupported)
{
  m_dcmPi2BPSKSupported = dcmPi2BPSKSupported;
}

void
PhyCapabilitiesSubelement::SetRate78ShortCwPuncturedSupported (bool rate78ShortCwPuncturedSupported)
{
  m_rate78ShortCwPuncturedSupported = rate78ShortCwPuncturedSupported;
}

void
PhyCapabilitiesSubelement::SetRate78ShortCwSuperimposedSupported (bool rate78ShortCwSuperimposedSupported)
{
  m_rate78ShortCwSuperimposedSupported = rate78ShortCwSuperimposedSupported;
}

void
PhyCapabilitiesSubelement::SetRate78LongCwPuncturedSupported (bool rate78LongCwPuncturedSupported)
{
  m_rate78LongCwPuncturedSupported = rate78LongCwPuncturedSupported;
}

void
PhyCapabilitiesSubelement::SetRate78LongCwSuperimposedSupported (bool rate78LongCwSuperimposedSupported)
{
  m_rate78LongCwSuperimposedSupported = rate78LongCwSuperimposedSupported;
}

void
PhyCapabilitiesSubelement::SetScMaxNumberofSuMimoSpatialStreamsSupported (uint8_t scMaxNumberofSuMimoSpatialStreamsSupported)
{
  m_scMaxNumberofSuMimoSpatialStreamsSupported = scMaxNumberofSuMimoSpatialStreamsSupported - 1;
}

void
PhyCapabilitiesSubelement::SetOfdmMaxNumberofSuMimoSpatialStreamsSupported (uint8_t ofdmMaxNumberofSuMimoSpatialStreamsSupported)
{
  m_ofdmMaxNumberofSuMimoSpatialStreamsSupported = ofdmMaxNumberofSuMimoSpatialStreamsSupported - 1;
}

void
PhyCapabilitiesSubelement::SetNucTxSupported (bool nucTxSupported)
{
  m_nucTxSupported = nucTxSupported;
}

void
PhyCapabilitiesSubelement::SetNucRxSupported (bool nucRxSupported)
{
  m_nucRxSupported = nucRxSupported;
}

void
PhyCapabilitiesSubelement::SetPi28PskSupported (bool pi28PskSupported)
{
  m_pi28PskSupported = pi28PskSupported;
}

void
PhyCapabilitiesSubelement::SetNumberOfConcurrentRfChains (uint8_t numberOfConcurrentRfChains)
{
  m_numberOfConcurrentRfChains = numberOfConcurrentRfChains - 1;
}

void
PhyCapabilitiesSubelement::SetStbcType (STBC stbcType)
{
  m_stbcType = stbcType;
}

void
PhyCapabilitiesSubelement::SetEdmgAPpdu (bool edmgAPpdu)
{
  m_edmgAPpdu = edmgAPpdu;
}

void
PhyCapabilitiesSubelement::SetLongCwSupported (bool longCwSupported)
{
  m_longCwSupported = longCwSupported;
}


bool
PhyCapabilitiesSubelement::GetPhaseHoppingSupported (void) const
{
  return m_phaseHoppingSupported;
}

bool
PhyCapabilitiesSubelement::GetOpenLoopPrecodingSupported (void) const
{
  return m_openLoopPrecodingSupported;
}

bool
PhyCapabilitiesSubelement::GetDcmPi2BPSKSupported (void) const
{
  return m_dcmPi2BPSKSupported;
}

bool
PhyCapabilitiesSubelement::GetRate78ShortCwPuncturedSupported (void) const
{
  return m_rate78ShortCwPuncturedSupported;
}

bool
PhyCapabilitiesSubelement::GetRate78ShortCwSuperimposedSupported (void) const
{
  return m_rate78ShortCwSuperimposedSupported;
}

bool
PhyCapabilitiesSubelement::GetRate78LongCwPuncturedSupported (void) const
{
  return m_rate78LongCwPuncturedSupported;
}

bool
PhyCapabilitiesSubelement::GetRate78LongCwSuperimposedSupported (void) const
{
  return m_rate78LongCwSuperimposedSupported;
}

uint8_t
PhyCapabilitiesSubelement::GetScMaxNumberofSuMimoSpatialStreamsSupported (void) const
{
  return m_scMaxNumberofSuMimoSpatialStreamsSupported + 1;
}

uint8_t
PhyCapabilitiesSubelement::GetOfdmMaxNumberofSuMimoSpatialStreamsSupported (void) const
{
  return m_ofdmMaxNumberofSuMimoSpatialStreamsSupported + 1;
}

bool
PhyCapabilitiesSubelement::GetNucTxSupported (void) const
{
  return m_nucTxSupported;
}

bool
PhyCapabilitiesSubelement::GetNucRxSupported (void) const
{
  return m_nucRxSupported;
}

bool
PhyCapabilitiesSubelement::GetPi28PskSupported (void) const
{
  return m_pi28PskSupported;
}

uint8_t
PhyCapabilitiesSubelement::GetNumberOfConcurrentRfChains (void) const
{
  return m_numberOfConcurrentRfChains + 1;
}

STBC PhyCapabilitiesSubelement::GetStbcType(void) const
{
  return m_stbcType;
}

bool
PhyCapabilitiesSubelement::GetEdmgAPpdu (void) const
{
  return m_edmgAPpdu;
}

bool
PhyCapabilitiesSubelement::GetLongCwSupported (void) const
{
  return m_longCwSupported;
}

/** Supported Channels Subelement **/

SupportedChannelsSubelement::SupportedChannelsSubelement ()
{
  SupportAllEdmgChannels ();
}

WifiInformationElementId
SupportedChannelsSubelement::ElementId () const
{
  return SUPPORTED_CHANNELS_SUBELEMENT;
}

uint8_t
SupportedChannelsSubelement::GetInformationFieldSize () const
{
  return (1 + m_channelsInfo.NumberOfEdmgChannels) + (1 + m_aggregatedChannelsInfo.NumberOfChannelAggregationCombinations * 2);
}

Buffer::Iterator
SupportedChannelsSubelement::Serialize (Buffer::Iterator i) const
{
  return WifiInformationElement::Serialize (i);
}

uint16_t
SupportedChannelsSubelement::GetSerializedSize () const
{
  return WifiInformationElement::GetSerializedSize ();
}

void
SupportedChannelsSubelement::SerializeInformationField (Buffer::Iterator start) const
{
  NS_ASSERT_MSG (m_channelsInfo.NumberOfEdmgChannels > 0,
                 "Support for at least one 2.16 GHz channel and one 4.32 GHz channel by an EDMG STA is mandatory.");
  start.WriteU8 (m_channelsInfo.NumberOfEdmgChannels);
  for (EdmgChannelListCI it = m_channelsInfo.list.begin (); it != m_channelsInfo.list.end (); it++)
    {
      start.WriteU8 (*it);
    }
  start.WriteU8 (m_aggregatedChannelsInfo.NumberOfChannelAggregationCombinations);
  for (ChannelAggregationCombinationListCI it = m_aggregatedChannelsInfo.list.begin (); it != m_aggregatedChannelsInfo.list.end (); it++)
    {
      start.WriteU8 (it->AggregatedChannel1);
      start.WriteU8 (it->AggregatedChannel2);
    }
}

uint8_t
SupportedChannelsSubelement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  m_channelsInfo.NumberOfEdmgChannels = start.ReadU8 ();
  NS_ASSERT_MSG (m_channelsInfo.NumberOfEdmgChannels > 0,
                 "Support for at least one 2.16 GHz channel and one 4.32 GHz channel by an EDMG STA is mandatory.");
  for (uint8_t i = 0; i < m_channelsInfo.NumberOfEdmgChannels; i++)
    {
      EdmgChannelNumber channel;
      channel = start.ReadU8 ();
      m_channelsInfo.list.push_back (channel);
    }
  m_aggregatedChannelsInfo.NumberOfChannelAggregationCombinations = start.ReadU8 ();
  for (uint8_t i = 0; i < m_aggregatedChannelsInfo.NumberOfChannelAggregationCombinations; i++)
    {
      ChannelAggregationCombination channelCombination;
      channelCombination.AggregatedChannel1 = start.ReadU8 ();
      channelCombination.AggregatedChannel2 = start.ReadU8 ();
      m_aggregatedChannelsInfo.list.push_back (channelCombination);
    }
  return length;
}

void
SupportedChannelsSubelement::SetNumberOfEdmgChannels (uint8_t numberOfEdmgChannels)
{
  m_channelsInfo.NumberOfEdmgChannels = numberOfEdmgChannels;
  m_channelsInfo.list.clear ();
}

void
SupportedChannelsSubelement::AddEdmgChannel (const EdmgChannelNumber &channel)
{
  m_channelsInfo.list.push_back (channel);
}

void
SupportedChannelsSubelement::SupportAllEdmgChannels (void)
{
  m_channelsInfo.list.clear ();
  m_channelsInfo.NumberOfEdmgChannels = EDMG_NUM_CHANNELS;
  for(uint8_t i = 1; i <= EDMG_NUM_CHANNELS; i++)
    m_channelsInfo.list.push_back (i);
}

void
SupportedChannelsSubelement::SetNumberOfChannelAggregationCombinations (uint8_t numberOfChannelAggregationCombinations)
{
  m_aggregatedChannelsInfo.NumberOfChannelAggregationCombinations = numberOfChannelAggregationCombinations;
}

void
SupportedChannelsSubelement::AddChannelAggregationCombination (const ChannelAggregationCombination &channelCombination)
{
  m_aggregatedChannelsInfo.list.push_back (channelCombination);
}

uint8_t
SupportedChannelsSubelement::GetNumberOfEdmgChannels (void) const
{
  return m_channelsInfo.NumberOfEdmgChannels;
}

EdmgChannelList
SupportedChannelsSubelement::GetEdmgChannelList (void) const
{
  return m_channelsInfo.list;
}

uint8_t
SupportedChannelsSubelement::GetNumberOfChannelAggregationCombinations (void) const
{
  return m_aggregatedChannelsInfo.NumberOfChannelAggregationCombinations;
}

ChannelAggregationCombinationList
SupportedChannelsSubelement::GetChannelAggregationCombinationList (void) const
{
  return m_aggregatedChannelsInfo.list;
}

/** MAC Capabilities Subelement **/

MacCapabilitiesSubelement::MacCapabilitiesSubelement ()
  : m_edmgMultiTidAggregationSupport (0),
    m_edmgAllAckSupport (false),
    m_smPowerSave (POWER_SAVE_DISABLED),
    m_scheduledRdSupported (false)
{
}

WifiInformationElementId
MacCapabilitiesSubelement::ElementId () const
{
  return MAC_CAPABILITIES_SUBELEMENT;
}

uint8_t
MacCapabilitiesSubelement::GetInformationFieldSize () const
{
  //we should not be here if dmg is not supported
  return 2;
}

Buffer::Iterator
MacCapabilitiesSubelement::Serialize (Buffer::Iterator i) const
{
  return WifiInformationElement::Serialize (i);
}

uint16_t
MacCapabilitiesSubelement::GetSerializedSize () const
{
  return WifiInformationElement::GetSerializedSize ();
}

void
MacCapabilitiesSubelement::SerializeInformationField (Buffer::Iterator start) const
{
  uint16_t InfoField = 0;
  InfoField |= m_edmgMultiTidAggregationSupport & 0xF;
  InfoField |= (m_edmgAllAckSupport & 0x1) << 4;
  InfoField |= (static_cast<uint8_t> (m_smPowerSave) & 0x3) << 5;
  InfoField |= (m_scheduledRdSupported & 0x1) << 6;
  start.WriteHtolsbU16 (InfoField);
}

uint8_t
MacCapabilitiesSubelement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;
  uint16_t InfoField = i.ReadLsbtohU16 ();

  m_edmgMultiTidAggregationSupport = InfoField & 0xF;
  m_edmgAllAckSupport              = (InfoField >> 4) & 0x1;
  m_smPowerSave                    = static_cast<smPowerSaveMode> ((InfoField >> 5) & 0x3);
  m_scheduledRdSupported           = (InfoField >> 7) & 0x1;
  return length;
}

void
MacCapabilitiesSubelement::SetEdmgMultiTidAggregationSupport (uint8_t edmgMultiTidAggregationSupport)
{
  m_edmgMultiTidAggregationSupport = edmgMultiTidAggregationSupport - 1;
}

void
MacCapabilitiesSubelement::SetEdmgAllAckSupport (bool edmgAllAckSupport)
{
  m_edmgAllAckSupport = edmgAllAckSupport;
}

void
MacCapabilitiesSubelement::SetSmPowerSave (smPowerSaveMode smPowerSave)
{
  m_smPowerSave = smPowerSave;
}

void
MacCapabilitiesSubelement::SetScheduledRdSupported (bool scheduledRdSupported)
{
  m_scheduledRdSupported = scheduledRdSupported;
}

uint8_t
MacCapabilitiesSubelement::GetEdmgMultiTidAggregationSupport (void) const
{
  return m_edmgMultiTidAggregationSupport + 1;
}

bool
MacCapabilitiesSubelement::GetEdmgAllAckSupport (void) const
{
  return m_edmgAllAckSupport;
}

smPowerSaveMode
MacCapabilitiesSubelement::GetSmPowerSave (void) const
{
  return m_smPowerSave;
}

bool
MacCapabilitiesSubelement::GetScheduledRdSupported (void) const
{
  return m_scheduledRdSupported;
}

EdmgCapabilities::EdmgCapabilities ()
  : m_ampduExponent (9),
    m_ampduMinimumSpacing (0),
    m_tp1Supported (0),
    m_tp4Supported (0),
    m_tn2Supported (0),
    m_tn4Supported (0),
    m_tn8Supported (0),
    m_rp1Supported (0),
    m_rp4Supported (0),
    m_rn2Supported (0),
    m_rn4Supported (0),
    m_rn8Supported (0),
    m_shortTrnSupported (0),
    m_longTrnSupported (0),
    m_maximumScMcs (0),
    m_maximumOfdmMcs (0),
    m_maximumPhyRate (0),
    m_scMcs6OfdmMcs5Supported (0)
{
}

WifiInformationElementId
EdmgCapabilities::ElementId () const
{
  return IE_EXTENSION;
}

WifiInformationElementId
EdmgCapabilities::ElementIdExt () const
{
  return IE_EXTENSION_EDMG_CAPABILITIES;
}

uint8_t
EdmgCapabilities::GetInformationFieldSize () const
{
  //Note: We should not be here if dmg is not supported
  uint8_t size = 7;   /* 7 Bytes: Extension Element ID + Size of Core Capabilites field */
  /* Add the size of the subelements present in the EDMG Capabilities element */
  Ptr<WifiInformationElement> element;
  for (WifiInformationSubelementMap::const_iterator elem = m_map.begin (); elem != m_map.end (); elem++)
    {
      element = elem->second;
      size += element->GetSerializedSize ();
    }
  return size;
}

Buffer::Iterator
EdmgCapabilities::Serialize (Buffer::Iterator i) const
{
  return WifiInformationElement::Serialize (i);
}

uint16_t
EdmgCapabilities::GetSerializedSize () const
{
  return WifiInformationElement::GetSerializedSize ();
}

void
EdmgCapabilities::SerializeInformationField (Buffer::Iterator start) const
{
  uint16_t InfoField1 = 0;
  uint32_t InfoField2 = 0;
  InfoField1 |= m_ampduExponent & 0xF;
  InfoField1 |= (m_ampduMinimumSpacing  & 0x7) << 4;
  InfoField1 |= (m_tp1Supported & 0x1) << 7;
  InfoField1 |= (m_tp4Supported & 0x1) << 8;
  InfoField1 |= (m_tn2Supported & 0x1) << 9;
  InfoField1 |= (m_tn4Supported & 0x1) << 10;
  InfoField1 |= (m_tn8Supported & 0x1) << 11;
  InfoField1 |= (m_rp1Supported & 0x1) << 12;
  InfoField1 |= (m_rp4Supported & 0x1) << 13;
  InfoField1 |= (m_rn2Supported & 0x1) << 14;
  InfoField1 |= (m_rn4Supported & 0x1) << 15;
  InfoField2 |= m_rn8Supported & 0x1;
  InfoField2 |= (m_shortTrnSupported & 0x1) << 1;
  InfoField2 |= (m_longTrnSupported & 0x1)  << 2;
  InfoField2 |= (m_maximumScMcs & 0x1F) << 3;
  InfoField2 |= (m_maximumOfdmMcs & 0x1F) << 8;
  InfoField2 |= (m_maximumPhyRate & 0xFFF) << 13;
  InfoField2 |= (m_scMcs6OfdmMcs5Supported & 0x1) << 25;
  start.WriteHtolsbU16 (InfoField1);
  start.WriteHtolsbU32 (InfoField2);
  Ptr<WifiInformationElement> element;
  for (WifiInformationSubelementMap::const_iterator elem = m_map.begin (); elem != m_map.end (); elem++)
    {
      element = elem->second;
      start = element->Serialize (start);
    }
}

uint8_t
EdmgCapabilities::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;
  uint16_t InfoField1 = i.ReadLsbtohU16 ();
  uint32_t InfoField2 = i.ReadLsbtohU32 ();

  m_ampduExponent       = InfoField1 & 0xF;
  m_ampduMinimumSpacing = (InfoField1 >> 4) & 0x7;
  m_tp1Supported        = (InfoField1 >> 7) & 0x1;
  m_tp4Supported        = (InfoField1 >> 8) & 0x1;
  m_tn2Supported        = (InfoField1 >> 9) & 0x1;
  m_tn4Supported        = (InfoField1 >> 10) & 0x1;
  m_tn8Supported        = (InfoField1 >> 11) & 0x1;
  m_rp1Supported        = (InfoField1 >> 12) & 0x1;
  m_rp4Supported        = (InfoField1 >> 13) & 0x1;
  m_rn2Supported        = (InfoField1 >> 14) & 0x1;
  m_rn4Supported        = (InfoField1 >> 15) & 0x1;
  m_rn8Supported        = InfoField2 & 0x1;
  m_shortTrnSupported   = (InfoField2 >> 1) & 0x1;
  m_longTrnSupported    = (InfoField2 >> 2) & 0x1;
  m_maximumScMcs        = (InfoField2 >> 3) & 0x1F;
  m_maximumOfdmMcs      = (InfoField2 >> 8) & 0x1F;
  m_maximumPhyRate      = (InfoField2 >> 13) & 0xFFF;
  m_scMcs6OfdmMcs5Supported = (InfoField2 >> 25) & 0x1;

  uint8_t deserializedLength = 6;

  Ptr<WifiInformationElement> element;
  uint8_t id, subelemLen;
  while (deserializedLength != length)
    {
      i = DeserializeElementID (i, id, subelemLen);
      switch (id)
        {
          case BEAMFORMING_CAPABILITY_SUBELEMENT:
            {
              element = Create<BeamformingCapabilitySubelement> ();
              break;
            }
          case ANTENNA_POLARIZATION_CAPABILITY_SUBELEMENT:
            {
              element = Create<AntennaPolarizationCapabilitySubelement> ();
              break;
            }
          case PHY_CAPABILITIES_SUBELEMENT:
            {
              element = Create<PhyCapabilitiesSubelement> ();
              break;
            }
          case SUPPORTED_CHANNELS_SUBELEMENT:
            {
              element = Create<SupportedChannelsSubelement> ();
              break;
            }
          case MAC_CAPABILITIES_SUBELEMENT:
            {
              element = Create<MacCapabilitiesSubelement> ();
              break;
            }
          default:
            {
              NS_FATAL_ERROR ("Cannot recognise the subelement ID");
              break;
            }
        }
      i = element->DeserializeElementBody (i, subelemLen);
      m_map[id] = element;
      deserializedLength += subelemLen + 2;
    }
  return length;
}

void
EdmgCapabilities::SetCoreCapabilites (uint64_t info)
{
  m_ampduExponent       = info & 0xF;
  m_ampduMinimumSpacing = (info >> 4) & 0x7;
  m_tp1Supported        = (info >> 7) & 0x1;
  m_tp4Supported        = (info >> 8) & 0x1;
  m_tn2Supported        = (info >> 9) & 0x1;
  m_tn4Supported        = (info >> 10) & 0x1;
  m_tn8Supported        = (info >> 11) & 0x1;
  m_rp1Supported        = (info >> 12) & 0x1;
  m_rp4Supported        = (info >> 13) & 0x1;
  m_rn2Supported        = (info >> 14) & 0x1;
  m_rn4Supported        = (info >> 15) & 0x1;
  m_rn8Supported        = info & 0x1;
  m_shortTrnSupported   = (info >> 1) & 0x1;
  m_longTrnSupported    = (info >> 2) & 0x1;
  m_maximumScMcs        = (info >> 3) & 0x1F;
  m_maximumOfdmMcs      = (info >> 8) & 0x1F;
  m_maximumPhyRate      = (info >> 13) & 0xFFF;
  m_scMcs6OfdmMcs5Supported = (info >> 25) & 0x1;
}

uint64_t
EdmgCapabilities::GetCoreCapabilities (void) const
{
  uint64_t val = 0;

  val |= uint64_t (m_ampduExponent & 0xF);
  val |= uint64_t (m_ampduMinimumSpacing  & 0x7) << 4;
  val |= uint64_t (m_tp1Supported & 0x1) << 7;
  val |= uint64_t (m_tp4Supported & 0x1) << 8;
  val |= uint64_t (m_tn2Supported & 0x1) << 9;
  val |= uint64_t (m_tn4Supported & 0x1) << 10;
  val |= uint64_t (m_tn8Supported & 0x1) << 11;
  val |= uint64_t (m_rp1Supported & 0x1) << 12;
  val |= uint64_t (m_rp4Supported & 0x1) << 13;
  val |= uint64_t (m_rn2Supported & 0x1) << 14;
  val |= uint64_t (m_rn4Supported & 0x1) << 15;
  val |= uint64_t (m_rn8Supported & 0x1);
  val |= uint64_t (m_shortTrnSupported & 0x1) << 1;
  val |= uint64_t (m_longTrnSupported & 0x1)  << 2;
  val |= uint64_t (m_maximumScMcs & 0x1F) << 3;
  val |= uint64_t (m_maximumOfdmMcs & 0x1F) << 8;
  val |= uint64_t (m_maximumPhyRate & 0xFFF) << 13;
  val |= uint64_t (m_scMcs6OfdmMcs5Supported & 0x1) << 25;

  return val;
}

/* Core Capabilities Info fields */
void
EdmgCapabilities::SetAmpduParameters (uint8_t ampduExponent, uint8_t minimumMpduSpacing)
{
  NS_ASSERT ((0 <= ampduExponent) && (ampduExponent <= 9));
  NS_ASSERT ((0 <= minimumMpduSpacing) && (minimumMpduSpacing <= 7));
  m_ampduExponent = ampduExponent;
  m_ampduMinimumSpacing = minimumMpduSpacing;
}

void
EdmgCapabilities::SetTrnParameters (bool TP1, bool TP4, bool TN2, bool TN4, bool TN8, bool RP1, bool RP4,
                                    bool RN2, bool RN4, bool RN8, bool shortTrn, bool longTrn)
{
  m_tp1Supported = TP1;
  m_tp4Supported = TP4;
  m_tn2Supported = TN2;
  m_tn4Supported = TN4;
  m_tn8Supported = TN8;
  m_rp1Supported = RP1;
  m_rp4Supported = RP4;
  m_rn2Supported = RN2;
  m_rn4Supported = RN4;
  m_rn8Supported = RN8;
  m_shortTrnSupported = shortTrn;
  m_longTrnSupported = longTrn;
}

void
EdmgCapabilities::SetSupportedMCS (uint8_t maximumScMcs, uint8_t maximumOfdmMcs, uint16_t maximumPhyRate,
                                   bool ScMcs6OfdmMcs5Supported)
{
  m_maximumScMcs = maximumScMcs;
  m_maximumOfdmMcs = maximumOfdmMcs;
  m_maximumPhyRate = maximumPhyRate;
  m_scMcs6OfdmMcs5Supported = ScMcs6OfdmMcs5Supported;
}

uint8_t
EdmgCapabilities::GetAmpduExponent (void) const
{
  return m_ampduExponent;
}
uint8_t
EdmgCapabilities::GetAmpduMinimumSpacing (void) const
{
  return m_ampduMinimumSpacing;
}

uint32_t
EdmgCapabilities::GetMaxAmpduLength (void) const
{
  return (1ul << (13 + m_ampduExponent)) - 1;
}

uint8_t
EdmgCapabilities::GetMaximumScMcs (void) const
{
  return m_maximumScMcs;
}
uint8_t
EdmgCapabilities::GetMaximumOfdmMcs (void) const
{
  return m_maximumOfdmMcs;
}
uint16_t
EdmgCapabilities::GetMaximumPhyRate (void) const
{
  return m_maximumPhyRate;
}
bool
EdmgCapabilities::GetScMcs6OfdmMcs5Supported (void) const
{
  return m_scMcs6OfdmMcs5Supported;
}

bool
EdmgCapabilities::GetTP1Supported (void) const
{
  return m_tp1Supported;
}
bool
EdmgCapabilities::GetTP4Supported (void) const
{
  return m_tp4Supported;
}
bool
EdmgCapabilities::GetTN2Supported (void) const
{
  return m_tn2Supported;
}
bool
EdmgCapabilities::GetTN4Supported (void) const
{
  return m_tn4Supported;
}
bool
EdmgCapabilities::GetTN8Supported (void) const
{
  return m_tn8Supported;
}

bool
EdmgCapabilities::GetRP1Supported (void) const
{
  return m_rp1Supported;
}

bool
EdmgCapabilities::GetRP4Supported (void) const
{
  return m_rp4Supported;
}

bool
EdmgCapabilities::GetRN2Supported (void) const
{
  return m_rn2Supported;
}

bool
EdmgCapabilities::GetRN4Supported (void) const
{
  return m_rn4Supported;
}

bool
EdmgCapabilities::GetRN8Supported (void) const
{
  return m_tn8Supported;
}

bool
EdmgCapabilities::GetShortTrnSupported (void) const
{
  return m_shortTrnSupported;
}

bool
EdmgCapabilities::GetLongTrnSupported (void) const
{
  return m_longTrnSupported;
}

void
EdmgCapabilities::AddSubElement (Ptr<WifiInformationElement> elem)
{
  m_map[elem->ElementId ()] = elem;
}

Ptr<WifiInformationElement>
EdmgCapabilities::GetSubElement (WifiInformationElementId id)
{
  WifiInformationSubelementMap::const_iterator it = m_map.find (id);
  if (it != m_map.end ())
    {
      return it->second;
    }
  else
    {
      return 0;
    }
}

WifiInformationSubelementMap
EdmgCapabilities::GetListOfSubElements (void) const
{
  return m_map;
}

ATTRIBUTE_HELPER_CPP (EdmgCapabilities)

std::ostream &
operator << (std::ostream &os, const EdmgCapabilities &EdmgCapabilities)
{
  os <<  EdmgCapabilities.GetCoreCapabilities ();
  return os;
}

std::istream &
operator >> (std::istream &is,EdmgCapabilities &EdmgCapabilities)
{
  uint64_t c1;
  is >> c1 ;

  EdmgCapabilities.SetCoreCapabilites (c1);
  return is;
}

} //namespace ns3
