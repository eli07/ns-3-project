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

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SimulationScript");

double check_period;
uint32_t prev_packets;
uint32_t total_packets;
uint32_t payloadSize;

ApplicationContainer serverApp, sinkApp;

static void PrintProgress(void) {
	total_packets = DynamicCast<UdpServer>(serverApp.Get(0))->GetReceived();
	uint32_t curr_packets = total_packets - prev_packets;
	double curr_throughput = (double)curr_packets / (check_period * 1000000.0) * payloadSize * 8;
	prev_packets = total_packets;

	NS_LOG_UNCOND("| " << curr_throughput << " Mbps");
	Simulator::Schedule(Seconds(check_period), &PrintProgress);
}

int main (int argc, char *argv[])
{
	// parameters --------------------------------------------------------------
    double sim_time = 6.0; //seconds
    double distance = 1.0; //meters
    uint16_t num_nodes = 12;
	payloadSize = 1472;
	
	double begin_time = 60.0;
	check_period = 1.0;

    CommandLine cmd;
    cmd.AddValue ("distance", "Distance in meters between the station and the access point", distance);
    cmd.AddValue ("time", "Simulation time in seconds", sim_time);
    cmd.AddValue ("nodes", "Number of nodes", num_nodes);
    cmd.Parse (argc,argv);

	NodeContainer wifiStaNode;
	wifiStaNode.Create(num_nodes);
	NodeContainer wifiApNode;
	wifiApNode.Create (1);

	YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
	YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
	phy.SetChannel (channel.Create ());

	// Set guard interval enabled ----------------------------------------------
	phy.Set ("ShortGuardEnabled", BooleanValue(1));

	WifiHelper wifi;
	wifi.SetStandard (WIFI_PHY_STANDARD_80211ac);
	WifiMacHelper mac;

	std::ostringstream oss;
	oss << "VhtMcs" << "9";
	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode", StringValue (oss.str ()),
			"ControlMode", StringValue (oss.str ()));

	Ssid ssid = Ssid ("ns3-80211ac");

	mac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid));

	NetDeviceContainer staDevice;
	staDevice = wifi.Install (phy, mac, wifiStaNode);

	mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));

	NetDeviceContainer apDevice;
	apDevice = wifi.Install (phy, mac, wifiApNode);

	// Set channel width to 160MHz ---------------------------------------------
	Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", UintegerValue (160));

	// mobility ----------------------------------------------------------------
	MobilityHelper mobility;
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

	positionAlloc->Add (Vector (0.0, 0.0, 0.0));

	for(uint16_t i = 0; i < num_nodes; i++) {
		positionAlloc->Add (Vector (distance + (0.1*i), 0.0, 0.0));
	}

	mobility.SetPositionAllocator (positionAlloc);
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (wifiApNode);
	mobility.Install (wifiStaNode);

	// Internet stack ----------------------------------------------------------
	InternetStackHelper stack;
	stack.Install (wifiApNode);
	stack.Install (wifiStaNode);

	Ipv4AddressHelper address;

	address.SetBase ("192.168.1.0", "255.255.255.0");
	Ipv4InterfaceContainer staNodeInterface;
	Ipv4InterfaceContainer apNodeInterface;

	staNodeInterface = address.Assign (staDevice);
	apNodeInterface = address.Assign (apDevice);

	// Setting applications ----------------------------------------------------
	//ApplicationContainer serverApp, sinkApp;
	UdpServerHelper myServer (9);
	serverApp = myServer.Install (wifiApNode.Get (0));
	serverApp.Start(Seconds(begin_time));
	serverApp.Stop(Seconds(begin_time+sim_time));

	UdpClientHelper myClient (apNodeInterface.GetAddress (0), 9);
	myClient.SetAttribute ("MaxPackets", UintegerValue (4294967295u));
	myClient.SetAttribute ("Interval", TimeValue (Seconds(0.00001*num_nodes))); //packets/s
	myClient.SetAttribute ("PacketSize", UintegerValue (payloadSize));

	for(uint16_t i = 0; i < num_nodes; i++) {
		ApplicationContainer clientApp = myClient.Install (wifiStaNode.Get (i));
		clientApp.Start(Seconds(begin_time + (0.001*i)));
		clientApp.Stop(Seconds(begin_time + sim_time));
	}

	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

	// Predefined Schedules ----------------------------------------------------
	prev_packets = 0;
	Simulator::Schedule(Seconds(begin_time), &PrintProgress);	

	// Run simulation ----------------------------------------------------------
	Simulator::Stop (Seconds (begin_time + sim_time + 0.1));
	Simulator::Run ();
	Simulator::Destroy ();

	// Throughput calculation --------------------------------------------------
	double throughput = 0;
	uint32_t totalPacketsThrough = DynamicCast<UdpServer> (serverApp.Get (0))->GetReceived ();
	throughput = totalPacketsThrough * payloadSize * 8 / (sim_time * 1000000.0); //Mbit/s

	NS_LOG_UNCOND("throughput: " << throughput << " Mbps");

	return 0;
}

