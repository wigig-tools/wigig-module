/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */

#include "ns3/address-utils.h"
#include "ns3/assert.h"
#include "ns3/log.h"

#include "dmg-capabilities.h"

NS_LOG_COMPONENT_DEFINE ("DmgCapabilities");

namespace ns3 {

DmgCapabilities::DmgCapabilities ()
  : m_reverseDirection (0),
    m_higherLayerTimerSynchronziation (0),
    m_tpc (0),
    m_spsh (0),
    m_RxDmgAntennas (0),
    m_fastLinkAdaption (0),
    m_sectorsNumber (0),
    m_rxssLength (0),
    m_dmgAntennaReciprocity (0),
    m_ampduMaximumLength (0),
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
    m_RxssTxRateSupported (0),
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
  return 17;
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
}

uint8_t
DmgCapabilities::DeserializeInformationField (Buffer::Iterator start,
                                              uint8_t length)
{
  Buffer::Iterator i = start;
  Mac48Address staAddress;

  ReadFrom (i, staAddress);
  uint8_t aid = i.ReadU8 ();
  uint16_t staCapability = i.ReadLsbtohU64 ();
  uint64_t apCapability = i.ReadLsbtohU16();

  SetStaAddress (staAddress);
  SetAID (aid);
  SetDmgStaCapabilityInfo (staCapability);
  SetDmgPcpApCapabilityInfo (apCapability);

  return length;
}

void
DmgCapabilities::SetStaAddress (Mac48Address address)
{
  m_staAddress = address;
}

Mac48Address
DmgCapabilities::GetStaAddress () const
{
  return m_staAddress;
}

void
DmgCapabilities::SetAID (uint8_t aid)
{
  m_aid = aid;
}

uint8_t
DmgCapabilities::GetAID () const
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

  m_ampduMaximumLength = (info >> 21) & 0x7;
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
  m_RxssTxRateSupported = (info >> 61) & 0x1;
}

uint64_t
DmgCapabilities::GetDmgStaCapabilityInfo () const
{
  uint64_t val = 0;

  val |= uint64_t(m_reverseDirection & 0x1);
  val |= uint64_t(m_higherLayerTimerSynchronziation & 0x1) << 1;
  val |= uint64_t(m_tpc & 0x1) <<  2;
  val |= uint64_t(m_spsh & 0x1) << 3;
  val |= uint64_t(m_RxDmgAntennas & 0x3) << 4;
  val |= uint64_t(m_fastLinkAdaption & 0x1) << 6;
  val |= uint64_t(m_sectorsNumber & 0x7F) << 7;
  val |= uint64_t(m_rxssLength & 0x3F) << 14;
  val |= uint64_t(m_dmgAntennaReciprocity & 0x1) << 20;

  val |= uint64_t(m_ampduMaximumLength & 0x7) << 21;
  val |= uint64_t(m_ampduMinimumSpacing & 0x7) << 24;

  val |= uint64_t(m_baFlowControl & 0x1) << 27;

  val |= uint64_t(m_maximumScRxMcs & 0x1F) << 28;
  val |= uint64_t(m_maximumOfdmRxMcs & 0x1F) << 32;
  val |= uint64_t(m_maximumScTxMcs & 0x1F) << 37;
  val |= uint64_t(m_maximumOfdmTxMcs & 0x1F) << 42;
  val |= uint64_t(m_lowPower & 0x1) << 47;
  val |= uint64_t(m_codeRate13_16 & 0x1) << 48;

  val |= uint64_t(m_dtpSupported & 0x1) << 52;
  val |= uint64_t(m_appduSupported & 0x1) << 53;
  val |= uint64_t(m_heartbeat & 0x1) << 54;
  val |= uint64_t(m_supportsOtherAid & 0x1) << 55;
  val |= uint64_t(m_antennaPatternReciprocity & 0x1) << 56;
  val |= uint64_t(m_heartbeatElapsedIndication & 0x7) << 57;
  val |= uint64_t(m_GrantAckSupported  & 0x1) << 60;
  val |= uint64_t(m_RxssTxRateSupported & 0x1) << 61;

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
DmgCapabilities::GetDmgPcpApCapabilityInfo () const
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
DmgCapabilities::SetRxssLength (uint8_t length)
{
  NS_ASSERT ((2 <= length) && (length <= 128) && (length % 2 == 0));
  m_rxssLength = length/2 - 1;
}

void
DmgCapabilities::SetDmgAntennaReciprocity (bool reciprocity)
{
  m_dmgAntennaReciprocity = reciprocity;
}

void
DmgCapabilities::SetAmpduParameters (uint8_t maximumLength, uint8_t minimumMpduSpacing)
{
  NS_ASSERT ((0 <= maximumLength) && (maximumLength <= 5));
  NS_ASSERT ((0 <= minimumMpduSpacing) && (minimumMpduSpacing <= 7));
  m_ampduMaximumLength = maximumLength;
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
  m_RxssTxRateSupported = value;
}

bool
DmgCapabilities::GetReverseDirection () const
{
  return m_reverseDirection;
}

bool
DmgCapabilities::GetHigherLayerTimerSynchronization () const
{
  return m_higherLayerTimerSynchronziation;
}

bool
DmgCapabilities::GetTPC () const
{
  return m_tpc;
}

bool
DmgCapabilities::GetSPSH () const
{
  return m_spsh;
}

uint8_t
DmgCapabilities::GetNumberOfRxDmgAntennas () const
{
  return m_RxDmgAntennas + 1;
}

bool
DmgCapabilities::GetFastLinkAdaption () const
{
  return m_fastLinkAdaption;
}

uint8_t
DmgCapabilities::GetNumberOfSectors () const
{
  return m_sectorsNumber + 1;
}

uint8_t
DmgCapabilities::GetRxssLength () const
{
  return (m_rxssLength + 1) * 2;
}

bool
DmgCapabilities::GetDmgAntennaReciprocity () const
{
  return m_dmgAntennaReciprocity;
}

uint8_t
DmgCapabilities::GetAmpduMaximumLength () const
{
  return m_ampduMaximumLength;
}

uint8_t
DmgCapabilities::GetAmpduMinimumSpacing () const
{
  return m_ampduMinimumSpacing;
}

bool
DmgCapabilities::GetBaFlowControl () const
{
  return m_baFlowControl;
}

uint8_t
DmgCapabilities::GetMaximumScRxMcs () const
{
  return m_maximumScRxMcs;
}

uint8_t
DmgCapabilities::GetMaximumOfdmRxMcs () const
{
  return m_maximumOfdmRxMcs;
}

uint8_t
DmgCapabilities::GetMaximumScTxMcs () const
{
  return m_maximumScTxMcs;
}

uint8_t
DmgCapabilities::GetMaximumOfdmTxMcs () const
{
  return m_maximumOfdmTxMcs;
}

bool
DmgCapabilities::GetLowPowerScSupported () const
{
  return m_lowPower;
}

bool
DmgCapabilities::GetCodeRate13_16Suppoted () const
{
  return m_codeRate13_16;
}

bool
DmgCapabilities::GetDtpSupported () const
{
  return m_dtpSupported;
}

bool
DmgCapabilities::GetAppduSupported () const
{
  return m_appduSupported;
}

bool
DmgCapabilities::GetHeartbeat () const
{
  return m_heartbeat;
}

bool
DmgCapabilities::GetSupportsOtherAid () const
{
  return m_supportsOtherAid;
}

bool
DmgCapabilities::GetAntennaPatternReciprocity () const
{
  return m_antennaPatternReciprocity;
}

uint8_t
DmgCapabilities::GetHeartbeatElapsedIndication () const
{
  return m_heartbeatElapsedIndication;
}

bool
DmgCapabilities::GetGrantAckSupported () const
{
  return m_GrantAckSupported;
}

bool
DmgCapabilities::GetRXSSTxRateSupported () const
{
  return m_RxssTxRateSupported;
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
DmgCapabilities::GetTDDTI () const
{
  return m_tddti;
}

bool
DmgCapabilities::GetPseudoStaticAllocations () const
{
  return m_pseudoStaticAllocations;
}

bool
DmgCapabilities::GetPcpHandover () const
{
  return m_pcpHandover;
}

uint8_t
DmgCapabilities::GetMaxAssociatedStaNumber () const
{
  return m_maxAssoicatedStaNumber;
}

bool
DmgCapabilities::GetPowerSource () const
{
  return m_powerSource;
}

bool
DmgCapabilities::GetDecentralizedClustering () const
{
  return m_decentralizedClustering;
}

bool
DmgCapabilities::GetPcpForwarding () const
{
  return m_pcpForwarding;
}

bool
DmgCapabilities::GetCentralizedClustering () const
{
  return m_centralizedClustering;
}

ATTRIBUTE_HELPER_CPP (DmgCapabilities)

std::ostream &
operator << (std::ostream &os, const DmgCapabilities &DmgCapabilities)
{
  os <<  DmgCapabilities.GetAID () << "|" <<
         DmgCapabilities.GetDmgStaCapabilityInfo () << "|" <<
         DmgCapabilities.GetDmgPcpApCapabilityInfo ();
  return os;
}

std::istream &
operator >> (std::istream &is,DmgCapabilities &DmgCapabilities)
{
  uint8_t c1;
  uint64_t c2;
  uint16_t c3;
  is >> c1 >> c2 >> c3;

  DmgCapabilities.SetAID (c1);
  DmgCapabilities.SetDmgStaCapabilityInfo (c2);
  DmgCapabilities.SetDmgPcpApCapabilityInfo (c3);

  return is;
}

} //namespace ns3
