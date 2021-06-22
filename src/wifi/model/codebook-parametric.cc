/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#include "ns3/log.h"
#include "ns3/string.h"
#include "codebook-parametric.h"

#include <algorithm>
#include <fstream>
#include <string>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CodebookParametric");

NS_OBJECT_ENSURE_REGISTERED (CodebookParametric);

float
CalculateNormalizationFactor (WeightsVector &weightsVector)
{
  float normlizationValue = 0;
  for (WeightsVectorI it = weightsVector.begin (); it != weightsVector.end (); it++)
    {
      normlizationValue += std::pow (std::abs (*it), 2.0);
    }
  normlizationValue = sqrt (normlizationValue);
  return normlizationValue;
}

/****** Parametric Pattern Configuration ******/

void
ParametricPatternConfig::SetWeights (WeightsVector weights)
{
  NS_LOG_FUNCTION (this);
  m_weights = weights;
  m_normalizationFactor = CalculateNormalizationFactor (weights);
}

WeightsVector
ParametricPatternConfig::GetWeights (void) const
{
  return m_weights;
}

float
ParametricPatternConfig::GetNormalizationFactor (void) const
{
  return m_normalizationFactor;
}

ArrayPattern
ParametricPatternConfig::GetArrayPattern (void) const
{
  return arrayPattern;
}

Complex
ParametricPatternConfig::GetArrayPattern (uint16_t azimuthAngle, uint16_t elevationAngle)
{
  return arrayPatternMap[std::make_pair (azimuthAngle, elevationAngle)];
}

void
ParametricPatternConfig::CalculateArrayPattern (Ptr<ParametricAntennaConfig> antennaConfig,
                                                uint16_t azimuthAngle, uint16_t elevationAngle)
{
  PatternAngles angles = std::make_pair (azimuthAngle, elevationAngle);
  ArrayPatternMapCI it = arrayPatternMap.find (angles);
  if (it == arrayPatternMap.end ())
    {
      Complex value = 0;
      uint16_t j = 0;
      for (WeightsVectorCI it = m_weights.begin (); it != m_weights.end (); it++, j++)
        {
          value += (*it) * antennaConfig->steeringVector [azimuthAngle][elevationAngle][j];
        }
      value *= antennaConfig->singleElementDirectivity[azimuthAngle][elevationAngle];
      arrayPatternMap[angles] = value;
    }
}

/****** Parametric Antenna Configuration ******/

void
ParametricAntennaConfig::CalculateArrayPattern (WeightsVector weights, ArrayPattern &arrayPattern)
{
  arrayPattern = new Complex *[AZIMUTH_CARDINALITY];
  for (uint16_t m = 0; m < AZIMUTH_CARDINALITY; m++)
    {
      arrayPattern[m] = new Complex[ELEVATION_CARDINALITY];
      for (uint16_t n = 0; n < ELEVATION_CARDINALITY; n++)
        {
          Complex value = 0;
          uint16_t j = 0;
          for (WeightsVectorCI it = weights.begin (); it != weights.end (); it++, j++)
            {
              value += (*it) * steeringVector[m][n][j];
            }
          value *= singleElementDirectivity[m][n];
          arrayPattern[m][n] = value;
        }
    }
}

Complex
ParametricAntennaConfig::GetQuasiOmniArrayPatternValue (uint16_t azimuthAngle, uint16_t elevationAngle) const
{
  return GetQuasiOmniConfig ()->arrayPatternMap[std::make_pair (azimuthAngle, elevationAngle)];
}

Ptr<ParametricPatternConfig>
ParametricAntennaConfig::GetQuasiOmniConfig (void) const
{
  return DynamicCast<ParametricPatternConfig> (m_quasiOmniConfig);
}

void
ParametricAntennaConfig::SetPhaseQuantizationBits (uint8_t num)
{
  m_phaseQuantizationBits = num;
  m_phaseQuantizationStepSize = 2*M_PI/(std::pow (2, num));
}

uint8_t
ParametricAntennaConfig::GetPhaseQuantizationBits (void) const
{
  return m_phaseQuantizationBits;
}

void
ParametricAntennaConfig::CopyAntennaArray (Ptr<ParametricAntennaConfig> srcAntennaConfig)
{
  NS_LOG_FUNCTION (this);
  azimuthOrientationDegree = srcAntennaConfig->azimuthOrientationDegree;
  elevationOrientationDegree = srcAntennaConfig->elevationOrientationDegree;
  orientation = srcAntennaConfig->orientation;
  numElements = srcAntennaConfig->numElements;
  singleElementDirectivity = srcAntennaConfig->singleElementDirectivity;
  steeringVector = srcAntennaConfig->steeringVector;
  amplitudeQuantizationBits = srcAntennaConfig->amplitudeQuantizationBits;
  m_phaseQuantizationBits =  srcAntennaConfig->m_phaseQuantizationBits;
  m_phaseQuantizationStepSize = srcAntennaConfig->m_phaseQuantizationStepSize;
}

/****** Parametric Codebook Configuration ******/

TypeId
CodebookParametric::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CodebookParametric")
    .SetGroupName ("Wifi")
    .SetParent<Codebook> ()
    .AddConstructor<CodebookParametric> ()
    .AddAttribute ("PrecalculatePatterns",
                   "Whether we precalculate all the beam patterns over the 3D space within our simulation or"
                   " we calculate only beam patterns based on the utilized angles in our Q-D traces.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&CodebookParametric::m_precalculatedPatterns),
                   MakeBooleanChecker ())
    .AddAttribute ("MimoCodebook",
                   "Special case to handle codebook of identical PAAs for MIMO communication.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&CodebookParametric::m_mimoCodebook),
                   MakeBooleanChecker ())
    .AddAttribute ("TotalAntennas",
                   "The total number of PAAs. This attribute is used together with MimoCodebook attribute.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&CodebookParametric::m_totalAntennas),
                   MakeUintegerChecker<uint8_t> (1, 8))
    .AddAttribute ("FileName",
                   "The name of the codebook file to load.",
                   StringValue (""),
                   MakeStringAccessor (&CodebookParametric::SetCodebookFileName),
                   MakeStringChecker ())
  ;
  return tid;
}

CodebookParametric::CodebookParametric ()
{
  NS_LOG_FUNCTION (this);
  m_cloned = false;
}

void
CodebookParametric::DisposeAntennaConfig (Ptr<ParametricAntennaConfig> antennaConfig)
{
  /* Iterate over all the sectors */
  for (SectorListI sectorIter = antennaConfig->sectorList.begin ();
       sectorIter != antennaConfig->sectorList.end (); sectorIter++)
    {
      Ptr<ParametricSectorConfig> sectorConfig = DynamicCast<ParametricSectorConfig> (sectorIter->second);
      for (uint16_t m = 0; m < AZIMUTH_CARDINALITY; m++)
        {
          delete[] sectorConfig->arrayPattern[m];
        }
      delete[] sectorConfig->arrayPattern;
      /* Iterate over all the custom AWVs */
      for (AWV_LIST_I awvIt = sectorConfig->awvList.begin (); awvIt != sectorConfig->awvList.end (); awvIt++)
        {
          Ptr<Parametric_AWV_Config> awvConfig = DynamicCast<Parametric_AWV_Config> (*awvIt);
          for (uint16_t m = 0; m < AZIMUTH_CARDINALITY; m++)
            {
              delete[] awvConfig->arrayPattern[m];
            }
          delete[] awvConfig->arrayPattern;
        }
    }

  for (uint16_t m = 0; m < AZIMUTH_CARDINALITY; m++)
    {
      delete[] antennaConfig->singleElementDirectivity[m];
      delete[] antennaConfig->GetQuasiOmniConfig ()->arrayPattern[m];
      for (uint16_t n = 0; n < ELEVATION_CARDINALITY; n++)
        {
          delete[] antennaConfig->steeringVector[m][n];
        }
      delete[] antennaConfig->steeringVector[m];
    }

  // Free the array of pointers
  delete[] antennaConfig->singleElementDirectivity;
  delete[] antennaConfig->GetQuasiOmniConfig ()->arrayPattern;
  delete[] antennaConfig->steeringVector;
}

void
CodebookParametric::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  if (!m_cloned)
    {
      if (m_mimoCodebook)
        {
          /* Iterate over all the antenna arrays */
          DisposeAntennaConfig (StaticCast<ParametricAntennaConfig> (m_antennaArrayList.begin ()->second));
        }
      else
        {
          /* Iterate over all the antenna arrays */
          for (AntennaArrayListI iter = m_antennaArrayList.begin (); iter != m_antennaArrayList.end (); iter++)
            {
              DisposeAntennaConfig (StaticCast<ParametricAntennaConfig> (iter->second));
            }
        }
    }
  Codebook::DoDispose ();
}

void
CodebookParametric::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  Codebook::DoInitialize ();
}

CodebookParametric::~CodebookParametric ()
{
  NS_LOG_FUNCTION (this);
}

void
CodebookParametric::SetCodebookFileName (std::string fileName)
{
  NS_LOG_FUNCTION (this << fileName);
  if (fileName != "")
    {
      m_fileName = fileName;
      if (m_mimoCodebook)
        {
          CreateMimoCodebook (m_fileName);
        }
      else
        {
          LoadCodebook (m_fileName);
        }
    }
}

inline void
CodebookParametric::NormalizeWeights (WeightsVector &weightsVector)
{
  NS_LOG_FUNCTION (this);
  double normlizationValue = 0;
  /* Calculate normalization value */
  for (WeightsVectorI it = weightsVector.begin (); it != weightsVector.end (); it++)
    {
      normlizationValue += std::pow (std::abs (*it), 2.0);
    }
  normlizationValue = sqrt (normlizationValue);
  /* Divide by the normalziation vector */
  for (WeightsVectorI it = weightsVector.begin (); it != weightsVector.end (); it++)
    {
      (*it) /= normlizationValue;
    }
}

WeightsVector
CodebookParametric::ReadAntennaWeightsVector (std::ifstream &file, uint16_t elements)
{
  WeightsVector weights;
  std::string amp, phase, line;
  std::getline (file, line);
  std::istringstream split(line);
  for (uint16_t i = 0; i < elements; i++)
    {
      getline (split, amp, ',');
      getline (split, phase, ',');
      weights.push_back (std::polar (std::stof (amp), std::stof (phase)));
    }
  return weights;
}

void
CodebookParametric::LoadCodebook (std::string filename)
{
  NS_LOG_FUNCTION (this << "Loading Parametric Codebook file " << filename);
  std::ifstream file;
  file.open (filename.c_str (), std::ifstream::in);
  NS_ASSERT_MSG (file.good (), " Codebook file not found in " + filename);
  std::string line;

  RFChainID rfChainID;
  uint8_t totalRFChains;  /* The total number of RF Chains. */
  uint8_t nSectors;       /* The total number of sectors within a phased antenna array */
  AntennaID antennaID;
  SectorID sectorID;
  std::string amp, phaseDelay, directivity;

  /** Create RF Chain List **/

  /* The first line determines the number of RF Chains within the device */
  Ptr<RFChain> rfChainConfig;
  std::getline (file, line);
  totalRFChains = std::stod (line);
  for (RFChainID rfChainID = 1; rfChainID <= totalRFChains; rfChainID++)
    {
      rfChainConfig = Create<RFChain> ();
      m_rfChainList[rfChainID] = rfChainConfig;
    }

  /** Create Antenna Array List **/

  /* The following line determines the number of phased antenna arrays within the device */
  std::getline (file, line);
  m_totalAntennas = std::stod (line);

  for (uint8_t antennaIndex = 0; antennaIndex < m_totalAntennas; antennaIndex++)
    {
      Ptr<ParametricAntennaConfig> antennaConfig = Create<ParametricAntennaConfig> ();
      SectorIDList bhiSectors, txBeamformingSectors, rxBeamformingSectors;

      /* Read phased antenna array ID */
      std::getline (file, line);
      antennaID = std::stoul (line);

      /* Read RF Chain ID (To which RF Chain we connect this phased antenna Array). */
      std::getline (file, line);
      rfChainID = std::stoul (line);
      rfChainConfig = m_rfChainList[rfChainID];
      rfChainConfig->ConnectPhasedAntennaArray (antennaID, antennaConfig);
      antennaConfig->rfChain = rfChainConfig;

      /* Read phased antenna array azimuth orientation degree */
      std::getline (file, line);
      antennaConfig->azimuthOrientationDegree = std::stod (line);

      /* Read phased antenna array elevation orientation degree */
      std::getline (file, line);
      antennaConfig->elevationOrientationDegree = std::stod (line);

      /* Initialize rotation axes */
      antennaConfig->orientation.psi = 0;
      antennaConfig->orientation.theta = 0;
      antennaConfig->orientation.phi = 0;

      /* Read the number of antenna elements */
      std::getline (file, line);
      antennaConfig->numElements = std::stod (line);

      /* Read the number of quantization bits for phase */
      std::getline (file, line);
      antennaConfig->SetPhaseQuantizationBits (std::stoul (line));

      /* Read the number of quantization bits for amplitude */
      std::getline (file, line);
      antennaConfig->amplitudeQuantizationBits = std::stod (line);

      /* Allocate the directivity matrix. */
      antennaConfig->singleElementDirectivity = new Directivity *[AZIMUTH_CARDINALITY];
      for (uint16_t m = 0; m < AZIMUTH_CARDINALITY; m++)
        {
          antennaConfig->singleElementDirectivity[m] = new Directivity[ELEVATION_CARDINALITY];
        }

      /* Read the directivity of a single antenna element */
      for (uint16_t m = 0; m < AZIMUTH_CARDINALITY; m++)
        {
          std::getline (file, line);
          std::istringstream split (line);
          for (uint16_t n = 0; n < ELEVATION_CARDINALITY; n++)
            {
              std::getline (split, directivity, ',');
              antennaConfig->singleElementDirectivity[m][n] = std::stod (directivity);
            }
        }

      /* Allocate the steering vector 3D Matrix. */
      antennaConfig->steeringVector = new Complex **[AZIMUTH_CARDINALITY];
      for (uint16_t m = 0; m < AZIMUTH_CARDINALITY; m++)
        {
          antennaConfig->steeringVector[m] = new Complex *[ELEVATION_CARDINALITY];
          for (uint16_t n = 0; n < ELEVATION_CARDINALITY; n++)
            {
              antennaConfig->steeringVector[m][n] = new Complex[antennaConfig->numElements];
            }
        }

      /* Read the 3D steering vector of the antenna array */
      for (uint16_t l = 0; l < antennaConfig->numElements; l++)
        {
          for (uint16_t m = 0; m < AZIMUTH_CARDINALITY; m++)
            {
              std::getline (file, line);
              std::istringstream split (line);
              for (uint16_t n = 0; n < ELEVATION_CARDINALITY; n++)
                {
                  std::getline (split, amp, ',');
                  std::getline (split, phaseDelay, ',');
                  antennaConfig->steeringVector[m][n][l] = std::polar (std::stod (amp), std::stod (phaseDelay));
                }
            }
        }

      /* Read Quasi-omni antenna weights and calculate its directivity */
      Ptr<ParametricPatternConfig> quasiOmni = Create<ParametricPatternConfig> ();
      quasiOmni->SetWeights (ReadAntennaWeightsVector (file, antennaConfig->numElements));
      if (m_precalculatedPatterns)
        {
          antennaConfig->CalculateArrayPattern (quasiOmni->GetWeights (), quasiOmni->arrayPattern);
        }
      antennaConfig->SetQuasiOmniConfig (quasiOmni);

      /* Read the number of sectors within this antenna array */
      std::getline (file, line);
      nSectors = std::stoul (line);
      m_totalSectors += nSectors;

      for (uint8_t sector = 0; sector < nSectors; sector++)
        {
          Ptr<ParametricSectorConfig> sectorConfig = Create<ParametricSectorConfig> ();

          /* Read Sector ID */
          std::getline (file, line);
          sectorID = std::stoul (line);

          /* Read Sector Type */
          std::getline (file, line);
          sectorConfig->sectorType = static_cast<SectorType> (std::stoul (line));

          /* Read Sector Usage */
          std::getline (file, line);
          sectorConfig->sectorUsage = static_cast<SectorUsage> (std::stoul (line));

          if ((sectorConfig->sectorUsage == BHI_SECTOR) || (sectorConfig->sectorUsage == BHI_SLS_SECTOR))
            {
              bhiSectors.push_back (sectorID);
            }
          if ((sectorConfig->sectorUsage == SLS_SECTOR) || (sectorConfig->sectorUsage == BHI_SLS_SECTOR))
            {
              if ((sectorConfig->sectorType == TX_SECTOR) || (sectorConfig->sectorType == TX_RX_SECTOR))
                {
                  txBeamformingSectors.push_back (sectorID);
                  m_totalTxSectors++;
                }
              if ((sectorConfig->sectorType == RX_SECTOR) || (sectorConfig->sectorType == TX_RX_SECTOR))
                {
                  rxBeamformingSectors.push_back (sectorID);
                  m_totalRxSectors++;
                }
            }

          /* Read sector antenna weights vector and calculate its directivity */
          sectorConfig->SetWeights (ReadAntennaWeightsVector (file, antennaConfig->numElements));
          if (m_precalculatedPatterns)
            {
              antennaConfig->CalculateArrayPattern (sectorConfig->GetWeights (), sectorConfig->arrayPattern);
            }
          antennaConfig->sectorList[sectorID] = sectorConfig;
        }

      if (bhiSectors.size () > 0)
        {
          m_bhiAntennaList[antennaID] = bhiSectors;
        }

      if (txBeamformingSectors.size () > 0)
        {
          m_txBeamformingSectors[antennaID] = txBeamformingSectors;
        }

      if (rxBeamformingSectors.size () > 0)
        {
          m_rxBeamformingSectors[antennaID] = rxBeamformingSectors;
        }

      m_antennaArrayList[antennaID] = antennaConfig;
    }

  /* Close the file */
  file.close ();
  // For testing purposes - appendds 8 AWVs to all sectors
  //AppendAwvsToAllSectors ();
}

void
CodebookParametric::CreateMimoCodebook (std::string filename)
{
  NS_LOG_FUNCTION (this << "Loading Parametric Codebook file for a single MIMO " << filename);
  std::ifstream file;
  file.open (filename.c_str (), std::ifstream::in);
  NS_ASSERT_MSG (file.good (), " Codebook file not found in " + filename);
  std::string line;

  uint8_t nSectors;       /* The total number of sectors within a phased antenna array */
  AntennaID antennaID = 1;
  SectorID sectorID;
  std::string amp, phaseDelay, directivity;
  Ptr<RFChain> rfChainConfig;

  /* The first line determines the number of RF Chains within the device */
  std::getline (file, line);

  /* The following line determines the number of phased antenna arrays within the device */
  std::getline (file, line);

  Ptr<ParametricAntennaConfig> antennaConfig = Create<ParametricAntennaConfig> ();
  SectorIDList bhiSectors, txBeamformingSectors, rxBeamformingSectors;

  /* Read phased antenna array ID */
  std::getline (file, line);

  /* Read RF Chain ID (To which RF Chain we connect this phased antenna Array). */
  std::getline (file, line);
  rfChainConfig = Create<RFChain> ();
  rfChainConfig->ConnectPhasedAntennaArray (antennaID, antennaConfig);
  antennaConfig->rfChain = rfChainConfig;
  m_rfChainList[antennaID] = rfChainConfig;

  /* Read phased antenna array azimuth orientation degree */
  std::getline (file, line);
  antennaConfig->azimuthOrientationDegree = std::stod (line);

  /* Read phased antenna array elevation orientation degree */
  std::getline (file, line);
  antennaConfig->elevationOrientationDegree = std::stod (line);

  /* Initialize rotation axes */
  antennaConfig->orientation.psi = 0;
  antennaConfig->orientation.theta = 0;
  antennaConfig->orientation.phi = 0;

  /* Read the number of antenna elements */
  std::getline (file, line);
  antennaConfig->numElements = std::stod (line);

  /* Read the number of quantization bits for phase */
  std::getline (file, line);
  antennaConfig->SetPhaseQuantizationBits (std::stoul (line));

  /* Read the number of quantization bits for amplitude */
  std::getline (file, line);
  antennaConfig->amplitudeQuantizationBits = std::stod (line);

  /* Allocate the directivity matrix. */
  antennaConfig->singleElementDirectivity = new Directivity *[AZIMUTH_CARDINALITY];
  for (uint16_t m = 0; m < AZIMUTH_CARDINALITY; m++)
    {
      antennaConfig->singleElementDirectivity[m] = new Directivity[ELEVATION_CARDINALITY];
    }

  /* Read the directivity of a single antenna element */
  for (uint16_t m = 0; m < AZIMUTH_CARDINALITY; m++)
    {
      std::getline (file, line);
      std::istringstream split (line);
      for (uint16_t n = 0; n < ELEVATION_CARDINALITY; n++)
        {
          std::getline (split, directivity, ',');
          antennaConfig->singleElementDirectivity[m][n] = std::stod (directivity);
        }
    }

  /* Allocate the steering vector 3D Matrix. */
  antennaConfig->steeringVector = new Complex **[AZIMUTH_CARDINALITY];
  for (uint16_t m = 0; m < AZIMUTH_CARDINALITY; m++)
    {
      antennaConfig->steeringVector[m] = new Complex *[ELEVATION_CARDINALITY];
      for (uint16_t n = 0; n < ELEVATION_CARDINALITY; n++)
        {
          antennaConfig->steeringVector[m][n] = new Complex[antennaConfig->numElements];
        }
    }

  /* Read the 3D steering vector of the antenna array */
  for (uint16_t l = 0; l < antennaConfig->numElements; l++)
    {
      for (uint16_t m = 0; m < AZIMUTH_CARDINALITY; m++)
        {
          std::getline (file, line);
          std::istringstream split (line);
          for (uint16_t n = 0; n < ELEVATION_CARDINALITY; n++)
            {
              std::getline (split, amp, ',');
              std::getline (split, phaseDelay, ',');
              antennaConfig->steeringVector[m][n][l] = std::polar (std::stod (amp), std::stod (phaseDelay));
            }
        }
    }

  /* Read Quasi-omni antenna weights and calculate its directivity */
  Ptr<ParametricPatternConfig> quasiOmni = Create<ParametricPatternConfig> ();
  quasiOmni->SetWeights (ReadAntennaWeightsVector (file, antennaConfig->numElements));
  if (m_precalculatedPatterns)
    {
      antennaConfig->CalculateArrayPattern (quasiOmni->GetWeights (), quasiOmni->arrayPattern);

    }
  antennaConfig->SetQuasiOmniConfig (quasiOmni);

  /* Read the number of sectors within this antenna array */
  std::getline (file, line);
  nSectors = std::stoul (line);
  m_totalSectors += nSectors;

  for (uint8_t sector = 0; sector < nSectors; sector++)
    {
      Ptr<ParametricSectorConfig> sectorConfig = Create<ParametricSectorConfig> ();

      /* Read Sector ID */
      std::getline (file, line);
      sectorID = std::stoul (line);

      /* Read Sector Type */
      std::getline (file, line);
      sectorConfig->sectorType = static_cast<SectorType> (std::stoul (line));

      /* Read Sector Usage */
      std::getline (file, line);
      sectorConfig->sectorUsage = static_cast<SectorUsage> (std::stoul (line));

      if ((sectorConfig->sectorUsage == BHI_SECTOR) || (sectorConfig->sectorUsage == BHI_SLS_SECTOR))
        {
          bhiSectors.push_back (sectorID);
        }
      if ((sectorConfig->sectorUsage == SLS_SECTOR) || (sectorConfig->sectorUsage == BHI_SLS_SECTOR))
        {
          if ((sectorConfig->sectorType == TX_SECTOR) || (sectorConfig->sectorType == TX_RX_SECTOR))
            {
              txBeamformingSectors.push_back (sectorID);
              m_totalTxSectors++;
            }
          if ((sectorConfig->sectorType == RX_SECTOR) || (sectorConfig->sectorType == TX_RX_SECTOR))
            {
              rxBeamformingSectors.push_back (sectorID);
              m_totalRxSectors++;
            }
        }

      /* Read sector antenna weights vector and calculate its directivity */
      sectorConfig->SetWeights (ReadAntennaWeightsVector (file, antennaConfig->numElements));
      if (m_precalculatedPatterns)
        {
          antennaConfig->CalculateArrayPattern (sectorConfig->GetWeights (), sectorConfig->arrayPattern);
        }
      antennaConfig->sectorList[sectorID] = sectorConfig;
    }

  if (bhiSectors.size () > 0)
    {
      m_bhiAntennaList[antennaID] = bhiSectors;
    }

  if (txBeamformingSectors.size () > 0)
    {
      m_txBeamformingSectors[antennaID] = txBeamformingSectors;
    }

  if (rxBeamformingSectors.size () > 0)
    {
      m_rxBeamformingSectors[antennaID] = rxBeamformingSectors;
    }

  m_antennaArrayList[antennaID] = antennaConfig;

  /* Create the rest of the antenna arrays */
  for (antennaID = 2; antennaID <= m_totalAntennas; antennaID++)
    {
      Ptr<ParametricSectorConfig> srcSectorConfig;
      Ptr<ParametricAntennaConfig> dstAntennaConfig = Create<ParametricAntennaConfig> ();
      /* Copy antenna array config */
      dstAntennaConfig->CopyAntennaArray (antennaConfig);
      /* Copy quasi-omni config */
      Ptr<ParametricPatternConfig> quasiPattern = Create<ParametricPatternConfig> ();
      quasiPattern->m_weights = antennaConfig->GetQuasiOmniConfig ()->m_weights;
      quasiPattern->m_normalizationFactor = antennaConfig->GetQuasiOmniConfig ()->m_normalizationFactor;
      quasiPattern->arrayPattern = antennaConfig->GetQuasiOmniConfig ()->arrayPattern;
      dstAntennaConfig->SetQuasiOmniConfig (quasiPattern);
      for (SectorListI sectorIter = antennaConfig->sectorList.begin ();
           sectorIter != antennaConfig->sectorList.end (); sectorIter++)
        {
          Ptr<ParametricSectorConfig> dstSectorConfig = Create<ParametricSectorConfig> ();
          /* Copy sector config */
          srcSectorConfig = DynamicCast<ParametricSectorConfig> (sectorIter->second);
          dstSectorConfig->sectorType = srcSectorConfig->sectorType;
          dstSectorConfig->sectorUsage = srcSectorConfig->sectorUsage;
          dstSectorConfig->m_weights = srcSectorConfig->m_weights;
          dstSectorConfig->m_normalizationFactor = srcSectorConfig->m_normalizationFactor;
          dstSectorConfig->arrayPattern = srcSectorConfig->arrayPattern;
          dstAntennaConfig->sectorList[sectorIter->first] = dstSectorConfig;
        }

      /* Read the number of sectors within this antenna array */
      m_totalSectors += nSectors;

      if (bhiSectors.size () > 0)
        {
          m_bhiAntennaList[antennaID] = bhiSectors;
        }
      if (txBeamformingSectors.size () > 0)
        {
          m_txBeamformingSectors[antennaID] = txBeamformingSectors;
          m_totalTxSectors += txBeamformingSectors.size ();
        }
      if (rxBeamformingSectors.size () > 0)
        {
          m_rxBeamformingSectors[antennaID] = rxBeamformingSectors;
          m_totalRxSectors += rxBeamformingSectors.size ();
        }
      m_antennaArrayList[antennaID] = dstAntennaConfig;

      /* Create a corresponding RF Chain */
      rfChainConfig = Create<RFChain> ();
      m_rfChainList[antennaID] = rfChainConfig;
      rfChainConfig->ConnectPhasedAntennaArray (antennaID, dstAntennaConfig);
      dstAntennaConfig->rfChain = rfChainConfig;
    }
  m_cloned = true;
  /* Close the file */
  file.close ();
}

uint8_t
CodebookParametric::GetNumberSectorsPerAntenna (AntennaID antennaID) const
{
  AntennaArrayListCI iter = m_antennaArrayList.find (antennaID);
  if (iter != m_antennaArrayList.end ())
    {
      Ptr<ParametricAntennaConfig> antennaConfig = StaticCast<ParametricAntennaConfig> (iter->second);
      return antennaConfig->sectorList.size ();
    }
  else
    {
      NS_ABORT_MSG ("Cannot find the specified antenna ID=" << static_cast<uint16_t> (antennaID));
    }
}

double
CodebookParametric::GetTxGainDbi (double angle)
{
  return GetTxGainDbi (angle, 0);
}

double
CodebookParametric::GetRxGainDbi (double angle)
{
  return GetRxGainDbi (angle, 0);
}

double
CodebookParametric::GetTxGainDbi (double azimuth, double elevation)
{
  NS_LOG_FUNCTION (this << azimuth << elevation);
  return -1;
}

double
CodebookParametric::GetRxGainDbi (double azimuth, double elevation)
{
  NS_LOG_FUNCTION (this << azimuth << elevation);
  return -1;
}

void
CodebookParametric::UpdateSectorWeights (AntennaID antennaID, SectorID sectorID, WeightsVector &weightsVector)
{
  AntennaArrayListCI iter = m_antennaArrayList.find (antennaID);
  if (iter != m_antennaArrayList.end ())
    {
      Ptr<ParametricAntennaConfig> antennaConfig = StaticCast<ParametricAntennaConfig> (iter->second);
      SectorListI sectorIter = antennaConfig->sectorList.find (sectorID);
      if (sectorIter != antennaConfig->sectorList.end ())
        {
          Ptr<ParametricSectorConfig> sectorConfig = DynamicCast<ParametricSectorConfig> (sectorIter->second);
          sectorConfig->SetWeights (weightsVector);
          antennaConfig->CalculateArrayPattern (weightsVector, sectorConfig->arrayPattern);
        }
      else
        {
          NS_ABORT_MSG ("Cannot find the specified Sector ID=" << static_cast<uint16_t> (sectorID));
        }
    }
  else
    {
      NS_ABORT_MSG ("Cannot find the specified Antenna ID=" << static_cast<uint16_t> (antennaID));
    }
}

void
CodebookParametric::CalculateArrayPatterns (AntennaID antennaID, uint16_t azimuthAngle, uint16_t elevationAngle)
{
  NS_LOG_FUNCTION (this << uint16_t (antennaID << azimuthAngle << elevationAngle));
  AntennaArrayListCI iter = m_antennaArrayList.find (antennaID);
  Ptr<ParametricAntennaConfig> antennaConfig;
  Ptr<ParametricSectorConfig> sectorConfig;
  Ptr<Parametric_AWV_Config> awvConfig;
  if (iter != m_antennaArrayList.end ())
    {
      antennaConfig = StaticCast<ParametricAntennaConfig> (iter->second);
      for (SectorListI sectorIter = antennaConfig->sectorList.begin ();
           sectorIter != antennaConfig->sectorList.end (); sectorIter++)
        {
          sectorConfig = DynamicCast<ParametricSectorConfig> (sectorIter->second);
          sectorConfig->CalculateArrayPattern (antennaConfig, azimuthAngle, elevationAngle);
          for (AWV_LIST_I awvIt = sectorConfig->awvList.begin ();  awvIt != sectorConfig->awvList.end (); awvIt++)
            {
              awvConfig = DynamicCast<Parametric_AWV_Config> (*awvIt);
              awvConfig->CalculateArrayPattern (antennaConfig, azimuthAngle, elevationAngle);
            }
        }
      antennaConfig->GetQuasiOmniConfig ()->CalculateArrayPattern (antennaConfig, azimuthAngle, elevationAngle);
    }
  else
    {
      NS_ABORT_MSG ("Cannot find the specified Antenna ID=" << static_cast<uint16_t> (antennaID));
    }
}

bool
CodebookParametric::ArrayPatternsPrecalculated (void) const
{
  return m_precalculatedPatterns;
}

void
CodebookParametric::PrintWeights (WeightsVector weightsVector)
{
  NS_LOG_FUNCTION (this);
  for (WeightsVectorI it = weightsVector.begin (); it != weightsVector.end () - 1; it++)
    {
      std::cout << *it << ",";
    }
  std::cout << *(weightsVector.end () - 1) << std::endl;
}

void
CodebookParametric::PrintCodebookContent (bool printAWVs)
{
  Ptr<ParametricAntennaConfig> antennaConfig;
  Ptr<ParametricSectorConfig> sectorConfig;
  for (AntennaArrayListI iter = m_antennaArrayList.begin (); iter != m_antennaArrayList.end (); iter++)
    {
      antennaConfig = StaticCast<ParametricAntennaConfig> (iter->second);
      std::cout << "**********************************************************" << std::endl;
      std::cout << "**********************************************************" << std::endl;
      std::cout << "Phased Antenna Array (" << uint16_t (iter->first) << ")" << std::endl;
      std::cout << "**********************************************************" << std::endl;
      std::cout << "**********************************************************" << std::endl;
      std::cout << "Number of Elements          = " << antennaConfig->numElements << std::endl;
      std::cout << "Antenna Orientation         = " << antennaConfig->azimuthOrientationDegree << std::endl;
      std::cout << "Amplitude Quantization Bits = " << uint16_t (antennaConfig->amplitudeQuantizationBits) << std::endl;
      std::cout << "Phase Quantization Bits     = " << uint16_t (antennaConfig->GetPhaseQuantizationBits ()) << std::endl;
      std::cout << "Number of Sectors           = " << antennaConfig->sectorList.size () << std::endl;
      std::cout << "Quasi-omni Weights          = ";
      PrintWeights (antennaConfig->GetQuasiOmniConfig ()->GetWeights ());
      for (SectorListI sectorIter = antennaConfig->sectorList.begin ();
           sectorIter != antennaConfig->sectorList.end (); sectorIter++)
        {
          sectorConfig = DynamicCast<ParametricSectorConfig> (sectorIter->second);
          std::cout << "**********************************************************" << std::endl;
          std::cout << "Sector ID (" << uint16_t (sectorIter->first) << ")" << std::endl;
          std::cout << "**********************************************************" << std::endl;
          std::cout << "Sector Type             = " << sectorConfig->sectorType << std::endl;
          std::cout << "Sector Usage            = " << sectorConfig->sectorUsage << std::endl;
          std::cout << "Sector Weights          = ";
          PrintWeights (sectorConfig->GetWeights ());
          if (printAWVs)
            {
              for (uint8_t awvIndex = 0; awvIndex < sectorConfig->awvList.size (); awvIndex++)
                {
                  std::cout << "**********************************************************" << std::endl;
                  std::cout << "AWV ID (" << uint16_t (awvIndex) << ")" << std::endl;
                  std::cout << "Weights             = ";
                  PrintWeights (DynamicCast<Parametric_AWV_Config> (sectorConfig->awvList[awvIndex])->GetWeights ());
                }
            }
        }
    }
}

void
CodebookParametric::PrintAWVs (AntennaID antennaID, SectorID sectorID)
{
  AntennaArrayListCI iter = m_antennaArrayList.find (antennaID);
  if (iter != m_antennaArrayList.end ())
    {
      Ptr<ParametricAntennaConfig> antennaConfig = StaticCast<ParametricAntennaConfig> (iter->second);
      SectorListI sectorIter = antennaConfig->sectorList.find (sectorID);
      if (sectorIter != antennaConfig->sectorList.end ())
        {
          Ptr<ParametricSectorConfig> sectorConfig = DynamicCast<ParametricSectorConfig> (sectorIter->second);
          for (uint8_t awvIndex = 0; awvIndex < sectorConfig->awvList.size (); awvIndex++)
            {
              std::cout << "**********************************************************" << std::endl;
              std::cout << "AWV ID (" << uint16_t (awvIndex) << ")" << std::endl;
              std::cout << "Weights             = ";
              PrintWeights (DynamicCast<Parametric_AWV_Config> (sectorConfig->awvList[awvIndex])->GetWeights ());
            }
        }
      else
        {
          NS_ABORT_MSG ("Cannot find the specified Sector ID=" << static_cast<uint16_t> (sectorID));
        }
    }
  else
    {
      NS_ABORT_MSG ("Cannot find the specified Antenna ID=" << static_cast<uint16_t> (antennaID));
    }
}

void
CodebookParametric::UpdateQuasiOmniWeights (AntennaID antennaID, WeightsVector &weightsVector)
{
  AntennaArrayListCI iter = m_antennaArrayList.find (antennaID);
  if (iter != m_antennaArrayList.end ())
    {
      Ptr<ParametricAntennaConfig> antennaConfig = StaticCast<ParametricAntennaConfig> (iter->second);
      antennaConfig->GetQuasiOmniConfig ()->SetWeights (weightsVector);
      antennaConfig->CalculateArrayPattern (weightsVector, antennaConfig->GetQuasiOmniConfig ()->arrayPattern);
    }
  else
    {
      NS_ABORT_MSG ("Cannot find the specified antenna ID=" << static_cast<uint16_t> (antennaID));
    }
}

void
CodebookParametric::AppendSector (AntennaID antennaID, SectorID sectorID, SectorUsage sectorUsage,
                                  SectorType sectorType, WeightsVector &weightsVector)
{
  AntennaArrayListCI iter = m_antennaArrayList.find (antennaID);
  if (iter != m_antennaArrayList.end ())
    {
      Ptr<ParametricAntennaConfig> antennaConfig = StaticCast<ParametricAntennaConfig> (iter->second);
      Ptr<ParametricSectorConfig> sectorConfig = Create<ParametricSectorConfig> ();
      sectorConfig->sectorType = sectorType;
      sectorConfig->sectorUsage = sectorUsage;
      if ((sectorConfig->sectorUsage == BHI_SECTOR) || (sectorConfig->sectorUsage == BHI_SLS_SECTOR))
        {
//          bhiSectors.push_back (sectorID);
        }
      if ((sectorConfig->sectorUsage == SLS_SECTOR) || (sectorConfig->sectorUsage == BHI_SLS_SECTOR))
        {
          if ((sectorConfig->sectorType == TX_SECTOR) || (sectorConfig->sectorType == TX_RX_SECTOR))
            {
//              txBeamformingSectors.push_back (sectorID);
              m_totalTxSectors++;
            }
          if ((sectorConfig->sectorType == RX_SECTOR) || (sectorConfig->sectorType == TX_RX_SECTOR))
            {
//              rxBeamformingSectors.push_back (sectorID);
              m_totalRxSectors++;
            }
        }

      /* Read sector weights and calculate directivity */
      sectorConfig->SetWeights (weightsVector);
//      sectorConfig->directivity = CalculateArrayPattern (weightsVector, antennaConfig->steeringVector);

      /* Check if the sector exists*/
      SectorListI sectorIter = antennaConfig->sectorList.find (sectorID);
      if (sectorIter != antennaConfig->sectorList.end ())
        {
          /* If the sector exists, we need to remove it from the current sector lists above*/
          NS_LOG_DEBUG ("Updating existing sector in the codebook");
        }
      else
        {
          m_totalSectors += 1;
          NS_LOG_DEBUG ("Appending new sector to the codebook");
        }
      antennaConfig->sectorList[sectorID] = sectorConfig;
    }
  else
    {
      NS_ABORT_MSG ("Cannot find the specified Antenna ID=" << static_cast<uint16_t> (antennaID));
    }
}

void
CodebookParametric::AppendBeamRefinementAwv (AntennaID antennaID, SectorID sectorID, WeightsVector &weightsVector)
{
  AntennaArrayListCI iter = m_antennaArrayList.find (antennaID);
  if (iter != m_antennaArrayList.end ())
    {
      Ptr<ParametricAntennaConfig> antennaConfig = StaticCast<ParametricAntennaConfig> (iter->second);
      SectorListI sectorIter = antennaConfig->sectorList.find (sectorID);
      if (sectorIter != antennaConfig->sectorList.end ())
        {
          Ptr<ParametricSectorConfig> sectorConfig = DynamicCast<ParametricSectorConfig> (sectorIter->second);
          Ptr<Parametric_AWV_Config> awvConfig = Create<Parametric_AWV_Config> ();
          awvConfig->SetWeights (weightsVector);
          antennaConfig->CalculateArrayPattern (sectorConfig->GetWeights (), awvConfig->arrayPattern);
          sectorConfig->awvList.push_back (awvConfig);
          /* Change this */
          NS_ASSERT_MSG (sectorConfig->awvList.size () <= 64, "We can append upto 64 AWV per sector.");
        }
      else
        {
          NS_ABORT_MSG ("Cannot find the specified Sector ID=" << static_cast<uint16_t> (sectorID));
        }
    }
  else
    {
      NS_ABORT_MSG ("Cannot find the specified Antenna ID=" << static_cast<uint16_t> (antennaID));
    }
}

void
CodebookParametric::AppendBeamRefinementAwv (AntennaID antennaID, SectorID sectorID,
                                             double steeringAngleAzimuth, double steeringAngleElevation)
{
  AntennaArrayListCI iter = m_antennaArrayList.find (antennaID);
  if (iter != m_antennaArrayList.end ())
    {
      Ptr<ParametricAntennaConfig> antennaConfig = StaticCast<ParametricAntennaConfig> (iter->second);
      SectorListI sectorIter = antennaConfig->sectorList.find (sectorID);
      if (sectorIter != antennaConfig->sectorList.end ())
        {
          Ptr<ParametricSectorConfig> sectorConfig = DynamicCast<ParametricSectorConfig> (sectorIter->second);
          NS_ASSERT_MSG (sectorConfig->awvList.size () <= 64, "We can append up-to 64 custom AWVs per sector.");
          Ptr<Parametric_AWV_Config> awvConfig = Create<Parametric_AWV_Config> ();
          if (steeringAngleAzimuth < 0)
            {
              steeringAngleAzimuth += 360;
            }
//          if (steeringAngleElevation < 0)
//            {
//              steeringAngleElevation += 180;
//            }
          steeringAngleElevation = std::abs (steeringAngleElevation - 90);

          uint azimuth = steeringAngleAzimuth;
          uint elevation = steeringAngleElevation;
          WeightsVector weightsVector;
          /* Calculate the weights vector as the conjugate of the steering vector to the specified direction */
          for (uint16_t i = 0; i < antennaConfig->numElements; i++)
            {
              weightsVector.push_back (std::conj (antennaConfig->steeringVector [azimuth][elevation][i]));
            }
          awvConfig->SetWeights (weightsVector);
          antennaConfig->CalculateArrayPattern (awvConfig->GetWeights (), awvConfig->arrayPattern);
          sectorConfig->awvList.push_back (awvConfig);
        }
      else
        {
          NS_ABORT_MSG ("Cannot find the specified Sector ID=" << static_cast<uint16_t> (sectorID));
        }
    }
  else
    {
      NS_ABORT_MSG ("Cannot find the specified Antenna ID=" << static_cast<uint16_t> (antennaID));
    }
}

void
CodebookParametric::AppendAwvsToAllSectors (void)
{
  AntennaArrayListCI iter = m_antennaArrayList.find (1);
  if (iter != m_antennaArrayList.end ())
    {
      Ptr<ParametricAntennaConfig> antennaConfig = StaticCast<ParametricAntennaConfig> (iter->second);
      for (SectorListI sectorIter = antennaConfig->sectorList.begin(); sectorIter != antennaConfig->sectorList.end (); sectorIter++)
      {
          AppendBeamRefinementAwv (1, sectorIter->first, -3, 0);
          AppendBeamRefinementAwv (1, sectorIter->first, -2, 0);
          AppendBeamRefinementAwv (1, sectorIter->first, -1, 0);
          AppendBeamRefinementAwv (1, sectorIter->first,  0, 0);
          AppendBeamRefinementAwv (1, sectorIter->first, +1, 0);
          AppendBeamRefinementAwv (1, sectorIter->first, +2, 0);
          AppendBeamRefinementAwv (1, sectorIter->first, +3, 0);
          AppendBeamRefinementAwv (1, sectorIter->first, +4, 0);
      }
    }
}

void
CodebookParametric::AppendAwvsForSuMimoBFT (void)
{
  for (AntennaArrayListI iter = m_antennaArrayList.begin (); iter != m_antennaArrayList.end (); iter++)
    {
      Ptr<ParametricAntennaConfig> antennaConfig = StaticCast<ParametricAntennaConfig> (iter->second);
      SectorListI sectorIter = antennaConfig->sectorList.begin();
      double azAngle = 0;
      double elAngle = -45;
      for (uint8_t i = 0; i < 3; i++)
      {
          for (uint8_t j = 0; j < 18; j++)
            {
              double azAwvAngle = azAngle - 10;
              for (uint8_t k = 0; k < 5; k++)
                {
                  if (azAwvAngle > 180)
                    azAwvAngle = azAwvAngle - 360;
                  AppendBeamRefinementAwv (iter->first, sectorIter->first, azAwvAngle, elAngle);
                  azAwvAngle += 5;
                }
              sectorIter++;
              azAngle+= 20;
            }
          azAngle = 0;
          elAngle+= 45;
      }
    }
}

void
CodebookParametric::AppendAwvsForSuMimoBFT_27 (void)
{
  for (AntennaArrayListI iter = m_antennaArrayList.begin (); iter != m_antennaArrayList.end (); iter++)
    {
      Ptr<ParametricAntennaConfig> antennaConfig = StaticCast<ParametricAntennaConfig> (iter->second);
      SectorListI sectorIter = antennaConfig->sectorList.begin();
      /* Set up the azimuth and elevation angles of the first sector of the codebook, as defined in the codebook generator */
      double azAngle = 0;
      double elAngle = - 45;
      /* We have 3 different elevation angles defined for this codebook */
      for (uint8_t i = 0; i < 3; i++)
      {
          /* The first 5 sectors are separated by 20 degrees in the azimuth */
          for (uint8_t j = 0; j < 5; j++)
            {
              /* The 5 AWVs defined per sector offer granularity of 5 degrees and we define them as azAngle -10, -5, +0, +5, +10 */
              double azAwvAngle = azAngle - 10;
              for (uint8_t k = 0; k < 5; k++)
                {
                  AppendBeamRefinementAwv (iter->first, sectorIter->first, azAwvAngle, elAngle);
                  azAwvAngle += 5;
                }
              sectorIter++;
              azAngle+= 20;
            }
          /* In order to avoid repetitions of the steering vectors due to the symmetry of the codebook we skip some azimuth sectors and continue from 200 = -160*/
          azAngle = -160;
          /* The next four sectors are again separated by 20 degrees in the azimuth */
          for (uint8_t j = 0; j < 4; j++)
            {
              double azAwvAngle = azAngle - 10;
              for (uint8_t k = 0; k < 5; k++)
                {
                  AppendBeamRefinementAwv (iter->first, sectorIter->first, azAwvAngle, elAngle);
                  azAwvAngle += 5;
                }
              sectorIter++;
              azAngle+= 20;
            }
          /* Move to the next elevation angle */
          azAngle = 0;
          elAngle+= 45;
      }
    }
}

uint16_t
CodebookParametric::GetNumberOfElements (AntennaID antennaID) const
{
  AntennaArrayListCI iter = m_antennaArrayList.find (antennaID);
  if (iter != m_antennaArrayList.end ())
    {
      Ptr<ParametricAntennaConfig> antennaConfig = StaticCast<ParametricAntennaConfig> (iter->second);
      return antennaConfig->numElements;
    }
  else
    {
      NS_ABORT_MSG ("Cannot find the specified Antenna ID=" << static_cast<uint16_t> (antennaID));
      return 0;
    }
}

void
CodebookParametric::CopyCodebook (const Ptr<Codebook> codebook)
{
  NS_LOG_FUNCTION (this << codebook);
  Ptr<CodebookParametric> srcCodebook = DynamicCast<CodebookParametric> (codebook);
  Ptr<ParametricAntennaConfig> srcAntennaConfig;
  Ptr<ParametricSectorConfig> srcSectorConfig;
  /* Copy antenna arrays */
  for (AntennaArrayListI arrayIt = srcCodebook->m_antennaArrayList.begin ();
       arrayIt != srcCodebook->m_antennaArrayList.end (); arrayIt++)
    {
      Ptr<ParametricAntennaConfig> dstAntennaConfig = Create<ParametricAntennaConfig> ();
      /* Copy antenna array config */
      srcAntennaConfig = StaticCast<ParametricAntennaConfig> (arrayIt->second);
      dstAntennaConfig->CopyAntennaArray (srcAntennaConfig);
      /* Copy RF chain (Temporary) */
      Ptr<RFChain> dstRfChain = Create<RFChain> ();
      dstRfChain->ConnectPhasedAntennaArray (arrayIt->first, dstAntennaConfig);
      dstAntennaConfig->rfChain = dstRfChain;
      m_rfChainList[arrayIt->first] = dstRfChain;
      /* Copy quasi-omni config */
      Ptr<ParametricPatternConfig> quasiPattern = Create<ParametricPatternConfig> ();
      quasiPattern->m_weights = srcAntennaConfig->GetQuasiOmniConfig ()->m_weights;
      quasiPattern->m_normalizationFactor = srcAntennaConfig->GetQuasiOmniConfig ()->m_normalizationFactor;
      quasiPattern->arrayPattern = srcAntennaConfig->GetQuasiOmniConfig ()->arrayPattern;
      dstAntennaConfig->SetQuasiOmniConfig (quasiPattern);
      for (SectorListI sectorIter = srcAntennaConfig->sectorList.begin ();
           sectorIter != srcAntennaConfig->sectorList.end (); sectorIter++)
        {
          Ptr<ParametricSectorConfig> dstSectorConfig = Create<ParametricSectorConfig> ();
          /* Copy sector config */
          srcSectorConfig = DynamicCast<ParametricSectorConfig> (sectorIter->second);
          dstSectorConfig->m_normalizationFactor = srcSectorConfig->m_normalizationFactor;
          dstSectorConfig->sectorType = srcSectorConfig->sectorType;
          dstSectorConfig->sectorUsage = srcSectorConfig->sectorUsage;
          dstSectorConfig->m_weights = srcSectorConfig->m_weights;
          dstSectorConfig->arrayPattern = srcSectorConfig->arrayPattern;
          dstAntennaConfig->sectorList[sectorIter->first] = dstSectorConfig;
        }
      m_antennaArrayList[arrayIt->first] = dstAntennaConfig;
    }
//  /* Copy RF chains */
//  for (RFChainListI chainIt = srcCodebook->m_rfChainList.begin ();
//       chainIt != srcCodebook->m_rfChainList.end (); chainIt++)
//    {
//      Ptr<RFChain> dstRfChain = Create<RFChain> ();
//      dstRfChain
//      m_rfChainList[chainIt->first] = dstRfChain;
//    }
//  Ptr<RFChain> srcRfChain;
  m_precalculatedPatterns = srcCodebook->m_precalculatedPatterns;
  /* Set that we have cloned another codebook to avoid*/
  m_cloned = true;
  /* Call parent class. */
  Codebook::CopyCodebook (srcCodebook);
}

Complex
CodebookParametric::GetAntennaArrayPattern (Ptr<PatternConfig> config,
                                            uint16_t azimuthAngle, uint16_t elevationAngle)
{
  Ptr<ParametricPatternConfig> parametricConfig = DynamicCast<ParametricPatternConfig> (config);
  if (m_precalculatedPatterns)
    {
      return parametricConfig->GetArrayPattern ()[azimuthAngle][elevationAngle];
    }
  else
    {
      return parametricConfig->GetArrayPattern (azimuthAngle, elevationAngle);
    }
}

Complex
CodebookParametric::GetTxAntennaArrayPattern (uint16_t azimuthAngle, uint16_t elevationAngle)
{
  if (m_precalculatedPatterns)
    {
      return DynamicCast<ParametricPatternConfig> (GetTxPatternConfig ())->GetArrayPattern ()[azimuthAngle][elevationAngle];
    }
  else
    {
      return DynamicCast<ParametricPatternConfig> (GetTxPatternConfig ())->GetArrayPattern (azimuthAngle, elevationAngle);
    }
}

Complex
CodebookParametric::GetRxAntennaArrayPattern (uint16_t azimuthAngle, uint16_t elevationAngle)
{
  if (m_precalculatedPatterns)
    {
      return DynamicCast<ParametricPatternConfig> (GetRxPatternConfig ())->GetArrayPattern ()[azimuthAngle][elevationAngle];
    }
  else
    {
      return DynamicCast<ParametricPatternConfig> (GetRxPatternConfig ())->GetArrayPattern (azimuthAngle, elevationAngle);
    }
}

}
