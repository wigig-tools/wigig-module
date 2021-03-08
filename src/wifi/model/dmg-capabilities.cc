/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2020 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */

#include "ns3/address-utils.h"
#include "ns3/assert.h"
#include "ns3/log.h"

#include "dmg-capabilities.h"

NS_LOG_COMPONENT_DEFINE ("DmgCapabilities");

namespace ns3 {

DmgCapabilities::DmgCapabilities ()
  : m_staAddress (),
    m_aid (0),
    m_dmgStaBeamTrackingTimeLimit (0),
    m_maximumExtendedScTxMcs (EXTENDED_NONE),
    m_codeRate7_8_Tx (false),
    m_maximumExtendedScRxMcs (EXTENDED_NONE),
    m_codeRate7_8_Rx (false),
    m_maximumNumberOfBasicAMSDU (BASIC_AMSDU_NUMBER_NONE),
    m_maximumNumberOfShortAMSDU (SHORT_AMSDU_NUMBER_NONE),
    m_tddChannelAccessSupported (false),
    m_parametersAcrossRXChainsSupported (false),
    m_ppduStatisticsSupported (false),
    m_ldpcStatisticsSupported (false),
    m_scOFDM_StatisticsSupported (false),
    m_tddSynchronizationMode (false),
    m_reverseDirection (0),
    m_higherLayerTimerSynchronziation (0),
    m_tpc (0),
    m_spsh (0),
    m_RxDmgAntennas (0),
    m_fastLinkAdaption (0),
    m_sectorsNumber (0),
    m_rxssLength (0),
    m_dmgAntennaReciprocity (0),
    m_ampduExponent (5),
    m_ampduMinimumSpacing (0),
    m_baFlowControl (0),
    m_maximumScRxMcs (0),
    m_maximumOfdmRxMcs (0),
    m_maximumScTxMcs (0),
    m_maximumOfdmTxMcs (0),
    m_lowPower (0),
    m_codeRate13_16 (0),
    m_dtpSupported (0),
    m_appduSupported (0),
    m_heartbeat (0),
    m_supportsOtherAid (0),
    m_antennaPatternReciprocity (0),
    m_heartbeatElapsedIndication (0),
    m_GrantAckSupported (0),
    m_rxssTxRateSupported (0),
    m_tddti (0),
    m_pseudoStaticAllocations (0),
    m_pcpHandover (0),
    m_maxAssoicatedStaNumber (0),
    m_powerSource (0),
    m_decentralizedClustering (0),
    m_pcpForwarding (0),
    m_centralizedClustering (0)
{
}

WifiInformationElementId
DmgCapabilities::ElementId () const
{
  return IE_DMG_CAPABILITIES;
}

uint8_t
DmgCapabilities::GetInformationFieldSize () const
{
  //we should not be here if dmg is not supported
  return 24;
}

Buffer::Iterator
DmgCapabilities::Serialize (Buffer::Iterator i) const
{
  return WifiInformationElement::Serialize (i);
}

uint16_t
DmgCapabilities::GetSerializedSize () const
{
  return WifiInformationElement::GetSerializedSize ();
}

void
DmgCapabilities::SerializeInformationField (Buffer::Iterator start) const
{
  WriteTo (start, GetStaAddress ());
  start.WriteU8 (GetAID ());
  start.WriteHtolsbU64 (GetDmgStaCapabilityInfo ());
  start.WriteHtolsbU16 (GetDmgPcpApCapabilityInfo ());

  /* IEEE 802.11-2016 */
  start.WriteHtolsbU16 (m_dmgStaBeamTrackingTimeLimit);
  uint8_t extendedCapabilities = 0;
  extendedCapabilities |= m_maximumExtendedScTxMcs & 0x7;
  extendedCapabilities |= (m_codeRate7_8_Tx & 0x1) << 3;
  extendedCapabilities |= (m_maximumExtendedScRxMcs & 0x7) << 4;
  extendedCapabilities |= (m_codeRate7_8_Rx & 0x1) << 7;
  start.WriteU8 (extendedCapabilities);
  start.WriteU8 (m_maximumNumberOfBasicAMSDU);
  start.WriteU8 (m_maximumNumberOfShortAMSDU);

  /* IEEE 802.11ay D4.0 */
  uint16_t tddCapability = 0;
  tddCapability |= m_tddChannelAccessSupported & 0x1;
  tddCapability |= (m_parametersAcrossRXChainsSupported & 0x1) << 1;
  tddCapability |= (m_ppduStatisticsSupported & 0x1) << 2;
  tddCapability |= (m_ldpcStatisticsSupported & 0x1) << 3;
  tddCapability |= (m_scOFDM_StatisticsSupported & 0x1) << 4;
  tddCapability |= (m_tddSynchronizationMode & 0x1) << 5;

  start.WriteHtolsbU16 (tddCapability);
}

uint8_t
DmgCapabilities::DeserializeInformationField (Buffer::Iterator start,
                                              uint8_t length)
{
  Buffer::Iterator i = start;
  Mac48Address staAddress;

  ReadFrom (i, staAddress);
  uint8_t aid = i.ReadU8 ();
  uint64_t staCapability = i.ReadLsbtohU64 ();
  uint16_t apCapability = i.ReadLsbtohU16();

  SetStaAddress (staAddress);
  SetAID (aid);
  SetDmgStaCapabilityInfo (staCapability);
  SetDmgPcpApCapabilityInfo (apCapability);

  m_dmgStaBeamTrackingTimeLimit = i.ReadLsbtohU16 ();
  uint8_t extendedCapabilities = i.ReadU8 ();
  m_maximumExtendedScTxMcs = static_cast<EXTENDED_MCS_NAME> (extendedCapabilities & 0x7);
  m_codeRate7_8_Tx = (extendedCapabilities  >> 3) & 0x1;
  m_maximumExtendedScRxMcs = (extendedCapabilities >> 4) & 0x7;
  m_codeRate7_8_Rx = (extendedCapabilities >> 7) & 0x1;
  m_maximumNumberOfBasicAMSDU = static_cast<MAXIMUM_BASIC_AMSDU_NUMBER> (i.ReadU8 ());
  m_maximumNumberOfShortAMSDU = static_cast<MAXIMUM_SHORT_AMSDU_NUMBER> (i.ReadU8 ());

  /* IEEE 802.11ay D4.0 */
  uint16_t tddCapability = i.ReadLsbtohU16 ();
  m_tddChannelAccessSupported = tddCapability & 0x1;
  m_parametersAcrossRXChainsSupported = (tddCapability >> 1) & 0x1;
  m_ppduStatisticsSupported = (tddCapability >> 2) & 0x1;
  m_ldpcStatisticsSupported = (tddCapability >> 3) & 0x1;
  m_scOFDM_StatisticsSupported = (tddCapability >> 4) & 0x1;
  m_tddSynchronizationMode = (tddCapability >> 5) & 0x1;

  return length;
}

void
DmgCapabilities::SetStaAddress (Mac48Address address)
{
  m_staAddress = address;
}

Mac48Address
DmgCapabilities::GetStaAddress (void) const
{
  return m_staAddress;
}

void
DmgCapabilities::SetAID (uint8_t aid)
{
  m_aid = aid;
}

uint8_t
DmgCapabilities::GetAID (void) const
{
  return m_aid;
}

void
DmgCapabilities::SetDmgStaCapabilityInfo (uint64_t info)
{
  m_reverseDirection = info & 0x1;
  m_higherLayerTimerSynchronziation = (info >> 1) & 0x1;
  m_tpc = (info >> 2) & 0x1;
  m_spsh = (info >> 3) & 0x1;
  m_RxDmgAntennas = (info >> 4) & 0x3;
  m_fastLinkAdaption = (info >> 6) & 0x1;
  m_sectorsNumber = (info >> 7) & 0x7F;
  m_rxssLength = (info >> 14) & 0x3F;
  m_dmgAntennaReciprocity = (info >> 20) & 0x1;

  m_ampduExponent = (info >> 21) & 0x7;
  m_ampduMinimumSpacing= (info >> 24) & 0x7;

  m_baFlowControl = (info >> 27) & 0x1;

  m_maximumScRxMcs = (info >> 28) & 0x1F;
  m_maximumOfdmRxMcs = (info >> 32) & 0x1F;
  m_maximumScTxMcs = (info >> 37) & 0x1F;
  m_maximumOfdmTxMcs = (info >> 42) & 0x1F;
  m_lowPower = (info >> 47) & 0x1;
  m_codeRate13_16 = (info >> 48) & 0x1 ;

  m_dtpSupported = (info >> 52) & 0x1;
  m_appduSupported = (info >> 53) & 0x1;
  m_heartbeat = (info >> 54) & 0x1;
  m_supportsOtherAid = (info >> 55) & 0x1;
  m_antennaPatternReciprocity = (info >> 56) & 0x1;
  m_heartbeatElapsedIndication = (info >> 57) & 0x7;
  m_GrantAckSupported = (info >> 60) & 0x1;
  m_rxssTxRateSupported = (info >> 61) & 0x1;
}

uint64_t
DmgCapabilities::GetDmgStaCapabilityInfo (void) const
{
  uint64_t val = 0;

  val |= uint64_t (m_reverseDirection & 0x1);
  val |= uint64_t (m_higherLayerTimerSynchronziation & 0x1) << 1;
  val |= uint64_t (m_tpc & 0x1) <<  2;
  val |= uint64_t (m_spsh & 0x1) << 3;
  val |= uint64_t (m_RxDmgAntennas & 0x3) << 4;
  val |= uint64_t (m_fastLinkAdaption & 0x1) << 6;
  val |= uint64_t (m_sectorsNumber & 0x7F) << 7;
  val |= uint64_t (m_rxssLength & 0x3F) << 14;
  val |= uint64_t (m_dmgAntennaReciprocity & 0x1) << 20;

  val |= uint64_t (m_ampduExponent & 0x7) << 21;
  val |= uint64_t (m_ampduMinimumSpacing & 0x7) << 24;

  val |= uint64_t (m_baFlowControl & 0x1) << 27;

  val |= uint64_t (m_maximumScRxMcs & 0x1F) << 28;
  val |= uint64_t (m_maximumOfdmRxMcs & 0x1F) << 32;
  val |= uint64_t (m_maximumScTxMcs & 0x1F) << 37;
  val |= uint64_t (m_maximumOfdmTxMcs & 0x1F) << 42;
  val |= uint64_t (m_lowPower & 0x1) << 47;
  val |= uint64_t (m_codeRate13_16 & 0x1) << 48;

  val |= uint64_t (m_dtpSupported & 0x1) << 52;
  val |= uint64_t (m_appduSupported & 0x1) << 53;
  val |= uint64_t (m_heartbeat & 0x1) << 54;
  val |= uint64_t (m_supportsOtherAid & 0x1) << 55;
  val |= uint64_t (m_antennaPatternReciprocity & 0x1) << 56;
  val |= uint64_t (m_heartbeatElapsedIndication & 0x7) << 57;
  val |= uint64_t (m_GrantAckSupported  & 0x1) << 60;
  val |= uint64_t (m_rxssTxRateSupported & 0x1) << 61;

  return val;
}

void
DmgCapabilities::SetDmgPcpApCapabilityInfo (uint16_t info)
{
  m_tddti = info & 0x1;
  m_pseudoStaticAllocations = (info >> 1) & 0x1;
  m_pcpHandover = (info >> 2) & 0x1;
  m_maxAssoicatedStaNumber = (info >> 3) & 0xFF;
  m_powerSource = (info >> 11) & 0x1;
  m_decentralizedClustering = (info >> 12) & 0x1;
  m_pcpForwarding = (info >> 13) & 0x1;
  m_centralizedClustering = (info >> 14) & 0x1;
}

uint16_t
DmgCapabilities::GetDmgPcpApCapabilityInfo (void) const
{
  uint16_t val = 0;

  val |= m_tddti & 0x1;
  val |= (m_pseudoStaticAllocations & 0x1) << 1;
  val |= (m_pcpHandover & 0x1) <<  2;
  val |= m_maxAssoicatedStaNumber << 3;
  val |= (m_powerSource & 0x1) << 11;
  val |= (m_decentralizedClustering & 0x1) << 12;
  val |= (m_pcpForwarding & 0x1) << 13;
  val |= (m_centralizedClustering & 0x1) << 14;

  return val;
}

void
DmgCapabilities::SetDmgStaBeamTrackingTimeLimit (uint16_t limit)
{
  m_dmgStaBeamTrackingTimeLimit = limit;
}

void
DmgCapabilities::SetMaximumExtendedScTxMcs (EXTENDED_MCS_NAME maximum)
{
  m_maximumExtendedScTxMcs = maximum;
}

void
DmgCapabilities::SetCodeRate7_8_Tx (bool value)
{
  m_codeRate7_8_Tx = value;
}

void
DmgCapabilities::SetMaximumExtendedScRxMcs (uint8_t maximum)
{
  m_maximumExtendedScRxMcs = maximum;
}

void
DmgCapabilities::SetCodeRate7_8_Rx (bool value)
{
  m_codeRate7_8_Rx = value;
}

void
DmgCapabilities::SetMaximumNumberOfBasicAMSDU (MAXIMUM_BASIC_AMSDU_NUMBER maximum)
{
  m_maximumNumberOfBasicAMSDU = maximum;
}

void
DmgCapabilities::SetMaximumNumberOfShortAMSDU (MAXIMUM_SHORT_AMSDU_NUMBER maximum)
{
  m_maximumNumberOfShortAMSDU = maximum;
}

void
DmgCapabilities::SetTDD_ChannelAccessSupported (bool supported)
{
  m_tddChannelAccessSupported = supported;
}

void
DmgCapabilities::SetParameters_Across_RX_ChainsSupported (bool supported)
{
  m_parametersAcrossRXChainsSupported = supported;
}

void
DmgCapabilities::SetPPDU_StatisticsSupported (bool supported)
{
  m_ppduStatisticsSupported = supported;
}

void
DmgCapabilities::SetLDPC_StatisticsSupported (bool supported)
{
  m_ldpcStatisticsSupported = supported;
}

void
DmgCapabilities::SetSC_OFDM_StatisticsSupported (bool supported)
{
  m_scOFDM_StatisticsSupported = supported;
}

void
DmgCapabilities::SetTDD_SynchronizationMode (bool mode)
{
  m_tddSynchronizationMode = mode;
}

uint16_t
DmgCapabilities::GetDmgStaBeamTrackingTimeLimit (void) const
{
  return m_dmgStaBeamTrackingTimeLimit;
}

EXTENDED_MCS_NAME
DmgCapabilities::GetMaximumExtendedScTxMcs (void) const
{
  return m_maximumExtendedScTxMcs;
}

bool
DmgCapabilities::GetCodeRate7_8_Tx (void) const
{
  return m_codeRate7_8_Tx;
}

uint8_t DmgCapabilities::GetMaximumExtendedScRxMcs (void) const
{
  return m_maximumExtendedScRxMcs;
}

bool DmgCapabilities::GetCodeRate7_8_Rx (void) const
{
  return m_codeRate7_8_Rx;
}

MAXIMUM_BASIC_AMSDU_NUMBER
DmgCapabilities::GetMaximumNumberOfBasicAMSDU (void) const
{
  return m_maximumNumberOfBasicAMSDU;
}

MAXIMUM_SHORT_AMSDU_NUMBER
DmgCapabilities::GetMaximumNumberOfShortAMSDU (void) const
{
  return m_maximumNumberOfShortAMSDU;
}

bool
DmgCapabilities::GetTDD_ChannelAccessSupported (void) const
{
  return m_tddChannelAccessSupported;
}

bool
DmgCapabilities::GetParameters_AcrossRXChainsSupported (void) const
{
  return m_parametersAcrossRXChainsSupported;
}

bool
DmgCapabilities::GetPPDU_StatisticsSupported (void) const
{
  return m_ppduStatisticsSupported;
}

bool
DmgCapabilities::GetLDPC_StatisticsSupported (void) const
{
  return m_ldpcStatisticsSupported;
}

bool
DmgCapabilities::GetSC_OFDM_StatisticsSupported (void) const
{
  return m_scOFDM_StatisticsSupported;
}

bool
DmgCapabilities::GetTDD_SynchronizationMode (void) const
{
  return m_tddSynchronizationMode;
}

void
DmgCapabilities::SetReverseDirection (bool value)
{
  m_reverseDirection = value;
}

void
DmgCapabilities::SetHigherLayerTimerSynchronization (bool value)
{
  m_higherLayerTimerSynchronziation = value;
}

void
DmgCapabilities::SetTPC (bool value)
{
  m_tpc = value;
}

void
DmgCapabilities::SetSPSH (bool value)
{
  m_spsh = value;
}

void
DmgCapabilities::SetNumberOfRxDmgAntennas (uint8_t number)
{
  NS_ASSERT ((1 <= number) && (number <= 4));
  m_RxDmgAntennas = number - 1;
}

void
DmgCapabilities::SetFastLinkAdaption (bool value)
{
  m_fastLinkAdaption = value;
}

void
DmgCapabilities::SetNumberOfSectors (uint8_t number)
{
  NS_ASSERT ((1 <= number) && (number <= 128));
  m_sectorsNumber = number - 1;
}

void
DmgCapabilities::SetRXSSLength (uint8_t length)
{
//  NS_ASSERT ((2 <= length) && (length <= 128));
  m_rxssLength = (length/2)-1;
}

void
DmgCapabilities::SetDmgAntennaReciprocity (bool reciprocity)
{
  m_dmgAntennaReciprocity = reciprocity;
}

void
DmgCapabilities::SetAmpduParameters (uint8_t ampduExponent, uint8_t minimumMpduSpacing)
{
  NS_ASSERT ((0 <= ampduExponent) && (ampduExponent <= 5));
  NS_ASSERT ((0 <= minimumMpduSpacing) && (minimumMpduSpacing <= 7));
  m_ampduExponent = ampduExponent;
  m_ampduMinimumSpacing = minimumMpduSpacing;
}

void
DmgCapabilities::SetBaFlowControl (bool value)
{
  m_baFlowControl = value;
}

void
DmgCapabilities::SetSupportedMCS (uint8_t maximumScRxMcs, uint8_t maximumOfdmRxMcs, uint8_t maximumScTxMcs,
                                  uint8_t maximumOfdmTxMcs, bool lowPower, bool codeRate13_16)
{
  m_maximumScRxMcs = maximumScRxMcs;
  m_maximumOfdmRxMcs = maximumOfdmRxMcs;
  m_maximumScTxMcs = maximumScTxMcs;
  m_maximumOfdmTxMcs = maximumOfdmTxMcs;
  m_lowPower = lowPower;
  m_codeRate13_16 = codeRate13_16;
}

void
DmgCapabilities::SetDtpSupported (bool value)
{
  m_dtpSupported = value;
}

void
DmgCapabilities::SetAppduSupported (bool value)
{
  m_appduSupported = value;
}

void
DmgCapabilities::SetHeartbeat (bool value)
{
  m_heartbeat = value;
}

void
DmgCapabilities::SetSupportsOtherAid (bool value)
{
  m_supportsOtherAid = value;
}

void
DmgCapabilities::SetAntennaPatternReciprocity (bool value)
{
  m_antennaPatternReciprocity = value;
}

void
DmgCapabilities::SetHeartbeatElapsedIndication (uint8_t indication)
{
  m_heartbeatElapsedIndication = indication;
}

void
DmgCapabilities::SetGrantAckSupported (bool value)
{
  m_GrantAckSupported = value;
}

void
DmgCapabilities::SetRXSSTxRateSupported (bool value)
{
  m_rxssTxRateSupported = value;
}

bool
DmgCapabilities::GetReverseDirection (void) const
{
  return m_reverseDirection;
}

bool
DmgCapabilities::GetHigherLayerTimerSynchronization (void) const
{
  return m_higherLayerTimerSynchronziation;
}

bool
DmgCapabilities::GetTPC (void) const
{
  return m_tpc;
}

bool
DmgCapabilities::GetSPSH (void) const
{
  return m_spsh;
}

uint8_t
DmgCapabilities::GetNumberOfRxDmgAntennas (void) const
{
  return m_RxDmgAntennas + 1;
}

bool
DmgCapabilities::GetFastLinkAdaption (void) const
{
  return m_fastLinkAdaption;
}

uint8_t
DmgCapabilities::GetNumberOfSectors (void) const
{
  return m_sectorsNumber + 1;
}

uint8_t
DmgCapabilities::GetRXSSLength (void) const
{
  return (m_rxssLength + 1)*2;
}

bool
DmgCapabilities::GetDmgAntennaReciprocity (void) const
{
  return m_dmgAntennaReciprocity;
}

uint8_t
DmgCapabilities::GetAmpduExponent (void) const
{
  return m_ampduExponent;
}

uint8_t
DmgCapabilities::GetAmpduMinimumSpacing (void) const
{
  return m_ampduMinimumSpacing;
}

uint32_t
DmgCapabilities::GetMaxAmpduLength (void) const
{
  return (1ul << (13 + m_ampduExponent)) - 1;
}

bool
DmgCapabilities::GetBaFlowControl (void) const
{
  return m_baFlowControl;
}

uint8_t
DmgCapabilities::GetMaximumScRxMcs (void) const
{
  return m_maximumScRxMcs;
}

uint8_t
DmgCapabilities::GetMaximumOfdmRxMcs (void) const
{
  return m_maximumOfdmRxMcs;
}

uint8_t
DmgCapabilities::GetMaximumScTxMcs (void) const
{
  return m_maximumScTxMcs;
}

uint8_t
DmgCapabilities::GetMaximumOfdmTxMcs (void) const
{
  return m_maximumOfdmTxMcs;
}

bool
DmgCapabilities::GetLowPowerScSupported (void) const
{
  return m_lowPower;
}

bool
DmgCapabilities::GetCodeRate13_16Suppoted (void) const
{
  return m_codeRate13_16;
}

bool
DmgCapabilities::GetDtpSupported (void) const
{
  return m_dtpSupported;
}

bool
DmgCapabilities::GetAppduSupported (void) const
{
  return m_appduSupported;
}

bool
DmgCapabilities::GetHeartbeat (void) const
{
  return m_heartbeat;
}

bool
DmgCapabilities::GetSupportsOtherAid (void) const
{
  return m_supportsOtherAid;
}

bool
DmgCapabilities::GetAntennaPatternReciprocity (void) const
{
  return m_antennaPatternReciprocity;
}

uint8_t
DmgCapabilities::GetHeartbeatElapsedIndication (void) const
{
  return m_heartbeatElapsedIndication;
}

bool
DmgCapabilities::GetGrantAckSupported (void) const
{
  return m_GrantAckSupported;
}

bool
DmgCapabilities::GetRXSSTxRateSupported (void) const
{
  return m_rxssTxRateSupported;
}

/* DMG PCP/AP Capability Info fields */
void
DmgCapabilities::SetTDDTI (bool tddti)
{
  m_tddti = tddti;
}

void
DmgCapabilities::SetPseudoStaticAllocations (bool pseudoStatic)
{
  m_pseudoStaticAllocations = pseudoStatic;
}

void
DmgCapabilities::SetPcpHandover (bool handover)
{
  m_pcpHandover = handover;
}

void
DmgCapabilities::SetMaxAssociatedStaNumber (uint8_t max)
{
  m_maxAssoicatedStaNumber = max;
}

void
DmgCapabilities::SetPowerSource (bool powerSource)
{
  m_powerSource = powerSource;
}

void
DmgCapabilities::SetDecentralizedClustering (bool decentralized)
{
  m_decentralizedClustering = decentralized;
}

void
DmgCapabilities::SetPcpForwarding (bool forwarding)
{
  m_pcpForwarding = forwarding;
}

void
DmgCapabilities::SetCentralizedClustering (bool centralized)
{
  m_centralizedClustering = centralized;
}

bool
DmgCapabilities::GetTDDTI (void) const
{
  return m_tddti;
}

bool
DmgCapabilities::GetPseudoStaticAllocations (void) const
{
  return m_pseudoStaticAllocations;
}

bool
DmgCapabilities::GetPcpHandover (void) const
{
  return m_pcpHandover;
}

uint8_t
DmgCapabilities::GetMaxAssociatedStaNumber (void) const
{
  return m_maxAssoicatedStaNumber;
}

bool
DmgCapabilities::GetPowerSource (void) const
{
  return m_powerSource;
}

bool
DmgCapabilities::GetDecentralizedClustering (void) const
{
  return m_decentralizedClustering;
}

bool
DmgCapabilities::GetPcpForwarding (void) const
{
  return m_pcpForwarding;
}

bool
DmgCapabilities::GetCentralizedClustering (void) const
{
  return m_centralizedClustering;
}

ATTRIBUTE_HELPER_CPP (DmgCapabilities)

std::ostream &
operator << (std::ostream &os, const DmgCapabilities &dmgCapabilities)
{
  os <<  dmgCapabilities.GetAID () << "|" <<
         dmgCapabilities.GetDmgStaCapabilityInfo () << "|" <<
         dmgCapabilities.GetDmgPcpApCapabilityInfo ();
  return os;
}

std::istream &
operator >> (std::istream &is,DmgCapabilities &dmgCapabilities)
{
  uint8_t c1;
  uint64_t c2;
  uint16_t c3;
  is >> c1 >> c2 >> c3;

  dmgCapabilities.SetAID (c1);
  dmgCapabilities.SetDmgStaCapabilityInfo (c2);
  dmgCapabilities.SetDmgPcpApCapabilityInfo (c3);

  return is;
}

} //namespace ns3
