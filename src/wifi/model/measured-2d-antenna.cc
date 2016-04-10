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

#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/object-vector.h"
#include "measured-2d-antenna.h"

#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("Measured2DAntenna");

NS_OBJECT_ENSURE_REGISTERED (Measured2DAntenna);

/* Utility Functions */
static double
getAngleDiff (double a, double b)
{
  double d = std::abs (a - b);
  while (d > 2 * M_PI)
    d -= 2*M_PI;
  if (d > M_PI)
    d = 2 * M_PI - d;
  return d;
}

static int
wrap_index (int i, int n)
{
  i = i % n;
  if (i < 0)
    return i+n;
  return i;
}

TypeId
M2D::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::M2D")
    .SetParent<Object> ()
    .AddConstructor<M2D> ()
  ;
  return tid;
}

M2D::M2D ()
{
}
M2D::M2D (double angle, double gain)
  : m_angle (angle), m_gain (gain)
{
}

M2D::~M2D ()
{
}

double
M2D::GetAngle (void) const
{
  return m_angle;
}

void
M2D::SetAngle (double angle)
{
  m_angle = angle;
}

double
M2D::GetGain (void) const
{
  return m_gain;
}

void
M2D::SetGain (double gain)
{
  m_gain = gain;
}

TypeId
Measured2DAntenna::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Measured2DAntenna")
    .SetParent<AbstractAntenna> ()
    .AddConstructor<Measured2DAntenna> ()
    .AddAttribute ("Azimuth",
                   "The azimuth angle (XY-plane) in which this antenna is pointed in radians.",
                   DoubleValue (0),
                   MakeDoubleAccessor (&Measured2DAntenna::m_azimuth),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Elevation",
                   "The elevation angle (Z-plane) in which this antenna is pointed in radians.",
                   DoubleValue (0),
                   MakeDoubleAccessor (&Measured2DAntenna::m_elevation),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("VerticalBeamwidth",
                   "The vertical beamwidth of this antenna in radians.",
                   DoubleValue (M_PI/18),	/* 10 degrees */
                   MakeDoubleAccessor (&Measured2DAntenna::m_verticalBeamwidth),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Mode",
		   "23 or 10.",
		   DoubleValue (23),
		   MakeDoubleAccessor (&Measured2DAntenna::GetMode, &Measured2DAntenna::SetMode),
		   MakeDoubleChecker<double> ())
    ;
  return tid;
}

Measured2DAntenna::Measured2DAntenna ()
  : m_verticalBeamwidth (M_PI/18)
{
}

Measured2DAntenna::~Measured2DAntenna ()
{
}

double
Measured2DAntenna::GetTxGainDbi (double azimuth, double elevation) const
{
  NS_LOG_FUNCTION (azimuth << elevation);
  if (getAngleDiff (elevation,m_elevation) > m_verticalBeamwidth/2)
    return -10000;
  return GetGain (azimuth);
}

double
Measured2DAntenna::GetRxGainDbi (double azimuth, double elevation) const
{
  NS_LOG_FUNCTION (azimuth << elevation);
  if (getAngleDiff (elevation,m_elevation) > m_verticalBeamwidth/2)
          return -10000;
  return GetGain (azimuth);
}

double
Measured2DAntenna::GetBeamwidth (void) const
{
  NS_LOG_FUNCTION (m_verticalBeamwidth);
  return m_verticalBeamwidth;
}

double
Measured2DAntenna::GetGain(double angle) const
{
  NS_LOG_FUNCTION (angle);

  if (m_measurements.size () == 0)
    NS_FATAL_ERROR ("trying to get gain with no measurements!");

  if (m_measurements.size () == 1)
    return m_measurements[0]->GetGain ();

  int i = 0, i1, i2;
  int S = m_measurements.size ();
  angle = angle - m_azimuth;
  double diff = getAngleDiff (m_measurements[0]->GetAngle (), angle);
  double diff1, diff2;
  double ret;
  while(true)
    {
      if (diff == 0)
        {
          ret = m_measurements[i]->GetGain();
          goto out;
        }

      i1 = wrap_index(i+1, S);
      diff1 = getAngleDiff (m_measurements[i1]->GetAngle (), angle);
      i2 = wrap_index(i-1, S);
      diff2 = getAngleDiff (m_measurements[i2]->GetAngle (), angle);

      if (diff1 < diff)
        {
          i = i1;
          diff = diff1;
          continue;
        }

      if (diff2 < diff)
        {
          i = i2;
          diff = diff2;
          continue;
        }

      if (diff1 < diff2)
        {
          break;
        }

      i1 = i;
      diff1 = diff;
      i = i2;
      diff = diff2;
      break;
    }

  /* At this point, i should be the closest, and i1===i+1 the next closest */
  NS_LOG(ns3::LOG_INFO, "i=" << i << " i1=" << i1 << " gain[i]=" <<
         m_measurements[i]->GetGain () << " gain[i1]=" <<
         m_measurements[i1]->GetGain () << " diff=" << diff <<
         " diff1=" << diff1);

  ret = m_measurements[i]->GetGain () * (diff1/(diff + diff1)) +
         m_measurements[i1]->GetGain () * (diff/(diff + diff1));

out:
  NS_LOG(ns3::LOG_INFO, "returning " << ret);
  return ret;
}

double
Measured2DAntenna::GetAzimuthAngle (void) const
{
  NS_LOG_FUNCTION (m_azimuth);
  return m_azimuth;
}

void
Measured2DAntenna::SetAzimuthAngle (double azimuth)
{
  NS_LOG_FUNCTION (azimuth);
  m_azimuth = azimuth;
}

double
Measured2DAntenna::GetElevationAngle (void) const
{
  NS_LOG_FUNCTION (m_elevation);
  return m_elevation;
}

void
Measured2DAntenna::SetElevationAngle (double elevation)
{
  NS_LOG_FUNCTION(elevation);
  m_elevation = elevation;
}

void
Measured2DAntenna::SetMode (double mode)
{
  NS_LOG_FUNCTION (mode);
  if (mode != 10 && mode != 23 && mode != 800)
    NS_FATAL_ERROR("illegal mode " << mode << " != 10 or 23 or 800");

  m_mode = mode;
  m_measurements.clear ();

  if (mode == 23)
    {
      m_measurements.push_back (CreateObject<M2D> (0, 45.9));
      m_measurements.push_back (CreateObject<M2D> (15, 25.3));
      m_measurements.push_back (CreateObject<M2D> (30, 18.2));
      m_measurements.push_back (CreateObject<M2D> (60, 6.2));
      m_measurements.push_back (CreateObject<M2D> (90, 2.6));
      m_measurements.push_back (CreateObject<M2D> (120, 0.4));
      m_measurements.push_back (CreateObject<M2D> (150, 2.8));
      m_measurements.push_back (CreateObject<M2D> (180, 0));
      m_measurements.push_back (CreateObject<M2D> (-150, 1));
      m_measurements.push_back (CreateObject<M2D> (-120, 2));
      m_measurements.push_back (CreateObject<M2D> (-90, 2.8));
      m_measurements.push_back (CreateObject<M2D> (-60, 6.9));
      m_measurements.push_back (CreateObject<M2D> (-30, 15.5));
      m_measurements.push_back (CreateObject<M2D> (-15, 28.7));
      for (unsigned int t = 0; t < m_measurements.size (); ++t)
        {
          m_measurements[t]->SetGain (m_measurements[t]->GetGain () - 16.8);
          m_measurements[t]->SetAngle (m_measurements[t]->GetAngle () * M_PI/180);
        }
  }
  else if (mode == 10)
    {
      m_measurements.push_back (CreateObject<M2D> (0, 26.3));
      m_measurements.push_back (CreateObject<M2D> (15, 25.8));
      m_measurements.push_back (CreateObject<M2D> (30, 22.8));
      m_measurements.push_back (CreateObject<M2D> (60, 12.6));
      m_measurements.push_back (CreateObject<M2D> (90, 4.1));
      m_measurements.push_back (CreateObject<M2D> (120, 3.4));
      m_measurements.push_back (CreateObject<M2D> (150, 2.9));
      m_measurements.push_back (CreateObject<M2D> (180, 0));
      m_measurements.push_back (CreateObject<M2D> (-150, 1.3));
      m_measurements.push_back (CreateObject<M2D> (-120, 2.6));
      m_measurements.push_back (CreateObject<M2D> (-90, 5.3));
      m_measurements.push_back (CreateObject<M2D> (-60, 13.9));
      m_measurements.push_back (CreateObject<M2D> (-30, 23.2));
      m_measurements.push_back (CreateObject<M2D> (-15, 26.8));
      for (unsigned int t = 0; t < m_measurements.size(); ++t)
        {
          m_measurements[t]->SetGain (m_measurements[t]->GetGain () - 16.8);
          m_measurements[t]->SetAngle (m_measurements[t]->GetAngle () * M_PI/180);
        }
    }
  else if (mode == 800)
    {
      m_measurements.push_back (CreateObject<M2D> (-90+5, -28.08));
      m_measurements.push_back (CreateObject<M2D> (-90+11.25, -17.08));
      m_measurements.push_back (CreateObject<M2D> (-90+22.5, -13.08));
      m_measurements.push_back (CreateObject<M2D> (-90+33.75, -5.08));
      m_measurements.push_back (CreateObject<M2D> (-90+45, -0.08));
      m_measurements.push_back (CreateObject<M2D> (-90+56.25, 4.92));
      m_measurements.push_back (CreateObject<M2D> (-90+67.5, 5.92));
      m_measurements.push_back (CreateObject<M2D> (-90+78.75, 6.92));
      m_measurements.push_back (CreateObject<M2D> (0, 7.92));
      m_measurements.push_back (CreateObject<M2D> (11.25, 6.92));
      m_measurements.push_back (CreateObject<M2D> (22.5, 5.92));
      m_measurements.push_back (CreateObject<M2D> (33.75, 4.92));
      m_measurements.push_back (CreateObject<M2D> (45, -0.08));
      m_measurements.push_back (CreateObject<M2D> (56.25, -5.08));
      m_measurements.push_back (CreateObject<M2D> (67.5, -13.08));
      m_measurements.push_back (CreateObject<M2D> (78.75, -17.08));
      m_measurements.push_back (CreateObject<M2D> (85, -28.08));
      m_measurements.push_back (CreateObject<M2D> (90, -16.08));
      m_measurements.push_back (CreateObject<M2D> (90+11.25, -17.08));
      m_measurements.push_back (CreateObject<M2D> (90+22.5, -28.08));
      m_measurements.push_back (CreateObject<M2D> (90+33.75, -16.08));
      m_measurements.push_back (CreateObject<M2D> (90+45, -14.08));
      m_measurements.push_back (CreateObject<M2D> (90+56.25, -15.08));
      m_measurements.push_back (CreateObject<M2D> (90+67.5, -28.08));
      m_measurements.push_back (CreateObject<M2D> (90+78.75, -14.58));
      m_measurements.push_back (CreateObject<M2D> (180, -14.08));
      m_measurements.push_back (CreateObject<M2D> (180+11.25, -14.58));
      m_measurements.push_back (CreateObject<M2D> (180+22.5, -28.08));
      m_measurements.push_back (CreateObject<M2D> (180+33.75, -15.08));
      m_measurements.push_back (CreateObject<M2D> (180+45, -14.08));
      m_measurements.push_back (CreateObject<M2D> (180+56.25, -16.08));
      m_measurements.push_back (CreateObject<M2D> (180+67.5, -28.08));
      m_measurements.push_back (CreateObject<M2D> (180+78.75, -17.08));
      m_measurements.push_back (CreateObject<M2D> (270, -16.08));

      for (unsigned int t = 0; t < m_measurements.size (); ++t)
        {
          m_measurements[t]->SetAngle (m_measurements[t]->GetAngle () * M_PI/180);
        }
    }
}

double
Measured2DAntenna::GetMode (void) const
{
  return m_mode;
}

}
