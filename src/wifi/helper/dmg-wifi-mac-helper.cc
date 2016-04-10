/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */

#include "dmg-wifi-mac-helper.h"
#include "ns3/boolean.h"

namespace ns3 {

DmgWifiMacHelper::DmgWifiMacHelper ()
{
}

DmgWifiMacHelper::~DmgWifiMacHelper ()
{
}

DmgWifiMacHelper DmgWifiMacHelper::Default (void)
{
  DmgWifiMacHelper helper;
  helper.SetType ("ns3::DmgStaWifiMac",
                  "QosSupported", BooleanValue (true),
                  "DmgSupported", BooleanValue (true));
  return helper;
}

} // namespace ns3

