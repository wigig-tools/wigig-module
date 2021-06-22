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

#include "ns3/string.h"
#include "ns3/names.h"
#include "ns3/pointer.h"
#include "ns3/log.h"
#include "dmg-wifi-helper.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/dmg-wifi-phy.h"
#include "ns3/dmg-wifi-mac.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/spectrum-dmg-wifi-phy.h"
#include "ns3/wifi-net-device.h"
#include "ns3/error-rate-model.h"
#include "ns3/wifi-ack-policy-selector.h"
#include "ns3/net-device-queue-interface.h"
#include "wifi-mac-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DmgWifiHelper");

DmgWifiChannelHelper::DmgWifiChannelHelper ()
{
}

DmgWifiChannelHelper
DmgWifiChannelHelper::Default (void)
{
  DmgWifiChannelHelper helper;
  helper.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  helper.AddPropagationLoss ("ns3::LogDistancePropagationLossModel");
  return helper;
}

void
DmgWifiChannelHelper::AddPropagationLoss (std::string type,
                                           std::string n0, const AttributeValue &v0,
                                           std::string n1, const AttributeValue &v1,
                                           std::string n2, const AttributeValue &v2,
                                           std::string n3, const AttributeValue &v3,
                                           std::string n4, const AttributeValue &v4,
                                           std::string n5, const AttributeValue &v5,
                                           std::string n6, const AttributeValue &v6,
                                           std::string n7, const AttributeValue &v7)
{
  ObjectFactory factory;
  factory.SetTypeId (type);
  factory.Set (n0, v0);
  factory.Set (n1, v1);
  factory.Set (n2, v2);
  factory.Set (n3, v3);
  factory.Set (n4, v4);
  factory.Set (n5, v5);
  factory.Set (n6, v6);
  factory.Set (n7, v7);
  m_propagationLoss.push_back (factory);
}

void
DmgWifiChannelHelper::SetPropagationDelay (std::string type,
                                            std::string n0, const AttributeValue &v0,
                                            std::string n1, const AttributeValue &v1,
                                            std::string n2, const AttributeValue &v2,
                                            std::string n3, const AttributeValue &v3,
                                            std::string n4, const AttributeValue &v4,
                                            std::string n5, const AttributeValue &v5,
                                            std::string n6, const AttributeValue &v6,
                                            std::string n7, const AttributeValue &v7)
{
  ObjectFactory factory;
  factory.SetTypeId (type);
  factory.Set (n0, v0);
  factory.Set (n1, v1);
  factory.Set (n2, v2);
  factory.Set (n3, v3);
  factory.Set (n4, v4);
  factory.Set (n5, v5);
  factory.Set (n6, v6);
  factory.Set (n7, v7);
  m_propagationDelay = factory;
}

Ptr<DmgWifiChannel>
DmgWifiChannelHelper::Create (void) const
{
  Ptr<DmgWifiChannel> channel = CreateObject<DmgWifiChannel> ();
  Ptr<PropagationLossModel> prev = 0;
  for (std::vector<ObjectFactory>::const_iterator i = m_propagationLoss.begin (); i != m_propagationLoss.end (); ++i)
    {
      Ptr<PropagationLossModel> cur = (*i).Create<PropagationLossModel> ();
      if (prev != 0)
        {
          prev->SetNext (cur);
        }
      if (m_propagationLoss.begin () == i)
        {
          channel->SetPropagationLossModel (cur);
        }
      prev = cur;
    }
  Ptr<PropagationDelayModel> delay = m_propagationDelay.Create<PropagationDelayModel> ();
  channel->SetPropagationDelayModel (delay);
  return channel;
}

int64_t
DmgWifiChannelHelper::AssignStreams (Ptr<DmgWifiChannel> c, int64_t stream)
{
  return c->AssignStreams (stream);
}

DmgWifiPhyHelper::DmgWifiPhyHelper ()
  : m_channel (0)
{
  m_phy.SetTypeId ("ns3::DmgWifiPhy");
}

void
DmgWifiPhyHelper::SetDefaultWiGigPhyValues (void)
{
  /* Set the correct error model */
  SetErrorRateModel ("ns3::DmgErrorModel",
                     "FileName", StringValue ("WigigFiles/ErrorModel/LookupTable_1458.txt"));
  Set ("RxNoiseFigure", DoubleValue (10));
  // The value correspond to DMG MCS-0.
  // The start of a valid DMG control PHY transmission at a receive level greater than the minimum sensitivity
  // for control PHY (–78 dBm) shall cause CCA to indicate busy with a probability > 90% within 3 μs.
  Set ("RxSensitivity", DoubleValue (-101)); //CCA-SD for 802.11 signals.
  // The start of a valid DMG SC PHY transmission at a receive level greater than the minimum sensitivity for
  // MCS 1 (–68 dBm) shall cause CCA to indicate busy with a probability > 90% within 1 μs. The receiver shall
  // hold the carrier sense signal busy for any signal 20 dB above the minimum sensitivity for MCS 1.
  Set ("CcaEdThreshold", DoubleValue (-48)); // CCA-ED for non-802.11 signals.
}

DmgWifiPhyHelper
DmgWifiPhyHelper::Default (void)
{
  DmgWifiPhyHelper helper;
  helper.SetDefaultWiGigPhyValues ();
  return helper;
}

void
DmgWifiPhyHelper::SetChannel (Ptr<DmgWifiChannel> channel)
{
  m_channel = channel;
}

void
DmgWifiPhyHelper::SetChannel (std::string channelName)
{
  Ptr<DmgWifiChannel> channel = Names::Find<DmgWifiChannel> (channelName);
  m_channel = channel;
}

Ptr<WifiPhy>
DmgWifiPhyHelper::Create (Ptr<Node> node, Ptr<NetDevice> device) const
{
  Ptr<DmgWifiPhy> phy = m_phy.Create<DmgWifiPhy> ();
  Ptr<ErrorRateModel> error = m_errorRateModel.Create<ErrorRateModel> ();
  phy->SetErrorRateModel (error);
  phy->SetChannel (m_channel);
  phy->SetDevice (device);
  return phy;
}


DmgWifiHelper::DmgWifiHelper ()
{
  SetStandard (WIFI_PHY_STANDARD_80211ad);
  SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                           "ControlMode", StringValue ("DMG_MCS4"),
                           "DataMode", StringValue ("DMG_MCS12"));
}

DmgWifiHelper::~DmgWifiHelper ()
{
}

NetDeviceContainer
DmgWifiHelper::Install (const DmgWifiPhyHelper &phyHelper,
                        const DmgWifiMacHelper &macHelper,
                        NodeContainer::Iterator first,
                        NodeContainer::Iterator last) const
{
  NetDeviceContainer devices;
  for (NodeContainer::Iterator i = first; i != last; ++i)
    {
      Ptr<Node> node = *i;
      Ptr<WifiNetDevice> device = CreateObject<WifiNetDevice> ();
      Ptr<WifiRemoteStationManager> manager = m_stationManager.Create<WifiRemoteStationManager> ();
      Ptr<DmgWifiMac> mac = StaticCast<DmgWifiMac> (macHelper.Create (device));
      Ptr<DmgWifiPhy> phy = StaticCast<DmgWifiPhy> (phyHelper.Create (node, device));
      Ptr<Codebook> codebook = m_codeBook.Create<Codebook> ();
      codebook->SetDevice (device);
      mac->SetAddress (Mac48Address::Allocate ());
      mac->ConfigureStandard (m_standard);
      mac->SetCodebook (codebook);
      phy->SetCodebook (codebook);
      phy->ConfigureStandard (m_standard);
      device->SetMac (mac);
      device->SetPhy (phy);
      device->SetRemoteStationManager (manager);
      node->AddDevice (device);
      devices.Add (device);
      NS_LOG_DEBUG ("node=" << node << ", mob=" << node->GetObject<MobilityModel> ());
      // Aggregate a NetDeviceQueueInterface object if a RegularWifiMac is installed
      Ptr<RegularWifiMac> rmac = DynamicCast<RegularWifiMac> (mac);
      if (rmac)
        {
          Ptr<NetDeviceQueueInterface> ndqi;
          BooleanValue qosSupported;
          PointerValue ptr;
          Ptr<WifiMacQueue> wmq;
          Ptr<WifiAckPolicySelector> ackSelector;

          rmac->GetAttributeFailSafe ("QosSupported", qosSupported);
          if (qosSupported.Get ())
            {
              ndqi = CreateObjectWithAttributes<NetDeviceQueueInterface> ("NTxQueues",
                                                                          UintegerValue (4));

              rmac->GetAttributeFailSafe ("BE_Txop", ptr);
              ackSelector = m_ackPolicySelector[AC_BE].Create<WifiAckPolicySelector> ();
              ackSelector->SetQosTxop (ptr.Get<QosTxop> ());
              ptr.Get<QosTxop> ()->SetAckPolicySelector (ackSelector);
              wmq = ptr.Get<QosTxop> ()->GetWifiMacQueue ();
              ndqi->GetTxQueue (0)->ConnectQueueTraces (wmq);

              rmac->GetAttributeFailSafe ("BK_Txop", ptr);
              ackSelector = m_ackPolicySelector[AC_BK].Create<WifiAckPolicySelector> ();
              ackSelector->SetQosTxop (ptr.Get<QosTxop> ());
              ptr.Get<QosTxop> ()->SetAckPolicySelector (ackSelector);
              wmq = ptr.Get<QosTxop> ()->GetWifiMacQueue ();
              ndqi->GetTxQueue (1)->ConnectQueueTraces (wmq);

              rmac->GetAttributeFailSafe ("VI_Txop", ptr);
              ackSelector = m_ackPolicySelector[AC_VI].Create<WifiAckPolicySelector> ();
              ackSelector->SetQosTxop (ptr.Get<QosTxop> ());
              ptr.Get<QosTxop> ()->SetAckPolicySelector (ackSelector);
              wmq = ptr.Get<QosTxop> ()->GetWifiMacQueue ();
              ndqi->GetTxQueue (2)->ConnectQueueTraces (wmq);

              rmac->GetAttributeFailSafe ("VO_Txop", ptr);
              ackSelector = m_ackPolicySelector[AC_VO].Create<WifiAckPolicySelector> ();
              ackSelector->SetQosTxop (ptr.Get<QosTxop> ());
              ptr.Get<QosTxop> ()->SetAckPolicySelector (ackSelector);
              wmq = ptr.Get<QosTxop> ()->GetWifiMacQueue ();
              ndqi->GetTxQueue (3)->ConnectQueueTraces (wmq);
              ndqi->SetSelectQueueCallback (m_selectQueueCallback);
            }
          else
            {
              ndqi = CreateObject<NetDeviceQueueInterface> ();

              rmac->GetAttributeFailSafe ("Txop", ptr);
              wmq = ptr.Get<Txop> ()->GetWifiMacQueue ();
              ndqi->GetTxQueue (0)->ConnectQueueTraces (wmq);
            }
          device->AggregateObject (ndqi);
        }
    }
  return devices;
}

NetDeviceContainer
DmgWifiHelper::Install (const DmgWifiPhyHelper &phyHelper,
                        const DmgWifiMacHelper &macHelper, NodeContainer c) const
{
  return Install (phyHelper, macHelper, c.Begin (), c.End ());
}

NetDeviceContainer
DmgWifiHelper::Install (const DmgWifiPhyHelper &phy,
                        const DmgWifiMacHelper &mac, Ptr<Node> node) const
{
  return Install (phy, mac, NodeContainer (node));
}

NetDeviceContainer
DmgWifiHelper::Install (const DmgWifiPhyHelper &phy,
                        const DmgWifiMacHelper &mac, std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return Install (phy, mac, NodeContainer (node));
}

void DmgWifiHelper::SetCodebook (std::string name,
				 std::string n0, const AttributeValue &v0,
				 std::string n1, const AttributeValue &v1,
				 std::string n2, const AttributeValue &v2,
				 std::string n3, const AttributeValue &v3,
				 std::string n4, const AttributeValue &v4,
				 std::string n5, const AttributeValue &v5,
				 std::string n6, const AttributeValue &v6,
				 std::string n7, const AttributeValue &v7)
{
  m_codeBook = ObjectFactory ();
  m_codeBook.SetTypeId (name);
  m_codeBook.Set (n0, v0);
  m_codeBook.Set (n1, v1);
  m_codeBook.Set (n2, v2);
  m_codeBook.Set (n3, v3);
  m_codeBook.Set (n4, v4);
  m_codeBook.Set (n5, v5);
  m_codeBook.Set (n6, v6);
  m_codeBook.Set (n7, v7);
}

NetDeviceContainer
DmgWifiHelper::Install (const SpectrumDmgWifiPhyHelper &phyHelper,
                        const DmgWifiMacHelper &macHelper,
                        NodeContainer::Iterator first,
                        NodeContainer::Iterator last, bool installCodebook) const
{
  NetDeviceContainer devices;
  for (NodeContainer::Iterator i = first; i != last; ++i)
    {
      Ptr<Node> node = *i;
      Ptr<WifiNetDevice> device = CreateObject<WifiNetDevice> ();
      Ptr<WifiRemoteStationManager> manager = m_stationManager.Create<WifiRemoteStationManager> ();
      Ptr<DmgWifiMac> mac = StaticCast<DmgWifiMac> (macHelper.Create (device));
      Ptr<SpectrumDmgWifiPhy> phy = StaticCast<SpectrumDmgWifiPhy> (phyHelper.Create (node, device));
      mac->SetAddress (Mac48Address::Allocate ());
      mac->ConfigureStandard (m_standard);
      if (installCodebook)
        {
          Ptr<Codebook> codebook = m_codeBook.Create<Codebook> ();
          mac->SetCodebook (codebook);
          phy->SetCodebook (codebook);
        }
      phy->ConfigureStandard (m_standard);
      device->SetMac (mac);
      device->SetPhy (phy);
      device->SetRemoteStationManager (manager);
      node->AddDevice (device);
      devices.Add (device);

      // Aggregate a NetDeviceQueueInterface object if a RegularWifiMac is installed
      Ptr<RegularWifiMac> rmac = DynamicCast<RegularWifiMac> (mac);
      if (rmac)
        {
          Ptr<NetDeviceQueueInterface> ndqi;
          BooleanValue qosSupported;
          PointerValue ptr;
          Ptr<WifiMacQueue> wmq;
          Ptr<WifiAckPolicySelector> ackSelector;

          rmac->GetAttributeFailSafe ("QosSupported", qosSupported);
          if (qosSupported.Get ())
            {
              ndqi = CreateObjectWithAttributes<NetDeviceQueueInterface> ("NTxQueues",
                                                                          UintegerValue (4));

              rmac->GetAttributeFailSafe ("BE_Txop", ptr);
              ackSelector = m_ackPolicySelector[AC_BE].Create<WifiAckPolicySelector> ();
              ackSelector->SetQosTxop (ptr.Get<QosTxop> ());
              ptr.Get<QosTxop> ()->SetAckPolicySelector (ackSelector);
              wmq = ptr.Get<QosTxop> ()->GetWifiMacQueue ();
              ndqi->GetTxQueue (0)->ConnectQueueTraces (wmq);

              rmac->GetAttributeFailSafe ("BK_Txop", ptr);
              ackSelector = m_ackPolicySelector[AC_BK].Create<WifiAckPolicySelector> ();
              ackSelector->SetQosTxop (ptr.Get<QosTxop> ());
              ptr.Get<QosTxop> ()->SetAckPolicySelector (ackSelector);
              wmq = ptr.Get<QosTxop> ()->GetWifiMacQueue ();
              ndqi->GetTxQueue (1)->ConnectQueueTraces (wmq);

              rmac->GetAttributeFailSafe ("VI_Txop", ptr);
              ackSelector = m_ackPolicySelector[AC_VI].Create<WifiAckPolicySelector> ();
              ackSelector->SetQosTxop (ptr.Get<QosTxop> ());
              ptr.Get<QosTxop> ()->SetAckPolicySelector (ackSelector);
              wmq = ptr.Get<QosTxop> ()->GetWifiMacQueue ();
              ndqi->GetTxQueue (2)->ConnectQueueTraces (wmq);

              rmac->GetAttributeFailSafe ("VO_Txop", ptr);
              ackSelector = m_ackPolicySelector[AC_VO].Create<WifiAckPolicySelector> ();
              ackSelector->SetQosTxop (ptr.Get<QosTxop> ());
              ptr.Get<QosTxop> ()->SetAckPolicySelector (ackSelector);
              wmq = ptr.Get<QosTxop> ()->GetWifiMacQueue ();
              ndqi->GetTxQueue (3)->ConnectQueueTraces (wmq);
              ndqi->SetSelectQueueCallback (m_selectQueueCallback);
            }
          else
            {
              ndqi = CreateObject<NetDeviceQueueInterface> ();

              rmac->GetAttributeFailSafe ("Txop", ptr);
              wmq = ptr.Get<Txop> ()->GetWifiMacQueue ();
              ndqi->GetTxQueue (0)->ConnectQueueTraces (wmq);
            }
          device->AggregateObject (ndqi);
        }
    }
  return devices;
}

NetDeviceContainer
DmgWifiHelper::Install (const SpectrumDmgWifiPhyHelper &phyHelper,
                        const DmgWifiMacHelper &macHelper, NodeContainer c, bool installCodebook) const
{
  return Install (phyHelper, macHelper, c.Begin (), c.End (), installCodebook);
}

NetDeviceContainer
DmgWifiHelper::Install (const SpectrumDmgWifiPhyHelper &phy,
                        const DmgWifiMacHelper &mac, Ptr<Node> node, bool installCodebook) const
{
  return Install (phy, mac, NodeContainer (node), installCodebook);
}

NetDeviceContainer
DmgWifiHelper::Install (const SpectrumDmgWifiPhyHelper &phy,
                        const DmgWifiMacHelper &mac, std::string nodeName, bool installCodebook) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return Install (phy, mac, NodeContainer (node), installCodebook);
}

SpectrumDmgWifiPhyHelper::SpectrumDmgWifiPhyHelper ()
  : m_channel (0)
{
  m_phy.SetTypeId ("ns3::SpectrumDmgWifiPhy");
}

SpectrumDmgWifiPhyHelper
SpectrumDmgWifiPhyHelper::Default (void)
{
  SpectrumDmgWifiPhyHelper helper;
  helper.SetDefaultWiGigPhyValues ();
  return helper;
}

void
SpectrumDmgWifiPhyHelper::SetChannel (Ptr<SpectrumChannel> channel)
{
  m_channel = channel;
}

void
SpectrumDmgWifiPhyHelper::SetChannel (std::string channelName)
{
  Ptr<SpectrumChannel> channel = Names::Find<SpectrumChannel> (channelName);
  m_channel = channel;
}

Ptr<WifiPhy>
SpectrumDmgWifiPhyHelper::Create (Ptr<Node> node, Ptr<NetDevice> device) const
{
  Ptr<SpectrumDmgWifiPhy> phy = m_phy.Create<SpectrumDmgWifiPhy> ();
  phy->CreateWifiSpectrumPhyInterface (device);
  Ptr<ErrorRateModel> error = m_errorRateModel.Create<ErrorRateModel> ();
  phy->SetErrorRateModel (error);
  phy->SetChannel (m_channel);
  phy->SetDevice (device);
  phy->SetMobility (node->GetObject<MobilityModel> ());
  return phy;
}

} //namespace ns3
