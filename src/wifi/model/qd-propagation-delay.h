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

#ifndef QD_PROPAGATION_DELAY_H
#define QD_PROPAGATION_DELAY_H

#include <ns3/mobility-model.h>
#include <ns3/propagation-delay-model.h>

#include <map>

namespace ns3 {

typedef std::vector<double> doubleVector_t;
typedef std::pair<uint32_t, uint32_t> CommunicatingPair;
typedef std::map<CommunicatingPair, doubleVector_t> PropagationDelays;
typedef PropagationDelays::iterator PropagationDelays_I;

class QdPropagationDelay : public PropagationDelayModel
{
public:
  static TypeId GetTypeId (void);
  QdPropagationDelay ();
  virtual ~QdPropagationDelay ();

  virtual Time GetDelay (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const;
  void SetSpeed (double speed);
  double GetSpeed (void) const;

protected:
  virtual void DoDispose ();

private:
  virtual int64_t DoAssignStreams (int64_t stream);
  doubleVector_t ReadQDPropagationDelays (uint16_t indexTx, uint16_t indexRx) const;
  void SetQdModelFolder (std::string folderName);
  void SetStartDistance (uint16_t startDistance);

private:
  std::string m_qdFolder;
  double m_speed;
  uint16_t m_startDistance;
  mutable uint16_t m_currentIndex;
  mutable PropagationDelays m_delays;

};

}  //namespace ns3

#endif /* QD_PROPAGATION_DELAY_H */
