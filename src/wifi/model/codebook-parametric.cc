/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#include "ns3/log.h"
#include "ns3/string.h"
#include "codebook-parametric.h"
#include <fstream>
#include <string>
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CodebookParametric");

NS_OBJECT_ENSURE_REGISTERED (CodebookParametric);

ArrayPattern
ParametricPatternConfig::GetArrayPattern (void) const
{
  return arrayPattern;
}

TypeId
CodebookParametric::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CodebookParametric")
    .SetGroupName ("Wifi")
    .SetParent<Codebook> ()
    .AddConstructor<CodebookParametric> ()
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
}

void
CodebookParametric::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  for (AntennaArrayListI iter = m_antennaArrayList.begin (); iter != m_antennaArrayList.end (); iter++)
    {
      Ptr<ParametricAntennaConfig> antennaConfig = StaticCast<ParametricAntennaConfig> (iter->second);
      for (SectorListI sectorIter = antennaConfig->sectorList.begin ();
           sectorIter != antennaConfig->sectorList.end (); sectorIter++)
        {
          Ptr<ParametricSectorConfig> sectorConfig = DynamicCast<ParametricSectorConfig> (sectorIter->second);
          for (uint16_t m = 0; m < AZIMUTH_CARDINALITY; m++)
            {
              delete[] sectorConfig->directivity[m];
              delete[] sectorConfig->arrayPattern[m];
            }
          delete[] sectorConfig->directivity;
          delete[] sectorConfig->arrayPattern;
          for (AWV_LIST_I awvIt = sectorConfig->awvList.begin (); awvIt != sectorConfig->awvList.end (); awvIt++)
            {
              Ptr<Parametric_AWV_Config> awvConfig = DynamicCast<Parametric_AWV_Config> (*awvIt);
              for (uint16_t m = 0; m < AZIMUTH_CARDINALITY; m++)
                {
                  delete[] awvConfig->directivity[m];
                  delete[] awvConfig->arrayPattern[m];
                }
              delete[] awvConfig->directivity;
              delete[] awvConfig->arrayPattern;
            }
        }

      for (uint16_t m = 0; m < AZIMUTH_CARDINALITY; m++)
        {
          delete[] antennaConfig->singleElementDirectivity[m];
          delete[] antennaConfig->quasiOmniDirectivity[m];
          delete[] antennaConfig->quasiOmniArrayPattern[m];
          for (uint16_t n = 0; n < ELEVATION_CARDINALITY; n++)
            {
              delete[] antennaConfig->steeringVector[m][n];
            }
          delete[] antennaConfig->steeringVector[m];
        }

      delete[] antennaConfig->singleElementDirectivity;
      delete[] antennaConfig->quasiOmniDirectivity;
      delete[] antennaConfig->quasiOmniArrayPattern;
      delete[] antennaConfig->steeringVector;
    }
  Codebook::DoDispose ();
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
      LoadCodebook (m_fileName);
    }
}

WeightsVector
CodebookParametric::ReadAntennaWeightsVector (std::ifstream &file, double elements)
{
  WeightsVector weights;
  std::string amp, phase;
  std::string line;
  std::getline (file, line);
  std::istringstream split (line);
  for (uint16_t i = 0; i < elements; i++)
    {
      getline (split, amp, ',');
      getline (split, phase, ',');
      weights.push_back (std::polar (std::stod (amp), std::stod (phase)));
    }
  return weights;
}

void
CodebookParametric::LoadCodebook (std::string filename)
{
  NS_LOG_FUNCTION (this << "Loading Numerical Codebook file " << filename);
  std::ifstream file;
  file.open (filename.c_str (), std::ifstream::in);
  NS_ASSERT_MSG (file.good (), " Codebook file not found in " + filename);
  std::string line;

  uint8_t nSectors;
  RFChain rfChainID;
  AntennaID antennaID;
  SectorID sectorID;
  std::string amp, phaseDelay, directivity;


  std::getline (file, line);
  m_totalAntennas = std::stod (line);

  for (uint8_t antennaIndex = 0; antennaIndex < m_totalAntennas; antennaIndex++)
    {
      Ptr<ParametricAntennaConfig> antennaConfig = Create<ParametricAntennaConfig> ();
      SectorIDList bhiSectors, txBeamformingSectors, rxBeamformingSectors;

      std::getline (file, line);
      antennaID = std::stoul (line);

      std::getline (file, line);
      antennaConfig->azimuthOrientationDegree = std::stod (line);

      std::getline (file, line);
      antennaConfig->elevationOrientationDegree = std::stod (line);

      antennaConfig->orientation.x = 0;
      antennaConfig->orientation.y = 0;
      antennaConfig->orientation.z = 1;

      std::getline (file, line);
      antennaConfig->elements = std::stod (line);

      std::getline (file, line);
      antennaConfig->phaseQuantizationBits = std::stoul (line);
      antennaConfig->phaseQuantizationStepSize = 2 * M_PI / (std::pow (2, antennaConfig->phaseQuantizationBits));

      std::getline (file, line);
      antennaConfig->amplitudeQuantizationBits = std::stod (line);

      antennaConfig->singleElementDirectivity = new Directivity *[AZIMUTH_CARDINALITY];
      for (uint16_t m = 0; m < AZIMUTH_CARDINALITY; m++)
        {
          antennaConfig->singleElementDirectivity[m] = new Directivity[ELEVATION_CARDINALITY];
        }

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

      antennaConfig->steeringVector = new Complex * *[AZIMUTH_CARDINALITY];
      for (uint16_t m = 0; m < AZIMUTH_CARDINALITY; m++)
        {
          antennaConfig->steeringVector[m] = new Complex *[ELEVATION_CARDINALITY];
          for (uint16_t n = 0; n < ELEVATION_CARDINALITY; n++)
            {
              antennaConfig->steeringVector[m][n] = new Complex[antennaConfig->elements];
            }
        }

      for (uint16_t l = 0; l < antennaConfig->elements; l++)
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

      antennaConfig->quasiOmniWeights = ReadAntennaWeightsVector (file, antennaConfig->elements);
      antennaConfig->CalculateDirectivity (&antennaConfig->quasiOmniWeights,
                                           antennaConfig->quasiOmniArrayPattern, antennaConfig->quasiOmniDirectivity);

      std::getline (file, line);
      nSectors = std::stoul (line);
      m_totalSectors += nSectors;

      for (uint8_t sector = 0; sector < nSectors; sector++)
        {
          Ptr<ParametricSectorConfig> sectorConfig = Create<ParametricSectorConfig> ();

          std::getline (file, line);
          sectorID = std::stoul (line);

          std::getline (file, line);
          sectorConfig->sectorType = static_cast<SectorType> (std::stoul (line));

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

          sectorConfig->elementsWeights = ReadAntennaWeightsVector (file, antennaConfig->elements);
          antennaConfig->CalculateDirectivity (&sectorConfig->elementsWeights, sectorConfig->arrayPattern, sectorConfig->directivity);
          antennaConfig->sectorList[sectorID] = sectorConfig;
        }

      if (bhiSectors.size () > 0)
        {
          m_bhiAntennasList[antennaID] = bhiSectors;
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
  return GetGainDbi (azimuth, elevation, DynamicCast<ParametricPatternConfig> (m_txPattern)->directivity);
}

double
CodebookParametric::GetRxGainDbi (double azimuth, double elevation)
{
  NS_LOG_FUNCTION (this << azimuth << elevation);
  if (m_quasiOmniMode)
    {
      Ptr<ParametricAntennaConfig> antennaConfig = StaticCast<ParametricAntennaConfig> (m_antennaArrayList[m_antennaID]);
      return GetGainDbi (azimuth, elevation, antennaConfig->quasiOmniDirectivity);
    }
  else
    {
      return GetGainDbi (azimuth, elevation, DynamicCast<ParametricPatternConfig> (m_rxPattern)->directivity);
    }
}

double
CodebookParametric::GetGainDbi (double azimuth, double elevation, DirectivityMatrix directivity) const
{
  NS_LOG_FUNCTION (this << azimuth << elevation);
  azimuth = RadiansToDegrees (azimuth);
  elevation = RadiansToDegrees (elevation);

  if (azimuth < 0)
    {
      azimuth += 360;
    }
  elevation += 90;
  uint azimuthIdx = floor (azimuth);
  uint elevationIdx = floor (elevation);
  return directivity[azimuthIdx][elevationIdx];
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
          sectorConfig->elementsWeights = weightsVector;
          antennaConfig->CalculateDirectivity (&weightsVector, sectorConfig->arrayPattern, sectorConfig->directivity);
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

double
ParametricAntennaConfig::CalculateDirectivity (double azimuth, double elevation, WeightsVector &weightsVector)
{
  Complex value = 0;
  uint16_t j = 0;
  uint16_t azimuthIdx = floor (azimuth);
  uint16_t elevationIdx = floor (elevation);
  for (WeightsVectorCI it = weightsVector.begin (); it != weightsVector.end (); it++, j++)
    {
      value += singleElementDirectivity[azimuthIdx][elevationIdx] * (*it) * steeringVector[azimuthIdx][elevationIdx][j];
    }
  return abs (value);
}

double
ParametricAntennaConfig::CalculateDirectivityForDirection (double azimuth, double elevation)
{
  uint16_t azimuthIdx = floor (azimuth);
  uint16_t elevationIdx = floor (elevation);

  WeightsVector weightsVector;
  double phaseShift, amp;
  Complex conjValue;
  for (uint16_t i = 0; i < elements; i++)
    {
      conjValue = std::conj (steeringVector[azimuthIdx][elevationIdx][i]);
      amp = std::abs (conjValue);
      phaseShift = phaseQuantizationStepSize * std::floor ((std::arg (conjValue) + M_PI) / phaseQuantizationStepSize);
      weightsVector.push_back (std::polar (amp, phaseShift));
    }
  return CalculateDirectivity (azimuth, elevation, weightsVector);
}

void
ParametricAntennaConfig::CalculateDirectivity (WeightsVector *weights, ArrayPattern &arrayPattern, DirectivityMatrix &directivity)
{
  arrayPattern = new Complex *[AZIMUTH_CARDINALITY];
  directivity = new double *[AZIMUTH_CARDINALITY];
  for (uint16_t m = 0; m < AZIMUTH_CARDINALITY; m++)
    {
      arrayPattern[m] = new Complex[ELEVATION_CARDINALITY];
      directivity[m] = new double[ELEVATION_CARDINALITY];
      for (uint16_t n = 0; n < ELEVATION_CARDINALITY; n++)
        {
          Complex value = 0;
          uint16_t j = 0;
          for (WeightsVectorCI it = weights->begin (); it != weights->end (); it++, j++)
            {
              value += (*it) * steeringVector[m][n][j];
            }
          value *= singleElementDirectivity[m][n];
          arrayPattern[m][n] = value;
          directivity[m][n] = 10.0 * std::log10 (abs (value));
        }
    }
}

ArrayPattern
ParametricAntennaConfig::GetQuasiOmniArrayPattern (void) const
{
  return quasiOmniArrayPattern;
}

void
CodebookParametric::PrintDirectivity (DirectivityMatrix directivity) const
{
  for (uint16_t m = 0; m < AZIMUTH_CARDINALITY; m++)
    {
      for (uint16_t n = 0; n < ELEVATION_CARDINALITY - 1; n++)
        {
          printf ("%2.4f,", directivity[m][n]);
        }
      printf ("%2.4f\n", directivity[m][ELEVATION_CARDINALITY - 1]);
    }
  std::cout << std::endl;
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
      std::cout << "Number of Elements          = " << antennaConfig->elements << std::endl;
      std::cout << "Antenna Orientation         = " << antennaConfig->azimuthOrientationDegree << std::endl;
      std::cout << "Amplitude Quantization Bits = " << uint16_t (antennaConfig->amplitudeQuantizationBits) << std::endl;
      std::cout << "Phase Quantization Bits     = " << uint16_t (antennaConfig->phaseQuantizationBits) << std::endl;
      std::cout << "Number of Sectors           = " << antennaConfig->sectorList.size () << std::endl;
      std::cout << "Quasi-Omni Directivity:" << std::endl;
      PrintDirectivity (antennaConfig->quasiOmniDirectivity);
      for (SectorListI sectorIter = antennaConfig->sectorList.begin ();
           sectorIter != antennaConfig->sectorList.end (); sectorIter++)
        {
          sectorConfig = DynamicCast<ParametricSectorConfig> (sectorIter->second);
          std::cout << "**********************************************************" << std::endl;
          std::cout << "Sector ID (" << uint16_t (sectorIter->first) << ")" << std::endl;
          std::cout << "**********************************************************" << std::endl;
          std::cout << "Sector Type             = " << sectorConfig->sectorType << std::endl;
          std::cout << "Sector Usage            = " << sectorConfig->sectorUsage << std::endl;
          std::cout << "Sector Directivity:" << std::endl;
          PrintDirectivity (sectorConfig->directivity);
          if (printAWVs)
            {
              for (uint8_t awvIndex = 0; awvIndex < sectorConfig->awvList.size (); awvIndex++)
                {
                  std::cout << "**********************************************************" << std::endl;
                  std::cout << "AWV ID (" << uint16_t (awvIndex) << ")" << std::endl;
                  std::cout << "**********************************************************" << std::endl;
                  PrintDirectivity (DynamicCast<Parametric_AWV_Config> (sectorConfig->awvList[awvIndex])->directivity);
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
              PrintDirectivity (DynamicCast<Parametric_AWV_Config> (sectorConfig->awvList[awvIndex])->directivity);
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
      antennaConfig->quasiOmniWeights = weightsVector;
      antennaConfig->CalculateDirectivity (&weightsVector, antennaConfig->quasiOmniArrayPattern, antennaConfig->quasiOmniDirectivity);
    }
  else
    {
      NS_ABORT_MSG ("Cannot find the specified antenna ID=" << static_cast<uint16_t> (antennaID));
    }
}

void
CodebookParametric::ChangeAntennaOrientation (AntennaID antennaID, double azimuthOrientation, double elevationOrientation)
{
  AntennaArrayListCI iter = m_antennaArrayList.find (antennaID);
  if (iter != m_antennaArrayList.end ())
    {
      Ptr<ParametricAntennaConfig> antennaConfig = StaticCast<ParametricAntennaConfig> (iter->second);
      antennaConfig->azimuthOrientationDegree = azimuthOrientation;
      antennaConfig->elevationOrientationDegree = elevationOrientation;

      antennaConfig->CalculateDirectivity (&antennaConfig->quasiOmniWeights,
                                           antennaConfig->quasiOmniArrayPattern, antennaConfig->quasiOmniDirectivity);

      for (SectorListI sectorIter = antennaConfig->sectorList.begin ();
           sectorIter != antennaConfig->sectorList.end (); sectorIter++)
        {
          Ptr<ParametricSectorConfig> sectorConfig = DynamicCast<ParametricSectorConfig> (sectorIter->second);
          antennaConfig->CalculateDirectivity (&sectorConfig->elementsWeights,
                                               sectorConfig->arrayPattern, sectorConfig->directivity);

          for (AWV_LIST_I awvIter = sectorConfig->awvList.begin (); awvIter != sectorConfig->awvList.end (); awvIter++)
            {
              Ptr<Parametric_AWV_Config> awvConfig = DynamicCast<Parametric_AWV_Config> (*awvIter);
              antennaConfig->CalculateDirectivity (&awvConfig->elementsWeights,
                                                   awvConfig->arrayPattern, awvConfig->directivity);
            }
        }
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
        }
      if ((sectorConfig->sectorUsage == SLS_SECTOR) || (sectorConfig->sectorUsage == BHI_SLS_SECTOR))
        {
          if ((sectorConfig->sectorType == TX_SECTOR) || (sectorConfig->sectorType == TX_RX_SECTOR))
            {
              m_totalTxSectors++;
            }
          if ((sectorConfig->sectorType == RX_SECTOR) || (sectorConfig->sectorType == TX_RX_SECTOR))
            {
              m_totalRxSectors++;
            }
        }


      sectorConfig->elementsWeights = weightsVector;

      SectorListI sectorIter = antennaConfig->sectorList.find (sectorID);
      if (sectorIter != antennaConfig->sectorList.end ())
        {
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
          awvConfig->elementsWeights = weightsVector;
          antennaConfig->CalculateDirectivity (&sectorConfig->elementsWeights, awvConfig->arrayPattern, awvConfig->directivity);
          sectorConfig->awvList.push_back (awvConfig);

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
          if (steeringAngleElevation < 0)
            {
              steeringAngleElevation += 180;
            }
          uint azimuth = steeringAngleAzimuth;
          uint elevation = steeringAngleElevation;
          WeightsVector weightsVector;
          for (uint16_t i = 0; i < antennaConfig->elements; i++)
            {
              weightsVector.push_back (std::conj (antennaConfig->steeringVector [azimuth][elevation][i]));
            }
          awvConfig->elementsWeights = weightsVector;
          antennaConfig->CalculateDirectivity (&awvConfig->elementsWeights, awvConfig->arrayPattern, awvConfig->directivity);
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

uint16_t
CodebookParametric::GetNumberOfElements (AntennaID antennaID) const
{
  AntennaArrayListCI iter = m_antennaArrayList.find (antennaID);
  if (iter != m_antennaArrayList.end ())
    {
      Ptr<ParametricAntennaConfig> antennaConfig = StaticCast<ParametricAntennaConfig> (iter->second);
      return antennaConfig->elements;
    }
  else
    {
      NS_ABORT_MSG ("Cannot find the specified Antenna ID=" << static_cast<uint16_t> (antennaID));
      return 0;
    }
}

ArrayPattern
CodebookParametric::GetTxAntennaArrayPattern (void)
{
  return DynamicCast<ParametricPatternConfig> (m_txPattern)->GetArrayPattern ();
}

ArrayPattern
CodebookParametric::GetRxAntennaArrayPattern (void)
{
  if (m_quasiOmniMode)
    {
      Ptr<ParametricAntennaConfig> antennaConfig = StaticCast<ParametricAntennaConfig> (m_antennaArrayList[m_antennaID]);
      return antennaConfig->GetQuasiOmniArrayPattern ();
    }
  else
    {
      return DynamicCast<ParametricPatternConfig> (m_rxPattern)->GetArrayPattern ();
    }
}

}
