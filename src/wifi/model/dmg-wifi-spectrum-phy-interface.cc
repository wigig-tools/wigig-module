/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 CTTC
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
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#include "ns3/log.h"
#include "ns3/spectrum-value.h"
#include "ns3/mobility-model.h"
#include "ns3/net-device.h"
#include "dmg-wifi-spectrum-phy-interface.h"
#include "spectrum-dmg-wifi-phy.h"

NS_LOG_COMPONENT_DEFINE ("DmgWifiSpectrumPhyInterface");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (DmgWifiSpectrumPhyInterface);

TypeId
DmgWifiSpectrumPhyInterface::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DmgWifiSpectrumPhyInterface")
    .SetParent<SpectrumPhy> ()
    .SetGroupName ("Wifi");
  return tid;
}

DmgWifiSpectrumPhyInterface::DmgWifiSpectrumPhyInterface ()
{
  NS_LOG_FUNCTION (this);
}

void
DmgWifiSpectrumPhyInterface::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_spectrumDmgWifiPhy = 0;
  m_netDevice = 0;
  m_channel = 0;
}

void DmgWifiSpectrumPhyInterface::SetSpectrumDmgWifiPhy (const Ptr<SpectrumDmgWifiPhy> spectrumDmgWifiPhy)
{
  m_spectrumDmgWifiPhy = spectrumDmgWifiPhy;
}

Ptr<NetDevice>
DmgWifiSpectrumPhyInterface::GetDevice () const
{
  return m_netDevice;
}

Ptr<MobilityModel>
DmgWifiSpectrumPhyInterface::GetMobility ()
{
  return m_spectrumDmgWifiPhy->GetMobility ();
}

void
DmgWifiSpectrumPhyInterface::SetDevice (const Ptr<NetDevice> d)
{
  m_netDevice = d;
}

void
DmgWifiSpectrumPhyInterface::SetMobility (const Ptr<MobilityModel> m)
{
  m_spectrumDmgWifiPhy->SetMobility (m);
}

void
DmgWifiSpectrumPhyInterface::SetChannel (const Ptr<SpectrumChannel> c)
{
  NS_LOG_FUNCTION (this << c);
  m_channel = c;
}

Ptr<const SpectrumModel>
DmgWifiSpectrumPhyInterface::GetRxSpectrumModel () const
{
  return m_spectrumDmgWifiPhy->GetRxSpectrumModel ();
}

Ptr<AntennaModel>
DmgWifiSpectrumPhyInterface::GetRxAntenna (void)
{
  NS_LOG_FUNCTION (this);
  return NULL;
}

void
DmgWifiSpectrumPhyInterface::StartRx (Ptr<SpectrumSignalParameters> params)
{
  m_spectrumDmgWifiPhy->StartRx (params);
}

} //namespace ns3
