/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "ns3/log.h"
#include "ns3/interference-helper.h"

#include "wifi-phy.h"
#include "sensitivity-model-60-ghz.h"
#include "sensitivity-lut.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SensitivityModel60GHz");

NS_OBJECT_ENSURE_REGISTERED (SensitivityModel60GHz);

TypeId
SensitivityModel60GHz::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SensitivityModel60GHz")
    .SetParent<ErrorRateModel> ()
    .AddConstructor<SensitivityModel60GHz> ()
  ;
  return tid;
}

SensitivityModel60GHz::SensitivityModel60GHz ()
{
}

double
SensitivityModel60GHz::GetChunkSuccessRate (WifiMode mode, WifiTxVector txVector, double snr, uint64_t nbits) const
{
  NS_ASSERT_MSG(mode.GetModulationClass () == WIFI_MOD_CLASS_DMG_CTRL ||
    mode.GetModulationClass () == WIFI_MOD_CLASS_DMG_SC ||
    mode.GetModulationClass () == WIFI_MOD_CLASS_DMG_OFDM ||
    mode.GetModulationClass () == WIFI_MOD_CLASS_EDMG_CTRL ||
    mode.GetModulationClass () == WIFI_MOD_CLASS_EDMG_SC ||
    mode.GetModulationClass () == WIFI_MOD_CLASS_EDMG_OFDM,
    "Expecting 802.11ad DMG CTRL, SC or OFDM modulation or 802.11ay EDMG CTRL, SC or OFDM modulation");
  std::string modename = mode.GetUniqueName ();

  /* This is kinda silly, but convert from SNR back to RSS (Hardcoding RxNoiseFigure)*/
  //thermal noise at 290K in J/s = W
  static const double BOLTZMANN = 1.3803e-23;
  double noise = BOLTZMANN * 290.0 * txVector.GetChannelWidth () * 1000000 * 10;

  /* Compute RSS in dBm, so add 30 from SNR */
  double rss = 10 * log10 (snr * noise) + 30;
  double rss_delta;
  double ber;

  /**** Control PHY ****/
  if (modename == "DMG_MCS0" || modename == "EDMG_MCS0")
    rss_delta = rss - -78;

  /**** SC PHY ****/
  else if (modename == "DMG_MCS1" || modename == "EDMG_SC_MCS1")
    rss_delta = rss - -68;
  else if (modename == "DMG_MCS2" || modename == "EDMG_SC_MCS2")
    rss_delta = rss - -66;
  else if (modename == "DMG_MCS3" || modename == "EDMG_SC_MCS3")
    rss_delta = rss - -65;
  else if (modename == "DMG_MCS4" || modename == "EDMG_SC_MCS4")
    rss_delta = rss - -64;
  else if (modename == "DMG_MCS5" || modename == "EDMG_SC_MCS5")
    rss_delta = rss - -62;
  else if (modename == "DMG_MCS6" || modename == "EDMG_SC_MCS7")
    rss_delta = rss - -63;
  else if (modename == "DMG_MCS7" || modename == "EDMG_SC_MCS8")
    rss_delta = rss - -62;
  else if (modename == "DMG_MCS8" || modename == "EDMG_SC_MCS9")
    rss_delta = rss - -61;
  else if (modename == "DMG_MCS9" || modename == "EDMG_SC_MCS10")
    rss_delta = rss - -59;
  else if (modename == "DMG_MCS10" || modename == "EDMG_SC_MCS12")
    rss_delta = rss - -55;
  else if (modename == "DMG_MCS11" || modename == "EDMG_SC_MCS13")
    rss_delta = rss - -54;
  else if (modename == "DMG_MCS12" || modename == "EDMG_SC_MCS14")
    rss_delta = rss - -53;

  /**** OFDM PHY ****/
  else if (modename == "DMG_MCS13")
    rss_delta = rss - -66;
  else if (modename == "DMG_MCS14")
    rss_delta = rss - -64;
  else if (modename == "DMG_MCS15")
    rss_delta = rss - -63;
  else if (modename == "DMG_MCS16")
    rss_delta = rss - -62;
  else if (modename == "DMG_MCS17")
    rss_delta = rss - -60;
  else if (modename == "DMG_MCS18")
    rss_delta = rss - -58;
  else if (modename == "DMG_MCS19")
    rss_delta = rss - -56;
  else if (modename == "DMG_MCS20")
    rss_delta = rss - -54;
  else if (modename == "DMG_MCS21")
    rss_delta = rss - -53;
  else if (modename == "DMG_MCS22")
    rss_delta = rss - -51;
  else if (modename == "DMG_MCS23")
    rss_delta = rss - -49;
  else if (modename == "DMG_MCS24")
    rss_delta = rss - -47;

  /**** Low power PHY ****/
  else if (modename == "DMG_MCS25")
    rss_delta = rss - -64;
  else if (modename == "DMG_MCS26")
    rss_delta = rss - -60;
  else if (modename == "DMG_MCS27")
    rss_delta = rss - -57;
  else if (modename == "DMG_MCS28")
    rss_delta = rss - -57;
  else if (modename == "DMG_MCS29")
    rss_delta = rss - -57;
  else if (modename == "DMG_MCS30")
    rss_delta = rss - -57;
  else if (modename == "DMG_MCS31")
    rss_delta = rss - -57;
  else
    NS_FATAL_ERROR("Unrecognized 60 GHz modulation");

  /* Compute BER in lookup table */
  if ((rss_delta < -12.0) || (snr < 0))
    ber = sensitivity_ber (0);
  else if (rss_delta > 6.0)
    ber = sensitivity_ber (180);
  else
    ber = sensitivity_ber ((int) std::abs ((10 * (rss_delta + 12))));

  NS_LOG_DEBUG ("SENSITIVITY: ber=" << ber << ", rss_delta=" << rss_delta << ", snr[linear]=" << snr << ", rss[dBm]=" << rss << ", bits=" << nbits);

  /* Compute PSR from BER */
  return pow (1 - ber, nbits);
}

} // namespace ns3
