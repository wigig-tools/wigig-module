/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2020 IMDEA Networks Institute
 * Copyright (c) 2018-2020 National Institute of Standards and Technology (NIST)
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
 * Autohrs: Hany Assasa <hany.assasa@gmail.com>
 *          Tanguy Ropitault <tanguy.ropitault@gmail.com>
 *
 * Certain portions of this software were contributed as a public
 * service by the National Institute of Standards and Technology (NIST)
 * and are not subject to US Copyright.  Such contributions are provided
 * “AS-IS” and may be used on an unrestricted basis.  To the extent
 * foreign copyright exists,  such contributions are subject to the
 * GNU General Public License version 2, as consistent with Federal
 * law. Individual source files clarify to which portion they belong.
 */

#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/math.h"
#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/simulator.h"
#include "ns3/node-list.h"
#include "ns3/string.h"
#include "qd-propagation-engine.h"
#include "spectrum-dmg-wifi-phy.h"
#include "wifi-mac.h"
#include "wifi-net-device.h"

#include <algorithm>
#include <fstream>
#include <string>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("QdPropagationEngine");

NS_OBJECT_ENSURE_REGISTERED (QdPropagationEngine);

TypeId
QdPropagationEngine::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::QdPropagationEngine")
    .SetParent<Object> ()
    .AddConstructor<QdPropagationEngine> ()
    .AddAttribute ("QDModelFolder",
                   "Path to the folder containing the ray tracing files of the Quasi-deterministic channel.",
                   StringValue (""),
                   MakeStringAccessor (&QdPropagationEngine::SetQdModelFolder),
                   MakeStringChecker ())
    .AddAttribute ("StartIndex",
                   "Select the starting index in a Q-D file.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&QdPropagationEngine::SetStartIndex),
                   MakeUintegerChecker<uint32_t> (0))
    .AddAttribute ("Interval",
                   "The time interval between two consecutive Q-D traces."
                   "This is the time interval at which we update the Q-D channel gains in ns-3.",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&QdPropagationEngine::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("UseCustomIDs",
                   "Flag to indicate whether we use a custom list to map ns-3 Nodes IDs to the Q-D Files IDs.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&QdPropagationEngine::m_useCustomIDs),
                   MakeBooleanChecker ())
  ;
  return tid;
}

QdPropagationEngine::QdPropagationEngine ()
{
  NS_LOG_FUNCTION (this);
  m_uniformRv = CreateObject<UniformRandomVariable> ();
}

QdPropagationEngine::~QdPropagationEngine ()
{
  NS_LOG_FUNCTION (this);
}

void
QdPropagationEngine::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_uniformRv = 0;
}

void
QdPropagationEngine::SetQdModelFolder (const std::string folderName)
{
  NS_LOG_INFO ("Q-D Channel Model Folder: " << folderName);
  m_qdFolder = folderName;
}

void
QdPropagationEngine::SetStartIndex (const uint32_t startIndex)
{
  NS_LOG_FUNCTION (this << startIndex);
  m_startIndex = startIndex;
  m_currentIndex = startIndex;
}

uint16_t
QdPropagationEngine::GetCurrentTraceIndex (void) const
{
  return m_currentIndex;
}

void
QdPropagationEngine::InitializeQDModelParameters (Ptr<const MobilityModel> txMobility, Ptr<const MobilityModel> rxMobility,
                                                  uint16_t indexTx, uint16_t indexRx) const
{
  NS_LOG_FUNCTION (this << indexTx << indexRx);
  std::string rayTracingPrefixFile  = m_qdFolder + "QdFiles/Tx";
  float2DVector_t rotmAod[8];     /* Rotation Matrix used to manage Angles of Departure depending on antenna orientation. */
  float2DVector_t rotmAoa[8];     /* Rotation Matrix used to manage Angles of Arrival depending on antenna orientation. */
  uint16_t traceIndex = 0;        /* Used for mobility. */

  Ptr<NetDevice> txDevice = txMobility->GetObject<Node> ()->GetDevice (0);
  Ptr<NetDevice> rxDevice = rxMobility->GetObject<Node> ()->GetDevice (0);
  Ptr<WifiNetDevice> wifiTxDevice = DynamicCast<WifiNetDevice> (txDevice);
  Ptr<WifiNetDevice> wifiRxDevice = DynamicCast<WifiNetDevice> (rxDevice);
  Ptr<SpectrumDmgWifiPhy> txSpectrum = StaticCast<SpectrumDmgWifiPhy> (wifiTxDevice->GetPhy ());
  Ptr<SpectrumDmgWifiPhy> rxSpectrum = StaticCast<SpectrumDmgWifiPhy> (wifiRxDevice->GetPhy ());
  Ptr<CodebookParametric> txCodebook = DynamicCast<CodebookParametric> (txSpectrum->GetCodebook ());
  Ptr<CodebookParametric> rxCodebook = DynamicCast<CodebookParametric> (rxSpectrum->GetCodebook ());

  uint8_t numTxAntennas = txCodebook->GetTotalNumberOfAntennas ();
  uint8_t numRxAntennas = rxCodebook->GetTotalNumberOfAntennas ();

  for (AntennaID i = 1 ; i <= numTxAntennas; i++)
    {
      EulerTransform (txSpectrum->GetCodebook ()->GetOrientation (i), rotmAod[i-1]);
    }
  for (AntennaID i = 1 ; i <= numRxAntennas; i++)
    {
      EulerTransform (rxSpectrum->GetCodebook ()->GetOrientation (i), rotmAoa[i-1]);
    }

  std::string qdParameterFile;
  std::ostringstream ssRx;
  ssRx << indexRx;
  std::string indexRxStr = ssRx.str ();

  std::ostringstream ssTx;
  ssTx << indexTx;
  std::string indexTxStr = ssTx.str ();

  /* Open the QD-model files (generated by Matlab) between transmitter and receiver */
  qdParameterFile = std::string (rayTracingPrefixFile) + std::string (indexTxStr) + std::string ("Rx")
      + std::string (indexRxStr) + std::string (".txt");
  NS_LOG_INFO ("Open Q-D Channel Model File: " << qdParameterFile);

  std::ifstream qdFile;
  qdFile.open (qdParameterFile.c_str (), std::ifstream::in);
  if (!qdFile.good ())
    {
      NS_FATAL_ERROR ("Error Opening Q-D Channel Model File: " << qdParameterFile);
    }

  std::string line;
  std::string token;
  uint16_t numPath = 0;
  QdChanneldentifier chId;     /* Q-D Channel Profile Identifier */

  /* Parse each line of the Q-D file */
  while (true)
    {
      for (AntennaID i = 1 ; i <= numTxAntennas; i++)
        {
          for (AntennaID j = 1 ; j <= numRxAntennas; j++)
            {
              chId = std::make_tuple (indexTx, indexRx, traceIndex, i, j);
              for (uint16_t parameterNumber = 0; parameterNumber < 8; parameterNumber++)
                {
                  std::getline (qdFile, line);
                  if (qdFile.eof ())
                    {
                      goto closeFile;
                    }
                  /* First parameter is the number of multipaths */
                  if (parameterNumber == 0)
                    {
                      numPath = std::stoul (line);
                      nbMultipathTxRx[chId] = numPath;
                    }
                  if ((numPath > 0) && (parameterNumber > 0))
                    {
                      floatVector_t values;
                      std::istringstream stream (line);
                      while (std::getline (stream, token, ',')) /* Parse each comma separated string in a line */
                        {
                          float tokenValue = 0.00;
                          std::stringstream stream (token) ;
                          stream >> tokenValue;
                          values.push_back (tokenValue);
                        }
                      switch (parameterNumber)
                        {
                          case 1:
                            /* Second parameter is the delay */
                            delayTxRx[chId] = values;
                            break;

                          case 2:
                            /* Third parameter is the path Loss */
                            pathLossTxRx[chId] = values;
                            break;

                          case 3:
                            /* Fourth parameter is the phase */
                            phaseTxRx[chId] = values;
                            break;

                          case 4:
                            /* Fifth parameter is the AoD Elevation */
                            aodElevationTxRx[chId] = values;
                            break;

                          case 5:
                            /* Sixth parameter is the AoD Azimuth */
                            aodAzimuthTxRx[chId] = values;

                            /* AoD Antenna orientation transformation */
                            float elevationMultipath, azimuthMultipath;
                            AnglesTransformed angles;
                            for (uint16_t k = 0; k < numPath; k++)
                              {
                                elevationMultipath = DegreesToRadians (aodElevationTxRx[chId].at (k));
                                azimuthMultipath = DegreesToRadians (aodAzimuthTxRx[chId].at (k));
                                angles = GetTransformedAngles (elevationMultipath, azimuthMultipath, false, rotmAod[i-1]);
                                aodElevationTxRx[chId].at (k) = angles.elevation;
                                aodAzimuthTxRx[chId].at (k) = angles.azimuth;
                                if (!txCodebook->ArrayPatternsPrecalculated ())
                                  {
                                    txCodebook->CalculateArrayPatterns (i, angles.azimuth, angles.elevation);
                                  }
                              }

                            break;

                          case 6:
                            /* Seventh parameter is the AoA Elevation */
                            aoaElevationTxRx[chId] = values;
                            break;

                          case 7:
                            /* Eighth parameter is the AoA Azimuth */
                            aoaAzimuthTxRx[chId] = values;

                            /* AoA Antenna orientation transformation */
                            for (uint16_t k = 0; k < numPath; k++)
                              {
                                elevationMultipath = DegreesToRadians (aoaElevationTxRx[chId].at (k));
                                azimuthMultipath = DegreesToRadians (aoaAzimuthTxRx[chId].at (k));
                                angles = GetTransformedAngles (elevationMultipath, azimuthMultipath, false, rotmAoa[j-1]);
                                aoaElevationTxRx[chId].at (k) = angles.elevation;
                                aoaAzimuthTxRx[chId].at (k) = angles.azimuth;
                                if (!rxCodebook->ArrayPatternsPrecalculated ())
                                  {
                                    rxCodebook->CalculateArrayPatterns (j, angles.azimuth, angles.elevation);
                                  }
                              }

                            break;
                          }
                    }
                  else if ((numPath == 0) && (parameterNumber == 0))
                    {
                      /* Handle a special case when there is no channel between devices/antennas */
                      nbMultipathTxRx[chId] = 0;
                      break;
                    }
                }
            }
        }
      traceIndex++;
    }

closeFile:
  m_numTraces = traceIndex;
  qdFile.close ();
}

void
QdPropagationEngine::AddCustomID (const uint32_t nodeID, const uint32_t qdID)
{
  nodeId2QdId[nodeID] = qdID;
}

uint32_t
QdPropagationEngine::GetQdID (const uint32_t nodeID) const
{
  std::map<uint32_t, uint32_t>::const_iterator it = nodeId2QdId.find (nodeID);
  if (it != nodeId2QdId.end ())
    {
      return (*it).second;
    }
  else
    {
      NS_FATAL_ERROR ("Cannot map Node ID=" << nodeID << " to any Q-D ID");
    }
}

void
QdPropagationEngine::ReadNodesConfigurationFile (std::string nodesConfugrationFile,
                                                 uint16_t &numAPs, NodeContainer &apWifiNodes,
                                                 NodeContainer &staWifiNodes, std::vector<NodeContainer> &staNodesGroups)
{
  uint16_t numSTAs;
  std::ifstream file;
  file.open (nodesConfugrationFile.c_str (), std::ifstream::in);
  NS_ASSERT_MSG (file.good (), " Configuration file not found");
  std::string line, tokens, value;
  uint32_t id;
  uint32_t firstValue, secondValue;
  bool rangeString;

  /* Set to use custom IDs */
  m_useCustomIDs = true;

  /* The first line determines the number of DMG APs within our scenario. */
  std::getline (file, line);
  numAPs = std::stoul (line);

  for (uint32_t ap = 0; ap < numAPs; ap++)
    {
      /* Create DMG PCP/AP */
      Ptr<Node> apNode = CreateObject<Node> ();
      apWifiNodes.Add (apNode);

      /* Read the Q-D ID that we want to use for the AP. */
      std::getline (file, line);
      id = std::stoul (line);
      AddCustomID (apNode->GetId (), id);

      /* Read the number of STAs associated with this STA. */
      std::getline (file, line);
      numSTAs = std::stoul (line);

      /* Create DMG STAs for this DMG AP. */
      NodeContainer nodes;
      nodes.Create (numSTAs);
      staNodesGroups.push_back (nodes);
      staWifiNodes.Add (nodes);

      /* Read the list of the IDs to be assigned to the STAs associated with this DMG AP. */
      std::getline (file, line);

      /* Split string based on ',' delimiter */
      std::istringstream firstLayer (line);
      uint32_t nodeIndex = 0;
      while (getline (firstLayer, tokens, ','))
        {
          std::istringstream tokensStream (tokens);
          /* Split string based on ':' delimiter */
          rangeString = false;
          while (getline (tokensStream, value, ':'))
            {
              if (!rangeString)
                {
                  firstValue = std::stoul (value);
                  AddCustomID (nodes.Get (nodeIndex)->GetId (), firstValue);
                  rangeString = true;
                  nodeIndex++;
                }
              else
                {
                  secondValue = std::stoul (value);
                  for (uint32_t i = firstValue + 1; i <= secondValue; i++, nodeIndex++)
                    {
                      AddCustomID (nodes.Get (nodeIndex)->GetId (), i);
                    }
                }
            }
        }
    }
}

Time
QdPropagationEngine::GetDelay (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
  NS_LOG_FUNCTION (this << a << b);
  uint32_t indexTx, indexRx;

  Ptr<NetDevice> txDevice = a->GetObject<Node> ()->GetDevice (0);
  Ptr<WifiNetDevice> wifiTxDevice = DynamicCast<WifiNetDevice> (txDevice);
  Ptr<SpectrumDmgWifiPhy> txSpectrum = StaticCast<SpectrumDmgWifiPhy> (wifiTxDevice->GetPhy ());
  Ptr<CodebookParametric> txCodebook = DynamicCast<CodebookParametric> (txSpectrum->GetCodebook ());

  Ptr<NetDevice> rxDevice = b->GetObject<Node> ()->GetDevice (0);
  Ptr<WifiNetDevice> wifiRxDevice = DynamicCast<WifiNetDevice> (rxDevice);
  Ptr<SpectrumDmgWifiPhy> rxSpectrum = StaticCast<SpectrumDmgWifiPhy> (wifiRxDevice->GetPhy ());
  Ptr<CodebookParametric> rxCodebook = DynamicCast<CodebookParametric> (rxSpectrum->GetCodebook ());

  if (m_useCustomIDs)
    {
      indexTx = GetQdID (txDevice->GetNode ()->GetId ());
      indexRx = GetQdID (rxDevice->GetNode ()->GetId ());
    }
  else
    {
      indexTx = txDevice->GetNode ()->GetId ();
      indexRx = rxDevice->GetNode ()->GetId ();
    }

  /* Mobility Management */
  HandleMobility ();

  CommunicatingPair pair = std::make_pair (indexTx, indexRx);
  TraceFiles_I trIt = find (m_traceFiles.begin (), m_traceFiles.end (), pair);
  if (trIt == m_traceFiles.end ())
    {
      /* Load Q-D files in order to fill all the needed parameters to compute channel gain */
      InitializeQDModelParameters (a, b, indexTx, indexRx);
      m_traceFiles.push_back (pair);
    }

  /* Create Q-D channel identifier */
  QdChanneldentifier chId = std::make_tuple (indexTx, indexRx, m_currentIndex,
                                             txCodebook->GetActiveAntennaID (), rxCodebook->GetActiveAntennaID ());

  /* The first multipath component has the smallest propagation delay */
  ChannelCoefficientMap_I it = delayTxRx.find (chId);
  if (it != delayTxRx.end ())
    {
      return Seconds (delayTxRx[chId].at (0));
    }
  else
    {
      return Seconds (0);
    }
}

Ptr<SpectrumValue>
QdPropagationEngine::GetChannelGain (Ptr<SpectrumValue> rxPsd,
                                     uint16_t pathNum, QdChanneldentifier chId,
                                     Ptr<CodebookParametric> txCodebook, Ptr<CodebookParametric> rxCodebook,
                                     Ptr<PatternConfig> txPattern, Ptr<PatternConfig> rxPattern) const
{
  NS_LOG_FUNCTION (this << pathNum);
  double t = Simulator::Now ().GetSeconds ();
  Ptr<SpectrumValue> tempPsd = Copy<SpectrumValue> (rxPsd);
  Bands::const_iterator fit = tempPsd->ConstBandsBegin ();

  float temp_delay, f_d, temp_Doppler, pathPowerLinear, phase;
  Complex delay, doppler,complexPhase, smallScaleFading, txSum, rxSum;
  uint16_t indexTxAzimuth, indexTxElevation, indexRxAzimuth, indexRxElevation;

  /* Iterate through tempPsd (vectors containing the power corresponding to a subband) to compute the gain */
  for (Values::iterator vit = tempPsd->ValuesBegin (); vit != tempPsd->ValuesEnd (); vit++, fit++)
    {
      if ((*vit) != 0.00)
        {
          Complex subsbandGain (0.0, 0.0);
          if (pathNum > 0)
            {
              for (uint pathIndex = 0; pathIndex < pathNum; pathIndex++)
                {
                  temp_delay = -2 * M_PI * fit->fc * delayTxRx[chId].at (pathIndex);
                  delay = Complex (cos (temp_delay), sin (temp_delay));

                  if (m_interval.IsStrictlyPositive ())
                    {
                      /* TODO We are not yet using Doppler */
                      f_d = 0.8;
                      temp_Doppler = 2*M_PI*t*f_d*dopplerShiftTxRx[chId].at (pathIndex);
                      doppler = Complex (cos (temp_Doppler), sin (temp_Doppler));
                    }
                  else
                    {
                      doppler = Complex (1, 0);
                    }

                  pathPowerLinear = std::pow (10.0, (pathLossTxRx[chId].at (pathIndex))/10.0);
                  phase = phaseTxRx[chId].at (pathIndex);
                  complexPhase = Complex (cos (phase), sin (phase));
                  smallScaleFading = float (sqrt (pathPowerLinear)) * doppler * delay * complexPhase;

                  /* Compute the gain for each subband */
                  indexTxAzimuth = aodAzimuthTxRx[chId].at (pathIndex);
                  indexTxElevation = aodElevationTxRx[chId].at (pathIndex);
                  txSum = txCodebook->GetAntennaArrayPattern (txPattern, indexTxAzimuth, indexTxElevation);

                  indexRxAzimuth = aoaAzimuthTxRx[chId].at (pathIndex);
                  indexRxElevation = aoaElevationTxRx[chId].at (pathIndex);
                  rxSum = rxCodebook->GetAntennaArrayPattern (rxPattern, indexRxAzimuth, indexRxElevation);
                  // Normalize at the receiver to take into acccount noise amplification (Check our WiKi page
                  // for more explanation regarding link budget calculations).
                  rxSum /= DynamicCast<ParametricPatternConfig> (rxPattern)->GetNormalizationFactor ();

                  /* Add multipath effect to the subband gain */
                  subsbandGain = subsbandGain + rxSum * txSum * smallScaleFading;
                }
            }
          else
            {
              subsbandGain = -std::numeric_limits<Complex>::infinity ();
            }
          /* All Multipath Done - Compute the power for the subband */
          *vit = (*vit) * (std::norm (subsbandGain));
        }
    }
  return tempPsd;
}

void
QdPropagationEngine::HandleMobility (void) const
{
  NS_LOG_FUNCTION (this);
  if (m_interval.IsStrictlyPositive ())
    {
      uint32_t traceIndex = m_startIndex + (Simulator::Now ()/m_interval).GetHigh ();
      /* We keep using the channel corresponding to the last entry in the Q-D file */
      if ((traceIndex < m_numTraces) && (traceIndex != m_currentIndex))
        {
          m_currentIndex = traceIndex;
          m_channelGainMatrix.clear ();
        }
    }
}

Ptr<SpectrumValue>
QdPropagationEngine::CalcRxPower (Ptr<SpectrumSignalParameters> params,
				  Ptr<const MobilityModel> a,
				  Ptr<const MobilityModel> b) const
{
  NS_LOG_FUNCTION (this);

  uint32_t indexTx, indexRx;

  Ptr<DmgWifiSpectrumSignalParameters> rxParams = DynamicCast<DmgWifiSpectrumSignalParameters> (params);

  Ptr<NetDevice> txDevice = a->GetObject<Node> ()->GetDevice (0);
  Ptr<NetDevice> rxDevice = b->GetObject<Node> ()->GetDevice (0);

  Ptr<WifiNetDevice> wifiTxDevice = DynamicCast<WifiNetDevice> (txDevice);
  Ptr<SpectrumDmgWifiPhy> txSpectrum = StaticCast<SpectrumDmgWifiPhy> (wifiTxDevice->GetPhy ());
  Ptr<CodebookParametric> txCodebook = DynamicCast<CodebookParametric> (txSpectrum->GetCodebook ());

  Ptr<WifiNetDevice> wifiRxDevice = DynamicCast<WifiNetDevice> (rxDevice);
  Ptr<SpectrumDmgWifiPhy> rxSpectrum = StaticCast<SpectrumDmgWifiPhy> (wifiRxDevice->GetPhy ());
  Ptr<CodebookParametric> rxCodebook = DynamicCast<CodebookParametric> (rxSpectrum->GetCodebook ());

  AntennaConfigTx antennaConfigTx = std::make_pair (rxParams->antennaId,
                                                    rxParams->txPatternConfig);
  AntennaConfigRx antennaConfigRx = std::make_pair (rxCodebook->GetActiveAntennaID (),
                                                    rxCodebook->GetRxPatternConfig ());

  LinkConfiguration key = std::make_tuple (txDevice, rxDevice, antennaConfigTx, antennaConfigRx);

  if (m_useCustomIDs)
    {
      indexTx = GetQdID (txDevice->GetNode ()->GetId ());
      indexRx = GetQdID (rxDevice->GetNode ()->GetId ());
    }
  else
    {
      indexTx = txDevice->GetNode ()->GetId ();
      indexRx = rxDevice->GetNode ()->GetId ();
    }

  /* Mobility Management */
  HandleMobility ();

  ChannelGainMatrix_I it = m_channelGainMatrix.find (key);
  Ptr<SpectrumValue> chPsd;

  /* Check if the channel has already been computed between transmitter and receiver for certain antenna configurations */
  if (it == m_channelGainMatrix.end ())
    {
      QdChanneldentifier chId = std::make_tuple (indexTx, indexRx, m_currentIndex,
                                                 rxParams->antennaId, rxCodebook->GetActiveAntennaID ());

      uint16_t pathNum = nbMultipathTxRx[chId];

      /* Doppler effect */
      if (m_interval.IsStrictlyPositive ())
        {
          floatVector_t dopplerShiftVec;
          for (uint16_t i = 0; i < pathNum; i++)
            {
              dopplerShiftVec.push_back (m_uniformRv->GetValue (0, 1));
            }
          dopplerShiftTxRx[chId] = dopplerShiftVec;
        }

      /*
       * Insert the channel into the Channel matrix to avoid
       * recomputing the channel every time if there is no Mobility.
       */
      chPsd = GetChannelGain (rxParams->psd, pathNum, chId,
                              txCodebook, rxCodebook,
                              rxParams->txPatternConfig, rxCodebook->GetRxPatternConfig ());
      m_channelGainMatrix[key] = chPsd;
    }
  else
    {
      /* The channel has already been computed */
      chPsd = (*it).second;
    }

  return chPsd;
}

void
QdPropagationEngine::CalcMimoRxPower (Ptr<SpectrumSignalParameters> params,
                                      Ptr<const MobilityModel> a,
                                      Ptr<const MobilityModel> b) const
{
  uint32_t indexTx, indexRx;

  Ptr<DmgWifiSpectrumSignalParameters> rxParams = DynamicCast<DmgWifiSpectrumSignalParameters> (params);

  Ptr<NetDevice> txDevice = a->GetObject<Node> ()->GetDevice (0);
  Ptr<NetDevice> rxDevice = b->GetObject<Node> ()->GetDevice (0);

  Ptr<WifiNetDevice> wifiTxDevice = DynamicCast<WifiNetDevice> (txDevice);
  Ptr<SpectrumDmgWifiPhy> txSpectrum = StaticCast<SpectrumDmgWifiPhy> (wifiTxDevice->GetPhy ());
  Ptr<CodebookParametric> txCodebook = DynamicCast<CodebookParametric> (txSpectrum->GetCodebook ());

  Ptr<WifiNetDevice> wifiRxDevice = DynamicCast<WifiNetDevice> (rxDevice);
  Ptr<SpectrumDmgWifiPhy> rxSpectrum = StaticCast<SpectrumDmgWifiPhy> (wifiRxDevice->GetPhy ());
  Ptr<CodebookParametric> rxCodebook = DynamicCast<CodebookParametric> (rxSpectrum->GetCodebook ());

  if (m_useCustomIDs)
    {
      indexTx = GetQdID (txDevice->GetNode ()->GetId ());
      indexRx = GetQdID (rxDevice->GetNode ()->GetId ());
    }
  else
    {
      indexTx = txDevice->GetNode ()->GetId ();
      indexRx = rxDevice->GetNode ()->GetId ();
    }

  /* Mobility Management */
  HandleMobility ();

//  for (auto const &txConfig : txCodebook->GetActiveTxPatternIDs ())
//    {
//      for (auto const &rxConfig : rxCodebook->GetActiveRxPatternIDs ())
//        {
//          NS_LOG_DEBUG ("Tx config:" << + txConfig.first.first << " " << +  txConfig.first.second << " " << +  txConfig.second);
//          NS_LOG_DEBUG ("Rx config:" << + rxConfig.first.first << " " << +  rxConfig.first.second << " " << +  rxConfig.second);
//          NS_LOG_DEBUG (" ");
//        }
//    }

  // Iterate over all the possible antenna combinations for MIMO computations.
  for (auto const &txAntenna : txCodebook->GetActiveTxPatternList ())
    {
      for (auto const &rxAntenna : rxCodebook->GetActiveRxPatternList ())
        {
          AntennaConfigTx antennaConfigTx = std::make_pair (txAntenna.first, txAntenna.second);
          AntennaConfigRx antennaConfigRx = std::make_pair (rxAntenna.first, rxAntenna.second);
          LinkConfiguration key = std::make_tuple (txDevice, rxDevice, antennaConfigTx, antennaConfigRx);

          ChannelGainMatrix_I it = m_channelGainMatrix.find (key);
          Ptr<SpectrumValue> chPsd;

          /* Check if the channel has already been computed between transmitter and receiver for certain antenna configurations */
          if (it == m_channelGainMatrix.end ())
            {
              QdChanneldentifier chId = std::make_tuple (indexTx, indexRx, m_currentIndex, txAntenna.first, rxAntenna.first);
              uint16_t pathNum = nbMultipathTxRx[chId];

              /* Doppler effect */
              if (m_interval.IsStrictlyPositive ())
                {
                  floatVector_t dopplerShiftVec;
                  for (uint16_t i = 0; i < pathNum; i++)
                    {
                      dopplerShiftVec.push_back (m_uniformRv->GetValue (0, 1));
                    }
                  dopplerShiftTxRx[chId] = dopplerShiftVec;
                }

              /*
               * Insert the channel into the Channel matrix to avoid
               * recomputing the channel every time if there is no Mobility.
               */
              chPsd = GetChannelGain (rxParams->psd, pathNum, chId,
                                      txCodebook, rxCodebook,
                                      txAntenna.second, rxAntenna.second);
              m_channelGainMatrix[key] = chPsd;
            }
          else
            {
              /* The channel has already been computed */
              chPsd = (*it).second;
            }
          rxParams->psdList.push_back (chPsd);
        }
    }
}

AnglesTransformed
QdPropagationEngine::GetTransformedAngles (double elevation, double azimuth, bool isDoa, float2DVector_t& rotmVector) const
{
  float doa[1][3];
  float doa_temp[1][3];
  int m1 = 1;
  int m2 = 3;
  int n2 = 3;

  doa_temp[0][0] = sin (elevation) * cos (azimuth);
  doa_temp[0][1] = sin (elevation) * sin (azimuth);
  doa_temp[0][2] = cos (elevation);

  for (int i = 0; i < m1; i++)
    {
      for (int j = 0; j < n2; j++)
        {
          doa[i][j] = 0;
          for (int x = 0; x < m2; x++)
            {
              *(*(doa + i) + j) += *(*(doa_temp + i) + x) * rotmVector[x][j];
            }
        }
    }

  // Truncate because of the float problem
  if (doa[0][0] <= 0.00001 && doa[0][0] >= 0)
    {
      doa[0][0] = 0;
    }
  if (doa[0][0] >= - 0.00001 && doa[0][0] <= 0)
    {
      doa[0][0] = 0;
    }
  if (doa[0][1] <= 0.00001 && doa[0][1] >= 0)
    {
      doa[0][1] = 0;
    }
  if (doa[0][1] >= - 0.00001 && doa[0][1] <= 0)
    {
      doa[0][1] = 0;
    }
  if ((doa[0][0] == doa[0][1]) && doa[0][0] == 0)
    {
      azimuth = 0;
    }
  else if (doa[0][1] < 0 && doa[0][0] >=0)
    {
      azimuth = 2*M_PI + atan (doa[0][1]/doa[0][0]);
    }
  else if (doa[0][1] <= 0 && doa[0][0] < 0)
    {
      azimuth = M_PI + atan (doa[0][1]/doa[0][0]);
    }
  else if (doa[0][1]>0 && doa[0][0]<0)
    {
      azimuth = M_PI + atan (doa[0][1]/doa[0][0]);
    }
  else
    {
      azimuth = atan (doa[0][1]/doa[0][0]);
    }

  azimuth *= 180/M_PI;
  elevation = acos (doa[0][2]) * 180/M_PI;

  AnglesTransformed angles;
  angles.elevation = round (elevation);
  angles.azimuth = round (azimuth);
  return angles;
}

void
QdPropagationEngine::EulerTransform (Orientation orientation, float2DVector_t& rotMatrix) const
{
  float rotm[3][3];     // Rotation matrix.

  rotm[0][0] = cos (orientation.psi) * cos (orientation.theta);
  rotm[0][1] = cos (orientation.psi) * sin (orientation.theta) * sin (orientation.phi) - sin (orientation.psi) * cos (orientation.phi);
  rotm[0][2] = cos (orientation.psi) * sin (orientation.theta) * cos (orientation.phi) + sin (orientation.psi) * sin (orientation.phi);

  rotm[1][0] = sin (orientation.psi) * cos (orientation.theta);
  rotm[1][1] = sin (orientation.psi) * sin (orientation.theta) * sin (orientation.phi) + cos (orientation.psi) * cos (orientation.phi);
  rotm[1][2] = sin (orientation.psi) * sin (orientation.theta) * cos (orientation.phi) - cos (orientation.psi) * sin (orientation.phi);

  rotm[2][0] = -sin (orientation.theta);
  rotm[2][1] = cos (orientation.theta) * sin (orientation.phi);
  rotm[2][2] = cos (orientation.theta) * cos (orientation.phi);

  for (uint8_t i = 0; i < 3 ; i++)
    {
      floatVector_t rotmRow;
      for (uint8_t j = 0; j < 3; j++)
        {
          rotmRow.push_back (rotm[i][j]);
        }
      rotMatrix.push_back (rotmRow);
    }
}

} // namespace ns3
