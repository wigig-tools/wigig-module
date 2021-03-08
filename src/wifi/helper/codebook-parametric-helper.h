/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2020 IMDEA Networks Institute
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

#ifndef CODEBOOK_PARAMETRIC_HELPER_H
#define CODEBOOK_PARAMETRIC_HELPER_H

#include "ns3/object-factory.h"
#include "ns3/net-device-container.h"

namespace ns3 {

/**
 * \brief helps to create Codebook objects associated with
 *
 * This class can help to create Codebook objects and assign
 * them to a list of WifiNetDevices. All the WifiNeDevices will
 * be sharing the same codebook file and the class will read
 * this file only once and clone it to the rest of the codebook
 * instances. In this way, we save both memory and compuation power.
 */
class CodebookParametricHelper
{
public:
  virtual ~CodebookParametricHelper ();

  CodebookParametricHelper ();
  /**
   * \param n0 the name of the attribute to set
   * \param v0 the value of the attribute to set
   * \param n1 the name of the attribute to set
   * \param v1 the value of the attribute to set
   * \param n2 the name of the attribute to set
   * \param v2 the value of the attribute to set
   * \param n3 the name of the attribute to set
   * \param v3 the value of the attribute to set
   * \param n4 the name of the attribute to set
   * \param v4 the value of the attribute to set
   * \param n5 the name of the attribute to set
   * \param v5 the value of the attribute to set
   * \param n6 the name of the attribute to set
   * \param v6 the value of the attribute to set
   * \param n7 the name of the attribute to set
   * \param v7 the value of the attribute to set
   *
   * Set the codebook parameters and its attributes to use when Install is called.
   */
  void SetCodebookParameters (std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
                              std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
                              std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
                              std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
                              std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue (),
                              std::string n5 = "", const AttributeValue &v5 = EmptyAttributeValue (),
                              std::string n6 = "", const AttributeValue &v6 = EmptyAttributeValue (),
                              std::string n7 = "", const AttributeValue &v7 = EmptyAttributeValue ());
  /**
   * Install parametric codebook on all the specified NetDevices.
   * \param c Netdevice container
   */
  void Install (NetDeviceContainer &c) const;

private:
  ObjectFactory m_codeBook;     //!< Codebook object factory.

};

} //namespace ns3

#endif // CODEBOOK_PARAMETRIC_FACTORY_H
