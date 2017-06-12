/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 *          Hany Assasa <hany.assasa@gmail.com>
 */

#include "yans-wifi-helper.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/yans-wifi-phy.h"
#include "ns3/names.h"
#include "ns3/log.h"
#include "ns3/multi-band-net-device.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("YansWifiHelper");

YansWifiChannelHelper::YansWifiChannelHelper ()
{
}

YansWifiChannelHelper
YansWifiChannelHelper::Default (void)
{
  YansWifiChannelHelper helper;
  helper.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  helper.AddPropagationLoss ("ns3::LogDistancePropagationLossModel");
  return helper;
}

void
YansWifiChannelHelper::AddPropagationLoss (std::string type,
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
YansWifiChannelHelper::SetPropagationDelay (std::string type,
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

void YansWifiPhyHelper::SetAntenna (std::string name,
				    std::string n0, const AttributeValue &v0,
				    std::string n1, const AttributeValue &v1,
				    std::string n2, const AttributeValue &v2,
				    std::string n3, const AttributeValue &v3,
				    std::string n4, const AttributeValue &v4,
				    std::string n5, const AttributeValue &v5,
				    std::string n6, const AttributeValue &v6,
				    std::string n7, const AttributeValue &v7)
{
  m_antenna = ObjectFactory ();
  m_antenna.SetTypeId (name);
  m_antenna.Set (n0, v0);
  m_antenna.Set (n1, v1);
  m_antenna.Set (n2, v2);
  m_antenna.Set (n3, v3);
  m_antenna.Set (n4, v4);
  m_antenna.Set (n5, v5);
  m_antenna.Set (n6, v6);
  m_antenna.Set (n7, v7);
}

Ptr<YansWifiChannel>
YansWifiChannelHelper::Create (void) const
{
  Ptr<YansWifiChannel> channel = CreateObject<YansWifiChannel> ();
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
YansWifiChannelHelper::AssignStreams (Ptr<YansWifiChannel> c, int64_t stream)
{
  return c->AssignStreams (stream);
}

YansWifiPhyHelper::YansWifiPhyHelper ()
  : m_channel (0),
    m_enableAntenna (false)
{
  m_phy.SetTypeId ("ns3::YansWifiPhy");
}

YansWifiPhyHelper
YansWifiPhyHelper::Default (void)
{
  YansWifiPhyHelper helper;
  helper.SetErrorRateModel ("ns3::NistErrorRateModel");
  return helper;
}

void
YansWifiPhyHelper::SetChannel (Ptr<YansWifiChannel> channel)
{
  m_channel = channel;
}

void
YansWifiPhyHelper::SetChannel (std::string channelName)
{
  Ptr<YansWifiChannel> channel = Names::Find<YansWifiChannel> (channelName);
  m_channel = channel;
}

void
YansWifiPhyHelper::EnableAntenna (bool value, bool directional)
{
  m_enableAntenna = value;
  m_directionalAntenna = directional;
}

Ptr<WifiPhy>
YansWifiPhyHelper::Create (Ptr<Node> node, Ptr<NetDevice> device) const
{
  Ptr<YansWifiPhy> phy = m_phy.Create<YansWifiPhy> ();
  Ptr<ErrorRateModel> error = m_errorRateModel.Create<ErrorRateModel> ();
  if (m_enableAntenna)
    {
      if (m_directionalAntenna)
        {
          Ptr<DirectionalAntenna> antenna = m_antenna.Create<DirectionalAntenna> ();
          phy->SetDirectionalAntenna (antenna);
        }
      else
        {
          Ptr<AbstractAntenna> antenna = m_antenna.Create<AbstractAntenna> ();
          phy->SetAntenna (antenna);
        }
    }
  phy->SetErrorRateModel (error);
  phy->SetChannel (m_channel);
  phy->SetDevice (device);
  return phy;
}

void
YansWifiPhyHelper::EnableMultiBandPcap (std::string prefix, Ptr<NetDevice> nd, Ptr<WifiPhy> phy)
{
  //All of the Pcap enable functions vector through here including the ones
  //that are wandering through all of devices on perhaps all of the nodes in
  //the system. We can only deal with devices of type WifiNetDevice.
  Ptr<MultiBandNetDevice> device = nd->GetObject<MultiBandNetDevice> ();
  if (device == 0)
    {
      NS_LOG_INFO ("YansWifiHelper::EnablePcapInternal(): Device " << &device << " not of type ns3::WifiNetDevice");
      return;
    }
  NS_ABORT_MSG_IF (phy == 0, "YansWifiPhyHelper::EnablePcapInternal(): Phy layer in MultiBandNetDevice must be set");

  PcapHelper pcapHelper;

  std::string filename;
  filename = pcapHelper.GetFilenameFromDevice (prefix, device);

  Ptr<PcapFileWrapper> file = pcapHelper.CreateFile (filename, std::ios::out, m_pcapDlt);

  phy->TraceConnectWithoutContext ("MonitorSnifferTx", MakeBoundCallback (&YansWifiPhyHelper::PcapSniffTxEvent, file));
  phy->TraceConnectWithoutContext ("MonitorSnifferRx", MakeBoundCallback (&YansWifiPhyHelper::PcapSniffRxEvent, file));
}

} //namespace ns3
