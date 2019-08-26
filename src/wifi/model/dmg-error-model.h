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

#ifndef DMG_ERROR_MODEL_H
#define DMG_ERROR_MODEL_H

#include "error-rate-model.h"
#include "wifi-mode.h"
#include <map>

namespace ns3 {

struct SNR2BER_STRUCT : public SimpleRefCount<SNR2BER_STRUCT> {
  void DetermineSnrOffset (void);

  double GetBitErrorRate (double snr);

  struct InterpolParams
  {
    double snrLoBound;
    double snrHiBound;
  };

  double RoundToNearestDataMidpoint (double value);
  InterpolParams FindDatapointBounds (double snr);
  int RoundDoubleToInt (double val);
  int DoubleToHashKeyInt (double val);
  double InterpolateAndRetrieveData (double xd, double x1d, double x2d);

  uint16_t numDataPoints;
  double snrMin;
  double snrMax;
  double berMin;
  double berMax;
  std::map<int, double> bitErrorRateTable;
  double snrOffset;
  uint8_t numSnrDecPlaces;
  double snrSpacing;

};

typedef uint8_t MCS_IDX;
typedef std::map<MCS_IDX, Ptr<SNR2BER_STRUCT> > SNR2BER_LIST;
typedef SNR2BER_LIST::iterator SNR2BER_LIST_I;

class DmgErrorModel : public ErrorRateModel
{
public:
  DmgErrorModel ();
  virtual ~DmgErrorModel ();

  static TypeId GetTypeId (void);
  virtual double GetChunkSuccessRate (WifiMode mode, WifiTxVector txVector, double snr, uint64_t nbits) const;

private:
  void SetErrorRateTablesFileName (std::string fileName);
  void LoadErrorRateTables (void);

private:
  std::string m_fileName;
  bool m_errorRateTablesLoaded;
  uint8_t m_numSnrDecPlaces;
  double m_snrSpacing;
  uint8_t m_numMCSs;
  SNR2BER_LIST m_snr2berList;

};

} // namespace ns3

#endif /* DMG_ERROR_MODEL_H */
