/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, NYU WIRELESS, Tandon School of Engineering, New York University
 * Copyright (c) 2018-2019 National Institute of Standards and Technology (NIST)
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
 * Authors: Marco Mezzavilla <mezzavilla@nyu.edu>
 *          Sourjya Dutta <sdutta@nyu.edu>
 *          Russell Ford <russell.ford@nyu.edu>
 *          Menglei Zhang <menglei@nyu.edu>
 *
 * The original file is adapted from NYU Channel Ray-Tracer.
 * Modified By: Tanguy Ropitault <tanguy.ropitault@gmail.com>
 *              Hany Assasa <hany.assasa@gmail.com>
 *
 * Certain portions of this software were contributed as a public
 * service by the National Institute of Standards and Technology (NIST)
 * and are not subject to US Copyright.  Such contributions are provided
 * “AS-IS” and may be used on an unrestricted basis.  To the extent
 * foreign copyright exists,  such contributions are subject to the
 * GNU General Public License version 2, as consistent with Federal
 * law. Individual source files clarify to which portion they belong.
 */

#include <ns3/double.h>
#include <ns3/log.h>
#include <ns3/math.h>
#include <ns3/node.h>
#include <ns3/simulator.h>
#include <ns3/node-list.h>

#include "qd-propagation-loss.h"
#include "spectrum-dmg-wifi-phy.h"
#include "wifi-mac.h"
#include "wifi-net-device.h"

#include <algorithm>
#include <fstream>
#include <string>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("QdPropagationLossModel");

NS_OBJECT_ENSURE_REGISTERED (QdPropagationLossModel);

TypeId
QdPropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::QdPropagationLossModel")
    .SetParent<SpectrumPropagationLossModel> ()
    .AddConstructor<QdPropagationLossModel> ()
    .AddAttribute ("QDModelFolder",
                   "Path to the folder containing the ray tracing files of the Quasi-deterministic channel.",
                   StringValue (""),
                   MakeStringAccessor (&QdPropagationLossModel::SetQdModelFolder),
                   MakeStringChecker ())
    .AddAttribute ("StartDistance",
                   "Select start point of the simulation, the range of data point is [0, 260] meters.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&QdPropagationLossModel::SetStartDistance),
                   MakeUintegerChecker<uint16_t> (0))
    .AddAttribute ("Speed",
                   "The speed of the node (m/s).",
                   DoubleValue (0.0),
                   MakeDoubleAccessor (&QdPropagationLossModel::m_speed),
                   MakeDoubleChecker<double> (0))
    .AddAttribute ("UseCustomIDs",
                   "Flag to indicate whether we use custom list to map ns-3 nodes IDs to Q-D Software IDs.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&QdPropagationLossModel::m_useCustomIDs),
                   MakeBooleanChecker ())
  ;
  return tid;
}

QdPropagationLossModel::QdPropagationLossModel ()
{
  NS_LOG_FUNCTION (this);
  m_uniformRv = CreateObject<UniformRandomVariable> ();
}

QdPropagationLossModel::~QdPropagationLossModel ()
{
  NS_LOG_FUNCTION (this);
}

void
QdPropagationLossModel::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_uniformRv = 0;
}

void
QdPropagationLossModel::SetQdModelFolder (const std::string folderName)
{
  NS_LOG_INFO ("Q-D Channel Model Folder: " << folderName);
  m_qdFolder = folderName;
}

void
QdPropagationLossModel::SetStartDistance (const uint16_t startDistance)
{
  NS_LOG_FUNCTION (this << startDistance);
  m_startDistance = startDistance;
  m_currentIndex = m_startDistance * 100;
}

uint16_t
QdPropagationLossModel::GetCurrentTraceIndex (void) const
{
  return m_currentIndex;
}

void
QdPropagationLossModel::InitializeQDModelParameters (Ptr<const MobilityModel> txMobility, Ptr<const MobilityModel> rxMobility,
                                                     uint16_t indexTx, uint16_t indexRx) const
{
  NS_LOG_FUNCTION (this << indexTx << indexRx);
  std::string rayTracingPrefixFile  = m_qdFolder + "QdFiles/Tx";
  float2DVector_t rotmAod[8];
  float2DVector_t rotmAoa[8];
  double antennaOrientationVector[3];
  uint16_t parameterNumber = 0;
  uint traceIndex = 0;

  Ptr<NetDevice> txDevice = txMobility->GetObject<Node> ()->GetDevice (0);
  Ptr<NetDevice> rxDevice = rxMobility->GetObject<Node> ()->GetDevice (0);
  Ptr<WifiNetDevice> wifiTxDevice = DynamicCast<WifiNetDevice> (txDevice);
  Ptr<WifiNetDevice> wifiRxDevice = DynamicCast<WifiNetDevice> (rxDevice);
  Ptr<SpectrumDmgWifiPhy> txSpectrum = StaticCast<SpectrumDmgWifiPhy> (wifiTxDevice->GetPhy ());
  Ptr<SpectrumDmgWifiPhy> rxSpectrum = StaticCast<SpectrumDmgWifiPhy> (wifiRxDevice->GetPhy ());

  uint8_t numAntennas = txSpectrum->GetCodebook ()->GetTotalNumberOfAntennas ();

  double referenceVector[3] = {0,0,1};
  for (AntennaID i = 1; i <= numAntennas; i++)
    {
      Orientation orientation = txSpectrum->GetCodebook ()->GetOrientation (i);
      antennaOrientationVector[0] = orientation.x;
      antennaOrientationVector[1] = orientation.y;
      antennaOrientationVector[2] = orientation.z;
      QuaternionTransform (referenceVector, antennaOrientationVector, rotmAod[i - 1]);
    }

  for (AntennaID i = 1; i <= numAntennas; i++)
    {
      Orientation orientation = rxSpectrum->GetCodebook ()->GetOrientation (i);
      double antennaOrientationVector[3];
      antennaOrientationVector[0] = orientation.x;
      antennaOrientationVector[1] = orientation.y;
      antennaOrientationVector[2] = orientation.z;
      QuaternionTransform (referenceVector, antennaOrientationVector, rotmAoa[i - 1]);
    }

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
  uint16_t numPath = 0;
  while (std::getline (rayTraycingFile, line))
    {
      if (parameterNumber == 8)
        {
          parameterNumber = 0;
          traceIndex++;
        }

      doubleVector_t path;
      if (parameterNumber == 0)
        {
          numPath = std::stoul (line);
          path.push_back (numPath);
        }

      if (numPath > 0)
        {
          std::istringstream stream (line);
          while (getline (stream, token, ','))
            {
              double tokenValue = 0.00;
              std::stringstream stream (token);
              stream >> tokenValue;
              path.push_back (tokenValue);
            }

          switch (parameterNumber)
            {
            case 0:
              nbMultipathTxRx[indexTx][indexRx].push_back (numPath);
              break;

            case 1:
              delayTxRx[indexTx][indexRx].push_back (path);
              break;

            case 2:
              pathLossTxRx[indexTx][indexRx].push_back (path);
              break;

            case 3:
              phaseTxRx[indexTx][indexRx].push_back (path);
              break;

            case 4:
              for (AntennaID i = 1; i <= numAntennas; i++)
                {
                  aodElevationTxRx[i][indexTx][indexRx].push_back (path);
                }
              break;

            case 5:
              for (AntennaID i = 1; i <= numAntennas; i++)
                {
                  aodAzimuthTxRx[i][indexTx][indexRx].push_back (path);
                }
              break;

            case 6:
              for (AntennaID i = 1; i <= numAntennas; i++)
                {
                  aoaElevationTxRx[i][indexTx][indexRx].push_back (path);
                }
              break;

            case 7:
              for (AntennaID i = 1; i <= numAntennas; i++)
                {
                  aoaAzimuthTxRx[i][indexTx][indexRx].push_back (path);
                }
              break;
            }
          parameterNumber++;
        }
      else
        {
          doubleVector_t emptyVector;
          nbMultipathTxRx[indexTx][indexRx].push_back (numPath);
          delayTxRx[indexTx][indexRx].push_back (emptyVector);
          pathLossTxRx[indexTx][indexRx].push_back (emptyVector);
          phaseTxRx[indexTx][indexRx].push_back (emptyVector);
          for (AntennaID i = 1; i <= numAntennas; i++)
            {
              aodElevationTxRx[i][indexTx][indexRx].push_back (emptyVector);
            }
          for (AntennaID i = 1; i <= numAntennas; i++)
            {
              aodAzimuthTxRx[i][indexTx][indexRx].push_back (emptyVector);
            }
          for (AntennaID i = 1; i <= numAntennas; i++)
            {
              aoaElevationTxRx[i][indexTx][indexRx].push_back (emptyVector);
            }
          for (AntennaID i = 1; i <= numAntennas; i++)
            {
              aoaAzimuthTxRx[i][indexTx][indexRx].push_back (emptyVector);
            }
          traceIndex++;
          continue;
        }

      double elevationMultipath, azimuthMultipath;
      AnglesTransformed angles;
      if (parameterNumber == 6)
        {
          for (AntennaID i = 1; i <= numAntennas; i++)
            {
              for (uint16_t j = 0; j < nbMultipathTxRx[indexTx][indexRx].at (traceIndex); j++)
                {
                  elevationMultipath = DegreesToRadians (aodElevationTxRx[i][indexTx][indexRx].at (traceIndex).at (j));
                  azimuthMultipath = DegreesToRadians (aodAzimuthTxRx[i][indexTx][indexRx].at (traceIndex).at (j));
                  angles = GetTransformedAngles (elevationMultipath, azimuthMultipath, false, rotmAod[i - 1]);
                  aodElevationTxRx[i][indexTx][indexRx].at (traceIndex).at (j) = angles.elevation;
                  aodAzimuthTxRx[i][indexTx][indexRx].at (traceIndex).at (j) = angles.azimuth;
                }
            }
        }

      if (parameterNumber == 8)
        {
          for (AntennaID i = 1; i <= numAntennas; i++)
            {
              for (uint16_t j = 0; j < nbMultipathTxRx[indexTx][indexRx].at (traceIndex); j++)
                {
                  elevationMultipath = DegreesToRadians (aoaElevationTxRx[i][indexTx][indexRx].at (traceIndex).at (j));
                  azimuthMultipath = DegreesToRadians (aoaAzimuthTxRx[i][indexTx][indexRx].at (traceIndex).at (j));
                  angles = GetTransformedAngles (elevationMultipath, azimuthMultipath, true, rotmAoa[i - 1]);
                  aoaElevationTxRx[i][indexTx][indexRx].at (traceIndex).at (j) = angles.elevation;
                  aoaAzimuthTxRx[i][indexTx][indexRx].at (traceIndex).at (j) = angles.azimuth;
                }
            }
        }
    }
  m_numTraces = traceIndex;
  rayTraycingFile.close ();
}

Ptr<SpectrumValue>
QdPropagationLossModel::GetChannelGain (Ptr<const SpectrumValue> txPsd, uint16_t pathNum,
                                        uint32_t indexTx, uint32_t indexRx,
                                        Ptr<CodebookParametric> txCodebook, Ptr<CodebookParametric> rxCodebook) const
{
  NS_LOG_FUNCTION (this << txPsd << pathNum << indexTx << indexRx << m_currentIndex);
  double t = Simulator::Now ().GetSeconds ();
  bool noSpeed = false;
  AntennaID txAntennaID = txCodebook->GetActiveAntennaID ();
  AntennaID rxAntennaID = rxCodebook->GetActiveAntennaID ();

  if (m_speed == 0)
    {
      noSpeed = true;
    }
  noSpeed = true;

  Ptr<SpectrumValue> tempPsd = Copy<SpectrumValue> (txPsd);
  Bands::const_iterator fit = tempPsd->ConstBandsBegin ();

  std::complex<double> delay, doppler;
  double temp_delay, f_d, temp_Doppler, pathPowerLinear, phase;
  std::complex<double> complexPhase, smallScaleFading, txSum, rxSum;
  double azimuthTxAngle, elevationTxAngle, azimuthRxAngle, elevationRxAngle;
  uint indexTxAzimuth, indexRxAzimuth, indexTxElevation, indexRxElevation;

  for (Values::iterator vit = tempPsd->ValuesBegin (); vit != tempPsd->ValuesEnd (); vit++, fit++)
    {
      if ((*vit) != 0.00)
        {
          std::complex<double> subsbandGain (0.0, 0.0);
          if (pathNum > 0)
            {
              for (uint pathIndex = 0; pathIndex < pathNum; pathIndex++)
                {
                  temp_delay = -2 * M_PI * fit->fc * delayTxRx[indexTx][indexRx].at (m_currentIndex).at (pathIndex);
                  delay = std::complex<double> (cos (temp_delay), sin (temp_delay));

                  if (noSpeed)
                    {
                      doppler = std::complex<double> (1, 0);
                    }
                  else
                    {
                      f_d = 0.8;
                      temp_Doppler = 2 * M_PI * t * f_d * dopplerShiftTxRx[indexTx][indexRx].at (m_currentIndex).at (pathIndex);
                      doppler = std::complex<double> (cos (temp_Doppler), sin (temp_Doppler));
                    }

                  pathPowerLinear = std::pow (10.0, (pathLossTxRx[indexTx][indexRx].at (m_currentIndex).at (pathIndex)) / 10.0);
                  phase = phaseTxRx[indexTx][indexRx].at (m_currentIndex).at (pathIndex);
                  complexPhase = std::complex<double> (cos (phase), sin (phase));
                  smallScaleFading = sqrt (pathPowerLinear) * doppler * delay * complexPhase;

                  azimuthTxAngle = round (aodAzimuthTxRx[txAntennaID][indexTx][indexRx].at (m_currentIndex).at (pathIndex));
                  elevationTxAngle = round (aodElevationTxRx[txAntennaID][indexTx][indexRx].at (m_currentIndex).at (pathIndex));
                  indexTxAzimuth = azimuthTxAngle;
                  indexTxElevation = elevationTxAngle;

                  txSum = txCodebook->GetTxAntennaArrayPattern ()[indexTxAzimuth][indexTxElevation];
                  azimuthRxAngle = round (aoaAzimuthTxRx[rxAntennaID][indexTx][indexRx].at (m_currentIndex).at (pathIndex));
                  elevationRxAngle = round (aoaElevationTxRx[rxAntennaID][indexTx][indexRx].at (m_currentIndex).at (pathIndex));
                  indexRxAzimuth = azimuthRxAngle;
                  indexRxElevation = elevationRxAngle;

                  rxSum = rxCodebook->GetRxAntennaArrayPattern ()[indexRxAzimuth][indexRxElevation];

                  subsbandGain = subsbandGain + rxSum * txSum * smallScaleFading;
                }
            }
          else
            {
              subsbandGain = -std::numeric_limits<Complex>::infinity ();
            }
          *vit = (*vit) * (std::norm (subsbandGain));
        }
    }
  return tempPsd;
}

void
QdPropagationLossModel::AddCustomID (const uint32_t nodeID, const uint32_t customID)
{
  nodeId2QdId[nodeID] = customID;
}

uint32_t
QdPropagationLossModel::MapID (const uint32_t nodeID) const
{
  std::map<uint32_t, uint32_t>::const_iterator it = nodeId2QdId.find (nodeID);
  if (it != nodeId2QdId.end ())
    {
      return (*it).second;
    }
  else
    {
      return nodeID;
    }
}

Ptr<SpectrumValue>
QdPropagationLossModel::DoCalcRxPowerSpectralDensity (Ptr<const SpectrumValue> txPsd,
                                                      Ptr<const MobilityModel> a,
                                                      Ptr<const MobilityModel> b) const
{
  NS_LOG_FUNCTION (this);

  uint32_t indexTx, indexRx;
  Ptr<SpectrumValue> rxPsd = Copy (txPsd);

  Ptr<NetDevice> txDevice = a->GetObject<Node> ()->GetDevice (0);
  Ptr<WifiNetDevice> wifiTxDevice = DynamicCast<WifiNetDevice> (txDevice);
  Ptr<SpectrumDmgWifiPhy> txSpectrum = StaticCast<SpectrumDmgWifiPhy> (wifiTxDevice->GetPhy ());
  Ptr<CodebookParametric> txCodebook = DynamicCast<CodebookParametric> (txSpectrum->GetCodebook ());

  Ptr<NetDevice> rxDevice = b->GetObject<Node> ()->GetDevice (0);
  Ptr<WifiNetDevice> wifiRxDevice = DynamicCast<WifiNetDevice> (rxDevice);
  Ptr<SpectrumDmgWifiPhy> rxSpectrum = StaticCast<SpectrumDmgWifiPhy> (wifiRxDevice->GetPhy ());
  Ptr<CodebookParametric> rxCodebook = DynamicCast<CodebookParametric> (rxSpectrum->GetCodebook ());

  Ptr<SpectrumValue> chPsd;
  AntennaConfigTx antennaConfigTx = std::make_tuple (txCodebook->GetActiveAntennaID (),
                                                     txCodebook->IsCustomAWVUsed (),
                                                     txCodebook->GetActiveTxPatternID ());
  AntennaConfigTx antennaConfigRx = std::make_tuple (rxCodebook->GetActiveAntennaID (),
                                                     rxCodebook->IsCustomAWVUsed (),
                                                     rxCodebook->GetActiveRxPatternID ());

  LinkConfiguration key = std::make_tuple (txDevice, rxDevice, antennaConfigTx, antennaConfigRx);

  if (m_useCustomIDs)
    {
      indexTx = MapID (txDevice->GetNode ()->GetId ());
      indexRx = MapID (rxDevice->GetNode ()->GetId ());
    }
  else
    {
      indexTx = txDevice->GetNode ()->GetId ();
      indexRx = rxDevice->GetNode ()->GetId ();
    }

  if (m_speed > 0)
    {
      double time = Simulator::Now ().GetSeconds ();
      uint16_t traceIndex = (m_startDistance + time * m_speed) * 100;
      if (traceIndex < m_numTraces)
        {
          if (traceIndex != m_currentIndex)
            {
              m_currentIndex = traceIndex;
              m_channelMatrixMap.clear ();
            }
        }
    }

  ChannelMatrix_I it = m_channelMatrixMap.find (key);

  if (it == m_channelMatrixMap.end ())
    {
      CommunicatingPair pair = std::make_pair (indexTx, indexRx);
      TraceFiles_I trIt = find (m_traceFiles.begin (), m_traceFiles.end (), pair);
      if (trIt == m_traceFiles.end ())
        {
          InitializeQDModelParameters (a, b, indexTx, indexRx);
          m_traceFiles.push_back (pair);
        }
      uint16_t pathNum = nbMultipathTxRx[indexTx][indexRx].at (m_currentIndex);
      if (m_speed > 0)
        {
          doubleVector_t dopplerShift;
          for (uint16_t i = 0; i < pathNum; i++)
            {
              dopplerShift.push_back (m_uniformRv->GetValue (0, 1));
            }
          dopplerShiftTxRx[indexTx][indexRx].push_back (dopplerShift);
          dopplerShiftTxRx[indexRx][indexTx].push_back (dopplerShift);
        }
      chPsd = GetChannelGain (rxPsd, pathNum, indexTx, indexRx, txCodebook, rxCodebook);
      m_channelMatrixMap[key] = chPsd;
    }
  else
    {
      chPsd = (*it).second;
    }

  return chPsd;
}

AnglesTransformed
QdPropagationLossModel::GetTransformedAngles (double elevation, double azimuth, bool isDoa, float2DVector_t& rotmVector) const
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
  if (doa[0][0] >= -0.00001 && doa[0][0] <= 0)
    {
      doa[0][0] = 0;
    }
  if (doa[0][1] <= 0.00001 && doa[0][1] >= 0)
    {
      doa[0][1] = 0;
    }
  if (doa[0][1] >= -0.00001 && doa[0][1] <= 0)
    {
      doa[0][1] = 0;
    }
  if ((doa[0][0] == doa[0][1]) && doa[0][0] == 0)
    {
      azimuth = 0;
    }
  else if (doa[0][1] < 0 && doa[0][0] >= 0)
    {
      azimuth = 2 * M_PI + atan (doa[0][1] / doa[0][0]);
    }
  else if (doa[0][1] <= 0 && doa[0][0] < 0)
    {
      azimuth = M_PI + atan (doa[0][1] / doa[0][0]);
    }
  else if (doa[0][1] > 0 && doa[0][0] < 0)
    {
      azimuth = M_PI + atan (doa[0][1] / doa[0][0]);
    }
  else
    {
      azimuth = atan (doa[0][1] / doa[0][0]);
    }

  azimuth *= 180 / M_PI;
  elevation = acos (doa[0][2]) * 180 / M_PI;

  AnglesTransformed angles;
  angles.elevation = elevation;
  angles.azimuth = azimuth;
  return angles;
}

void
QdPropagationLossModel::QuaternionTransform (double givenAxis[3], double desiredAxis[3], float2DVector_t& rotmVector) const
{
  double theta;
  float rotm[3][3];
  float b[3];

  double num = givenAxis[0] * desiredAxis[0] + givenAxis[1] * desiredAxis[1] + givenAxis[2] * desiredAxis[2];
  double denum = sqrt ((pow (givenAxis[0], 2) + pow (givenAxis[1], 2) + pow (givenAxis[2], 2)) * (pow (desiredAxis[0], 2)
                                                                                                  + pow (desiredAxis[1], 2) + pow (desiredAxis[2], 2)));
  theta = acos (num / denum);

  b[0] = givenAxis[1] * desiredAxis[2] - desiredAxis[1] * givenAxis[2];
  b[1] = desiredAxis[0] * givenAxis[2] - givenAxis[0] * desiredAxis[2];
  b[2] = givenAxis[0] * desiredAxis[1] - desiredAxis[0] * givenAxis[1];

  float normalizedB = sqrt (pow (b[0], 2) + pow (b[1], 2) + pow (b[2], 2));
  if (normalizedB != 0)
    {
      float q0,q1,q2,q3;
      float magnitudeQ;

      b[0] = b[0] / normalizedB;
      b[1] = b[1] / normalizedB;
      b[2] = b[2] / normalizedB;

      q0 = cos (theta / 2);
      q1 = sin (theta / 2) * b[0];
      q2 = sin (theta / 2) * b[1];
      q3 = sin (theta / 2) * b[2];
      magnitudeQ = sqrt (pow (q0,2) + pow (q1, 2) + pow (q2, 2) + pow (q3, 2));
      if (magnitudeQ != 0)
        {
          q0 = q0 / magnitudeQ;
          q1 = q1 / magnitudeQ;
          q2 = q2 / magnitudeQ;
          q3 = q3 / magnitudeQ;
        }

      rotm[0][0] = pow (q0, 2) + pow (q1, 2) - pow (q2,2) - pow (q3, 2);
      rotm[0][1] = 2 * (q1 * q2 - q0 * q3);

      if (rotm[0][1] == -0.0)
        {
          rotm[0][1] = 0;
        }
      rotm[0][2] = 2 * (q0 * q2 + q1 * q3);

      rotm[1][0] = 2 * (q0 * q3 + q1 * q2);
      rotm[1][1] = pow (q0,2) - pow (q1, 2) + pow (q2, 2) - pow (q3, 2);
      rotm[1][2] = 2 * (q2 * q3 - q0 * q1);

      rotm[2][0] = 2 * (q1 * q3 - q0 * q2);
      rotm[2][1] = 2 * (q0 * q1 + q2 * q3);
      rotm[2][2] = pow (q0, 2) - pow (q1, 2) - pow (q2, 2) + pow (q3, 2);
    }
  else if (normalizedB == 0 && theta == M_PI)
    {
      rotm[0][0] = -1;
      rotm[0][1] = 0;
      rotm[0][2] = 0;

      rotm[1][0] = 0;
      rotm[1][1] = -1;
      rotm[1][2] = 0;

      rotm[2][0] = 0;
      rotm[2][1] = 0;
      rotm[2][2] = -1;
    }
  else if (normalizedB == 0 && theta == 0)
    {
      rotm[0][0] = 1;
      rotm[0][1] = 0;
      rotm[0][2] = 0;

      rotm[1][0] = 0;
      rotm[1][1] = 1;
      rotm[1][2] = 0;

      rotm[2][0] = 0;
      rotm[2][1] = 0;
      rotm[2][2] = 1;
    }

  for (uint8_t i = 0; i < 3; i++)
    {
      floatVector_t rotmRow;
      for (uint8_t j = 0; j < 3; j++)
        {
          rotmRow.push_back (rotm[i][j]);
        }
      rotmVector.push_back (rotmRow);
    }
}

} // namespace ns3
