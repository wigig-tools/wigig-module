/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 CTTC
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
 * Authors: Nicola Baldo <nbaldo@cttc.es>
 *          Ghada Badawy <gbadawy@gmail.com>
 *          Hany Assasa <hany.assasa@gmail.com>
 */

#include "wifi-tx-vector.h"

namespace ns3 {

WifiTxVector::WifiTxVector ()
  : m_preamble (WIFI_PREAMBLE_LONG),
    m_channelWidth (20),
    m_guardInterval (800),
    m_nTx (1),
    m_nss (1),
    m_ness (0),
    m_aggregation (false),
    m_stbc (false),
    m_bssColor (0),
    m_modeInitialized (false)
{
  DoInitialize ();
}

WifiTxVector::WifiTxVector (WifiMode mode,
                            uint8_t powerLevel,
                            WifiPreamble preamble,
                            uint16_t channelWidth,
                            bool aggregation)
  : m_mode (mode),
    m_txPowerLevel (powerLevel),
    m_preamble (preamble),
    m_channelWidth (channelWidth),
    m_guardInterval (800),
    m_nTx (1),
    m_nss (1),
    m_ness (0),
    m_aggregation (aggregation),
    m_stbc (false),
    m_bssColor (0),
    m_modeInitialized (true)
{
  DoInitialize ();
}

WifiTxVector::WifiTxVector (WifiMode mode,
                            uint8_t powerLevel,
                            WifiPreamble preamble,
                            uint16_t guardInterval,
                            uint8_t nTx,
                            uint8_t nss,
                            uint8_t ness,
                            uint16_t channelWidth,
                            bool aggregation,
                            bool stbc,
                            uint8_t bssColor)
  : m_mode (mode),
    m_txPowerLevel (powerLevel),
    m_preamble (preamble),
    m_channelWidth (channelWidth),
    m_guardInterval (guardInterval),
    m_nTx (nTx),
    m_nss (nss),
    m_ness (ness),
    m_aggregation (aggregation),
    m_stbc (stbc),
    m_bssColor (bssColor),
    m_modeInitialized (true)
{
  DoInitialize ();
}

void
WifiTxVector::DoInitialize (void)
{
  m_traingFieldLength = 0;
  m_beamTrackingRequest = false;
  m_lastRssi = 0;
  m_numSts = 1;
  m_numUsers = 1;
  m_guardIntervalType = GI_NORMAL;
  m_chBandwidth = 1;    // This corresponds to EDMG Channel 2 with 2.16 GHz bandwidth.
  m_NCB = 1;
  m_chAggregation = false;
  m_nTxChains = 1;
  m_shortLongLDCP = false;
  m_edmgTrnLength = 0;
  m_TrnSeqLen = TRN_SEQ_LENGTH_NORMAL;
  m_isDMGBeacon = false;
  m_trnRxPattern = QUASI_OMNI;
  m_isControlTrailerPresent = false;
}

bool
WifiTxVector::GetModeInitialized (void) const
{
  return m_modeInitialized;
}

WifiMode
WifiTxVector::GetMode (void) const
{
  if (!m_modeInitialized)
    {
      NS_FATAL_ERROR ("WifiTxVector mode must be set before using");
    }
  return m_mode;
}

uint8_t
WifiTxVector::GetTxPowerLevel (void) const
{
  return m_txPowerLevel;
}

WifiPreamble
WifiTxVector::GetPreambleType (void) const
{
  return m_preamble;
}

uint16_t
WifiTxVector::GetChannelWidth (void) const
{
  return m_channelWidth;
}

uint16_t
WifiTxVector::GetGuardInterval (void) const
{
  return m_guardInterval;
}

uint8_t
WifiTxVector::GetNTx (void) const
{
  return m_nTx;
}

uint8_t
WifiTxVector::GetNss (void) const
{
  return m_nss;
}

uint8_t
WifiTxVector::GetNess (void) const
{
  return m_ness;
}

bool
WifiTxVector::IsAggregation (void) const
{
  return m_aggregation;
}

bool
WifiTxVector::IsStbc (void) const
{
  return m_stbc;
}

void
WifiTxVector::SetMode (WifiMode mode)
{
  m_mode = mode;
  m_modeInitialized = true;
}

void
WifiTxVector::SetTxPowerLevel (uint8_t powerlevel)
{
  m_txPowerLevel = powerlevel;
}

void
WifiTxVector::SetPreambleType (WifiPreamble preamble)
{
  m_preamble = preamble;
}

void
WifiTxVector::SetChannelWidth (uint16_t channelWidth)
{
  m_channelWidth = channelWidth;
}

void
WifiTxVector::SetGuardInterval (uint16_t guardInterval)
{
  m_guardInterval = guardInterval;
}

void
WifiTxVector::SetNTx (uint8_t nTx)
{
  m_nTx = nTx;
}

void
WifiTxVector::SetNss (uint8_t nss)
{
  m_nss = nss;
}

void
WifiTxVector::SetNess (uint8_t ness)
{
  m_ness = ness;
}

void
WifiTxVector::SetAggregation (bool aggregation)
{
  m_aggregation = aggregation;
}

void
WifiTxVector::SetStbc (bool stbc)
{
  m_stbc = stbc;
}

void
WifiTxVector::SetBssColor (uint8_t color)
{
  m_bssColor = color;
}

uint8_t
WifiTxVector::GetBssColor (void) const
{
  return m_bssColor;
}

bool
WifiTxVector::IsValid (void) const
{
  if (!GetModeInitialized ())
    {
      return false;
    }
  std::string modeName = m_mode.GetUniqueName ();
  if (m_channelWidth == 20)
    {
      if (m_nss != 3 && m_nss != 6)
        {
          return (modeName != "VhtMcs9");
        }
    }
  else if (m_channelWidth == 80)
    {
      if (m_nss == 3 || m_nss == 7)
        {
          return (modeName != "VhtMcs6");
        }
      else if (m_nss == 6)
        {
          return (modeName != "VhtMcs9");
        }
    }
  else if (m_channelWidth == 160)
    {
      if (m_nss == 3)
        {
          return (modeName != "VhtMcs9");
        }
    }
  return true;
}

std::ostream & operator << ( std::ostream &os, const WifiTxVector &v)
{
  os << "mode: " << v.GetMode () <<
    " txpwrlvl: " << +v.GetTxPowerLevel () <<
    " preamble: " << v.GetPreambleType () <<
    " channel width: " << v.GetChannelWidth () <<
    " GI: " << v.GetGuardInterval () <<
    " NTx: " << +v.GetNTx () <<
    " Nss: " << +v.GetNss () <<
    " Ness: " << +v.GetNess () <<
    " MPDU aggregation: " << v.IsAggregation () <<
    " STBC: " << v.IsStbc ();
  return os;
}

//// WIGIG ////

void
WifiTxVector::SetPacketType (PacketType type)
{
  m_packetType = type;
}

PacketType
WifiTxVector::GetPacketType (void) const
{
  return m_packetType;
}

void
WifiTxVector::SetTrainngFieldLength (uint8_t length)
{
  m_traingFieldLength = length;
}

uint8_t
WifiTxVector::GetTrainngFieldLength (void) const
{
  return m_traingFieldLength;
}

void
WifiTxVector::SetEDMGTrainingFieldLength (uint8_t length)
{
  m_edmgTrnLength = length;
}

uint8_t
WifiTxVector::GetEDMGTrainingFieldLength (void) const
{
  return m_edmgTrnLength;
}

void
WifiTxVector::RequestBeamTracking (void)
{
  m_beamTrackingRequest = true;
}

bool
WifiTxVector::IsBeamTrackingRequested (void) const
{
  return m_beamTrackingRequest;
}

void
WifiTxVector::SetLastRssi (uint8_t level)
{
  m_lastRssi = level;
}

uint8_t
WifiTxVector::GetLastRssi (void) const
{
  return m_lastRssi;
}

void
WifiTxVector::Set_NUM_STS (uint8_t num)
{
  NS_ASSERT (num > 0 && num <= 8);
  m_numSts = num;
}

uint8_t
WifiTxVector::Get_NUM_STS (void) const
{
  return m_numSts;
}

uint8_t
WifiTxVector::Get_SC_EDMG_CEF (void) const
{
  if ((m_numSts == 1) || (m_numSts == 2))
    {
      return 1;
    }
  else if ((m_numSts == 3) || (m_numSts == 4))
    {
      return 2;
    }
  else
    {
      return 4;
    }
}

uint8_t
WifiTxVector::Get_OFDM_EDMG_CEF (void) const
{
  if ((m_numSts == 1) || (m_numSts == 2))
    {
      return 2;
    }
  else if ((m_numSts == 3) || (m_numSts == 4))
    {
      return m_numSts;
    }
  else if ((m_numSts == 5) || (m_numSts == 6))
    {
      return 6;
    }
  else
    {
      return 8;
    }
}
void
WifiTxVector::SetNumUsers (uint8_t num)
{
  m_numUsers = num;
}

uint8_t
WifiTxVector::GetNumUsers (void) const
{
  return m_numUsers;
}

void
WifiTxVector::SetGaurdIntervalType (GuardIntervalLength gi)
{
  m_guardIntervalType = gi;
}

GuardIntervalLength
WifiTxVector::GetGaurdIntervalType (void) const
{
  return m_guardIntervalType;
}

void
WifiTxVector::SetPrimaryChannelNumber (uint8_t primaryCh)
{
  m_primaryChannel = primaryCh;
}

uint8_t
WifiTxVector::GetPrimaryChannelNumber (void) const
{
  return m_primaryChannel;
}

void
WifiTxVector::SetChBandwidth (EDMG_CHANNEL_CONFIG chConfig)
{
  m_primaryChannel = chConfig.primayChannel;
  m_chBandwidth = chConfig.ch_bandwidth;
  m_NCB = chConfig.NCB;
  m_mask = chConfig.mask;
}

void
WifiTxVector::SetChannelConfiguration (uint8_t primaryCh, uint8_t bw)
{
  EDMG_CHANNEL_CONFIG chConfig = FindChannelConfiguration (primaryCh, bw);
  m_primaryChannel = primaryCh;
  m_channelWidth = chConfig.channelWidth;
  SetChBandwidth (chConfig);
}

uint8_t
WifiTxVector::GetChBandwidth (void) const
{
  return m_chBandwidth;
}

EDMG_TRANSMIT_MASK
WifiTxVector::GetTransmitMask (void) const
{
  return m_mask;
}

uint8_t
WifiTxVector::GetNCB (void) const
{
  return m_NCB;
}

void
WifiTxVector::SetChannelAggregation (bool chAggregation)
{
  m_chAggregation = chAggregation;
}

bool
WifiTxVector::GetChannelAggregation (void) const
{
  return m_chAggregation;
}

void
WifiTxVector::SetNumberOfTxChains (uint8_t nTxChains)
{
  m_nTxChains = nTxChains;
}

uint8_t
WifiTxVector::GetNumberOfTxChains (void) const
{
  return m_nTxChains;
}

void
WifiTxVector::SetLdcpCwLength (bool cwLength)
{
  m_shortLongLDCP = cwLength;
}

bool
WifiTxVector::GetLdcpCwLength (void) const
{
  return m_shortLongLDCP;
}
void
WifiTxVector::Set_TRN_SEQ_LEN (TRN_SEQ_LENGTH number)
{
  m_TrnSeqLen = number;
}

TRN_SEQ_LENGTH
WifiTxVector::Get_TRN_SEQ_LEN (void) const
{
  return m_TrnSeqLen;
}

void
WifiTxVector::Set_EDMG_TRN_P (uint8_t number)
{
  m_edmgTrnP = number;
}

uint8_t
WifiTxVector::Get_EDMG_TRN_P (void) const
{
  return m_edmgTrnP;
}

void
WifiTxVector::Set_EDMG_TRN_M (uint8_t number)
{
  m_edmgTrnM = number;
}

uint8_t
WifiTxVector::Get_EDMG_TRN_M (void) const
{
  return m_edmgTrnM;
}

void
WifiTxVector::Set_EDMG_TRN_N (uint8_t number)
{
  m_edmgTrnN = number;
}

uint8_t
WifiTxVector::Get_EDMG_TRN_N (void) const
{
  return m_edmgTrnN;
}

uint8_t
WifiTxVector::Get_TRN_T (void) const
{
  if (m_TrnSeqLen == TRN_SEQ_LENGTH_NORMAL)
    {
      return 2;
    }
  else if (m_TrnSeqLen == TRN_SEQ_LENGTH_LONG)
    {
      return 1;
    }
  else if (m_TrnSeqLen == TRN_SEQ_LENGTH_SHORT)
    {
      return 4;
    }
  else
    NS_FATAL_ERROR ("Unvalid TRN Subfield Sequence Length");
}

void
WifiTxVector::Set_RxPerTxUnits (uint8_t number)
{
  m_rxPerTxUnits = number;
}

uint8_t
WifiTxVector::Get_RxPerTxUnits (void) const
{
  return m_rxPerTxUnits;
}

void
WifiTxVector::SetSender (Mac48Address sender)
{
  m_sender = sender;
}

Mac48Address
WifiTxVector::GetSender (void) const
{
  return m_sender;
}

void
WifiTxVector::SetDMGBeacon (bool beacon)
{
  m_isDMGBeacon = beacon;
}

bool
WifiTxVector::IsDMGBeacon (void) const
{
  return m_isDMGBeacon;
}

void
WifiTxVector::SetTrnRxPattern (rxPattern trnRxPattern)
{
  m_trnRxPattern = trnRxPattern;
}

rxPattern
WifiTxVector::GetTrnRxPattern (void) const
{
  return m_trnRxPattern;
}

void
WifiTxVector::SetBrpCdown (uint8_t brpCdown)
{
  m_brpCdown = brpCdown;
}

uint8_t
WifiTxVector::GetBrpCdown (void) const
{
  return m_brpCdown;
}

void
WifiTxVector::SetControlTrailerPresent (bool flag)
{
  m_isControlTrailerPresent = flag;
}

bool
WifiTxVector::IsControlTrailerPresent (void) const
{
  return m_isControlTrailerPresent;
}
//// WIGIG ////

} //namespace ns3
