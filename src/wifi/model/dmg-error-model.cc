/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 University of Washington
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
 * Authors: Benjamin Cizdziel <ben.cizdziel@gmail.com>
 *          Hany Assasa <hany.assasa@imdea.org>
 */

#include "ns3/enum.h"
#include "ns3/log.h"
#include "ns3/string.h"
#include "dmg-error-model.h"
#include "wifi-utils.h"
#include "wifi-tx-vector.h"

#include <cmath>
#include <fstream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DmgErrorModel");

NS_OBJECT_ENSURE_REGISTERED (DmgErrorModel);

void
SNR2BER_STRUCT::DetermineSnrOffset (void)
{
  NS_LOG_FUNCTION (this);
  double snr = snrMin;
  double toleranceForAssumingZero = 0.0001;
  for (int i = 0; i < numSnrDecPlaces; i++)
    {
      toleranceForAssumingZero /= 10;
    }
  if (snrMin < 0)
    {
      for (; snr <= 2 * snrSpacing; snr += snrSpacing)
        {
          if (snr >= 0)
            {
              snrOffset = snr;
              break;
            }
        }
      if (std::abs (snr) <= toleranceForAssumingZero) //if less than tolerance assume 0
        {
          snrOffset = 0;
        }
    }
  else if (snrMin > 0)
    {
      for (; snr >= -2 * snrSpacing; snr -= snrSpacing)
        {
          if (snr <= 0)
            {
              snrOffset = snr;
              break;
            }
        }
      if (std::abs (snr) <= toleranceForAssumingZero) //if less than tolerance assume 0
        {
          snrOffset = 0;
        }
    }
  else //if m_snrMin == 0
    {
      snrOffset = 0;
    }
}

double
SNR2BER_STRUCT::GetBitErrorRate (double snr)
{
  NS_LOG_FUNCTION (this << snr);
  if (snr <= snrMin)
    {
      NS_LOG_DEBUG ("SNR is lower than the minimum datapoint; No interpolation needed--direct table lookup for bit error rate.");
      return berMin;
    }
  else if (snr >= snrMax)
    {
      NS_LOG_DEBUG ("SNR is higher than the maximum datapoint; No interpolation needed--direct table lookup for bit error rate.");
      return berMax;
    }
  else
    {
      NS_LOG_DEBUG ("Performing linear interpolation on snr for bit error rate lookup.");
      InterpolParams iParams;
      double ber; //bit error rate to return
      iParams = FindDatapointBounds (snr);
      ber = InterpolateAndRetrieveData (snr, iParams.snrLoBound, iParams.snrHiBound);
      return ber;
    }
}

int
SNR2BER_STRUCT::RoundDoubleToInt (double val)
{
  NS_LOG_FUNCTION (this << val);
  if (val > 0.0)
    {
      return val + 0.5;
    }
  else
    {
      return val - 0.5;
    }
}

double
SNR2BER_STRUCT::RoundToNearestDataMidpoint (double value)
{
  NS_LOG_FUNCTION (this << value);
  double newOffset = (snrSpacing / 2) + snrOffset;
  return (RoundDoubleToInt ((value - newOffset) / snrSpacing) * snrSpacing) + newOffset;
}

SNR2BER_STRUCT::InterpolParams
SNR2BER_STRUCT::FindDatapointBounds (double snr)
{
  InterpolParams iParams;
  double snrRatio = snr / snrSpacing;
  double snrLoBoundNoOffset = floor (snrRatio) * snrSpacing;
  double snrHiBoundNoOffset = ceil (snrRatio) * snrSpacing;
  double nearestMidSnrPoint = RoundToNearestDataMidpoint (snr);
  if (snr > nearestMidSnrPoint)
    {
      iParams.snrLoBound = snrLoBoundNoOffset - snrOffset;
      iParams.snrHiBound = snrHiBoundNoOffset - snrOffset;
    }
  else
    {
      iParams.snrLoBound = snrLoBoundNoOffset + snrOffset;
      iParams.snrHiBound = snrHiBoundNoOffset + snrOffset;
    }
  return iParams;
}

int
SNR2BER_STRUCT::DoubleToHashKeyInt (double val)
{
  NS_LOG_FUNCTION (this << val);
  for (uint8_t i = 0; i < numSnrDecPlaces; i++)
    {
      val = val * 10;
    }
  return (int)(round (val));
}

double
SNR2BER_STRUCT::InterpolateAndRetrieveData (double xd, double x1d, double x2d)
{
  NS_LOG_FUNCTION (this << xd << x1d << x2d);
  int x1 = DoubleToHashKeyInt (x1d);
  int x2 = DoubleToHashKeyInt (x2d);
  std::map<int, double>::iterator elemIter;
  double fp; //retrieved value after any interpolation
  double fq1, fq2;
  elemIter = bitErrorRateTable.find (x1);
  if (elemIter == bitErrorRateTable.end ())
    {
      NS_FATAL_ERROR ("No bit error rate data stored for snr key = " << x1);
    }
  else
    {
      fq1 = (*elemIter).second;
    }
  elemIter = bitErrorRateTable.find (x2);
  if (elemIter == bitErrorRateTable.end ())
    {
      NS_FATAL_ERROR ("No bit error rate data stored for snr key = " << x2);
    }
  else
    {
      fq2 = (*elemIter).second;
    }
  fp = (((x2d - xd) / (x2d - x1d)) * fq1) + (((xd - x1d) / (x2d - x1d)) * fq2);
  NS_LOG_DEBUG ("BER1=" << fq1 << ", BER2=" << fq2 << ", BER=" << fp);
  return fp;
}

TypeId
DmgErrorModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DmgErrorModel")
    .SetParent<ErrorRateModel> ()
    .AddConstructor<DmgErrorModel> ()
    .AddAttribute ("FileName",
                   "The name of the file that contains SNR to BER tables.",
                   StringValue (""),
                   MakeStringAccessor (&DmgErrorModel::SetErrorRateTablesFileName),
                   MakeStringChecker ())
  ;
  return tid;
}

DmgErrorModel::DmgErrorModel ()
  : m_errorRateTablesLoaded (false),
    m_numSnrDecPlaces (0),
    m_snrSpacing (1),
    m_numMCSs (0)
{
  NS_LOG_FUNCTION (this);
}

DmgErrorModel::~DmgErrorModel ()
{
  NS_LOG_FUNCTION (this);
}

double
DmgErrorModel::GetChunkSuccessRate (WifiMode mode, WifiTxVector txVector, double snr, uint64_t nbits) const
{
  NS_LOG_FUNCTION (this << mode.GetModulationClass () << uint16_t (mode.GetMcsValue ()) << RatioToDb (snr) << nbits);
  NS_ASSERT_MSG (mode.GetModulationClass () == WIFI_MOD_CLASS_DMG_CTRL ||
    mode.GetModulationClass () == WIFI_MOD_CLASS_DMG_SC ||
    mode.GetModulationClass () == WIFI_MOD_CLASS_DMG_OFDM ||
    mode.GetModulationClass () == WIFI_MOD_CLASS_EDMG_CTRL ||
    mode.GetModulationClass () == WIFI_MOD_CLASS_EDMG_SC ||
    mode.GetModulationClass () == WIFI_MOD_CLASS_EDMG_OFDM,
    "Expecting 802.11ad DMG CTRL, SC or OFDM modulation or 802.11ay EDMG CTRL, SC or OFDM modulation");

  Ptr<SNR2BER_STRUCT> snr2ber = m_snr2berList.find (mode.GetMcsValue ())->second;
  double ber = snr2ber->GetBitErrorRate (RatioToDb (snr));
  double psr = pow (1 - ber, nbits); /* Compute Packet Success Rate (PSR) from BER */
  NS_LOG_DEBUG ("PSR=" << psr);

  return psr;
}

void
DmgErrorModel::SetErrorRateTablesFileName (std::string fileName)
{
  NS_LOG_FUNCTION (this << fileName);
  if (fileName != "")
    {
      m_fileName = fileName;
      LoadErrorRateTables ();
    }
}

void
DmgErrorModel::LoadErrorRateTables (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (!m_errorRateTablesLoaded, "bit error rate table has already been loaded");
  std::pair<std::map<int, double>::iterator, bool> ret;

  std::ifstream file;
  file.open (m_fileName, std::ifstream::in);
  NS_ASSERT_MSG (file.good (), "SNR to BER File not found");
  std::string line;

  MCS_IDX idx;
  std::string value;
  int snrInt;  /* SNR converted to an integer to use as hash key */

  /* Read the number of MCSs in the file */
  std::getline (file, line);
  m_numMCSs = std::stod (line);

  /* Read number of SNR Decimal Places */
  std::getline (file, line);
  m_numSnrDecPlaces = std::stoul (line);

  /* Read SNR Spacing value */
  std::getline (file, line);
  m_snrSpacing = std::stod (line);

  for (uint8_t i = 0; i < m_numMCSs; i++)
    {
      std::vector<double> snrs, bers;
      Ptr<SNR2BER_STRUCT> snr2berStruct = Create<SNR2BER_STRUCT> ();

      /* Assign the global SNR Spacing and number of decimal places */
      snr2berStruct->numSnrDecPlaces = m_numSnrDecPlaces;
      snr2berStruct->snrSpacing = m_snrSpacing;

      /* Read MCS Index */
      std::getline (file, line);
      idx = std::stoul (line);

      /* Read SNR min value */
      std::getline (file, line);
      snr2berStruct->snrMin = std::stod (line);

      /* Read SNR max value */
      std::getline (file, line);
      snr2berStruct->snrMax = std::stod (line);

      /* Read BER value corresponding to the min SNR value */
      std::getline (file, line);
      snr2berStruct->berMin = std::stod (line);

      /* Read BER value corresponding to the max SNR value */
      std::getline (file, line);
      snr2berStruct->berMax = std::stod (line);

      /* Read the number of SNR values */
      std::getline (file, line);
      snr2berStruct->numDataPoints = std::stoul (line);

      /* Read SNR Values */
      std::getline (file, line);
      std::istringstream split1 (line);
      for (uint16_t n = 0; n < snr2berStruct->numDataPoints; n++)
        {
          std::getline (split1, value, ',');
          snrs.push_back (std::stod (value));
        }

      /* Read BER Values */
      std::getline (file, line);
      std::istringstream split2 (line);
      for (uint16_t n = 0; n < snr2berStruct->numDataPoints; n++)
        {
          std::getline (split2, value, ',');
          bers.push_back (std::stod (value));
        }

      /* Build SNR to BER Table using integer SNR values */
      for (uint16_t n = 0; n < snr2berStruct->numDataPoints; n++)
        {
          snrInt = snr2berStruct->DoubleToHashKeyInt (snrs[n]);
          /* Add datapoint to hash map */
          ret = snr2berStruct->bitErrorRateTable.insert (std::pair<int, double> (snrInt, bers[n]));
          NS_ASSERT_MSG (ret.second, "element with SNR hash of " << snrInt <<
                                     " already exists in bit error table hash map with value of " << ret.first->second);
        }

      /* Determine SNR Offset from 0 */
      snr2berStruct->DetermineSnrOffset ();

      m_snr2berList[idx] = snr2berStruct;
    }

  /* Close the file */
  file.close ();

  m_errorRateTablesLoaded = true;
}

} // namespace ns3
