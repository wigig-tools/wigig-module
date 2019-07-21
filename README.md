## Latest News:
We just got a new paper accepted in WNS3-2019, which brings a lot of new features and capabilities to the current model. The new features include:

1. Beamforming Codebook design for beamforming training and beam steering.
1. Quasi-deterministic channel model to simulate real propagation environments.
1. Codebook Generator application in MATLAB to generate codebook samples similar to ones used in Qualcomm QCA9500 Chipset.
1. Multi-antenna beamforming training.
1. Accurate BER vs. SNR lookup tables for all PHY layers.
1. Beam refinement for both transmit and receive beam patterns.
1. Multi-AP support without the need for decentralized clustering.
1. Many new examples and bug fixes.

We are planning to release the new version of the code around August 2019.

## Introduction:
This is a repository for the development of WLAN IEEE 802.11ad/ay model in network simulator ns-3. The implemented model supports the following features:

1. DMG Channel Access Periods (BTI/A-BFT/ATI/DTI with both CBAP and Service Periods).
1. Beamforming Training (BT) in BHI and DTI.
1. DMG PLCP Model for 802.11ad frame transmission and reception.
1. Abstract DMG PHY layer for DMG CTRL/SC/OFDM.
1. 60 GHz Directional Antenna Model.
1. Fast Session Transfer (FST) Mechanism.
1. DMG Relay Support (Full Duplex and Half Duplex Modes).
1. Dynamic Channel Allocation (Polling).
1. Service Period Allocation.
1. Beamformed Link Maintenance for Service Period Allocation.
1. Decentralized Clustering.
1. Spatial Sharing and Interference Assessment. 

## Acknowledgment:
The implementation is based on the existing model of the WLAN IEEE 802.11 in ns-3. The following papers include background on IEEE 802.11ad, implementation details, and evaluation section for this model. If you use our model in your research, please cite the following papers: 

**[Implementation and Evaluation of a WLAN IEEE 802.11ad Model in ns-3.](https://dl.acm.org/citation.cfm?id=2915377)**
Hany Assasa, Joerg Widmer (June 2016)
The Workshop on ns-3 (WNS3 2016), 15-16 June 2016, Seattle, WA, USA

    @inproceedings{Assasa:2016:IEW:2915371.2915377,
     author = {Assasa, Hany and Widmer, Joerg},
     title = {Implementation and Evaluation of a WLAN IEEE 802.11Ad Model in Ns-3},
     booktitle = {Proceedings of the Workshop on Ns-3},
     series = {WNS3 '16},
     year = {2016},
     isbn = {978-1-4503-4216-2},
     location = {Seattle, WA, USA},
     pages = {57--64},
     numpages = {8},
     url = {http://doi.acm.org/10.1145/2915371.2915377},
     doi = {10.1145/2915371.2915377},
     acmid = {2915377},
     publisher = {ACM},
     address = {New York, NY, USA},
     keywords = {60 GHz, IEEE 802.11ad, Millimeter Wave, ns-3},
    } 

**[Extending the IEEE 802.11ad Model: Scheduled Access, Spatial Reuse, Clustering, and Relaying.](https://dl.acm.org/citation.cfm?id=3067667)**
Hany Assasa, Joerg Widmer (June 2017) 
The Workshop on ns-3 (WNS3), 13-14 June 2017, Porto, Portugal

    @inproceedings{Assasa:2017:EIM:3067665.3067667,
     author = {Assasa, Hany and Widmer, Joerg},
     title = {Extending the IEEE 802.11ad Model: Scheduled Access, Spatial Reuse, Clustering, and Relaying},
     booktitle = {Proceedings of the Workshop on Ns-3},
     series = {WNS3 '17},
     year = {2017},
     isbn = {978-1-4503-5219-2},
     location = {Porto, Portugal},
     pages = {39--46},
     numpages = {8},
     url = {http://doi.acm.org/10.1145/3067665.3067667},
     doi = {10.1145/3067665.3067667},
     acmid = {3067667},
     publisher = {ACM},
     address = {New York, NY, USA},
     keywords = {60 GHz, IEEE 802.11ad, Millimeter Wave, WiGig, ns-3},
    } 

**[Enhancing the ns-3 IEEE 802.11ad Model Fidelity: Beam Codebooks, Multi-antenna Beamforming Training, and Quasi-deterministic mmWave Channel.](https://dl.acm.org/citation.cfm?id=3321354)**
Hany Assasa, Joerg Widmer, Tanguy  Ropitault, Nada Golmie (June 2019) 
The Workshop on ns-3 (WNS3), 19-20 June 2019, Florence, Italy

@inproceedings{Assasa:2019:ENI:3321349.3321354,
 author = {Assasa, Hany and Widmer, Joerg and Ropitault, Tanguy and Golmie, Nada},
 title = {Enhancing the Ns-3 IEEE 802.11Ad Model Fidelity: Beam Codebooks, Multi-antenna Beamforming Training, and Quasi-deterministic mmWave Channel},
 booktitle = {Proceedings of the 2019 Workshop on Ns-3},
 series = {WNS3 2019},
 year = {2019},
 isbn = {978-1-4503-7140-7},
 location = {Florence, Italy},
 pages = {33--40},
 numpages = {8},
 url = {http://doi.acm.org/10.1145/3321349.3321354},
 doi = {10.1145/3321349.3321354},
 acmid = {3321354},
 publisher = {ACM},
 address = {New York, NY, USA},
 keywords = {60 GHz, IEEE 802.11ad, IEEE 802.11ay, Millimeter Wave, Simulations, WiGig, ns-3},
} 

## Project Road-map:

### Short-Term Road-map:
I am planning to release the following new features around June/July:
1. Codebook design for supporting multiple-phased antenna array per device. 
1. Codebook generator application.
1. Accurate error models (BER-SNR lookup table for different Modulation and Coding Schemes).
1. Improved beamforming training mechanisms for multi-antenna support.
1. QD-Channel model using Ray-Tracer application.
1. Bug fixes and code usability improvement.

### Long Term Road-map:
The following features are being developed but in slow progress:
1. Add 802.11ay PHY frame structure, new MAC frame formats, and new Information Elements.
1. Add support for SU/MU-MIMO signaling and communication. I am planning to model it in a more advanced way compared to the way currently done in 802.11n/ac model (Doubling the achieved throughout).
1. Improve the PHY layer model and implementation.

## Limitation:
Below is a list of the limitation in the code:
1. Data communication in SP allocation does not support A-MPDU aggregation and only A-MSDU aggregation. The reason is A-MPDU aggregation requires establishing Block Acknowledgement agreement between the initiator and the responder, which requires a bi-directional transmission in the SP allocation. However, communication in SP is only allowed in a uni-directional way. To solve the previous problem, Reverse Directional Protocol is required.
1. The current implementation does not support Multi-AP operation using CSMA/CA. To support Multi-AP scenario, you need to enable Decentralized clustering.
1. Not all the features have been tested in complex scenarios. In case, you run into a bug, feel free to report it.
1. The model uses a simplified Error Model contributed by Daniel Halperin in "Augmenting Data Center Networks with
Multi-gigabit Wireless Links". The error uses a single PER vs. SNR curve for all MCSs.

## Prerequisites:
Before start using the 802.11ad model, please keep the following in mind:

1. Understand WLAN IEEE 802.11 MAC/PHY operations. There are plenty of references on the Internet describing CSMA/CA protocol and the evolution of 802.11 protocol.
1. Get familiar with ns-3 and how to run simulations in ns-3. Have a look on the tutorial page of the simulator.
1. Understand the existing Wifi Model in ns-3 which implements WLAN IEEE 802.11a/b/g/n/ac/ax.
1. Finally, do not contact me asking how to use the model or provide you with some documentation on how to use ns-3. I will ignore your email :)

Once you have completed all these steps, you can proceed with my model.

## Building the Project:
The current implementation is based on ns3-26. As I changed some of the Wifi module APIs, this will affect Mesh module too. For this reason, you should disable building both examples and tests; otherwise, the build will fail. To save time and evaluate the IEEE 802.11ad model only, type the following command:

    ./waf configure --disable-examples --disable-tests --disable-python --enable-modules='applications','core','internet','point-to-point','wifi'
    ./waf build

The previous command builds the required models only to run IEEE 802.11ad with its provided scripts in debug mode. To build the project in optimized mode for fast execution type the following command:

    ./waf configure --disable-examples --disable-tests --disable-python --enable-modules='applications','core','internet','point-to-point','wifi' --enable-static -d optimized
    ./waf build

## Tutorial Scripts:
The project includes different scripts located in the scratch folder to test the previously listed features and mechanisms. At the beginning of each script, I add some description regarding the tested functionality, network topology, expected output, and usage method.

## Reporting:
In case you come across a bug during the usage of the original model, please report the problem to the following email address (hany.assasa@gmail.com). In the email, please include the following:

1. Simulation file with a small description of the simulated scenario and the expected output.
1. The set of input parameters which caused the simulation to crash.

Please do not report any problem related to your modification of the original code.

## Author Information:
The model is developed and maintained by [Hany Assasa](http://people.networks.imdea.org/~hany_assasa/). For more information about the author research, please check his website.
