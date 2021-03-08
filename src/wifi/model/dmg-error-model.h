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
  /**
   * Determines offset from zero of SNR datapoints in lookup table.
   */
  void DetermineSnrOffset (void);

  /**
   * Returns the bit error rate (BER) for the given signal to noise ratio (SNR) input
   * from the lookup table. If the input SNR lie in between SNR datapoints, linear
   * interpolation is performed to calculate the BER value to be returned.
   * If the input SNR falls below/above the minimum/maximum SNR of the datapoints,
   * the BER corresponding to the minimum/maximum SNR datapoint is returned.
   * \param snr the signal to noise ratio (in dB) corresponding to the BER to
   * look up
   * \return the retrieved frame sync error rate corresponding to the input SNR
   */
  double GetBitErrorRate (double snr);

  /**
   * A struct containing the lower and upper SNR datapoint bounds used in linear
   * interpolation.
   */
  struct InterpolParams
  {
    double snrLoBound;
    double snrHiBound;
  };

  /**
   * Rounds input value to the nearest data midpoint with given data spacing
   * and offset from zero. For example, an SNR dataset with SNR datapoints at
   * {-5, -2, 1, 4, 7, 10,...} has offset=1 and dataSpacing=3. The midpoints
   * between these numbers would be {-3.5, -0.5, 2.5, 5.5, 8.5,...}. So for
   * value=3.5 for this dataset, this method will return 2.5.
   * \param value the input value to round
   * \return the nearest data midpoint value
   */
  double RoundToNearestDataMidpoint (double value);
  /**
   * Finds upper and lower SNR datapoint bounds around input SNR. If input SNR
   * is at upper or lower bound, both bounds are set equal to input SNR.
   * \param snr the input SNR (in dB).
   * \return the struct containing the upper and lower SNR datapoint bounds
   */
  InterpolParams FindDatapointBounds (double snr);
  /**
   * Rounds input double value to the nearest integer.
   * \param val the double value to round.
   * \return the input value rounded to the nearest integer.
   */
  int RoundDoubleToInt (double val);
  /**
   * Converts double value to integer key used for lookup table indexing.
   * \param val the double value to convert to integer key.
   * \return the integer key corresponding to the input double value.
   */
  int DoubleToHashKeyInt (double val);
  /**
   * Retrieves BER value from lookup table based on input SNR and its SNR
   * datapoint bounds, performing linear interpolation.
   * \param xd the SNR (in dB) corresponding to the SER to look up
   * \param x1d the lower SNR datapoint bound (in dB) of xd.
   * \param x2d the upper SNR datapoint bound (in dB) of xd.
   * \return the retrieved SER
   */
  double InterpolateAndRetrieveData (double xd, double x1d, double x2d);

  uint16_t numDataPoints;                    //!< The number of SNR to BER datapoints.
  double snrMin;                             //!< Minimum (in dB) SNR datapoint value.
  double snrMax;                             //!< Maximum (in dB) SNR datapoint value.
  double berMin;                             //!< BER datapoint value corresponding to the minimum SNR value.
  double berMax;                             //!< BER datapoint value corresponding to the maximum SNR value.
  std::map<int, double> bitErrorRateTable;   //!< Stores frame synchronization error rate values indexed by integer SNR key.
  double snrOffset;                          //!< Offset from zero (in dB) of SNR datapoints.
  uint8_t numSnrDecPlaces;                   //!< Number of decimal places in SNR datapoints.
  double snrSpacing;                         //!< Spacing (in dB) between SNR datapoints.

};

typedef uint8_t MCS_IDX;                                        //!< Typedef for MCS index.
typedef std::map<MCS_IDX, Ptr<SNR2BER_STRUCT> > SNR2BER_LIST;   //!< Typedef for mapping MCS to its SNR to BER tables.
typedef SNR2BER_LIST::iterator SNR2BER_LIST_I;                  //!< Typedef for iterator over SNR to BER tables.

/**
 * \ingroup wifi
 *
 * \brief Provides lookup for SNR to bit error rate mapping.
 */
class DmgErrorModel : public ErrorRateModel
{
public:
  DmgErrorModel ();           //!< Default constructor
  virtual ~DmgErrorModel ();  //!< Destructor

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);
  /**
   * A pure virtual method that must be implemented in the subclass.
   * This method returns the probability that the given 'chunk' of the
   * packet will be successfully received by the PHY.
   *
   * A chunk can be viewed as a part of a packet with equal SNR.
   * The probability of successfully receiving the chunk depends on
   * the mode, the SNR, and the size of the chunk.
   *
   * Note that both a WifiMode and a WifiTxVector (which contains a WifiMode)
   * are passed into this method.  The WifiTxVector may be from a signal that
   * contains multiple modes (e.g. PLCP header sent differently from PLCP
   * payload).  Consequently, the mode parameter is what the method uses
   * to calculate the chunk error rate, and the txVector is used for
   * other information as needed.
   *
   * \param mode the Wi-Fi mode applicable to this chunk.
   * \param txVector TXVECTOR of the overall transmission.
   * \param snr the SNR of the chunk in linear.
   * \param nbits the number of bits in this chunk.
   * \return probability of successfully receiving the chunk.
   */
  virtual double GetChunkSuccessRate (WifiMode mode, WifiTxVector txVector, double snr, uint64_t nbits) const;

private:
  /**
   * Set the name of the file containing the list of error rate tables.
   * \param fileName The name of the file containing the list of error rate tables.
   */
  void SetErrorRateTablesFileName (std::string fileName);
  /**
   * Load SNR to BER Tables.
   */
  void LoadErrorRateTables (void);

private:
  std::string m_fileName;           //!< The name of the file describing the transmit and receive patterns.
  bool m_errorRateTablesLoaded;     //!< Indicates if frames BER tables has been loaded.
  uint8_t m_numSnrDecPlaces;        //!< Number of decimal places in SNR datapoints.
  double m_snrSpacing;              //!< Spacing (in dB) between SNR datapoints.
  uint8_t m_numMCSs;                //!< The first line determines the number of MCSs within the lookup table.
  SNR2BER_LIST m_snr2berList;       //!< List of SNR to BER Tables.

};

} // namespace ns3

#endif /* DMG_ERROR_MODEL_H */
