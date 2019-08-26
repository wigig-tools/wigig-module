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

#ifndef QD_CHANNEL_H
#define QD_CHANNEL_H

#include <ns3/angles.h>
#include <ns3/mobility-model.h>
#include <ns3/net-device.h>
#include <ns3/net-device-container.h>
#include <ns3/random-variable-stream.h>
#include <ns3/spectrum-propagation-loss-model.h>
#include <ns3/spectrum-signal-parameters.h>
#include <ns3/spectrum-value.h>

#include <complex>
#include <map>
#include <tuple>

#include "codebook-parametric.h"

namespace ns3 {

typedef std::vector<double> doubleVector_t;
typedef std::vector<std::complex<double> > complexVector_t;
typedef std::vector<complexVector_t> complex2DVector_t;
typedef std::vector<doubleVector_t> double2DVector_t;
typedef std::vector<float> floatVector_t;
typedef std::vector<floatVector_t> float2DVector_t;

struct AnglesTransformed {
  double elevation;
  double azimuth;
};

typedef std::tuple<AntennaID, bool, uint8_t> AntennaConfig;
typedef AntennaConfig AntennaConfigTx;
typedef AntennaConfig AntennaConfigRx;
typedef std::tuple<Ptr<NetDevice>, Ptr<NetDevice>, AntennaConfigTx, AntennaConfigRx> LinkConfiguration;
typedef std::map<LinkConfiguration, Ptr<SpectrumValue> > ChannelMatrix;
typedef ChannelMatrix::iterator ChannelMatrix_I;
typedef ChannelMatrix::const_iterator ChannelMatrix_CI;
typedef std::pair<uint32_t, uint32_t> CommunicatingPair;
typedef std::vector<CommunicatingPair> TraceFiles;
typedef TraceFiles::iterator TraceFiles_I;

class QdPropagationLossModel : public SpectrumPropagationLossModel
{
public:
  static TypeId GetTypeId (void);

  QdPropagationLossModel ();
  virtual ~QdPropagationLossModel ();

  uint16_t GetCurrentTraceIndex (void) const;
  void AddCustomID (const uint32_t nodeID, const uint32_t customID);

protected:
  virtual void DoDispose ();

private:
  Ptr<SpectrumValue> DoCalcRxPowerSpectralDensity (Ptr<const SpectrumValue> txPsd,
                                                   Ptr<const MobilityModel> a,
                                                   Ptr<const MobilityModel> b) const;
  void InitializeQDModelParameters (Ptr<const MobilityModel> a, Ptr<const MobilityModel> b,
                                    uint16_t indexTx, uint16_t indexRx) const;
  Ptr<SpectrumValue> GetChannelGain (Ptr<const SpectrumValue> txPsd, uint16_t pathNum,
                                     uint32_t indexTx, uint32_t indexRx,
                                     Ptr<CodebookParametric> txCodebook, Ptr<CodebookParametric> rxCodebook) const;
  void QuaternionTransform (double givenAxix[3], double desiredAxix[3], float2DVector_t& rotmVector) const;
  AnglesTransformed GetTransformedAngles(double elevation, double azimuth, bool isDoa, float2DVector_t& rotmVector) const;
  void SetQdModelFolder (const std::string folderName);
  void SetStartDistance (const uint16_t startDistance);
  uint32_t MapID (const uint32_t nodeID) const;

private:
  mutable ChannelMatrix m_channelMatrixMap;
  std::string m_qdFolder;
  Ptr<UniformRandomVariable> m_uniformRv;
  double m_speed;
  uint16_t m_startDistance;
  mutable uint16_t m_currentIndex;
  mutable TraceFiles m_traceFiles;
  mutable uint16_t m_numTraces;

  mutable std::map<uint16_t, std::map<uint16_t, doubleVector_t> >   nbMultipathTxRx;
  mutable std::map<uint16_t, std::map<uint16_t, double2DVector_t> > delayTxRx;
  mutable std::map<uint16_t, std::map<uint16_t, double2DVector_t> > pathLossTxRx;
  mutable std::map<uint16_t, std::map<uint16_t, double2DVector_t> > phaseTxRx;
  mutable std::map<uint16_t, std::map<uint16_t, double2DVector_t> > dopplerShiftTxRx;
  mutable std::map<uint16_t, std::map<uint16_t, double2DVector_t> > aodAzimuthTxRx[8];
  mutable std::map<uint16_t, std::map<uint16_t, double2DVector_t> > aodElevationTxRx[8];
  mutable std::map<uint16_t, std::map<uint16_t, double2DVector_t> > aoaElevationTxRx[8];
  mutable std::map<uint16_t, std::map<uint16_t, double2DVector_t> > aoaAzimuthTxRx[8];

  std::map<uint32_t, uint32_t> nodeId2QdId;
  bool m_useCustomIDs;

};

}  //namespace ns3

#endif /* QD_CHANNEL_H */
