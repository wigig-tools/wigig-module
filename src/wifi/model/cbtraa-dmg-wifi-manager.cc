/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */

#include "ns3/log.h"
#include "cbtraa-dmg-wifi-manager.h"
#include "wifi-phy.h"
#include "wifi-utils.h"

namespace ns3 {

/**
 * \brief hold per-remote-station state for Ideal Wifi manager.
 *
 * This struct extends from WifiRemoteStation struct to hold additional
 * information required by the Ideal Wifi manager
 */
struct CbtraaDmgWifiRemoteStation : public WifiRemoteStation
{
  double m_lastSnrCached;    //!< SNR most recently used to select a rate.
  WifiMode m_lastMode;       //!< Mode most recently used to the remote station.
};

/// To avoid using the cache before a valid value has been cached
static const double CACHE_INITIAL_VALUE = -100;

NS_OBJECT_ENSURE_REGISTERED (CbtraaDmgWifiManager);

NS_LOG_COMPONENT_DEFINE ("CbtraaDmgWifiManager");

TypeId
CbtraaDmgWifiManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CbtraaDmgWifiManager")
    .SetParent<WifiRemoteStationManager> ()
    .SetGroupName ("Wifi")
    .AddConstructor<CbtraaDmgWifiManager> ()
    .AddAttribute ("BerThreshold",
                   "The maximum Bit Error Rate acceptable at any transmission mode",
                   DoubleValue (1e-9),
                   MakeDoubleAccessor (&CbtraaDmgWifiManager::m_ber),
                   MakeDoubleChecker<double> ())
    .AddTraceSource ("Rate",
                     "Traced value for MCS changes",
                     MakeTraceSourceAccessor (&CbtraaDmgWifiManager::m_mcsChanged),
                     "ns3::CbtraaDmgWifiManager::McsChangedTracedCallback")
  ;
  return tid;
}

CbtraaDmgWifiManager::CbtraaDmgWifiManager ()
{
  NS_LOG_FUNCTION (this);
}

CbtraaDmgWifiManager::~CbtraaDmgWifiManager ()
{
  NS_LOG_FUNCTION (this);
}

void
CbtraaDmgWifiManager::DoInitialize ()
{
  NS_LOG_FUNCTION (this);
  WifiMode mode;
  WifiTxVector txVector;
  uint8_t nModes = GetPhy ()->GetNModes ();
  for (uint8_t i = 1; i < nModes; i++)
    {
      mode = GetPhy ()->GetMode (i);
      txVector.SetChannelWidth (GetPhy ()->GetChannelWidth ());
      txVector.SetMode (mode);
      NS_LOG_DEBUG ("Initialize, adding mode = " << mode.GetUniqueName ());
      AddSnrThreshold (txVector, RatioToDb (GetPhy ()->CalculateSnr (txVector, m_ber)));
    }
}

double
CbtraaDmgWifiManager::GetSnrThreshold (WifiTxVector txVector) const
{
  NS_LOG_FUNCTION (this << txVector.GetMode ().GetUniqueName ());
  for (Thresholds::const_iterator i = m_thresholds.begin (); i != m_thresholds.end (); i++)
    {
      NS_LOG_DEBUG ("Checking " << i->second.GetMode ().GetUniqueName ()
                    << " against TxVector " << txVector.GetMode ().GetUniqueName ());
      if (txVector.GetMode () == i->second.GetMode ())
        {
          return i->first;
        }
    }
  NS_ASSERT (false);
  return 0.0;
}

void
CbtraaDmgWifiManager::AddSnrThreshold (WifiTxVector txVector, double snr)
{
  NS_LOG_FUNCTION (this << txVector.GetMode ().GetUniqueName () << snr);
  m_thresholds.push_back (std::make_pair (snr, txVector));
}

WifiRemoteStation *
CbtraaDmgWifiManager::DoCreateStation (void) const
{
  NS_LOG_FUNCTION (this);
  CbtraaDmgWifiRemoteStation *station = new CbtraaDmgWifiRemoteStation ();
  station->m_lastSnrCached = CACHE_INITIAL_VALUE;
  station->m_lastMode = GetDefaultMode ();
  return station;
}

void
CbtraaDmgWifiManager::DoReportRxOk (WifiRemoteStation *station, double rxSnr, WifiMode txMode)
{
  NS_LOG_FUNCTION (this << station << rxSnr << txMode);
}

void
CbtraaDmgWifiManager::DoReportRtsFailed (WifiRemoteStation *station)
{
  NS_LOG_FUNCTION (this << station);
}

void
CbtraaDmgWifiManager::DoReportDataFailed (WifiRemoteStation *station)
{
  NS_LOG_FUNCTION (this << station);
}

void
CbtraaDmgWifiManager::DoReportRtsOk (WifiRemoteStation *st,
                                    double ctsSnr, WifiMode ctsMode, double rtsSnr)
{
  NS_LOG_FUNCTION (this << st << ctsSnr << ctsMode.GetUniqueName () << rtsSnr);
}

void
CbtraaDmgWifiManager::DoReportDataOk (WifiRemoteStation *st, double ackSnr, WifiMode ackMode,
                                      double dataSnr, uint16_t dataChannelWidth, uint8_t dataNss)
{
  NS_LOG_FUNCTION (this << st << ackSnr << ackMode.GetUniqueName () << dataSnr << dataChannelWidth << +dataNss);
}

void
CbtraaDmgWifiManager::DoReportAmpduTxStatus (WifiRemoteStation *st, uint8_t nSuccessfulMpdus,
                                            uint8_t nFailedMpdus, double rxSnr, double dataSnr, uint16_t dataChannelWidth, uint8_t dataNss)
{
  NS_LOG_FUNCTION (this << st << +nSuccessfulMpdus << +nFailedMpdus << rxSnr << dataSnr << dataChannelWidth << +dataNss);
}

void
CbtraaDmgWifiManager::DoReportFinalRtsFailed (WifiRemoteStation *station)
{
  NS_LOG_FUNCTION (this << station);
}

void
CbtraaDmgWifiManager::DoReportFinalDataFailed (WifiRemoteStation *station)
{
  NS_LOG_FUNCTION (this << station);
}

WifiTxVector
CbtraaDmgWifiManager::DoGetDataTxVector (WifiRemoteStation *st)
{
  NS_LOG_FUNCTION (this << st);
  CbtraaDmgWifiRemoteStation *station = (CbtraaDmgWifiRemoteStation *)st;
  //We search within the Supported rate set the mode with the
  //highest data rate for which the snr threshold is smaller than m_lastSnr
  //to ensure correct packet delivery.
  WifiMode maxMode = GetDefaultMode ();
  WifiTxVector txVector;
  WifiMode mode;
  uint64_t bestRate = 0;
  if (station->m_lastSnrCached != CACHE_INITIAL_VALUE && station->m_lastSnrCached == station->m_state->m_linkSnr)
    {
      // SNR has not changed, so skip the search and use the last
      // mode selected
      maxMode = station->m_lastMode;
      NS_LOG_DEBUG ("Using cached mode = " << maxMode.GetUniqueName () <<
                    " link SNR " << station->m_state->m_linkSnr <<
                    " cached " << station->m_lastSnrCached);
    }
  else
    {
      for (uint8_t i = 1; i < GetNSupported (station); i++)
        {
          mode = GetSupported (station, i);
          txVector.SetMode (mode);
          txVector.SetChannelWidth (GetPhy ()->GetChannelWidth ());
          double threshold = GetSnrThreshold (txVector);
          uint64_t dataRate = mode.GetDmgDataRate ();
          NS_LOG_DEBUG ("mode = " << mode.GetUniqueName () <<
                        " threshold " << threshold  <<
                        " link SNR " << station->m_state->m_linkSnr);
          if (dataRate > bestRate && threshold < station->m_state->m_linkSnr)
            {
              NS_LOG_DEBUG ("Candidate mode = " << mode.GetUniqueName () <<
                            " data rate " << dataRate <<
                            " threshold " << threshold  <<
                            " link SNR " << station->m_state->m_linkSnr);
              bestRate = dataRate;
              maxMode = mode;
            }
        }
      NS_LOG_DEBUG ("Updating cached values for station to " <<  maxMode.GetUniqueName () << " SNR " << station->m_state->m_linkSnr);
      station->m_lastSnrCached = station->m_state->m_linkSnr;
      if (station->m_lastMode.GetMcsValue () != maxMode.GetMcsValue ())
        {
          NS_LOG_DEBUG ("New DMG MCS-" << +maxMode.GetMcsValue ());
          station->m_lastMode = maxMode;
          m_mcsChanged (station->m_state->m_address, maxMode.GetMcsValue ());
        }
    }
  NS_LOG_DEBUG ("Found maxMode: " << maxMode);
  return WifiTxVector (maxMode, GetDefaultTxPowerLevel (),
                       GetPreambleForTransmission (maxMode.GetModulationClass (), false, false),
                       GetPhy ()->GetChannelWidth (), GetAggregation (station));
}

WifiTxVector
CbtraaDmgWifiManager::DoGetRtsTxVector (WifiRemoteStation *st)
{
  NS_LOG_FUNCTION (this << st);
  CbtraaDmgWifiRemoteStation *station = (CbtraaDmgWifiRemoteStation *)st;
  //We search within the Basic rate set the mode with the highest
  //snr threshold possible which is smaller than m_lastSnr to
  //ensure correct packet delivery.
  double maxThreshold = 0.0;
  WifiTxVector txVector;
  WifiMode mode;
  WifiMode maxMode = GetDefaultMode ();
  //avoid to use legacy rate adaptation algorithms for IEEE 802.11n/ac/ax
  //RTS is sent in a legacy frame; RTS with HT/VHT/HE is not yet supported
  for (uint8_t i = 0; i < GetNBasicModes (); i++)
    {
      mode = GetBasicMode (i);
      txVector.SetMode (mode);
      txVector.SetChannelWidth (GetPhy ()->GetChannelWidth ());
      double threshold = GetSnrThreshold (txVector);
      if (threshold > maxThreshold && threshold < station->m_state->m_linkSnr)
        {
          maxThreshold = threshold;
          maxMode = mode;
        }
    }
  return WifiTxVector (maxMode, GetDefaultTxPowerLevel (),
                       GetPreambleForTransmission (maxMode.GetModulationClass (), false, false),
                       GetPhy ()->GetChannelWidth (), GetAggregation (station));
}

bool
CbtraaDmgWifiManager::IsLowLatency (void) const
{
  return true;
}

} //namespace ns3
