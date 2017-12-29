#include "mr-wifi-helper.h"
#include "wifi-helper.h"
#include "ns3/mr-wifi-net-device.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-mac.h"
#include "ns3/regular-wifi-mac.h"
#include "ns3/dca-txop.h"
#include "ns3/edca-txop-n.h"
#include "ns3/minstrel-wifi-manager.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-remote-station-manager.h"
#include "ns3/wifi-channel.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include "ns3/config.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/names.h"

NS_LOG_COMPONENT_DEFINE ("MrWifiHelper");

namespace ns3 {

MrWifiHelper::MrWifiHelper() {
    m_standard = WIFI_PHY_STANDARD_80211a;
    m_numRadios = 1;        // default: single radio
}

MrWifiHelper::~MrWifiHelper() { }

MrWifiHelper
MrWifiHelper::Default (void)
{
  MrWifiHelper helper;
  helper.SetRemoteStationManager ("ns3::ArfWifiManager");
  return helper;
}

void
MrWifiHelper::SetRemoteStationManager (std::string type,
                                     std::string n0, const AttributeValue &v0,
                                     std::string n1, const AttributeValue &v1,
                                     std::string n2, const AttributeValue &v2,
                                     std::string n3, const AttributeValue &v3,
                                     std::string n4, const AttributeValue &v4,
                                     std::string n5, const AttributeValue &v5,
                                     std::string n6, const AttributeValue &v6,
                                     std::string n7, const AttributeValue &v7)
{
  m_stationManager = ObjectFactory ();
  m_stationManager.SetTypeId (type);
  m_stationManager.Set (n0, v0);
  m_stationManager.Set (n1, v1);
  m_stationManager.Set (n2, v2);
  m_stationManager.Set (n3, v3);
  m_stationManager.Set (n4, v4);
  m_stationManager.Set (n5, v5);
  m_stationManager.Set (n6, v6);
  m_stationManager.Set (n7, v7);
}

void
MrWifiHelper::SetStandard (enum WifiPhyStandard standard)
{
  m_standard = standard;
}

void
MrWifiHelper::SetNumRadios(uint16_t numRadios) {
    NS_ASSERT(numRadios > 0);
    m_numRadios = numRadios;
}

NetDeviceContainer
MrWifiHelper::Install (const WifiPhyHelper &phy,
                     const WifiMacHelper &mac, Ptr<Node> node) const
{
  return Install (phy, mac, NodeContainer (node));
}

NetDeviceContainer
MrWifiHelper::Install (const WifiPhyHelper &phy,
                     const WifiMacHelper &mac, std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return Install (phy, mac, NodeContainer (node));
}

NetDeviceContainer
MrWifiHelper::Install(const WifiPhyHelper &phyHelper,
                     const WifiMacHelper &macHelper, NodeContainer c) const
{
    NetDeviceContainer devices;

    for(NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
        Ptr<Node> node = *i;

        // create MrWifiNetDevice
        Ptr<MrWifiNetDevice> mrdevice = CreateObject<MrWifiNetDevice>();

        // for now, use a common address for all radios
        Mac48Address addr = Mac48Address::Allocate();

        for(int i=0; i<m_numRadios; i++) {

            // create WifiNetDevice
            Ptr<WifiNetDevice> device = CreateObject<WifiNetDevice>();

            // create WifiRemoteStationManager, WifiMac, and WifiPhy
            Ptr<WifiRemoteStationManager> manager = m_stationManager.Create<WifiRemoteStationManager> ();
            Ptr<WifiMac> mac = macHelper.Create ();
            Ptr<WifiPhy> phy = phyHelper.Create (node, device);

            // assign parameters to MAC and PHY
            mac->SetAddress(addr);
            mac->ConfigureStandard(m_standard);
            phy->ConfigureStandard(m_standard);

            device->SetMac(mac);
            device->SetPhy(phy);
            device->SetRemoteStationManager(manager);

            device->SetNode(node);
            device->SetIfIndex(i);

            // add this WifiNetDevice to MrWifiNetDevice
            mrdevice->AddRadio(device);
        }

        // connect NetDevice to the node
        node->AddDevice(mrdevice);

        // connect NetDevice to the NetDeviceContainer
        devices.Add(mrdevice);

        NS_LOG_DEBUG ("node=" << node << ", mob=" << node->GetObject<MobilityModel> ());
    }

    return devices;
}

void
MrWifiHelper::EnableLogComponents(void) {

}

} // namespace ns3

