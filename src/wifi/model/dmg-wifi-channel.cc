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

#include "ns3/boolean.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "dmg-wifi-channel.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "wifi-utils.h"
#include <fstream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DmgWifiChannel");

NS_OBJECT_ENSURE_REGISTERED (DmgWifiChannel);

TypeId
DmgWifiChannel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DmgWifiChannel")
    .SetParent<Channel> ()
    .SetGroupName ("Wifi")
    .AddConstructor<DmgWifiChannel> ()
    .AddAttribute ("PropagationLossModel", "A pointer to the propagation loss model attached to this channel.",
                   PointerValue (),
                   MakePointerAccessor (&DmgWifiChannel::m_loss),
                   MakePointerChecker<PropagationLossModel> ())
    .AddAttribute ("PropagationDelayModel", "A pointer to the propagation delay model attached to this channel.",
                   PointerValue (),
                   MakePointerAccessor (&DmgWifiChannel::m_delay),
                   MakePointerChecker<PropagationDelayModel> ())
    /* New trace sources for DMG PLCP */
    .AddTraceSource ("PhyActivityTracker",
                     "Trace source for transmitting/receiving PLCP field (PHY Tracker).",
                     MakeTraceSourceAccessor (&DmgWifiChannel::m_phyActivityTrace),
                     "ns3::DmgWifiChannel::PhyActivityTracedCallback")
  ;
  return tid;
}

DmgWifiChannel::DmgWifiChannel ()
  : m_blockage (0),
    m_packetDropper (0),
    m_experimentalMode (false)
{
}

DmgWifiChannel::~DmgWifiChannel ()
{
  NS_LOG_FUNCTION (this);
  m_phyList.clear ();
}

void
DmgWifiChannel::SetPropagationLossModel (const Ptr<PropagationLossModel> loss)
{
  m_loss = loss;
}

void
DmgWifiChannel::SetPropagationDelayModel (const Ptr<PropagationDelayModel> delay)
{
  m_delay = delay;
}

void
DmgWifiChannel::AddBlockage (double (*blockage)(), Ptr<WifiPhy> srcWifiPhy, Ptr<WifiPhy> dstWifiPhy)
{
  m_blockage = blockage;
  m_srcWifiPhy = srcWifiPhy;
  m_dstWifiPhy = dstWifiPhy;
}

void
DmgWifiChannel::RemoveBlockage (void)
{
  m_blockage = 0;
  m_srcWifiPhy = 0;
  m_dstWifiPhy = 0;
}

void
DmgWifiChannel::AddPacketDropper (bool (*dropper)(), Ptr<WifiPhy> srcWifiPhy, Ptr<WifiPhy> dstWifiPhy)
{
  m_packetDropper = dropper;
  m_srcWifiPhy = srcWifiPhy;
  m_dstWifiPhy = dstWifiPhy;
}

void
DmgWifiChannel::RemovePacketDropper (void)
{
  m_packetDropper = 0;
  m_srcWifiPhy = 0;
  m_dstWifiPhy = 0;
}

void
DmgWifiChannel::RecordPhyActivity (uint32_t srcID, uint32_t dstID, Time duration, double power,
                                   PLCP_FIELD_TYPE fieldType, ACTIVITY_TYPE activityType) const
{
  m_phyActivityTrace (srcID, dstID, duration, power, fieldType, activityType);
}

void
DmgWifiChannel::LoadReceivedSignalStrengthFile (std::string fileName, Time updateFreuqnecy)
{
  NS_LOG_FUNCTION (this << "Loading Received signal strength file" << fileName);
  std::ifstream file;
  file.open (fileName.c_str (), std::ifstream::in);
  NS_ASSERT_MSG (file.good (), " file not found");
  std::string line;
  double strength;
  while (std::getline (file, line))
    {
      std::stringstream value (line);
      value >> strength;
      m_receivedSignalStrength.push_back (strength);
    }
  file.close ();
  m_currentSignalStrengthIndex = 0;
  m_experimentalMode = true;
  m_updateFrequency = updateFreuqnecy;
  /* Schedule Update event */
  Simulator::Schedule (m_updateFrequency, &DmgWifiChannel::UpdateSignalStrengthValue, this);
}

void
DmgWifiChannel::UpdateSignalStrengthValue (void)
{
  NS_LOG_FUNCTION (this);
  m_currentSignalStrengthIndex++;
  if (m_currentSignalStrengthIndex == m_receivedSignalStrength.size ())
    {
      m_currentSignalStrengthIndex = 0;
    }
  Simulator::Schedule (m_updateFrequency, &DmgWifiChannel::UpdateSignalStrengthValue, this);
}

void
DmgWifiChannel::Send (Ptr<DmgWifiPhy> sender, Ptr<const Packet> packet, double txPowerDbm, Time duration) const
{
  NS_LOG_FUNCTION (this << sender << packet << txPowerDbm << duration.GetSeconds ());
  Ptr<MobilityModel> senderMobility = sender->GetMobility ();
  NS_ASSERT (senderMobility != 0);
  for (PhyList::const_iterator i = m_phyList.begin (); i != m_phyList.end (); i++)
    {
      if (sender != (*i))
        {
          //For now don't account for inter channel interference nor channel bonding
          if ((*i)->GetChannelNumber () != sender->GetChannelNumber ())
            {
              continue;
            }

          /* Packet Dropper */
          if ((m_packetDropper != 0) && ((m_srcWifiPhy == sender) && (m_dstWifiPhy == (*i))))
            {
              if (m_packetDropper ())
                {
                  continue;
                }
            }

          Vector sender_pos = senderMobility->GetPosition ();
          Ptr<Codebook> senderCodebook = sender->GetCodebook ();
          Ptr<MobilityModel> receiverMobility= (*i)->GetMobility ()->GetObject<MobilityModel> ();
          Time delay = m_delay->GetDelay (senderMobility, receiverMobility);
          double rxPowerDbm;
          double azimuthTx = CalculateAzimuthAngle (sender_pos, receiverMobility->GetPosition ());
          double azimuthRx = CalculateAzimuthAngle (receiverMobility->GetPosition (), sender_pos);
          double gtx = senderCodebook->GetTxGainDbi (azimuthTx);        // Sender's antenna gain in dBi.
          double grx = (*i)->GetCodebook ()->GetRxGainDbi (azimuthRx);  // Receiver's antenna gain in dBi.

          NS_LOG_DEBUG ("POWER: azimuthTx=" << azimuthTx
                        << ", azimuthRx=" << azimuthRx
                        << ", txPowerDbm=" << txPowerDbm
                        << ", RxPower=" << m_loss->CalcRxPower (txPowerDbm, senderMobility, receiverMobility)
                        << ", Gtx=" << gtx
                        << ", Grx=" << grx);

          if (m_experimentalMode)
            {
              rxPowerDbm = m_receivedSignalStrength[m_currentSignalStrengthIndex];
            }
          else
            {
              rxPowerDbm = m_loss->CalcRxPower (txPowerDbm, senderMobility, receiverMobility) + gtx + grx;
            }

          /* External Attenuator */
          if ((m_blockage != 0) &&
              (((m_srcWifiPhy == sender) && (m_dstWifiPhy == (*i))) ||
               ((m_srcWifiPhy == (*i)) && (m_dstWifiPhy == sender))))
            {
              NS_LOG_DEBUG ("Blockage is inserted");
              rxPowerDbm += m_blockage ();
            }

          NS_LOG_DEBUG ("propagation: txPower=" << txPowerDbm << "dbm, rxPower=" << rxPowerDbm << "dbm, " <<
                        "distance=" << senderMobility->GetDistanceFrom (receiverMobility) << "m, delay=" << delay);
          Ptr<Packet> copy = packet->Copy ();
          Ptr<NetDevice> dstNetDevice = (*i)->GetDevice ();
          uint32_t dstNode;
          if (dstNetDevice == 0)
            {
              dstNode = 0xffffffff;
            }
          else
            {
              dstNode = dstNetDevice->GetNode ()->GetId ();
            }

          Simulator::ScheduleWithContext (dstNode,
                                          delay, &DmgWifiChannel::Receive,
                                          (*i), copy, rxPowerDbm, duration);

          /* PHY Activity Monitor */
          uint32_t srcNode = sender->GetDevice ()->GetNode ()->GetId ();
          RecordPhyActivity (srcNode, dstNode, duration, txPowerDbm + gtx, PLCP_80211AD_PREAMBLE_HDR_DATA, TX_ACTIVITY);
          Simulator::Schedule (delay, &DmgWifiChannel::RecordPhyActivity, this,
                               srcNode, dstNode, duration, rxPowerDbm, PLCP_80211AD_PREAMBLE_HDR_DATA, RX_ACTIVITY);
        }
    }
}

void
DmgWifiChannel::SendAgcSubfield (Ptr<DmgWifiPhy> sender, double txPowerDbm, WifiTxVector txVector) const
{
  NS_LOG_FUNCTION (this << sender << txPowerDbm << txVector);
  Ptr<MobilityModel> senderMobility = sender->GetMobility ()->GetObject<MobilityModel> ();
  NS_ASSERT (senderMobility != 0);
  Ptr<MobilityModel> receiverMobility;
  uint32_t j = 0; /* Phy ID */
  Time delay; /* Propagation delay of the signal */
  for (PhyList::const_iterator i = m_phyList.begin (); i != m_phyList.end (); i++, j++)
    {
      if (sender != (*i))
        {
          // For now don't account for inter channel interference.
          if ((*i)->GetChannelNumber () != sender->GetChannelNumber ())
            {
              continue;
            }

          receiverMobility = (*i)->GetMobility ()->GetObject<MobilityModel> ();
          delay = m_delay->GetDelay (senderMobility, receiverMobility);
          Ptr<Codebook> senderCodebook = sender->GetCodebook ();
          double azimuthTx = CalculateAzimuthAngle (senderMobility->GetPosition (), receiverMobility->GetPosition ());
          double gtx = senderCodebook->GetTxGainDbi (azimuthTx);

          Ptr<Object> dstNetDevice = m_phyList[j]->GetDevice ();
          uint32_t dstNode;	/* Destination node (Receiver) */
          if (dstNetDevice == 0)
            {
              dstNode = 0xffffffff;
            }
          else
            {
              dstNode = dstNetDevice->GetObject<NetDevice> ()->GetNode ()->GetId ();
            }

          /* PHY Activity Monitor */
          RecordPhyActivity (sender->GetDevice ()->GetNode ()->GetId (), dstNode,
                             AGC_SF_DURATION, txPowerDbm + gtx, PLCP_80211AD_AGC_SF, TX_ACTIVITY);
          Simulator::ScheduleWithContext (dstNode, delay, &DmgWifiChannel::ReceiveAgcSubfield, this, j,
                                          sender, txVector, txPowerDbm, gtx);
        }
    }
}

void
DmgWifiChannel::SendTrnCeSubfield (Ptr<DmgWifiPhy> sender, double txPowerDbm, WifiTxVector txVector) const
{
  NS_LOG_FUNCTION (this << sender << txPowerDbm << txVector);
  Ptr<MobilityModel> senderMobility = sender->GetMobility ()->GetObject<MobilityModel> ();
  NS_ASSERT (senderMobility != 0);
  Ptr<MobilityModel> receiverMobility;
  uint32_t j = 0; /* Phy ID */
  Time delay; /* Propagation delay of the signal */
  for (PhyList::const_iterator i = m_phyList.begin (); i != m_phyList.end (); i++, j++)
    {
      if (sender != (*i))
        {
          // For now don't account for inter channel interference.
          if ((*i)->GetChannelNumber () != sender->GetChannelNumber ())
            {
              continue;
            }

          receiverMobility = (*i)->GetMobility ()->GetObject<MobilityModel> ();
          delay = m_delay->GetDelay (senderMobility, receiverMobility);
          Ptr<Codebook> senderCodebook = sender->GetCodebook ();
          double azimuthTx = CalculateAzimuthAngle (senderMobility->GetPosition (), receiverMobility->GetPosition ());
          double gtx = senderCodebook->GetTxGainDbi (azimuthTx);

          Ptr<Object> dstNetDevice = m_phyList[j]->GetDevice ();
          uint32_t dstNode;	/* Destination node (Receiver) */
          if (dstNetDevice == 0)
            {
              dstNode = 0xffffffff;
            }
          else
            {
              dstNode = dstNetDevice->GetObject<NetDevice> ()->GetNode ()->GetId ();
            }

          /* PHY Activity Monitor */
          RecordPhyActivity (sender->GetDevice ()->GetNode ()->GetId (), dstNode,
                             TRN_CE_DURATION, txPowerDbm + gtx, PLCP_80211AD_TRN_CE_SF, TX_ACTIVITY);
          Simulator::ScheduleWithContext (dstNode, delay, &DmgWifiChannel::ReceiveTrnCeSubfield, this, j,
                                          sender, txVector, txPowerDbm, gtx);
        }
    }
}

void
DmgWifiChannel::SendTrnSubfield (Ptr<DmgWifiPhy> sender, double txPowerDbm, WifiTxVector txVector) const
{
  NS_LOG_FUNCTION (this << sender << txPowerDbm << txVector);
  Ptr<MobilityModel> senderMobility = sender->GetMobility ()->GetObject<MobilityModel> ();
  NS_ASSERT (senderMobility != 0);
  Ptr<MobilityModel> receiverMobility;
  uint32_t j = 0; /* Phy ID */
  Time delay; /* Propagation delay of the signal */
  for (PhyList::const_iterator i = m_phyList.begin (); i != m_phyList.end (); i++, j++)
    {
      if (sender != (*i))
        {
          // For now don't account for inter-channel interference.
          if ((*i)->GetChannelNumber () != sender->GetChannelNumber ())
            {
              continue;
            }

          receiverMobility = (*i)->GetMobility ()->GetObject<MobilityModel> ();
          delay = m_delay->GetDelay (senderMobility, receiverMobility);
          Ptr<Codebook> senderCodebook = sender->GetCodebook ();
          double azimuthTx = CalculateAzimuthAngle (senderMobility->GetPosition (), receiverMobility->GetPosition ());
          double gtx = senderCodebook->GetTxGainDbi (azimuthTx);

          Ptr<Object> dstNetDevice = m_phyList[j]->GetDevice ();
          uint32_t dstNode;	/* Destination node (Receiver) */
          if (dstNetDevice == 0)
            {
              dstNode = 0xffffffff;
            }
          else
            {
              dstNode = dstNetDevice->GetObject<NetDevice> ()->GetNode ()->GetId ();
            }

          /* PHY Activity Monitor */
          RecordPhyActivity (sender->GetDevice ()->GetNode ()->GetId (), dstNode,
                             TRN_SUBFIELD_DURATION, txPowerDbm + gtx, PLCP_80211AD_TRN_SF, TX_ACTIVITY);
          Simulator::ScheduleWithContext (dstNode, delay, &DmgWifiChannel::ReceiveTrnSubfield, this, j,
                                          sender, txVector, txPowerDbm, gtx);
        }
    }
}

void
DmgWifiChannel::Receive (Ptr<DmgWifiPhy> phy, Ptr<Packet> packet, double rxPowerDbm, Time duration)
{
  NS_LOG_FUNCTION (phy << packet << rxPowerDbm << duration.GetSeconds ());
  phy->StartReceivePreambleAndHeader (packet, DbmToW (rxPowerDbm + phy->GetRxGain ()), duration);
}

double
DmgWifiChannel::ReceiveSubfield (uint32_t i, Ptr<DmgWifiPhy> sender, WifiTxVector txVector,
                                 double txPowerDbm, double txAntennaGainDbi,
                                 Time duration, PLCP_FIELD_TYPE type) const
{
  NS_LOG_FUNCTION (this << i << sender << txVector << txPowerDbm << txAntennaGainDbi);
  /* Calculate SNR upon the receiption of the TRN Field */
  Ptr<MobilityModel> senderMobility = sender->GetMobility ()->GetObject<MobilityModel> ();
  Ptr<MobilityModel> receiverMobility = m_phyList[i]->GetMobility ()->GetObject<MobilityModel> ();
  NS_ASSERT ((senderMobility != 0) && (receiverMobility != 0));
  double azimuthRx = CalculateAzimuthAngle (receiverMobility->GetPosition (), senderMobility->GetPosition ());
  double rxPowerDbm;

  NS_LOG_DEBUG ("POWER: Gtx=" << txAntennaGainDbi
                << ", Grx=" << m_phyList[i]->GetCodebook ()->GetRxGainDbi (azimuthRx));

  rxPowerDbm = m_loss->CalcRxPower (txPowerDbm, senderMobility, receiverMobility) +
               txAntennaGainDbi +                                           // Sender's antenna gain.
               m_phyList[i]->GetCodebook ()->GetRxGainDbi (azimuthRx);      // Receiver's antenna gain.

  /* PHY Activity Monitor */
  RecordPhyActivity (sender->GetDevice ()->GetNode ()->GetId (),
                     m_phyList[i]->GetDevice ()->GetNode ()->GetId (),
                     duration, rxPowerDbm, type, RX_ACTIVITY);

  /* External Attenuator */
  if ((m_blockage != 0) && (m_srcWifiPhy == sender) && (m_dstWifiPhy == m_phyList[i]))
    {
      rxPowerDbm += m_blockage ();
    }

  NS_LOG_DEBUG ("propagation: txPower=" << txPowerDbm << "dbm, rxPower=" << rxPowerDbm << "dbm");

  return rxPowerDbm;
}

void
DmgWifiChannel::ReceiveAgcSubfield (uint32_t i, Ptr<DmgWifiPhy> sender, WifiTxVector txVector,
                                    double txPowerDbm, double txAntennaGainDbi) const
{
  NS_LOG_FUNCTION (this << i << sender << txVector << txPowerDbm << txAntennaGainDbi);
  double rxPowerDbm = ReceiveSubfield (i, sender, txVector, txPowerDbm, txAntennaGainDbi,
                                       AGC_SF_DURATION, PLCP_80211AD_AGC_SF);
  m_phyList[i]->StartReceiveAgcSubfield (txVector, rxPowerDbm);
}

void
DmgWifiChannel::ReceiveTrnCeSubfield (uint32_t i, Ptr<DmgWifiPhy> sender, WifiTxVector txVector,
                                      double txPowerDbm, double txAntennaGainDbi) const
{
  NS_LOG_FUNCTION (this << i << sender << txVector << txPowerDbm << txAntennaGainDbi);
  double rxPowerDbm = ReceiveSubfield (i, sender, txVector, txPowerDbm, txAntennaGainDbi,
                                       TRN_CE_DURATION, PLCP_80211AD_TRN_CE_SF);
  m_phyList[i]->StartReceiveCeSubfield (txVector, rxPowerDbm);
}

void
DmgWifiChannel::ReceiveTrnSubfield (uint32_t i, Ptr<DmgWifiPhy> sender, WifiTxVector txVector,
                                    double txPowerDbm, double txAntennaGainDbi) const
{
  NS_LOG_FUNCTION (this << i << sender << txVector << txPowerDbm << txAntennaGainDbi);
  double rxPowerDbm = ReceiveSubfield (i, sender, txVector, txPowerDbm, txAntennaGainDbi,
                                       TRN_SUBFIELD_DURATION, PLCP_80211AD_TRN_SF);
  /* Report the received SNR to the higher layers. */
  m_phyList[i]->StartReceiveTrnSubfield (txVector, rxPowerDbm);
}

uint32_t
DmgWifiChannel::GetNDevices (void) const
{
  return m_phyList.size ();
}

Ptr<NetDevice>
DmgWifiChannel::GetDevice (uint32_t i) const
{
  return m_phyList[i]->GetDevice ()->GetObject<NetDevice> ();
}

void
DmgWifiChannel::Add (Ptr<DmgWifiPhy> phy)
{
  m_phyList.push_back (phy);
}

int64_t
DmgWifiChannel::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  int64_t currentStream = stream;
  currentStream += m_loss->AssignStreams (stream);
  return (currentStream - stream);
}

} //namespace ns3
