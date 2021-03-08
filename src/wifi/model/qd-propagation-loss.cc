/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
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

#include <ns3/double.h>
#include <ns3/log.h>
#include <ns3/math.h>
#include <ns3/node.h>
#include <ns3/simulator.h>
#include <ns3/node-list.h>

#include "qd-propagation-loss.h"
#include "qd-propagation-engine.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("QdPropagationLossModel");

NS_OBJECT_ENSURE_REGISTERED (QdPropagationLossModel);

TypeId
QdPropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::QdPropagationLossModel")
    .SetParent<SpectrumPropagationLossModel> ()
    .AddConstructor<QdPropagationLossModel> ()
  ;
  return tid;
}

QdPropagationLossModel::QdPropagationLossModel ()
{
  NS_LOG_FUNCTION (this);
}

QdPropagationLossModel::QdPropagationLossModel (Ptr<QdPropagationEngine> qdPropagationEngine)
{
  NS_LOG_FUNCTION (this);
  m_qdPropagationEngine = qdPropagationEngine;
}

QdPropagationLossModel::~QdPropagationLossModel ()
{
  NS_LOG_FUNCTION (this);
}

void
QdPropagationLossModel::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

Ptr<SpectrumValue>
QdPropagationLossModel::DoCalcRxPowerSpectralDensity (Ptr<const SpectrumValue> txPsd,
                                                      Ptr<const MobilityModel> a,
                                                      Ptr<const MobilityModel> b) const
{
  NS_FATAL_ERROR ("This function should not be called here");
  return 0;
}

bool
QdPropagationLossModel::DoCalculateRxPowerAtReceiverSide (void) const
{
  return true;
}

Ptr<SpectrumValue>
QdPropagationLossModel::CalcRxPower (Ptr<SpectrumSignalParameters> params,
				     Ptr<const MobilityModel> a,
				     Ptr<const MobilityModel> b) const
{
  return m_qdPropagationEngine->CalcRxPower (params, a, b);
}

void
QdPropagationLossModel::CalcMimoRxPower (Ptr<SpectrumSignalParameters> rxParams,
                                         Ptr<const MobilityModel> a,
                                         Ptr<const MobilityModel> b) const
{
  return m_qdPropagationEngine->CalcMimoRxPower (rxParams, a, b);
}

} // namespace ns3
