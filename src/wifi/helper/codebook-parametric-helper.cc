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

#include "ns3/codebook-parametric.h"
#include "ns3/dmg-wifi-mac.h"
#include "ns3/log.h"
#include "ns3/spectrum-dmg-wifi-phy.h"
#include "ns3/wifi-net-device.h"

#include "codebook-parametric-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CodebookParametricHelper");

CodebookParametricHelper::~CodebookParametricHelper ()
{
}

CodebookParametricHelper::CodebookParametricHelper ()
{
}

void
CodebookParametricHelper::SetCodebookParameters (std::string n0, const AttributeValue &v0,
						 std::string n1, const AttributeValue &v1,
						 std::string n2, const AttributeValue &v2,
						 std::string n3, const AttributeValue &v3,
						 std::string n4, const AttributeValue &v4,
						 std::string n5, const AttributeValue &v5,
						 std::string n6, const AttributeValue &v6,
						 std::string n7, const AttributeValue &v7)
{
  m_codeBook = ObjectFactory ();
  m_codeBook.SetTypeId ("ns3::CodebookParametric");
  m_codeBook.Set (n0, v0);
  m_codeBook.Set (n1, v1);
  m_codeBook.Set (n2, v2);
  m_codeBook.Set (n3, v3);
  m_codeBook.Set (n4, v4);
  m_codeBook.Set (n5, v5);
  m_codeBook.Set (n6, v6);
  m_codeBook.Set (n7, v7);
}

void
CodebookParametricHelper::Install (NetDeviceContainer &c) const
{
  Ptr<CodebookParametric> originalCodebook = m_codeBook.Create<CodebookParametric> ();
  bool originalSet = false;
  for (NetDeviceContainer::Iterator it = c.Begin (); it != c.End (); it++)
    {
      Ptr<WifiNetDevice> device = StaticCast<WifiNetDevice> (*it);
      Ptr<DmgWifiMac> mac = StaticCast<DmgWifiMac> (device->GetMac ());
      Ptr<SpectrumDmgWifiPhy> phy = StaticCast<SpectrumDmgWifiPhy> (device->GetPhy ());
      Ptr<CodebookParametric> codebook;
      if (originalSet)
        {
          codebook = Create<CodebookParametric> ();
          codebook->CopyCodebook (originalCodebook);
        }
      else
        {
          codebook = originalCodebook;
          originalSet = true;
        }
      mac->SetCodebook (codebook);
      phy->SetCodebook (codebook);
    }
}

} //namespace ns3
