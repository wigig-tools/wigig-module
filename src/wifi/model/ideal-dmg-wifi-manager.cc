/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *         Hany Assasa <hany.assasa@gmail.com>
 */

#include "ns3/log.h"
#include "ideal-dmg-wifi-manager.h"
#include "wifi-phy.h"

namespace ns3 {

/**
 * \brief hold per-remote-station state for Ideal Wifi manager.
 *
 * This struct extends from WifiRemoteStation struct to hold additional
 * information required by the Ideal Wifi manager
 */
struct IdealDmgWifiRemoteStation : public WifiRemoteStation
{
  double m_lastSnrObserved;  //!< SNR of most recently reported packet sent to the remote station
  double m_lastSnrCached;    //!< SNR most recently used to select a rate
  WifiMode m_lastMode;       //!< Mode most recently used to the remote station
};

/// To avoid using the cache before a valid value has been cached
static const double CACHE_INITIAL_VALUE = -100;

NS_OBJECT_ENSURE_REGISTERED (IdealDmgWifiManager);

NS_LOG_COMPONENT_DEFINE ("IdealDmgWifiManager");

TypeId
IdealDmgWifiManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::IdealDmgWifiManager")
    .SetParent<WifiRemoteStationManager> ()
    .SetGroupName ("Wifi")
    .AddConstructor<IdealDmgWifiManager> ()
    .AddAttribute ("BerThreshold",
                   "The maximum Bit Error Rate acceptable at any transmission mode",
                   DoubleValue (1e-6),
                   MakeDoubleAccessor (&IdealDmgWifiManager::m_ber),
                   MakeDoubleChecker<double> ())
    .AddTraceSource ("Rate",
                     "Traced value for MCS changes",
                     MakeTraceSourceAccessor (&IdealDmgWifiManager::m_mcsChanged),
                     "ns3::IdealDmgWifiManager::McsChangedTracedCallback")
  ;
  return tid;
}

IdealDmgWifiManager::IdealDmgWifiManager ()
{
  NS_LOG_FUNCTION (this);
}

IdealDmgWifiManager::~IdealDmgWifiManager ()
{
  NS_LOG_FUNCTION (this);
}

void
IdealDmgWifiManager::SetupPhy (const Ptr<WifiPhy> phy)
{
  NS_LOG_FUNCTION (this << phy);
  WifiRemoteStationManager::SetupPhy (phy);
}

void
IdealDmgWifiManager::DoInitialize ()
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
      AddSnrThreshold (txVector, GetPhy ()->CalculateSnr (txVector, m_ber));
    }
}

double
IdealDmgWifiManager::GetSnrThreshold (WifiTxVector &txVector) const
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
IdealDmgWifiManager::AddSnrThreshold (WifiTxVector txVector, double snr)
{
  NS_LOG_FUNCTION (this << txVector.GetMode ().GetUniqueName () << snr);
  m_thresholds.push_back (std::make_pair (snr, txVector));
}

WifiRemoteStation *
IdealDmgWifiManager::DoCreateStation (void) const
{
  NS_LOG_FUNCTION (this);
  IdealDmgWifiRemoteStation *station = new IdealDmgWifiRemoteStation ();
  Reset (station);
  return station;
}

void
IdealDmgWifiManager::Reset (WifiRemoteStation *station) const
{
  NS_LOG_FUNCTION (this << station);
  IdealDmgWifiRemoteStation *st = static_cast<IdealDmgWifiRemoteStation*> (station);
  st->m_lastSnrObserved = 0.0;
  st->m_lastSnrCached = CACHE_INITIAL_VALUE;
  st->m_lastMode = GetDefaultMode ();
}

void
IdealDmgWifiManager::DoReportRxOk (WifiRemoteStation *station, double rxSnr, WifiMode txMode)
{
  NS_LOG_FUNCTION (this << station << rxSnr << txMode);
}

void
IdealDmgWifiManager::DoReportRtsFailed (WifiRemoteStation *station)
{
  NS_LOG_FUNCTION (this << station);
}

void
IdealDmgWifiManager::DoReportDataFailed (WifiRemoteStation *station)
{
  NS_LOG_FUNCTION (this << station);
}

void
IdealDmgWifiManager::DoReportRtsOk (WifiRemoteStation *st,
                                    double ctsSnr, WifiMode ctsMode, double rtsSnr)
{
  NS_LOG_FUNCTION (this << st << ctsSnr << ctsMode.GetUniqueName () << rtsSnr);
  IdealDmgWifiRemoteStation *station = static_cast<IdealDmgWifiRemoteStation*> (st);
  station->m_lastSnrObserved = rtsSnr;
}

void
IdealDmgWifiManager::DoReportDataOk (WifiRemoteStation *st, double ackSnr, WifiMode ackMode,
                                     double dataSnr, uint16_t dataChannelWidth, uint8_t dataNss)
{
  NS_LOG_FUNCTION (this << st << ackSnr << ackMode.GetUniqueName () << dataSnr << dataChannelWidth << +dataNss);
  IdealDmgWifiRemoteStation *station = static_cast<IdealDmgWifiRemoteStation*> (st);
  if (dataSnr == 0)
    {
      NS_LOG_WARN ("DataSnr reported to be zero; not saving this report.");
      return;
    }
  station->m_lastSnrObserved = dataSnr;
}

void
IdealDmgWifiManager::DoReportAmpduTxStatus (WifiRemoteStation *st, uint8_t nSuccessfulMpdus,
                                            uint8_t nFailedMpdus, double rxSnr, double dataSnr, uint16_t dataChannelWidth, uint8_t dataNss)
{
  NS_LOG_FUNCTION (this << st << +nSuccessfulMpdus << +nFailedMpdus << rxSnr << dataSnr << dataChannelWidth << +dataNss);
  IdealDmgWifiRemoteStation *station = static_cast<IdealDmgWifiRemoteStation*> (st);
  if (dataSnr == 0)
    {
      NS_LOG_WARN ("DataSnr reported to be zero; not saving this report.");
      return;
    }
  station->m_lastSnrObserved = dataSnr;
}

void
IdealDmgWifiManager::DoReportFinalRtsFailed (WifiRemoteStation *station)
{
  NS_LOG_FUNCTION (this << station);
  Reset (station);
  m_mcsChanged (station->m_state->m_address, GetDefaultMode ().GetMcsValue ());
}

void
IdealDmgWifiManager::DoReportFinalDataFailed (WifiRemoteStation *station)
{
  NS_LOG_FUNCTION (this << station);
  Reset (station);
  m_mcsChanged (station->m_state->m_address, GetDefaultMode ().GetMcsValue ());
}

WifiTxVector
IdealDmgWifiManager::DoGetDataTxVector (WifiRemoteStation *st)
{
  NS_LOG_FUNCTION (this << st);
  IdealDmgWifiRemoteStation *station = static_cast<IdealDmgWifiRemoteStation*> (st);
  //We search within the Supported rate set the mode with the
  //highest data rate for which the SNR threshold is smaller than m_lastSnr
  //to ensure correct packet delivery.
  WifiMode maxMode = GetDefaultMode ();
  WifiTxVector txVector;
  WifiMode mode;
  uint64_t bestRate = 0;
  if (station->m_lastSnrCached != CACHE_INITIAL_VALUE && station->m_lastSnrObserved == station->m_lastSnrCached)
    {
      // SNR has not changed, so skip the search and use the last
      // mode selected
      maxMode = station->m_lastMode;
      NS_LOG_DEBUG ("Using cached mode = " << maxMode.GetUniqueName () <<
                    " last snr observed " << station->m_lastSnrObserved <<
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
                        " last snr observed " <<
                        station->m_lastSnrObserved);
          if (dataRate > bestRate && threshold < station->m_lastSnrObserved)
            {
              NS_LOG_DEBUG ("Candidate mode = " << mode.GetUniqueName () <<
                            " data rate " << dataRate <<
                            " threshold " << threshold  <<
                            " last snr observed " <<
                            station->m_lastSnrObserved);
              bestRate = dataRate;
              maxMode = mode;
            }
          NS_LOG_DEBUG ("Updating cached SNR value for station to " << station->m_lastSnrObserved);
          station->m_lastSnrCached = station->m_lastSnrObserved;
        }
      if (station->m_lastMode.GetMcsValue () != maxMode.GetMcsValue ())
        {
          NS_LOG_DEBUG ("Updating MCS value for station to " <<  maxMode.GetUniqueName ());
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
IdealDmgWifiManager::DoGetRtsTxVector (WifiRemoteStation *st)
{
  NS_LOG_FUNCTION (this << st);
  IdealDmgWifiRemoteStation *station = static_cast<IdealDmgWifiRemoteStation*> (st);
  //We search within the Basic rate set the mode with the highest
  //SNR threshold possible which is smaller than m_lastSnr to
  //ensure correct packet delivery.
  double maxThreshold = 0.0;
  WifiTxVector txVector;
  WifiMode mode;
  WifiMode maxMode = GetDefaultMode ();
  for (uint8_t i = 0; i < GetNBasicModes (); i++)
    {
      mode = GetBasicMode (i);
      txVector.SetMode (mode);
      txVector.SetChannelWidth (GetPhy ()->GetChannelWidth ());
      double threshold = GetSnrThreshold (txVector);
      if (threshold > maxThreshold && threshold < station->m_lastSnrObserved)
        {
          maxThreshold = threshold;
          maxMode = mode;
        }
    }
  return WifiTxVector (maxMode, GetDefaultTxPowerLevel (), GetPreambleForTransmission (maxMode.GetModulationClass (), false, false),
                       GetPhy ()->GetChannelWidth (), GetAggregation (station));
}

bool
IdealDmgWifiManager::IsLowLatency (void) const
{
  return true;
}

} //namespace ns3
