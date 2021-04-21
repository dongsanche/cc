/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef MYSERVER_HELPER_H
#define MYSERVER_HELPER_H

#include "ns3/object-factory.h"
#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/ipv4-routing-helper.h"
#include <map>
#include <set>
#include "ns3/myserver.h"

namespace ns3 {

class MyServerHelper : public Ipv4RoutingHelper
{
public:
    MyServerHelper ();
    MyServerHelper (const MyServerHelper &);
    MyServerHelper* Copy (void) const;
    virtual Ptr<Ipv4RoutingProtocol> Create (Ptr<Node> node) const;
    void Set (std::string name, const AttributeValue &value);
    void Install (NodeContainer c) const;

private:
    MyServerHelper &operator = (const MyServerHelper &);
    ObjectFactory m_agentFactory;
};

}

#endif /* MYSERVER_HELPER_H */

