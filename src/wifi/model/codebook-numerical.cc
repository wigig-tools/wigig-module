/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#include "ns3/log.h"
#include "ns3/string.h"
#include "codebook-numerical.h"

#include <fstream>
#include <string>
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CodebookNumerical");

NS_OBJECT_ENSURE_REGISTERED (CodebookNumerical);

Ptr<NumericalPatternConfig>
NumericalAntennaConfig::GetQuasiOmniConfig (void) const
{
  return DynamicCast<NumericalPatternConfig> (m_quasiOmniConfig);
}

TypeId
CodebookNumerical::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CodebookNumerical")
    .SetGroupName ("Wifi")
    .SetParent<Codebook> ()
    .AddConstructor<CodebookNumerical> ()
    .AddAttribute ("FileName",
                   "The name of the codebook file to load.",
                   StringValue (""),
                   MakeStringAccessor (&CodebookNumerical::SetCodebookFileName),
                   MakeStringChecker ())
  ;
  return tid;
}

CodebookNumerical::CodebookNumerical ()
{
  NS_LOG_FUNCTION (this);
}

void
CodebookNumerical::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  for (AntennaArrayListI iter = m_antennaArrayList.begin (); iter != m_antennaArrayList.end (); iter++)
    {
      Ptr<NumericalAntennaConfig> antennaConfig = StaticCast<NumericalAntennaConfig> (iter->second);
      for (SectorListI sectorIter = antennaConfig->sectorList.begin ();
           sectorIter != antennaConfig->sectorList.end (); sectorIter++)
        {
          Ptr<NumericalSectorConfig> sectorConfig = DynamicCast<NumericalSectorConfig> (sectorIter->second);
          delete[] sectorConfig->directivity;
        }
      delete[] antennaConfig->GetQuasiOmniConfig ()->directivity;
    }
  Codebook::DoDispose ();
}

CodebookNumerical::~CodebookNumerical ()
{
  NS_LOG_FUNCTION (this);
}

void
CodebookNumerical::SetCodebookFileName (std::string fileName)
{
  NS_LOG_FUNCTION (this << fileName);
  if (fileName != "")
    {
      m_fileName = fileName;
      LoadCodebook (m_fileName);
    }
}

void
CodebookNumerical::LoadCodebook (std::string filename)
{
  NS_LOG_FUNCTION (this << "Loading Numerical Codebook file " << filename);
  std::ifstream file;
  file.open (filename.c_str (), std::ifstream::in);
  NS_ASSERT_MSG (file.good (), " Codebook file not found");
  std::string line;

  RFChainID rfChainID;
  uint8_t totalRFChains;  /* The total number of RF Chains. */
  uint8_t nSectors;       /* The total number of sectors within a phased antenna array */
  AntennaID antennaID;
  SectorID sectorID;
  Directivity directivity;

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

  /* The first line determines the number of phased antenna arrays within the device */
  std::getline (file, line);
  m_totalAntennas = std::stod (line);

  for (uint8_t antennaIndex = 0; antennaIndex < m_totalAntennas; antennaIndex++)
    {
      Ptr<NumericalAntennaConfig> antennaConfig =  Create<NumericalAntennaConfig> ();
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

      /* Read phased antenna array orientation degree */
      std::getline (file, line);
      antennaConfig->azimuthOrientationDegree = std::stod (line);

      /* Read Quasi-omni sector directivity in dBi */
      Ptr<NumericalPatternConfig> quasiOmni = Create<NumericalPatternConfig> ();
      quasiOmni->directivity = new double[AZIMUTH_CARDINALITY];
      for (uint16_t i = 0; i < AZIMUTH_CARDINALITY; i++)
        {
          std::getline (file, line);
          directivity = std::stod (line);
          quasiOmni->directivity[i] = directivity;
        }
      antennaConfig->SetQuasiOmniConfig (quasiOmni);

      /* Read the number of sectorList within this antenna array */
      std::getline (file, line);
      nSectors = std::stoul (line);
      m_totalSectors += nSectors;

      for (uint8_t sector = 0; sector < nSectors; sector++)
        {
          Ptr<NumericalSectorConfig> sectorConfig = Create<NumericalSectorConfig> ();

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

          /* Read directivity in dBi */
          sectorConfig->directivity = new double[AZIMUTH_CARDINALITY];
          for (uint16_t i = 0; i < AZIMUTH_CARDINALITY; i++)
            {
              std::getline (file, line);
              directivity = std::stod (line);
              sectorConfig->directivity[i] = directivity;
            }

          antennaConfig->sectorList[sectorID] = sectorConfig;
        }

      /* Change antenna orientation in case it-is non-zero */
      if (antennaConfig->azimuthOrientationDegree != 0)
        {
          ChangeAntennaOrientation (antennaID, antennaConfig->azimuthOrientationDegree, 0);
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
}

uint8_t
CodebookNumerical::GetNumberSectorsPerAntenna (AntennaID antennaID) const
{
  AntennaArrayListCI iter = m_antennaArrayList.find (antennaID);
  if (iter != m_antennaArrayList.end ())
    {
      Ptr<NumericalAntennaConfig> sectorList = StaticCast<NumericalAntennaConfig> (iter->second);
      return sectorList->sectorList.size ();
    }
  else
    {
      NS_ABORT_MSG ("Cannot find the specified antenna ID=" << static_cast<uint16_t> (antennaID));
    }
}

double
CodebookNumerical::GetTxGainDbi (double angle)
{
  NS_LOG_FUNCTION (this << angle);
  return GetGainDbi (angle, DynamicCast<NumericalPatternConfig> (GetTxPatternConfig ())->directivity);
}

double
CodebookNumerical::GetRxGainDbi (double angle)
{
  NS_LOG_FUNCTION (this << angle);
  return GetGainDbi (angle, DynamicCast<NumericalPatternConfig> (GetRxPatternConfig ())->directivity);
}

double
CodebookNumerical::GetTxGainDbi (double azimuth, double elevation)
{
  return GetTxGainDbi (azimuth);
}

double
CodebookNumerical::GetRxGainDbi (double azimuth, double elevation)
{
  return GetRxGainDbi (azimuth);
}

double
CodebookNumerical::GetGainDbi (double angle, DirectivityTable directivity) const
{
  NS_LOG_FUNCTION (this << angle);
  double gain;  // retrieved gain value after any interpolation
  angle = RadiansToDegrees (angle);
  /* Convert to positive angle */
  if (angle < 0)
    {
      angle += 2 * 180;
    }
  int x1 = floor (angle);
  int x2 = ceil (angle);
  if (x1 != x2) // 1D linear interpolation (x)
    {
      NS_LOG_DEBUG ("Performing linear interpolation on sector directivity");
      Directivity g1 = directivity[x1];
      Directivity g2 = directivity[x2];
      gain = (((x2 - angle) / (x2 - x1)) * g1) + (((angle - x1) / (x2 - x1)) * g2);
    }
  else //x1 == x2; no interpolation needed
    {
      NS_LOG_DEBUG ("No interpolation needed");
      gain = directivity[x1];
    }
  gain = 10.0 * std::log10 (gain);;
  NS_LOG_DEBUG ("Angle=" << angle << ", Gain[dBi]=" << gain);
  return gain;
}

void
CodebookNumerical::ChangeAntennaOrientation (AntennaID antennaID, double orientation, double elevationOrientation)
{
  AntennaArrayListCI iter = m_antennaArrayList.find (antennaID);
  if (iter != m_antennaArrayList.end ())
    {
      Ptr<NumericalAntennaConfig> antennaConfig = StaticCast<NumericalAntennaConfig> (iter->second);
      antennaConfig->azimuthOrientationDegree = orientation;
      /* Rotate Quasi-omni directivity table */
      std::rotate (antennaConfig->GetQuasiOmniConfig ()->directivity,
                   antennaConfig->GetQuasiOmniConfig ()->directivity + uint (orientation),
                   antennaConfig->GetQuasiOmniConfig ()->directivity + AZIMUTH_CARDINALITY);
      /* Rotate each sector directivity table */
      Ptr<NumericalSectorConfig> sectorConfig;
      for (SectorListI sectorIter = antennaConfig->sectorList.begin ();
           sectorIter != antennaConfig->sectorList.end (); sectorIter++)
        {
          sectorConfig = DynamicCast<NumericalSectorConfig> (sectorIter->second);
          std::rotate (sectorConfig->directivity,
                       sectorConfig->directivity + uint (orientation),
                       sectorConfig->directivity + AZIMUTH_CARDINALITY);
        }
    }
  else
    {
      NS_ABORT_MSG ("Cannot find the specified Antenna ID=" << static_cast<uint16_t> (antennaID));
    }
}

}
