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

#ifndef QD_PROPAGATION_ENGINE_H
#define QD_PROPAGATION_ENGINE_H

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

typedef std::vector<float> floatVector_t;
typedef std::vector<floatVector_t> float2DVector_t;

/**
 * A unique identifier for the Q-D channel trace.
 * SRC Node ID, Destination Node ID, Q-D TraceIndex, Tx Antenna ID, Rx Antenna ID.
 */
typedef std::tuple<uint32_t, uint32_t, uint32_t, AntennaID, AntennaID> QdChanneldentifier;
typedef std::map<QdChanneldentifier, floatVector_t> ChannelCoefficientMap;
typedef ChannelCoefficientMap::iterator ChannelCoefficientMap_I;
typedef ChannelCoefficientMap::iterator ChannelCoefficientMap_CI;

/**
 * The transformed angles after rounding the double values.
 */
struct AnglesTransformed {
  uint16_t elevation;
  uint16_t azimuth;
};

typedef std::pair<AntennaID, Ptr<PatternConfig> > AntennaConfig;                //!< Generic antenna array configuration pair.
typedef AntennaConfig AntennaConfigTx;                                          //!< Transmit phased antenna array configuration pair.
typedef AntennaConfig AntennaConfigRx;                                          //!< Receive phased antenna array configuration pair.
typedef std::tuple<Ptr<NetDevice>, Ptr<NetDevice>, AntennaConfigTx, AntennaConfigRx> LinkConfiguration; //!< Link Configuration key.
typedef std::map<LinkConfiguration, Ptr<SpectrumValue> > ChannelGainMatrix;     //!< Channel gain matrix defining channel gain for all the possible combinations in the scenario.
typedef ChannelGainMatrix::iterator ChannelGainMatrix_I;                        //!< Typedef for iterator over channel gain matrix.
typedef ChannelGainMatrix::const_iterator ChannelMatrix_CI;                     //!< Typedef for constant iterator over channel matrix.
typedef std::pair<uint32_t, uint32_t> CommunicatingPair;                        //!< Typedef for identifying communicating pair.
typedef std::vector<CommunicatingPair> TraceFiles;                              //!< Check whether trace files have been loaded or not.
typedef TraceFiles::iterator TraceFiles_I;                                      //!< Typedef for iterator over traces files.

class NodeContainer;
struct DmgWifiSpectrumSignalParameters;

class QdPropagationEngine : public Object
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  QdPropagationEngine ();
  virtual ~QdPropagationEngine ();

  /**
   * Get Current Trace Index.
   * \return Return the trace index in Q-D Channel.
   */
  uint16_t GetCurrentTraceIndex (void) const;
  /**
   * Map ns-3 node's ID to a custom ID to be used for reading the Q-D files.
   * \param nodeID The ID assigned to the node by ns-3.
   * \param qdID The ID assigned by the Q-D realization software.
   */
  void AddCustomID (const uint32_t nodeID, const uint32_t qdID);
  /**
   * Convert ns-3 node's ID to the Q-D ID to be used for reading Q-D files.
   * \param nodeID The ID assigned to the node by ns-3.
   * \return The Q-D ID to be used fo reading Q-D files.
   */
  uint32_t GetQdID (const uint32_t nodeID) const;
  /**
   * Read nodes configuration file and create the corresponding list of DMG PCP/APs and DMG STAs.
   * \param The name of the nodes configuration file containing a description to network topology.
   * \param numAPs The number of created DMG PCP/APs.
   * \param apWifiNodes The node container for DMG PCP/AP.
   * \param staWifiNodes The node container for DMG STA.
   * \param staNodesGroups The list of node containers.
   */
  void ReadNodesConfigurationFile (std::string nodesConfugrationFile,
                                   uint16_t &numAPs, NodeContainer &apWifiNodes,
                                   NodeContainer &staWifiNodes, std::vector<NodeContainer> &staNodesGroups);

protected:
  virtual void DoDispose ();

protected:
  friend class QdPropagationLossModel;
  friend class QdPropagationDelayModel;

  /**
   * Get propagation delay between two devices/antennas.
   * It is important that this function gets called after the CalcRxPowerSpectralDensity function.
   * \param a Pointer to the mobility model associated with the transmitting node.
   * \param a Pointer to the mobility model associated with the receiving node.
   * \return The propagation delay between two devices/antennas in Seconds.
   */
  Time GetDelay (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const;
  /**
   * This method is to be called to calculate PSD at the receiver side.
   *
   * \param params the SpectrumSignalParameters of the signal being received.
   * \param a sender mobility
   * \param b receiver mobility
   *
   * \return set of values Vs frequency representing the received
   * power in the same units used for the txPower parameter.
   */
  Ptr<SpectrumValue> CalcRxPower (Ptr<SpectrumSignalParameters> params,
                                  Ptr<const MobilityModel> a,
                                  Ptr<const MobilityModel> b) const;
  /**
   * This method is to be called to calculate PSD for MIMO transmission.
   *
   * \param params the SpectrumSignalParameters of the signal being received.
   * \param a sender mobility
   * \param b receiver mobility
   *
   * \return set of values Vs frequency representing the received
   * power in the same units used for the txPower parameter.
   */
  void CalcMimoRxPower (Ptr<SpectrumSignalParameters> params,
                        Ptr<const MobilityModel> a,
                        Ptr<const MobilityModel> b) const;

private:
  /**
   * Handle mobility by changing Q-D trace index.
   */
  void HandleMobility (void) const;

  /**
   * Initialize Q-D Channel model parameters.
   * \param a Mobility model of the transmitter.
   * \param b Mobility model of the receivers.
   * \param indexTx The ID of the Tx node.
   * \param indexRx The ID of the Rx node.
   */
  void InitializeQDModelParameters (Ptr<const MobilityModel> a, Ptr<const MobilityModel> b,
                                    uint16_t indexTx, uint16_t indexRx) const;
  /**
   * Compute the channel gain between two devices or antennas.
   * \param rxPsd The received power spectral density.
   * \param pathNum The number of multipath components between Tx and Rx devices/antennas.
   * \param chId Q-D channel profile identifier.
   * \param txCodebook Pointer to the codebook of the Tx device.
   * \param rxCodebook Pointer to the codebook of the Rx device.
   * \param txPattern Pointer to the transmit pattern configuration.
   * \param rxPattern Pointer to the receive pattern configuration
   * \return Channel gain between Tx and Tx device as Spectrum Value.
   */
  Ptr<SpectrumValue> GetChannelGain (Ptr<SpectrumValue> rxPsd,
                                     uint16_t pathNum, QdChanneldentifier chId,
                                     Ptr<CodebookParametric> txCodebook, Ptr<CodebookParametric> rxCodebook,
                                     Ptr<PatternConfig> txPattern, Ptr<PatternConfig> rxPattern) const;
  /**
   * Euler Transformtion for phased antenna array rotation.
   * \param orientation The orienation of the phased antenna array using Euler angles.
   * \param rotMatrix Rotation matrix corresponding to the euler transformation.
   */
  void EulerTransform (Orientation orientation, float2DVector_t& rotMatrix) const;
  /**
   * Transform angles using the rotation vector obtained from the quaternion transformation.
   * \param elevation The elevation angle in radians.
   * \param azimuth The azimuth angle in radians.
   * \param isDoa Flag to indicate if it is direction of arrival.
   * \param rotmVector Rotation vectors.
   * \return The transformed angles in the new coordinate system measured in degree.
   */
  AnglesTransformed GetTransformedAngles (double elevation, double azimuth, bool isDoa, float2DVector_t& rotmVector) const;
  /**
   * Set Q-D Channel model folder path.
   * \param folderName The path to the Q-D Channel files.
   */
  void SetQdModelFolder (const std::string folderName);
  /**
   * Set the starting index in a Q-D file.
   * \param startIndex The starting index in a Q-D file.
   */
  void SetStartIndex (const uint32_t startIndex);

private:
  mutable ChannelGainMatrix m_channelGainMatrix;//!< Channel matrix for the whole communication network.
  std::string m_qdFolder;                       //!< Folder that contains all the Q-D Channel model files.
  Ptr<UniformRandomVariable> m_uniformRv;       //!< Uniform random variable for doppler.
  Time m_interval;                              //!< The interval between two consecutive traces.
  uint32_t m_startIndex;                        //!< Starting point in a Q-D file.
  mutable uint32_t m_currentIndex;              //!< Current index in the trace file.
  mutable TraceFiles m_traceFiles;              //!< Status of the traces files.
  mutable uint32_t m_numTraces;                 //!< The number of traces in Q-D files.

  mutable std::map<QdChanneldentifier, uint32_t>  nbMultipathTxRx;      //!< Number of multipaths components.
  mutable ChannelCoefficientMap                   delayTxRx;            //!< Delay spread in ns.
  mutable ChannelCoefficientMap                   pathLossTxRx;         //!< PathLoss (dB).
  mutable ChannelCoefficientMap                   phaseTxRx;            //!< Phase (radians).
  mutable ChannelCoefficientMap                   dopplerShiftTxRx;     //!< Doppler shift in Hz.
  mutable ChannelCoefficientMap                   aodAzimuthTxRx;       //!< AoD Azimuth (Degrees).
  mutable ChannelCoefficientMap                   aodElevationTxRx;     //!< AoD Elevation (Degrees).
  mutable ChannelCoefficientMap                   aoaElevationTxRx;     //!< AoA Elevation (Degrees).
  mutable ChannelCoefficientMap                   aoaAzimuthTxRx;       //!< AoA Azimuth (Degrees).

  std::map<uint32_t, uint32_t> nodeId2QdId; //!< Structure to map node ID to Q-D Channel ID.
  bool m_useCustomIDs;                      //!< Flag to indicate whether we use custom list to map ns-3 nodes IDs to Q-D Software IDs.

};

}  //namespace ns3

#endif /* QD_PROPAGATION_ENGINE_H */
