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

typedef std::complex<float> Complex;                          //!< Typedef for a complex number.
typedef std::vector<Complex> WeightsVector;                   //!< Typedef for an antenna weights vector.
typedef WeightsVector::iterator WeightsVectorI;               //!< Typedef for an iterator for AWV.
typedef WeightsVector::const_iterator WeightsVectorCI;        //!< Typedef for a constant iterator for AWV.
typedef Complex** ArrayPattern;                               //!< Typedef for an phased antenna array pattern.
typedef Directivity** DirectivityMatrix;                      //!< Typedef for phased antenna directivity matrix.
typedef Complex*** SteeringVector;                            //!< Typedef for phased antenna steering vector.
typedef std::pair<uint16_t, uint16_t> PatternAngles;          //!< Tyepdef for angles (Azimuth and Elevation) in degrees.
typedef std::map<PatternAngles, Complex> ArrayPatternMap;     //!< Tyepdef for mapping between angles and array pattern value.
typedef ArrayPatternMap::iterator ArrayPatternMapI;           //!< Typedef for array pattern map iterator.
typedef ArrayPatternMap::const_iterator ArrayPatternMapCI;    //!< Typedef for array pattern map constant iterator.

struct ParametricPatternConfig;

/**
 * Calculate the normalization factor associated with the antennas weights vector.
 * \param weightsVector The antennas weights vector to be normalized.
 * \return The normalization factor assoicated with the antennas weights vector.
 */
float CalculateNormalizationFactor (WeightsVector &weightsVector);

/**
 * Parametric phased antenna array configuration.
 */
struct ParametricAntennaConfig : public PhasedAntennaArrayConfig {
public:
  /**
   * Calculate phased antenna array complex pattern.
   * \param weightsVector The complex weights of the antenna elements.
   * \param arrayPattern Pointer to the complex matrix of antenna array pattern.
   */
  void CalculateArrayPattern (WeightsVector weights, ArrayPattern &arrayPattern);
  /**
   * Get the quasi-omni antenna array pattern associated with this array.
   * \param azimuthAngle
   * \param elevationAngle
   * \return
   */
  Complex GetQuasiOmniArrayPatternValue (uint16_t azimuthAngle, uint16_t elevationAngle) const;
  /**
   * Get a pointer to the quasi-omni pattern associated with this array.
   * \return A pointer to the quasi-omni pattern.
   */
  Ptr<ParametricPatternConfig> GetQuasiOmniConfig (void) const;
  /**
   * Set the number of bits for quanitizing phase values.
   * \param num The number of bits for quanitizing phase values
   */
  void SetPhaseQuantizationBits (uint8_t num);
  /**
   * Get the number of bits for quanitizing phase values.
   * \return The number of bits for quanitizing phase values.
   */
  uint8_t GetPhaseQuantizationBits (void) const;
  /**
   * Copy the content of the source antenna array.
   * \param srcAntennaConfig Pointer to the source antenna array config.
   */
  void CopyAntennaArray (Ptr<ParametricAntennaConfig> srcAntennaConfig);

public:
  uint16_t numElements;                           //!<The number of the antenna elements in the phased antenna array.
  SteeringVector steeringVector;                  //!< Steering matrix of MxNxL where L is the number of antenna elements
                                                  // and M and N represent the azimuth and elevation angles of the incoming plane wave.

  DirectivityMatrix singleElementDirectivity;     //!< The directivity of a single antenna element in linear scale.
  uint8_t amplitudeQuantizationBits;              //!< Number of bits for quanitizing gain (amplitude) value.

private:
  uint8_t m_phaseQuantizationBits;                //!< Number of bits for quanitizing phase values.
  double m_phaseQuantizationStepSize;             //<! phase quantization step size.

};

/**
 * Interface for generating radiation pattern using parametric method.
 */
struct ParametricPatternConfig : virtual public PatternConfig {
public:
  /**
   * Set the weights that define the array pattern of the phased antenna array.
   * \param weights Vector of complex weights values.
   */
  void SetWeights (WeightsVector weights);
  /**
   * Get the weights that define the array pattern of the phased antenna array.
   * \return Vector of complex weights values.
   */
  WeightsVector GetWeights (void) const;
  /**
   * Get the normalization value based on the current antenna weights vector.
   * \return The normalization value based on the current antenna weights vector.
   */
  float GetNormalizationFactor (void) const;
  /**
   * Get the array pattern associated with this sector/awv.
   * \return The array pattern of the antenna array.
   */
  ArrayPattern GetArrayPattern (void) const;
  /**
   * Get the array pattern value associated with this sector/awv for particular angles.
   * \param azimuthAngle The azimuth angle in degrees.
   * \param elevationAngle The azimuth angle in degrees.
   * \return The array pattern of the antenna array for particular angles.
   */
  Complex GetArrayPattern (uint16_t azimuthAngle, uint16_t elevationAngle);
  /**
   * Calculate the complex array pattern value for particular angles.
   * \param azimuthAngle The azimuth angle in degrees.
   * \param elevationAngle The azimuth angle in degrees.
   */
  void CalculateArrayPattern (Ptr<ParametricAntennaConfig> antennaConfig, uint16_t azimuthAngle, uint16_t elevationAngle);

protected:
  friend class CodebookParametric;
  friend class ParametricAntennaConfig;

  ArrayPattern arrayPattern;                    //<! The complex phased antenna array pattern after applying the weights vector.
  ArrayPatternMap arrayPatternMap;              //<! The complex values of the phased antenna array pattern.

private:
  WeightsVector m_weights;                      //!< Weights that define the directivity of the phased antenna array.
  float m_normalizationFactor;                  //!< Normalization factor based on the current weights.

};

/**
 * Parametric AWV Configuration.
 */
struct Parametric_AWV_Config : virtual public AWV_Config, virtual public ParametricPatternConfig {
};

/**
 * Parametric Sector configuration.
 */
struct ParametricSectorConfig : virtual public SectorConfig, virtual public ParametricPatternConfig {
};

/**
 * \brief Codebook for Sectors generated using antenna Array Factor (AF). The user describes the steering vector
 * of the antenna. In addition, the user defines list of antenna weight to describe shapes.
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

  void CreateMimoCodebook (std::string filename);
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
   * Get transmit antenna gain in dBi.
   * \param azimuth The azimuth angle towards the intedned receiver.
   * \param elevation The elevation angle towards the intedned receiver.
   * \return Transmit antenna gain in dBi based on the steering angle.
   */
  double GetTxGainDbi (double azimuth, double elevation);
  /**
   * Get receive antenna gain in dBi.
   * \param azimuth The azimuth angle towards the intedned receiver.
   * \param elevation The elevation angle towards the intedned receiver.
   * \return Receive antenna gain in dBi based on the steering angle.
   */
  double GetRxGainDbi (double azimuth, double elevation);
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
   * \printAWVs To indicate whether to print the directivity of the custom AWVs.
   */
  void PrintCodebookContent (bool printAWVs);
  /**
   * Print directivity of the custom AWVs for specfic sector within a specific antenna array.
   * \param antennaID The ID of the antenna array.
   * \param sectorID The ID of the sector to add custom AWV to.
   */
  void PrintAWVs (AntennaID antennaID, SectorID sectorID);
  /**
   * Append a custom AWV for the Beam Refinement and Beam Tracking (BT) phases.
   * \param antennaID The ID of the antenna array.
   * \param sectorID The ID of the sector to add custom AWV to.
   * \param weightsVector The custom antenna weight vectors.
   */
  void AppendBeamRefinementAwv (AntennaID antennaID, SectorID sectorID, WeightsVector &weightsVector);
  /**
   * Append a custom AWV for the Beam Refinement and Beam Tracking (BT) phases.
   * \param antennaID The ID of the antenna array.
   * \param sectorID The ID of the sector to ad custom AWV to.
   * \param steeringAngleAzimuth The steering angle of the custom AWV in the azimuth plane (defined between -180 and +180).
   * \param steeringAngleElevation The steering angle of the custom AWV in the elevation plane (defined between -90 and 90).
   */
  void AppendBeamRefinementAwv (AntennaID antennaID, SectorID sectorID,
                                double steeringAngleAzimuth, double steeringAngleElevation);
  /**
   * For testing purposes - append custom AWVs for all sectors in antenna - used in Group Beamforming.
   * Call after loading the codebook in order to populate all sectors with AWVs.
   * The AWVs for each sector have the same steering angles and will not give meaningful results for determining the optimal
   * antenna configuration - function is meant to purely test the code in terms of operation with AWVs.
   */
  void AppendAwvsToAllSectors (void);
  /**
   * Append custom AWVs for all sectors in all antennas - used in SU-MIMO BFT. Call after loading
   * the codebook in order to populate all sectors with AWVs.
   * The function asssumes a codebook with 54 sectors (18 different azimuth angles - granularity of 20 degrees and 3 elevation angles - granularity of 45 degrees)
   * It adds 5 AWVs to each sector which increases the granularity in the azimuth plane to 5 degrees.
   * Needs to be modified to work with a different codebook since it requires knowledge about the azimuth and elevation steering angles of the sectors in the codebook.
   */
  void AppendAwvsForSuMimoBFT (void);
  /**
   * Append custom AWVs for all sectors in all antennas - used in SU-MIMO BFT. Call after loading
   * the codebook in order to populate all sectors with AWVs.
   * The function asssumes a codebook with 27 sectors (9 different azimuth angles - granularity of 20 degrees and 3 elevation angles - granularity of 45 degrees)
   * It adds 5 AWVs to each sector which increases the granularity in the azimuth plane to 5 degrees.
   * Needs to be modified to work with a different codebook since it requires knowledge about the azimuth and elevation steering angles of the sectors in the codebook.
   */
  void AppendAwvsForSuMimoBFT_27 (void);
  /**
   * Get the number of antenna elements in specific antenna array.
   * \param antennaID The ID of the antenna array.
   * \return The number of the antenna elements in the antenna array.
   */
  uint16_t GetNumberOfElements (AntennaID antennaID) const;
  /**
   * Copy the content of an existing codebook to the current codebook.
   * \param codebook A pointer to the codebook that we want to copy its contents.
   */
  virtual void CopyCodebook (const Ptr<Codebook> codebook);

protected:
  friend class QdPropagationEngine;
  /**
   * Get the complex value of the specified antenna array pattern.
   * \param config Pointer to the pattern configuration.
   * \param azimuthAngle The azimuth angle in degrees.
   * \param elevationAngle The elevation angle in degrees.
   * \return The complex array pattern of the given pattern.
   */
  Complex GetAntennaArrayPattern (Ptr<PatternConfig> config, uint16_t azimuthAngle, uint16_t elevationAngle);
  /**
   * Get current active transmit antenna array pattern.
   * \param azimuthAngle The azimuth angle in degrees.
   * \param elevationAngle The azimuth angle in degrees.
   * \return The array pattern of the transmit phased antenna array.
   */
  Complex GetTxAntennaArrayPattern (uint16_t azimuthAngle, uint16_t elevationAngle);
  /**
   * Get current active receive antenna array pattern.
   * \param azimuthAngle The azimuth angle in degrees.
   * \param elevationAngle The azimuth angle in degrees.
   * \return The array pattern of the receive phased antenna array.
   */
  Complex GetRxAntennaArrayPattern (uint16_t azimuthAngle, uint16_t elevationAngle);
  /**
   * Calculate array pattern for a particular pair of angles for all the sectors and their AWVs in the
   * specified antenna array ID.
   * \param antennaID The ID of the antenna array.
   * \param azimuthAngle The azimuth angle in degrees after rounding it.
   * \param elevationAngle The elevation angle in degrees after rounding it.
   */
  void CalculateArrayPatterns (AntennaID antennaID, uint16_t azimuthAngle, uint16_t elevationAngle);
  /**
   * \return True if the array patterns are precalculated, otherwise false.
   */
  bool ArrayPatternsPrecalculated (void) const;

private:
  void DoDispose (void);
  void DoInitialize (void);

  void DisposeAntennaConfig (Ptr<ParametricAntennaConfig> antennaConfig);
  /**
   * Print antenna weights vector or beamforming vector.
   * \param weightsVector The list of antenna weights to be printed.
   */
  void PrintWeights (WeightsVector weightsVector);
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
  WeightsVector ReadAntennaWeightsVector (std::ifstream &file, uint16_t elements);
  /**
   * Normalize antennas weights vector.
   * \param weightsVector The antennas weights vector to be normalized.
   */
  inline void NormalizeWeights (WeightsVector &weightsVector);

private:
  bool m_precalculatedPatterns;   //!< Flag to indicate whether we have precalculated the array pattern.
  bool m_cloned;                  //!< Flag to indicate if we have cloned this codebook.
  bool m_mimoCodebook;            //!< Flag to indicate if we have MIMO codebook or typical legacy codebook.

};

} // namespace ns3

#endif /* CODEBOOK_PARAMETRIC_H */
