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

// Global variables ------------------------------------------------------------
double check_period;
uint32_t prev_packets;
uint32_t total_packets;
uint32_t payloadSize;

uint32_t num_nodes;

ApplicationContainer serverApp, sinkApp;
NetDeviceContainer apDevice;
NetDeviceContainer staDevice;

uint32_t progress_count;
//------------------------------------------------------------------------------

static void ResetStatistics(void) {
	for(uint32_t i=0; i<num_nodes; i++) {
        Ptr<MrWifiNetDevice> mrdevice = DynamicCast<MrWifiNetDevice>(staDevice.Get(i));
		mrdevice->ResetStatistics();
	}
}

static void PrintProgress(void) {

	progress_count++;

	if(progress_count % 10) fprintf(stderr, ".");
	else fprintf(stderr, "%d", progress_count/10);
	Simulator::Schedule(Seconds(check_period), &PrintProgress);

	/*
	total_packets = DynamicCast<UdpServer>(serverApp.Get(0))->GetReceived();
	uint32_t curr_packets = total_packets - prev_packets;
	double curr_throughput = (double)curr_packets / (check_period * 1000000.0) * payloadSize * 8;
	prev_packets = total_packets;

	NS_LOG_UNCOND("| " << curr_throughput << " Mbps");
	*/
}

int main (int argc, char *argv[])
{
	FILE *outfile;
    char outfilename[80];

	//LogComponentEnable("EdcaTxopN", LogLevel(LOG_LEVEL_ALL|LOG_PREFIX_TIME));
	//LogComponentEnable("DcfManager", LogLevel(LOG_LEVEL_ALL|LOG_PREFIX_TIME));
	//LogComponentEnable("MacLow", LogLevel(LOG_LEVEL_ALL|LOG_PREFIX_TIME));

	uint16_t config_index = 1;

	// parameters --------------------------------------------------------------
    double sim_time = 1.0; //seconds
    double distance = 1.0; //meters
    num_nodes = 4;
	uint16_t num_radios = 2;
	uint16_t channel_width;
	uint16_t mcs_level = 7;
	payloadSize = 1472;
	uint16_t proposed = 1;
	uint16_t debug = 99;

	double pre_begin_time = 100.0;
	double pre_interval = 10.0;
	double pre_duration = 0.01;
	
	double begin_time = 1000.0;
	check_period = 0.01;
	progress_count = 0;

	uint32_t run = 1;
	//--------------------------------------------------------------------------

	// command-line arguments --------------------------------------------------
    CommandLine cmd;
    cmd.AddValue ("distance", "Distance in meters between the station and the access point", distance);
    cmd.AddValue ("time", "Simulation time in seconds", sim_time);
	cmd.AddValue ("config", "Configuration index", config_index);
    cmd.AddValue ("nodes", "Number of nodes", num_nodes);
	cmd.AddValue ("radios", "Number of radios", num_radios);
	cmd.AddValue ("proposed", "Proposed Scheme", proposed);
	cmd.AddValue ("run", "Run index (for setting repeatable seeds)", run);
	cmd.AddValue ("debug", "Debug", debug);
    cmd.Parse (argc,argv);
	//--------------------------------------------------------------------------

	SeedManager::SetSeed(1);
	SeedManager::SetRun(run);

	// Set channel width -------------------------------------------------------
	if(num_radios <= 1) {
		channel_width = 160;
	} else if(num_radios <= 2) {
		channel_width = 80;
	} else if(num_radios <= 4) {
		channel_width = 40;
	} else channel_width = 20;

	// Create nodes ------------------------------------------------------------
	NodeContainer wifiStaNode;
	wifiStaNode.Create(num_nodes);
	NodeContainer wifiApNode;
	wifiApNode.Create(1);
	//--------------------------------------------------------------------------

	// Wi-Fi network device ----------------------------------------------------
	MrWifiHelper wifi;
	wifi.SetNumRadios(num_radios);
	wifi.SetStandard (WIFI_PHY_STANDARD_80211ac);
	//--------------------------------------------------------------------------

	// channel & PHY -----------------------------------------------------------
	YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
	YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
	phy.Set ("ShortGuardEnabled", BooleanValue(1));
	phy.SetChannel (channel.Create ());
	//--------------------------------------------------------------------------

	// Remote station manager --------------------------------------------------
	std::ostringstream oss;
	oss << "VhtMcs" << mcs_level;
	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
			"DataMode", StringValue (oss.str ()),
			"ControlMode", StringValue (oss.str ()));
	//--------------------------------------------------------------------------

	// MAC ---------------------------------------------------------------------
	WifiMacHelper mac;

	Ssid ssid = Ssid ("ns3-80211ac");

	//mac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid));
	mac.SetType("ns3::AdhocWifiMac");
	staDevice = wifi.Install (phy, mac, wifiStaNode);

	//mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));
	mac.SetType("ns3::AdhocWifiMac");
	apDevice = wifi.Install (phy, mac, wifiApNode);
	//--------------------------------------------------------------------------

	// Set channel width to 160MHz ---------------------------------------------
	//Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", UintegerValue (160));

	Ptr<MrWifiNetDevice> mrdevice;
	Ptr<WifiNetDevice> device;
	for(uint32_t i=0; i<num_nodes; i++) {
        mrdevice = DynamicCast<MrWifiNetDevice>(staDevice.Get(i));
		for(uint32_t j=0; j<num_radios; j++) {
			device = mrdevice->GetPhysicalNetDevice(j);
			if(channel_width == 160) {
				device->GetPhy()->SetChannelWidth(160);
				device->GetPhy()->SetChannelNumber(50);
			} else if(channel_width == 80) {
				device->GetPhy()->SetChannelWidth(80);
				device->GetPhy()->SetChannelNumber(42+j*16);
			} else if(channel_width == 40) {
				device->GetPhy()->SetChannelWidth(40);
				device->GetPhy()->SetChannelNumber(38+j*8);
			} else if(channel_width == 20) {
				device->GetPhy()->SetChannelWidth(20);
				device->GetPhy()->SetChannelNumber(36+j*4);
			}
		}
		if(proposed == 0) mrdevice->SetProposed(false);
		else mrdevice->SetProposed(true);
		mrdevice->SetDebug(debug);
	}

	mrdevice = DynamicCast<MrWifiNetDevice>(apDevice.Get(0));
	for(uint32_t j=0; j<num_radios; j++) {
		device = mrdevice->GetPhysicalNetDevice(j);
		if(channel_width == 160) {
			device->GetPhy()->SetChannelWidth(160);
			device->GetPhy()->SetChannelNumber(50);
		} else if(channel_width == 80) {
			device->GetPhy()->SetChannelWidth(80);
			device->GetPhy()->SetChannelNumber(42+j*16);
		} else if(channel_width == 40) {
			device->GetPhy()->SetChannelWidth(40);
			device->GetPhy()->SetChannelNumber(38+j*8);
		} else if(channel_width == 20) {
			device->GetPhy()->SetChannelWidth(20);
			device->GetPhy()->SetChannelNumber(36+j*4);
		}
		if(proposed == 0) mrdevice->SetProposed(false);
		else mrdevice->SetProposed(true);
		mrdevice->SetDebug(debug);
	}
	//--------------------------------------------------------------------------

	// locations ---------------------------------------------------------------
	MobilityHelper mobility;
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

	positionAlloc->Add (Vector (0.0, 0.0, 1.0));

	for(uint16_t i = 0; i < num_nodes; i++) {
		//positionAlloc->Add (Vector (distance + (0.1*i), 0.0, 10.0));
		positionAlloc->Add (Vector (1.0, 0.0, 1.0));
	}

	mobility.SetPositionAllocator (positionAlloc);
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (wifiApNode);
	mobility.Install (wifiStaNode);
	//--------------------------------------------------------------------------

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
	//--------------------------------------------------------------------------

	// Setting applications ----------------------------------------------------
	//ApplicationContainer serverApp, sinkApp;
	UdpServerHelper myServer (9);
	serverApp = myServer.Install (wifiApNode.Get (0));
	serverApp.Start(Seconds(begin_time));
	serverApp.Stop(Seconds(begin_time+sim_time));

	UdpClientHelper myClient (apNodeInterface.GetAddress (0), 9);
	myClient.SetAttribute ("MaxPackets", UintegerValue (4294967295u));
	
	// 1472 bytes + 0.00001 packets/sec --> 1,177.6Mbps (higher than highest expected throughput of a channel)
	myClient.SetAttribute ("Interval", TimeValue (Seconds(0.00002/num_radios))); //packets/s
	myClient.SetAttribute ("PacketSize", UintegerValue (payloadSize));

	for(uint16_t i = 0; i < num_nodes; i++) {
		ApplicationContainer clientPreApp = myClient.Install(wifiStaNode.Get(i));
		clientPreApp.Start(Seconds(pre_begin_time+pre_interval*i));
		clientPreApp.Stop(Seconds(pre_begin_time+pre_interval*i+pre_duration));
	}

	//myClient.SetAttribute ("Interval", TimeValue (Seconds(0.0001*num_radios))); //packets/s
	//myClient.SetAttribute ("Interval", TimeValue (Seconds(0.00001*num_nodes/num_radios))); //packets/s

	for(uint16_t i = 0; i < num_nodes; i++) {
		ApplicationContainer clientApp = myClient.Install (wifiStaNode.Get (i));
		clientApp.Start(Seconds(begin_time + (0.001*i)));
		clientApp.Stop(Seconds(begin_time + sim_time));
	}

	//Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
	//--------------------------------------------------------------------------
	
	// Predefined Schedules ----------------------------------------------------
	prev_packets = 0;
	Simulator::Schedule(Seconds(begin_time), &PrintProgress);	
	Simulator::Schedule(Seconds(begin_time), &ResetStatistics);

	// Display configuration ---------------------------------------------------
	NS_LOG_UNCOND("----------------------------------------------");
	NS_LOG_UNCOND(" Number of mobile stations: " << num_nodes);
	NS_LOG_UNCOND(" Number of radios: " << num_radios);
	NS_LOG_UNCOND(" Channel bandwidth: " << channel_width);
	NS_LOG_UNCOND(" MCS level: " << mcs_level);
	NS_LOG_UNCOND(" Simulation time: " << sim_time);
	NS_LOG_UNCOND(" Proposed protocol: " << proposed);
	NS_LOG_UNCOND(" Run index: " << run);
	NS_LOG_UNCOND("----------------------------------------------");

	// Run simulation ----------------------------------------------------------
	Simulator::Stop (Seconds (begin_time + sim_time + 0.001));
	Simulator::Run ();
	fprintf(stderr, "\n");

	uint32_t total_num_acks_received[8] = {0};
	uint32_t total_num_acks_missed[8] = {0};
	for(uint32_t i=0; i<num_nodes; i++) {
        mrdevice = DynamicCast<MrWifiNetDevice>(staDevice.Get(i));
		for(uint32_t j=0; j<num_radios; j++) {
			NS_LOG_UNCOND("[" << i << ":" << j << "] number of acks received: " << mrdevice->GetNumAcksReceived(j));
			total_num_acks_received[j] += mrdevice->GetNumAcksReceived(j);
			NS_LOG_UNCOND("[" << i << ":" << j << "] number of acks missed: " << mrdevice->GetNumAcksMissed(j));
			total_num_acks_missed[j] += mrdevice->GetNumAcksMissed(j);
		}
	}

	NS_LOG_UNCOND(" ");

	for(uint32_t j=0; j<num_radios; j++) {
		NS_LOG_UNCOND("[ " << j << " ] total number of acks received: " << total_num_acks_received[j]);
		NS_LOG_UNCOND("[ " << j << " ] total number of acks missed  : " << total_num_acks_missed[j]);
	}

	/*
	NS_LOG_UNCOND("total number of acks received: " << total_num_acks_received);
	NS_LOG_UNCOND("total number of acks missed: " << total_num_acks_missed);
	*/

	Simulator::Destroy ();

	sprintf(outfilename, "tput_c%03d_n%03d_r%02d_w%03d_m%01d_p%01d_s%d.txt", config_index, num_nodes, num_radios, channel_width, mcs_level, proposed, run);
    outfile = fopen(outfilename, "w");

	// Throughput calculation --------------------------------------------------
	double throughput = 0;
	uint32_t totalPacketsThrough = DynamicCast<UdpServer> (serverApp.Get (0))->GetReceived ();
	throughput = totalPacketsThrough * payloadSize * 8 / (sim_time * 1000000.0); //Mbit/s

	NS_LOG_UNCOND("throughput: " << throughput << " Mbps");

	fprintf(outfile, " %d", config_index);
    fprintf(outfile, " %d", num_nodes);
    fprintf(outfile, " %d", num_radios);
    fprintf(outfile, " %d", channel_width);
    fprintf(outfile, " %d", mcs_level);
    fprintf(outfile, " %d", proposed);
    fprintf(outfile, " %d", run);
    fprintf(outfile, " %8.2f", throughput);
    for(uint32_t j=0; j<num_radios; j++) {
        fprintf(outfile, " %5d", total_num_acks_missed[j]);
    }
    fprintf(outfile, "\n");
    fclose(outfile);

	return 0;
}





