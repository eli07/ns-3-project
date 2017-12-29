//==============================================================================
// run.cc
// - 2 nodes
// - node 0 is the source, node 1 is the sink
// - UDP flow
//
// - example run:
// ./waf --run scratch/run
//==============================================================================

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/ipv4-header.h"
#include "ns3/tcp-header.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>

NS_LOG_COMPONENT_DEFINE("SimulationScript");
using namespace ns3;

//------------------------------------------------------------------------------
// global variables
NetDeviceContainer devices;
uint32_t num_nodes;
uint32_t num_radios;
//------------------------------------------------------------------------------

int main(int argc, char *argv[]) {

	std::string phyMode("OfdmRate54Mbps");
	double begin_time = 60.0;
	double duration = 10.0;
	
	// parameters
	num_nodes = 2;
	num_radios = 2;

	//--------------------------------------------------------------------------
	// command line processing
	CommandLine cmd;
	cmd.Parse(argc, argv);
	//--------------------------------------------------------------------------

	//--------------------------------------------------------------------------
	// default configurations
	ConfigStore config;
	config.ConfigureDefaults();
	Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
    Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
    Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (phyMode));
	//--------------------------------------------------------------------------

	//--------------------------------------------------------------------------
	// create node container and nodes
	NodeContainer c;
	c.Create(num_nodes);
	//--------------------------------------------------------------------------
	
	//--------------------------------------------------------------------------
	// multi-radio WiFi device
	MrWifiHelper wifi;
	wifi.SetNumRadios(2);
	wifi.SetStandard(WIFI_PHY_STANDARD_80211a);
	//--------------------------------------------------------------------------

	//--------------------------------------------------------------------------
	// PHY
	YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
    wifiPhy.Set("RxGain", DoubleValue(0));
    wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
	//--------------------------------------------------------------------------

	//--------------------------------------------------------------------------
	// configure channel model
	YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
	//--------------------------------------------------------------------------

	//--------------------------------------------------------------------------
	// create channel and connect PHY to channel
	wifiPhy.SetChannel(wifiChannel.Create());
	//--------------------------------------------------------------------------

	//--------------------------------------------------------------------------
	// MAC
	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default();
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue(phyMode),
                                "ControlMode", StringValue(phyMode));

    wifiMac.SetType("ns3::AdhocWifiMac");
	//--------------------------------------------------------------------------

	//--------------------------------------------------------------------------
	// install PHY and MAC to nodes
	devices = wifi.Install(wifiPhy, wifiMac, c);
	//--------------------------------------------------------------------------

	//--------------------------------------------------------------------------
	// set data rates for interfaces
	for(uint32_t i=0; i<num_nodes; i++) {
        Ptr<MrWifiNetDevice> mrdevice = DynamicCast<MrWifiNetDevice>(devices.Get(i));
        Ptr<WifiNetDevice> device = mrdevice->GetPhysicalNetDevice(0);
        device->GetRemoteStationManager()->SetAttribute("DataMode",       StringValue("OfdmRate54Mbps"));
        device->GetRemoteStationManager()->SetAttribute("ControlMode",    StringValue("OfdmRate54Mbps"));
        device->GetRemoteStationManager()->SetAttribute("NonUnicastMode", StringValue("OfdmRate54Mbps"));

        device = mrdevice->GetPhysicalNetDevice(1);
        device->GetRemoteStationManager()->SetAttribute("DataMode",       StringValue("OfdmRate54Mbps"));
        device->GetRemoteStationManager()->SetAttribute("ControlMode",    StringValue("OfdmRate54Mbps"));
        device->GetRemoteStationManager()->SetAttribute("NonUnicastMode", StringValue("OfdmRate54Mbps"));
    }
	//--------------------------------------------------------------------------

	//--------------------------------------------------------------------------
	// node locations and mobility patterns
	MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector( 0.0,  0.0, 1.0));
    positionAlloc->Add(Vector(80.0,  0.0, 1.0));	// 80 meters apart
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(c);
	//--------------------------------------------------------------------------
	
	//--------------------------------------------------------------------------
	// network layer: IP
    InternetStackHelper internet;
    //internet.SetRoutingHelper(list);      // routing not used here.
    internet.Install(c);
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("192.168.150.0", "255.255.255.0");
    Ipv4InterfaceContainer i = ipv4.Assign(devices);
	//--------------------------------------------------------------------------

	//--------------------------------------------------------------------------
	// transport layer: UDP
	ApplicationContainer serverApps;		// moved to global variables
	UdpServerHelper myServer(5000);
	serverApps = myServer.Install(c.Get(1));	// server: node 1
    serverApps.Start(Seconds(begin_time));
    serverApps.Stop(Seconds(begin_time+duration));

    UdpClientHelper myClient(i.GetAddress(1), 5000);	// destination: node 1
    myClient.SetAttribute("MaxPackets", UintegerValue(4294967295u));
    myClient.SetAttribute("Interval", TimeValue(Time("0.0001")));	// 0.0001 ==> 117.6Mbps (1470-byte packets)
    myClient.SetAttribute("PacketSize", UintegerValue(1472));

    ApplicationContainer clientApps = myClient.Install(c.Get(0));	// client: node 0
    clientApps.Start(Seconds(begin_time));
    clientApps.Stop(Seconds(begin_time+duration));
	//--------------------------------------------------------------------------

	//--------------------------------------------------------------------------
	// predefined schedules
	Simulator::Stop(Seconds(begin_time + duration + 10.0));
	//--------------------------------------------------------------------------

	//--------------------------------------------------------------------------
	// run simulation
	NS_LOG_UNCOND("begin simulation...");
    Simulator::Run();
    NS_LOG_UNCOND("simulation complete.");
    Simulator::Destroy();
	//--------------------------------------------------------------------------

	//--------------------------------------------------------------------------
	// post-simulation statistics
	uint32_t totalPacketsThrough = DynamicCast<UdpServer>(serverApps.Get(0))->GetReceived();
    double throughput = totalPacketsThrough / (duration * 1000000.0) * 1472 * 8;	
    NS_LOG_UNCOND("Throughput: " << throughput << "Mbps");
	//--------------------------------------------------------------------------

	return 0;
}
	









