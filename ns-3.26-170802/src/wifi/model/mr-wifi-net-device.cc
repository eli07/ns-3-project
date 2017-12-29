#include "ns3/simulator.h"
#include "mr-wifi-net-device.h"
#include "wifi-net-device.h"
#include "wifi-mac.h"
#include "regular-wifi-mac.h"
#include "wifi-phy.h"
#include "wifi-remote-station-manager.h"
#include "wifi-channel.h"
#include "ns3/llc-snap-header.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include "ns3/node.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/log.h"
#include "ns3/random-variable-stream.h"
#include "ns3/internet-module.h"

NS_LOG_COMPONENT_DEFINE("MrWifiNetDevice");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(MrWifiNetDevice);

TypeId
MrWifiNetDevice::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::MrWifiNetDevice")
        .SetParent<NetDevice>()
        .AddConstructor<MrWifiNetDevice>()
        .AddAttribute ("Mtu", "The MAC-level Maximum Transmission Unit",
                   UintegerValue (MAX_MSDU_SIZE - LLC_SNAP_HEADER_LENGTH),
                   MakeUintegerAccessor (&MrWifiNetDevice::SetMtu,
                                         &MrWifiNetDevice::GetMtu),
                   MakeUintegerChecker<uint16_t> (1,MAX_MSDU_SIZE - LLC_SNAP_HEADER_LENGTH))
    ;
    return tid;
}

MrWifiNetDevice::MrWifiNetDevice() {
    NS_LOG_FUNCTION_NOARGS();

    m_numRadios = 0;            // incremented in AddRadio()
    m_rxCallback.Nullify();     // initialize RX callback
    m_txCallback.Nullify();     // initialize TX callback

	m_proposed = true;
	m_debug = 99;
}

MrWifiNetDevice::~MrWifiNetDevice() {
    NS_LOG_FUNCTION_NOARGS();
}

void MrWifiNetDevice::DoDispose() {
    NS_LOG_FUNCTION_NOARGS();

    m_node = 0;

    for(int i=0; i<m_numRadios; i++) {
        Ptr<WifiNetDevice> device = *(m_devices.begin()+i);
        device->Dispose();
        device = 0;
    }

    m_devices.clear();

    NetDevice::DoDispose();
}

void MrWifiNetDevice::DoInitialize(void) {
    NS_LOG_FUNCTION_NOARGS();

    // assign default channels to interfaces 
    for(int i=0; i<m_numRadios; i++) {
        Ptr<WifiNetDevice> device = *(m_devices.begin()+i);
        device->Initialize();
		/*
		switch(device->GetPhy()->GetChannelWidth()) {
		case 20:
			device->GetPhy()->SetChannelNumber(36);
			break;
		case 40:
			device->GetPhy()->SetChannelNumber(38);
			break;
		case 80:
			device->GetPhy()->SetChannelNumber(42);
			break;
		case 160:
			device->GetPhy()->SetChannelNumber(50);
			break;
		default:
			NS_LOG_UNCOND("WARNING: invalid channel width");
			device->GetPhy()->SetChannelNumber(36);
		}
		*/
    }

	tx_radio = 0;
	m_token = false;

	for(uint32_t i=0; i<8; i++) {
		num_acks_received[i] = 0;
		num_acks_missed[i] = 0;
	}

    NetDevice::DoInitialize();
}

void MrWifiNetDevice::ResetStatistics() {
	for(uint32_t i=0; i<8; i++) {
		num_acks_received[i] = 0;
		num_acks_missed[i] = 0;
	}
}

void MrWifiNetDevice::SetProposed(bool cmd) {
	m_proposed = cmd;
}

void MrWifiNetDevice::AddRadio(Ptr<WifiNetDevice> device) {
    NS_LOG_FUNCTION_NOARGS();

    m_devices.push_back(device);
    device->SetReceiveCallback(MakeCallback(&MrWifiNetDevice::ReceiveFromDevice, this));
	device->SetGotAckCallback(MakeCallback(&MrWifiNetDevice::GotAck, this));
	device->SetMissedAckCallback(MakeCallback(&MrWifiNetDevice::MissedAck, this));
	device->SetOverheardAckCallback(MakeCallback(&MrWifiNetDevice::OverheardAck, this));

    m_numRadios++;
}

Ptr<WifiNetDevice> MrWifiNetDevice::GetPhysicalNetDevice(int index) {
    NS_LOG_FUNCTION_NOARGS();

    if(index >= (int)(m_devices.size())) {
        NS_LOG_UNCOND("Exception: index violation");
        return NULL;
    }

    return m_devices[index];
}

void MrWifiNetDevice::SetReceiveCallback(MrWifiNetDeviceReceiveCallback callback) {
    m_rxCallback = callback;
}

void MrWifiNetDevice::SetSendCallback(MrWifiNetDeviceSendCallback callback) {
    m_txCallback = callback;
}

uint32_t MrWifiNetDevice::GetNumAcksReceived(uint32_t ifIndex) {
	return num_acks_received[ifIndex];
}

uint32_t MrWifiNetDevice::GetNumAcksMissed(uint32_t ifIndex) {
	return num_acks_missed[ifIndex];
}

void MrWifiNetDevice::GotAck(uint32_t ifIndex) {

	// Except for the first interface, possess token.
	if(ifIndex == 0 && m_proposed == true) {
    	for(int i=1; i<m_numRadios; i++) {
    	    Ptr<WifiNetDevice> device = *(m_devices.begin()+i);
    	    device->SetToken(true);
    	}

		if(m_token == false) {
			if(m_debug != 99) NS_LOG_UNCOND(Simulator::Now() << " " << GetAddress() << " POSSESS TOKEN Queued Packets: " << m_devices[2]->GetBEQueueSize());
			m_token = true;
		}
	}

	num_acks_received[ifIndex]++;
	return;
}

void MrWifiNetDevice::MissedAck(uint32_t ifIndex) {
	num_acks_missed[ifIndex]++;
	return;
}

void MrWifiNetDevice::OverheardAck(uint32_t ifIndex) {

	// Except for the first interface, release token.
    for(int i=1; i<m_numRadios; i++) {
        Ptr<WifiNetDevice> device = *(m_devices.begin()+i);
        device->SetToken(false);
    }

	if(m_token == true) {
		if(m_debug != 99) NS_LOG_UNCOND(Simulator::Now() << " " << GetAddress() << " RELEASE TOKEN");
		m_token = false;
	}

	return;
}

bool MrWifiNetDevice::ReceiveFromDevice(Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol, Address const &from) {

    // call callback function for statistics collection
    if(!m_rxCallback.IsNull()) {
        m_rxCallback(GetNode()->GetId(), device, packet, protocol);
    }

    m_forwardUp(this, packet, protocol, from);
    return true;
}

void MrWifiNetDevice::SetDebug(uint32_t intf) {
	m_debug = intf;

    for(uint32_t i=0; i<m_numRadios; i++) {
        Ptr<WifiNetDevice> device = *(m_devices.begin()+i);
        if(intf == i) device->SetDebug(true);
    }
}

uint32_t MrWifiNetDevice::GetDebug() {
	return m_debug;
}

//------------------------------------------------------------------------------
// inherited functions
//------------------------------------------------------------------------------
void
MrWifiNetDevice::SetIfIndex(const uint32_t index) {
    m_ifIndex = index;
}

uint32_t
MrWifiNetDevice::GetIfIndex(void) const {
    return m_ifIndex;
}

Ptr<Channel>
MrWifiNetDevice::GetChannel(void) const {

    // for now, return the channel of the first radio
    Ptr<WifiNetDevice> device = *(m_devices.begin());
    return device->GetChannel();
}

void
MrWifiNetDevice::SetAddress(Address address) {

    // for now, assign all radios the with same mac address to avoid ARP problem
    for(int i=0; i<m_numRadios; i++) {
        Ptr<WifiNetDevice> device = *(m_devices.begin()+i);
        device->SetAddress(Mac48Address::ConvertFrom(address));
    }
}

Address
MrWifiNetDevice::GetAddress(void) const {

    // for now, return the address of the first radio
    Ptr<WifiNetDevice> device = *(m_devices.begin());
    return device->GetAddress();
}

bool
MrWifiNetDevice::SetMtu(const uint16_t mtu) {
    NS_LOG_FUNCTION_NOARGS();
    if(mtu > MAX_MSDU_SIZE - LLC_SNAP_HEADER_LENGTH) return false;
    m_mtu = mtu;
    return true;
}

uint16_t
MrWifiNetDevice::GetMtu(void) const {
    NS_LOG_FUNCTION(this << m_mtu);
    return m_mtu;
}

bool
MrWifiNetDevice::IsLinkUp(void) const {
    //return m_phy != 0 && m_linkUp;

    if(m_linkUp == false) return false;
    if(m_devices.size() == 0) return false;
    return true;
}

void
MrWifiNetDevice::AddLinkChangeCallback(Callback<void> callback) {
    m_linkChanges.ConnectWithoutContext(callback);
}

bool
MrWifiNetDevice::IsBroadcast(void) const {
    return true;
}

Address
MrWifiNetDevice::GetBroadcast(void) const {
    return Mac48Address::GetBroadcast();
}

bool
MrWifiNetDevice::IsMulticast(void) const {
    return true;
}

Address
MrWifiNetDevice::GetMulticast(Ipv4Address multicastGroup) const {
    return Mac48Address::GetMulticast(multicastGroup);
}

Address
MrWifiNetDevice::GetMulticast(Ipv6Address addr) const {
    return Mac48Address::GetMulticast(addr);
}

bool
MrWifiNetDevice::IsPointToPoint(void) const {
    return false;
}

bool
MrWifiNetDevice::IsBridge(void) const {
    return false;
}

uint32_t
MrWifiNetDevice::SelectInterface(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber) {

	// Check queue
	uint32_t min_radio = 0;
	uint32_t min_size = m_devices[0]->GetBEQueueSize();

	for(uint16_t i=1; i<m_devices.size(); i++) {
		uint32_t adjusted_size = m_devices[i]->GetBEQueueSize() + 10;
		if(adjusted_size < min_size) {
			min_radio = i;
			min_size = adjusted_size;
		}
	}

	return min_radio;

	//tx_radio = (tx_radio + 1) % m_numRadios;
    //return tx_radio;
}

bool
MrWifiNetDevice::Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber) {

    NS_LOG_FUNCTION(this);

    // broadcast packets -------------------------------------------------------
    // send a copy of packet on each channel
    if(dest == GetBroadcast()) {
		/*
        for(uint16_t i=0; i<m_devices.size(); i++) {
            Ptr<Packet> p = packet->Copy();
            m_devices[i]->Send(p, dest, protocolNumber);
        }
		*/
		m_devices[0]->Send(packet, dest, protocolNumber);
    }

    // unicast packets ---------------------------------------------------------
    else {
        uint32_t k = SelectInterface(packet, dest, protocolNumber);
		//if(GetAddress() == Mac48Address("00:00:00:00:00:02")) NS_LOG_UNCOND(Simulator::Now() << " " << GetAddress() << " Sending Packet to Interface " << k);
        m_devices[k]->Send(packet, dest, protocolNumber);
    }

    return true;
}

Ptr<Node>
MrWifiNetDevice::GetNode(void) const {
    return m_node;
}

void
MrWifiNetDevice::SetNode(Ptr<Node> node) {
    m_node = node;
}

bool
MrWifiNetDevice::NeedsArp(void) const {
    return true;
}

void
MrWifiNetDevice::SetReceiveCallback(NetDevice::ReceiveCallback cb) {
    m_forwardUp = cb;
}

void
MrWifiNetDevice::ForwardUp(Ptr<Packet> packet, Mac48Address from, Mac48Address to) {
    NS_LOG_UNCOND("MrWifiNetDevice::ForwardUp: Is this function called?");
}

void
MrWifiNetDevice::LinkUp(void) {
    m_linkUp = true;
    m_linkChanges();
}

void
MrWifiNetDevice::LinkDown(void) {
    m_linkUp = false;
    m_linkChanges();
}

void
MrWifiNetDevice::SetPromiscReceiveCallback(NetDevice::PromiscReceiveCallback cb) {
    m_promiscRx = cb;
}

bool
MrWifiNetDevice::SendFrom(Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber) {

    // for now, send the packet to the first radio
    Ptr<WifiNetDevice> device = *(m_devices.begin());
    device->SendFrom(packet, source, dest, protocolNumber);

    return true;
}

bool
MrWifiNetDevice::SupportsSendFrom(void) const {
    Ptr<WifiNetDevice> device = *(m_devices.begin());
    return device->SupportsSendFrom();
}
//--- end of inherited functions -----------------------------------------------

} // namespace ns3






























































