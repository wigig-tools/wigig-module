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
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */

#include "ampdu-tag.h"
#include "dmg-wifi-phy.h"
#include "dmg-wifi-channel.h"
#include "wifi-utils.h"
#include "wifi-phy-state-helper.h"
#include "wifi-phy-tag.h"
#include "frame-capture-model.h"
#include "wifi-radio-energy-model.h"

#include "ns3/boolean.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

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
    .AddAttribute ("SupportOfdmPhy", "Whether the DMG STA supports OFDM PHY layer.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&DmgWifiPhy::m_supportOFDM),
                   MakeBooleanChecker ())
    .AddAttribute ("SupportLpScPhy", "Whether the DMG STA supports LP-SC PHY layer.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&DmgWifiPhy::m_supportLpSc),
                   MakeBooleanChecker ())
  ;
  return tid;
}

DmgWifiPhy::DmgWifiPhy ()
{
  NS_LOG_FUNCTION (this);
  m_rdsActivated = false;
  m_lastTxDuration = NanoSeconds (0.0);
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

WifiMode
DmgWifiPhy::GetPlcpHeaderMode (WifiTxVector txVector)
{
  switch (txVector.GetMode ().GetModulationClass ())
    {
    case WIFI_MOD_CLASS_DMG_CTRL:
      return DmgWifiPhy::GetDMG_MCS0 ();

    case WIFI_MOD_CLASS_DMG_SC:
      return DmgWifiPhy::GetDMG_MCS1 ();

    case WIFI_MOD_CLASS_DMG_OFDM:
      return DmgWifiPhy::GetDMG_MCS13 ();

    default:
      NS_FATAL_ERROR ("unsupported modulation class");
      return WifiMode ();
    }
}

void
DmgWifiPhy::SendPacket (Ptr<const Packet> packet, WifiTxVector txVector, Time frameDuration, MpduType mpdutype)
{
  NS_LOG_FUNCTION (this << packet << txVector.GetMode ()
                        << txVector.GetMode ().GetDataRate (txVector)
                        << txVector.GetPreambleType ()
                        << (uint16_t)txVector.GetTxPowerLevel ()
                        << frameDuration
                        << (uint16_t)mpdutype);
  /* Transmission can happen if:
   *  - we are syncing on a packet. It is the responsability of the
   *    MAC layer to avoid doing this but the PHY does nothing to
   *    prevent it.
   *  - we are idle
   */
  NS_ASSERT (!m_state->IsStateTx () && !m_state->IsStateSwitching ());

  bool sendTrnField = false;

  if (m_state->IsStateSleep ())
    {
      NS_LOG_DEBUG ("Dropping packet because in sleep mode");
      NotifyTxDrop (packet);
      return;
    }

  Time txDuration = frameDuration;
  WifiPreamble preamble = txVector.GetPreambleType ();
  NS_ASSERT (txDuration > NanoSeconds (0));

  /* Check if the MPDU is single or last aggregate MPDU */
  if (((mpdutype == NORMAL_MPDU && preamble != WIFI_PREAMBLE_NONE) ||
       (mpdutype == LAST_MPDU_IN_AGGREGATE && preamble == WIFI_PREAMBLE_NONE)) && (txVector.GetTrainngFieldLength () > 0))
    {
      NS_LOG_DEBUG ("Append " << uint16_t (txVector.GetTrainngFieldLength ()) << " TRN Subfields");
      txDuration += txVector.GetTrainngFieldLength () * (AGC_SF_DURATION + TRN_SUBFIELD_DURATION)
                 + (txVector.GetTrainngFieldLength ()/4) * TRN_CE_DURATION;
      NS_LOG_DEBUG ("TxDuration=" << txDuration);
      sendTrnField = true;
    }

  if (m_state->IsStateRx ())
    {
      NS_LOG_DEBUG ("Cancel current reception");
      m_endPlcpRxEvent.Cancel ();
      m_endRxEvent.Cancel ();
      m_interference.NotifyRxEnd ();
    }
  NotifyTxBegin (packet);
  if ((mpdutype == MPDU_IN_AGGREGATE) && (txVector.GetPreambleType () != WIFI_PREAMBLE_NONE))
    {
      //send the first MPDU in an MPDU
      m_txMpduReferenceNumber++;
    }
  MpduInfo aMpdu;
  aMpdu.type = mpdutype;
  aMpdu.mpduRefNumber = m_txMpduReferenceNumber;
  NotifyMonitorSniffTx (packet, GetFrequency (), txVector, aMpdu);
  m_state->SwitchToTx (txDuration, packet, GetPowerDbm (txVector.GetTxPowerLevel ()), txVector);

  Ptr<Packet> newPacket = packet->Copy (); // obtain non-const Packet
  WifiPhyTag oldtag;
  newPacket->RemovePacketTag (oldtag);
  uint8_t isFrameComplete = 1;
  if (m_wifiRadioEnergyModel != 0 && m_wifiRadioEnergyModel->GetMaximumTimeInState (WifiPhyState::TX) < txDuration)
    {
      isFrameComplete = 0;
    }
  WifiPhyTag tag (txVector, mpdutype, isFrameComplete);
  newPacket->AddPacketTag (tag);

  StartTx (newPacket, txVector, frameDuration);

  /* Send TRN Units if beam refinement or tracking is requested */
  if (sendTrnField)
    {
      /* Prepare transmission of the AGC Subfields */
      Simulator::Schedule (frameDuration, &DmgWifiPhy::StartAgcSubfieldsTx, this, txVector);
    }

  /* Record duration of the current transmission */
  m_lastTxDuration = txDuration;
  /* New Trace for Tx End */
  Simulator::Schedule (txDuration, &WifiPhy::NotifyTxEnd, this, packet);
}

void
DmgWifiPhy::StartTx (Ptr<Packet> packet, WifiTxVector txVector, Time txDuration)
{
  NS_LOG_DEBUG ("Start transmission: signal power before antenna gain=" << GetPowerDbm (txVector.GetTxPowerLevel ()) << " dBm");
  m_channel->Send (this, packet, GetPowerDbm (txVector.GetTxPowerLevel ()) + GetTxGain (), txDuration);
}

void
DmgWifiPhy::StartAgcSubfieldsTx (WifiTxVector txVector)
{
  NS_LOG_DEBUG ("Start AGC Subfields transmission: signal power before antenna gain=" <<
                GetPowerDbm (txVector.GetTxPowerLevel ()) << " dBm");
  if (txVector.GetPacketType () == TRN_T)
    {
      /* We are the initiator of the TRN-TX */
      m_codebook->UseCustomAWV ();
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
  m_codebook->UseCustomAWV ();
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
DmgWifiPhy::StartReceiveAgcSubfield (WifiTxVector txVector, double rxPowerDbm)
{
  NS_LOG_FUNCTION (this << txVector.GetMode () << rxPowerDbm);
  double rxPowerW = DbmToW (rxPowerDbm);
  if (m_plcpSuccess && m_state->IsStateRx ())
    {
      /* Add Interference event for AGC Subfield */
      Ptr<Event> event;
      event = m_interference.Add (txVector,
                                  AGC_SF_DURATION,
                                  rxPowerW);

      if (txVector.GetPacketType () == TRN_R)
        {
          /* We are the initiator of the TRN-RX, we switch between different AWVs */
          m_codebook->GetNextAWV ();
        }
    }
  else
    {
      NS_LOG_DEBUG ("Drop AGC Subfield because signal power too Small (" << rxPowerW << "<" << GetEdThresholdW ());
    }
}

void
DmgWifiPhy::StartReceiveCeSubfield (WifiTxVector txVector, double rxPowerDbm)
{
  NS_LOG_FUNCTION (this << txVector.GetMode () << rxPowerDbm);
  double rxPowerW = DbmToW (rxPowerDbm);
  if (m_plcpSuccess && m_state->IsStateRx ())
    {
      /* Add Interference event for TRN-CE Subfield */
      Ptr<Event> event;
      event = m_interference.Add (txVector,
                                  TRN_CE_DURATION,
                                  rxPowerW);
      if (txVector.GetPacketType () == TRN_R)
        {
          /* We are the initiator of the TRN-RX, we switch between different AWVs */
          m_codebook->UseCustomAWV ();
        }
    }
  else
    {
      NS_LOG_DEBUG ("Drop TRN-CE Subfield because signal power too Small (" << rxPowerW << "<" << GetEdThresholdW ());
    }
}

void
DmgWifiPhy::StartReceiveTrnSubfield (WifiTxVector txVector, double rxPowerDbm)
{
  NS_LOG_FUNCTION (this << txVector.GetMode () << rxPowerDbm << uint16_t (txVector.remainingTrnSubfields));
  double rxPowerW = DbmToW (rxPowerDbm);
  if (m_plcpSuccess && m_state->IsStateRx ())
    {
      /* Add Interference event for TRN Subfield */
      Ptr<Event> event;
      event = m_interference.Add (txVector,
                                  TRN_SUBFIELD_DURATION,
                                  rxPowerW);

      if (txVector.GetPacketType () == TRN_R)
        {
          /**
           * We are the initiator of the beam refinement phase and we want to refine the reception pattern.
           * The receiver send all the TRN Subfields using the same beam pattern and the transmitter
           * switches among either its receive sector or through AWVs.
           */

          /* Change Rx Sector for the next TRN Subfield */
          m_codebook->GetNextAWV ();

          /* Schedule an event for the complete reception of this TRN Subfield */
          Simulator::Schedule (TRN_SUBFIELD_DURATION, &DmgWifiPhy::EndReceiveTrnSubfield, this,
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
          Simulator::Schedule (TRN_SUBFIELD_DURATION, &DmgWifiPhy::EndReceiveTrnSubfield, this,
                               m_codebook->GetActiveTxSectorID (), m_codebook->GetActiveAntennaID (),
                               txVector, event);
        }
    }
  else
    {
      NS_LOG_DEBUG ("Drop TRN Subfield because signal power too Small (" << rxPowerW << "<" << GetEdThresholdW ());
    }
}

void
DmgWifiPhy::EndReceiveTrnSubfield (SectorID sectorId, AntennaID antennaId, WifiTxVector txVector, Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (sectorId) << static_cast<uint16_t> (antennaId) << txVector.GetMode ()
                   << uint16_t (txVector.remainingTrnUnits) << uint16_t (txVector.remainingTrnSubfields) << event->GetRxPowerW ());
  /* Calculate SNR and report it to the upper layer */
  double snr = m_interference.CalculatePlcpTrnSnr (event);
  m_reportSnrCallback (antennaId, sectorId, txVector.remainingTrnUnits, txVector.remainingTrnSubfields,
                       snr, (txVector.GetPacketType () == TRN_T));
  /* Check if this is the last TRN subfield in the current transmission */
  if ((txVector.remainingTrnUnits == 0) && (txVector.remainingTrnSubfields == 0))
    {
      EndReceiveTrnField ();
    }
}

void
DmgWifiPhy::EndReceiveTrnField (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (IsStateRx ());
  m_interference.NotifyRxEnd ();
  if (m_plcpSuccess && m_psduSuccess)
    {
      m_state->SwitchFromRxEndOk ();
    }
  else
    {
      m_state->SwitchFromRxEndError ();
    }
}

void
DmgWifiPhy::StartRx (Ptr<Packet> packet, WifiTxVector txVector, MpduType mpdutype, double rxPowerW,
                     Time rxDuration, Time totalDuration,
                     Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << packet << txVector << (uint16_t)mpdutype << rxPowerW << rxDuration);
  if (rxPowerW > GetEdThresholdW ()) //checked here, no need to check in the payload reception (current implementation assumes constant rx power over the packet duration)
    {
      if (m_rdsActivated)
        {
          NS_LOG_DEBUG ("Receiving as RDS in FD-AF Mode");
          /**
           * We are working in Full Duplex-Amplify and Forward. So we receive the packet without decoding it
           * or checking its header. Then we amplify it and redirect it to the the destination.
           * We take a simple approach to model full duplex communication by swapping current steering sector.
           * Another approach would by adding new PHY layer which adds more complexity to the solution.
           */

          if ((mpdutype == NORMAL_MPDU) || (mpdutype == LAST_MPDU_IN_AGGREGATE))
            {
              if ((m_rdsSector == m_srcSector) && (m_rdsAntenna == m_srcAntenna))
                {
                  m_rdsSector = m_dstSector;
                  m_rdsAntenna = m_dstAntenna;
                }
              else
                {
                  m_rdsSector = m_srcSector;
                  m_rdsAntenna = m_srcAntenna;
                }

              m_codebook->SetActiveTxSectorID (m_rdsSector, m_rdsAntenna);
            }

          /* We simply transmit the frame on the channel without passing it to the upper layers (We amplify the power) */
          StartTx (packet, txVector, rxDuration);
        }
      else
        {
          AmpduTag ampduTag;
          WifiPreamble preamble = txVector.GetPreambleType ();
          if (preamble == WIFI_PREAMBLE_NONE && (m_mpdusNum == 0 || m_plcpSuccess == false))
            {
              m_plcpSuccess = false;
              m_mpdusNum = 0;
              NS_LOG_DEBUG ("drop packet because no PLCP preamble/header has been received");
              NotifyRxDrop (packet);
              MaybeCcaBusyDuration ();
              return;
            }
          else if (preamble != WIFI_PREAMBLE_NONE && packet->PeekPacketTag (ampduTag) && m_mpdusNum == 0)
            {
              //received the first MPDU in an A-MPDU
              m_mpdusNum = ampduTag.GetRemainingNbOfMpdus ();
              m_rxMpduReferenceNumber++;
            }
          else if (preamble == WIFI_PREAMBLE_NONE && packet->PeekPacketTag (ampduTag) && m_mpdusNum > 0)
            {
              //received the other MPDUs that are part of the A-MPDU
              if (ampduTag.GetRemainingNbOfMpdus () < (m_mpdusNum - 1))
                {
                  NS_LOG_DEBUG ("Missing MPDU from the A-MPDU " << m_mpdusNum - ampduTag.GetRemainingNbOfMpdus ());
                  m_mpdusNum = ampduTag.GetRemainingNbOfMpdus ();
                }
              else
                {
                  m_mpdusNum--;
                }
            }
          else if (preamble != WIFI_PREAMBLE_NONE && packet->PeekPacketTag (ampduTag) && m_mpdusNum > 0)
            {
              NS_LOG_DEBUG ("New A-MPDU started while " << m_mpdusNum << " MPDUs from previous are lost");
              m_mpdusNum = ampduTag.GetRemainingNbOfMpdus ();
            }
          else if (preamble != WIFI_PREAMBLE_NONE && m_mpdusNum > 0 )
            {
              NS_LOG_DEBUG ("Didn't receive the last MPDUs from an A-MPDU " << m_mpdusNum);
              m_mpdusNum = 0;
            }

          NS_LOG_DEBUG ("sync to signal (power=" << rxPowerW << "W)");
          m_currentEvent = event;
          m_state->SwitchToRx (totalDuration);
          NS_ASSERT (m_endPlcpRxEvent.IsExpired ());
          NotifyRxBegin (packet);
          m_interference.NotifyRxStart ();

          if (preamble != WIFI_PREAMBLE_NONE)
            {
              NS_ASSERT (m_endPlcpRxEvent.IsExpired ());
              Time preambleAndHeaderDuration = CalculatePlcpPreambleAndHeaderDuration (txVector);
              m_endPlcpRxEvent = Simulator::Schedule (preambleAndHeaderDuration, &WifiPhy::StartReceivePacket, this,
                                                      packet, txVector, mpdutype, event);
            }

          NS_ASSERT (m_endRxEvent.IsExpired ());
          if (txVector.GetTrainngFieldLength () == 0)
            {
              m_endRxEvent = Simulator::Schedule (rxDuration, &DmgWifiPhy::EndPsduReceive, this,
                                                  packet, preamble, mpdutype, event);
            }
          else
            {
              m_endRxEvent = Simulator::Schedule (rxDuration, &DmgWifiPhy::EndPsduOnlyReceive, this,
                                                  packet, txVector, preamble, mpdutype, event);
            }
        }
    }
  else
    {
      NS_LOG_DEBUG ("drop packet because signal power too Small (" <<
                    WToDbm (rxPowerW) << "<" << WToDbm (GetEdThresholdW ()) << ")");
      NotifyRxDrop (packet);
      m_plcpSuccess = false;
      MaybeCcaBusyDuration ();
    }
}

void
DmgWifiPhy::StartReceivePreambleAndHeader (Ptr<Packet> packet, double rxPowerW, Time rxDuration)
{
  //This function should be later split to check separately whether plcp preamble and plcp header can be successfully received.
  //Note: plcp preamble reception is not yet modeled.
  if (m_state->GetState () == WifiPhyState::OFF)
    {
      NS_LOG_DEBUG ("Cannot start RX because device is OFF");
      return;
    }

  NS_LOG_FUNCTION (this << packet << WToDbm (rxPowerW) << rxDuration);

  WifiPhyTag tag;
  bool found = packet->RemovePacketTag (tag);
  if (!found)
    {
      NS_FATAL_ERROR ("Received Wi-Fi Signal with no WifiPhyTag");
      return;
    }

  if (tag.GetFrameComplete () == 0)
    {
        NS_LOG_DEBUG ("drop packet because of incomplete frame");
        NotifyRxDrop (packet);
        m_plcpSuccess = false;
        return;
    }

  WifiTxVector txVector = tag.GetWifiTxVector ();
  Time totalDuration;
  if (txVector.GetTrainngFieldLength () > 0)
    {
        totalDuration = txVector.GetTrainngFieldLength () * (AGC_SF_DURATION + TRN_SUBFIELD_DURATION)
                   + (txVector.GetTrainngFieldLength ()/4) * TRN_CE_DURATION;
    }
  totalDuration += rxDuration;
  m_rxDuration = totalDuration; // Duraion of the last received frame.
  Time endRx = Simulator::Now () + totalDuration;

  Ptr<Event> event;
  event = m_interference.Add (packet,
                              txVector,
                              rxDuration,
                              rxPowerW);

  MpduType mpdutype = tag.GetMpduType ();
  switch (m_state->GetState ())
    {
    case WifiPhyState::SWITCHING:
      NS_LOG_DEBUG ("drop packet because of channel switching");
      NotifyRxDrop (packet);
      m_plcpSuccess = false;
      /*
       * Packets received on the upcoming channel are added to the event list
       * during the switching state. This way the medium can be correctly sensed
       * when the device listens to the channel for the first time after the
       * switching e.g. after channel switching, the channel may be sensed as
       * busy due to other devices' tramissions started before the end of
       * the switching.
       */
      if (endRx > Simulator::Now () + m_state->GetDelayUntilIdle ())
        {
          //that packet will be noise _after_ the completion of the
          //channel switching.
          MaybeCcaBusyDuration ();
          return;
        }
      break;
    case WifiPhyState::RX:
      NS_ASSERT (m_currentEvent != 0);
      if (m_frameCaptureModel != 0
          && m_frameCaptureModel->CaptureNewFrame (m_currentEvent, event))
        {
          AbortCurrentReception ();
          NS_LOG_DEBUG ("Switch to new packet");
          StartRx (packet, txVector, mpdutype, rxPowerW, rxDuration, totalDuration, event);
        }
      else
        {
          NS_LOG_DEBUG ("drop packet because already in Rx (power=" <<
                        rxPowerW << "W)");
          NotifyRxDrop (packet);
          if (endRx > Simulator::Now () + m_state->GetDelayUntilIdle ())
            {
              //that packet will be noise _after_ the reception of the
              //currently-received packet.
              MaybeCcaBusyDuration ();
              return;
            }
        }
      break;
    case WifiPhyState::TX:
      NS_LOG_DEBUG ("drop packet because already in Tx (power=" <<
                    rxPowerW << "W)");
      NotifyRxDrop (packet);
      if (endRx > Simulator::Now () + m_state->GetDelayUntilIdle ())
        {
          //that packet will be noise _after_ the transmission of the
          //currently-transmitted packet.
          MaybeCcaBusyDuration ();
          return;
        }
      break;
    case WifiPhyState::CCA_BUSY:
    case WifiPhyState::IDLE:
      StartRx (packet, txVector, mpdutype, rxPowerW, rxDuration, totalDuration, event);
      break;
    case WifiPhyState::SLEEP:
      NS_LOG_DEBUG ("drop packet because in sleep mode");
      NotifyRxDrop (packet);
      m_plcpSuccess = false;
      break;
    default:
      NS_FATAL_ERROR ("Invalid WifiPhy state.");
      break;
    }
}

void
DmgWifiPhy::StartReceivePacket (Ptr<Packet> packet,
                                WifiTxVector txVector,
                                MpduType mpdutype,
                                Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << packet << txVector.GetMode () << txVector.GetPreambleType () << +mpdutype);
  NS_ASSERT (IsStateRx ());
  NS_ASSERT (m_endPlcpRxEvent.IsExpired ());
  WifiMode txMode = txVector.GetMode ();

  InterferenceHelper::SnrPer snrPer;
  snrPer = m_interference.CalculatePlcpHeaderSnrPer (event);

  NS_LOG_DEBUG ("snr(dB)=" << RatioToDb (snrPer.snr) << ", per=" << snrPer.per);

  if (m_random->GetValue () > snrPer.per) //plcp reception succeeded
    {
      if (IsModeSupported (txMode) || IsMcsSupported (txMode))
        {
          double powerdBm = WToDbm (event->GetRxPowerW ());
          NS_LOG_DEBUG ("receiving plcp payload"); //endReceive is already scheduled
          m_plcpSuccess = true;
          if ((powerdBm < 0) && (powerdBm > -110))      /* Received channel power indicator (RCPI) measurement */
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
        }
      else //mode is not allowed
        {
          NS_LOG_DEBUG ("drop packet because it was sent using an unsupported mode (" << txMode << ")");
          NotifyRxDrop (packet);
          m_plcpSuccess = false;
        }
    }
  else //plcp reception failed
    {
      NS_LOG_DEBUG ("drop packet because plcp preamble/header reception failed");
      NotifyRxDrop (packet);
      m_plcpSuccess = false;
    }
}

void
DmgWifiPhy::EndPsduOnlyReceive (Ptr<Packet> packet, WifiTxVector txVector, WifiPreamble preamble,
                                MpduType mpdutype, Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << packet << event);
  NS_ASSERT (IsStateRx ());

  bool isEndOfFrame = ((mpdutype == NORMAL_MPDU && preamble != WIFI_PREAMBLE_NONE) || (mpdutype == LAST_MPDU_IN_AGGREGATE && preamble == WIFI_PREAMBLE_NONE));
  struct InterferenceHelper::SnrPer snrPer;
  snrPer = m_interference.CalculatePlcpPayloadSnrPer (event);

  if (m_plcpSuccess == true)
    {
      NS_LOG_DEBUG ("mode=" << (event->GetPayloadMode ().GetDataRate ()) <<
                    ", snr(dB)=" << RatioToDb(snrPer.snr) << ", per=" << snrPer.per << ", size=" << packet->GetSize ());
      double rnd = m_random->GetValue ();
      m_psduSuccess = (rnd > snrPer.per);
      if (m_psduSuccess)
        {
          NotifyRxEnd (packet);
          SignalNoiseDbm signalNoise;
          signalNoise.signal = RatioToDb (event->GetRxPowerW ()) + 30;
          signalNoise.noise = RatioToDb (event->GetRxPowerW () / snrPer.snr) - GetRxNoiseFigure () + 30;
          MpduInfo aMpdu;
          aMpdu.type = mpdutype;
          aMpdu.mpduRefNumber = m_rxMpduReferenceNumber;
          NotifyMonitorSniffRx (packet, GetFrequency (), event->GetTxVector (), aMpdu, signalNoise);
          m_state->ReportPsduEndOk (packet, snrPer.snr, event->GetTxVector ());
        }
      else
        {
          /* failure. */
          NS_LOG_DEBUG ("drop packet because the probability to receive it = " << rnd << " is lower than " << snrPer.per);
          NotifyRxDrop (packet);
          m_state->ReportPsduEndError (packet, snrPer.snr);
        }
    }
  else
    {
      m_state->ReportPsduEndError (packet, snrPer.snr);
    }

  if (preamble == WIFI_PREAMBLE_NONE && mpdutype == LAST_MPDU_IN_AGGREGATE)
    {
      m_plcpSuccess = false;
    }

  if (isEndOfFrame && (txVector.GetPacketType () == TRN_R))
    {
      /* We are the initiator of the Beam refinement and the the responder has TRN-R Subfields, we start changing AWVs  */
      m_codebook->UseCustomAWV ();
      /* Schedule the next change of the AWV */
      Simulator::Schedule (AGC_SF_DURATION, &DmgWifiPhy::PrepareForAGC_RX_Reception, this, txVector.GetTrainngFieldLength () - 1);
    }
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

Time
DmgWifiPhy::GetPlcpHeaderDuration (WifiTxVector txVector)
{
  WifiPreamble preamble = txVector.GetPreambleType ();
  if (preamble == WIFI_PREAMBLE_NONE)
    {
      return MicroSeconds (0);
    }
  switch (txVector.GetMode ().GetModulationClass ())
    {
    case WIFI_MOD_CLASS_DMG_CTRL:
      /* From Annex L (L.5.2.5) */
      return NanoSeconds (4654);
    case WIFI_MOD_CLASS_DMG_SC:
    case WIFI_MOD_CLASS_DMG_LP_SC:
      /* From Table 21-4 in 802.11ad spec 21.3.4 */
      return NanoSeconds (582);
    case WIFI_MOD_CLASS_DMG_OFDM:
      /* From Table 21-4 in 802.11ad spec 21.3.4 */
      return NanoSeconds (242);
    default:
      NS_FATAL_ERROR ("unsupported modulation class");
      return MicroSeconds (0);
    }
}

Time
DmgWifiPhy::GetPlcpPreambleDuration (WifiTxVector txVector)
{
  WifiPreamble preamble = txVector.GetPreambleType ();
  if (preamble == WIFI_PREAMBLE_NONE)
    {
      return MicroSeconds (0);
    }
  switch (txVector.GetMode ().GetModulationClass ())
    {
    case WIFI_MOD_CLASS_DMG_CTRL:
      // CTRL Preamble = (6400 + 1152) Samples * Tc (Chip Time for SC), Tc = Tccp = 0.57ns.
      // CTRL Preamble = 4.291 micro seconds.
      return NanoSeconds (4291);

    case WIFI_MOD_CLASS_DMG_SC:
    case WIFI_MOD_CLASS_DMG_LP_SC:
      // SC Preamble = 3328 Samples (STF: 2176 + CEF: 1152) * Tc (Chip Time for SC), Tc = 0.57ns.
      // SC Preamble = 1.89 micro seconds.
      return NanoSeconds (1891);

    case WIFI_MOD_CLASS_DMG_OFDM:
      // OFDM Preamble = 4992 Samples (STF: 2176 + CEF: 1152) * Ts (Chip Time for OFDM), Tc = 0.38ns.
      // OFDM Preamble = 1.89 micro seconds.
      return NanoSeconds (1891);

    default:
      NS_FATAL_ERROR ("unsupported modulation class");
      return MicroSeconds (0);
    }
}

Time
DmgWifiPhy::GetPayloadDuration (uint32_t size, WifiTxVector txVector, uint16_t frequency, MpduType mpdutype, uint8_t incFlag)
{
  WifiMode payloadMode = txVector.GetMode ();
  NS_LOG_FUNCTION (size << payloadMode);

  if (payloadMode.GetModulationClass () == WIFI_MOD_CLASS_DMG_CTRL)
    {
      if (txVector.GetTrainngFieldLength () == 0)
        {
          uint32_t Ncw;                       /* Number of LDPC codewords. */
          uint32_t Ldpcw;                     /* Number of bits in the second and any subsequent codeword except the last. */
          uint32_t Ldplcw;                    /* Number of bits in the last codeword. */
          uint32_t DencodedSymmbols;          /* Number of differentailly encoded payload symbols. */
          uint32_t Chips;                     /* Number of chips (After spreading using Ga32 Golay Sequence). */
          uint32_t Nbits = (size - 8) * 8;    /* Number of bits in the payload part. */

          Ncw = 1 + (uint32_t) ceil ((double (size) - 6) * 8/168);
          Ldpcw = (uint32_t) ceil ((double (size) - 6) * 8/(Ncw - 1));
          Ldplcw = (size - 6) * 8 - (Ncw - 2) * Ldpcw;
          DencodedSymmbols = (672 - (504 - Ldpcw)) * (Ncw - 2) + (672 - (504 - Ldplcw));
          Chips = DencodedSymmbols * 32;

          /* Make sure the result is in nanoseconds. */
          double ret = double (Chips)/1.76;
          NS_LOG_DEBUG ("bits " << Nbits << " Diff encoded Symmbols " << DencodedSymmbols << " rate " << payloadMode.GetDataRate() << " Payload Time " << ret << " ns");

          return NanoSeconds (ceil (ret));
        }
      else
        {
          uint32_t Ncw;                       /* Number of LDPC codewords. */
          Ncw = 1 + (uint32_t) ceil ((double (size) - 6) * 8/168);
          return NanoSeconds (ceil (double ((88 + (size - 6) * 8 + Ncw * 168) * 0.57 * 32)));
        }
    }
  else if (payloadMode.GetModulationClass () == WIFI_MOD_CLASS_DMG_LP_SC)
    {
//        uint32_t Nbits = (size * 8);  /* Number of bits in the payload part. */
//        uint32_t Nrsc;                /* The total number of Reed Solomon codewords */
//        uint32_t Nrses;               /* The total number of Reed Solomon encoded symbols */
//        Nrsc = (uint32_t) ceil(Nbits/208);
//        Nrses = Nbits + Nrsc * 16;

//        uint32_t Nsbc;                 /* Short Block code Size */
//        if (payloadMode.GetCodeRate() == WIFI_CODE_RATE_13_28)
//          Nsbc = 16;
//        else if (payloadMode.GetCodeRate() == WIFI_CODE_RATE_13_21)
//          Nsbc = 12;
//        else if (payloadMode.GetCodeRate() == WIFI_CODE_RATE_52_63)
//          Nsbc = 9;
//        else if (payloadMode.GetCodeRate() == WIFI_CODE_RATE_13_14)
//          Nsbc = 8;
//        else
//          NS_FATAL_ERROR("unsupported code rate");

//        uint32_t Ncbps;               /* Ncbps = Number of coded bits per symbol. Check Table 21-21 for different constellations. */
//        if (payloadMode.GetConstellationSize() == 2)
//          Ncbps = 336;
//        else if (payloadMode.GetConstellationSize() == 4)
//          Ncbps = 2 * 336;
//          NS_FATAL_ERROR("unsupported constellation size");

//        uint32_t Neb;                 /* Total number of encoded bits */
//        uint32_t Nblks;               /* Total number of 512 blocks containing 392 data symbols */
//        Neb = Nsbc * Nrses;
//        Nblks = (uint32_t) ceil(neb/());
      return NanoSeconds (0);
    }
  else if (payloadMode.GetModulationClass () == WIFI_MOD_CLASS_DMG_SC)
    {
      /* 21.3.4 Timeing Related Parameters, Table 21-4 TData = (Nblks * 512 + 64) * Tc. */
      /* 21.6.3.2.3.3 (4), Compute Nblks = The number of symbol blocks. */

      uint32_t Ncbpb; // Ncbpb = Number of coded bits per symbol block. Check Table 21-20 for different constellations.
      if (payloadMode.GetConstellationSize () == 2)
        Ncbpb = 448;
      else if (payloadMode.GetConstellationSize () == 4)
        Ncbpb = 2 * 448;
      else if (payloadMode.GetConstellationSize () == 16)
        Ncbpb = 4 * 448;
      else if (payloadMode.GetConstellationSize () == 64)
        Ncbpb = 6 * 448;
      else
        NS_FATAL_ERROR ("unsupported constellation size");

      uint32_t Nbits = (size * 8); /* Nbits = Number of bits in the payload part. */
      uint32_t Ncbits;             /* Ncbits = Number of coded bits in the payload part. */

      if (payloadMode.GetCodeRate () == WIFI_CODE_RATE_1_4)
        Ncbits = Nbits * 4;
      else if (payloadMode.GetCodeRate () == WIFI_CODE_RATE_1_2)
        Ncbits = Nbits * 2;
      else if (payloadMode.GetCodeRate () == WIFI_CODE_RATE_13_16)
        Ncbits = (uint32_t) ceil (double (Nbits) * 16.0 / 13);
      else if (payloadMode.GetCodeRate () == WIFI_CODE_RATE_3_4)
        Ncbits = (uint32_t) ceil (double (Nbits) * 4.0 / 3);
      else if (payloadMode.GetCodeRate () == WIFI_CODE_RATE_5_8)
        Ncbits = (uint32_t) ceil (double (Nbits) * 8.0 / 5);
      else if (payloadMode.GetCodeRate () == WIFI_CODE_RATE_7_8)
        Ncbits = (uint32_t) ceil (double (Nbits) * 8.0 / 7);
      else
        NS_FATAL_ERROR ("unsupported code rate");

      uint16_t Lcw; /* The LDPC codeword length. */
      if (payloadMode.GetCodeRate () == WIFI_CODE_RATE_7_8)
        Lcw = 624;
      else
        Lcw = 672;

      uint32_t Ncw = (uint32_t) ceil (double (Ncbits) / double (Lcw));  /* Ncw = The number of LDPC codewords. */
      uint32_t Nblks = (uint32_t) ceil (double (Ncw) * double (Lcw) / Ncbpb);  /* Nblks = The number of symbol blocks. */

      /* Make sure the result is in nanoseconds. */
      uint32_t tData = lrint (ceil ((double (Nblks) * 512 + 64) / 1.76)); /* The duration of the data part */
      NS_LOG_DEBUG ("bits " << Nbits << " cbits " << Ncbits
                    << " Ncw " << Ncw
                    << " Nblks " << Nblks
                    << " rate " << payloadMode.GetDataRate() << " Payload Time " << tData << " ns");

      if (txVector.GetTrainngFieldLength () != 0)
        {
          if (tData < OFDMSCMin)
            tData = OFDMSCMin;
        }
      return NanoSeconds (tData);
    }
  else if (payloadMode.GetModulationClass () == WIFI_MOD_CLASS_DMG_OFDM)
    {
      /* 21.3.4 Timeing Related Parameters, Table 21-4 TData = Nsym * Tsys(OFDM) */
      /* 21.5.3.2.3.3 (5), Compute Nsym = Number of OFDM Symbols */

      uint32_t Ncbps; // Ncbps = Number of coded bits per symbol. Check Table 21-20 for different constellations.
      if (payloadMode.GetConstellationSize () == 2)
        Ncbps = 336;
      else if (payloadMode.GetConstellationSize () == 4)
        Ncbps = 2 * 336;
      else if (payloadMode.GetConstellationSize () == 16)
        Ncbps = 4 * 336;
      else if (payloadMode.GetConstellationSize () == 64)
        Ncbps = 6 * 336;
      else
        NS_FATAL_ERROR ("unsupported constellation size");

      uint32_t Nbits = (size * 8); /* Nbits = Number of bits in the payload part. */
      uint32_t Ncbits;             /* Ncbits = Number of coded bits in the payload part. */

      if (payloadMode.GetCodeRate () == WIFI_CODE_RATE_1_4)
        Ncbits = Nbits * 4;
      else if (payloadMode.GetCodeRate () == WIFI_CODE_RATE_1_2)
        Ncbits = Nbits * 2;
      else if (payloadMode.GetCodeRate () == WIFI_CODE_RATE_13_16)
        Ncbits = (uint32_t) ceil (double (Nbits) * 16.0 / 13);
      else if (payloadMode.GetCodeRate () == WIFI_CODE_RATE_3_4)
        Ncbits = (uint32_t) ceil (double (Nbits) * 4.0 / 3);
      else if (payloadMode.GetCodeRate () == WIFI_CODE_RATE_5_8)
        Ncbits = (uint32_t) ceil (double (Nbits) * 8.0 / 5);
      else
        NS_FATAL_ERROR ("unsupported code rate");

      uint32_t Ncw = (uint32_t) ceil (double (Ncbits) / 672.0);         /* Ncw = The number of LDPC codewords.  */
      uint32_t Nsym = (uint32_t) ceil (double (Ncw * 672.0) / Ncbps);   /* Nsym = Number of OFDM Symbols. */

      /* Make sure the result is in nanoseconds */
      uint32_t tData;       /* The duration of the data part */
      tData = Nsym * 242;   /* Tsys(OFDM) = 242ns */
      NS_LOG_DEBUG ("bits " << Nbits << " cbits " << Ncbits << " rate " << payloadMode.GetDataRate() << " Payload Time " << tData << " ns");

      if (txVector.GetTrainngFieldLength () != 0)
        {
          if (tData < OFDMBRPMin)
            tData = OFDMBRPMin;
        }
      return NanoSeconds (tData);
    }
  else
    {
      NS_FATAL_ERROR ("unsupported modulation class");
      return MicroSeconds (0);
    }
}

void
DmgWifiPhy::DoConfigureStandard (void)
{
  NS_LOG_FUNCTION (this);

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
  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS9_1 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS10 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS11 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS12 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS12_1 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS12_2 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS12_3 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS12_4 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS12_5 ());
  m_deviceRateSet.push_back (DmgWifiPhy::GetDMG_MCS12_6 ());

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

/**** DMG Control PHY MCS ****/
WifiMode
DmgWifiPhy::GetDMG_MCS0 ()
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
DmgWifiPhy::GetDMG_MCS1 ()
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
DmgWifiPhy::GetDMG_MCS2 ()
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
DmgWifiPhy::GetDMG_MCS3 ()
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
DmgWifiPhy::GetDMG_MCS4 ()
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
DmgWifiPhy::GetDMG_MCS5 ()
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
DmgWifiPhy::GetDMG_MCS6 ()
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
DmgWifiPhy::GetDMG_MCS7 ()
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
DmgWifiPhy::GetDMG_MCS8 ()
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
DmgWifiPhy::GetDMG_MCS9 ()
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
DmgWifiPhy::GetDMG_MCS9_1 ()
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
DmgWifiPhy::GetDMG_MCS10 ()
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
DmgWifiPhy::GetDMG_MCS11 ()
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
DmgWifiPhy::GetDMG_MCS12 ()
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
DmgWifiPhy::GetDMG_MCS12_1 ()
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
DmgWifiPhy::GetDMG_MCS12_2 ()
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
DmgWifiPhy::GetDMG_MCS12_3 ()
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
DmgWifiPhy::GetDMG_MCS12_4 ()
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
DmgWifiPhy::GetDMG_MCS12_5 ()
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
DmgWifiPhy::GetDMG_MCS12_6 ()
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
DmgWifiPhy::GetDMG_MCS13 ()
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
DmgWifiPhy::GetDMG_MCS14 ()
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
DmgWifiPhy::GetDMG_MCS15 ()
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
DmgWifiPhy::GetDMG_MCS16 ()
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
DmgWifiPhy::GetDMG_MCS17 ()
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
DmgWifiPhy::GetDMG_MCS18 ()
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
DmgWifiPhy::GetDMG_MCS19 ()
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
DmgWifiPhy::GetDMG_MCS20 ()
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
DmgWifiPhy::GetDMG_MCS21 ()
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
DmgWifiPhy::GetDMG_MCS22 ()
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
DmgWifiPhy::GetDMG_MCS23 ()
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
DmgWifiPhy::GetDMG_MCS24 ()
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
DmgWifiPhy::GetDMG_MCS25 ()
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
DmgWifiPhy::GetDMG_MCS26 ()
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
DmgWifiPhy::GetDMG_MCS27 ()
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
DmgWifiPhy::GetDMG_MCS28 ()
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
DmgWifiPhy::GetDMG_MCS29 ()
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
DmgWifiPhy::GetDMG_MCS30 ()
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
DmgWifiPhy::GetDMG_MCS31 ()
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
  }
} g_constructor; ///< the constructor

}
