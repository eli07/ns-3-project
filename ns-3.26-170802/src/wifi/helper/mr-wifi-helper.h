#ifndef MR_WIFI_HELPER_H
#define MR_WIFI_HELPER_H

#include <string>
#include "ns3/attribute.h"
#include "ns3/object-factory.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/wifi-phy-standard.h"
#include "ns3/trace-helper.h"
#include "wifi-helper.h"

namespace ns3 {

class WifiPhy;
class WifiMac;
class WifiNetDevice;
class Node;

class MrWifiHelper
{
public:
    virtual ~MrWifiHelper();
    MrWifiHelper();

    static MrWifiHelper Default(void);

    void SetRemoteStationManager (std::string type,
                              std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
                              std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
                              std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
                              std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
                              std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue (),
                              std::string n5 = "", const AttributeValue &v5 = EmptyAttributeValue (),
                              std::string n6 = "", const AttributeValue &v6 = EmptyAttributeValue (),
                              std::string n7 = "", const AttributeValue &v7 = EmptyAttributeValue ());

    virtual NetDeviceContainer Install (const WifiPhyHelper &phy,
                              const WifiMacHelper &mac, NodeContainer c) const;

    virtual NetDeviceContainer Install (const WifiPhyHelper &phy,
                              const WifiMacHelper &mac, Ptr<Node> node) const;

    virtual NetDeviceContainer Install (const WifiPhyHelper &phy,
                              const WifiMacHelper &mac, std::string nodeName) const;

    virtual void SetStandard(enum WifiPhyStandard standard);

    virtual void SetNumRadios(uint16_t numRadios);

    static void EnableLogComponents (void);

    //int64_t AssignStreams(NetDeviceContainer c, int64_t stream);

protected:
    ObjectFactory m_stationManager;
    enum WifiPhyStandard m_standard;
    uint16_t m_numRadios;
};

} // namespace ns3
#endif // MR_WIFI_HELPER_H

