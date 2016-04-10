/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006,2007 INRIA
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
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
 * Authors: Mathieu Lacage, <mathieu.lacage@sophia.inria.fr>
 *          Hany Assasa <hany.assasa@gmail.com>
 */

#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/mobility-model.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/object-factory.h"
#include "yans-wifi-channel.h"
#include "yans-wifi-phy.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("YansWifiChannel");

NS_OBJECT_ENSURE_REGISTERED (YansWifiChannel);

TypeId
YansWifiChannel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::YansWifiChannel")
    .SetParent<WifiChannel> ()
    .SetGroupName ("Wifi")
    .AddConstructor<YansWifiChannel> ()
    .AddAttribute ("PropagationLossModel", "A pointer to the propagation loss model attached to this channel.",
                   PointerValue (),
                   MakePointerAccessor (&YansWifiChannel::m_loss),
                   MakePointerChecker<PropagationLossModel> ())
    .AddAttribute ("PropagationDelayModel", "A pointer to the propagation delay model attached to this channel.",
                   PointerValue (),
                   MakePointerAccessor (&YansWifiChannel::m_delay),
                   MakePointerChecker<PropagationDelayModel> ())
  ;
  return tid;
}

YansWifiChannel::YansWifiChannel ()
  : m_blockage (0),
    m_packetDropper (0)
{
}

YansWifiChannel::~YansWifiChannel ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_phyList.clear ();
}

void
YansWifiChannel::SetPropagationLossModel (Ptr<PropagationLossModel> loss)
{
  m_loss = loss;
}

void
YansWifiChannel::SetPropagationDelayModel (Ptr<PropagationDelayModel> delay)
{
  m_delay = delay;
}

void
YansWifiChannel::AddBlockage (double (*blockage)(), Ptr<WifiPhy> srcWifiPhy, Ptr<WifiPhy> dstWifiPhy)
{
  m_blockage = blockage;
  m_srcWifiPhy = srcWifiPhy;
  m_dstWifiPhy = dstWifiPhy;
}

void
YansWifiChannel::AddPacketDropper (bool (*dropper)(), Ptr<WifiPhy> srcWifiPhy, Ptr<WifiPhy> dstWifiPhy)
{
  m_packetDropper = dropper;
  m_srcWifiPhy = srcWifiPhy;
  m_dstWifiPhy = dstWifiPhy;
}

void
YansWifiChannel::Send (Ptr<YansWifiPhy> sender, Ptr<const Packet> packet, double txPowerDbm,
                       WifiTxVector txVector, WifiPreamble preamble, enum mpduType mpdutype, Time duration) const
{
  NS_LOG_FUNCTION (this << sender << packet << txPowerDbm << txVector << preamble << mpdutype << duration);
  Ptr<MobilityModel> senderMobility = sender->GetMobility ()->GetObject<MobilityModel> ();
  NS_ASSERT (senderMobility != 0);
  uint32_t j = 0; /* Phy ID */
  Vector sender_pos = senderMobility->GetPosition ();
//  Ptr<AbstractAntenna> senderAnt = sender->GetAntenna();
  Ptr<DirectionalAntenna> senderAnt = sender->GetDirectionalAntenna ();
  double rxPowerDbm;
  Time delay; /* Propagation delay of the signal */
  Ptr<MobilityModel> receiverMobility;
  for (PhyList::const_iterator i = m_phyList.begin (); i != m_phyList.end (); i++, j++)
    {
      if (sender != (*i))
        {
          // For now don't account for inter channel interference.
          if ((*i)->GetChannelNumber () != sender->GetChannelNumber ())
            {
              continue;
            }

          /* Packet Dropper */
          if ((m_packetDropper != 0) &&
             (((m_srcWifiPhy == sender) && (m_dstWifiPhy == (*i))) ||
             ((m_srcWifiPhy == (*i)) && (m_dstWifiPhy == sender))))
            {
              if (m_packetDropper ())
                {
                  continue;
                }
            }

          receiverMobility = (*i)->GetMobility ()->GetObject<MobilityModel> ();
          double azimuthTx = CalculateAzimuthAngle (sender_pos, receiverMobility->GetPosition ());
          double azimuthRx = CalculateAzimuthAngle (receiverMobility->GetPosition (), sender_pos);

          /* Check if the destination node fall within the tx sector */
//          if (senderAnt->IsPeerNodeInTheCurrentSector (azimuth))
//            {
              delay = m_delay->GetDelay (senderMobility, receiverMobility);

              if (senderAnt != 0)
                {
  //                double elevation = CalculateElevationAngle (sender_pos, receiverMobility->GetPosition());
  //                NS_LOG_DEBUG("POWER: txPowerDbm=" << txPowerDbm
  //                          << ", RxPower=" << m_loss->CalcRxPower (txPowerDbm, senderMobility, receiverMobility)
  //                          << ", Gtx=" << sender_ant->GetTxGainDbi (azimuth, elevation)
  //                          << ", Grx=" << (*i)->GetAntenna ()->GetRxGainDbi (azimuth + M_PI, -elevation));
  //                rxPowerDbm = m_loss->CalcRxPower (txPowerDbm, senderMobility, receiverMobility) +
  //                             sender_ant->GetTxGainDbi (azimuth, elevation) +                // Sender's antenna gain.
  //                             (*i)->GetAntenna ()->GetRxGainDbi (azimuth + M_PI, -elevation); // Receiver's antenna gain.

                  NS_LOG_DEBUG ("POWER: azimuthTx=" << azimuthTx
                                << ", azimuthRx=" << azimuthRx
                                << ", txPowerDbm=" << txPowerDbm
                                << ", RxPower=" << m_loss->CalcRxPower (txPowerDbm, senderMobility, receiverMobility)
                                << ", Gtx=" << senderAnt->GetTxGainDbi (azimuthTx)
                                << ", Grx=" << (*i)->GetDirectionalAntenna ()->GetRxGainDbi (azimuthRx));

                  rxPowerDbm = m_loss->CalcRxPower (txPowerDbm, senderMobility, receiverMobility) +
                               senderAnt->GetTxGainDbi (azimuthTx) +                            // Sender's antenna gain.
                               (*i)->GetDirectionalAntenna ()->GetRxGainDbi (azimuthRx);        // Receiver's antenna gain.

                  /* External Attenuator */
                  if ((m_blockage != 0) &&
                      (((m_srcWifiPhy == sender) && (m_dstWifiPhy == (*i))) ||
                       ((m_srcWifiPhy == (*i)) && (m_dstWifiPhy == sender))))
                    {
                      NS_LOG_DEBUG ("Blockage is inserted");
                      rxPowerDbm += m_blockage ();
                    }
                }
              else
                {
                  rxPowerDbm = m_loss->CalcRxPower (txPowerDbm, senderMobility, receiverMobility) ;
                }

              NS_LOG_DEBUG ("propagation: txPower=" << txPowerDbm << "dbm, rxPower=" << rxPowerDbm << "dbm, " <<
                            "distance=" << senderMobility->GetDistanceFrom (receiverMobility) << "m, delay=" << delay);

              Ptr<Packet> copy = packet->Copy ();   /* Copy of the packet to be received. */
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

              /* We are sending PSDU Packet */
              struct Parameters parameters;
              parameters.rxPowerDbm = rxPowerDbm;
              parameters.type = mpdutype;
              parameters.duration = duration;
              parameters.txVector = txVector;
              parameters.preamble = preamble;
              NS_LOG_DEBUG ("Receiving Node ID=" << dstNode);

              Simulator::ScheduleWithContext (dstNode, delay, &YansWifiChannel::Receive, this, j, copy, parameters);
//            }
        }
    }
}

void
YansWifiChannel::SendTrn (Ptr<YansWifiPhy> sender, double txPowerDbm, WifiTxVector txVector, uint8_t fieldsRemaining) const
{
  NS_LOG_FUNCTION (this << sender << txPowerDbm << txVector << uint (fieldsRemaining));
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

          NS_LOG_DEBUG ("propagation: distance=" << senderMobility->GetDistanceFrom (receiverMobility)
                        << "m, delay=" << delay);

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

          Simulator::ScheduleWithContext (dstNode, delay, &YansWifiChannel::ReceiveTrn, this, j,
                                          sender, txVector, txPowerDbm, fieldsRemaining);
        }
    }
}

void
YansWifiChannel::Receive (uint32_t i, Ptr<Packet> packet, struct Parameters parameters) const
{
  NS_LOG_FUNCTION (this << i << packet);
  m_phyList[i]->StartReceivePreambleAndHeader (packet, parameters.rxPowerDbm, parameters.txVector,
                                               parameters.preamble, parameters.type, parameters.duration);
}

void
YansWifiChannel::ReceiveTrn (uint32_t i, Ptr<YansWifiPhy> sender, WifiTxVector txVector,
                             double txPowerDbm, uint8_t fieldsRemaining) const
{
  NS_LOG_FUNCTION (this << i << sender << txVector << txPowerDbm<< uint (fieldsRemaining));
  /* Calculate SNR upon the receiption of the TRN Field */
  Ptr<MobilityModel> senderMobility = sender->GetMobility ()->GetObject<MobilityModel> ();
  Ptr<MobilityModel> receiverMobility = m_phyList[i]->GetMobility ()->GetObject<MobilityModel> ();
  NS_ASSERT ((senderMobility != 0) && (receiverMobility != 0));
  Ptr<DirectionalAntenna> senderAnt = sender->GetDirectionalAntenna ();
  double azimuthTx = CalculateAzimuthAngle (senderMobility->GetPosition (), receiverMobility->GetPosition ());
  double azimuthRx = CalculateAzimuthAngle (receiverMobility->GetPosition (), senderMobility->GetPosition ());
  double rxPowerDbm;

  NS_LOG_DEBUG ("POWER: azimuthTx=" << azimuthTx
                << ", azimuthRx=" << azimuthRx
                << ", RxPower=" << m_loss->CalcRxPower (txPowerDbm, senderMobility, receiverMobility)
                << ", Gtx=" << senderAnt->GetTxGainDbi (azimuthTx)
                << ", Grx=" << m_phyList[i]->GetDirectionalAntenna ()->GetRxGainDbi (azimuthRx));

  rxPowerDbm = m_loss->CalcRxPower (txPowerDbm, senderMobility, receiverMobility) +
               senderAnt->GetTxGainDbi (azimuthTx) +                                      // Sender's antenna gain.
               m_phyList[i]->GetDirectionalAntenna ()->GetRxGainDbi (azimuthRx);          // Receiver's antenna gain.

  /* External Attenuator */
  if ((m_blockage != 0) && (m_srcWifiPhy == sender) && (m_dstWifiPhy == m_phyList[i]))
    {
      rxPowerDbm += m_blockage ();
    }

  NS_LOG_DEBUG ("propagation: txPower=" << txPowerDbm << "dbm, rxPower=" << rxPowerDbm << "dbm");

  /* Report the received SNR to the higher layers */
  m_phyList[i]->StartReceiveTrnField (txVector, rxPowerDbm, fieldsRemaining);
}

uint32_t
YansWifiChannel::GetNDevices (void) const
{
  return m_phyList.size ();
}

Ptr<NetDevice>
YansWifiChannel::GetDevice (uint32_t i) const
{
  return m_phyList[i]->GetDevice ()->GetObject<NetDevice> ();
}

void
YansWifiChannel::Add (Ptr<YansWifiPhy> phy)
{
  m_phyList.push_back (phy);
}

int64_t
YansWifiChannel::AssignStreams (int64_t stream)
{
  int64_t currentStream = stream;
  currentStream += m_loss->AssignStreams (stream);
  return (currentStream - stream);
}

} //namespace ns3
