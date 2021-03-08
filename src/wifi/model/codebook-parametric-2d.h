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

typedef std::complex<double> Complex;                   //<! Typedef for a complex number.
typedef std::vector<Complex> WeightsVector;             //<! Typedef for an antenna weights vector.
typedef WeightsVector::const_iterator WeightsVectorCI;
typedef Complex* ArrayFactor;                           //<! Typedef for array factor (Future use).
typedef Directivity* DirectivityTable;                  //<! Typedef for antenna directivity vector in the azimuth plane.

struct ParametricPatternConfig { //!< Interface for generating radiation pattern using parametric method.
public:
  ArrayFactor GetArrayFactor (void) const;

public:
  WeightsVector sectorWeights;                //!< Weights that define the directivity of the pattern.

protected:
  friend class CodebookParametric;
  ArrayFactor sectorArrayFactor;              //<! Sector array factcor after applying the weights vector.
  DirectivityTable sectorDirectivity;         //<! Sector pattern after applying the weights vector.

};

ArrayFactor
ParametricPatternConfig::GetArrayFactor (void) const
{
  return sectorArrayFactor;
}

struct Parametric_AWV_Config : public AWV_Config, ParametricPatternConfig {//!< Parametric AWV Configuration.
};

typedef std::vector<Ptr<Parametric_AWV_Config> > Parametric_AWV_LIST;   //<! Typedef for a list of Parametric AWVs.
typedef Parametric_AWV_LIST::iterator Parametric_AWV_LIST_CI;           //<! Typedef for a parametric AWV constant iterator.
typedef Parametric_AWV_LIST::const_iterator Parametric_AWV_LIST_I;      //<! Typedef for a parametric AWV iterator.

//!< Parametric Sector configuration.
struct ParametricSectorConfig : public SectorConfig, ParametricPatternConfig {
  Parametric_AWV_LIST awvList;                //!< List of AWVs associated with this sector.

public:
  /**
   * Get the total numbe of AWVs covered by this sector.
   * \return The total numbe of AWVs covered by this sector.
   */
  virtual uint8_t GetTotalNumberOfAWVs (void) const;

};

struct ParametricAntennaConfig : public PhasedAntennaArrayConfig {
  uint16_t elements;                          //!< Number of the antenna elements in the antenna array.
  Complex **steeringVector;                   //!< Steering matrix of NxM where N is the number of antenna elements
                                              // and M is number of waves. This matrix describes phase delays among
                                              // antenna elements for each incoming plane wave. Currently, we desribe
                                              // each incoming wave using its azimuth angle only.

  WeightsVector quasiOmniWeights;             //!< Weights that define the directivity of the Quasi-omni pattern.
  uint8_t amplitudeQuantizationBits;          //!< Number of bits for quanitizing gain (amplitude) value.
  uint8_t phaseQuantizationBits;              //!< Number of bits for quanitizing phase value.

  /**
   * Calculate directivity towards certain direction for a given antenna weights vector.
   * \param angle The angle in degrees for which we want to calculate the directivity towards.
   * \param weightsVector The antenna weights vector to apply to the steering vector of the antenna array.
   * \return the antenna array directivity in linear scale.
   */
  double CalculateDirectivity (double angle, WeightsVector &weightsVector);
  /**
   * Calculate directivity towards a certain direction using the steering vector.
   * \param angle The angle for which we want to calculate the directivity towards.
   * \return
   */
  double CalculateDirectivityForDirection (double angle);

private:
  friend class CodebookParametric;
  ArrayFactor quasiOmniArrayFactor;           //<! Quasi-omni pattern array factor after applying the weights.
  DirectivityTable quasiOmniDirectivity;      //<! Quasi-omni pattern directivity after applying the weights.
  double phaseQuantizationStepSize;           //<! phase quantization step size.

};

/**
 * Calculate antenna array factor and directivity for specific weights.
 * \param weightsVector of antenna weights.
 * \param steeringVector The steering vector of the corresponding phased antenna array.
 * \return Vector of antenna directivity in linear scale.
 */
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

/**
 * \brief Codebook for Sectors generated using Array Factor (AF). The user describes the steering vector of the antenna.
 * In addition, the user defines list of antenna weight to describe shapes.
 */
class CodebookParametric : public Codebook
{
public:
  static TypeId GetTypeId (void);

  CodebookParametric (void);
  virtual ~CodebookParametric (void);
  /**
   * Load code book from a text file.
   */
  void LoadCodebook (std::string filename);
  /**
   * Get transmit antenna gain dBi.
   * \param angle The angle towards the intended receiver.
   * \return Transmit antenna gain in dBi based on the steering angle.
   */
  double GetTxGainDbi (double angle);
  /**
   * Get transmit antenna gain in dBi.
   * \param angle The angle towards the intended receiver.
   * \return Receive antenna gain in dBi based on the steering angle.
   */
  double GetRxGainDbi (double angle);
  /**
   * Get the total number of sectors for a specific phased antenna array.
   * \param antennaID The ID of the phased antenna array.
   * \return The number of sectors for the specified phased antenna array.
   */
  uint8_t GetNumberSectorsPerAntenna (AntennaID antennaID) const;
  /**
   * Update sector antenna weight vector.
   * \param antennaID The ID of the antenna array.
   * \param sectorID The ID of the sector to update its pattern.
   * \param weightsVector Vector of the antenna weights (Amplitude and phase excitation).
   */
  void UpdateSectorWeights (AntennaID antennaID, SectorID sectorID, WeightsVector &weightsVector);
  /**
   * Update antenna weights corresponding to the quasi-omni pattern.
   * \param antennaID The ID of the antenna array.
   * \param weightsVector Vector of the antenna weights.
   */
  void UpdateQuasiOmniWeights (AntennaID antennaID, WeightsVector &weightsVector);
  /**
   * Append a new sector to for a specific antenna array or update an existing
   * sector in the codebook. If the sector is new then the number of sectors
   * should not exceed 64 sectors per antenna array and 128 across all the antennas.
   * \param antennaID The of the phased antenna array.
   * \param sectorID The ID of the sector to ID.
   * \param sectorUsage The usage of the sector.
   * \param sectorType The type of the sector.
   * \param weightsVector The antenna weights vector of the sector.
   */
  void AppendSector (AntennaID antennaID, SectorID sectorID, SectorUsage sectorUsage,
                     SectorType sectorType, WeightsVector &weightsVector);
  /**
   * Print Codebook Content to the standard output.
   */
  void PrintCodebookContent (void);
  /**
   * Change specific antenna orientation.
   * \param antennaID The ID of the antenna.
   * \param orientation The new orientation of the antenna in degrees.
   */
  void ChangeAntennaOrientation (AntennaID antennaID, double orientation);
  /**
   * Append a custom AWV for the Beam Refinement and Beam Tracking (BT) phases.
   * \param antennaID The ID of the antenna array.
   * \param sectorID The ID of the sector to ad custom AWV to.
   * \param weightsVector The custom antenna weight vectors.
   */
  void AppendBeamRefinementAwv (AntennaID antennaID, SectorID sectorID, WeightsVector &weightsVector);
  /**
   * Initiate Beam Refinement Protocol (BRP) after completing SLS phase.
   * \param antennaID The ID of the phased antenna array.
   * \param sectorID The ID of the sector to refine wthin the antenna array.
   * \param type Beam Refinement Type (Transmit/Receive).
   */
  virtual void InitiateBRP (AntennaID antennaID, SectorID sectorID, BeamRefinementType type);
  /**
   * Get the next AWV in the AWV list of the selected transmit/receive sector.
   * \return True if we've tried all the AWVs, otherwise False.
   */
  virtual bool GetNextAWV (void);
  /**
   * Set the ID of the active transmit sector.
   * \param antennaID The ID of the phased antenna array.
   * \param sectorID The ID of the new sector wthin the antenna.
   */
  virtual void UseLastTxSector (void);

  uint16_t GetNumberOfElements (AntennaID antennaID) const;
  ArrayFactor GetAntennaArrayFactor (SectorID sectorID) const;
  /**
   * Get Antenna Array Factor when applying quasi-omni weights.
   * \return The array factor of the phased antenna array using Quasi-omni weights.
   */
  ArrayFactor GetAntennaArrayFactor (void) const;

private:
  /**
   * Print antenna array directivity in the azimuth plane for a specific sector.
   * \param directivity The pre-calculated directivity for a specific sector.
   */
  void PrintDirectivity (DirectivityTable directivity) const;
  /**
   * Get transmission gain in dBi based on the selected antenna and sector.
   * \param angle The azimuth angle towards the peer device.
   * \param directivity The directivity of the sector after multiplying the antenna weights with the steering vector.
   * \return Transmit/Receive antenna gain in dBi based on the azimuth angle and the selected sector.
   */
  double GetGainDbi (double angle, DirectivityTable directivity) const;
  /**
   * Set Codebook File Name.
   * \param fileName The name of the codebook file to load.
   */
  void SetCodebookFileName (std::string fileName);
  /**
   * Read Antenna Weights Vector for a predefined antenna pattern.
   * \param file The file from where to read the values.
   * \param elements The number of antenna elements in the antenna array.
   * \return A weight vector that includes the excitation (Phase and amplitude) for each antenna element.
   */
  WeightsVector ReadAntennaWeightsVector (std::ifstream &file, double elements);

private:
  Parametric_AWV_LIST *m_currentAwvList;                //!< The current AWV List to try from during BRP phase.
  Parametric_AWV_LIST_CI m_currentAwvIter;              //!< The current AWV
//  Ptr<ParametricPatternConfig> m_curentAwv;             //!< Pointer to the current custom AWV.

};

} // namespace ns3

#endif /* CODEBOOK_PARAMETRIC_H */
