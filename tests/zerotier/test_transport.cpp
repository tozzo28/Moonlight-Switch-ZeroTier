// Exercises the production socket wrappers with an in-memory Ethernet peer.
// No ZeroTier controller or Switch services are simulated by this test.
#include "Embedded.hpp"
#include "StateStore.hpp"
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <chrono>
#ifdef TEST_MODERN_ENET
#include <enet/enet.h>
#include <thread>
extern "C" {
int __real_socket(int,int,int);
int __real_bind(int,const sockaddr*,socklen_t);
int __real_close(int);
ssize_t __real_sendto(int,const void*,size_t,int,const sockaddr*,socklen_t);
ssize_t __real_recvfrom(int,void*,size_t,int,sockaddr*,socklen_t*);
}
#endif
using namespace embedded;
using namespace ztnx::net;
namespace embedded {
Runtime &runtime(){static Runtime r;return r;}
uint32_t ticks(){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
}
// This constructor runs before Transport.cpp's map constructor in this test's
// link order, mirroring libnx service setup before application initialization.
struct EarlySocket {
    EarlySocket(){int fd=socket(AF_INET,SOCK_DGRAM,0);assert(fd>=0);assert(close(fd)==0);}
};
static EarlySocket earlySocket;
static VNet peer;
static std::deque<Frame> inbound;
static uint32_t localIp=0xc0000209; // synthetic TEST-NET-1 local address
static uint32_t peerIp=0xc0000212; // synthetic TEST-NET-1 peer
static constexpr uint64_t localMac=0x012233445566, peerMac=0x023344556677;
static uint32_t serverSeq=4000;
static void reply(uint16_t port,uint16_t dest,uint32_t ack,unsigned flags,const char *data="") {
    unsigned n=strlen(data);std::vector<uint8_t> p(20+n);
    wr16(p.data(),port);wr16(p.data()+2,dest);wr32(p.data()+4,serverSeq);wr32(p.data()+8,ack);
    p[12]=0x50;p[13]=flags;wr16(p.data()+14,16000);memcpy(p.data()+20,data,n);
    uint32_t sum=(peerIp>>16)+(peerIp&65535)+(localIp>>16)+(localIp&65535)+6+p.size();
    wr16(p.data()+16,checksum16(p.data(),p.size(),sum));
    peer.sendIp(localIp,6,p.data(),p.size());
    serverSeq+=n+!!(flags&2)+!!(flags&1);
}
static void pump() {
    auto &r=runtime();std::lock_guard<std::mutex> guard(r.lock);
    for(int i=0;i<100 && (!r.output.empty() || !inbound.empty());i++){
        auto out=std::move(r.output);r.output.clear();for(auto &f:out)peer.onFrame(localMac,f.mac,f.type,f.data.data(),f.data.size());
        auto in=std::move(inbound);inbound.clear();for(auto &f:in)r.vnet.onFrame(peerMac,f.mac,f.type,f.data.data(),f.data.size());
    }
    r.tcp->poll();
}
static sockaddr_in addr(uint32_t ip,uint16_t port){sockaddr_in a={};a.sin_family=AF_INET;a.sin_addr.s_addr=htonl(ip);a.sin_port=htons(port);return a;}
static int makeUdp(){int fd=socket(AF_INET,SOCK_DGRAM,0);if(fd<0)perror("socket");assert(fd>=0);auto a=addr(0,0);assert(bind(fd,(sockaddr*)&a,sizeof(a))==0);assert(fcntl(fd,F_SETFL,O_NONBLOCK)==0);return fd;}
#ifdef TEST_MODERN_ENET
static void testModernEnet() {
    assert(enet_initialize()==0);
    sockaddr_in local=addr(localIp,0),native=addr(0x7f000001,0),remote=addr(peerIp,47999);
    ENetAddress localAddress={},serverAddress={},remoteAddress={};
    assert(enet_address_set_address(&localAddress,(sockaddr*)&local,sizeof(local))==0);
    ENetHost *client=enet_host_create(AF_INET,&localAddress,1,2,0,0);
    assert(client); // This failed before support for binding the managed IP.
    assert(enet_address_set_address(&serverAddress,(sockaddr*)&native,sizeof(native))==0);
    ENetHost *server=enet_host_create(AF_INET,&serverAddress,1,2,0,0);
    assert(server);
    socklen_t len=sizeof(native);
    assert(getsockname(server->socket,(sockaddr*)&native,&len)==0);
    sockaddr_in virtualLocal={};len=sizeof(virtualLocal);
    assert(getsockname(client->socket,(sockaddr*)&virtualLocal,&len)==0);
    assert(ntohl(virtualLocal.sin_addr.s_addr)==localIp && virtualLocal.sin_port!=0);
    const uint16_t virtualPort=ntohs(virtualLocal.sin_port);
    int virtualServer=peer.udpOpen();assert(virtualServer>=0);
    assert(peer.udpBind(virtualServer,0,47999)==0);
    int bridge=__real_socket(AF_INET,SOCK_DGRAM,0);assert(bridge>=0);
    auto bridgeAddress=addr(0x7f000001,0);
    assert(__real_bind(bridge,(sockaddr*)&bridgeAddress,sizeof(bridgeAddress))==0);
    assert(fcntl(bridge,F_SETFL,O_NONBLOCK)==0);
    assert(enet_address_set_address(&remoteAddress,(sockaddr*)&remote,sizeof(remote))==0);
    ENetPeer *clientPeer=enet_host_connect(client,&remoteAddress,2,0x12345678);
    assert(clientPeer);
    bool clientConnected=false,serverConnected=false,receivedControl=false,receivedReply=false;
    ENetPeer *serverPeer=nullptr;
    auto step=[&] {
        ENetEvent event;
        while(enet_host_service(client,&event,0)>0) {
            if(event.type==ENET_EVENT_TYPE_CONNECT)clientConnected=true;
            if(event.type==ENET_EVENT_TYPE_RECEIVE){receivedReply=event.packet->dataLength==5 && !memcmp(event.packet->data,"reply",5);enet_packet_destroy(event.packet);}
        }
        enet_host_flush(client);pump();
        unsigned char buffer[2048];uint32_t ip;uint16_t port;
        while(peer.udpReadable(virtualServer)) {
            int n=peer.udpRecvFrom(virtualServer,buffer,sizeof(buffer),&ip,&port);
            assert(n>0 && ip==localIp && port==virtualPort);
            assert(__real_sendto(bridge,buffer,n,0,(sockaddr*)&native,sizeof(native))==n);
        }
        while(enet_host_service(server,&event,0)>0) {
            if(event.type==ENET_EVENT_TYPE_CONNECT){serverConnected=true;serverPeer=event.peer;}
            if(event.type==ENET_EVENT_TYPE_RECEIVE){receivedControl=event.packet->dataLength==7 && !memcmp(event.packet->data,"control",7);enet_packet_destroy(event.packet);}
        }
        enet_host_flush(server);
        for(;;) {
            ssize_t n=__real_recvfrom(bridge,buffer,sizeof(buffer),MSG_DONTWAIT,nullptr,nullptr);
            if(n<0){assert(errno==EAGAIN || errno==EWOULDBLOCK);break;}
            assert(peer.udpSendTo(virtualServer,localIp,virtualPort,buffer,n)==n);
        }
        pump();std::this_thread::sleep_for(std::chrono::milliseconds(1));
    };
    for(int i=0;i<2000 && (!clientConnected || !serverConnected);++i)step();
    assert(clientConnected && serverConnected && serverPeer);
    assert(enet_peer_send(clientPeer,0,enet_packet_create("control",7,ENET_PACKET_FLAG_RELIABLE))==0);
    for(int i=0;i<2000 && !receivedControl;++i)step();
    assert(receivedControl);
    assert(enet_peer_send(serverPeer,0,enet_packet_create("reply",5,ENET_PACKET_FLAG_RELIABLE))==0);
    for(int i=0;i<2000 && !receivedReply;++i)step();
    assert(receivedReply);
    enet_host_destroy(client);enet_host_destroy(server);__real_close(bridge);peer.udpClose(virtualServer);
    puts("modern ENet: managed-IP bind, connection handshake and reliable bidirectional control over VNet OK");
}
#endif

int main(int argc, char**) {
    if (argc > 1) {localIp=0xac1b0009;peerIp=0xac1b0012;}
    runtime().peer=peerIp;
    const char *root="/tmp/moonlight-zt-tests/state";mkdir(root,0700);
    uint64_t known[2]={0xabcdef,0},unknown[2]={};char secret[16];
    assert(writeState(root,ZT_STATE_OBJECT_IDENTITY_SECRET,known,"secret",6));
    assert(readState(root,ZT_STATE_OBJECT_IDENTITY_SECRET,unknown,secret,sizeof(secret))==6);
    assert(!memcmp(secret,"secret",6));
    assert(readState(root,ZT_STATE_OBJECT_IDENTITY_SECRET,unknown,secret,2)==-1);
    assert(writeState(root,ZT_STATE_OBJECT_IDENTITY_SECRET,known,nullptr,-1));
    assert(readState(root,ZT_STATE_OBJECT_IDENTITY_SECRET,unknown,secret,sizeof(secret))==-1);
    setTransportEnabled(true);
    auto &r=runtime();r.running=true;r.vnet.reset();r.vnet.configure(localMac,localIp,0xffffff00,1500);
    r.vnet.setEmit([](void*,uint64_t m,uint16_t t,const void *p,unsigned n){auto *b=(const uint8_t*)p;runtime().output.push_back({m,t,{b,b+n}});},nullptr);
    r.tcp->init([](void*,uint32_t ip,uint8_t proto,const uint8_t *p,unsigned n){runtime().vnet.sendIp(ip,proto,p,n);return true;},nullptr,ticks);
    r.tcp->setAddress(localIp,0xffffff00);
    r.vnet.setTcpInput([](void*,uint32_t src,uint32_t dst,const uint8_t *p,unsigned n){runtime().tcp->input(src,dst,p,n);},nullptr);
    peer.reset();peer.configure(peerMac,peerIp,0xffffff00,1500);
    peer.setEmit([](void*,uint64_t m,uint16_t t,const void *p,unsigned n){auto *b=(const uint8_t*)p;inbound.push_back({m,t,{b,b+n}});},nullptr);
    peer.setTcpInput([](void*,uint32_t,uint32_t,const uint8_t *p,unsigned n){
        uint16_t port=rd16(p+2),src=rd16(p);uint32_t seq=rd32(p+4);unsigned hdr=(p[12]>>4)*4;
        if(port==47984 && (p[13]&2)){reply(port,src,seq+1,0x14);return;}
        if(port!=47989)return;
        if(p[13]&2)reply(port,src,seq+1,0x12);
        else if(n>hdr)reply(port,src,seq+n-hdr,0x18,"HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK");
    },nullptr);
    // Warm up ARP, just as the runtime probe does.
    r.vnet.sendEchoRequest(peerIp);pump();
    int udpFd=makeUdp();auto dest=addr(peerIp,47998);
    int ps=peer.udpOpen();assert(ps>=0);assert(peer.udpBind(ps,0,47998)==0);
    char one[]="ab",two[]="cd";iovec parts[]={{one,2},{two,2}};msghdr msg={};msg.msg_name=&dest;msg.msg_namelen=sizeof(dest);msg.msg_iov=parts;msg.msg_iovlen=2;
    assert(sendmsg(udpFd,&msg,0)==4);pump();
    char buf[256];uint32_t ip;uint16_t port;
    assert(peer.udpRecvFrom(ps,buf,sizeof(buf),&ip,&port)==4);assert(!memcmp(buf,"abcd",4));assert(ip==localIp);
    assert(peer.udpSendTo(ps,ip,port,"video",5)==5);pump();
    pollfd pf={udpFd,POLLIN,0};assert(poll(&pf,1,0)==1 && (pf.revents&POLLIN));
    sockaddr_in from={};socklen_t len=sizeof(from);
    assert(recvfrom(udpFd,buf,sizeof(buf),MSG_PEEK,(sockaddr*)&from,&len)==5);assert(ntohl(from.sin_addr.s_addr)==peerIp);
    char first[2],rest[8];iovec output[]={{first,2},{rest,8}};msghdr rx={};rx.msg_iov=output;rx.msg_iovlen=2;
    assert(recvmsg(udpFd,&rx,0)==5);assert(!memcmp(first,"vi",2)&&!memcmp(rest,"deo",3));
    assert(recv(udpFd,buf,sizeof(buf),0)==-1 && errno==EAGAIN);
    // Explicit managed-IP bind, as used by modern ENet after the RTSP socket.
    {
        int bound=socket(AF_INET,SOCK_DGRAM,0);assert(bound>=0);
        auto requested=addr(localIp,0);assert(bind(bound,(sockaddr*)&requested,sizeof(requested))==0);
        socklen_t boundLen=sizeof(requested);assert(getsockname(bound,(sockaddr*)&requested,&boundLen)==0);
        assert(ntohl(requested.sin_addr.s_addr)==localIp && requested.sin_port!=0);
        assert(getpeername(bound,(sockaddr*)&from,&len)==-1 && errno==ENOTCONN);
        assert(send(bound,"x",1,0)==-1 && errno==EDESTADDRREQ);
        assert(sendto(bound,"bound",5,0,(sockaddr*)&dest,sizeof(dest))==5);pump();
        uint32_t sourceIp;uint16_t sourcePort;
        assert(peer.udpRecvFrom(ps,buf,sizeof(buf),&sourceIp,&sourcePort)==5);
        assert(sourceIp==localIp && sourcePort==ntohs(requested.sin_port));
        assert(peer.udpSendTo(ps,sourceIp,sourcePort,"ok",2)==2);pump();
        assert(recv(bound,buf,sizeof(buf),0)==2 && !memcmp(buf,"ok",2));
        close(bound);
    }
#ifdef TEST_MODERN_ENET
    testModernEnet();
#endif
    // Native UDP still exchanges data alongside the virtual socket.
    int native=makeUdp();auto loop=addr(0x7f000001,0);len=sizeof(loop);assert(getsockname(native,(sockaddr*)&loop,&len)==0);
    assert(sendto(udpFd,"wifi",4,0,(sockaddr*)&loop,sizeof(loop))==4);
    assert(peer.udpSendTo(ps,ip,port,"audio",5)==5);pump();
    pollfd mixed[]={{udpFd,POLLIN,0},{native,POLLIN,0}};
    assert(poll(mixed,2,0)==2);
    assert(recv(udpFd,buf,sizeof(buf),0)==5 && !memcmp(buf,"audio",5));
    assert(recv(native,buf,sizeof(buf),0)==4 && !memcmp(buf,"wifi",4));close(native);
    int listener=socket(AF_INET,SOCK_STREAM,0);assert(listener>=0);
    auto lan=addr(0x7f000001,0);assert(bind(listener,(sockaddr*)&lan,sizeof(lan))==0);assert(listen(listener,1)==0);
    len=sizeof(lan);assert(getsockname(listener,(sockaddr*)&lan,&len)==0);
    int lanClient=socket(AF_INET,SOCK_STREAM,0);assert(lanClient>=0);assert(connect(lanClient,(sockaddr*)&lan,sizeof(lan))==0);
    int lanServer=accept(listener,nullptr,nullptr);assert(lanServer>=0);
    assert(send(lanClient,"LAN",3,0)==3);assert(recv(lanServer,buf,sizeof(buf),0)==3 && !memcmp(buf,"LAN",3));
    close(lanServer);close(lanClient);close(listener);
    int tcpFd=socket(AF_INET,SOCK_STREAM,0);assert(tcpFd>=0);assert(fcntl(tcpFd,F_SETFL,O_NONBLOCK)==0);
    dest=addr(peerIp,47989);assert(connect(tcpFd,(sockaddr*)&dest,sizeof(dest))==-1 && errno==EINPROGRESS);pump();
    pf={tcpFd,POLLOUT,0};assert(poll(&pf,1,0)==1 && (pf.revents&POLLOUT));
    int err=-1;len=sizeof(err);assert(getsockopt(tcpFd,SOL_SOCKET,SO_ERROR,&err,&len)==0 && err==0);
    assert(write(tcpFd,"GET / HTTP/1.0\r\n\r\n",18)==18);pump();
    fd_set read;FD_ZERO(&read);FD_SET(tcpFd,&read);timeval timeout={0,0};assert(select(tcpFd+1,&read,nullptr,nullptr,&timeout)==1);
    ssize_t count=::read(tcpFd,buf,sizeof(buf)-1);assert(count>0);buf[count]=0;assert(strstr(buf,"200 OK") && strstr(buf,"\r\n\r\nOK"));
    len=sizeof(from);assert(getsockname(tcpFd,(sockaddr*)&from,&len)==0 && ntohl(from.sin_addr.s_addr)==localIp);
    len=sizeof(from);assert(getpeername(tcpFd,(sockaddr*)&from,&len)==0 && ntohl(from.sin_addr.s_addr)==peerIp);
    assert(shutdown(tcpFd,SHUT_RD)==0);
    assert(recv(tcpFd,buf,sizeof(buf),0)==0);
    close(tcpFd);pump();
    int refused=socket(AF_INET,SOCK_STREAM,0);assert(fcntl(refused,F_SETFL,O_NONBLOCK)==0);dest=addr(peerIp,47984);
    assert(connect(refused,(sockaddr*)&dest,sizeof(dest))==-1 && errno==EINPROGRESS);pump();
    pf={refused,POLLOUT,0};assert(poll(&pf,1,0)==1 && (pf.revents&POLLERR));len=sizeof(err);assert(getsockopt(refused,SOL_SOCKET,SO_ERROR,&err,&len)==0 && err==ECONNREFUSED);close(refused);
    dest=addr(peerIp,47998);assert(connect(udpFd,(sockaddr*)&dest,sizeof(dest))==0);
    len=sizeof(from);assert(getsockname(udpFd,(sockaddr*)&from,&len)==0 && ntohl(from.sin_addr.s_addr)==localIp);
    assert(fcntl(udpFd,F_SETFL,0)==0);timeval shortTimeout={0,20000};
    assert(setsockopt(udpFd,SOL_SOCKET,SO_RCVTIMEO,&shortTimeout,sizeof(shortTimeout))==0);
    uint32_t before=ticks();assert(recv(udpFd,buf,sizeof(buf),0)==-1 && errno==EAGAIN);assert(ticks()-before>=20);
    // Link-down invalidates old virtual handles without touching BSD sockets.
    ++r.generation;r.vnet.reset();r.tcp->setAddress(0,0);
    dest=addr(peerIp,47998);assert(sendto(udpFd,"x",1,0,(sockaddr*)&dest,sizeof(dest))==-1 && errno==ENETUNREACH);close(udpFd);
    setTransportEnabled(false);
    int after=socket(AF_INET,SOCK_DGRAM,0);assert(after>=0);close(after);
    puts("transport: UDP scatter/gather, poll/select, TCP HTTP, errors, native LAN and link-down OK");
}
