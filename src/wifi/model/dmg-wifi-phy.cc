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
 * Authors: Hany Assasa <hany.assasa@gmail.com>
 *          Nina Grosheva <nina.grosheva@gmail.com>
 */

#include "ns3/simulator.h"
#include "ns3/boolean.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/mobility-model.h"
#include "ns3/random-variable-stream.h"
#include "ns3/error-model.h"
#include "wifi-phy.h"
#include "ampdu-tag.h"
#include "dmg-wifi-phy.h"
#include "dmg-wifi-channel.h"
#include "wifi-utils.h"
#include "frame-capture-model.h"
#include "preamble-detection-model.h"
#include "wifi-radio-energy-model.h"
#include "error-rate-model.h"
#include "wifi-net-device.h"
#include "mpdu-aggregator.h"
#include "wifi-psdu.h"
#include "wifi-ppdu.h"
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DmgWifiPhy");

NS_OBJECT_ENSURE_REGISTERED (DmgWifiPhy);

TypeId
DmgWifiPhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DmgWifiPhy")
    .SetParent<WifiPhy> ()
    .SetGroupName ("Wifi")
    .AddConstructor<DmgWifiPhy> ()
    .AddAttribute ("PrimaryChannelNumber", "The primary 2.16 GHz channel number.",
                   UintegerValue (2),
                   MakeUintegerAccessor (&DmgWifiPhy::SetPrimaryChannelNumber,
                                         &DmgWifiPhy::GetPrimaryChannelNumber),
                   MakeUintegerChecker<uint8_t> (1, 8))
    .AddAttribute ("SupportOfdmPhy", "Whether the DMG STA supports OFDM PHY layer.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&DmgWifiPhy::SetSupportOfdmPhy,
                                        &DmgWifiPhy::GetSupportOfdmPhy),
                   MakeBooleanChecker ())
    .AddAttribute ("SupportLpScPhy", "Whether the DMG STA supports LP-SC PHY layer.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&DmgWifiPhy::SetSupportLpScPhy,
                                        &DmgWifiPhy::GetSupportLpScPhy),
                   MakeBooleanChecker ())
    .AddAttribute ("MaxScTxMcs", "The maximum supported MCS for transmission by SC PHY.",
                   UintegerValue (12),
                   MakeUintegerAccessor (&DmgWifiPhy::m_maxScTxMcs),
                   MakeUintegerChecker<uint8_t> (4, 12))
    .AddAttribute ("MaxScRxMcs", "The maximum supported MCS for reception by SC PHY.",
                   UintegerValue (12),
                   MakeUintegerAccessor (&DmgWifiPhy::m_maxScRxMcs),
                   MakeUintegerChecker<uint8_t> (4, 12))
    .AddAttribute ("MaxOfdmTxMcs", "The maximum supported MCS for transmission by OFDM PHY.",
                   UintegerValue (24),
                   MakeUintegerAccessor (&DmgWifiPhy::m_maxOfdmTxMcs),
                   MakeUintegerChecker<uint8_t> (18, 24))
    .AddAttribute ("MaxOfdmRxMcs", "The maximum supported MCS for reception by OFDM PHY.",
                   UintegerValue (24),
                   MakeUintegerAccessor (&DmgWifiPhy::m_maxOfdmRxMcs),
                   MakeUintegerChecker<uint8_t> (18, 24))
    /* EDMG PHY Layer Capabilities */
    .AddAttribute ("MaxScMcs", "Maximum supported MCS by EDMG SC PHY",
                   UintegerValue (21),
                   MakeUintegerAccessor (&DmgWifiPhy::m_maxScMcs),
                   MakeUintegerChecker<uint8_t> (4, 21))
    .AddAttribute ("MaxOfdmMcs", "Maximum supported MCS by EDMG OFDM PHY",
                   UintegerValue (20),
                   MakeUintegerAccessor (&DmgWifiPhy::m_maxOfdmMcs),
                   MakeUintegerChecker<uint8_t> (1, 20))
    .AddAttribute ("MaxPhyRate", "Maximum supported receive PHY data rate",
                   UintegerValue (380),
                   MakeUintegerAccessor (&DmgWifiPhy::m_maxPhyRate),
                   MakeUintegerChecker<uint16_t> ())
    /* EDMG MIMO Capabilities */
    .AddAttribute ("SupportSuMimo", "Whether the EDMG STA supports SU-MIMO BF protocol and data transmission.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&DmgWifiPhy::m_suMimoSupported),
                   MakeBooleanChecker ())
    .AddAttribute ("SupportMuMimo", "Whether the EDMG STA supports MU-MIMO BF protocol and data transmission.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&DmgWifiPhy::m_muMimoSupported),
                   MakeBooleanChecker ())
  ;
  return tid;
}

void
DmgWifiPhy::SetPrimaryChannelNumber (uint8_t primaryCh)
{
  NS_LOG_FUNCTION (this << uint16_t (primaryCh));
  m_primaryChannel = primaryCh;
  SetChannelConfiguration ();
}

uint8_t
DmgWifiPhy::GetPrimaryChannelNumber (void) const
{
  return m_primaryChannel;
}

void
DmgWifiPhy::SetChannelWidth (uint16_t channelWidth)
{
  NS_LOG_FUNCTION (this << channelWidth);
  WifiPhy::SetChannelWidth (channelWidth);
  SetChannelConfiguration ();
}

void
DmgWifiPhy::SetChannelConfiguration (void)
{
  NS_LOG_FUNCTION (this);
  if (m_isConstructed)
    {
      for (const auto &it : edmgChannelConfigurations)
        {
          if ((it.primayChannel == m_primaryChannel) && (it.channelWidth == m_channelWidth))
            {
              m_channelConfiguration = it;
              return;
            }
        }
      NS_FATAL_ERROR ("Wrong channel configuration given Primary="
                      << uint16_t (m_primaryChannel) << ", BW=" << m_channelWidth <<
                      ". Please refer to IEEE 802.11ay Channelization.");
    }
}

EDMG_CHANNEL_CONFIG
DmgWifiPhy::GetChannelConfiguration (void) const
{
  return m_channelConfiguration;
}

DmgWifiPhy::DmgWifiPhy ()
  : m_rdsActivated (false)
{
  NS_LOG_FUNCTION (this);
  m_rdsActivated = false;
  m_lastTxDuration = NanoSeconds (0.0);
  m_psduSuccess = false;
  m_receivingTRNfield = false;
  m_suMimoBeamformingTraining = false;
  m_muMimoBeamformingTraining = false;
  m_recordSnrValues = true;
}

DmgWifiPhy::~DmgWifiPhy ()
{
  NS_LOG_FUNCTION (this);
}

void
DmgWifiPhy::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_channel = 0;
  WifiPhy::DoDispose ();
}

void
DmgWifiPhy::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  /* PHY Layer Information */
  if (!m_supportOFDM)
    {
      m_maxOfdmTxMcs = 0;
      m_maxOfdmRxMcs = 0;
      if (GetStandard () == WIFI_PHY_STANDARD_80211ay)
        {
          m_maxOfdmMcs = 0;
        }
    }
  m_edmgTrnFieldDuration = NanoSeconds (0);
  WifiPhy::DoInitialize ();
}

Ptr<Channel>
DmgWifiPhy::GetChannel (void) const
{
  return m_channel;
}

void
DmgWifiPhy::SetChannel (const Ptr<DmgWifiChannel> channel)
{
  m_channel = channel;
  m_channel->Add (this);
}

void
DmgWifiPhy::ActivateRdsOpereation (uint8_t srcSector, uint8_t srcAntenna,
                                   uint8_t dstSector, uint8_t dstAntenna)
{
  NS_LOG_FUNCTION (this << srcSector << srcAntenna << dstSector << dstAntenna);
  m_rdsActivated = true;
  m_rdsSector = m_srcSector = srcSector;
  m_rdsAntenna = m_srcAntenna = srcAntenna;
  m_dstSector = dstSector;
  m_dstAntenna = dstAntenna;
}

void
DmgWifiPhy::ResumeRdsOperation (void)
{
  NS_LOG_FUNCTION (this);
  m_rdsActivated = true;
  m_rdsSector = m_srcSector;
  m_rdsAntenna = m_srcAntenna;
}

void
DmgWifiPhy::SuspendRdsOperation (void)
{
  NS_LOG_FUNCTION (this);
  m_rdsActivated = false;
}

void
DmgWifiPhy::SetCodebook (Ptr<Codebook> codebook)
{
  m_codebook = codebook;
}

Ptr<Codebook>
DmgWifiPhy::GetCodebook (void) const
{
  return m_codebook;
}

void
DmgWifiPhy::SetSupportOfdmPhy (bool value)
{
  NS_LOG_FUNCTION (this << value);
  m_supportOFDM = value;
}

bool
DmgWifiPhy::GetSupportOfdmPhy (void) const
{
  return m_supportOFDM;
}

void
DmgWifiPhy::SetSupportLpScPhy (bool value)
{
  NS_LOG_FUNCTION (this << value);
  m_supportLpSc = value;
}

bool
DmgWifiPhy::GetSupportLpScPhy (void) const
{
  return m_supportLpSc;
}

uint8_t
DmgWifiPhy::GetMaxScTxMcs (void) const
{
  return m_maxScTxMcs;
}

uint8_t
DmgWifiPhy::GetMaxScRxMcs (void) const
{
  return m_maxScRxMcs;
}

uint8_t
DmgWifiPhy::GetMaxOfdmTxMcs (void) const
{
  return m_maxOfdmTxMcs;
}

uint8_t
DmgWifiPhy::GetMaxOfdmRxMcs (void) const
{
  return m_maxOfdmRxMcs;
}

uint8_t
DmgWifiPhy::GetMaxScMcs (void) const
{
  return m_maxScMcs;
}

uint8_t
DmgWifiPhy::GetMaxOfdmMcs (void) const
{
  return m_maxOfdmMcs;
}

uint8_t
DmgWifiPhy::GetMaxPhyRate (void) const
{
  return m_maxPhyRate;
}

bool
DmgWifiPhy::IsSuMimoSupported (void) const
{
  return m_suMimoSupported;
}

bool
DmgWifiPhy::IsMuMimoSupported (void) const
{
  return m_muMimoSupported;
}

void
DmgWifiPhy::SetSuMimoBeamformingTraining (bool value)
{
  NS_LOG_FUNCTION (this << value);
  m_suMimoBeamformingTraining = value;
}

void
DmgWifiPhy::SetMuMimoBeamformingTraining (bool value)
{
  NS_LOG_FUNCTION (this << value);
  m_muMimoBeamformingTraining = value;
}

bool
DmgWifiPhy::GetMuMimoBeamformingTraining (void) const
{
  return m_muMimoBeamformingTraining;
}

//// NINA ////
void
DmgWifiPhy::SetPeerTxssPackets (uint8_t number)
{
  NS_LOG_FUNCTION (this << uint16_t (number));
  m_peerTxssPackets = number;
}

void
DmgWifiPhy::SetTxssRepeat (uint8_t number)
{
  NS_LOG_FUNCTION (this << uint16_t (number));
  m_txssRepeat = number;
}
//// NINA ////

void
DmgWifiPhy::EndAllocationPeriod (void)
{
  NS_LOG_FUNCTION (this);
  switch (m_state->GetState ())
    {
    case WifiPhyState::RX:
      AbortCurrentReception (ALLOCATION_ENDED);
      MaybeCcaBusyDuration ();
    case WifiPhyState::TX:
    case WifiPhyState::SWITCHING:
    case WifiPhyState::CCA_BUSY:
    case WifiPhyState::IDLE:
    case WifiPhyState::SLEEP:
      break;
    default:
      NS_ASSERT (false);
      break;
    }
}

/* Channel Measurement Functions for 802.11ad-2012 */

void
DmgWifiPhy::StartMeasurement (uint16_t measurementDuration, uint8_t blocks)
{
  m_measurementBlocks = blocks;
  m_measurementUnit = floor (measurementDuration/blocks);
  Simulator::Schedule (MicroSeconds (measurementDuration), &DmgWifiPhy::EndMeasurement, this);
}

void
DmgWifiPhy::MeasurementUnitEnded (void)
{
  /* Add measurement to the list of measurements */
  m_measurementList.push_back (m_measurementBlocks);
  m_measurementBlocks--;
  if (m_measurementBlocks > 0)
    {
      /* Schedule new measurement Unit */
      Simulator::Schedule (MicroSeconds (m_measurementUnit), &DmgWifiPhy::EndMeasurement, this);
    }
}

void
DmgWifiPhy::EndMeasurement (void)
{
  m_reportMeasurementCallback (m_measurementList);
}

void
DmgWifiPhy::RegisterMeasurementResultsReady (ReportMeasurementCallback callback)
{
  m_reportMeasurementCallback = callback;
}

void
DmgWifiPhy::RegisterReportSnrCallback (ReportSnrCallback callback)
{
  NS_LOG_FUNCTION (this);
  m_reportSnrCallback = callback;
}

void
DmgWifiPhy::RegisterBeaconTrainingCallback (BeaconTrainingCallback callback)
{
  NS_LOG_FUNCTION (this);
  m_beaconTrainingCallback = callback;
}

//// NINA ////
void
DmgWifiPhy::RegisterReportMimoSnrCallback (ReportMimoSnrCallback callback)
{
  NS_LOG_FUNCTION (this);
  m_reportMimoSnrCallback = callback;
}
//// NINA ////

void
DmgWifiPhy::RegisterEndReceiveMimoTRNCallback (EndReceiveMimoTRNCallback callback)
{
  NS_LOG_FUNCTION (this);
  m_endRxMimoTrnCallback = callback;
}

void
DmgWifiPhy::Send (Ptr<const WifiPsdu> psdu, WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this << *psdu << txVector);
  /* Transmission can happen if:
   *  - we are syncing on a packet. It is the responsibility of the
   *    MAC layer to avoid doing this but the PHY does nothing to
   *    prevent it.
   *  - we are idle
   */
  NS_ASSERT (!m_state->IsStateTx () && !m_state->IsStateSwitching ());
  NS_ASSERT (m_endTxEvent.IsExpired ());

  if (txVector.GetNss () > GetMaxSupportedTxSpatialStreams ())
    {
      NS_FATAL_ERROR ("Unsupported number of spatial streams!");
    }

  if (m_state->IsStateSleep ())
    {
      NS_LOG_DEBUG ("Dropping packet because in sleep mode");
      NotifyTxDrop (psdu);
      return;
    }

  Time frameDuration = CalculateTxDuration (psdu->GetSize (), txVector, GetFrequency ());
  Time txDuration = frameDuration;
  NS_ASSERT (txDuration.IsStrictlyPositive ());

  //// WIGIG ////
  /* Check if there is a DMG TRN field appended to this PSDU */
  if ((GetStandard () == WIFI_PHY_STANDARD_80211ad) && (txVector.GetTrainngFieldLength () > 0))
    {
      NS_LOG_DEBUG ("Append " << uint16_t (txVector.GetTrainngFieldLength ()) << "DMG TRN Subfields");
      txDuration += txVector.GetTrainngFieldLength () * (AGC_SF_DURATION + TRN_SUBFIELD_DURATION)
                 + (txVector.GetTrainngFieldLength ()/4) * TRN_CE_DURATION;
      NS_LOG_DEBUG ("TxDuration=" << txDuration);
      //sendTrnField = true;
    }
  /* Check if there is an EDMG TRN field appended to this PSDU */
  else if ((GetStandard () == WIFI_PHY_STANDARD_80211ay) && (txVector.GetEDMGTrainingFieldLength () > 0))
    {
      NS_LOG_DEBUG ("Append " << uint16_t (txVector.GetEDMGTrainingFieldLength ()) << " EDMG TRN Units");
      /* Calculate the duration of the EDMG TRN Subfield according to the duration of the Golay Sequence length. */
      txVector.SetNumberOfTxChains (m_codebook->GetNumberOfActiveRFChains ());
      txVector.edmgTrnSubfieldDuration = CalculateEdmgTrnSubfieldDuration (txVector);
      NS_LOG_DEBUG ("TRN Subfield Duration = " << txVector.edmgTrnSubfieldDuration);
      if (txVector.GetPacketType () == TRN_R)
        {
          txDuration += txVector.edmgTrnSubfieldDuration * txVector.GetEDMGTrainingFieldLength () * EDMG_TRN_UNIT_SIZE;
        }
      else
        {
          txDuration += txVector.edmgTrnSubfieldDuration * (txVector.Get_TRN_T () +
                                                            txVector.GetEDMGTrainingFieldLength () *
                                                            (txVector.Get_EDMG_TRN_P () + txVector.Get_EDMG_TRN_M ()) +
                                                            txVector.Get_EDMG_TRN_P ());
        }
      NS_LOG_DEBUG ("TxDuration=" << txDuration);
      //sendTrnField = true;setup
    }
  if (m_suMimoBeamformingTraining || m_muMimoBeamformingTraining)
    m_edmgTrnFieldDuration = GetTRN_Field_Duration (txVector);
  NS_LOG_DEBUG ("TRN Field Duration = " << m_edmgTrnFieldDuration);
  //// WIGIG ////

  if ((m_currentEvent != 0) && (m_currentEvent->GetEndTime () > (Simulator::Now () + m_state->GetDelayUntilIdle ())))
    {
      //that packet will be noise _after_ the transmission.
      MaybeCcaBusyDuration ();
    }

  if (m_currentEvent != 0)
    {
      AbortCurrentReception (RECEPTION_ABORTED_BY_TX);
    }

  if (m_state->GetState () == WifiPhyState::OFF)
    {
      NS_LOG_DEBUG ("Transmission canceled because device is OFF");
      return;
    }

  double txPowerW = DbmToW (GetTxPowerForTransmission (txVector) + GetTxGain ());
  NotifyTxBegin (psdu, txPowerW);
  m_phyTxPsduBeginTrace (psdu, txVector, txPowerW);
  NotifyMonitorSniffTx (psdu, GetFrequency (), txVector);
  m_state->SwitchToTx (txDuration, psdu->GetPacket (), GetPowerDbm (txVector.GetTxPowerLevel ()), txVector);
  Ptr<WifiPpdu> ppdu = Create<WifiPpdu> (psdu, txVector, txDuration, GetFrequency ());

  if (m_wifiRadioEnergyModel != 0 && m_wifiRadioEnergyModel->GetMaximumTimeInState (WifiPhyState::TX) < txDuration)
    {
      ppdu->SetTruncatedTx ();
    }

  m_endTxEvent = Simulator::Schedule (txDuration, &WifiPhy::NotifyTxEnd, this, psdu);
  StartTx (ppdu);

  //// WIGIG ////
  /* Send TRN Units if beam refinement or tracking is requested */
  if (txVector.GetTrainngFieldLength () > 0)
    {
      /* Prepare transmission of the AGC Subfields */
      Simulator::Schedule (frameDuration, &DmgWifiPhy::StartAgcSubfieldsTx, this, txVector);
    }
  else if ((txVector.GetEDMGTrainingFieldLength () > 0)  && (GetStandard () == WIFI_PHY_STANDARD_80211ay))
    {
      Simulator::Schedule (frameDuration, &DmgWifiPhy::StartEdmgTrnFieldTx, this, txVector);
    }
  /* Record duration of the current transmission */

  m_lastTxDuration = txDuration;
  //// WIGIG ////

  m_channelAccessRequested = false;
  m_powerRestricted = false;
}

void
DmgWifiPhy::StartTx (Ptr<WifiPpdu> ppdu)
{
  NS_LOG_FUNCTION (this << ppdu);
  WifiTxVector txVector = ppdu->GetTxVector ();
  NS_LOG_DEBUG ("Start transmission: signal power before antenna gain=" << GetPowerDbm (txVector.GetTxPowerLevel ()) << "dBm");
  m_channel->Send (this, ppdu, GetTxPowerForTransmission (txVector) + GetTxGain ());
}

void
DmgWifiPhy::StartAgcSubfieldsTx (WifiTxVector txVector)
{
  NS_LOG_DEBUG ("Start AGC Subfields transmission: signal power before antenna gain=" <<
                GetPowerDbm (txVector.GetTxPowerLevel ()) << " dBm");
  if (txVector.GetPacketType () == TRN_T)
    {
      /* We are the initiator of the TRN-TX */
      m_codebook->UseCustomAWV (RefineTransmitSector);
    }
  SendAgcSubfield (txVector, txVector.GetTrainngFieldLength ());
}

void
DmgWifiPhy::SendAgcSubfield (WifiTxVector txVector, uint8_t subfieldsRemaining)
{
  NS_LOG_FUNCTION (this << txVector.GetMode () << uint16_t (subfieldsRemaining));

  subfieldsRemaining--;
  StartAgcSubfieldTx (txVector);

  if (subfieldsRemaining > 0)
    {
      Simulator::Schedule (AGC_SF_DURATION, &DmgWifiPhy::SendAgcSubfield, this, txVector, subfieldsRemaining);
    }
  else
    {
      txVector.remainingTrnUnits = txVector.GetTrainngFieldLength () / 4;
      Simulator::Schedule (AGC_SF_DURATION, &DmgWifiPhy::StartTrnUnitTx, this, txVector);
    }

  if (txVector.GetPacketType () == TRN_T)
    {
      /* We are the initiator of the TRN-TX */
      m_codebook->GetNextAWV ();
    }
}

void
DmgWifiPhy::StartAgcSubfieldTx (WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this << txVector.GetMode ());
  m_channel->SendAgcSubfield (this, GetPowerDbm (txVector.GetTxPowerLevel ()) + GetTxGain (), txVector);
}

void
DmgWifiPhy::StartTrnUnitTx (WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this << txVector << uint16_t (txVector.remainingTrnUnits));
  txVector.remainingTrnUnits--;
  SendCeSubfield (txVector);
}

void
DmgWifiPhy::SendCeSubfield (WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this);
  if (txVector.GetPacketType () == TRN_T)
    {
      /* We are the initiator of the TRN-TX */
      /* The CE Subfield of the TRN-Unit is transmitted using the sector used for transmiting the CEF of the preamble */
      m_codebook->UseLastTxSector ();
    }
  StartCeSubfieldTx (txVector);
  Simulator::Schedule (TRN_CE_DURATION, &DmgWifiPhy::StartTrnSubfieldsTx, this, txVector);
}

void
DmgWifiPhy::StartCeSubfieldTx (WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this << txVector.GetMode ());
  m_channel->SendTrnCeSubfield (this, GetPowerDbm (txVector.GetTxPowerLevel ()) + GetTxGain (), txVector);
}

void
DmgWifiPhy::StartTrnSubfieldsTx (WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this << uint16_t (txVector.remainingTrnUnits));
  m_codebook->UseCustomAWV (RefineTransmitSector);
  txVector.remainingTrnSubfields = TRN_UNIT_SIZE;
  SendTrnSubfield (txVector);
}

void
DmgWifiPhy::SendTrnSubfield (WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this << txVector.GetMode () << uint16_t (txVector.remainingTrnUnits) << uint16_t (txVector.remainingTrnSubfields));

  txVector.remainingTrnSubfields--;
  StartTrnSubfieldTx (txVector);

  if (txVector.remainingTrnSubfields != 0)
    {
      Simulator::Schedule (TRN_SUBFIELD_DURATION, &DmgWifiPhy::SendTrnSubfield, this, txVector);
    }
  else if ((txVector.remainingTrnUnits > 0) && (txVector.remainingTrnSubfields == 0))
    {
      Simulator::Schedule (TRN_SUBFIELD_DURATION, &DmgWifiPhy::StartTrnUnitTx, this, txVector);
    }

  if (txVector.GetPacketType () == TRN_T)
    {
      /* We are the initiator of the TRN-TX */
      m_codebook->GetNextAWV ();
    }
}

void
DmgWifiPhy::StartTrnSubfieldTx (WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this << txVector.GetMode ());
  m_channel->SendTrnSubfield (this, GetPowerDbm (txVector.GetTxPowerLevel ()) + GetTxGain (), txVector);
}

void
DmgWifiPhy::StartEdmgTrnFieldTx (WifiTxVector txVector)
{
  NS_LOG_DEBUG ("Start EDMG TRN Field transmission: signal power before antenna gain=" <<
                GetPowerDbm (txVector.GetTxPowerLevel ()) << " dBm");
  /* Activate all antennas that need to be trained + set them to the correct sector (best sector for comm with that station) */
  //// NINA ////
  if (m_suMimoBeamformingTraining || m_muMimoBeamformingTraining)
    {
      m_codebook->StartMimoTx ();
    }
  //// NINA ////
  txVector.remainingTrnUnits = txVector.GetEDMGTrainingFieldLength ();
  if (txVector.GetPacketType () == TRN_R)
    {
      StartEdmgTrnUnitTx(txVector);
    }
  else
    {
      txVector.remainingTrnSubfields = txVector.Get_EDMG_TRN_M ();
      txVector.remainingPSubfields = txVector.Get_EDMG_TRN_P ();
      txVector.remainingTSubfields = txVector.Get_TRN_T ();
      if (txVector.GetPacketType () == TRN_RT)
        {
          txVector.repeatSameAWVUnit = txVector.Get_RxPerTxUnits ();
        }
      SendTSubfields (txVector);
    }
}

void
DmgWifiPhy::SendTSubfields (WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this);
  txVector.remainingTSubfields--;
  StartEdmgTrnSubfieldTx (txVector);
  if (txVector.remainingTSubfields == 0)
    {
      Simulator::Schedule (txVector.edmgTrnSubfieldDuration, &DmgWifiPhy::StartEdmgTrnUnitTx, this, txVector);
    }
  else
    {
      Simulator::Schedule (txVector.edmgTrnSubfieldDuration, &DmgWifiPhy::SendTSubfields, this, txVector);
    }
}

void
DmgWifiPhy::StartEdmgTrnUnitTx (WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this << txVector << uint16_t (txVector.remainingTrnUnits));
  if ((txVector.GetPacketType () == TRN_RT) && (txVector.repeatSameAWVUnit == 0))
    {
      //// NINA ////
      if (m_suMimoBeamformingTraining || m_muMimoBeamformingTraining)
        {
          m_codebook->GetMimoNextSectorWithCombinations (false);
        }
      else
        {
          m_codebook->GetNextAWV ();
        }
      //// NINA ////
      txVector.repeatSameAWVUnit = txVector.Get_RxPerTxUnits ();
    }
  txVector.remainingTrnUnits--;
  txVector.repeatSameAWVUnit--;
  if (txVector.GetPacketType () == TRN_R)
    {
      txVector.remainingTrnSubfields = EDMG_TRN_UNIT_SIZE;
      SendEdmgTrnSubfield (txVector);
    }
  else
    {
      txVector.remainingTrnSubfields = txVector.Get_EDMG_TRN_M ();
      txVector.remainingPSubfields = txVector.Get_EDMG_TRN_P ();
      if (txVector.GetPacketType () == TRN_T)
        {
          txVector.repeatSameAWVSubfield = txVector.Get_EDMG_TRN_N ();
        }
      else
        {
          txVector.repeatSameAWVSubfield = txVector.Get_EDMG_TRN_M ();
        }
      SendPSubfields (txVector);
    }
}

void
DmgWifiPhy::SendPSubfields (WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this);
  /* If SU-MIMO Beamforming training and we are not sending the first TRN Unit - reset to the sectors used in
   * transmitting the previous P subfields (If we are sending the first TRN Unit we are set to the correct sector) */
  //// NINA ////
  if ((m_suMimoBeamformingTraining || m_muMimoBeamformingTraining)
      && (txVector.remainingTrnUnits != txVector.GetEDMGTrainingFieldLength () - 1))
    {
      m_codebook->UseOldTxSector ();
    }
  else if (m_codebook->IsCustomAWVUsed ())
    {
      m_codebook->UseLastTxSector ();
    }
  //// NINA ////
  txVector.remainingPSubfields--;
  StartEdmgTrnSubfieldTx (txVector);
  if (txVector.remainingPSubfields > 0)
    {
      Simulator::Schedule (txVector.edmgTrnSubfieldDuration, &DmgWifiPhy::SendPSubfields, this, txVector);
    }
  else if (txVector.remainingTrnSubfields > 0)
    {
      Simulator::Schedule (txVector.edmgTrnSubfieldDuration, &DmgWifiPhy::SendEdmgTrnSubfield, this, txVector);
    }
  else if ((txVector.GetPacketType () == TRN_RT) && (txVector.repeatSameAWVUnit == 0) &&
           (m_suMimoBeamformingTraining || m_muMimoBeamformingTraining))
    {
      m_codebook->GetMimoNextSectorWithCombinations (false);
    }
}

void
DmgWifiPhy::SendEdmgTrnSubfield (WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this << txVector.GetMode () << uint16_t (txVector.remainingTrnUnits) << uint16_t (txVector.remainingTrnSubfields));
  if (txVector.GetPacketType () != TRN_R && txVector.remainingTrnSubfields == txVector.Get_EDMG_TRN_M ())
    {
      //// NINA ////
      // If SU-MIMO Beamforming training
      if (m_suMimoBeamformingTraining || m_muMimoBeamformingTraining)
        {
          bool firstSector = (txVector.remainingTrnUnits == (txVector.GetEDMGTrainingFieldLength () - 1));
          if (txVector.GetPacketType () == TRN_T)
            {
              m_codebook->GetMimoNextSector (firstSector);
            }
          else if (txVector.GetPacketType () == TRN_RT)
            {
              m_codebook->GetMimoNextSectorWithCombinations (true);
            }
          else
            NS_LOG_ERROR("TRN-R Packets are not used in MIMO BFT");
        }
      else
        m_codebook->UseCustomAWV (RefineTransmitSector);
      //// NINA ////
    }
  else
    {
      if ((txVector.GetPacketType () == TRN_T) && (txVector.repeatSameAWVSubfield == 0))
        {
          //// NINA ////
          if (m_suMimoBeamformingTraining )
            {
              m_codebook->GetMimoNextSector (false);
            }
          else
            {
              m_codebook->GetNextAWV ();
            }
          //// NINA ////
          txVector.repeatSameAWVSubfield = txVector.Get_EDMG_TRN_N ();
        }
    }
  txVector.remainingTrnSubfields--;
  txVector.repeatSameAWVSubfield--;
  if (txVector.remainingTrnSubfields != 0)
    {
      Simulator::Schedule (txVector.edmgTrnSubfieldDuration, &DmgWifiPhy::SendEdmgTrnSubfield, this, txVector);
    }
  else if ((txVector.remainingTrnUnits > 0) && (txVector.remainingTrnSubfields == 0))
    {
      Simulator::Schedule (txVector.edmgTrnSubfieldDuration, &DmgWifiPhy::StartEdmgTrnUnitTx, this, txVector);
    }
  else if ((txVector.remainingTrnUnits == 0) && (txVector.remainingTrnSubfields == 0) && (txVector.GetPacketType () != TRN_R))
    {
      txVector.remainingPSubfields = txVector.Get_EDMG_TRN_P ();
      Simulator::Schedule (txVector.edmgTrnSubfieldDuration, &DmgWifiPhy::SendPSubfields, this, txVector);
    }
  StartEdmgTrnSubfieldTx (txVector);
}

void
DmgWifiPhy::StartEdmgTrnSubfieldTx (WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this << txVector.GetMode ());
  m_channel->SendTrnSubfield (this, GetPowerDbm (txVector.GetTxPowerLevel ()) + GetTxGain (), txVector);
}

void
DmgWifiPhy::StartReceiveAgcSubfield (WifiTxVector txVector, double rxPowerDbm)
{
  NS_LOG_FUNCTION (this << txVector.GetMode () << rxPowerDbm
                   << m_psduSuccess << m_state->IsStateRx ()
                   << txVector.GetSender () << m_currentSender);
  double rxPowerW = DbmToW (rxPowerDbm);
  if (m_psduSuccess && m_state->IsStateRx () && txVector.GetSender () == m_currentSender)
    {
      /* Add Interference event for AGC Subfield */
      Ptr<Event> event;
      event = m_interference.Add (txVector,
                                  AGC_SF_DURATION,
                                  rxPowerW);
    }
  else
    {
      NS_LOG_DEBUG ("Drop AGC Subfield because did not receive successfully the PHY frame");
    }
}

void
DmgWifiPhy::StartReceiveCeSubfield (WifiTxVector txVector, double rxPowerDbm)
{
  NS_LOG_FUNCTION (this << txVector.GetMode () << rxPowerDbm);
  double rxPowerW = DbmToW (rxPowerDbm);
  if (m_psduSuccess && m_state->IsStateRx () && txVector.GetSender () == m_currentSender)
    {
      /* Add Interference event for TRN-CE Subfield */
      Ptr<Event> event;
      event = m_interference.Add (txVector, TRN_CE_DURATION, rxPowerW);
      if (txVector.GetPacketType () == TRN_R)
        {
          /* We are the initiator of the TRN-RX, we switch between different AWVs */
          m_codebook->UseCustomAWV (RefineReceiveSector);
          /* If at the first TRN Unit switch the AWV after ending the last AGC subfield */
          if (txVector.remainingTrnUnits == txVector.GetTrainngFieldLength () / 4)
            {
              m_codebook->GetNextAWV ();
            }
        }
    }
  else
    {
      NS_LOG_DEBUG ("Drop TRN-CE Subfield because did not receive successfully the PHY frame");
    }
}

void
DmgWifiPhy::StartReceiveTrnSubfield (WifiTxVector txVector, double rxPowerDbm)
{
  NS_LOG_FUNCTION (this << txVector.GetMode () << rxPowerDbm << uint16_t (txVector.remainingTrnSubfields));
  double rxPowerW = DbmToW (rxPowerDbm);
  if (m_psduSuccess && m_state->IsStateRx () && txVector.GetSender () == m_currentSender)
    {
      /* Add Interference event for TRN Subfield */
      Ptr<Event> event;
      event = m_interference.Add (txVector,
                                  TRN_SUBFIELD_DURATION,
                                  rxPowerW);

      if (txVector.GetPacketType () == TRN_R)
        {
          /* Schedule an event for the complete reception of this TRN Subfield */
          Simulator::Schedule (TRN_SUBFIELD_DURATION - NanoSeconds (1), &DmgWifiPhy::EndReceiveTrnSubfield, this,
                               m_codebook->GetActiveRxSectorID (), m_codebook->GetActiveAntennaID (),
                               txVector, event);
        }
      else
        {
          /**
           * We are the responder of the beam refinement phase and the initiator wants to refine its transmit pattern.
           * The transmitter sends every TRN Subfield using unique beam pattern i.e. the transmitter switches among
           * among the AWVs within the main sector. We record the SNR value of each TRN-SF and feedback the best
           * AWV ID to the initiator.
           */

          /* Schedule an event for the complete reception of this TRN Subfield */
          Simulator::Schedule (TRN_SUBFIELD_DURATION - NanoSeconds (1), &DmgWifiPhy::EndReceiveTrnSubfield, this,
                               m_codebook->GetActiveTxSectorID (), m_codebook->GetActiveAntennaID (),
                               txVector, event);
        }
    }
  else
    {
      NS_LOG_DEBUG ("Drop TRN Subfield because did not receive successfully the PHY frame");
    }
}

void
DmgWifiPhy::StartReceiveEdmgTrnSubfield (WifiTxVector txVector, double rxPowerDbm)
{
  NS_LOG_FUNCTION (this << txVector.GetMode () << rxPowerDbm << uint16_t (txVector.remainingTrnSubfields));
  double rxPowerW = DbmToW (rxPowerDbm);
  /* Receive only TRN subfield that belong to the packet that we are already receiving + make check if the packet
   * is properly received */
  if (m_psduSuccess && m_state->IsStateRx () && txVector.GetSender () == m_currentSender)
    {
      /* Add Interference event for TRN Subfield */
      Ptr<Event> event = m_interference.Add (txVector, txVector.edmgTrnSubfieldDuration, rxPowerW);
      /**
       * We are receiving TRN-R subfields. The transmitter transmits all of them with the same transmission pattern
       * and the receiver switches beam patterns while receiving the different TRN subfields.
       */
      if (txVector.GetPacketType () == TRN_R)
        {
          /* if beamforming training using TRN-R fields appended to beacons. */
          if (txVector.IsDMGBeacon ())
            {
              /* For now APs don't beamform using TRN-R fields appended to beacons from other APs that they overhear. */
              if (!m_isAp)
                {
                  /* Schedule an event for the complete reception of this TRN Subfield. */
                  if (m_codebook->IsCustomAWVUsed ())
                    {
                      /* If receiving using a custom AWV pass the AWV ID. */
                      Simulator::Schedule (txVector.edmgTrnSubfieldDuration - NanoSeconds (1),
                                           &DmgWifiPhy::EndReceiveBeaconTrnSubfield, this,
                                           m_codebook->GetActiveRxSectorID (), m_codebook->GetActiveAntennaID (),
                                           m_codebook->GetActiveRxPatternID (), txVector, event);
                    }
                  else
                    {
                      /* If not pass NO_AWV_ID = 255 to signal that no AWV is used. */
                      Simulator::Schedule (txVector.edmgTrnSubfieldDuration - NanoSeconds (1),
                                           &DmgWifiPhy::EndReceiveBeaconTrnSubfield, this,
                                           m_codebook->GetActiveRxSectorID (), m_codebook->GetActiveAntennaID (),
                                           NO_AWV_ID, txVector, event);
                    }
                }
            }
          /* If receiving a regular TRN field as part of a BRP packet. */
          else
            {
              /* Schedule an event for the complete reception of this TRN Subfield. */
              Simulator::Schedule (txVector.edmgTrnSubfieldDuration - NanoSeconds (1), &DmgWifiPhy::EndReceiveTrnSubfield, this,
                                   m_codebook->GetActiveRxSectorID (), m_codebook->GetActiveAntennaID (),
                                   txVector, event);
            }
        }
      else /* If receiving a TRN-T or TRN-RT field. */
        {
          /**
           * We are receiving TRN-TX or TRN-RX/TX fields. The transmitter switches beam patterns when
           * transmitting the TRN subfields and we need to record the received SNR and feedback the
           * best transmit sector for the transmitter to communicate with us.
           */
          /* The first T subfields and the first P subfields from each unit are not used for training. */
          if ((txVector.remainingTSubfields == 0) && (txVector.remainingPSubfields == 0)
           && (txVector.remainingTrnSubfields != txVector.Get_EDMG_TRN_M ()))
            {
              /* Schedule an event for the complete reception of this TRN Subfield. */
              Simulator::Schedule (txVector.edmgTrnSubfieldDuration - NanoSeconds (1), &DmgWifiPhy::EndReceiveTrnSubfield, this,
                                   m_codebook->GetActiveRxSectorID (), m_codebook->GetActiveAntennaID (),
                                   txVector, event);
            }
          /* Make sure to receive the final training subfields before the last P subfields */
          if ((txVector.remainingTrnUnits == 0) && (txVector.remainingPSubfields == txVector.Get_EDMG_TRN_P ())
           && (txVector.remainingTrnSubfields == 0))
            {
              /* Schedule an event for the complete reception of this TRN Subfield. */
              Simulator::Schedule (txVector.edmgTrnSubfieldDuration - NanoSeconds (1), &DmgWifiPhy::EndReceiveTrnSubfield, this,
                                   m_codebook->GetActiveRxSectorID (), m_codebook->GetActiveAntennaID (),
                                   txVector, event);
            }
        }
    }
  else
    {
      NS_LOG_DEBUG ("Drop TRN Subfield because did not receive successfully the PHY frame");
    }
}

void
DmgWifiPhy::EndReceiveTrnSubfield (SectorID sectorId, AntennaID antennaId, WifiTxVector txVector, Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (sectorId) << static_cast<uint16_t> (antennaId) << txVector.GetMode ()
                   << uint16_t (txVector.remainingTrnUnits) << uint16_t (txVector.remainingTrnSubfields) << event->GetRxPowerW ());
  /* Calculate SNR and report it to the upper layer */
  double snr = m_interference.CalculatePlcpTrnSnr (event);
  /* Helps calculate the index of the AWV for EDMG TRN-Tx and EDMG TRN-Rx/Tx fields - in other cases should be 1. */
  uint8_t index = 1;
  if (txVector.GetEDMGTrainingFieldLength () > 0)
    {
      if ((txVector.GetPacketType () == TRN_T) && (txVector.Get_EDMG_TRN_N () != 0))
        {
          index=txVector.Get_EDMG_TRN_N ();
        }
      else if (txVector.GetPacketType () == TRN_RT)
        {
          index = txVector.Get_RxPerTxUnits () * txVector.Get_EDMG_TRN_M ();
        }
    }
  uint8_t pSubfields = 0;
  if ((txVector.GetEDMGTrainingFieldLength () > 0) && (txVector.GetPacketType () != TRN_R))
    {
      pSubfields = txVector.remainingPSubfields;
    }
  // Put a super low value for the snr for the last EDMG TRN subfield send in order to disregard its value.
  // In TRN-TX and TRN-RX/TX packets, the last P subfields transmitted shouldn't be used for training.
  // However, it's necessary to signal to the upper layer when the field is finished receiving.
  if ((txVector.GetEDMGTrainingFieldLength () > 0) && (txVector.GetPacketType () != TRN_R)
       && (txVector.remainingTrnUnits == 0) && (txVector.remainingTrnSubfields == 0) && (txVector.remainingPSubfields == 0))
    {
      snr = 0.5;
    }

  m_reportSnrCallback (antennaId, sectorId, txVector.remainingTrnUnits, txVector.remainingTrnSubfields, pSubfields,
                       snr, (txVector.GetPacketType () != TRN_R), index);
  /* Switch to the next antenna configuration in case of receive training */
  if (txVector.GetPacketType () == TRN_R)
    {
      /* Change Rx Sector for the next TRN Subfield */
      m_codebook->GetNextAWV ();
    }
  /* TRN-RX/TX subfields are used for simultaneous transmit and receive training - the transmitter
   * repeats each Unit several times so that the responder can train it's receive sectors */
  else if (txVector.GetPacketType () == TRN_RT)
    {
      if (txVector.remainingTrnSubfields == 0 && txVector.repeatSameAWVUnit == 0)
        {
          m_codebook->UseFirstAWV ();
        }
      else
        {
          m_codebook->GetNextAWV ();
        }
    }
}

void
DmgWifiPhy::EndReceiveBeaconTrnSubfield (SectorID sectorId, AntennaID antennaId, AWV_ID awvId,
                                         WifiTxVector txVector, Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (sectorId) << static_cast<uint16_t> (antennaId) << txVector.GetMode ()
                   << uint16_t (txVector.remainingTrnUnits) << uint16_t (txVector.remainingTrnSubfields) << event->GetRxPowerW ());
  /* Calculate SNR and report it to the upper layer */
  double snr = m_interference.CalculatePlcpTrnSnr (event);
  m_beaconTrainingCallback (antennaId, sectorId, awvId, txVector.remainingTrnUnits,
                            txVector.remainingTrnSubfields, snr, txVector.GetSender ());
  /* For now APs do not do receive training when overhearing beacons from other APs */
  if (!m_isAp)
    {
      /* If we're using AWVs in the current sector and not at the end of the AWV list change
       *  the receive pattern for the next AWV */
      if ((m_codebook->IsCustomAWVUsed ()) && (m_codebook->GetRemaingAWVCount () != 0))
        {
          m_codebook->GetNextAWV ();
        }
      else
        {
          /* If not using AWVs or at the end of the AWV list for the current sector change sectors
           * and start sweeping the AWV list of the new sector (if there are any - if not stick to sectors) */
          bool changeAntenna;
          m_codebook->GetNextSector (changeAntenna);
          m_codebook->StartSectorRefinement ();
        }
    }
}

void
DmgWifiPhy::EndReceiveTrnField (bool isBeacon)
{
  NS_LOG_FUNCTION (this << isBeacon << GetState ()->GetState ());
//  NS_ASSERT (IsStateRx ());
  m_interference.NotifyRxEnd ();
  m_currentSender = Mac48Address ();
  m_currentEvent = 0;
  MaybeCcaBusyDuration ();
  if (m_psduSuccess && !m_isAp)
    {
      m_receivingTRNfield = false;
    }
  if (isBeacon && m_psduSuccess)
    {
      if (m_isQuasiOmni)
        {
          m_codebook->SetReceivingInQuasiOmniMode (m_oldAntennaID);
        }
      else
        {
          m_codebook->SetActiveRxSectorID (m_oldAntennaID, m_oldSectorID);
          if (m_oldAwvID != NO_AWV_ID)
            {
              m_codebook->SetActiveRxAwvID (m_oldAwvID);
            }
        }
    }
  //// NINA ////
  if (m_suMimoBeamformingTraining || m_muMimoBeamformingTraining)
    {
      m_codebook->SetCommunicationMode (SISO_MODE);
      m_codebook->SetReceivingInQuasiOmniMode ();
      m_endRxMimoTrnCallback ();
    }
  //// NINA ////
  if (m_psduSuccess)
    {
      m_state->SwitchFromRxEndOk ();
    }
  else
    {
      m_state->SwitchFromRxEndError ();
    }
}

//// NINA ////
void
DmgWifiPhy::StartReceiveEdmgTrnSubfield (WifiTxVector txVector, std::vector<double> rxPowerDbm)
{
  NS_LOG_FUNCTION (this << txVector.GetMode () << uint16_t (txVector.remainingTrnSubfields));

  /* Convert power values from dBm to Watt */
  std::vector<double> rxPowerW;
  NS_LOG_DEBUG("MIMO TRN Rx Power");
  for (const auto& value: rxPowerDbm)
    {
      rxPowerW.push_back (DbmToW (value));
      NS_LOG_DEBUG (value);
    }

  /* Receive only TRN subfields that belong to the PPDU that we are already receiving + make sure that the PPDU
   * has been received successfully */
  if (m_psduSuccess && m_state->IsStateRx () && (txVector.GetSender () == m_currentSender) && m_psduSuccess)
    {
      /* Add Interference event for EDMG TRN subfield */
      Ptr<Event> event = m_interference.Add (txVector, txVector.edmgTrnSubfieldDuration);

      /**
       * We are receiving EDMG TRN-R subfields. The transmitter transmits them all with the same transmission pattern
       * and the receiver switches beam patterns while receiving the different TRN subfields.
       */
      if (txVector.GetPacketType () == TRN_R)
        {
          /* Schedule an event for the complete reception of this TRN subfield. */
          Simulator::Schedule (txVector.edmgTrnSubfieldDuration - NanoSeconds (1), &DmgWifiPhy::EndReceiveMimoTrnSubfield, this,
                               txVector, event, rxPowerW);
        }
      else /* If receiving a TRN-T or TRN-RT subfield. */
        {
          /**
           * We are receiving TRN-TX or TRN-RX/TX subfields. The transmitter switches beam patterns when
           * transmitting the TRN subfields and we need to record the received SNR and feedback the
           * best transmit sector for the transmitter to communicate with us.
           */
          /* The first T subfields and the first P subfields from each unit are not used for training. */
          if ((txVector.remainingTSubfields == 0) && (txVector.remainingPSubfields == 0)
           && (txVector.remainingTrnSubfields != txVector.Get_EDMG_TRN_M ()))
            {
              /* Schedule an event for the complete reception of this TRN Subfield. */
              Simulator::Schedule (txVector.edmgTrnSubfieldDuration - NanoSeconds (1), &DmgWifiPhy::EndReceiveMimoTrnSubfield, this,
                                   txVector, event, rxPowerW);
            }
          /* Make sure to receive the final training subfields before the last P subfields */
          if ((txVector.remainingTrnUnits == 0) && (txVector.remainingPSubfields == txVector.Get_EDMG_TRN_P ())
           && (txVector.remainingTrnSubfields == 0))
            {
              /* Schedule an event for the complete reception of this TRN Subfield. */
              Simulator::Schedule (txVector.edmgTrnSubfieldDuration - NanoSeconds (1), &DmgWifiPhy::EndReceiveMimoTrnSubfield, this,
                                   txVector, event, rxPowerW);
            }
        }
    }
  else
    {
      NS_LOG_DEBUG ("Drop TRN Subfield because PPDU reception failed ");
    }
}

void
DmgWifiPhy::EndReceiveMimoTrnSubfield (WifiTxVector txVector, Ptr<Event> event, std::vector<double> rxPowerW)
{
  NS_LOG_FUNCTION (this << txVector.GetMode () << uint16_t (txVector.remainingTrnUnits)
                   << uint16_t (txVector.remainingTrnSubfields) << event->GetRxPowerW ());

  /* Calculate SNR and report it to the upper layer */
  std::vector<double> snrValues;
  /* Currently in the SISO phase of SU-MIMO BFT we report the SINR without taking account the inter-stream interference, while
   * in the MIMO phase we do take into consideration the inter-stream interference */
  if ((m_suMimoBeamformingTraining || m_muMimoBeamformingTraining) && txVector.GetPacketType () == TRN_RT)
    {
      snrValues = m_interference.CalculateMimoTrnSnr (event, rxPowerW, false, m_codebook->GetActiveAntennasIDs ().size ());
    }
  else
    snrValues = m_interference.CalculateMimoTrnSnr (event, rxPowerW);

  // Make sure not to report the last P subfield.
  if ( (m_suMimoBeamformingTraining || m_muMimoBeamformingTraining) &&
      !((txVector.GetPacketType () != TRN_R) && (txVector.remainingTrnUnits == 0)
      && (txVector.remainingTrnSubfields == 0) && (txVector.remainingPSubfields == 0)) && m_recordSnrValues)
    {
      m_reportMimoSnrCallback (m_codebook->GetActiveAntennasIDs (), snrValues);
    }

  /* Switch to the next antenna configuration in case of receive training */
  if (txVector.GetPacketType () == TRN_R)
    {
      /* Change Rx Sector for the next TRN Subfield */
      m_codebook->GetNextAWV ();
    }

  /* TRN-RX/TX subfields are used for simultaneous transmit and receive training - the transmitter
   * repeats each Unit several times so that the responder can train it's receive sectors */
  else if (txVector.GetPacketType () == TRN_RT)
    {
      if (txVector.remainingTrnSubfields == 0 && txVector.repeatSameAWVUnit == 0)
        {
          if (m_suMimoBeamformingTraining || m_muMimoBeamformingTraining)
            {
              m_codebook->SetMimoFirstCombination ();
              m_recordSnrValues = true;
            }
          else
            {
              m_codebook->UseFirstAWV ();
            }
        }
      else
        {
          if (m_suMimoBeamformingTraining || m_muMimoBeamformingTraining)
            {
              if (m_codebook->GetRemaingSectorCount () == 0)
                m_recordSnrValues = false;
              m_codebook->GetMimoNextSector (false);
            }
          else
            {
              m_codebook->GetNextAWV ();
            }
        }
    }
}
//// NINA ////

void
DmgWifiPhy::StartRx (Ptr<Event> event, double rxPowerW)
{
  NS_LOG_FUNCTION (this << *event << rxPowerW);

  NS_LOG_DEBUG ("sync to signal (power=" << rxPowerW << "W)");
  m_interference.NotifyRxStart (); //We need to notify it now so that it starts recording events
  if (!m_endPreambleDetectionEvent.IsRunning ())
    {
      //// WIGIG ////
      m_currentSender = event->GetTxVector ().GetSender ();
      //// WIGIG ////
      Time startOfPreambleDuration = GetPreambleDetectionDuration ();
      Time remainingRxDuration = event->GetDuration () - startOfPreambleDuration;
      m_endPreambleDetectionEvent = Simulator::Schedule (startOfPreambleDuration, &DmgWifiPhy::StartReceiveHeader, this, event);
    }
  else if ((m_frameCaptureModel != 0) && (rxPowerW > m_currentEvent->GetRxPowerW ()))
    {
      //// WIGIG ////
      m_currentSender = event->GetTxVector ().GetSender ();
      //// WIGIG ////
      NS_LOG_DEBUG ("Received a stronger signal during preamble detection: drop current packet and switch to new packet");
      NotifyRxDrop (m_currentEvent->GetPsdu (), PREAMBLE_DETECTION_PACKET_SWITCH);
      m_interference.NotifyRxEnd ();
      m_endPreambleDetectionEvent.Cancel ();
      m_interference.NotifyRxStart ();
      Time startOfPreambleDuration = GetPreambleDetectionDuration ();
      Time remainingRxDuration = event->GetDuration () - startOfPreambleDuration;
      m_endPreambleDetectionEvent = Simulator::Schedule (startOfPreambleDuration, &DmgWifiPhy::StartReceiveHeader, this, event);
    }
  else
    {
      NS_LOG_DEBUG ("Drop packet because RX is already decoding preamble");
      NotifyRxDrop (event->GetPsdu (), BUSY_DECODING_PREAMBLE);
      return;
    }
  m_currentEvent = event;
}

void
DmgWifiPhy::StartReceiveHeader (Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << *event);
  NS_ASSERT (!IsStateRx ());
  NS_ASSERT (m_endPhyRxEvent.IsExpired ());
  NS_ASSERT (m_currentEvent != 0);
  NS_ASSERT (event->GetStartTime () == m_currentEvent->GetStartTime ());
  NS_ASSERT (event->GetEndTime () == m_currentEvent->GetEndTime ());

  InterferenceHelper::SnrPer snrPer = m_interference.CalculateDmgPhyHeaderSnrPer (event); //// WIGIG It should be only SNR calculation
  double snr = snrPer.snr;
  NS_LOG_DEBUG ("snr(dB)=" << RatioToDb (snrPer.snr) << ", per=" << snrPer.per);

  if (!m_preambleDetectionModel || (m_preambleDetectionModel->IsPreambleDetected (event->GetRxPowerW (), snr, m_channelWidth)))
    {
      NotifyRxBegin (event->GetPsdu ());

      m_timeLastPreambleDetected = Simulator::Now ();
      WifiTxVector txVector = event->GetTxVector ();

      //Schedule end of DMG PHY header
      Time remainingPreambleAndHeaderDuration = GetPhyPreambleDuration (txVector) + GetPhyHeaderDuration (txVector) - GetPreambleDetectionDuration ();
      m_state->SwitchMaybeToCcaBusy (remainingPreambleAndHeaderDuration);
      m_endPhyRxEvent = Simulator::Schedule (remainingPreambleAndHeaderDuration, &DmgWifiPhy::ContinueReceiveHeader, this, event);
    }
  else
    {
      NS_LOG_DEBUG ("Drop packet because PHY preamble detection failed");
      NotifyRxDrop (event->GetPsdu (), PREAMBLE_DETECT_FAILURE);
      m_interference.NotifyRxEnd ();
      m_currentEvent = 0;
      //// WIGIG ////
      m_psduSuccess = false;
      //// WIGIG ////

      // Like CCA-SD, CCA-ED is governed by the 4μs CCA window to flag CCA-BUSY
      // for any received signal greater than the CCA-ED threshold.
      if (event->GetEndTime () > (Simulator::Now () + m_state->GetDelayUntilIdle ()))
        {
          MaybeCcaBusyDuration ();
        }
    }
}

void
DmgWifiPhy::ContinueReceiveHeader (Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << *event);
  NS_ASSERT (m_endPhyRxEvent.IsExpired ());

  InterferenceHelper::SnrPer snrPer;
  snrPer = m_interference.CalculateDmgPhyHeaderSnrPer (event);

  if (m_random->GetValue () > snrPer.per) //DMG PHY header reception succeeded
    {
      NS_LOG_DEBUG ("Received DMG PHY header");
      WifiTxVector txVector = event->GetTxVector ();
      Time remainingRxDuration = event->GetEndTime () - Simulator::Now ();
      m_state->SwitchMaybeToCcaBusy (remainingRxDuration);
      Time remainingPreambleHeaderDuration = CalculatePhyPreambleAndHeaderDuration (txVector) - GetPhyPreambleDuration (txVector) - GetPhyHeaderDuration (txVector);
      m_endPhyRxEvent = Simulator::Schedule (remainingPreambleHeaderDuration, &DmgWifiPhy::StartReceivePayload, this, event);
    }
  else //DMG/EDMG PHY header reception failed
    {
      NS_LOG_DEBUG ("Abort reception because DMG PHY header reception failed");
      //// WIGIG ////
      m_psduSuccess = false;
      //// WIGIG ////
      AbortCurrentReception (DMG_HEADER_FAILURE);
      if (event->GetEndTime () > (Simulator::Now () + m_state->GetDelayUntilIdle ()))
        {
          MaybeCcaBusyDuration ();
        }
    }
}

//void
//DmgWifiPhy::StartRx (Ptr<Packet> packet, WifiTxVector txVector, MpduType mpdutype, double rxPowerW,
//                     Time rxDuration, Time totalDuration,
//                     Ptr<Event> event)
//{
//  NS_LOG_FUNCTION (this << packet << txVector << (uint16_t)mpdutype << rxPowerW << rxDuration);
//  if (rxPowerW > GetEdThresholdW ()) //checked here, no need to check in the payload reception (current implementation assumes constant rx power over the packet duration)
//    {
//      if (m_rdsActivated)
//        {
//          NS_LOG_DEBUG ("Receiving as RDS in FD-AF Mode");
//          /**
//           * We are working in Full Duplex-Amplify and Forward. So we receive the packet without decoding it
//           * or checking its header. Then we amplify it and redirect it to the the destination.
//           * We take a simple approach to model full duplex communication by swapping current steering sector.
//           * Another approach would by adding new PHY layer which adds more complexity to the solution.
//           */

//          if ((mpdutype == NORMAL_MPDU) || (mpdutype == LAST_MPDU_IN_AGGREGATE))
//            {
//              if ((m_rdsSector == m_srcSector) && (m_rdsAntenna == m_srcAntenna))
//                {
//                  m_rdsSector = m_dstSector;
//                  m_rdsAntenna = m_dstAntenna;
//                }
//              else
//                {
//                  m_rdsSector = m_srcSector;
//                  m_rdsAntenna = m_srcAntenna;
//                }

//              m_codebook->SetActiveTxSectorID (m_rdsAntenna, m_rdsSector);
//            }

//          /* We simply transmit the frame on the channel without passing it to the upper layers (We amplify the power) */
//          StartTx (packet, txVector, rxDuration);
//        }
//      else
//        {
//          AmpduTag ampduTag;
//          WifiPreamble preamble = txVector.GetPreambleType ();
//          if (preamble == WIFI_PREAMBLE_NONE && (m_mpdusNum == 0 || m_phyHeaderSuccess == false))
//            {
//              // add interference event for TRN field
//              if (txVector.GetEDMGTrainingFieldLength () > 0 || txVector.GetTrainngFieldLength () > 0)
//                {

//                  Simulator::Schedule(rxDuration, &InterferenceHelper::AddForeignSignal, &m_interference, totalDuration - rxDuration, rxPowerW);
//                  Simulator::Schedule(rxDuration, &DmgWifiPhy::MaybeCcaBusyDuration, this);
//                }
//              m_phyHeaderSuccess = false;
//              m_mpdusNum = 0;
//              NS_LOG_DEBUG ("drop packet because no PLCP preamble/header has been received");
//              NotifyRxDrop (packet);
//              MaybeCcaBusyDuration ();
//              return;
//            }
//          else if (preamble != WIFI_PREAMBLE_NONE && packet->PeekPacketTag (ampduTag) && m_mpdusNum == 0)
//            {
//              //received the first MPDU in an A-MPDU
//              m_mpdusNum = ampduTag.GetRemainingNbOfMpdus ();
//              m_rxMpduReferenceNumber++;
//            }
//          else if (preamble == WIFI_PREAMBLE_NONE && packet->PeekPacketTag (ampduTag) && m_mpdusNum > 0)
//            {
//              //received the other MPDUs that are part of the A-MPDU
//              if (ampduTag.GetRemainingNbOfMpdus () < (m_mpdusNum - 1))
//                {
//                  NS_LOG_DEBUG ("Missing MPDU from the A-MPDU " << m_mpdusNum - ampduTag.GetRemainingNbOfMpdus ());
//                  m_mpdusNum = ampduTag.GetRemainingNbOfMpdus ();
//                }
//              else
//                {
//                  m_mpdusNum--;
//                }
//            }
//          else if (preamble != WIFI_PREAMBLE_NONE && packet->PeekPacketTag (ampduTag) && m_mpdusNum > 0)
//            {
//              NS_LOG_DEBUG ("New A-MPDU started while " << m_mpdusNum << " MPDUs from previous are lost");
//              m_mpdusNum = ampduTag.GetRemainingNbOfMpdus ();
//            }
//          else if (preamble != WIFI_PREAMBLE_NONE && m_mpdusNum > 0 )
//            {
//              NS_LOG_DEBUG ("Didn't receive the last MPDUs from an A-MPDU " << m_mpdusNum);
//              m_mpdusNum = 0;
//            }

//          NS_LOG_DEBUG ("sync to signal (power=" << rxPowerW << "W)");
//          m_currentEvent = event;
//          m_state->SwitchToRx (totalDuration);
//          NS_ASSERT (m_endPhyRxEvent.IsExpired ());
//          NotifyRxBegin (packet);
//          m_interference.NotifyRxStart ();
//          if ((txVector.GetEDMGTrainingFieldLength () > 0) || (txVector.GetTrainngFieldLength () > 0))
//            {
//               m_currentSender = txVector.GetSender ();
//            }
//          if (preamble != WIFI_PREAMBLE_NONE)
//            {
//              NS_ASSERT (m_endPhyRxEvent.IsExpired ());
//              Time preambleAndHeaderDuration = CalculatePhyPreambleAndHeaderDuration (txVector);
//              m_endPhyRxEvent = Simulator::Schedule (preambleAndHeaderDuration, &WifiPhy::StartReceivePacket, this,
//                                                      packet, txVector, mpdutype, event);
//            }

//          NS_ASSERT (m_endRxEvent.IsExpired ());
//          if ((txVector.GetTrainngFieldLength () > 0) || ((txVector.GetEDMGTrainingFieldLength () > 0) && (GetStandard () == WIFI_PHY_STANDARD_80211ay)))
//            {
//              m_endRxEvent = Simulator::Schedule (rxDuration - NanoSeconds (1), &DmgWifiPhy::EndPsduOnlyReceive, this,
//                                                  packet, txVector, preamble, mpdutype, event);
//            }
//          else
//            {
//              m_endRxEvent = Simulator::Schedule (rxDuration, &DmgWifiPhy::EndPsduReceive, this,
//                                                  packet, preamble, mpdutype, event);
//            }
//        }
//    }
//  else
//    {
//      NS_LOG_DEBUG ("drop packet because signal power too Small (" <<
//                    WToDbm (rxPowerW) << "<" << WToDbm (GetEdThresholdW ()) << ")");
//      NotifyRxDrop (packet);
//      // Add interference event for the TRN field.
//      if (txVector.GetEDMGTrainingFieldLength () > 0 || txVector.GetTrainngFieldLength () > 0)
//        {
//          Simulator::Schedule(rxDuration, &InterferenceHelper::AddForeignSignal, &m_interference, totalDuration - rxDuration, rxPowerW);
//          Simulator::Schedule(rxDuration, &DmgWifiPhy::MaybeCcaBusyDuration, this);
//        }
//      m_phyHeaderSuccess = false;
//      MaybeCcaBusyDuration ();
//    }
//}

Time
DmgWifiPhy::GetPreambleDetectionDuration (void)
{
  return NanoSeconds (290);
}

Time
DmgWifiPhy::GetTRN_Field_Duration (WifiTxVector &txVector)
{
  Time trnFieldDuration = Seconds (0);
  if (txVector.GetTrainngFieldLength () > 0)
    {
      trnFieldDuration = txVector.GetTrainngFieldLength () * (AGC_SF_DURATION + TRN_SUBFIELD_DURATION)
          + (txVector.GetTrainngFieldLength ()/4) * TRN_CE_DURATION;
    }
  else if (txVector.GetEDMGTrainingFieldLength () > 0)
    {
      /* Store the duration of the EDMG TRN Subfield and the number of TRN Units appended - needed for calculating the
      start of the A-BFT in the MAC */
      m_beaconTrnFieldLength = txVector.GetEDMGTrainingFieldLength ();
      txVector.edmgTrnSubfieldDuration = CalculateEdmgTrnSubfieldDuration (txVector);
      m_edmgTrnSubfieldDuration = txVector.edmgTrnSubfieldDuration;
      if (txVector.GetPacketType () == TRN_R)
        {
          trnFieldDuration = txVector.edmgTrnSubfieldDuration * txVector.GetEDMGTrainingFieldLength () * EDMG_TRN_UNIT_SIZE;

        }
      else
        {
          trnFieldDuration = txVector.edmgTrnSubfieldDuration * (txVector.Get_TRN_T () +
                                                                 txVector.GetEDMGTrainingFieldLength () * (txVector.Get_EDMG_TRN_P () + txVector.Get_EDMG_TRN_M ()) +
                                                                 txVector.Get_EDMG_TRN_P ());
        }
    }
  NS_LOG_DEBUG ("TRN Field Duration=" << trnFieldDuration);
  return trnFieldDuration;
}

void
DmgWifiPhy::StartReceivePreamble (Ptr<WifiPpdu> ppdu, std::vector<double> rxPowerList)
{
  NS_LOG_FUNCTION (this << *ppdu);
  WifiTxVector txVector = ppdu->GetTxVector ();
  Time rxDuration = ppdu->GetTxDuration ();
  Ptr<const WifiPsdu> psdu = ppdu->GetPsdu ();

  /* Check if the transmission mode is SISO or MIMO */
  double rxPowerW;
  Ptr<Event> event;
  if (rxPowerList.size () == 1)
    {
      /* In SISO mode there is only one value for the received power in the list */
      NS_LOG_INFO ("Receiving packet in SISO mode");
      rxPowerW = rxPowerList.at (0);
      event = m_interference.Add (ppdu, txVector, rxDuration, rxPowerW);
    }
  else
    {
      /* In MIMO mode we make decision regarding frame capture, entering CCA busy mode or preamble detection
       * based on the maximum Rx power */
      NS_LOG_INFO ("Receiving packet in MIMO mode");
      rxPowerW = *(std::max_element (rxPowerList.begin (), rxPowerList.end ()));
      /* During MIMO BFT we transmit packets using multiple transmit chains and applying a spatial
       * expansion with cyclic shift diversity - for now we model this by assuming the transmission is
       * equal to a SISO transmission using the
       * maximum Rx Power, since we are able decode the strongest path and remove the interference from
       * the other Tx antennas
       */
      if (txVector.Get_NUM_STS () == 1)
        event = m_interference.Add (ppdu, txVector, rxDuration, rxPowerW);
      else
        event = m_interference.Add (ppdu, txVector, rxDuration, rxPowerW, rxPowerList);
    }

  Time totalDuration = rxDuration + GetTRN_Field_Duration (txVector);
  m_rxDuration = totalDuration; // Duration of the last received frame including TRN field/.
  Time endRx = Simulator::Now () + totalDuration;

  if (m_state->GetState () == WifiPhyState::OFF)
    {
      NS_LOG_DEBUG ("Cannot start RX because device is OFF");
      if (endRx > (Simulator::Now () + m_state->GetDelayUntilIdle ()))
        {
          MaybeCcaBusyDuration ();
        }
      return;
    }

  if (ppdu->IsTruncatedTx ())
    {
      NS_LOG_DEBUG ("Packet reception stopped because transmitter has been switched off");
      if (endRx > (Simulator::Now () + m_state->GetDelayUntilIdle ()))
        {
          MaybeCcaBusyDuration ();
        }
      return;
    }

  if (!txVector.GetModeInitialized ())
    {
      //If SetRate method was not called above when filling in txVector, this means the PHY does support the rate indicated in PHY SIG headers
      NS_LOG_DEBUG ("drop packet because of unsupported RX mode");
      NotifyRxDrop (psdu, UNSUPPORTED_SETTINGS);
      if (endRx > (Simulator::Now () + m_state->GetDelayUntilIdle ()))
        {
          MaybeCcaBusyDuration ();
        }
      return;
    }

  switch (m_state->GetState ())
    {
    case WifiPhyState::SWITCHING:
      NS_LOG_DEBUG ("drop packet because of channel switching");
      NotifyRxDrop (psdu, CHANNEL_SWITCHING);
      /*
       * Packets received on the upcoming channel are added to the event list
       * during the switching state. This way the medium can be correctly sensed
       * when the device listens to the channel for the first time after the
       * switching e.g. after channel switching, the channel may be sensed as
       * busy due to other devices' transmissions started before the end of
       * the switching.
       */
      if (endRx > (Simulator::Now () + m_state->GetDelayUntilIdle ()))
        {
          //that packet will be noise _after_ the completion of the channel switching.
          //// WIGIG ////
          // Add interference event for TRN field.
          if (txVector.GetEDMGTrainingFieldLength () > 0 || txVector.GetTrainngFieldLength () > 0)
            {
              Simulator::Schedule (rxDuration, &InterferenceHelper::AddForeignSignal, &m_interference, totalDuration - rxDuration, rxPowerW);
              Simulator::Schedule (rxDuration, &DmgWifiPhy::MaybeCcaBusyDuration, this);
            }
          //// WIGIG ////
          MaybeCcaBusyDuration ();
          return;
        }
      break;
    case WifiPhyState::RX:
      NS_ASSERT (m_currentEvent != 0);
      if (m_frameCaptureModel != 0
          && m_frameCaptureModel->IsInCaptureWindow (m_timeLastPreambleDetected)
          && m_frameCaptureModel->CaptureNewFrame (m_currentEvent, event))
        {
          AbortCurrentReception (FRAME_CAPTURE_PACKET_SWITCH);
          NS_LOG_DEBUG ("Switch to new packet");
          StartRx (event, rxPowerW);
        }
      else
        {
          NS_LOG_DEBUG ("Drop packet because already in Rx (power=" <<
                        rxPowerW << "W)");
          NotifyRxDrop (psdu, RXING);
          if (endRx > (Simulator::Now () + m_state->GetDelayUntilIdle ()))
            {
              //that packet will be noise _after_ the reception of the currently-received packet.
              //// WIGIG ////
              // Add interference event for the TRN field.
              if (txVector.GetEDMGTrainingFieldLength () > 0 || txVector.GetTrainngFieldLength () > 0)
                {
                  Simulator::Schedule (rxDuration, &InterferenceHelper::AddForeignSignal, &m_interference, totalDuration - rxDuration, rxPowerW);
                  Simulator::Schedule (rxDuration, &DmgWifiPhy::MaybeCcaBusyDuration, this);
                }
              //// WIGIG ////
              MaybeCcaBusyDuration ();
            }
        }
      break;
    case WifiPhyState::TX:
      NS_LOG_DEBUG ("Drop packet because already in Tx (power=" <<
                    rxPowerW << "W)");
      NotifyRxDrop (psdu, TXING);
      if (endRx > (Simulator::Now () + m_state->GetDelayUntilIdle ()))
        {
          //that packet will be noise _after_ the transmission of the currently-transmitted packet.
          //// WIGIG ////
          // Add interference event for the TRN field.
          if (txVector.GetEDMGTrainingFieldLength () > 0 || txVector.GetTrainngFieldLength () > 0)
            {
              Simulator::Schedule (rxDuration, &InterferenceHelper::AddForeignSignal, &m_interference, totalDuration - rxDuration, rxPowerW);
              Simulator::Schedule (rxDuration, &DmgWifiPhy::MaybeCcaBusyDuration, this);
            }
          //// WIGIG ////
          MaybeCcaBusyDuration ();
        }
      break;
    case WifiPhyState::CCA_BUSY:
      if (m_currentEvent != 0)
        {
          if (m_frameCaptureModel != 0
              && m_frameCaptureModel->IsInCaptureWindow (m_timeLastPreambleDetected)
              && m_frameCaptureModel->CaptureNewFrame (m_currentEvent, event))
            {
              AbortCurrentReception (FRAME_CAPTURE_PACKET_SWITCH);
              NS_LOG_DEBUG ("Switch to new packet");
              StartRx (event, rxPowerW);
            }
          else
            {
              NS_LOG_DEBUG ("Drop packet because already in Rx (power=" <<
                            rxPowerW << "W)");
              NotifyRxDrop (psdu, RXING);
              if (endRx > (Simulator::Now () + m_state->GetDelayUntilIdle ()))
                {
                  // WIGIG Add HERE (NINA)
                  if (txVector.GetEDMGTrainingFieldLength () > 0 || txVector.GetTrainngFieldLength () > 0)
                    {
                      Simulator::Schedule (rxDuration, &InterferenceHelper::AddForeignSignal, &m_interference, totalDuration - rxDuration, rxPowerW);
                      Simulator::Schedule (rxDuration, &DmgWifiPhy::MaybeCcaBusyDuration, this);
                    }
                  //that packet will be noise _after_ the reception of the currently-received packet.
                  MaybeCcaBusyDuration ();
                }
            }
        }
      else
        {
          StartRx (event, rxPowerW);
        }
      break;
    case WifiPhyState::IDLE:
      StartRx (event, rxPowerW);
      break;
    case WifiPhyState::SLEEP:
      NS_LOG_DEBUG ("Drop packet because in sleep mode");
      NotifyRxDrop (psdu, SLEEPING);
      if (endRx > (Simulator::Now () + m_state->GetDelayUntilIdle ()))
        {
          //that packet will be noise _after_ the sleep period.
          //// WIGIG ////
          // Add interference event for the TRN Field.
          if (txVector.GetEDMGTrainingFieldLength () > 0 || txVector.GetTrainngFieldLength () > 0)
            {
              Simulator::Schedule (rxDuration, &InterferenceHelper::AddForeignSignal, &m_interference, totalDuration - rxDuration, rxPowerW);
              Simulator::Schedule (rxDuration, &DmgWifiPhy::MaybeCcaBusyDuration, this);
            }
          //// WIGIG ////
          MaybeCcaBusyDuration ();
        }
      break;
    default:
      NS_FATAL_ERROR ("Invalid WifiPhy state.");
      break;
    }
}

void
DmgWifiPhy::StartReceivePayload (Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << *event);
  NS_ASSERT (m_endPhyRxEvent.IsExpired ());
  NS_ASSERT (m_endRxEvent.IsExpired ());
  WifiTxVector txVector = event->GetTxVector ();
  WifiMode txMode = txVector.GetMode ();

  Time payloadDuration = event->GetEndTime () - event->GetStartTime () - CalculatePhyPreambleAndHeaderDuration (txVector);

  if (txVector.GetNss () > GetMaxSupportedRxSpatialStreams ())
    {
      NS_LOG_DEBUG ("Packet reception could not be started because not enough RX antennas");
      NotifyRxDrop (event->GetPsdu (), UNSUPPORTED_SETTINGS);
    }
  else if ((txVector.GetChannelWidth () >= 2160) && (txVector.GetChannelWidth () > GetChannelWidth ()))
    {
      NS_LOG_DEBUG ("Packet reception could not be started because not enough channel width");
      NotifyRxDrop (event->GetPsdu (), UNSUPPORTED_SETTINGS);
    }
  else if (!IsModeSupported (txMode) && !IsMcsSupported (txMode))
    {
      NS_LOG_DEBUG ("Drop packet because it was sent using an unsupported mode (" << txMode << ")");
      NotifyRxDrop (event->GetPsdu (), UNSUPPORTED_SETTINGS);
    }
  else
    {
      double powerdBm = WToDbm (event->GetRxPowerW ());
      if ((powerdBm < 0.0) && (powerdBm > -110.0))      /* Received channel power indicator (RCPI) measurement */
        {
          m_lastRcpiValue = uint8_t ((powerdBm + 110) * 2);
        }
      else if (powerdBm >= 0)
        {
          m_lastRcpiValue = 220;
        }
      else
        {
          m_lastRcpiValue = 0;
        }
      //// WIGIG ////
      // We switch to Rx for the duration of the payload + TRN field.
      // Need to check if we need to switch to Rx independently at each AGC and TRN subfield.
      m_state->SwitchToRx (payloadDuration + GetTRN_Field_Duration (txVector));
      NS_LOG_DEBUG ("Rx Duration=" << payloadDuration + GetTRN_Field_Duration (txVector));
      NS_LOG_DEBUG ("End Rx=" << Simulator::Now () + payloadDuration + GetTRN_Field_Duration (txVector));
      if (txVector.GetEDMGTrainingFieldLength () > 0 || txVector.GetTrainngFieldLength () > 0)
        {
          Simulator::Schedule (payloadDuration + GetTRN_Field_Duration (txVector), &DmgWifiPhy::EndReceiveTrnField, this, txVector.IsDMGBeacon ());
        }
      //// WIGIG ////
      m_endRxEvent = Simulator::Schedule (payloadDuration, &DmgWifiPhy::EndReceive, this, event);
      NS_LOG_DEBUG ("Receiving PSDU");
      m_phyRxPayloadBeginTrace (txVector, payloadDuration); //this callback (equivalent to PHY-RXSTART primitive) is triggered only if headers have been correctly decoded and that the mode within is supported
      return;
    }
}

void
DmgWifiPhy::EndReceive (Ptr<Event> event)
{
  WifiTxVector txVector = event->GetTxVector ();
  Time psduDuration = event->GetEndTime () - event->GetStartTime () - CalculatePhyPreambleAndHeaderDuration (txVector);
  NS_LOG_FUNCTION (this << *event << psduDuration);
//  NS_ASSERT (GetLastRxEndTime () == Simulator::Now ());
//  NS_ASSERT (event->GetEndTime () == Simulator::Now ());

  double snr = m_interference.CalculatePayloadSnr (event);
  NS_LOG_DEBUG ("snr(dB)=" << RatioToDb (snr) );
  std::vector<bool> statusPerMpdu;
  SignalNoiseDbm signalNoise;

  Ptr<const WifiPsdu> psdu = event->GetPsdu ();
  Time relativeStart = NanoSeconds (0);
  bool receptionOkAtLeastForOneMpdu = false;
  std::pair<bool, SignalNoiseDbm> rxInfo;
  size_t nMpdus = psdu->GetNMpdus ();

  NS_LOG_DEBUG ("PSDU Size=" << psdu->GetSize () << ", PPDU Duration="
                << event->GetPpdu ()->GetTxDuration () << ", #MPDUs=" << nMpdus);

  if (nMpdus > 1)
    {
      //Extract all MPDUs of the A-MPDU to compute per-MPDU PER stats
      Time remainingAmpduDuration = psduDuration;
      Time mpduDuration;
      auto mpdu = psdu->begin ();
      for (size_t i = 0; i < nMpdus && mpdu != psdu->end (); ++mpdu)
        {
          if (i == (nMpdus - 1))
            {
              mpduDuration = remainingAmpduDuration;
            }
          else
            {
              mpduDuration = NanoSeconds ((double (psdu->GetAmpduSubframeSize (i))/double (psdu->GetSize ())) * psduDuration.GetNanoSeconds ());
            }
          remainingAmpduDuration -= mpduDuration;

          NS_LOG_DEBUG ("H1: MPDU Duration=" << mpduDuration);
          NS_LOG_DEBUG ("H2: Remaining Duration=" << remainingAmpduDuration);

          rxInfo = GetReceptionStatus (Create<WifiPsdu> (*mpdu, false),
                                       event, relativeStart, mpduDuration);
          NS_LOG_DEBUG ("Extracted MPDU #" << i << ": size=" << psdu->GetAmpduSubframeSize (i) <<
                        ", duration: " << mpduDuration.GetNanoSeconds () << "ns" <<
                        ", correct reception: " << rxInfo.first <<
                        ", Signal/Noise: " << rxInfo.second.signal << "/" << rxInfo.second.noise << "dBm");
          signalNoise = rxInfo.second; //same information for all MPDUs
          statusPerMpdu.push_back (rxInfo.first);
          receptionOkAtLeastForOneMpdu |= rxInfo.first;

          //Prepare next iteration
          ++i;
          relativeStart += mpduDuration;
        }
      if (!remainingAmpduDuration.IsZero ())
        {
          NS_FATAL_ERROR ("Remaining A-MPDU duration should be zero");
        }
    }
  else
    {
      rxInfo = GetReceptionStatus (psdu, event, relativeStart, psduDuration);
      signalNoise = rxInfo.second; //same information for all MPDUs
      statusPerMpdu.push_back (rxInfo.first);
      receptionOkAtLeastForOneMpdu = rxInfo.first;
    }

  //// WIGIG: Note Check this in the old code
  NotifyRxEnd (psdu);

  //// WIGIG ////
  if ((txVector.GetTrainngFieldLength () > 0) || (txVector.GetEDMGTrainingFieldLength () > 0))
    {
      if (receptionOkAtLeastForOneMpdu)
        {
          /* Save the receive configuration so that we can return to it after the TRN subfields are received */
          m_isQuasiOmni = m_codebook->IsQuasiOmniMode ();
          m_oldAntennaID = m_codebook->GetActiveAntennaID ();
          if (!m_isQuasiOmni)
            {
              m_oldSectorID = m_codebook->GetActiveRxSectorID ();
              if (m_codebook->IsCustomAWVUsed ())
                {
                  m_oldAwvID = m_codebook->GetActiveRxPatternID ();
                }
              else
                {
                  m_oldAwvID = NO_AWV_ID;
                }
            }

          /* The following two variables will indicate if we can receive the following TRN subfields or drop them */
          m_psduSuccess = true;

          NotifyMonitorSniffRx (psdu, GetFrequency (), txVector, signalNoise, statusPerMpdu);
          m_state->ReportPsduRxOk (Copy (psdu), snr, txVector, statusPerMpdu);

          /* Signal that we are in the middle of receiving TRN fields and save the receive power in case of ending the reception due to end of allocation period */
          m_receivingTRNfield = true;
          m_trnReceivePower = event->GetRxPowerW ();
          if (txVector.GetPacketType () == TRN_R)
            {
              if (txVector.IsDMGBeacon ())
                {
                  // APs do not train using the TRN fields appended to beacons from other APs
                  if (!m_isAp)
                    {
                      m_codebook->StartBeaconTraining ();
                    }
                }
              else
                {
                  m_codebook->UseCustomAWV (RefineReceiveSector);
                }
              if (txVector.GetTrainngFieldLength () > 0 )
                {
                  /* Schedule the next change of the AWV for DMG TRN fields*/
                  Simulator::Schedule (AGC_SF_DURATION, &DmgWifiPhy::PrepareForAGC_RX_Reception, this,
                                       txVector.GetTrainngFieldLength () - 1);
                }
            }
          //// NINA ////
          else if (m_suMimoBeamformingTraining && txVector.GetPacketType () == TRN_T)
            {
              // Find and activate the antenna combination needed to receive this packet.
              uint8_t combination = m_txssRepeat - (txVector.GetBrpCdown () / m_peerTxssPackets ) - 1;
              m_codebook->StartMimoRx (combination);
            }
          else if ((m_suMimoBeamformingTraining || m_muMimoBeamformingTraining) && (txVector.GetPacketType () == TRN_RT))
            {
              m_codebook->StartMimoRx (0);
              m_codebook->GetMimoNextSector (true);
            }          
          /* If the received frame has TRN-R subfields, we should sweep antenna configuration at the beginning of each subfield. */
          if (!(m_suMimoBeamformingTraining || m_muMimoBeamformingTraining) && (txVector.GetPacketType () == TRN_RT))
            {
              m_codebook->UseCustomAWV (RefineReceiveSector);
              m_codebook->SetFirstAWV ();
            }
          //// NINA ////
        }
      else
      //// WIGIG ////
        {
          m_state->ReportPsduEndError (Copy (psdu), snr);
          //// WIGIG ////
          m_psduSuccess = false;
          // Add interference event for the TRN field.
          event = m_interference.Add (txVector,
                                      m_state->GetDelayUntilIdle (),
                                      event->GetRxPowerW ());
        }
    }
  //// WIGIG ////
  else
    {
      if (receptionOkAtLeastForOneMpdu)
        {
          NotifyMonitorSniffRx (psdu, GetFrequency (), txVector, signalNoise, statusPerMpdu);
          m_state->SwitchFromRxEndOk (Copy (psdu), snr, txVector, statusPerMpdu);
        }
      else
        {
          m_state->SwitchFromRxEndError (Copy (psdu), snr);
        }
      m_currentEvent = 0;
      MaybeCcaBusyDuration ();
    }
  m_interference.NotifyRxEnd ();
}

void
DmgWifiPhy::PrepareForAGC_RX_Reception (uint8_t remainingAgcRxSubields)
{
  NS_LOG_FUNCTION (this << uint16_t (remainingAgcRxSubields));
  m_codebook->GetNextAWV ();
  remainingAgcRxSubields--;
  if (remainingAgcRxSubields > 0)
    {
      Simulator::Schedule (AGC_SF_DURATION, &DmgWifiPhy::PrepareForAGC_RX_Reception, this, remainingAgcRxSubields);
    }
}

uint8_t
DmgWifiPhy::GetBeaconTrnFieldLength (void) const
{
  return m_beaconTrnFieldLength;
}

Time
DmgWifiPhy::GetBeaconTrnSubfieldDuration (void) const
{
  return m_edmgTrnSubfieldDuration;
}

Time
DmgWifiPhy::GetEdmgTrnFieldDuration (void) const
{
  return m_edmgTrnFieldDuration;
}

void
DmgWifiPhy::SetIsAP (bool ap)
{
  m_isAp = ap;
}

bool
DmgWifiPhy::IsReceivingTRNField (void) const
{
  return m_receivingTRNfield;
}

Time
DmgWifiPhy::GetDelayUntilEndRx (void)
{
  if (m_state->IsStateRx ())
    return GetDelayUntilIdle ();
  else
    return NanoSeconds (0);
}

Time
DmgWifiPhy::CalculateEdmgTrnSubfieldDuration (const WifiTxVector &txVector)
{
  Time subfieldDuration;
  uint8_t nTxChains = txVector.GetNumberOfTxChains ();
  uint8_t N_TRN_NTX;      /* The number of TRN basic subfields for specific number of transmit chains */
  uint16_t trn_bl;
  if (txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_EDMG_CTRL
      || txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_EDMG_SC)
    {
      if (txVector.GetChannelAggregation ())
        {
          if ((nTxChains == 2) || (nTxChains == 4))
            N_TRN_NTX = 1;
          else if ((nTxChains == 6) || (nTxChains == 8))
            N_TRN_NTX = 2;
          else
            NS_FATAL_ERROR ("Unvalid combination of Tx Chains and channel Aggregation");
        }
      else
        {
          if ((nTxChains == 1) || (nTxChains == 2))
            N_TRN_NTX = 1;
          else if ((nTxChains == 3) || (nTxChains == 4))
            N_TRN_NTX = 2;
          else
            N_TRN_NTX = 4;
        }

      if (txVector.Get_TRN_SEQ_LEN () == TRN_SEQ_LENGTH_NORMAL)
        trn_bl = 128;
      else if (txVector.Get_TRN_SEQ_LEN () == TRN_SEQ_LENGTH_LONG)
        trn_bl = 256;
      else if (txVector.Get_TRN_SEQ_LEN () == TRN_SEQ_LENGTH_SHORT)
        trn_bl = 64;
      else
        NS_FATAL_ERROR ("Unvalid TRN Sequence Length");

      subfieldDuration = NanoSeconds (6 * trn_bl * N_TRN_NTX * 0.57);
      return subfieldDuration;
    }
  else if (txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_EDMG_OFDM)
    {
      if ((nTxChains == 1) || (nTxChains == 2))
        N_TRN_NTX = 2;
      else if (nTxChains == 3)
        N_TRN_NTX = 3;
      else if (nTxChains == 4)
        N_TRN_NTX = 4;
      else if ((nTxChains == 5) || (nTxChains == 6))
        N_TRN_NTX = 6;
      else
        N_TRN_NTX = 8;

      if (txVector.Get_TRN_SEQ_LEN () == TRN_SEQ_LENGTH_NORMAL)
        trn_bl = 2*704;
      else if (txVector.Get_TRN_SEQ_LEN () == TRN_SEQ_LENGTH_LONG)
        trn_bl = 4*704;
      else if (txVector.Get_TRN_SEQ_LEN () == TRN_SEQ_LENGTH_SHORT)
        trn_bl = 704;
      else
        NS_FATAL_ERROR ("Unvalid TRN Subfield Sequence Length");

      subfieldDuration = NanoSeconds (trn_bl * N_TRN_NTX * 0.38);
      return subfieldDuration;
    }
  else
    NS_FATAL_ERROR ("Unsupported modulation class");
}

void
DmgWifiPhy::EndTRNReception (void)
{
  NS_LOG_FUNCTION (this);
  if (m_receivingTRNfield)
    {
      m_interference.AddForeignSignal (m_state->GetDelayUntilIdle (), m_receivingTRNfield);
      uint8_t buffer[6] = {0};
      m_currentSender.CopyFrom (buffer);
      m_receivingTRNfield = false;
    }
}

void
DmgWifiPhy::DoConfigureStandard (void)
{
  NS_LOG_FUNCTION (this);
  switch (GetStandard ())
    {
    case WIFI_PHY_STANDARD_80211ad:
      Configure80211ad ();
      break;
    case WIFI_PHY_STANDARD_80211ay:
      Configure80211ay ();
      break;
    default:
      NS_ASSERT (false);
      break;
    }
}

void
DmgWifiPhy::Configure80211ad (void)
{
  /* CTRL-PHY */
  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS0 ());

  /* SC-PHY */
  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS1 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS2 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS3 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS4 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS5 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS6 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS7 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS8 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS9 ());
//  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS9_1 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS10 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS11 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS12 ());
//  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS12_1 ());
//  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS12_2 ());
//  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS12_3 ());
//  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS12_4 ());
//  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS12_5 ());
//  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS12_6 ());

  /* OFDM-PHY */
  if (m_supportOFDM)
    {
      m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS13 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS14 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS15 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS16 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS17 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS18 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS19 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS20 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS21 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS22 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS23 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS24 ());
    }

  /* LP-SC PHY */
  if (m_supportLpSc)
    {
      m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS25 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS26 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS27 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS28 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS29 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS30 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS31 ());
    }
}

void
DmgWifiPhy::Configure80211ay (void)
{
  NS_LOG_FUNCTION (this);

  /* CTRL-PHY */
  m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_MCS0 ());

  /* SC-PHY */
  m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_SC_MCS1 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_SC_MCS2 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_SC_MCS3 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_SC_MCS4 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_SC_MCS5 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_SC_MCS6 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_SC_MCS7 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_SC_MCS8 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_SC_MCS9 ());
//  m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_SC_MCS9_1 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_SC_MCS10 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_SC_MCS11 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_SC_MCS12 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_SC_MCS13 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_SC_MCS14 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_SC_MCS15 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_SC_MCS16 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_SC_MCS17 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_SC_MCS18 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_SC_MCS19 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_SC_MCS20 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_SC_MCS21 ());
//  m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_SC_MCS12_1 ());
//  m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_SC_MCS12_2 ());
//  m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_SC_MCS12_3 ());
//  m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_SC_MCS12_4 ());
//  m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_SC_MCS12_5 ());
//  m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_SC_MCS12_6 ());

  /* OFDM-PHY */
  if (m_supportOFDM)
    {
      m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_OFDM_MCS1 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_OFDM_MCS2 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_OFDM_MCS3 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_OFDM_MCS4 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_OFDM_MCS5 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_OFDM_MCS6 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_OFDM_MCS7 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_OFDM_MCS8 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_OFDM_MCS9 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_OFDM_MCS10 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_OFDM_MCS11 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_OFDM_MCS12 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_OFDM_MCS13 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_OFDM_MCS14 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_OFDM_MCS15 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_OFDM_MCS16 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_OFDM_MCS17 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_OFDM_MCS18 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_OFDM_MCS19 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_OFDM_MCS20 ());
//      m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_OFDM_MCS21 ());
//      m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_OFDM_MCS22 ());
//      m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_OFDM_MCS23 ());
//      m_deviceRateSet.push_back (DmgWifiPhy::GetEDMG_OFDM_MCS24 ());
    }

  /* LP-SC PHY */
  if (m_supportLpSc)
    {
      m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS25 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS26 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS27 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS28 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS29 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS30 ());
      m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS31 ());
    }
}

WifiMode
DmgWifiPhy::GetDmgMcs (uint8_t index)
{
  switch (index)
  {
    case 0:
      return GetDMG_MCS0 ();
    case 1:
      return GetDMG_MCS1 ();
    case 2:
      return GetDMG_MCS2 ();
    case 3:
      return GetDMG_MCS3 ();
    case 4:
      return GetDMG_MCS4 ();
    case 5:
      return GetDMG_MCS5 ();
    case 6:
      return GetDMG_MCS6 ();
    case 7:
      return GetDMG_MCS7 ();
    case 8:
      return GetDMG_MCS8 ();
    case 9:
      return GetDMG_MCS9 ();
    case 10:
      return GetDMG_MCS10 ();
    case 11:
      return GetDMG_MCS11 ();
    case 12:
      return GetDMG_MCS12 ();
    case 13:
      return GetDMG_MCS13 ();
    case 14:
      return GetDMG_MCS14 ();
    case 15:
      return GetDMG_MCS15 ();
    case 16:
      return GetDMG_MCS16 ();
    case 17:
      return GetDMG_MCS17 ();
    case 18:
      return GetDMG_MCS18 ();
    case 19:
      return GetDMG_MCS19 ();
    case 20:
      return GetDMG_MCS20 ();
    case 21:
      return GetDMG_MCS21 ();
    case 22:
      return GetDMG_MCS22 ();
    case 23:
      return GetDMG_MCS23 ();
    case 24:
      return GetDMG_MCS24 ();
    case 25:
      return GetDMG_MCS25 ();
    case 26:
      return GetDMG_MCS26 ();
    case 27:
      return GetDMG_MCS27 ();
    case 28:
      return GetDMG_MCS28 ();
    case 29:
      return GetDMG_MCS29 ();
    case 30:
      return GetDMG_MCS30 ();
    case 31:
      return GetDMG_MCS31 ();
    default:
      NS_ABORT_MSG ("Inexistent (or not supported) index (" << +index << ") requested for DMG PHY");
      return WifiMode ();
    }
}

WifiMode
DmgWifiPhy::GetEdmgMcs (WifiModulationClass modulation, uint8_t index)
{
  if (modulation == WIFI_MOD_CLASS_EDMG_CTRL)
    return GetEDMG_MCS0 ();
  else if (modulation == WIFI_MOD_CLASS_EDMG_SC)
    {
      switch (index)
      {
        case 1:
          return GetEDMG_SC_MCS1 ();
        case 2:
          return GetEDMG_SC_MCS2 ();
        case 3:
          return GetEDMG_SC_MCS3 ();
        case 4:
          return GetEDMG_SC_MCS4 ();
        case 5:
          return GetEDMG_SC_MCS5 ();
        case 6:
          return GetEDMG_SC_MCS6 ();
        case 7:
          return GetEDMG_SC_MCS7 ();
        case 8:
          return GetEDMG_SC_MCS8 ();
        case 9:
          return GetEDMG_SC_MCS9 ();
        case 10:
          return GetEDMG_SC_MCS10 ();
        case 11:
          return GetEDMG_SC_MCS11 ();
        case 12:
          return GetEDMG_SC_MCS12 ();
        case 13:
          return GetEDMG_SC_MCS13 ();
        case 14:
          return GetEDMG_SC_MCS14 ();
        case 15:
          return GetEDMG_SC_MCS15 ();
        case 16:
          return GetEDMG_SC_MCS16 ();
        case 17:
          return GetEDMG_SC_MCS17 ();
        case 18:
          return GetEDMG_SC_MCS18 ();
        case 19:
          return GetEDMG_SC_MCS19 ();
        case 20:
          return GetEDMG_SC_MCS20 ();
        case 21:
          return GetEDMG_SC_MCS21 ();
        default:
          NS_ABORT_MSG ("Inexistent (or not supported) index (" << +index << ") requested for DMG PHY");
          return WifiMode ();
      }
    }
  else if (modulation == WIFI_MOD_CLASS_EDMG_OFDM)
    {
      switch (index)
      {
        case 1:
          return GetEDMG_OFDM_MCS1 ();
        case 2:
          return GetEDMG_OFDM_MCS2 ();
        case 3:
          return GetEDMG_OFDM_MCS3 ();
        case 4:
          return GetEDMG_OFDM_MCS4 ();
        case 5:
          return GetEDMG_OFDM_MCS5 ();
        case 6:
          return GetEDMG_OFDM_MCS6 ();
        case 7:
          return GetEDMG_OFDM_MCS7 ();
        case 8:
          return GetEDMG_OFDM_MCS8 ();
        case 9:
          return GetEDMG_OFDM_MCS9 ();
        case 10:
          return GetEDMG_OFDM_MCS10 ();
        case 11:
          return GetEDMG_OFDM_MCS11 ();
        case 12:
          return GetEDMG_OFDM_MCS12 ();
        case 13:
          return GetEDMG_OFDM_MCS13 ();
        case 14:
          return GetEDMG_OFDM_MCS14 ();
        case 15:
          return GetEDMG_OFDM_MCS15 ();
        case 16:
          return GetEDMG_OFDM_MCS16 ();
        case 17:
          return GetEDMG_OFDM_MCS17 ();
        case 18:
          return GetEDMG_OFDM_MCS18 ();
        case 19:
          return GetEDMG_OFDM_MCS19 ();
        case 20:
          return GetEDMG_OFDM_MCS20 ();
        default:
          NS_ABORT_MSG ("Inexistent (or not supported) index (" << +index << ") requested for DMG PHY");
          return WifiMode ();
      }
    }
  else
    NS_FATAL_ERROR ("Unsupported EDMG modulation type");
}

/**** DMG Control PHY MCS ****/
WifiMode
DmgWifiPhy::GetDMG_MCS0 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS0", 0,
                                     WIFI_MOD_CLASS_DMG_CTRL,
                                     true,
                                     2160000000, 27500000,
                                     WIFI_CODE_RATE_1_2,
                                     2);
  return mode;
}

/**** DMG SC PHY MCSs ****/
WifiMode
DmgWifiPhy::GetDMG_MCS1 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS1", 1,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     true,
                                     2160000000, 385000000,
                                     WIFI_CODE_RATE_1_4, /* 2 repetition */
                                     2);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS2 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS2", 2,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     true,
                                     2160000000, 770000000,
                                     WIFI_CODE_RATE_1_2,
                                     2);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS3 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS3", 3,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     true,
                                     2160000000, 962500000,
                                     WIFI_CODE_RATE_5_8,
                                     2);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS4 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS4", 4,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     true, /* VHT SC MCS1-4 mandatory*/
                                     2160000000, 1155000000,
                                     WIFI_CODE_RATE_3_4,
                                     2);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS5 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS5", 5,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     false,
                                     2160000000, 1251250000,
                                     WIFI_CODE_RATE_13_16,
                                     2);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS6 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS6", 6,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     false,
                                     2160000000, 1540000000,
                                     WIFI_CODE_RATE_1_2,
                                     4);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS7 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS7", 7,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     false,
                                     2160000000, 1925000000,
                                     WIFI_CODE_RATE_5_8,
                                     4);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS8 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS8", 8,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     false,
                                     2160000000, 2310000000ULL,
                                     WIFI_CODE_RATE_3_4,
                                     4);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS9 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS9", 9,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     false,
                                     2160000000, 2502500000ULL,
                                     WIFI_CODE_RATE_13_16,
                                     4);
  return mode;
}

/**** Extended SC MCS ****/
WifiMode
DmgWifiPhy::GetDMG_MCS9_1 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS9_1", 6,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     false,
                                     2160000000, 2695000000ULL,
                                     WIFI_CODE_RATE_7_8,
                                     4);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS10 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS10", 10,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     false,
                                     2160000000, 3080000000ULL,
                                     WIFI_CODE_RATE_1_2,
                                     16);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS11 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS11", 11,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     false,
                                     2160000000, 3850000000ULL,
                                     WIFI_CODE_RATE_5_8,
                                     16);
  return mode;
}

/**** Extended SC MCSs Below ****/
WifiMode
DmgWifiPhy::GetDMG_MCS12 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS12", 12,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     false,
                                     2160000000, 4620000000ULL,
                                     WIFI_CODE_RATE_3_4,
                                     16);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS12_1 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS12_1", 7,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     false,
                                     2160000000, 5005000000ULL,
                                     WIFI_CODE_RATE_13_16,
                                     16);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS12_2 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS12_2", 8,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     false,
                                     2160000000, 5390000000ULL,
                                     WIFI_CODE_RATE_7_8,
                                     16);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS12_3 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS12_3", 9,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     false,
                                     2160000000, 5775000000ULL,
                                     WIFI_CODE_RATE_5_8,
                                     64);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS12_4 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS12_4", 10,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     false,
                                     2160000000, 6390000000ULL,
                                     WIFI_CODE_RATE_3_4,
                                     64);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS12_5 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS12_5", 11,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     false,
                                     2160000000, 7507500000ULL,
                                     WIFI_CODE_RATE_13_16,
                                     64);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS12_6 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS12_6", 12,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     false,
                                     2160000000, 8085000000ULL,
                                     WIFI_CODE_RATE_7_8,
                                     64);
  return mode;
}

/**** OFDM MCSs BELOW ****/
WifiMode
DmgWifiPhy::GetDMG_MCS13 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS13", 13,
                                     WIFI_MOD_CLASS_DMG_OFDM,
                                     true,
                                     2160000000, 693000000ULL,
                                     WIFI_CODE_RATE_1_2,
                                     2);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS14 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS14", 14,
                                     WIFI_MOD_CLASS_DMG_OFDM,
                                     false,
                                     2160000000, 866250000ULL,
                                     WIFI_CODE_RATE_5_8,
                                     2);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS15 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS15", 15,
                                     WIFI_MOD_CLASS_DMG_OFDM,
                                     false,
                                     2160000000, 1386000000ULL,
                                     WIFI_CODE_RATE_1_2,
                                     4);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS16 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS16", 16,
                                     WIFI_MOD_CLASS_DMG_OFDM,
                                     false,
                                     2160000000, 1732500000ULL,
                                     WIFI_CODE_RATE_5_8,
                                     4);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS17 (void)
{
  static WifiMode mode =
  WifiModeFactory::CreateWifiMode ("DMG_MCS17", 17,
                                   WIFI_MOD_CLASS_DMG_OFDM,
                                   false,
                                   2160000000, 2079000000ULL,
                                   WIFI_CODE_RATE_3_4,
                                   4);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS18 (void)
{
  static WifiMode mode =
  WifiModeFactory::CreateWifiMode ("DMG_MCS18", 18,
                                   WIFI_MOD_CLASS_DMG_OFDM,
                                   false,
                                   2160000000, 2772000000ULL,
                                   WIFI_CODE_RATE_1_2,
                                   16);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS19 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS19", 19,
                                     WIFI_MOD_CLASS_DMG_OFDM,
                                     false,
                                     2160000000, 3465000000ULL,
                                     WIFI_CODE_RATE_5_8,
                                     16);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS20 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS20", 20,
                                     WIFI_MOD_CLASS_DMG_OFDM,
                                     false,
                                     2160000000, 4158000000ULL,
                                     WIFI_CODE_RATE_3_4,
                                     16);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS21 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS21", 21,
                                     WIFI_MOD_CLASS_DMG_OFDM,
                                     false,
                                     2160000000, 4504500000ULL,
                                     WIFI_CODE_RATE_13_16,
                                     16);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS22 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS22", 22,
                                     WIFI_MOD_CLASS_DMG_OFDM,
                                     false,
                                     2160000000, 5197500000ULL,
                                     WIFI_CODE_RATE_5_8,
                                     64);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS23 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS23", 23,
                                     WIFI_MOD_CLASS_DMG_OFDM,
                                     false,
                                     2160000000, 6237000000ULL,
                                     WIFI_CODE_RATE_3_4,
                                     64);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS24 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS24", 24,
                                     WIFI_MOD_CLASS_DMG_OFDM,
                                     false,
                                     2160000000, 6756750000ULL,
                                     WIFI_CODE_RATE_13_16,
                                     64);
  return mode;
}

/**** Low Power SC MCSs ****/
WifiMode
DmgWifiPhy::GetDMG_MCS25 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS25", 25,
                                     WIFI_MOD_CLASS_DMG_LP_SC,
                                     false,
                                     2160000000, 626000000,
                                     WIFI_CODE_RATE_13_28,
                                     2);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS26 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS26", 26,
                                     WIFI_MOD_CLASS_DMG_LP_SC,
                                     false,
                                     2160000000, 834000000,
                                     WIFI_CODE_RATE_13_21,
                                     2);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS27 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS27", 27,
                                     WIFI_MOD_CLASS_DMG_LP_SC,
                                     false,
                                     2160000000, 1112000000ULL,
                                     WIFI_CODE_RATE_52_63,
                                     2);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS28 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS28", 28,
                                     WIFI_MOD_CLASS_DMG_LP_SC,
                                     false,
                                     2160000000, 1251000000ULL,
                                     WIFI_CODE_RATE_13_28,
                                     2);
  return mode;
}

WifiMode
DmgWifiPhy::GetDMG_MCS29 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS29", 29,
                                     WIFI_MOD_CLASS_DMG_LP_SC,
                                     false,
                                     2160000000, 1668000000ULL,
                                     WIFI_CODE_RATE_13_21,
                                     4);
  return mode;
}


WifiMode
DmgWifiPhy::GetDMG_MCS30 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS30", 30,
                                     WIFI_MOD_CLASS_DMG_LP_SC,
                                     false,
                                     2160000000, 2224000000ULL,
                                     WIFI_CODE_RATE_52_63,
                                     4);
  return mode;
}


WifiMode
DmgWifiPhy::GetDMG_MCS31 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS31", 31,
                                     WIFI_MOD_CLASS_DMG_LP_SC,
                                     false,
                                     2160000000, 2503000000ULL,
                                     WIFI_CODE_RATE_13_14,
                                     4);
  return mode;
}

/* EDMG Control PHY MCS */
WifiMode
DmgWifiPhy::GetEDMG_MCS0 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_MCS0", 0,
                                     WIFI_MOD_CLASS_EDMG_CTRL,
                                     true,
                                     2160000000, 27500000,
                                     WIFI_CODE_RATE_1_2,
                                     2);
  return mode;
}

/**** EDMG SC PHY MCSs (Normal GI) ****/
WifiMode
DmgWifiPhy::GetEDMG_SC_MCS1 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_SC_MCS1", 1,
                                     WIFI_MOD_CLASS_EDMG_SC,
                                     true,
                                     2160000000, 385000000,
                                     WIFI_CODE_RATE_1_4, /* 2 repetition */
                                     2);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_SC_MCS2 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_SC_MCS2", 2,
                                     WIFI_MOD_CLASS_EDMG_SC,
                                     true,
                                     2160000000, 770000000,
                                     WIFI_CODE_RATE_1_2,
                                     2);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_SC_MCS3 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_SC_MCS3", 3,
                                     WIFI_MOD_CLASS_EDMG_SC,
                                     true,
                                     2160000000, 962500000,
                                     WIFI_CODE_RATE_5_8,
                                     2);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_SC_MCS4 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_SC_MCS4", 4,
                                     WIFI_MOD_CLASS_EDMG_SC,
                                     true,
                                     2160000000, 1155000000,
                                     WIFI_CODE_RATE_3_4,
                                     2);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_SC_MCS5 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_SC_MCS5", 5,
                                     WIFI_MOD_CLASS_EDMG_SC,
                                     false,
                                     2160000000, 1251250000,
                                     WIFI_CODE_RATE_13_16,
                                     2);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_SC_MCS6 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_SC_MCS6", 6,
                                     WIFI_MOD_CLASS_EDMG_SC,
                                     false,
                                     2160000000, 1347500000,
                                     WIFI_CODE_RATE_7_8,
                                     2);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_SC_MCS7 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_SC_MCS7", 7,
                                     WIFI_MOD_CLASS_EDMG_SC,
                                     false,
                                     2160000000, 1540000000,
                                     WIFI_CODE_RATE_1_2,
                                     4);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_SC_MCS8 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_SC_MCS8", 8,
                                     WIFI_MOD_CLASS_EDMG_SC,
                                     false,
                                     2160000000, 1925000000ULL,
                                     WIFI_CODE_RATE_5_8,
                                     4);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_SC_MCS9 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_SC_MCS9", 9,
                                     WIFI_MOD_CLASS_EDMG_SC,
                                     false,
                                     2160000000, 2310000000ULL,
                                     WIFI_CODE_RATE_3_4,
                                     4);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_SC_MCS10 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_SC_MCS10", 10,
                                     WIFI_MOD_CLASS_EDMG_SC,
                                     false,
                                     2160000000, 2502500000ULL,
                                     WIFI_CODE_RATE_13_16,
                                     4);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_SC_MCS11 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_SC_MCS11", 11,
                                     WIFI_MOD_CLASS_EDMG_SC,
                                     false,
                                     2160000000, 2695000000ULL,
                                     WIFI_CODE_RATE_7_8,
                                     4);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_SC_MCS12 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_SC_MCS12", 12,
                                     WIFI_MOD_CLASS_EDMG_SC,
                                     false,
                                     2160000000, 3080000000ULL,
                                     WIFI_CODE_RATE_1_2,
                                     16);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_SC_MCS13 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_SC_MCS13", 13,
                                     WIFI_MOD_CLASS_EDMG_SC,
                                     false,
                                     2160000000, 3850000000ULL,
                                     WIFI_CODE_RATE_5_8,
                                     16);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_SC_MCS14 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_SC_MCS14", 14,
                                     WIFI_MOD_CLASS_EDMG_SC,
                                     false,
                                     2160000000, 4620000000ULL,
                                     WIFI_CODE_RATE_3_4,
                                     16);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_SC_MCS15 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_SC_MCS15", 15,
                                     WIFI_MOD_CLASS_EDMG_SC,
                                     false,
                                     2160000000, 5005000000ULL,
                                     WIFI_CODE_RATE_13_16,
                                     16);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_SC_MCS16 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_SC_MCS16", 16,
                                     WIFI_MOD_CLASS_EDMG_SC,
                                     false,
                                     2160000000, 5390000000ULL,
                                     WIFI_CODE_RATE_7_8,
                                     16);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_SC_MCS17 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_SC_MCS17", 17,
                                     WIFI_MOD_CLASS_EDMG_SC,
                                     false,
                                     2160000000, 4620000000ULL,
                                     WIFI_CODE_RATE_1_2,
                                     64);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_SC_MCS18 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_SC_MCS18", 18,
                                     WIFI_MOD_CLASS_EDMG_SC,
                                     false,
                                     2160000000, 5775000000ULL,
                                     WIFI_CODE_RATE_5_8,
                                     64);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_SC_MCS19 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_SC_MCS19", 19,
                                     WIFI_MOD_CLASS_EDMG_SC,
                                     false,
                                     2160000000, 6930000000ULL,
                                     WIFI_CODE_RATE_3_4,
                                     64);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_SC_MCS20 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_SC_MCS20", 20,
                                     WIFI_MOD_CLASS_EDMG_SC,
                                     false,
                                     2160000000, 7507500000ULL,
                                     WIFI_CODE_RATE_13_16,
                                     64);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_SC_MCS21 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_SC_MCS21", 21,
                                     WIFI_MOD_CLASS_EDMG_SC,
                                     false,
                                     2160000000, 8085000000ULL,
                                     WIFI_CODE_RATE_7_8,
                                     64);
  return mode;
}

/**** EDMG OFDM MCSs BELOW ****/

/*
 * Table 28-69 – Data rate for the EDMG OFDM mode with N SD = 336, 734 IEEE 802.11ay - D5.0
 * NSD = 336, Short GI
 */

WifiMode
DmgWifiPhy::GetEDMG_OFDM_MCS1 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_OFDM_MCS1", 1,
                                     WIFI_MOD_CLASS_EDMG_OFDM,
                                     true,
                                     2160000000, 792000000ULL,
                                     WIFI_CODE_RATE_1_2,
                                     2);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_OFDM_MCS2 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_OFDM_MCS2", 2,
                                     WIFI_MOD_CLASS_EDMG_OFDM,
                                     false,
                                     2160000000, 990000000ULL,
                                     WIFI_CODE_RATE_5_8,
                                     2);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_OFDM_MCS3 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_OFDM_MCS3", 3,
                                     WIFI_MOD_CLASS_EDMG_OFDM,
                                     false,
                                     2160000000, 1188000000ULL,
                                     WIFI_CODE_RATE_3_4,
                                     2);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_OFDM_MCS4 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_OFDM_MCS4", 4,
                                     WIFI_MOD_CLASS_EDMG_OFDM,
                                     false,
                                     2160000000, 1287000000ULL,
                                     WIFI_CODE_RATE_13_16,
                                     2);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_OFDM_MCS5 (void)
{
  static WifiMode mode =
  WifiModeFactory::CreateWifiMode ("EDMG_OFDM_MCS5", 5,
                                   WIFI_MOD_CLASS_EDMG_OFDM,
                                   false,
                                   2160000000, 1386000000ULL,
                                   WIFI_CODE_RATE_7_8,
                                   2);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_OFDM_MCS6 (void)
{
  static WifiMode mode =
  WifiModeFactory::CreateWifiMode ("EDMG_OFDM_MCS6", 6,
                                   WIFI_MOD_CLASS_EDMG_OFDM,
                                   false,
                                   2160000000, 1584000000ULL,
                                   WIFI_CODE_RATE_1_2,
                                   4);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_OFDM_MCS7 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_OFDM_MCS7", 7,
                                     WIFI_MOD_CLASS_EDMG_OFDM,
                                     false,
                                     2160000000, 1980000000ULL,
                                     WIFI_CODE_RATE_5_8,
                                     4);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_OFDM_MCS8 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_OFDM_MCS8", 8,
                                     WIFI_MOD_CLASS_EDMG_OFDM,
                                     false,
                                     2160000000, 2376000000ULL,
                                     WIFI_CODE_RATE_3_4,
                                     4);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_OFDM_MCS9 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_OFDM_MCS9", 9,
                                     WIFI_MOD_CLASS_EDMG_OFDM,
                                     false,
                                     2160000000, 2574000000ULL,
                                     WIFI_CODE_RATE_13_16,
                                     4);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_OFDM_MCS10 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_OFDM_MCS10", 10,
                                     WIFI_MOD_CLASS_EDMG_OFDM,
                                     false,
                                     2160000000, 2772000000ULL,
                                     WIFI_CODE_RATE_7_8,
                                     4);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_OFDM_MCS11 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_OFDM_MCS11", 11,
                                     WIFI_MOD_CLASS_EDMG_OFDM,
                                     false,
                                     2160000000, 3168000000ULL,
                                     WIFI_CODE_RATE_1_2,
                                     16);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_OFDM_MCS12 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_OFDM_MCS12", 12,
                                     WIFI_MOD_CLASS_EDMG_OFDM,
                                     false,
                                     2160000000, 3960000000ULL,
                                     WIFI_CODE_RATE_5_8,
                                     16);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_OFDM_MCS13 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_OFDM_MCS13", 13,
                                     WIFI_MOD_CLASS_EDMG_OFDM,
                                     false,
                                     2160000000, 4752000000ULL,
                                     WIFI_CODE_RATE_3_4,
                                     16);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_OFDM_MCS14 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_OFDM_MCS14", 14,
                                     WIFI_MOD_CLASS_EDMG_OFDM,
                                     false,
                                     2160000000, 5148000000ULL,
                                     WIFI_CODE_RATE_13_16,
                                     16);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_OFDM_MCS15 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_OFDM_MCS15", 15,
                                     WIFI_MOD_CLASS_EDMG_OFDM,
                                     false,
                                     2160000000, 5544000000ULL,
                                     WIFI_CODE_RATE_7_8,
                                     16);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_OFDM_MCS16 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_OFDM_MCS16", 16,
                                     WIFI_MOD_CLASS_EDMG_OFDM,
                                     false,
                                     2160000000, 4752000000ULL,
                                     WIFI_CODE_RATE_1_2,
                                     64);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_OFDM_MCS17 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_OFDM_MCS17", 17,
                                     WIFI_MOD_CLASS_EDMG_OFDM,
                                     false,
                                     2160000000, 5940000000ULL,
                                     WIFI_CODE_RATE_5_8,
                                     64);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_OFDM_MCS18 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_OFDM_MCS18", 18,
                                     WIFI_MOD_CLASS_EDMG_OFDM,
                                     false,
                                     2160000000, 7128000000ULL,
                                     WIFI_CODE_RATE_3_4,
                                     64);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_OFDM_MCS19 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_OFDM_MCS19", 19,
                                     WIFI_MOD_CLASS_EDMG_OFDM,
                                     false,
                                     2160000000, 7722000000ULL,
                                     WIFI_CODE_RATE_13_16,
                                     64);
  return mode;
}

WifiMode
DmgWifiPhy::GetEDMG_OFDM_MCS20 (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("EDMG_OFDM_MCS20", 20,
                                     WIFI_MOD_CLASS_EDMG_OFDM,
                                     false,
                                     2160000000, 8316000000ULL,
                                     WIFI_CODE_RATE_7_8,
                                     64);
  return mode;
}

} //namespace ns3

namespace {

/**
 * Constructor class
 */
static class Constructor
{
public:
  Constructor ()
  {
    ns3::DmgWifiPhy::GetDMG_MCS0 ();
    ns3::DmgWifiPhy::GetDMG_MCS1 ();
    ns3::DmgWifiPhy::GetDMG_MCS2 ();
    ns3::DmgWifiPhy::GetDMG_MCS3 ();
    ns3::DmgWifiPhy::GetDMG_MCS4 ();
    ns3::DmgWifiPhy::GetDMG_MCS5 ();
    ns3::DmgWifiPhy::GetDMG_MCS6 ();
    ns3::DmgWifiPhy::GetDMG_MCS7 ();
    ns3::DmgWifiPhy::GetDMG_MCS8 ();
    ns3::DmgWifiPhy::GetDMG_MCS9 ();
    ns3::DmgWifiPhy::GetDMG_MCS9_1 ();
    ns3::DmgWifiPhy::GetDMG_MCS10 ();
    ns3::DmgWifiPhy::GetDMG_MCS11 ();
    ns3::DmgWifiPhy::GetDMG_MCS12 ();
    ns3::DmgWifiPhy::GetDMG_MCS12_1 ();
    ns3::DmgWifiPhy::GetDMG_MCS12_2 ();
    ns3::DmgWifiPhy::GetDMG_MCS12_3 ();
    ns3::DmgWifiPhy::GetDMG_MCS12_4 ();
    ns3::DmgWifiPhy::GetDMG_MCS12_5 ();
    ns3::DmgWifiPhy::GetDMG_MCS12_6 ();
    ns3::DmgWifiPhy::GetDMG_MCS13 ();
    ns3::DmgWifiPhy::GetDMG_MCS14 ();
    ns3::DmgWifiPhy::GetDMG_MCS15 ();
    ns3::DmgWifiPhy::GetDMG_MCS16 ();
    ns3::DmgWifiPhy::GetDMG_MCS17 ();
    ns3::DmgWifiPhy::GetDMG_MCS18 ();
    ns3::DmgWifiPhy::GetDMG_MCS19 ();
    ns3::DmgWifiPhy::GetDMG_MCS20 ();
    ns3::DmgWifiPhy::GetDMG_MCS21 ();
    ns3::DmgWifiPhy::GetDMG_MCS22 ();
    ns3::DmgWifiPhy::GetDMG_MCS23 ();
    ns3::DmgWifiPhy::GetDMG_MCS24 ();

    ns3::DmgWifiPhy::GetEDMG_MCS0 ();

    ns3::DmgWifiPhy::GetEDMG_SC_MCS1 ();
    ns3::DmgWifiPhy::GetEDMG_SC_MCS2 ();
    ns3::DmgWifiPhy::GetEDMG_SC_MCS3 ();
    ns3::DmgWifiPhy::GetEDMG_SC_MCS4 ();
    ns3::DmgWifiPhy::GetEDMG_SC_MCS5 ();
    ns3::DmgWifiPhy::GetEDMG_SC_MCS6 ();
    ns3::DmgWifiPhy::GetEDMG_SC_MCS7 ();
    ns3::DmgWifiPhy::GetEDMG_SC_MCS8 ();
    ns3::DmgWifiPhy::GetEDMG_SC_MCS9 ();
    ns3::DmgWifiPhy::GetEDMG_SC_MCS10 ();
    ns3::DmgWifiPhy::GetEDMG_SC_MCS11 ();
    ns3::DmgWifiPhy::GetEDMG_SC_MCS12 ();
    ns3::DmgWifiPhy::GetEDMG_SC_MCS13 ();
    ns3::DmgWifiPhy::GetEDMG_SC_MCS14 ();
    ns3::DmgWifiPhy::GetEDMG_SC_MCS15 ();
    ns3::DmgWifiPhy::GetEDMG_SC_MCS16 ();
    ns3::DmgWifiPhy::GetEDMG_SC_MCS17 ();
    ns3::DmgWifiPhy::GetEDMG_SC_MCS18 ();
    ns3::DmgWifiPhy::GetEDMG_SC_MCS19 ();
    ns3::DmgWifiPhy::GetEDMG_SC_MCS20 ();
    ns3::DmgWifiPhy::GetEDMG_SC_MCS21 ();

    ns3::DmgWifiPhy::GetEDMG_OFDM_MCS1 ();
    ns3::DmgWifiPhy::GetEDMG_OFDM_MCS2 ();
    ns3::DmgWifiPhy::GetEDMG_OFDM_MCS3 ();
    ns3::DmgWifiPhy::GetEDMG_OFDM_MCS4 ();
    ns3::DmgWifiPhy::GetEDMG_OFDM_MCS5 ();
    ns3::DmgWifiPhy::GetEDMG_OFDM_MCS6 ();
    ns3::DmgWifiPhy::GetEDMG_OFDM_MCS7 ();
    ns3::DmgWifiPhy::GetEDMG_OFDM_MCS8 ();
    ns3::DmgWifiPhy::GetEDMG_OFDM_MCS9 ();
    ns3::DmgWifiPhy::GetEDMG_OFDM_MCS10 ();
    ns3::DmgWifiPhy::GetEDMG_OFDM_MCS11 ();
    ns3::DmgWifiPhy::GetEDMG_OFDM_MCS12 ();
    ns3::DmgWifiPhy::GetEDMG_OFDM_MCS13 ();
    ns3::DmgWifiPhy::GetEDMG_OFDM_MCS14 ();
    ns3::DmgWifiPhy::GetEDMG_OFDM_MCS15 ();
    ns3::DmgWifiPhy::GetEDMG_OFDM_MCS16 ();
    ns3::DmgWifiPhy::GetEDMG_OFDM_MCS17 ();
    ns3::DmgWifiPhy::GetEDMG_OFDM_MCS18 ();
    ns3::DmgWifiPhy::GetEDMG_OFDM_MCS19 ();
    ns3::DmgWifiPhy::GetEDMG_OFDM_MCS20 ();

  }
} g_constructor; ///< the constructor

}
