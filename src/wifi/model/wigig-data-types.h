/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2020 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#ifndef WIGIG_DATA_TYPES_H
#define WIGIG_DATA_TYPES_H

#include <stdint.h>
#include <vector>

namespace ns3 {

typedef uint8_t RFChainID;                      //!< Typedef for Radio Frequency (RF) Chain ID in the Codebook.
typedef uint8_t AntennaID;                      //!< Typedef for Antenna Array ID in the Codebook.
typedef uint8_t SectorID;                       //!< Typedef for Sector ID within the Codebook.
typedef uint8_t AWV_ID;                         //!< Typedef for AWV ID.
typedef float Directivity;                      //!< Typedef for direcitvity gain for a certain angle.
typedef std::vector<AntennaID> AntennaList;     //!< Typedef for list of antennas IDs.
typedef AntennaID RX_ANTENNA_ID;
typedef AntennaID TX_ANTENNA_ID;
typedef AWV_ID AWV_ID_TX;
typedef AWV_ID AWV_ID_RX;

//// NINA ////
typedef std::pair<AntennaID, SectorID>            ANTENNA_CONFIGURATION;             //!< Typedef for antenna configuration (AntennaID, SectorID).
typedef std::pair<ANTENNA_CONFIGURATION, AWV_ID>  AWV_CONFIGURATION;                 //!< Typedef for antenna pattern Config ((AntennaID, SectorID), AWVID)
typedef std::vector<AWV_CONFIGURATION>            MIMO_AWV_CONFIGURATION;            //!< Typedef for a combination of antenna pattern configs used for MIMO
typedef std::vector<MIMO_AWV_CONFIGURATION>       MIMO_AWV_CONFIGURATIONS;           //!< Typedef for a vector of antenna combinations used for MIMO
//// NINA ////

typedef enum  {
  SERVICE_PERIOD_ALLOCATION = 0,
  CBAP_ALLOCATION = 1
} AllocationType;

typedef uint8_t AllocationID;                   //!< Typedef for the allocation ID.
typedef uint8_t NCB;                            //!< Typedef for number of bonded channels.

/* IEEE 802.11ad BRP PHY Parameters */
#define aBRPminSCblocks       18
#define aBRPTRNBlock          4992
#define aSCGILength           64
#define aBRPminOFDMblocks     20
#define aSCBlockSize          512
#define TRNUnit               NanoSeconds (ceil (aBRPTRNBlock * 0.57))
#define OFDMSCMin             (aBRPminSCblocks * aSCBlockSize + aSCGILength) * 0.57
#define OFDMBRPMin            aBRPminOFDMblocks * 242

/* DMG TRN parameters */
#define AGC_SF_DURATION       NanoSeconds (182)     /* AGC Subfield Duration. */
#define TRN_CE_DURATION       NanoSeconds (655)     /* TRN Channel Estimation Subfield Duration. */
#define TRN_SUBFIELD_DURATION NanoSeconds (364)     /* TRN Subfield Duration. */
#define TRN_UNIT_SIZE         4                     /* The number of TRN Subfield within TRN Unit. */

/* DMG MAC Parameters */
#define MAX_DMG_AMSDU_LENGTH                "7935"              /* The maximum length of an A-MSDU in an DMG MPDU is 7935 octets. */
#define MAX_DMG_AMPDU_LENGTH                "262143"            /* The maximum length of an A-MPDU in an DMG PPDU is 262,143 octets. */
#define WIGIG_OFDM_SUBCARRIER_SPACING       5156250             /* Subcarrier frequency spacing */
#define WIGIG_GUARD_BANDWIDTH               1980                /* The guard bandwidth in MHz for 2.16 GHz channel */

  /* EDMG Parameters */
#define EDMG_TRN_UNIT_SIZE                  10                  /* Number of TRN Subfields within an EDMG TRN Unit - for TRN-R fields. */
#define L_EDMG_Header_A1                    6                   /* The length of EDMG Header A1 in Bytes for EDMG Control. */
#define L_EDMG_Header_A2                    3                   /* The length of EDMG Header A2 in Bytes for EDMG Control. */
#define NO_AWV_ID                           255
#define MAX_EDMG_AMPDU_LENGTH               "4194303"           /* The maximum length of an A-MPDU in an EDMG PPDU is 4,194,303 octets. */

enum ChannelAggregation {
  NOT_AGGREGATE = 0,
  AGGREGATE = 1,
};

/**
 * Type of EDMG PHY transmit mask.
 */
enum EDMG_TRANSMIT_MASK {
  CH_BANDWIDTH_216 = 0,      /* For 2.16 GHz */
  CH_BANDWIDTH_432,      /* For 4.32 GHz */
  CH_BANDWIDTH_648,      /* For 6.48 GHz */
  CH_BANDWIDTH_864,      /* For 8.64 GHz */
  CH_BANDWIDTH_216_216,  /* For 2.16+2.16 GHz */
  CH_BANDWIDTH_432_432,  /* for 4.32+4.32 GHz */
};

enum EDMG_CHANNEL_ID {
  EDMG_CHANNEL_CH1 = 1,
  EDMG_CHANNEL_CH2 = 2,
  EDMG_CHANNEL_CH3 = 3,
  EDMG_CHANNEL_CH4 = 4,
  EDMG_CHANNEL_CH5 = 5,
  EDMG_CHANNEL_CH6 = 6,
  EDMG_CHANNEL_CH7 = 7,
  EDMG_CHANNEL_CH8 = 8,
};

struct EDMG_CHANNEL_CONFIG {
  uint8_t ch_bandwidth;               //!< Bitmap representing enabled channels.
  ChannelAggregation aggregation;     //!< Enumeration to indicate if we've aggregate or non-aggregate.
  uint8_t primayChannel;              //!< The primary 2.16 GHz channel number.
  uint16_t channelWidth;              //!< Thta bandwidth of the bonded channel in MHz.
  uint8_t NCB;                        //!< The number of bonded channels.
  EDMG_TRANSMIT_MASK mask;            //!< The EDMG transmit mask.
  uint8_t chNumber;                   //!< The channel number according to IEEE 802.11ay D5.0 Figure 28-7.
};

extern std::vector<EDMG_CHANNEL_CONFIG> edmgChannelConfigurations;

/**
 * Find a WiGig channel configuration corresponding to the given parameters.
 * \param primaryCh
 * \param ch_bandwidth
 */
EDMG_CHANNEL_CONFIG FindChannelConfiguration (uint8_t primaryCh, uint8_t ch_bandwidth);
/**
 * Find a WiGig channel configuration corresponding to the given channel number.
 * This is not a one-to-one mapping but rather the function returns the
 * first channel configuration that matches the given channel number.
 * \param channelNumber The channel number.
 */
EDMG_CHANNEL_CONFIG FindChannelConfiguration (uint8_t channelNumber);

}

#endif // WIGIG_DATA_TYPES_H
