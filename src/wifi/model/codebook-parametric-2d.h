/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */

#ifndef CODEBOOK_PARAMETRIC_H
#define CODEBOOK_PARAMETRIC_H

#include "ns3/object.h"
#include "codebook.h"
#include <complex>
#include <iostream>

namespace ns3 {

typedef std::complex<double> Complex;
typedef std::vector<Complex> WeightsVector;
typedef WeightsVector::const_iterator WeightsVectorCI;
typedef Complex* ArrayFactor;
typedef Directivity* DirectivityTable;

struct ParametricPatternConfig {
public:
  ArrayFactor GetArrayFactor (void) const;

public:
  WeightsVector sectorWeights;

protected:
  friend class CodebookParametric;
  ArrayFactor sectorArrayFactor;
  DirectivityTable sectorDirectivity;

};

ArrayFactor
ParametricPatternConfig::GetArrayFactor (void) const
{
  return sectorArrayFactor;
}

struct Parametric_AWV_Config : public AWV_Config, ParametricPatternConfig {
};

typedef std::vector<Ptr<Parametric_AWV_Config> > Parametric_AWV_LIST;
typedef Parametric_AWV_LIST::iterator Parametric_AWV_LIST_CI;
typedef Parametric_AWV_LIST::const_iterator Parametric_AWV_LIST_I;

struct ParametricSectorConfig : public SectorConfig, ParametricPatternConfig {
  Parametric_AWV_LIST awvList;

public:
  virtual uint8_t GetTotalNumberOfAWVs (void) const;

};

struct ParametricAntennaConfig : public PhasedAntennaArrayConfig {
  uint16_t elements;
  Complex **steeringVector;

  WeightsVector quasiOmniWeights;
  uint8_t amplitudeQuantizationBits;
  uint8_t phaseQuantizationBits;

  double CalculateDirectivity (double angle, WeightsVector &weightsVector);
  double CalculateDirectivityForDirection (double angle);

private:
  friend class CodebookParametric;
  ArrayFactor quasiOmniArrayFactor;
  DirectivityTable quasiOmniDirectivity;
  double phaseQuantizationStepSize;

};

void
CalculateDirectivity (WeightsVector *weights, Complex **steeringVector, ArrayFactor &arrayFactor, DirectivityTable &directivity)
{
  arrayFactor = new Complex[AZIMUTH_CARDINALITY];
  directivity = new double[AZIMUTH_CARDINALITY];
  for (uint16_t k = 0; k < AZIMUTH_CARDINALITY; k++)
    {
      Complex value = 0;
      uint16_t j = 0;
      for (WeightsVectorCI it = weights->begin (); it != weights->end (); it++, j++)
        {
          value += (*it) * steeringVector[j][k];
        }
      arrayFactor[k] = value;
      directivity[k] = abs (value);
    }
}

class CodebookParametric : public Codebook
{
public:
  static TypeId GetTypeId (void);

  CodebookParametric (void);
  virtual ~CodebookParametric (void);

  void LoadCodebook (std::string filename);
  double GetTxGainDbi (double angle);
  double GetRxGainDbi (double angle);
  uint8_t GetNumberSectorsPerAntenna (AntennaID antennaID) const;
  void UpdateSectorWeights (AntennaID antennaID, SectorID sectorID, WeightsVector &weightsVector);
  void UpdateQuasiOmniWeights (AntennaID antennaID, WeightsVector &weightsVector);
  void AppendSector (AntennaID antennaID, SectorID sectorID, SectorUsage sectorUsage,
                     SectorType sectorType, WeightsVector &weightsVector);
  void PrintCodebookContent (void);
  void ChangeAntennaOrientation (AntennaID antennaID, double orientation);
  void AppendBeamRefinementAwv (AntennaID antennaID, SectorID sectorID, WeightsVector &weightsVector);
  virtual void InitiateBRP (AntennaID antennaID, SectorID sectorID, BeamRefinementType type);
  virtual bool GetNextAWV (void);
  virtual void UseLastTxSector (void);

  uint16_t GetNumberOfElements (AntennaID antennaID) const;
  ArrayFactor GetAntennaArrayFactor (SectorID sectorID) const;
  ArrayFactor GetAntennaArrayFactor (void) const;

private:
  void PrintDirectivity (DirectivityTable directivity) const;
  double GetGainDbi (double angle, DirectivityTable directivity) const;
  void SetCodebookFileName (std::string fileName);
  WeightsVector ReadAntennaWeightsVector (std::ifstream &file, double elements);

private:
  Parametric_AWV_LIST *m_currentAwvList;
  Parametric_AWV_LIST_CI m_currentAwvIter;

};

} // namespace ns3

#endif /* CODEBOOK_PARAMETRIC_H */
