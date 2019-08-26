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

uint8_t
ParametricSectorConfig::GetTotalNumberOfAWVs (void) const
{
  return awvList.size ();
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

CodebookParametric::~CodebookParametric ()
{
  NS_LOG_FUNCTION (this);
  for (CodebookConfigI iter = m_antennaArrayList.begin (); iter != m_antennaArrayList.end (); iter++)
    {
      Ptr<ParametricAntennaConfig> antennaConfig = StaticCast<ParametricAntennaConfig> (iter->second);
      for (SectorListI sectorIter = antennaConfig->sectorList.begin ();
           sectorIter != antennaConfig->sectorList.end (); sectorIter++)
        {
          Ptr<ParametricSectorConfig> sectorConfig = StaticCast<ParametricSectorConfig> (sectorIter->second);
          delete[] sectorConfig->sectorDirectivity;
        }
      delete[] antennaConfig->quasiOmniDirectivity;

      for (int i = 0; i < antennaConfig->elements; i++)
        {
          delete[] antennaConfig->steeringVector[i];
        }
      delete[] antennaConfig->steeringVector;
    }
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
  std::istringstream split(line);
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
  NS_ASSERT_MSG (file.good (), " Codebook file not found");
  std::string line;

  uint8_t nSectors;
  AntennaID antennaID;
  SectorID sectorID;

  std::getline (file, line);
  m_totalAntennas = std::stod (line);

  for (uint8_t antennaIndex = 0; antennaIndex < m_totalAntennas; antennaIndex++)
    {
      Ptr<ParametricAntennaConfig> config = Create<ParametricAntennaConfig> ();
      SectorIDList bhiSectors, txBeamformingSectors, rxBeamformingSectors;

      std::getline (file, line);
      antennaID = std::stoul (line);

      std::getline (file, line);
      config->azimuthOrientationDegree = std::stod (line);

      config->orientation.x = 1;
      config->orientation.y = 0;
      config->orientation.z = 0;

      std::getline (file, line);
      config->elements = std::stod (line);

      std::getline (file, line);
      config->phaseQuantizationBits = std::stod (line);
      config->phaseQuantizationStepSize = 2*M_PI/(std::pow (2, config->phaseQuantizationBits));

      std::getline (file, line);
      config->amplitudeQuantizationBits = std::stod (line);

      std::string amp, phaseDelay;
      config->steeringVector = new Complex *[config->elements];
      for (int i = 0; i < config->elements; i++)
        {
          config->steeringVector[i] = new Complex[AZIMUTH_CARDINALITY];
        }
      for (uint16_t i = 0; i < AZIMUTH_CARDINALITY; i++)
        {
          std::getline (file, line);
          std::istringstream split(line);
          for (uint16_t j = 0; j < config->elements; j++)
            {
              getline (split, amp, ',');
              getline (split, phaseDelay, ',');
              config->steeringVector[j][i] = std::polar (std::stod (amp), std::stod (phaseDelay));
            }
        }

      config->quasiOmniWeights = ReadAntennaWeightsVector (file, config->elements);
      CalculateDirectivity (&config->quasiOmniWeights, config->steeringVector,
                            config->quasiOmniArrayFactor, config->quasiOmniDirectivity);

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

          sectorConfig->sectorWeights = ReadAntennaWeightsVector (file, config->elements);
          CalculateDirectivity (&sectorConfig->sectorWeights, config->steeringVector,
                                sectorConfig->sectorArrayFactor, sectorConfig->sectorDirectivity);
          config->sectorList[sectorID] = sectorConfig;
        }

      if (config->azimuthOrientationDegree != 0)
        {
          ChangeAntennaOrientation (antennaID, config->azimuthOrientationDegree);
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

      m_antennaArrayList[antennaID] = config;
    }

  file.close ();
}

uint8_t
CodebookParametric::GetNumberSectorsPerAntenna (AntennaID antennaID) const
{
  CodebookConfigCI iter = m_antennaArrayList.find (antennaID);
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
  NS_LOG_FUNCTION (this << angle);
  Ptr<ParametricAntennaConfig> antennaConfig = StaticCast<ParametricAntennaConfig> (m_antennaArrayList[m_antennaID]);
  Ptr<ParametricSectorConfig> sectorConfig = StaticCast<ParametricSectorConfig> (antennaConfig->sectorList[m_txSectorID]);
  if (m_useAWV)
    {
      return -1;
    }
  else
    {
      return GetGainDbi (angle, sectorConfig->sectorDirectivity);
    }
}

double
CodebookParametric::GetRxGainDbi (double angle)
{
  NS_LOG_FUNCTION (this << angle);
  Ptr<ParametricAntennaConfig> antennaConfig;
  DirectivityTable directivityTable;
  if (m_useAWV)
    {
//      ParametricSectorConfig *sectorConfig;
//      antennaConfig = static_cast<ParametricAntennaConfig *> (m_antennaArrayList[m_antennaID]);
//      sectorConfig = static_cast<ParametricSectorConfig *> (&antennaConfig->sectorList[m_rxSectorID]);
//      directivityTable = static_cast<ParametricAntennaConfig *> (sectorConfig->);
    }
  else
    {
      if (m_quasiOmniMode)
        {
          antennaConfig = StaticCast<ParametricAntennaConfig> (m_antennaArrayList[m_antennaID]);
          directivityTable = antennaConfig->quasiOmniDirectivity;
        }
      else
        {
          Ptr<ParametricSectorConfig> sectorConfig;
          antennaConfig = StaticCast<ParametricAntennaConfig> (m_antennaArrayList[m_antennaID]);
          sectorConfig = StaticCast<ParametricSectorConfig> (antennaConfig->sectorList[m_rxSectorID]);
          directivityTable = sectorConfig->sectorDirectivity;
        }
    }
  return GetGainDbi (angle, directivityTable);
}

double
CodebookParametric::GetGainDbi (double angle, DirectivityTable directivity) const
{
  NS_LOG_FUNCTION (this << angle);
  double gain;
  angle = RadiansToDegrees (angle);
  if (angle < 0)
    {
      angle += 360;
    }
  int x1 = floor (angle);
  int x2 = ceil (angle);
  if (x1 != x2)
    {
      NS_LOG_DEBUG ("Performing linear interpolation on sector directivity");
      Directivity g1 = directivity[x1];
      Directivity g2 = directivity[x2];
      gain = (((x2 - angle) / (x2 - x1)) * g1) + (((angle - x1) / (x2 - x1)) * g2);
    }
  else
    {
      NS_LOG_DEBUG ("No interpolation needed");
      gain = directivity[x1];
    }
  gain = 10.0 * std::log10 (gain);;
  NS_LOG_DEBUG ("Angle=" << angle << ", Gain[dBi]=" << gain);
  return gain;
}

void
CodebookParametric::UpdateSectorWeights (AntennaID antennaID, SectorID sectorID, WeightsVector &weightsVector)
{
  CodebookConfigCI iter = m_antennaArrayList.find (antennaID);
  if (iter != m_antennaArrayList.end ())
    {
      Ptr<ParametricAntennaConfig> antennaConfig = StaticCast<ParametricAntennaConfig> (iter->second);
      SectorListI sectorIter = antennaConfig->sectorList.find (sectorID);
      if (sectorIter != antennaConfig->sectorList.end ())
        {
          Ptr<ParametricSectorConfig> sectorConfig = StaticCast<ParametricSectorConfig> (sectorIter->second);
          sectorConfig->sectorWeights = weightsVector;
          CalculateDirectivity (&weightsVector, antennaConfig->steeringVector,
                                sectorConfig->sectorArrayFactor, sectorConfig->sectorDirectivity);
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
ParametricAntennaConfig::CalculateDirectivity (double angle, WeightsVector &weightsVector)
{
  Complex value = 0;
  uint16_t j = 0;
  uint16_t angleIndex = floor (angle);
  for (WeightsVectorCI it = weightsVector.begin (); it != weightsVector.end (); it++, j++)
    {
      value += (*it) * steeringVector[j][angleIndex];
    }
  return abs (value);
}

double
ParametricAntennaConfig::CalculateDirectivityForDirection (double angle)
{
  uint16_t angleIndex = floor (angle);
  WeightsVector weightsVector;
  double phaseShift, amp;
  Complex conjValue;
  for (uint16_t i = 0; i <= elements; i++)
    {
      conjValue = std::conj (steeringVector[i][angleIndex]);
      amp = std::abs (conjValue);
      phaseShift = phaseQuantizationStepSize * std::floor ((std::arg (conjValue) + M_PI)/phaseQuantizationStepSize);
      weightsVector.push_back (std::polar (amp, phaseShift));
    }
  return CalculateDirectivity (angle, weightsVector);
}

void
CodebookParametric::PrintDirectivity (DirectivityTable directivity) const
{
  for (uint16_t i = 0; i < AZIMUTH_CARDINALITY; i++)
    {
      printf ("%2.2f ", directivity[i]);
    }
  std::cout << std::endl;
}

void
CodebookParametric::PrintCodebookContent (void)
{
  Ptr<ParametricAntennaConfig> antennaConfig;
  Ptr<ParametricSectorConfig> sectorConfig;
  for (CodebookConfigI iter = m_antennaArrayList.begin (); iter != m_antennaArrayList.end (); iter++)
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
          sectorConfig = StaticCast<ParametricSectorConfig> (sectorIter->second);
          std::cout << "**********************************************************" << std::endl;
          std::cout << "Sector ID (" << uint16_t (sectorIter->first) << ")" << std::endl;
          std::cout << "**********************************************************" << std::endl;
          std::cout << "Sector Type             = " << sectorConfig->sectorType << std::endl;
          std::cout << "Sector Usage            = " << sectorConfig->sectorUsage << std::endl;
          std::cout << "Sector Directivity:" << std::endl;
          PrintDirectivity (sectorConfig->sectorDirectivity);
        }
    }
}

void
CodebookParametric::UpdateQuasiOmniWeights (AntennaID antennaID, WeightsVector &weightsVector)
{
  CodebookConfigCI iter = m_antennaArrayList.find (antennaID);
  if (iter != m_antennaArrayList.end ())
    {
      Ptr<ParametricAntennaConfig> antennaConfig = StaticCast<ParametricAntennaConfig> (iter->second);
      antennaConfig->quasiOmniWeights = weightsVector;
      CalculateDirectivity (&weightsVector, antennaConfig->steeringVector,
                            antennaConfig->quasiOmniArrayFactor, antennaConfig->quasiOmniDirectivity);
    }
  else
    {
      NS_ABORT_MSG ("Cannot find the specified antenna ID=" << static_cast<uint16_t> (antennaID));
    }
}

void
CodebookParametric::ChangeAntennaOrientation (AntennaID antennaID, double orientation)
{
  CodebookConfigCI iter = m_antennaArrayList.find (antennaID);
  if (iter != m_antennaArrayList.end ())
    {
      Ptr<ParametricAntennaConfig> antennaConfig = StaticCast<ParametricAntennaConfig> (iter->second);
      antennaConfig->azimuthOrientationDegree = orientation;
      for (uint16_t i = 0; i < antennaConfig->elements; i++)
        {
          std::rotate (antennaConfig->steeringVector[i],
                       antennaConfig->steeringVector[i] + uint (orientation),
                       antennaConfig->steeringVector[i] + AZIMUTH_CARDINALITY);
        }
      CalculateDirectivity (&antennaConfig->quasiOmniWeights, antennaConfig->steeringVector,
                            antennaConfig->quasiOmniArrayFactor, antennaConfig->quasiOmniDirectivity);
      for (SectorListI sectorIter = antennaConfig->sectorList.begin ();
           sectorIter != antennaConfig->sectorList.end (); sectorIter++)
        {
          Ptr<ParametricSectorConfig> sectorConfig = StaticCast<ParametricSectorConfig> (sectorIter->second);
          CalculateDirectivity (&sectorConfig->sectorWeights, antennaConfig->steeringVector,
                                sectorConfig->sectorArrayFactor, sectorConfig->sectorDirectivity);
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
  CodebookConfigCI iter = m_antennaArrayList.find (antennaID);
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

      sectorConfig->sectorWeights = weightsVector;

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
  CodebookConfigCI iter = m_antennaArrayList.find (antennaID);
  if (iter != m_antennaArrayList.end ())
    {
      Ptr<ParametricAntennaConfig> antennaConfig = StaticCast<ParametricAntennaConfig> (iter->second);
      SectorListI sectorIter = antennaConfig->sectorList.find (sectorID);
      if (sectorIter != antennaConfig->sectorList.end ())
        {
          Ptr<ParametricSectorConfig> sectorConfig = StaticCast<ParametricSectorConfig> (sectorIter->second);
          Ptr<Parametric_AWV_Config> awvConfig = Create<Parametric_AWV_Config> ();
          awvConfig->sectorWeights = weightsVector;
          CalculateDirectivity (&sectorConfig->sectorWeights, antennaConfig->steeringVector,
                                awvConfig->sectorArrayFactor, awvConfig->sectorDirectivity);
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
CodebookParametric::InitiateBRP (AntennaID antennaID, SectorID sectorID, BeamRefinementType type)
{
  Ptr<ParametricAntennaConfig> antennaConfig = StaticCast<ParametricAntennaConfig> (m_antennaArrayList[antennaID]);
  Ptr<ParametricSectorConfig> sectorConfig = StaticCast<ParametricSectorConfig> (antennaConfig->sectorList[sectorID]);
  NS_ASSERT_MSG (sectorConfig->awvList.size () % 4 == 0, "The number of AWVs should be multiple of 4.");
  m_useAWV = true;
  m_currentAwvList = &sectorConfig->awvList;
  m_currentAwvIter = m_currentAwvList->begin ();
}

bool
CodebookParametric::GetNextAWV (void)
{
  m_currentAwvIter = m_currentAwvIter++;
  if (m_currentAwvIter == m_currentAwvList->end ())
    {
      m_currentAwvIter = m_currentAwvList->begin ();
      return true;
    }
  return false;
}

void
CodebookParametric::UseLastTxSector (void)
{
  Ptr<ParametricAntennaConfig> antennaConfig = StaticCast<ParametricAntennaConfig> (m_antennaArrayList[m_antennaID]);
  Ptr<ParametricSectorConfig> sectorConfig = StaticCast<ParametricSectorConfig> (antennaConfig->sectorList[m_txSectorID]);
}

uint16_t
CodebookParametric::GetNumberOfElements (AntennaID antennaID) const
{
  CodebookConfigCI iter = m_antennaArrayList.find (antennaID);
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

ArrayFactor
CodebookParametric::GetAntennaArrayFactor (SectorID sectorID) const
{
  CodebookConfigCI iter = m_antennaArrayList.find (m_antennaID);
  Ptr<ParametricAntennaConfig> antennaConfig = StaticCast<ParametricAntennaConfig> (iter->second);
  SectorListI sectorIter = antennaConfig->sectorList.find (sectorID);
  Ptr<ParametricSectorConfig> sectorConfig = StaticCast<ParametricSectorConfig> (sectorIter->second);
  return sectorConfig->GetArrayFactor ();
}

ArrayFactor
CodebookParametric::GetAntennaArrayFactor (void) const
{
  CodebookConfigCI iter = m_antennaArrayList.find (m_antennaID);
  Ptr<ParametricAntennaConfig> antennaConfig = StaticCast<ParametricAntennaConfig> (iter->second);
  return antennaConfig->quasiOmniArrayFactor;
}

}
