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

#include <fstream>
#include <string>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("QdPropagationDelay");

NS_OBJECT_ENSURE_REGISTERED (QdPropagationDelay);

TypeId
QdPropagationDelay::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::QdPropagationDelay")
    .SetParent<PropagationDelayModel> ()
    .AddConstructor<QdPropagationDelay> ()
    .AddAttribute ("QDModelFolder",
                   "Path to the folder containing the ray tracing files of the Quasi-deterministic channel.",
                   StringValue (""),
                   MakeStringAccessor (&QdPropagationDelay::SetQdModelFolder),
                   MakeStringChecker ())
    .AddAttribute ("StartDistance",
                   "Select start point of the simulation, the range of data point is [0, 260] meters",
                   UintegerValue (0),
                   MakeUintegerAccessor (&QdPropagationDelay::SetStartDistance),
                   MakeUintegerChecker<uint16_t> (0, 260))
    .AddAttribute ("Speed",
                   "The speed of the node (m/s)",
                   DoubleValue (0.0),
                   MakeDoubleAccessor (&QdPropagationDelay::m_speed),
                   MakeDoubleChecker<double> (0))
  ;
  return tid;
}

QdPropagationDelay::QdPropagationDelay ()
{
  NS_LOG_FUNCTION (this);
}

QdPropagationDelay::~QdPropagationDelay ()
{
  NS_LOG_FUNCTION (this);
}

void
QdPropagationDelay::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

void
QdPropagationDelay::SetQdModelFolder (std::string folderName)
{
  NS_LOG_INFO ("Q-D Channel Model Folder: " << folderName);
  m_qdFolder = folderName;
}

void
QdPropagationDelay::SetStartDistance (uint16_t startDistance)
{
  NS_LOG_FUNCTION (this << startDistance);
  m_startDistance = startDistance;
  m_currentIndex = m_startDistance * 100;
}

doubleVector_t
QdPropagationDelay::ReadQDPropagationDelays (uint16_t indexTx, uint16_t indexRx) const
{
  NS_LOG_FUNCTION (this << indexTx << indexRx);
  std::string rayTracingPrefixFile  = m_qdFolder + "QdFiles/Tx";
  uint16_t parameterNumber = 0;
  std::string qdParameterFile;

  std::ostringstream ssRx;
  ssRx << indexRx;
  std::string indexRxStr = ssRx.str ();

  std::ostringstream ssTx;
  ssTx << indexTx;
  std::string indexTxStr = ssTx.str ();

  qdParameterFile = std::string (rayTracingPrefixFile) + std::string (indexTxStr) + std::string ("Rx")
      + std::string (indexRxStr) + std::string (".txt");
  NS_LOG_INFO ("Open Q-D Channel Model File: " << qdParameterFile);

  std::ifstream rayTraycingFile;
  rayTraycingFile.open (qdParameterFile.c_str (), std::ifstream::in);
  if (!rayTraycingFile.good ())
    {
      NS_FATAL_ERROR ("Error Opening Q-D Channel Model File: " << qdParameterFile);
    }

  std::string line;
  std::string token;
  doubleVector_t delayTxRx;
  uint traceIndex = 0;

  while (std::getline (rayTraycingFile, line))
    {
      if (parameterNumber == 8)
        {
          parameterNumber = 0;
          traceIndex++;
        }

      if (parameterNumber == 1)
        {
          std::istringstream stream (line);
          double tokenValue = 0.00;
          while (getline (stream, token, ','))
            {
              std::stringstream stream (token) ;
              stream >> tokenValue;
              break;
            }
          delayTxRx.push_back (tokenValue);
        }

      parameterNumber++;
    }
  rayTraycingFile.close ();
  return delayTxRx;
}

Time
QdPropagationDelay::GetDelay (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
  NS_LOG_FUNCTION (this);
  uint32_t indexTx, indexRx;

  Ptr<NetDevice> txDevice = a->GetObject<Node> ()->GetDevice (0);
  Ptr<NetDevice> rxDevice = b->GetObject<Node> ()->GetDevice (0);
  indexTx = txDevice->GetNode ()->GetId ();
  indexRx = rxDevice->GetNode ()->GetId ();

  if (m_speed > 0)
    {
      double time = Simulator::Now ().GetSeconds ();
      uint16_t traceIndex = (m_startDistance + time * m_speed) * 100;
      NS_ASSERT_MSG (traceIndex <= 26050, "The maximum trace index is 26050.");
      if (traceIndex != m_currentIndex)
        {
          m_currentIndex = traceIndex;
        }
    }

  CommunicatingPair pair = std::make_pair (indexTx, indexRx);
  PropagationDelays_I it = m_delays.find (pair);
  if (it == m_delays.end ())
    {
      CommunicatingPair reversePair = std::make_pair (indexRx, indexTx);
      doubleVector_t delayValues = ReadQDPropagationDelays (indexTx, indexRx);
      m_delays[pair] = delayValues;
      m_delays[reversePair] = delayValues;
      return Seconds (delayValues[m_currentIndex]);
    }
  else
    {
      return Seconds (it->second[m_currentIndex]);
    }
}

int64_t
QdPropagationDelay::DoAssignStreams (int64_t stream)
{
  return 0;
}

} // namespace ns3
