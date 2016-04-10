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

#include "ns3/log.h"
#include "ns3/double.h"
#include "omni-antenna.h"
#include <math.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("OmniAntenna");
NS_OBJECT_ENSURE_REGISTERED (OmniAntenna);

static double
GainDbiToBeamwidth(double gainDbi)
{
	double invgain = pow(10, -gainDbi/10);
	return asin(invgain)*2;
}

TypeId
OmniAntenna::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::OmniAntenna")
    .SetParent<AbstractAntenna> ()
    .AddConstructor<OmniAntenna> ()
    .AddAttribute ("GainDbi",
                   "The gain of this omni antenna in dBi.",
                   DoubleValue (7),
                   MakeDoubleAccessor (&OmniAntenna::GetGainDbi,
			   &OmniAntenna::SetGainDbi),
                   MakeDoubleChecker<double> ())
    ;
  return tid;
}

OmniAntenna::OmniAntenna()
	: m_gainDbi(0), m_beamwidth(M_PI)
{
}
OmniAntenna::~OmniAntenna() { }

double
OmniAntenna::GetTxGainDbi(double azimuth, double elevation) const
{
	NS_LOG_FUNCTION(azimuth << elevation);
	if (elevation > m_beamwidth/2)
		return 0;
	return m_gainDbi;
}

double
OmniAntenna::GetRxGainDbi(double azimuth, double elevation) const
{
	NS_LOG_FUNCTION(azimuth << elevation);
	if (elevation > m_beamwidth/2)
		return 0;
	return m_gainDbi;
}

double
OmniAntenna::GetGainDbi(void) const
{
	NS_LOG_FUNCTION(m_gainDbi);
	return m_gainDbi;
}

void
OmniAntenna::SetGainDbi(double gainDbi)
{
	m_gainDbi = gainDbi;
	m_beamwidth = GainDbiToBeamwidth(m_gainDbi);
	NS_LOG_FUNCTION(gainDbi << m_beamwidth);
}

double
OmniAntenna::GetBeamwidth(void) const
{
	NS_LOG_FUNCTION(m_beamwidth);
	return m_beamwidth;
}

};
