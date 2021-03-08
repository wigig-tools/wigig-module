/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
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

#ifndef QD_PROPAGATION_LOSS_MODEL_H
#define QD_PROPAGATION_LOSS_MODEL_H

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

namespace ns3 {

class QdPropagationEngine;

class QdPropagationLossModel : public SpectrumPropagationLossModel
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  QdPropagationLossModel ();
  virtual ~QdPropagationLossModel ();
  /**
   * Class constructor
   * \param qdPropagationEngine Pointer to the Q-D Propagation Engine class.
   */
  QdPropagationLossModel (Ptr<QdPropagationEngine> qdPropagationEngine);

protected:
  virtual void DoDispose ();

private:
  Ptr<SpectrumValue> DoCalcRxPowerSpectralDensity (Ptr<const SpectrumValue> txPsd,
                                                   Ptr<const MobilityModel> a,
                                                   Ptr<const MobilityModel> b) const;
  /**
   * By default this function is set to false to allow backward compatability.
   * \return Return true if we store we calculate the received power at the receiver side, otherwise false.
   */
  bool DoCalculateRxPowerAtReceiverSide (void) const;
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
  virtual Ptr<SpectrumValue> CalcRxPower (Ptr<SpectrumSignalParameters> params,
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
  Ptr<QdPropagationEngine> m_qdPropagationEngine;

};

}  //namespace ns3

#endif /* QD_PROPAGATION_LOSS_MODEL_H */
