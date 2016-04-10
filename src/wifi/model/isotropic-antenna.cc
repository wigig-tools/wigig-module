/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010
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
 * Author: Daniel Halperin <dhalperi@cs.washington.edu>
 */

#include "ns3/log.h"
#include "isotropic-antenna.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("IsotropicAntenna");

NS_OBJECT_ENSURE_REGISTERED (IsotropicAntenna);

TypeId
IsotropicAntenna::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::IsotropicAntenna")
    .SetParent<AbstractAntenna> ()
    .AddConstructor<IsotropicAntenna> ()
    ;
  return tid;
}

IsotropicAntenna::IsotropicAntenna() { }
IsotropicAntenna::~IsotropicAntenna() { }

double
IsotropicAntenna::GetTxGainDbi(double azimuth, double elevation) const
{
    return 0;
}

double
IsotropicAntenna::GetRxGainDbi(double azimuth, double elevation) const
{
    return GetTxGainDbi(azimuth, elevation);
}

}
