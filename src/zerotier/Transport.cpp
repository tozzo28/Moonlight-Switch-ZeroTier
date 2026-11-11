// Process-local socket routing. The fd remains a real BSD socket, so ordinary
// LAN sockets and non-socket descriptors retain their original behavior.
#include "Embedded.hpp"
#include <map>
#include <array>
#include <algorithm>
#include <climits>
#include <cerrno>
#include <cstdarg>
#include <cstring>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
extern "C" {
int __real_socket(int,int,int); int __real_connect(int,const sockaddr*,socklen_t);
int __real_bind(int,const sockaddr*,socklen_t);int __real_close(int);int __real_shutdown(int,int);
int __real_setsockopt(int,int,int,const void*,socklen_t);int __real_getsockopt(int,int,int,void*,socklen_t*);
int __real_getsockname(int,sockaddr*,socklen_t*);int __real_getpeername(int,sockaddr*,socklen_t*);
ssize_t __real_send(int,const void*,size_t,int);ssize_t __real_recv(int,void*,size_t,int);
ssize_t __real_sendto(int,const void*,size_t,int,const sockaddr*,socklen_t);
ssize_t __real_recvfrom(int,void*,size_t,int,sockaddr*,socklen_t*);
ssize_t __real_sendmsg(int,const msghdr*,int);ssize_t __real_recvmsg(int,msghdr*,int);
ssize_t __real_read(int,void*,size_t);ssize_t __real_write(int,const void*,size_t);
int __real_select(int,fd_set*,fd_set*,fd_set*,timeval*);
int __real_poll(pollfd*,nfds_t,int);int __real_fcntl(int,int,...);int __real_ioctl(int,unsigned long,...);
}
namespace {
// Constant initialization is essential: libnx can call socket functions from
// userAppInit(), before C++ global constructors have initialized the fd map.
std::atomic<bool> transportEnabled{false};
}
namespace embedded {
void setTransportEnabled(bool enabled) { transportEnabled.store(enabled); }
}
namespace {
using namespace embedded;
struct Sock { int type=0,udp=-1,tcp=-1;unsigned generation=0;bool virt=false,nb=false,readClosed=false,writeClosed=false;uint32_t peer=0;uint16_t port=0,bound=0;int recvMs=0,sendMs=0; };
std::map<int,Sock> sockets; // protected by Runtime::lock, along with all lwIP calls
int error(int e){errno=e;return -1;}
bool target(const sockaddr *a,socklen_t n) {return a && n>=sizeof(sockaddr_in) && a->sa_family==AF_INET && ntohl(((const sockaddr_in*)a)->sin_addr.s_addr)==embedded::runtime().peer;}
void address(sockaddr *a,socklen_t *n,uint32_t ip,uint16_t port) {
    if(!a || !n)return;
    sockaddr_in out={};out.sin_family=AF_INET;out.sin_addr.s_addr=htonl(ip);out.sin_port=htons(port);
    memcpy(a,&out,std::min<size_t>(*n,sizeof(out)));*n=sizeof(out);
}
void sync(Sock &s) {
    if(s.generation!=runtime().generation){s.udp=-1;s.tcp=-1;s.generation=runtime().generation;}
}
int udp(Sock &s) {
    sync(s);
    auto &r=runtime();if(s.udp>=0)return s.udp;
    if(!r.vnet.configured())return -1;
    int v=r.vnet.udpOpen();if(v<0)return -1;
    if(r.vnet.udpBind(v,0,s.bound)<0){r.vnet.udpClose(v);return -1;}
    s.udp=v;return v;
}
void waitTick(){std::this_thread::sleep_for(std::chrono::milliseconds(2));}
bool expired(uint32_t start,int ms){return ms>0 && ticks()-start>=(uint32_t)ms;}
}
extern "C" int __wrap_socket(int domain,int type,int proto) {
    if(!transportEnabled.load())return __real_socket(domain,type,proto);

    int fd=__real_socket(domain,type,proto);
    if(fd>=0 && domain==AF_INET && (type==SOCK_STREAM || type==SOCK_DGRAM)) {std::lock_guard<std::mutex> g(embedded::runtime().lock);Sock s;s.type=type;s.generation=embedded::runtime().generation;sockets[fd]=s;}
    return fd;
}
extern "C" int __wrap_bind(int fd,const sockaddr *a,socklen_t n) {
    if(!transportEnabled.load())return __real_bind(fd,a,n);

    // Modern ENet binds to the local address obtained from the RTSP socket.
    // A managed ZeroTier IP does not exist on Horizon's physical BSD interface.
    // Reserve the real port on ANY, and expose/bind the managed IP in VNet.
    if(a && n >= sizeof(sockaddr_in) && a->sa_family == AF_INET) {
        std::lock_guard<std::mutex> guard(embedded::runtime().lock);
        auto it = sockets.find(fd);
        const auto *requested = reinterpret_cast<const sockaddr_in *>(a);
        auto &r = embedded::runtime();
        if(it != sockets.end() && it->second.type == SOCK_DGRAM &&
           r.vnet.configured() && ntohl(requested->sin_addr.s_addr) == r.vnet.localIp()) {
            Sock &s = it->second;
            sync(s);
            sockaddr_in reservation = *requested;
            reservation.sin_addr.s_addr = htonl(INADDR_ANY);
            if(__real_bind(fd, reinterpret_cast<const sockaddr *>(&reservation), sizeof(reservation)) < 0) return -1;
            socklen_t size = sizeof(reservation);
            if(__real_getsockname(fd, reinterpret_cast<sockaddr *>(&reservation), &size) < 0) return -1;
            s.bound = ntohs(reservation.sin_port);
            if(s.udp >= 0) {
                if(r.vnet.udpBind(s.udp, 0, s.bound) < 0) return error(ENOBUFS);
            } else if(udp(s) < 0) return error(ENOBUFS);
            s.virt = true;
            return 0;
        }
    }
    int rc=__real_bind(fd,a,n);if(rc<0)return rc;
    std::lock_guard<std::mutex> g(embedded::runtime().lock);auto it=sockets.find(fd);
    if(it!=sockets.end() && a && n>=sizeof(sockaddr_in) && a->sa_family==AF_INET) {
        sockaddr_in actual={};socklen_t len=sizeof(actual);if(!__real_getsockname(fd,(sockaddr*)&actual,&len))it->second.bound=ntohs(actual.sin_port);
        if(it->second.type==SOCK_DGRAM)udp(it->second);
    }return rc;
}
extern "C" int __wrap_connect(int fd,const sockaddr *a,socklen_t n) {
    if(!transportEnabled.load())return __real_connect(fd,a,n);

    using namespace embedded;
    if(!target(a,n))return __real_connect(fd,a,n);
    std::unique_lock<std::mutex> g(runtime().lock);auto it=sockets.find(fd);if(it==sockets.end())return error(EBADF);
    Sock &s=it->second;sync(s);if(!runtime().running || !runtime().vnet.configured())return error(ENETUNREACH);
    s.peer=runtime().peer;s.port=ntohs(((const sockaddr_in*)a)->sin_port);s.virt=true;
    if(s.type==SOCK_DGRAM)return udp(s)<0?error(ENOBUFS):0;
    if(s.tcp>=0)return error(runtime().tcp->established(s.tcp)?EISCONN:EALREADY);
    s.tcp=runtime().tcp->connect(s.peer,s.port);if(s.tcp<0)return error(ENOBUFS);
    if(s.nb)return error(EINPROGRESS);
    uint32_t start=ticks();int timeout=s.sendMs?s.sendMs:10000;
    for(;;){
        auto current=sockets.find(fd);if(current==sockets.end())return error(EBADF);
        sync(current->second);
        if(runtime().tcp->established(current->second.tcp))return 0;
        if(runtime().tcp->failed(current->second.tcp))return error(ECONNREFUSED);
        if(expired(start,timeout))return error(ETIMEDOUT);
        g.unlock();waitTick();g.lock();
    }
}
extern "C" ssize_t __wrap_sendto(int fd,const void *p,size_t n,int flags,const sockaddr *a,socklen_t len) {
    if(!transportEnabled.load())return __real_sendto(fd,p,n,flags,a,len);

    using namespace embedded;
    std::unique_lock<std::mutex> g(runtime().lock);auto it=sockets.find(fd);
    bool route=target(a,len) || (it!=sockets.end() && it->second.virt && !a);
    if(it==sockets.end() || !route){g.unlock();return __real_sendto(fd,p,n,flags,a,len);}
    Sock &s=it->second;sync(s);
    if(!runtime().vnet.configured())return error(ENETUNREACH);
    if(s.type==SOCK_DGRAM) {
        if(!a && (!s.peer || !s.port))return error(EDESTADDRREQ);
        if(n>ztnx::net::MaxDatagram)return error(EMSGSIZE);
        int v=udp(s);if(v<0)return error(ENOBUFS);
        uint32_t ip=a?ntohl(((const sockaddr_in*)a)->sin_addr.s_addr):s.peer;
        uint16_t port=a?ntohs(((const sockaddr_in*)a)->sin_port):s.port;
        return runtime().vnet.udpSendTo(v,ip,port,p,n)<0?error(ENOBUFS):(ssize_t)n;
    }
    if(s.writeClosed)return error(EPIPE);
    if(n==0)return 0;
    uint32_t start=ticks();bool nb=s.nb || (flags&MSG_DONTWAIT);int timeout=s.sendMs;
    for(;;){it=sockets.find(fd);if(it==sockets.end())return error(EBADF);
        sync(it->second);
        int result=runtime().tcp->send(it->second.tcp,p,std::min<size_t>(n,UINT_MAX));
        if(result>=0)return result;
        if(result==-2)return error(EPIPE);
        if(nb || expired(start,timeout))return error(EAGAIN);
        g.unlock();waitTick();g.lock();
    }
}
extern "C" ssize_t __wrap_send(int fd,const void *p,size_t n,int flags) {
    if(!transportEnabled.load())return __real_send(fd,p,n,flags);

    {std::unique_lock<std::mutex> g(embedded::runtime().lock);auto it=sockets.find(fd);if(it==sockets.end() || !it->second.virt){g.unlock();return __real_send(fd,p,n,flags);}}
    return __wrap_sendto(fd,p,n,flags,nullptr,0);
}
extern "C" ssize_t __wrap_recvfrom(int fd,void *p,size_t n,int flags,sockaddr *a,socklen_t *len) {
    if(!transportEnabled.load())return __real_recvfrom(fd,p,n,flags,a,len);

    using namespace embedded;
    std::unique_lock<std::mutex> g(runtime().lock);auto it=sockets.find(fd);
    if(it==sockets.end() || (!it->second.virt && it->second.type!=SOCK_DGRAM)){g.unlock();return __real_recvfrom(fd,p,n,flags,a,len);}
    bool nb=it->second.nb || (flags&MSG_DONTWAIT);int timeout=it->second.recvMs;uint32_t start=ticks();
    for(;;){
        it=sockets.find(fd);if(it==sockets.end())return error(EBADF);Sock &s=it->second;sync(s);
        if(s.readClosed)return 0;
        if(s.type==SOCK_STREAM){
            if(n==0)return 0;
            int rc=runtime().tcp->recv(s.tcp,p,std::min<size_t>(n,UINT_MAX),flags&MSG_PEEK);
            if(rc>=0){address(a,len,s.peer,s.port);return rc;}if(rc==-2)return error(ECONNRESET);
        } else {
            int v=udp(s);uint32_t ip;uint16_t port;
            if(v>=0 && runtime().vnet.udpReadable(v)){
                int rc=runtime().vnet.udpRecvFrom(v,p,std::min<size_t>(n,UINT_MAX),&ip,&port,flags&MSG_PEEK);
                address(a,len,ip,port);return rc;
            }
            if(!s.virt){g.unlock();ssize_t rc=__real_recvfrom(fd,p,n,flags|MSG_DONTWAIT,a,len);int e=errno;g.lock();if(rc>=0)return rc;if(e!=EAGAIN && e!=EWOULDBLOCK)return error(e);}
        }
        if(nb || expired(start,timeout))return error(EAGAIN);
        g.unlock();waitTick();g.lock();
    }
}
extern "C" ssize_t __wrap_recv(int fd,void *p,size_t n,int flags) {
    if(!transportEnabled.load())return __real_recv(fd,p,n,flags);
return __wrap_recvfrom(fd,p,n,flags,nullptr,nullptr);}
extern "C" int __wrap_close(int fd) {
    if(!transportEnabled.load())return __real_close(fd);

    {std::lock_guard<std::mutex> g(embedded::runtime().lock);auto it=sockets.find(fd);if(it!=sockets.end()){
        sync(it->second);
        if(it->second.udp>=0)embedded::runtime().vnet.udpClose(it->second.udp);
        if(it->second.tcp>=0)embedded::runtime().tcp->close(it->second.tcp);
        sockets.erase(it);
    }}return __real_close(fd);
}
extern "C" int __wrap_shutdown(int fd,int how) {
    if(!transportEnabled.load())return __real_shutdown(fd,how);

    std::lock_guard<std::mutex> g(embedded::runtime().lock);auto it=sockets.find(fd);
    if(it==sockets.end() || !it->second.virt)return __real_shutdown(fd,how);
    sync(it->second);
    it->second.readClosed|=how!=SHUT_WR;
    it->second.writeClosed|=how!=SHUT_RD;
    if(it->second.tcp>=0 && how!=SHUT_RD)embedded::runtime().tcp->shutdownWrite(it->second.tcp);
    return 0;
}
extern "C" int __wrap_setsockopt(int fd,int level,int opt,const void *p,socklen_t n) {
    if(!transportEnabled.load())return __real_setsockopt(fd,level,opt,p,n);

    std::lock_guard<std::mutex> g(embedded::runtime().lock);auto it=sockets.find(fd);
    if(it!=sockets.end()){
        Sock &s=it->second;sync(s);
        #ifdef SO_NONBLOCK
        if(level==SOL_SOCKET && n>=sizeof(int) && opt==SO_NONBLOCK)s.nb=*(const int*)p!=0;
#endif
        if(level==SOL_SOCKET && n>=sizeof(timeval) && (opt==SO_RCVTIMEO || opt==SO_SNDTIMEO)){
            auto *t=(const timeval*)p;int ms=std::min<int64_t>(INT_MAX,t->tv_sec*1000LL+(t->tv_usec+999)/1000);
            (opt==SO_RCVTIMEO?s.recvMs:s.sendMs)=ms;
        }
        if(s.virt && (level==IPPROTO_TCP || level==IPPROTO_IP))return 0;
    }return __real_setsockopt(fd,level,opt,p,n);
}
extern "C" int __wrap_getsockopt(int fd,int level,int opt,void *p,socklen_t *n) {
    if(!transportEnabled.load())return __real_getsockopt(fd,level,opt,p,n);

    std::lock_guard<std::mutex> g(embedded::runtime().lock);auto it=sockets.find(fd);
    if(it!=sockets.end())sync(it->second);
    if(it!=sockets.end() && it->second.virt && level==SOL_SOCKET && opt==SO_ERROR){
        if(!p || !n || *n<sizeof(int))return error(EINVAL);
        *(int*)p=it->second.tcp>=0 && embedded::runtime().tcp->failed(it->second.tcp)?ECONNREFUSED:0;*n=sizeof(int);return 0;
    }return __real_getsockopt(fd,level,opt,p,n);
}
extern "C" int __wrap_getsockname(int fd,sockaddr *a,socklen_t *n) {
    if(!transportEnabled.load())return __real_getsockname(fd,a,n);

    std::lock_guard<std::mutex> g(embedded::runtime().lock);auto it=sockets.find(fd);
    if(it!=sockets.end() && it->second.virt){auto &s=it->second;sync(s);address(a,n,embedded::runtime().vnet.localIp(),s.tcp>=0?embedded::runtime().tcp->localPort(s.tcp):embedded::runtime().vnet.udpLocalPort(s.udp));return 0;}
    return __real_getsockname(fd,a,n);
}
extern "C" int __wrap_getpeername(int fd,sockaddr *a,socklen_t *n) {
    if(!transportEnabled.load())return __real_getpeername(fd,a,n);

    std::lock_guard<std::mutex> g(embedded::runtime().lock);auto it=sockets.find(fd);
    if(it!=sockets.end() && it->second.virt){if(!it->second.peer)return error(ENOTCONN);address(a,n,it->second.peer,it->second.port);return 0;}return __real_getpeername(fd,a,n);
}
extern "C" int __wrap_poll(pollfd *fds,nfds_t count,int timeout) {
    if(!transportEnabled.load())return __real_poll(fds,count,timeout);

    using namespace embedded;uint32_t start=ticks();std::vector<pollfd> native(count);
    for(;;){int ready=0;
        {std::lock_guard<std::mutex> g(runtime().lock);
            for(nfds_t i=0;i<count;i++){
                fds[i].revents=0;native[i]=fds[i];auto it=sockets.find(fds[i].fd);if(it==sockets.end())continue;Sock &s=it->second;sync(s);
                if(s.virt){native[i].fd=-1;
                    if(s.type==SOCK_STREAM){if(s.readClosed || runtime().tcp->readable(s.tcp))fds[i].revents|=fds[i].events&POLLIN;if(runtime().tcp->writable(s.tcp))fds[i].revents|=fds[i].events&POLLOUT;if(runtime().tcp->failed(s.tcp))fds[i].revents|=POLLERR;}
                    else {if(udp(s)>=0 && runtime().vnet.udpReadable(s.udp))fds[i].revents|=fds[i].events&POLLIN;fds[i].revents|=fds[i].events&POLLOUT;}
                }else if(s.type==SOCK_DGRAM && udp(s)>=0 && runtime().vnet.udpReadable(s.udp))fds[i].revents|=fds[i].events&POLLIN;
            }
        }
        int rc=__real_poll(native.data(),count,0);if(rc<0)return rc;
        for(nfds_t i=0;i<count;i++){fds[i].revents|=native[i].revents;if(fds[i].revents)ready++;}
        if(ready || timeout==0 || expired(start,timeout))return ready;
        waitTick();
    }
}
extern "C" int __wrap_select(int nfds,fd_set *read,fd_set *write,fd_set *except,timeval *tv) {
    if(!transportEnabled.load())return __real_select(nfds,read,write,except,tv);

    if(nfds<0 || nfds>FD_SETSIZE)return error(EINVAL);
    std::vector<pollfd> p;for(int fd=0;fd<nfds;fd++){short events=0;if(read && FD_ISSET(fd,read))events|=POLLIN;if(write && FD_ISSET(fd,write))events|=POLLOUT;if(except && FD_ISSET(fd,except))events|=POLLPRI;if(events)p.push_back({fd,events,0});}
    int timeout=tv?(int)std::min<int64_t>(INT_MAX,tv->tv_sec*1000LL+(tv->tv_usec+999)/1000):-1;
    int rc=__wrap_poll(p.data(),p.size(),timeout);if(rc<0)return rc;
    if(read)FD_ZERO(read);
    if(write)FD_ZERO(write);
    if(except)FD_ZERO(except);
    int bits=0;
    for(auto &f:p){if(f.revents&POLLNVAL)return error(EBADF);if(read && (f.events&POLLIN) && (f.revents&(POLLIN|POLLERR|POLLHUP))){FD_SET(f.fd,read);bits++;}if(write && (f.events&POLLOUT) && (f.revents&(POLLOUT|POLLERR))){FD_SET(f.fd,write);bits++;}if(except && (f.revents&POLLPRI)){FD_SET(f.fd,except);bits++;}}
    return bits;
}
extern "C" int __wrap_fcntl(int fd,int cmd,...) {
    if(cmd==F_GETFL || cmd==F_GETFD)return __real_fcntl(fd,cmd);
    va_list ap;va_start(ap,cmd);
    if(cmd!=F_SETFL && cmd!=F_SETFD && cmd!=F_DUPFD){
        void *arg=va_arg(ap,void*);va_end(ap);return __real_fcntl(fd,cmd,arg);
    }
    int arg=va_arg(ap,int);va_end(ap);
    int rc=__real_fcntl(fd,cmd,arg);
    if(transportEnabled.load() && rc>=0 && cmd==F_SETFL){std::lock_guard<std::mutex> g(embedded::runtime().lock);auto it=sockets.find(fd);if(it!=sockets.end())it->second.nb=arg&O_NONBLOCK;}
    return rc;
}
extern "C" int __wrap_ioctl(int fd,unsigned long cmd,...) {
    va_list ap;va_start(ap,cmd);void *p=va_arg(ap,void*);va_end(ap);
    if(transportEnabled.load() && cmd==(unsigned long)FIONBIO){std::lock_guard<std::mutex> g(embedded::runtime().lock);auto it=sockets.find(fd);if(it!=sockets.end())it->second.nb=*(int*)p!=0;}
    return __real_ioctl(fd,cmd,p);
}
extern "C" ssize_t __wrap_sendmsg(int fd,const msghdr *m,int flags) {
    if(!transportEnabled.load())return __real_sendmsg(fd,m,flags);

    if(!m)return error(EINVAL);
    bool routed;{std::lock_guard<std::mutex> g(embedded::runtime().lock);auto it=sockets.find(fd);routed=target((sockaddr*)m->msg_name,m->msg_namelen)||(it!=sockets.end()&&it->second.virt);}
    if(!routed)return __real_sendmsg(fd,m,flags);
    std::vector<unsigned char> data;for(size_t i=0;i<(size_t)m->msg_iovlen;i++){auto &v=m->msg_iov[i];if(data.size()+v.iov_len>65536)return error(EMSGSIZE);auto *p=(const unsigned char*)v.iov_base;data.insert(data.end(),p,p+v.iov_len);}
    return __wrap_sendto(fd,data.data(),data.size(),flags,(sockaddr*)m->msg_name,m->msg_namelen);
}
extern "C" ssize_t __wrap_recvmsg(int fd,msghdr *m,int flags) {
    if(!transportEnabled.load())return __real_recvmsg(fd,m,flags);

    if(!m)return error(EINVAL);
    {std::unique_lock<std::mutex> g(embedded::runtime().lock);auto it=sockets.find(fd);if(it==sockets.end() || (!it->second.virt && it->second.type!=SOCK_DGRAM)){g.unlock();return __real_recvmsg(fd,m,flags);}}
    size_t cap=0;for(size_t i=0;i<(size_t)m->msg_iovlen;i++)cap+=m->msg_iov[i].iov_len;
    std::vector<unsigned char> data(std::min<size_t>(cap,65536));socklen_t len=m->msg_namelen;
    ssize_t rc=__wrap_recvfrom(fd,data.data(),data.size(),flags,(sockaddr*)m->msg_name,&len);
    if(rc<0)return rc;
    size_t off=0;for(size_t i=0;i<(size_t)m->msg_iovlen && off<(size_t)rc;i++){size_t n=std::min<size_t>(m->msg_iov[i].iov_len,rc-off);memcpy(m->msg_iov[i].iov_base,data.data()+off,n);off+=n;}
    m->msg_namelen=len;m->msg_flags=0;m->msg_controllen=0;return rc;
}
extern "C" ssize_t __wrap_read(int fd,void *p,size_t n) {
    if(!transportEnabled.load())return __real_read(fd,p,n);

    {std::unique_lock<std::mutex> g(embedded::runtime().lock);auto it=sockets.find(fd);if(it==sockets.end() || !it->second.virt){g.unlock();return __real_read(fd,p,n);}}return __wrap_recv(fd,p,n,0);
}
extern "C" ssize_t __wrap_write(int fd,const void *p,size_t n) {
    if(!transportEnabled.load())return __real_write(fd,p,n);

    {std::unique_lock<std::mutex> g(embedded::runtime().lock);auto it=sockets.find(fd);if(it==sockets.end() || !it->second.virt){g.unlock();return __real_write(fd,p,n);}}return __wrap_send(fd,p,n,0);
}
