/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Ghada Badawy <gbadawy@gmail.com>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 *
 * Ported from yans-wifi-phy.cc by several contributors starting
 * with Nicola Baldo and Dean Armstrong
 */

#include "ns3/wifi-spectrum-value-helper.h"
#include "ns3/abort.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "spectrum-dmg-wifi-phy.h"
#include "dmg-wifi-spectrum-phy-interface.h"
#include "wifi-utils.h"
#include "wifi-ppdu.h"
#include "wifi-psdu.h"
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SpectrumDmgWifiPhy");

NS_OBJECT_ENSURE_REGISTERED (SpectrumDmgWifiPhy);

DmgWifiSpectrumSignalParameters::DmgWifiSpectrumSignalParameters ()
{
  NS_LOG_FUNCTION (this);
}

DmgWifiSpectrumSignalParameters::DmgWifiSpectrumSignalParameters (const DmgWifiSpectrumSignalParameters& p)
  : SpectrumSignalParameters (p)
{
  NS_LOG_FUNCTION (this << &p);
  ppdu = p.ppdu;
  plcpFieldType = p.plcpFieldType;
  txVector = p.txVector;
  antennaId = p.antennaId;
  txPatternConfig = p.txPatternConfig;
}

Ptr<SpectrumSignalParameters>
DmgWifiSpectrumSignalParameters::Copy ()
{
  NS_LOG_FUNCTION (this);
  // Ideally we would use:
  //   return Copy<WifiSpectrumSignalParameters> (*this);
  // but for some reason it doesn't work. Another alternative is
  //   return Copy<WifiSpectrumSignalParameters> (this);
  // but it causes a double creation of the object, hence it is less efficient.
  // The solution below is copied from the implementation of Copy<> (Ptr<>) in ptr.h
  Ptr<DmgWifiSpectrumSignalParameters> wssp (new DmgWifiSpectrumSignalParameters (*this), false);
  return wssp;
}

TypeId
SpectrumDmgWifiPhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SpectrumDmgWifiPhy")
    .SetParent<DmgWifiPhy> ()
    .SetGroupName ("Wifi")
    .AddConstructor<SpectrumDmgWifiPhy> ()
    .AddAttribute ("DisableWifiReception", "Prevent Wi-Fi frame sync from ever happening",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SpectrumDmgWifiPhy::m_disableWifiReception),
                   MakeBooleanChecker ())
    .AddTraceSource ("SignalArrival",
                     "Signal arrival",
                     MakeTraceSourceAccessor (&SpectrumDmgWifiPhy::m_signalCb),
                     "ns3::SpectrumDmgWifiPhy::SignalArrivalCallback")
  ;
  return tid;
}

SpectrumDmgWifiPhy::SpectrumDmgWifiPhy ()
{
  NS_LOG_FUNCTION (this);
}

SpectrumDmgWifiPhy::~SpectrumDmgWifiPhy ()
{
  NS_LOG_FUNCTION (this);
}

void
SpectrumDmgWifiPhy::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_channel = 0;
  m_wifiSpectrumPhyInterface = 0;
  DmgWifiPhy::DoDispose ();
}

void
SpectrumDmgWifiPhy::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  DmgWifiPhy::DoInitialize ();
  // This connection is deferred until frequency and channel width are set
  if (m_channel && m_wifiSpectrumPhyInterface)
    {
      m_channel->AddRx (m_wifiSpectrumPhyInterface);
    }
  else
    {
      NS_FATAL_ERROR ("SpectrumDmgWifiPhy misses channel and WifiSpectrumPhyInterface objects at initialization time");
    }
}

Ptr<const SpectrumModel>
SpectrumDmgWifiPhy::GetRxSpectrumModel () const
{
  NS_LOG_FUNCTION (this);
  if (m_rxSpectrumModel)
    {
      return m_rxSpectrumModel;
    }
  else
    {
      if (GetFrequency () == 0)
        {
          NS_LOG_DEBUG ("Frequency is not set; returning 0");
          return 0;
        }
      else
        {
          uint16_t channelWidth = GetChannelWidth ();
          NS_LOG_DEBUG ("Creating spectrum model from frequency/width pair of (" << GetFrequency () << ", " << channelWidth << ")");
          m_rxSpectrumModel = WifiSpectrumValueHelper::GetSpectrumModel (GetFrequency (), channelWidth,
                                                                         WIGIG_OFDM_SUBCARRIER_SPACING, GetGuardBandwidth ());
        }
    }
  return m_rxSpectrumModel;
}

Ptr<Channel>
SpectrumDmgWifiPhy::GetChannel (void) const
{
  return m_channel;
}

void
SpectrumDmgWifiPhy::SetChannel (const Ptr<SpectrumChannel> channel)
{
  m_channel = channel;
}

void
SpectrumDmgWifiPhy::ResetSpectrumModel (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (IsInitialized (), "Executing method before run-time");
  uint16_t channelWidth = GetChannelWidth ();
  NS_LOG_DEBUG ("Run-time change of spectrum model from frequency/width pair of (" << GetFrequency () << ", " << channelWidth << ")");
  // Replace existing spectrum model with new one, and must call AddRx ()
  // on the SpectrumChannel to provide this new spectrum model to it
  m_rxSpectrumModel = WifiSpectrumValueHelper::GetSpectrumModel (GetFrequency (), channelWidth,
                                                                 WIGIG_OFDM_SUBCARRIER_SPACING, GetGuardBandwidth ());
  m_channel->AddRx (m_wifiSpectrumPhyInterface);
}

void
SpectrumDmgWifiPhy::SetChannelNumber (uint8_t nch)
{
  NS_LOG_FUNCTION (this << +nch);
  DmgWifiPhy::SetChannelNumber (nch);
  if (IsInitialized ())
    {
      ResetSpectrumModel ();
    }
}

void
SpectrumDmgWifiPhy::SetFrequency (uint16_t freq)
{
  NS_LOG_FUNCTION (this << freq);
  DmgWifiPhy::SetFrequency (freq);
  if (IsInitialized ())
    {
      ResetSpectrumModel ();
    }
}

void
SpectrumDmgWifiPhy::SetChannelWidth (uint16_t channelwidth)
{
  NS_LOG_FUNCTION (this << channelwidth);
  DmgWifiPhy::SetChannelWidth (channelwidth);
  if (IsInitialized ())
    {
      ResetSpectrumModel ();
    }
}

void
SpectrumDmgWifiPhy::ConfigureStandard (WifiPhyStandard standard)
{
  NS_LOG_FUNCTION (this << standard);
  DmgWifiPhy::ConfigureStandard (standard);
  if (IsInitialized ())
    {
      ResetSpectrumModel ();
    }
}

double
SpectrumDmgWifiPhy::FilterSignal (Ptr<SpectrumValue> filter, Ptr<SpectrumValue> receivedSignalPsd)
{
  SpectrumValue filteredSignal = (*filter) * (*receivedSignalPsd);
  // Add receiver antenna gain
  NS_LOG_DEBUG ("Signal power received (watts) before antenna gain: " << Integral (filteredSignal));
  double rxPowerW = Integral (filteredSignal) * DbToRatio (GetRxGain ());
  NS_LOG_DEBUG ("Signal power received after antenna gain: " << rxPowerW << " W (" << WToDbm (rxPowerW) << " dBm)");
  return rxPowerW;
}

void
SpectrumDmgWifiPhy::StartRx (Ptr<SpectrumSignalParameters> rxParams)
{
  NS_LOG_FUNCTION (this << rxParams);
  Time rxDuration = rxParams->duration;
  Ptr<SpectrumValue> receivedSignalPsd = rxParams->psd;
  //NS_LOG_DEBUG ("Received signal with PSD " << *receivedSignalPsd << " and duration " << rxDuration.As (Time::NS));
  uint32_t senderNodeId = 0;
  if (rxParams->txPhy)
    {
      senderNodeId = rxParams->txPhy->GetDevice ()->GetNode ()->GetId ();
    }
  NS_LOG_DEBUG ("Received signal from " << senderNodeId << " with unfiltered power " << WToDbm (Integral (*receivedSignalPsd)) << " dBm");
  // Integrate over our receive bandwidth (i.e., all that the receive
  // spectral mask representing our filtering allows) to find the
  // total energy apparent to the "demodulator".
  uint16_t channelWidth = GetChannelWidth ();
  Ptr<SpectrumValue> filter = WifiSpectrumValueHelper::CreateRfFilter (GetFrequency (), channelWidth,
                                                                       WIGIG_OFDM_SUBCARRIER_SPACING, GetGuardBandwidth ());
  double rxPowerW;
  std::vector<double> rxPowerList;
  if (rxParams->psdList.size () > 0)
    {
      for (auto &psd : rxParams->psdList)
        {
         rxPowerList.push_back (FilterSignal (filter, psd));
        }
      rxPowerW = *std::max_element(rxPowerList.begin (), rxPowerList.end ());
    }
  else
    {
      rxPowerW = FilterSignal (filter, receivedSignalPsd);
      rxPowerList.push_back (rxPowerW);
    }

  /* MIMO calculation */
//  std::cout << "SISO: " << WToDbm (rxPowerW) << std::endl;
//  for (auto &psd : rxParams->psdList)
//    {
//      std::cout << "MIMO: " << WToDbm (FilterSignal (filter, psd)) << std::endl;
//    }

  Ptr<DmgWifiSpectrumSignalParameters> wifiRxParams = DynamicCast<DmgWifiSpectrumSignalParameters> (rxParams);

  // Log the signal arrival to the trace source
  m_signalCb (wifiRxParams ? true : false, senderNodeId, WToDbm (rxPowerW), rxDuration);

  // Do no further processing if signal is too weak
  // Current implementation assumes constant RX power over the PPDU duration
  if (WToDbm (rxPowerW) < GetRxSensitivity () && wifiRxParams->plcpFieldType != PLCP_80211AY_TRN_SF)
    {
      NS_LOG_INFO ("Received signal too weak to process: " << WToDbm (rxPowerW) << " dBm");
      return;
    }
  if (wifiRxParams == 0)
    {
      NS_LOG_INFO ("Received non Wi-Fi signal");
      m_interference.AddForeignSignal (rxDuration, rxPowerW);
      SwitchMaybeToCcaBusy ();
      return;
    }
  if (wifiRxParams && m_disableWifiReception)
    {
      NS_LOG_INFO ("Received Wi-Fi signal but blocked from syncing");
      m_interference.AddForeignSignal (rxDuration, rxPowerW);
      SwitchMaybeToCcaBusy ();
      return;
    }

  if ((wifiRxParams->plcpFieldType == PLCP_80211AD_PREAMBLE_HDR_DATA) || (wifiRxParams->plcpFieldType == PLCP_80211AY_PREAMBLE_HDR_DATA))
    {
      NS_LOG_INFO ("Received DMG/EDMG WiFi signal");
      Ptr<WifiPpdu> ppdu = Copy (wifiRxParams->ppdu);
      if (rxParams->psdList.size () > 0)
        {
          NS_LOG_INFO ("Received EDMG WiFi signal in MIMO mode");
          StartReceivePreamble (ppdu, rxPowerList);
         }
      else
        {
          NS_LOG_INFO ("Received EDMG WiFi signal in SISO mode");
          StartReceivePreamble (ppdu, rxPowerList);
        }
    }
  else if (wifiRxParams->plcpFieldType == PLCP_80211AD_AGC_SF)
    {
      NS_LOG_INFO ("Received DMG WiFi AGC-SF signal");
      StartReceiveAgcSubfield (wifiRxParams->txVector, WToDbm (rxPowerW));
    }
  else if (wifiRxParams->plcpFieldType == PLCP_80211AD_TRN_CE_SF)
    {
      NS_LOG_INFO ("Received DMG WiFi TRN-CE Subfield signal");
      StartReceiveCeSubfield (wifiRxParams->txVector, WToDbm (rxPowerW));
    }
  else if (wifiRxParams->plcpFieldType == PLCP_80211AD_TRN_SF)
    {
      NS_LOG_INFO ("Received DMG WiFi TRN-SF signal");
      StartReceiveTrnSubfield (wifiRxParams->txVector, WToDbm (rxPowerW));
    }
  else if (wifiRxParams->plcpFieldType == PLCP_80211AY_TRN_SF)
    {
      if (rxParams->psdList.size () > 0)
        {
          NS_LOG_INFO ("Received EDMG WiFi TRN-SF signal in MIMO mode");
          std::vector<double> rxPowerListDbm;
          for (auto &rxPower : rxPowerList)
            {
             rxPowerListDbm.push_back (WToDbm (rxPower));
            }
          StartReceiveEdmgTrnSubfield (wifiRxParams->txVector, rxPowerListDbm);
        }
      else
        {
          NS_LOG_INFO ("Received EDMG WiFi TRN-SF signal");
          StartReceiveEdmgTrnSubfield (wifiRxParams->txVector, WToDbm (rxPowerW));
        }
    }
}

Ptr<DmgWifiSpectrumPhyInterface>
SpectrumDmgWifiPhy::GetSpectrumPhy (void) const
{
  return m_wifiSpectrumPhyInterface;
}

void
SpectrumDmgWifiPhy::CreateWifiSpectrumPhyInterface (Ptr<NetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  m_wifiSpectrumPhyInterface = CreateObject<DmgWifiSpectrumPhyInterface> ();
  m_wifiSpectrumPhyInterface->SetSpectrumDmgWifiPhy (this);
  m_wifiSpectrumPhyInterface->SetDevice (device);
}

Ptr<SpectrumValue>
SpectrumDmgWifiPhy::GetTxPowerSpectralDensity (uint16_t centerFrequency, uint16_t channelWidth,
                                               double txPowerW, WifiModulationClass modulationClass) const
{
  NS_LOG_FUNCTION (centerFrequency << channelWidth << txPowerW << modulationClass);
  Ptr<SpectrumValue> v;
  switch (modulationClass)
    {
    case WIFI_MOD_CLASS_DMG_CTRL:
    case WIFI_MOD_CLASS_DMG_SC:
    case WIFI_MOD_CLASS_DMG_LP_SC:
      v = WifiSpectrumValueHelper::CreateWigigSingleCarrierTxPowerSpectralDensity (centerFrequency, channelWidth, txPowerW,
                                                                                   GetGuardBandwidth ());
      break;
    case WIFI_MOD_CLASS_EDMG_CTRL:
    case WIFI_MOD_CLASS_EDMG_SC:
      v = WifiSpectrumValueHelper::CreateWigigSingleCarrierTxPowerSpectralDensity (centerFrequency, channelWidth, txPowerW,
                                                                                   GetGuardBandwidth (), m_channelConfiguration.NCB);
      break;
    case WIFI_MOD_CLASS_DMG_OFDM:
    case WIFI_MOD_CLASS_EDMG_OFDM:
      // Create one different function for every different case
      v = WifiSpectrumValueHelper::CreateDmgOfdmTxPowerSpectralDensity (centerFrequency, channelWidth, txPowerW,
                                                                        GetGuardBandwidth ());
      break;
    default:
      NS_FATAL_ERROR ("modulation class unknown: " << modulationClass);
      break;
    }
  return v;
}

uint16_t
SpectrumDmgWifiPhy::GetCenterFrequencyForChannelWidth (WifiTxVector txVector) const
{
  NS_LOG_FUNCTION (this << txVector);
  uint16_t centerFrequencyForSupportedWidth = GetFrequency ();
  uint16_t supportedWidth = GetChannelWidth ();
  uint16_t currentWidth = txVector.GetChannelWidth ();
  if (currentWidth != supportedWidth)
    {
      uint16_t startingFrequency = centerFrequencyForSupportedWidth - (supportedWidth / 2);
      return startingFrequency + (currentWidth / 2); // primary channel is in the lower part (for the time being)
    }
  return centerFrequencyForSupportedWidth;
}

void
SpectrumDmgWifiPhy::StartTx (Ptr<WifiPpdu> ppdu)
{
  NS_LOG_FUNCTION (this << ppdu);
  WifiTxVector txVector = ppdu->GetTxVector ();
  double txPowerDbm = GetTxPowerForTransmission (txVector) + GetTxGain ();
  NS_LOG_DEBUG ("Start transmission: signal power before antenna array=" << txPowerDbm << "dBm");
  double txPowerWatts = DbmToW (txPowerDbm);
  //// WIGIG ////
  // The total transmit power is equally divided over the transmit chains.
  txPowerWatts /= GetCodebook ()->GetNumberOfActiveRFChains ();
  //// WIGIG ////
  Ptr<SpectrumValue> txPowerSpectrum = GetTxPowerSpectralDensity (GetCenterFrequencyForChannelWidth (txVector),
                                                                  txVector.GetChannelWidth (),
                                                                  txPowerWatts, txVector.GetMode ().GetModulationClass ());
  Ptr<DmgWifiSpectrumSignalParameters> txParams = Create<DmgWifiSpectrumSignalParameters> ();
  txParams->duration = ppdu->GetTxDuration ();
  txParams->psd = txPowerSpectrum;
  NS_ASSERT_MSG (m_wifiSpectrumPhyInterface, "SpectrumPhy() is not set; maybe forgot to call CreateWifiSpectrumPhyInterface?");
  txParams->txPhy = m_wifiSpectrumPhyInterface->GetObject<SpectrumPhy> ();
  txParams->ppdu = ppdu;
  if (GetStandard () == WIFI_PHY_STANDARD_80211ad)
    {
      txParams->plcpFieldType = PLCP_80211AD_PREAMBLE_HDR_DATA;
    }
  else
    {
      txParams->plcpFieldType = PLCP_80211AY_PREAMBLE_HDR_DATA;
    }
  txParams->txVector = txVector;
  txParams->antennaId = GetCodebook ()->GetActiveAntennaID ();
  txParams->txPatternConfig = GetCodebook ()->GetTxPatternConfig ();
  txParams->isMimo = (GetCodebook ()->GetNumberOfActiveRFChains () > 1);
  NS_LOG_DEBUG ("Starting transmission with power " << WToDbm (txPowerWatts) << " dBm on channel " << +GetChannelNumber ());
  NS_LOG_DEBUG ("Starting transmission with integrated spectrum power " << WToDbm (Integral (*txPowerSpectrum)) << " dBm; spectrum model Uid: " << txPowerSpectrum->GetSpectrumModel ()->GetUid ());
  m_channel->StartTx (txParams);
}

void
SpectrumDmgWifiPhy::TxSubfield (WifiTxVector txVector, PLCP_FIELD_TYPE fieldType, Time txDuration)
{
  NS_LOG_DEBUG ("Start transmission: signal power before antenna gain=" << GetPowerDbm (txVector.GetTxPowerLevel ()) << "dBm");
  double txPowerWatts = DbmToW (GetPowerDbm (txVector.GetTxPowerLevel ()) + GetTxGain ());
  Ptr<SpectrumValue> txPowerSpectrum = GetTxPowerSpectralDensity (GetCenterFrequencyForChannelWidth (txVector), txVector.GetChannelWidth (), txPowerWatts, txVector.GetMode ().GetModulationClass ());
  Ptr<DmgWifiSpectrumSignalParameters> txParams = Create<DmgWifiSpectrumSignalParameters> ();
  txParams->duration = txDuration;
  txParams->psd = txPowerSpectrum;
  NS_ASSERT_MSG (m_wifiSpectrumPhyInterface, "SpectrumPhy() is not set; maybe forgot to call CreateWifiSpectrumPhyInterface?");
  txParams->txPhy = m_wifiSpectrumPhyInterface->GetObject<SpectrumPhy> ();
  txParams->plcpFieldType = fieldType;
  txParams->txVector = txVector;
  txParams->antennaId = GetCodebook ()->GetActiveAntennaID ();
  txParams->txPatternConfig = GetCodebook ()->GetTxPatternConfig ();
  txParams->isMimo = (GetCodebook ()->GetNumberOfActiveRFChains () > 1);
  NS_LOG_DEBUG ("Starting transmission with power " << WToDbm (txPowerWatts) << " dBm on channel " << +GetChannelNumber ());
  NS_LOG_DEBUG ("Starting transmission with integrated spectrum power " << WToDbm (Integral (*txPowerSpectrum)) << " dBm; spectrum model Uid: " << txPowerSpectrum->GetSpectrumModel ()->GetUid ());
  m_channel->StartTx (txParams);
}

void
SpectrumDmgWifiPhy::StartAgcSubfieldTx (WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this);
  TxSubfield (txVector, PLCP_80211AD_AGC_SF, AGC_SF_DURATION);
}

void
SpectrumDmgWifiPhy::StartCeSubfieldTx (WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this);
  TxSubfield (txVector, PLCP_80211AD_TRN_CE_SF, TRN_CE_DURATION);
}

void
SpectrumDmgWifiPhy::StartTrnSubfieldTx (WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this);
  TxSubfield (txVector, PLCP_80211AD_TRN_SF, TRN_SUBFIELD_DURATION);
}

void
SpectrumDmgWifiPhy::StartEdmgTrnSubfieldTx (WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this);
  //txVector.edmgTrnSubfieldDuration = NanoSeconds (double (0.57) * 6 * txVector.Get_TRN_SEQ_LEN ());
  TxSubfield (txVector, PLCP_80211AY_TRN_SF, txVector.edmgTrnSubfieldDuration);
}

uint16_t
SpectrumDmgWifiPhy::GetGuardBandwidth (void) const
{
  uint16_t guardBandwidth = WIGIG_GUARD_BANDWIDTH;
  if (m_standard == WIFI_PHY_STANDARD_80211ay)
    {
      guardBandwidth *= m_channelConfiguration.NCB;
    }
  return guardBandwidth;
}

} //namespace ns3
