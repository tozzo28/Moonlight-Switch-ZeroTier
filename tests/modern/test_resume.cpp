#include "client.h"
#include "http.h"
#include "errors.h"
#include <cassert>
#include <cstdio>
static std::string reply;
static std::string requested;
static int requests=0;
extern "C" const char *LiGetLaunchUrlQueryParameters(void) { return ""; }
int http_request(const std::string &url,Data *data,HTTPRequestTimeout) {
    requested=url;++requests;*data=Data((char*)reply.data(),reply.size());return GS_OK;
}
void http_diagnostic(const char*,...) {}
static SERVER_DATA server(int app) {
    SERVER_DATA value={};value.serverInfo.address="192.0.2.18";
    value.httpPort=47989;value.httpsPort=47984;value.currentGame=app;return value;
}
int main() {
    STREAM_CONFIGURATION config={};config.width=1280;config.height=720;config.fps=60;
    config.audioConfiguration=AUDIO_CONFIGURATION_STEREO;
    auto resumed=server(123);
    reply="<root status_code=\"200\"><resume>1</resume><sessionUrl0>rtsp://192.0.2.18:48010</sessionUrl0></root>";
    assert(gs_start_app(&resumed,&config,123,true,false,1)==GS_OK);
    assert(requested.find("/resume?")!=std::string::npos && resumed.currentGame==123);
    assert(resumed.serverInfo.rtspSessionUrl && std::string(resumed.serverInfo.rtspSessionUrl)=="rtsp://192.0.2.18:48010");
    delete[] resumed.serverInfo.rtspSessionUrl;resumed.serverInfo.rtspSessionUrl=nullptr;
    reply="<root status_code=\"200\"><resume>0</resume></root>";
    assert(gs_start_app(&resumed,&config,321,true,false,1)==GS_FAILED);
    assert(resumed.currentGame==123); // A rejected resume must preserve active app state.
    reply="<root status_code=\"503\" status_message=\"No running app to resume\"><resume>0</resume></root>";
    assert(gs_start_app(&resumed,&config,123,true,false,1)==GS_ERROR);
    assert(gs_error()=="No running app to resume");
    reply="<root status_code=\"200\"><gamesession>1</gamesession><sessionUrl0>rtsp://192.0.2.18:48010</sessionUrl0></root>";
    assert(gs_start_app(&resumed,&config,123,true,false,1)==GS_OK); // Compatible older hosts.
    delete[] resumed.serverInfo.rtspSessionUrl;resumed.serverInfo.rtspSessionUrl=nullptr;
    auto launched=server(0);
    assert(gs_start_app(&launched,&config,456,true,false,1)==GS_OK);
    assert(requested.find("/launch?")!=std::string::npos && launched.currentGame==456);
    delete[] launched.serverInfo.rtspSessionUrl;launched.serverInfo.rtspSessionUrl=nullptr;
    launched.currentGame=0;reply="<root status_code=\"200\"><resume>1</resume></root>";
    assert(gs_start_app(&launched,&config,456,true,false,1)==GS_FAILED); // Wrong field for launch.
    assert(launched.currentGame==0);
    printf("resume: Sunshine resume=1, launch gamesession=1, errors and compatible fallback OK (%d requests)\n",requests);
}
