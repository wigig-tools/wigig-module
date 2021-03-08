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
#include <ns3/string.h>
#include <ns3/uinteger.h>

#include "qd-propagation-delay.h"
#include "qd-propagation-engine.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("QdPropagationDelayModel");

NS_OBJECT_ENSURE_REGISTERED (QdPropagationDelayModel);

TypeId
QdPropagationDelayModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::QdPropagationDelayModel")
    .SetParent<PropagationDelayModel> ()
    .AddConstructor<QdPropagationDelayModel> ()
  ;
  return tid;
}

QdPropagationDelayModel::QdPropagationDelayModel ()
{
  NS_LOG_FUNCTION (this);
}

QdPropagationDelayModel::QdPropagationDelayModel (Ptr<QdPropagationEngine> qdPropagationEngine)
{
  NS_LOG_FUNCTION (this);
  m_qdPropagationEngine = qdPropagationEngine;
}

QdPropagationDelayModel::~QdPropagationDelayModel ()
{
  NS_LOG_FUNCTION (this);
}

void
QdPropagationDelayModel::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

int64_t
QdPropagationDelayModel::DoAssignStreams (int64_t stream)
{
  return 0;
}

Time
QdPropagationDelayModel::GetDelay (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
  return m_qdPropagationEngine->GetDelay (a, b);
}

} // namespace ns3
