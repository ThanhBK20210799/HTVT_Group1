#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
 
using namespace ns3;
 
int main() {
    // Tạo nodes
    NodeContainer nodes;
    nodes.Create(2);
 
    // Tạo kênh truyền
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("400Kbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("1ms"));
 
    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);
 
    Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
    em->SetAttribute("ErrorRate", DoubleValue(0.0001));
    devices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));
 
    // Cài đặt giao thức TCP
    InternetStackHelper stack;
    stack.Install(nodes);
 
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);
 
    // Tạo ứng dụng truyền dữ liệu
    uint16_t port = 9;
    BulkSendHelper source("ns3::TcpSocketFactory", InetSocketAddress(interfaces.GetAddress(1), port));
    source.SetAttribute("MaxBytes", UintegerValue(0));
    ApplicationContainer sourceApps = source.Install(nodes.Get(0));
    sourceApps.Start(Seconds(1.0));
    sourceApps.Stop(Seconds(10.0));
 
    // Cài đặt ứng dụng nhận dữ liệu
    PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApps = sink.Install(nodes.Get(1));
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(10.0));
 
    // Theo dõi luồng dữ liệu
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
 
    // Bắt đầu mô phỏng
    Simulator::Stop(Seconds(10.0));
    Simulator::Run();
 
    // In ra thông tin
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i) {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
        if (t.sourceAddress == "10.1.1.1" && t.destinationAddress == "10.1.1.2") {
            std::cout << "Tx Packets: " << i->second.txPackets << std::endl;
            std::cout << "Tx Bytes: " << i->second.txBytes << std::endl;
            std::cout << "TxOffered: " << i->second.txBytes * 8.0 / 10000 << " Kbps" << std::endl;
            std::cout << "Rx Packets: " << i->second.rxPackets << std::endl;
            std::cout << "Rx Bytes: " << i->second.rxBytes << std::endl;
            std::cout << "Throughput: " << i->second.rxBytes * 8.0 / 10000 << " Kbps" << std::endl;
        }
    }
 
    uint64_t totalBytes = DynamicCast<PacketSink>(sinkApps.Get(0))->GetTotalRx();
    double goodput = totalBytes * 8.0 / 1000.0 / 10.0; // Kbps
    NS_LOG_UNCOND("Goodput: " << goodput << " Kbps");
 
    Simulator::Destroy();
    return 0;
}
 