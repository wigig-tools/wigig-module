/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#include "ns3/enum.h"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "codebook-analytical.h"
#include <fstream>
#include <string>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CodebookAnalytical");

NS_OBJECT_ENSURE_REGISTERED (CodebookAnalytical);

TypeId
CodebookAnalytical::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CodebookAnalytical")
    .SetGroupName ("Wifi")
    .SetParent<Codebook> ()
    .AddConstructor<CodebookAnalytical> ()
    .AddAttribute ("FileName",
                   "The name of the codebook file to load.",
                   StringValue (""),
                   MakeStringAccessor (&CodebookAnalytical::SetCodebookFileName),
                   MakeStringChecker ())
    .AddAttribute ("Antennas", "The number of antenna arrays for the simple analytical codebook.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&CodebookAnalytical::m_antennas),
                   MakeUintegerChecker<uint8_t> (1, 4))
    .AddAttribute ("Sectors", "The number of sectors per antenna for the simple analytical codebook.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&CodebookAnalytical::m_sectors),
                   MakeUintegerChecker<uint8_t> (1, 64))
    .AddAttribute ("AWVs", "The number of custom AWVs per virtual sector.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&CodebookAnalytical::m_awvs),
                   MakeUintegerChecker<uint8_t> (0, 64))
    .AddAttribute ("OverlapPercentage", "The percentage of overlapping between AWVs of the same sector.",
                   DoubleValue (0.8),
                   MakeDoubleAccessor (&CodebookAnalytical::m_overlapPercentage),
                   MakeDoubleChecker<double> (0.5, 1))
    .AddAttribute ("CodebookType",
                   "The type of the analytical codebook.",
                   EnumValue (SIMPLE_CODEBOOK),
                   MakeEnumAccessor (&CodebookAnalytical::SetCodeBookType),
                   MakeEnumChecker (SIMPLE_CODEBOOK, "simple",
                                    CUSTOM_CODEBOOK, "custom",
                                    EMPTY_CODEBOOK, "empty"))
  ;
  return tid;
}

CodebookAnalytical::CodebookAnalytical ()
{
  NS_LOG_FUNCTION (this);
}

CodebookAnalytical::~CodebookAnalytical ()
{
  NS_LOG_FUNCTION (this);
}

void
CodebookAnalytical::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  Codebook::DoInitialize ();
}

void
CodebookAnalytical::SetCodebookFileName (std::string fileName)
{
  NS_LOG_FUNCTION (this << fileName);
  m_fileName = fileName;
}

void
CodebookAnalytical::SetCodeBookType (AnalyticalCodebookType type)
{
  NS_LOG_FUNCTION (this << type);
  if (type == SIMPLE_CODEBOOK)
    {
      CreateEquallySizedSectors (m_antennas, m_sectors, m_awvs);
    }
  else if (type == CUSTOM_CODEBOOK)
    {
      LoadCodebook (m_fileName);
    }
}

void
CodebookAnalytical::LoadCodebook (std::string filename)
{
  NS_LOG_FUNCTION (this << "Loading Analytical Codebook file " << filename);
  std::ifstream file;
  file.open (filename.c_str (), std::ifstream::in);
  NS_ASSERT_MSG (file.good (), " Codebook file not found");
  std::string line;

  uint8_t nSectors;     /* The total number of sectors within phased antenna array */
  AntennaID antennaID;
  SectorID sectorID;

  /* The first line determines the number of phased antenna arrays within the device */
  std::getline (file, line);
  m_totalAntennas = std::stod (line);

  for (uint8_t antennaIndex = 0; antennaIndex < m_totalAntennas; antennaIndex++)
    {
      Ptr<AnalyticalAntennaConfig> config = Create<AnalyticalAntennaConfig> ();
      SectorIDList bhiSectors, txBeamformingSectors, rxBeamformingSectors;

      /* Read phased antenna array ID */
      std::getline (file, line);
      antennaID = std::stod (line);

      /* Read phased antenna array orientation degree */
      std::getline (file, line);
      config->azimuthOrientationDegree = std::stod (line);

      /* Read phased antenna array quasi-omni gain */
      std::getline (file, line);
      config->quasiOmniGain = std::stod (line);

      /* Read number of sectors within this antenna array */
      std::getline (file, line);
      nSectors = std::stoul (line);
      m_totalSectors += nSectors;

      for (uint8_t sector = 0; sector < nSectors; sector++)
        {
          Ptr<AnalyticalSectorConfig> sectorConfig = Create<AnalyticalSectorConfig> ();

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

          /* Read Sector Steering Angle */
          std::getline (file, line);
          sectorConfig->steeringAngle = std::stod (line);

          /* Read Sector MainLobe Width */
          std::getline (file, line);
          sectorConfig->mainLobeBeamWidth = std::stod (line);

          /* Calculate Analytical Sector Parameters */
          SetPatternConfiguration (sectorConfig);

          config->sectorList[sectorID] = sectorConfig;
        }

      if (bhiSectors.size () > 0)
        m_bhiAntennaList[antennaID] = bhiSectors;

      if (txBeamformingSectors.size () > 0)
        m_txBeamformingSectors[antennaID] = txBeamformingSectors;

      if (rxBeamformingSectors.size () > 0)
        m_rxBeamformingSectors[antennaID] = rxBeamformingSectors;

      m_antennaArrayList[antennaID] = config;
    }

  /* Close the file */
  file.close ();
}

void
CodebookAnalytical::CreateEquallySizedSectors (uint8_t numberOfAntennas, uint8_t numberOfSectors, uint8_t numberOfAwvs)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (numberOfAntennas) << static_cast<uint16_t> (numberOfSectors)
                   << static_cast<uint16_t> (numberOfAwvs));

  NS_ASSERT_MSG ((1 <= numberOfAntennas) && (numberOfAntennas <= MAXIMUM_NUMBER_OF_ANTENNAS),
                 "The minimum number of antennas is 1 and the maximum number of antennas is limited to 4.");
  NS_ASSERT_MSG ((1 <= numberOfSectors) && (numberOfSectors <= MAXIMUM_SECTORS_PER_ANTENNA),
                 "The minimum number of sectors is 1 and the maximum number of sectors per antenna is limited to 64 sectors.");
  NS_ASSERT_MSG (numberOfAntennas * numberOfSectors <= MAXIMUM_NUMBER_OF_SECTORS,
                 "The maximum total number of sectors is limited to 128 sectors.");
  NS_ASSERT_MSG (numberOfAwvs % 4 == 0, "The number of AWVs [" << uint16_t (numberOfAwvs) <<  "] is not multiple of 4.");

  double sectorBeamWidth, antennaBeamWidth;

  m_antennaArrayList.clear ();
  m_totalAntennas = numberOfAntennas;
  m_totalTxSectors = numberOfAntennas * numberOfSectors;
  m_totalRxSectors = m_totalTxSectors;
  sectorBeamWidth =  2 * M_PI/m_totalTxSectors;
  antennaBeamWidth = 2 * M_PI/numberOfAntennas;

  /* Create single RF Chain */
  RFChainID rfID = 1;
  Ptr<RFChain> rfChainConfig = CreateObject<RFChain> ();

  for (AntennaID antennaID = 1; antennaID <= numberOfAntennas; antennaID++)
    {
      Ptr<AnalyticalAntennaConfig> antennaConfig = Create<AnalyticalAntennaConfig> ();
      SectorIDList bhiSectors, txBeamformingSectors, rxBeamformingSectors;

      antennaConfig->azimuthOrientationDegree = antennaBeamWidth * (antennaID - 1);
      antennaConfig->quasiOmniGain = 0;

      for (SectorID sectorID = 1; sectorID <= numberOfSectors; sectorID++)
        {
          Ptr<AnalyticalSectorConfig> sectorConfig = Create<AnalyticalSectorConfig> ();

          /* Specific Analytical Sector Configuration */
          sectorConfig->steeringAngle = sectorBeamWidth * (sectorID - 1);
          sectorConfig->mainLobeBeamWidth = sectorBeamWidth;
          SetPatternConfiguration (sectorConfig);

          /* General Sector Configuration */
          sectorConfig->sectorType = TX_RX_SECTOR;
          sectorConfig->sectorUsage = BHI_SLS_SECTOR;

          /* Add the current sector to all the exisiting beamforming lists */
          bhiSectors.push_back (sectorID);
          txBeamformingSectors.push_back (sectorID);
          rxBeamformingSectors.push_back (sectorID);

          /* Add custom AWVs within each sector */
//          double startAngle = sectorConfig->steeringAngle - sectorConfig->mainLobeBeamWidth/2;
//          double awvBeamWidth = sectorBeamWidth/numberOfAwvs;
//          for (uint8_t k = 0; k < numberOfAwvs; k++)
//            {
//              Ptr<Analytical_AWV_Config> awvConfig = Create<Analytical_AWV_Config> ();
//              awvConfig->mainLobeBeamWidth = awvBeamWidth;
//              awvConfig->steeringAngle = startAngle + awvConfig->mainLobeBeamWidth * k;
//              SetPatternConfiguration (awvConfig);
//              sectorConfig->awvList.push_back (awvConfig);
//            }

          double awvBeamWidth = sectorBeamWidth/(numberOfAwvs/2);
          for (uint8_t k = 1; k <= numberOfAwvs; k++)
            {
              Ptr<Analytical_AWV_Config> awvConfig = Create<Analytical_AWV_Config> ();
              awvConfig->mainLobeBeamWidth = awvBeamWidth;
              if (k == 1)
                {
                  awvConfig->steeringAngle = sectorConfig->steeringAngle;
                }
              else if (k <= numberOfAwvs/2)
                {
                  awvConfig->steeringAngle = sectorConfig->steeringAngle + (1 - m_overlapPercentage) * (k - 1) * awvBeamWidth/2;
                }
              else
                {
                  awvConfig->steeringAngle = sectorConfig->steeringAngle - (1- m_overlapPercentage) * (numberOfAwvs - k + 1) * awvBeamWidth/2;
                }
              SetPatternConfiguration (awvConfig);
              sectorConfig->awvList.push_back (awvConfig);
            }

          antennaConfig->sectorList[sectorID] = sectorConfig;
        }
      m_bhiAntennaList[antennaID] = bhiSectors;
      m_txBeamformingSectors[antennaID] = txBeamformingSectors;
      m_rxBeamformingSectors[antennaID] = rxBeamformingSectors;
      m_antennaArrayList[antennaID] = antennaConfig;
      rfChainConfig->ConnectPhasedAntennaArray (antennaID, antennaConfig);
      antennaConfig->rfChain = rfChainConfig;
    }
  m_rfChainList[rfID] = rfChainConfig;
}

void
CodebookAnalytical::AppendRFChain (RFChainID rfchainID)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (rfchainID));
  Ptr<RFChain> rfChain = Create<RFChain> ();
  m_rfChainList[rfchainID] = rfChain;
}

void
CodebookAnalytical::AppendAntenna (RFChainID rfchainID, AntennaID antennaID, double orientation, double quasiOmniGain)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (antennaID) << orientation << quasiOmniGain);

  NS_ASSERT_MSG ((1 <= antennaID) && (antennaID <= MAXIMUM_NUMBER_OF_ANTENNAS),
                 "The ID of the antenna should be between 1 and 4.");
  NS_ASSERT_MSG ((0 <= m_totalAntennas) && (m_totalAntennas < MAXIMUM_NUMBER_OF_ANTENNAS),
                 "The maximum number of antennas is limited to 4.");

  Ptr<AnalyticalAntennaConfig> antennaConfig = Create<AnalyticalAntennaConfig> ();
  antennaConfig->azimuthOrientationDegree = orientation;
  antennaConfig->quasiOmniGain = quasiOmniGain;
  m_antennaArrayList[antennaID] = antennaConfig;
  m_totalAntennas++;

  /* Connect to phase antenna array */
  Ptr<RFChain> rfChainConfig = m_rfChainList[rfchainID];
  rfChainConfig->ConnectPhasedAntennaArray (antennaID, antennaConfig);
  antennaConfig->rfChain = rfChainConfig;
}

void
CodebookAnalytical::AddSectorToBeamformingLists (AntennaID antennaID, SectorID sectorID, Ptr<SectorConfig> sectorConfig)
{
  if ((sectorConfig->sectorUsage == BHI_SECTOR) || (sectorConfig->sectorUsage == BHI_SLS_SECTOR))
    {
      m_bhiAntennaList[antennaID].push_back (sectorID);
    }
  if ((sectorConfig->sectorUsage == SLS_SECTOR) || (sectorConfig->sectorUsage == BHI_SLS_SECTOR))
    {
      if ((sectorConfig->sectorType == TX_SECTOR) || (sectorConfig->sectorType == TX_RX_SECTOR))
        {
          m_txBeamformingSectors[antennaID].push_back (sectorID);
          m_totalTxSectors++;
        }
      if ((sectorConfig->sectorType == RX_SECTOR) || (sectorConfig->sectorType == TX_RX_SECTOR))
        {
          m_rxBeamformingSectors[antennaID].push_back (sectorID);
          m_totalRxSectors++;
        }
    }
}

void
CodebookAnalytical::SetPatternConfiguration (Ptr<AnalyticalPatternConfig> patternConfig)
{
  NS_ASSERT_MSG (patternConfig->mainLobeBeamWidth > 0, "Main Lobe beam widt should be larger than zero.");
  patternConfig->halfPowerBeamWidth = GetHalfPowerBeamWidth (patternConfig->mainLobeBeamWidth);
  patternConfig->maxGain = GetMaxGainDbi (patternConfig->halfPowerBeamWidth);
  patternConfig->sideLobeGain = GetSideLobeGain (patternConfig->halfPowerBeamWidth);
}

void
CodebookAnalytical::AppendSector (AntennaID antennaID, SectorID sectorID, Ptr<AnalyticalSectorConfig> sectorConfig)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (antennaID) << static_cast<uint16_t> (sectorID));

  NS_ASSERT_MSG ((0 <= sectorConfig->steeringAngle) && (sectorConfig->steeringAngle <= 360),
                 "The steering angle is limited between 0 and 360 degrees.");
  NS_ASSERT_MSG ((0 <= sectorConfig->mainLobeBeamWidth) && (sectorConfig->mainLobeBeamWidth <= 360),
                 "The main lobe beamwidth is limited between 0 and 360 degrees.");

  AntennaArrayListI it = m_antennaArrayList.find (antennaID);
  if (it != m_antennaArrayList.end ())
    {
      Ptr<AnalyticalAntennaConfig> config = StaticCast<AnalyticalAntennaConfig> (it->second);
      uint8_t numberOfSectors = config->sectorList.size ();

      NS_ASSERT_MSG ((1 <= sectorID) && (sectorID <= MAXIMUM_SECTORS_PER_ANTENNA),
                     "The ID of the sector should be between 1 and 64.");
      NS_ASSERT_MSG (numberOfSectors < MAXIMUM_SECTORS_PER_ANTENNA,
                     "The maximum number of sectors per antenna is limited to maximum of 64 sectors.");
      NS_ASSERT_MSG (m_totalSectors < MAXIMUM_NUMBER_OF_SECTORS,
                     "The maximum total number of sectors is limited to 128 sectors.");

      /* Calculate Analytical Sector Parameters */
      SetPatternConfiguration (sectorConfig);

      /* Add the sector to the corresponding beamformnig lists */
      AddSectorToBeamformingLists (antennaID, sectorID, sectorConfig);

      config->sectorList[sectorID] = sectorConfig;
      m_totalSectors++;
    }
  else
    {
      NS_FATAL_ERROR ("Antenna [" << antennaID << "] does not exist");
    }
}

void
CodebookAnalytical::AppendSector (AntennaID antennaID, SectorID sectorID,
                                  double steeringAngle, double mainLobeBeamWidth,
                                  SectorType sectorType, SectorUsage sectorUsage)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (antennaID) << static_cast<uint16_t> (sectorID)
                   << steeringAngle << mainLobeBeamWidth
                   << static_cast<uint16_t> (sectorType) << static_cast<uint16_t> (sectorUsage));

  NS_ASSERT_MSG ((0 <= steeringAngle) && (steeringAngle <= 360), "The steering angle is limited between 0 and 360 degrees.");
  NS_ASSERT_MSG ((0 <= mainLobeBeamWidth) && (mainLobeBeamWidth <= 360), "The main lobe beamwidth is limited between 0 and 360 degrees.");

  AntennaArrayListCI it = m_antennaArrayList.find (antennaID);
  if (it != m_antennaArrayList.end ())
    {
      Ptr<AnalyticalAntennaConfig> config = StaticCast<AnalyticalAntennaConfig> (it->second);
      uint8_t numberOfSectors = config->sectorList.size ();

      NS_ASSERT_MSG ((1 <= sectorID) && (sectorID <= MAXIMUM_SECTORS_PER_ANTENNA),
                     "The ID of the sector should be between 1 and 64.");
      NS_ASSERT_MSG (numberOfSectors < MAXIMUM_SECTORS_PER_ANTENNA,
                     "The maximum number of sectors per antenna is limited to maximum of 64 sectors.");
      NS_ASSERT_MSG (m_totalSectors < MAXIMUM_NUMBER_OF_SECTORS,
                     "The maximum total number of sectors is limited to 128 sectors.");

      Ptr<AnalyticalSectorConfig> sectorConfig = Create<AnalyticalSectorConfig> ();
      sectorConfig->steeringAngle = DegreesToRadians (steeringAngle);
      sectorConfig->mainLobeBeamWidth = DegreesToRadians (mainLobeBeamWidth);
      sectorConfig->sectorType = sectorType;
      sectorConfig->sectorUsage = sectorUsage;

      /* Calculate Analytical Sector Parameters */
      SetPatternConfiguration (sectorConfig);

      /* Add the sector to the corresponding beamformnig lists */
      AddSectorToBeamformingLists (antennaID, sectorID, sectorConfig);

      config->sectorList[sectorID] = sectorConfig;
      m_totalSectors++;
    }
  else
    {
      NS_FATAL_ERROR ("Antenna [" << antennaID << "] does not exist");
    }
}

uint8_t
CodebookAnalytical::GetNumberSectorsPerAntenna (AntennaID antennaID) const
{
  AntennaArrayListCI iter = m_antennaArrayList.find (antennaID);
  if (iter != m_antennaArrayList.end ())
    {
      Ptr<AnalyticalAntennaConfig> antennaConfig = StaticCast<AnalyticalAntennaConfig> (iter->second);
      return antennaConfig->sectorList.size ();
    }
  else
    {
      NS_ABORT_MSG ("Cannot find the specified antenna ID=" << static_cast<uint16_t> (antennaID));
    }
}

double
CodebookAnalytical::GetTxGainDbi (double angle)
{
  NS_LOG_FUNCTION (this << angle);
  return GetGainDbi (angle, DynamicCast<AnalyticalPatternConfig> (GetTxPatternConfig ()));
}

double
CodebookAnalytical::GetRxGainDbi (double angle)
{
  NS_LOG_FUNCTION (this << angle);
  if (m_activeRFChain->IsQuasiOmniMode ())
    {
      return StaticCast<AnalyticalAntennaConfig> (GetAntennaArrayConfig ())->quasiOmniGain;
    }
  else
    {
      return GetGainDbi (angle, DynamicCast<AnalyticalPatternConfig> (GetRxPatternConfig ()));
    }
}

double
CodebookAnalytical::GetTxGainDbi (double azimuth, double elevation)
{
  return GetTxGainDbi (azimuth);
}

double
CodebookAnalytical::GetRxGainDbi (double azimuth, double elevation)
{
  return GetRxGainDbi (azimuth);
}

double
CodebookAnalytical::GetGainDbi (double angle, Ptr<AnalyticalPatternConfig> patternConfig)
{
  NS_LOG_FUNCTION (this << angle);
  Ptr<AnalyticalAntennaConfig> antennaConfig = StaticCast<AnalyticalAntennaConfig> (GetAntennaArrayConfig ());
  double gain;
  NS_LOG_DEBUG ("Angle=" << angle << ", MainLobeBeamWidth=" << patternConfig->mainLobeBeamWidth
                << ", azimuthOrientationDegree=" << antennaConfig->azimuthOrientationDegree
                << ", steeringAngle=" << patternConfig->steeringAngle);
  /* Do this shifting to deal with positive angles only */
  angle += patternConfig->mainLobeBeamWidth/2 - (antennaConfig->azimuthOrientationDegree + patternConfig->steeringAngle);
  if (angle < 0)
    {
      angle += M_PI * 2;
    }
  angle = fmod (angle, 2 * M_PI);

  if ((0 <= angle) && (angle <= patternConfig->mainLobeBeamWidth))
    {
      /* Calculate relative angle with respect to the current sector */
      double virtualAngle = std::abs (angle - patternConfig->mainLobeBeamWidth/2);
      NS_LOG_DEBUG ("VirtualAngle=" << virtualAngle);
      gain = patternConfig->maxGain - 3.01 * pow (2 * virtualAngle/patternConfig->halfPowerBeamWidth, 2);
    }
  else
    {
      gain = patternConfig->sideLobeGain;
    }
  NS_LOG_DEBUG ("Angle=" << angle << ", Gain[dBi]=" << gain);
  return gain;
}

double
CodebookAnalytical::GetHalfPowerBeamWidth (double mainLobeWidth) const
{
  NS_LOG_FUNCTION (this << mainLobeWidth);
  return mainLobeWidth/2.6;
}

double
CodebookAnalytical::GetMaxGainDbi (double halfPowerBeamWidth) const
{
  NS_LOG_FUNCTION (this << halfPowerBeamWidth);
  return 10 * log10 (pow (1.6162/sin (halfPowerBeamWidth / 2), 2));
}

double
CodebookAnalytical::GetSideLobeGain (double halfPowerBeamWidth) const
{
  NS_LOG_FUNCTION (this << halfPowerBeamWidth);
  return -0.4111 * log (halfPowerBeamWidth) - 10.597;
}

void
CodebookAnalytical::AppendListOfAWV (AntennaID antennaID, SectorID sectorID, uint8_t numberOfAWVs)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (antennaID) << static_cast<uint16_t> (sectorID)
                   << static_cast<uint16_t> (numberOfAWVs));
  NS_ASSERT_MSG (numberOfAWVs % 4 == 0, "The number of AWVs should be multiple of 4.");
  NS_ASSERT_MSG (numberOfAWVs <= 64, "The maximum number of AWVs is 64.");
  AntennaArrayListCI it = m_antennaArrayList.find (antennaID);
  if (it != m_antennaArrayList.end ())
    {
      Ptr<AnalyticalAntennaConfig> config = StaticCast<AnalyticalAntennaConfig> (it->second);
      SectorListCI sectorIt = config->sectorList.find (sectorID);
      if (sectorIt != config->sectorList.end ())
        {
          Ptr<AnalyticalSectorConfig> sectorConfig = DynamicCast<AnalyticalSectorConfig> (sectorIt->second);
          /* Add custom AWVs within each sector */
          double startAngle = sectorConfig->steeringAngle - sectorConfig->mainLobeBeamWidth/2;
          double awvBeamWidth = sectorConfig->mainLobeBeamWidth/m_awvs;
          for (uint8_t k = 0; k < numberOfAWVs; k++)
            {
              Ptr<Analytical_AWV_Config> awvConfig = Create<Analytical_AWV_Config> ();
              awvConfig->mainLobeBeamWidth = awvBeamWidth;
              awvConfig->steeringAngle = startAngle + awvConfig->mainLobeBeamWidth * k;
              SetPatternConfiguration (awvConfig);
              sectorConfig->awvList.push_back (awvConfig);
            }
        }
      else
        {
          NS_FATAL_ERROR ("Sector [" << sectorID << "] does not exist");
        }
    }
  else
    {
      NS_FATAL_ERROR ("Antenna [" << antennaID << "] does not exist");
    }
}

}
