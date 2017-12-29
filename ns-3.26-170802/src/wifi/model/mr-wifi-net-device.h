#ifndef MR_WIFI_NET_DEVICE_H
#define MR_WIFI_NET_DEVICE_H

#include "ns3/net-device.h"
#include "ns3/packet.h"
#include "ns3/traced-callback.h"
#include "ns3/mac48-address.h"
#include <vector>

namespace ns3 {

class WifiNetDevice;
class WifiRemoteStationManager;
class WifiChannel;
class WifiPhy;
class WifiMac;

class MrWifiNetDevice : public NetDevice
{
public:

	//------------------------------------------------------------------------------------------------------------------
	// typedefs
    typedef Callback<void, uint32_t, Ptr<NetDevice>, Ptr<const Packet>, uint16_t> MrWifiNetDeviceReceiveCallback;
    typedef Callback<void, uint32_t, Ptr<NetDevice>, Ptr<const Packet>, uint16_t> MrWifiNetDeviceSendCallback;
	//------------------------------------------------------------------------------------------------------------------

    MrWifiNetDevice();
    virtual ~MrWifiNetDevice();
    static TypeId GetTypeId(void);

	//------------------------------------------------------------------------------------------------------------------
	// TX/RX interface
    bool ReceiveFromDevice(Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol, const Address &from);
	void GotAck(uint32_t ifIndex);
	void MissedAck(uint32_t ifIndex);
	void OverheardAck(uint32_t ifIndex);
    void SendUp(Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol, const Address &from);

	void SetDebug(uint32_t intf);
	uint32_t GetDebug();
	//------------------------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------------------------
	// callback function variables
	MrWifiNetDeviceReceiveCallback m_rxCallback;
	MrWifiNetDeviceSendCallback m_txCallback;
	//------------------------------------------------------------------------------------------------------------------

    //------------------------------------------------------------------------------------------------------------------
    // These functions are inherited from NetDevice base class
    virtual void SetIfIndex (const uint32_t index);
    virtual uint32_t GetIfIndex (void) const;
    virtual Ptr<Channel> GetChannel (void) const;
    virtual void SetAddress (Address address);
    virtual Address GetAddress (void) const;
    virtual bool SetMtu (const uint16_t mtu);
    virtual uint16_t GetMtu (void) const;
    virtual bool IsLinkUp (void) const;
    virtual void AddLinkChangeCallback (Callback<void> callback);
    virtual bool IsBroadcast (void) const;
    virtual Address GetBroadcast (void) const;
    virtual bool IsMulticast (void) const;
    virtual Address GetMulticast (Ipv4Address multicastGroup) const;
    virtual bool IsPointToPoint (void) const;
    virtual bool IsBridge (void) const;
    virtual bool Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber);
    virtual Ptr<Node> GetNode (void) const;
    virtual void SetNode (Ptr<Node> node);
    virtual bool NeedsArp (void) const;
    virtual void SetReceiveCallback (NetDevice::ReceiveCallback cb);
    virtual Address GetMulticast (Ipv6Address addr) const;
    virtual bool SendFrom(Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber);
    virtual void SetPromiscReceiveCallback(NetDevice::PromiscReceiveCallback cb);
    virtual bool SupportsSendFrom(void) const;
    //------------------------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------------------------
	// additional functions
	void AddRadio(Ptr<WifiNetDevice> device);
	Ptr<WifiNetDevice> GetPhysicalNetDevice(int index);
	void SetReceiveCallback(MrWifiNetDeviceReceiveCallback callback);
	void SetSendCallback(MrWifiNetDeviceSendCallback callback);
	uint32_t SelectInterface(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber);
	//------------------------------------------------------------------------------------------------------------------

	void ResetStatistics();
	void SetProposed(bool cmd);

	uint32_t GetNumAcksReceived(uint32_t ifIndex);
	uint32_t GetNumAcksMissed(uint32_t ifIndex);

protected:
    virtual void DoDispose(void);
    virtual void DoInitialize(void);
    virtual void ForwardUp(Ptr<Packet> packet, Mac48Address from, Mac48Address to);

private:
    static const uint16_t MAX_MSDU_SIZE = 2304;

    void LinkUp(void);
    void LinkDown(void);

    Ptr<Node> m_node;                   // node that this netdevice is attached to
	uint32_t tx_radio;

	//------------------------------------------------------------------------------------------------------------------
	// slave network interfaces
    uint16_t m_numRadios;
    std::vector<Ptr<WifiNetDevice> > m_devices;
	//------------------------------------------------------------------------------------------------------------------

    NetDevice::ReceiveCallback m_forwardUp;
    NetDevice::PromiscReceiveCallback m_promiscRx;

	//------------------------------------------------------------------------------------------------------------------
    // internal variables
    uint32_t m_ifIndex;
    bool m_linkUp;
    TracedCallback<> m_linkChanges;
    mutable uint16_t m_mtu;
	//------------------------------------------------------------------------------------------------------------------

	// statistics
	uint32_t num_acks_received[8];
	uint32_t num_acks_missed[8];
	bool m_proposed;
	bool m_token;
	uint32_t m_debug;	
};

}   // namespace ns3

#endif

