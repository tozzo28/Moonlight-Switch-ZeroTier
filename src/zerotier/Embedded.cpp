#include "Embedded.hpp"
#include "StateStore.hpp"
#include "Logger.hpp"
#include <switch.h>
#include <new>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
extern "C" {
int __real_socket(int,int,int);
int __real_bind(int,const sockaddr*,socklen_t);
int __real_close(int);
ssize_t __real_sendto(int,const void*,size_t,int,const sockaddr*,socklen_t);
ssize_t __real_recvfrom(int,void*,size_t,int,sockaddr*,socklen_t*);
int __real_poll(pollfd*,nfds_t,int);
char *ztnx_genmem_alloc() { char *p=(char*)malloc(2097152); if(!p) throw std::bad_alloc(); return p; }
void ztnx_genmem_free(char *p) { free(p); }
void ztnx_secure_random_fill(void *p, unsigned long n) { randomGet(p,n); }
}
namespace embedded {
static Runtime state;
static Thread worker;
static bool workerCreated = false;
static int networkStatus = -1;
static uint64_t nodeAddress = 0;
Status status() {
    std::lock_guard<std::mutex> guard(state.lock);
    return {state.running.load(), networkStatus, nodeAddress, state.vnet.localIp()};
}
static unsigned wireTx = 0, wireRx = 0, wireErrors = 0;
Runtime &runtime() { return state; }
uint32_t ticks() { return (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static int64_t now() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(); }
static constexpr const char *StateRoot="sdmc:/switch/moonlight/zerotier";
static int get(ZT_Node*,void*,void*,ZT_StateObjectType t,const uint64_t id[2],void *data,unsigned cap) {
    return readState(StateRoot,t,id,data,cap);
}
static void put(ZT_Node*,void*,void*,ZT_StateObjectType t,const uint64_t id[2],const void *data,int n) {
    if(!writeState(StateRoot,t,id,data,n)) Logger::error("ZeroTier","state persistence failed (type=%u)",(unsigned)t);
}
static int wire(ZT_Node*,void*,void*,int64_t,const sockaddr_storage *to,const void *data,unsigned n,unsigned) {
    if(to->ss_family!=AF_INET) return -1;
    if(__real_sendto(state.wire,data,n,0,(const sockaddr*)to,sizeof(sockaddr_in))==(ssize_t)n){++wireTx;return 0;}
    ++wireErrors;return -1;
}
static void emit(void*,uint64_t mac,uint16_t type,const void *p,unsigned n) {
    if(state.output.size()>=1024) return;
    const auto *b=(const unsigned char*)p;
    state.output.push_back({mac,type,{b,b+n}});
}
static void frame(ZT_Node*,void*,void*,uint64_t,void**,uint64_t src,uint64_t dst,unsigned type,unsigned,const void *data,unsigned n) {
    std::lock_guard<std::mutex> guard(state.lock);
    state.vnet.onFrame(src,dst,type,data,n);
}
static void resetNetwork() {
    ++state.generation;
    state.output.clear();
    state.vnet.reset();
    state.tcp->init([](void*,uint32_t ip,uint8_t proto,const uint8_t *p,unsigned n){state.vnet.sendIp(ip,proto,p,n);return true;},nullptr,ticks);
}
static int config(ZT_Node*,void*,void*,uint64_t,void**,ZT_VirtualNetworkConfigOperation op,const ZT_VirtualNetworkConfig *c) {
    if(c) Logger::info("ZeroTier","network status=%d addresses=%u MTU=%u",(int)c->status,c->assignedAddressCount,c->mtu);
    std::lock_guard<std::mutex> guard(state.lock);
    if(op==ZT_VIRTUAL_NETWORK_CONFIG_OPERATION_DOWN || op==ZT_VIRTUAL_NETWORK_CONFIG_OPERATION_DESTROY){networkStatus=-1;resetNetwork();return 0;}
    if(!c) return 0;
    networkStatus=c->status;
    bool assigned=false;
    for(unsigned i=0;i<c->assignedAddressCount;i++) {
        auto *a=(const sockaddr_in*)&c->assignedAddresses[i]; if(a->sin_family!=AF_INET) continue;
        unsigned bits=ntohs(a->sin_port); if(bits>32) continue;
        uint32_t ip=ntohl(a->sin_addr.s_addr),mask=bits?0xffffffffU<<(32-bits):0;
        if(state.vnet.localIp()!=ip || state.vnet.netmask()!=mask) resetNetwork();
        state.vnet.configure(c->mac,ip,mask, c->mtu<1500?c->mtu:1500);
        state.tcp->setAddress(ip,mask);state.subscribe=true;assigned=true;
        break;
    }
    if(!assigned){resetNetwork();}
    return 0;
}
static void event(ZT_Node*,void*,void*,ZT_Event e,const void*) { Logger::info("ZeroTier","event=%d",(int)e); }
static void writeStatus() {
    ZT_NodeStatus status={};
    ZT_Node_status(state.node,&status);
    uint32_t ip;ztnx::net::Stats stats;unsigned conns;
    {
        std::lock_guard<std::mutex> guard(state.lock);
        ip=state.vnet.localIp();stats=state.vnet.stats();conns=state.tcp->connsInUse();
    }
    FILE *f=fopen("sdmc:/switch/moonlight/zerotier/status.txt","w");
    if(!f)return;
    fprintf(f,"node=%010llx\nnetwork=%016llx\nonline=%d\nnetwork_status=%d\nipv4=%u.%u.%u.%u\n"
              "wire_tx=%u\nwire_rx=%u\nwire_errors=%u\nframes_rx=%u\nudp_rx=%u\nudp_tx=%u\n"
              "tcp_rx=%u\ntcp_tx=%u\ntcp_connections=%u\ndrop_buffer=%u\ndrop_checksum=%u\nupdated_ms=%lld\n",
        (unsigned long long)ZT_Node_address(state.node),(unsigned long long)state.network,status.online,networkStatus,
        ip>>24,(ip>>16)&255,(ip>>8)&255,ip&255,wireTx,wireRx,wireErrors,stats.rxFrames,stats.rxUdp,stats.txUdp,
        stats.rxTcp,stats.txTcp,conns,stats.dropNoBuffer,stats.dropBadChecksum,(long long)now());
    fclose(f);
}
static void loop() {
    ZT_Node_Callbacks cb={}; cb.version=0; cb.stateGetFunction=get;cb.statePutFunction=put;cb.wirePacketSendFunction=wire;
    cb.virtualNetworkFrameFunction=frame;cb.virtualNetworkConfigFunction=config;cb.eventCallback=event;
    ZT_Node_Config cfg={};
    auto rc=ZT_Node_new(&state.node,&cfg,nullptr,nullptr,&cb,now());
    if(rc!=ZT_RESULT_OK){Logger::error("ZeroTier","node creation failed=%d",(int)rc);state.running=false;return;}
    {std::lock_guard<std::mutex> guard(state.lock);nodeAddress=ZT_Node_address(state.node);}
    Logger::info("ZeroTier","node=%010llx network=%016llx",(unsigned long long)ZT_Node_address(state.node),(unsigned long long)state.network);
    rc=ZT_Node_join(state.node,state.network,nullptr,nullptr);
    if(rc!=ZT_RESULT_OK){Logger::error("ZeroTier","join failed=%d",(int)rc);state.running=false;}
    int64_t deadline=0;uint32_t lastStatus=ticks()-5000,lastProbe=ticks()-1000;
    while(state.running) {
        pollfd p={state.wire,POLLIN,0};__real_poll(&p,1,2);
        if(p.revents&POLLIN) {
            for(int i=0;i<64;i++) {
                unsigned char packet[65536];sockaddr_storage from={};socklen_t len=sizeof(from);
                auto n=__real_recvfrom(state.wire,packet,sizeof(packet),MSG_DONTWAIT,(sockaddr*)&from,&len);
                if(n<0) break;
                ++wireRx;
                ZT_Node_processWirePacket(state.node,nullptr,now(),0,&from,packet,n,&deadline);
            }
        }
        if(now()>=deadline) ZT_Node_processBackgroundTasks(state.node,nullptr,now(),&deadline);
        std::deque<Frame> out; bool sub;
        {std::lock_guard<std::mutex> guard(state.lock);state.tcp->poll();
            if(ticks()-lastProbe>=1000 && state.vnet.configured()){state.vnet.sendEchoRequest(state.peer);lastProbe=ticks();}
            out.swap(state.output);sub=state.subscribe;state.subscribe=false;}
        if(sub) {
            ZT_Node_multicastSubscribe(state.node,nullptr,state.network,0xffffffffffffULL,0);
            uint32_t ip;{std::lock_guard<std::mutex> guard(state.lock);ip=state.vnet.localIp();}
            ZT_Node_multicastSubscribe(state.node,nullptr,state.network,0xffffffffffffULL,ip);
        }
        if(ticks()-lastStatus>=5000){writeStatus();lastStatus=ticks();}
        for(auto &f:out) ZT_Node_processVirtualNetworkFrame(state.node,nullptr,now(),state.network,state.vnet.localMac(),f.mac,f.type,0,f.data.data(),f.data.size(),&deadline);
    }
    ZT_Node_delete(state.node);state.node=nullptr;
}
unsigned streamPacketSize(const std::string& address) {
    uint32_t ip = 0;
    return state.running && parsePeer(address, ip) && ip == state.peer ? 1024 : 1392;
}
bool start(const Options& options) {
    // Options are applied once per launch, before any networking workers start.
    // Keeping the lifecycle unchanged avoids tearing down sockets during a stream.
    if (workerCreated) return false;
    if (!options.enabled) {setTransportEnabled(false);return true;}
    uint64_t network;
    uint32_t peer;
    if (!parseNetwork(options.network, network) || !parsePeer(options.peer, peer)) return false;
    state.network=network;state.peer=peer;
    mkdir("sdmc:/switch",0777);mkdir("sdmc:/switch/moonlight",0777);mkdir("sdmc:/switch/moonlight/zerotier",0700);
    FILE *status=fopen("sdmc:/switch/moonlight/zerotier/status.txt","w");
    if(status){fputs("starting=1\n",status);fclose(status);}
    state.wire=__real_socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP); if(state.wire<0)return false;
    sockaddr_in a={};a.sin_family=AF_INET;a.sin_port=htons(9993);
    if(__real_bind(state.wire,(sockaddr*)&a,sizeof(a))<0){a.sin_port=0;if(__real_bind(state.wire,(sockaddr*)&a,sizeof(a))<0){__real_close(state.wire);state.wire=-1;return false;}}
    state.vnet.reset();state.vnet.setEmit(emit,nullptr);
    state.tcp->init([](void*,uint32_t ip,uint8_t proto,const uint8_t *p,unsigned n){state.vnet.sendIp(ip,proto,p,n);return true;},nullptr,ticks);
    state.vnet.setTcpInput([](void*,uint32_t src,uint32_t dst,const uint8_t *p,unsigned n){state.tcp->input(src,dst,p,n);},nullptr);
    setTransportEnabled(true);
    state.running=true;
    Result rc=threadCreate(&worker,[](void*){
        try {loop();} catch(...) {
            Logger::error("ZeroTier","worker exception; transport stopped");
            state.running=false;
            if(state.node){ZT_Node_delete(state.node);state.node=nullptr;}
        }
    },nullptr,nullptr,1024*1024,0x2c,-2);
    if(R_SUCCEEDED(rc)) {workerCreated=true;rc=threadStart(&worker);}
    if(R_FAILED(rc)){if(workerCreated){threadClose(&worker);workerCreated=false;}state.running=false;setTransportEnabled(false);__real_close(state.wire);state.wire=-1;return false;}
    return true;
}
void stop(){state.running=false;if(workerCreated){threadWaitForExit(&worker);threadClose(&worker);workerCreated=false;}if(state.wire>=0)__real_close(state.wire);state.wire=-1;setTransportEnabled(false);}
}
