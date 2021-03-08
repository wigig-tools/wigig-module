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

#ifndef QD_PROPAGATION_DELAY_MODEL_H
#define QD_PROPAGATION_DELAY_MODEL_H

#include <ns3/mobility-model.h>
#include <ns3/propagation-delay-model.h>

#include <map>

namespace ns3 {

class QdPropagationEngine;

class QdPropagationDelayModel : public PropagationDelayModel
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
   * Use the default parameters from PropagationDelayConstantSpeed.
   */
  QdPropagationDelayModel ();
  virtual ~QdPropagationDelayModel ();
  /**
   * Class constructor
   * \param qdPropagationEngine Pointer to the Q-D Propagation Engine class.
   */
  QdPropagationDelayModel (Ptr<QdPropagationEngine> qdPropagationEngine);

  /**
   * Get Delay
   * \param a
   * \param b
   * \return
   */
  virtual Time GetDelay (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const;

protected:
  virtual void DoDispose ();

private:
  virtual int64_t DoAssignStreams (int64_t stream);

private:
  Ptr<QdPropagationEngine> m_qdPropagationEngine;

};

}  //namespace ns3

#endif /* QD_PROPAGATION_DELAY_H */
