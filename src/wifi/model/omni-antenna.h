/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010
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
 * Author: Daniel Halperin <dhalperi@cs.washington.edu>
 */

#ifndef OMNI_ANTENNA_H
#define OMNI_ANTENNA_H

#include "abstract-antenna.h"

namespace ns3 {

/**
 * \brief Antenna functionality for wireless devices
 *
 */
class OmniAntenna : public AbstractAntenna
{

public:
  static TypeId GetTypeId (void);

  OmniAntenna ();
  ~OmniAntenna ();

  double GetTxGainDbi (double azimuth, double elevation) const;
  double GetRxGainDbi (double azimuth, double elevation) const;

  double GetGainDbi (void) const;
  void SetGainDbi (double gainDbi);
  double GetBeamwidth (void) const;

private:
  OmniAntenna (const OmniAntenna &o);
  OmniAntenna & operator = (const OmniAntenna &o);

  double m_gainDbi;
  double m_beamwidth;
};

} // namespace ns3

#endif /* OMNI_ANTENNA_H */
