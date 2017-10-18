#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/csma-module.h>
#include <ns3/internet-module.h>
#include <ns3/applications-module.h>
#include <ns3/internet-apps-module.h>
#include <ns3/ofswitch13-module.h>
#include <ns3/traffic-control-helper.h>
#include <ns3/traffic-control-helper.h>
#include "hederautil.h"
#include "routing.h"
#include "hedera-controller.h"
#include "ripl.h"
#include <stdio.h>


using namespace ns3;

typedef std::unordered_map<std::string, int>  NameIdMap;
typedef std::unordered_map<std::string, std::vector<std::pair< Ptr<NetDevice>, uint32_t>> > NodePortMap;
typedef std::vector<std::pair<int, uint32_t>> TrafficMatrix;

NameIdMap
map_name_to_id(FatTreeTopo topo)
{
    NameIdMap name_id;
    std::vector<std::string> hosts = topo.layer_nodes(LAYER_HOST);
    std::vector<std::string> aggs = topo.layer_nodes(LAYER_AGG);
    std::vector<std::string> edges = topo.layer_nodes(LAYER_EDGE);
    std::vector<std::string> cores = topo.layer_nodes(LAYER_CORE);
    int i = 0;
    for (auto h: hosts ){
        name_id[h] = i;
        i += 1;
    }

    i = 0;
    for(auto sw: aggs){
        name_id[sw] = i;
        i += 1; 
    }
    for(auto sw: edges){
        name_id[sw] = i;
        i += 1; 
    }
    for(auto sw: cores){
        name_id[sw] = i;
        i += 1; 
    }

    return name_id;
}

/* Uglish */
NodePortMap 
create_links(FatTreeTopo topo, NameIdMap name_id, 
             NodeContainer switches, NodeContainer hosts, 
             CsmaHelper csmaHelper)
{
    NodePortMap node_port_map;
    NetDeviceContainer link;
    // Use the CsmaHelper to connect host nodes to the switch node

    std::vector<Link> links = topo.bidirectional_links;
    for (auto l: links){
        std::pair<std::string, uint32_t> src = l.first;
        std::pair<std::string, uint32_t> dst = l.second;
        std::string src_name, dst_name;
        uint32_t src_port, dst_port;
        src_name = src.first;
        src_port = src.second;
        dst_name = dst.first;
        dst_port = dst.second;
        int id_src = name_id[src_name];
        int id_dst = name_id[dst_name];
        // Two hosts
        if (topo.layer(src_name).compare(LAYER_HOST) == 0 &&
            topo.layer(dst_name).compare(LAYER_HOST) == 0) {

            link = csmaHelper.Install (NodeContainer (hosts.Get(id_src), hosts.Get (id_dst)));
        }
        // Src is host/ dst is switch
        else if (topo.layer(src_name).compare(LAYER_HOST) == 0 &&
            topo.layer(dst_name).compare(LAYER_HOST) != 0){
            link = csmaHelper.Install (NodeContainer (hosts.Get (id_src), switches.Get (id_dst)));
        }
        // Dst is host/ src is switch
        else if (topo.layer(src_name).compare(LAYER_HOST) != 0 &&
            topo.layer(dst_name).compare(LAYER_HOST) == 0) {
            link = csmaHelper.Install (NodeContainer (switches.Get (id_src), hosts.Get (id_dst)));
        }
        // Two switches
        else {
            link = csmaHelper.Install (NodeContainer (switches.Get (id_src), switches.Get (id_dst)));
        }
        node_port_map[src_name].push_back(std::make_pair(link.Get(0), src_port));
        node_port_map[dst_name].push_back(std::make_pair(link.Get(1), dst_port));
    }
    return node_port_map;
}

TrafficMatrix create_flows(uint32_t k, std::string pattern, int fph)
{
    TrafficMatrix traffic;
    std::vector<std::string> args;
    split(pattern, args, ',');
    std::string type = args[0];
    for (int fcount = 0; fcount < fph; ++fcount){
        std::vector<uint32_t> matrix;
        if (type.compare("stride") == 0){
           uint32_t stride_amt = stoul(args[1]);
           matrix = compute_stride(k, stride_amt);
        }
        else if (type.compare("stag") == 0){
            double edge_prob = atof(args[1].c_str());
            double pod_prob = atof(args[2].c_str()); 
            matrix = compute_stagger_prob(k, edge_prob, pod_prob);
        }
        else if (type.compare("random") == 0){
            matrix = compute_random(k);
        }
        else if (type.compare("randbij") == 0){
            matrix = compute_randbij(k);
        }
        else {
            std::cout << "Unknown traffic pattern\n";
            exit(0); 
        }
        for (size_t i = 0; i < matrix.size(); ++i){
            traffic.push_back(std::make_pair(i, matrix[i]));  
        } 
    }
    return traffic;
}

int
main (int argc, char *argv[])
{

    uint16_t simTime = 60;
    bool hedera = false;
    bool verbose = false;
    bool trace = false;
    int k = 4;
    char traffic[]= "stag,0.2,0.3";
    int flowPerHost = 1;
    int seed = 1111;
    int trial = 1;
    char routing[] = "random";

    // Configure command line parameters
    CommandLine cmd;
    cmd.AddValue ("simTime", "Simulation time (seconds)", simTime);
    cmd.AddValue ("k", "FatTree size", k);
    cmd.AddValue ("t", "Traffic pattern", traffic);
    cmd.AddValue ("flowPerHost", "Number of flows per host", flowPerHost);
    cmd.AddValue ("s", "Seed", seed);
    cmd.AddValue ("hedera", "Run Hedera Instead of RIPL", hedera);
    cmd.AddValue ("r", "routing", routing);
    cmd.AddValue ("trial", "Trial", trial);
    cmd.AddValue ("verbose", "Enable verbose output", verbose);
    cmd.AddValue ("trace", "Enable datapath stats and pcap traces", trace);
    cmd.Parse (argc, argv);


    if (verbose)
    {
      OFSwitch13Helper::EnableDatapathLogs ();
      // LogComponentEnable ("OFSwitch13Interface", LOG_LEVEL_ALL);
      // LogComponentEnable ("OFSwitch13Device", LOG_LEVEL_ALL);
      // LogComponentEnable ("OFSwitch13Port", LOG_LEVEL_ALL);
      // LogComponentEnable ("OFSwitch13Queue", LOG_LEVEL_ALL);
      // LogComponentEnable ("OFSwitch13SocketHandler", LOG_LEVEL_ALL);
      // LogComponentEnable ("OFSwitch13Controller", LOG_LEVEL_ALL);
      LogComponentEnable ("RIPLController", LOG_LEVEL_ALL);
      // LogComponentEnable ("OFSwitch13Helper", LOG_LEVEL_ALL);
      // LogComponentEnable ("OFSwitch13InternalHelper", LOG_LEVEL_ALL);
      // LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
      // LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);
    }

    // Configure dedicated connections between controller and switches
    // Config::SetDefault ("ns3::OFSwitch13Helper::ChannelType", EnumValue (OFSwitch13Helper::DEDICATEDCSMA));

    // Enable checksum computations (required by OFSwitch13 module)
    GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

    FatTreeTopo topo = ns3::FatTreeTopo::build(k, 1);
    NameIdMap name_id = map_name_to_id(topo);

    NodeContainer hosts, switches;
    hosts.Create (topo.nhosts);
    switches.Create(topo.nswitches);

    CsmaHelper csmaHelper;
    csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("10Mbps")));
    csmaHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (1)));
    NodePortMap node_ports = create_links(topo, name_id, switches, 
                                          hosts, csmaHelper);    


    // Create the controller node
    Ptr<Node> controllerNode = CreateObject<Node> ();

    Ptr<OFSwitch13InternalHelper> OFHelper =
    CreateObject<OFSwitch13InternalHelper> ();
    
    if (hedera) {
        Ptr<HederaController> hederaCtrl = CreateObject<HederaController> ();
        OFHelper->InstallController (controllerNode, hederaCtrl);
    }
    else {
        Ptr<RIPLController> riplCtrl = CreateObject<RIPLController> (routing, topo);
        OFHelper->InstallController (controllerNode, riplCtrl); 
    }

    // Add the switches
    OFSwitch13DeviceContainer ofSwitchDevices;
    for(auto node: name_id) {
        std::string node_name = node.first;
        int id = node.second;
        // Only create if it is not a host
        if (topo.layer(node_name).compare(LAYER_HOST) != 0){
            char *end;
            uint64_t dpid = std::strtoull(topo.node_info[node_name]["dpid"].c_str(), &end, 16);
            ofSwitchDevices.Add (OFHelper->InstallSwitch (switches.Get (id), dpid, node_ports[node_name]));
        }
    }
    OFHelper->CreateOpenFlowChannels ();

    //Install the TCP/IP stack into hosts nodes
    InternetStackHelper internet;
    internet.Install (hosts);

    Ipv4InterfaceContainer ipIfaces;
    Ipv4AddressHelper ipv4Helper;
    Ipv4InterfaceContainer hostIpIfaces;
    for(auto node: name_id) {
        std::string node_name = node.first;

        // Only assign if it is  host
        if (topo.layer(node_name).compare(LAYER_HOST) == 0){
            std::string mac = topo.node_info[node_name]["mac"];
            std::string ip = topo.node_info[node_name]["ip"];
            node_ports[node_name][0].first->SetAddress(Mac48Address(mac.c_str()));

            Ptr<Node> node = node_ports[node_name][0].first->GetNode ();
            Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
            int32_t interface = ipv4->GetInterfaceForDevice (node_ports[node_name][0].first);
            if (interface == -1)
            {
                interface = ipv4->AddInterface (node_ports[node_name][0].first);
            }
            Ipv4InterfaceAddress ipv4Addr = Ipv4InterfaceAddress (Ipv4Address(ip.c_str()), Ipv4Mask("255.0.0.0"));
            ipv4->AddAddress (interface, ipv4Addr);
            ipv4->SetMetric (interface, 1);
            ipv4->SetUp (interface);
            ipIfaces.Add (ipv4, interface);
            Ptr<TrafficControlLayer> tc = node->GetObject<TrafficControlLayer> ();
            if (tc && DynamicCast<LoopbackNetDevice> (node_ports[node_name][0].first) == 0 && tc->GetRootQueueDiscOnDevice (node_ports[node_name][0].first) == 0) {
             // NS_LOG_LOGIC ("Installing default traffic control   configuration");
                TrafficControlHelper tcHelper = 
                                            TrafficControlHelper::Default ();
                tcHelper.Install (node_ports[node_name][0].first);
           }
            // std::cout << node_name << ":" << ipIfaces.Get (interface-1).second;
        }
    }

    TrafficMatrix traffic_matrix = create_flows(k, traffic, flowPerHost);

    /* Receivers */
    uint16_t port = 5001;
    ApplicationContainer apps;
    ApplicationContainer app;
    for (auto dst: traffic_matrix) {
        std::string dest_host_name = get_host_name(k, dst.second);       
        UdpServerHelper server (port);
        app = server.Install (hosts.Get (name_id[dest_host_name]));
        apps.Add(app.Get(0));
    } 
    apps.Start (Seconds (1.0));
    apps.Stop (Seconds (simTime));
    ApplicationContainer client_apps;
    /* Senders */
    uint32_t MaxPacketSize = 1024;
    Time interPacketInterval = Seconds (0.05);
    uint32_t maxPacketCount = 320;
    for (auto t: traffic_matrix){
        int src_index = t.first;
        uint32_t dest_index = t.second;
        std::string dest_host_name = get_host_name(k, dest_index);
        std::string src_name = get_host_name(k, src_index);
        std::string dst_ip = topo.node_info[dest_host_name]["ip"];
        UdpClientHelper client (Address(Ipv4Address(dst_ip.c_str())), port);
        client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
        client.SetAttribute ("Interval", TimeValue (interPacketInterval));
        client.SetAttribute ("PacketSize", UintegerValue (MaxPacketSize));
        app = client.Install (hosts.Get (name_id[src_name]));
        client_apps.Add(app.Get(0));
    }

    client_apps.Start (Seconds (2.0));
    client_apps.Stop (Seconds (simTime));
    // Enable datapath stats and pcap traces at hosts, switch(es), and controller(s)
    if (trace)
    {
      OFHelper->EnableOpenFlowPcap ("openflow");
      OFHelper->EnableDatapathStats ("switch-stats");
      // csmaHelper.EnablePcap ("switch", switchPorts, true);
      // csmaHelper.EnablePcap ("switch", switches, true);
      // csmaHelper.EnablePcap ("host", hosts);
    }

    // Run the simulation
    Simulator::Stop (Seconds (simTime));
    Simulator::Run ();
    Simulator::Destroy ();
    std::cout << "Done.\n";
    return 0;
}