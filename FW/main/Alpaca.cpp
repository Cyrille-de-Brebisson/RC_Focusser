// Of interest: ascom pdf and web versions of API descriptions...
// https://ascom-standards.org/AlpacaDeveloper/ASCOMAlpacaAPIReference.html#[11,%22XYZ%22,69,495,null]
// https://ascom-standards.org/api/?urls.primaryName=ASCOM%20Alpaca%20Management%20API#/
// https://ascom-standards.org/api/#/


////////////////////////////////////////////////////////////////////////////
// Alpaca API system.
// Alpaca "hubs" listen to a UDP port.
// Alpaca application broadcast a "hey" on said port and "hub" reply with the port used by the hub http connection
// Alpaca application will then use http protocol to access the following urls on the hub:
// 
// GET /management/apiversions
// GET /management/v1/description
// GET /management/v1/configureddevices
// GET /setup
// GET /setup/v1/focuser/0/setup
// GET /api/v1/focuser/0/command?parameters=...&parameters=...&
// PUT /api/v1/focuser/0/command with the parameters=...&parameters=...& under as "data"....
// 
// ---------------------
// Typical HTTP response:
// HTTP/1.1 200 OK
// Content-Type: application/json; charset=utf-8
// Transfer-Encoding: chunked
// 
// 5b8
// {"Value":[{"DeviceName":"Alpaca Camera Sim","DeviceType":"Camera","DeviceNumber":0,"UniqueID":"295cd610-dd10-48bc-a2aa-3e3c8a9cf95c"},{"DeviceName":"Alpaca CoverCalibrator Simulator","DeviceType":"CoverCalibrator","DeviceNumber":0,"UniqueID":"7c53db45-11ce-43ce-aa24-43a158c38489"},{"DeviceName":"Alpaca Dome Simulator","DeviceType":"Dome","DeviceNumber":0,"UniqueID":"88da0d0e-f36d-47bc-9382-f2b55d442300"},{"DeviceName":"Alpaca Filter Wheel Simulator","DeviceType":"FilterWheel","DeviceNumber":0,"UniqueID":"a48deb53-ac03-4dd9-bbf8-45f78d99c082"},{"DeviceName":"Alpaca Focuser Simulator","DeviceType":"Focuser","DeviceNumber":0,"UniqueID":"d1a1068f-f5ba-477c-bf7b-dbfc132c9c87"},{"DeviceName":"Alpaca Observing Conditions Simulator","DeviceType":"ObservingConditions","DeviceNumber":0,"UniqueID":"774623d2-4202-4803-ab30-e79e5b12a785"},{"DeviceName":"Alpaca Rotator Simulator","DeviceType":"Rotator","DeviceNumber":0,"UniqueID":"750dccec-e7b6-4d4f-a452-775250a91240"},{"DeviceName":"Alpaca Safety Monitor Simulator","DeviceType":"SafetyMonitor","DeviceNumber":0,"UniqueID":"74af2f9f-b76e-4f67-be5f-a192af65ef41"},{"DeviceName":"Alpaca Switch Simulator","DeviceType":"Switch","DeviceNumber":0,"UniqueID":"92c294ed-7fbd-4856-899a-eca52291c9bd"},{"DeviceName":"Alpaca Telescope Simulator","DeviceType":"Telescope","DeviceNumber":0,"UniqueID":"7a5bc92c-3f0d-4f29-b5a2-0ea93b83e6a0"}],"ClientTransactionID":0,"ServerTransactionID":8,"ErrorNumber":0,"ErrorMessage":""}
// 0
//
// -------------------
// Typical PUT API data... Seems that the standard is xform encoded, NOT jason!!!
// PUT /api/v1/focuser/0/connected HTTP/1.1
// Connection: keep-alive
// Content-Type: application/x-www-form-urlencoded
// Content-Length: 48
// 
// Connected=False&ClientID=1&ClientTransactionID=1
// -------------------
// Typical GET API data...
// GET /api/v1/focuser/0/connected?ClientID=1&ClientTransactionID=1 HTTP/1.1
// Connection: keep-alive
// Content-Type: application/x-www-form-urlencoded
// 
//





#include "Alpaca.h"
#include <atomic>

#ifndef _WIN32
    #include <esp_mac.h>
    #include <esp_err.h>
    #include <freertos/FreeRTOS.h>
    #include <lwip/sockets.h>
    #include <lwip/netdb.h>
    #include <esp_log.h>
    void CAlpaca::osinit() 
    { // Generates a uniqeID used for devices identification...
        uint8_t mac[8];
        esp_efuse_mac_get_default(mac);
        for (int i=0; i<6; i++) uniqueid[i*2]= 'a'+(mac[i]&0x15), uniqueid[i*2+1]= 'a'+((mac[i]>>4)&0x15);
    }
    typedef int SOCKET;
    static int const INVALID_SOCKET= -1;
    #define packdebug(...) // define to print request debugs...
    static void inline _itoa(int v, char *b, int radix) { itoa(v, b, radix); }
    #if 1 // change if you do, or do not have issues with this...
      extern "C" int lwip_hook_ip6_input(struct pbuf *p, struct netif *inp) __attribute__((weak));
      extern "C" int lwip_hook_ip6_input(struct pbuf *p, struct netif *inp) { if (ip6_addr_isany_val(inp->ip6_addr[0].u_addr.ip6)) { pbuf_free(p); return 1; } return 0; }
    #endif
      
#else
    // This is a bunch of code that will allow you to compile under windows... Which can make testing a WHOLE lot easier!!!!
    #define LOCALHOSTONLY // define this to limit UDP listening to localhost! avoids getting messages from multiple routes...
    #include <WinSock2.h>
    #pragma comment(lib, "WS2_32.lib")
    #include <ws2tcpip.h>
    void CAlpaca::osinit() 
    { 
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2,2), &wsaData); if (iResult != 0) 
        printf("WSAStartup failed with error: %d\n", iResult); 
        strcpy(uniqueid, "234564321234");
    }
    #include <stdint.h>
    #include <stdio.h>
    typedef unsigned int uint;
    #define ESP_LOGE(A, ...) { printf(__VA_ARGS__); printf("\r\n"); }
    #define ESP_LOGD(A, ...) { printf(__VA_ARGS__); printf("\r\n"); }
    #define ESP_LOGI(A, ...) { printf(__VA_ARGS__); printf("\r\n"); }
    static void vTaskDelay(int a) { Sleep(a); }
    typedef int socklen_t;
    static void close(SOCKET) {}
    void xTaskCreate(void (*cb)(void*), char const *, int stack, void *p, int, void*) { CreateThread(0, stack, (LPTHREAD_START_ROUTINE)cb, p, 0, nullptr); }
    void vTaskDelete(void *){}
    #define packdebug(...) printf("%s", __VA_ARGS__) // define to print request debugs...
#endif

// returns true if s1 starts with s2. assumes s1 is lowercase, but s2 migth not be...
static bool startsWithNonCase(char const *s1, char const *s2)
{
    while (true)
    {
        char c2= *s2++; if (c2==0) return true; if (c2>='A' && c2<='Z') c2+= 'a'-'A';
        char c1= *s1++; if (c1>='A' && c1<='Z') c1+= 'a'-'A';
        if (c1!=c2) return false;
    }
}
// read a positive int from s, and moves s to the end of the int.... If no int, returns 0...
template <typename T>
static int readInt(T *&s) 
{
    int res= 0;
    while (*s>='0' && *s<='9') res= res*10+*s++-'0';
    return res;
}
template <typename T>
static float readFloat(T *&s)
{
    float r= readInt(s);
    if (*s!='.') return r; s++;
    float div= 0.1;
    while (*s>='0' && *s<='9') { r+= div*(*s++-'0'); div/=10.0f; }
    return r;
}

static const char *TAG = "Alpaca"; // For debug on ESP32...

// This listen to UDP discovery requests and replies by saying "I am here"!!!
// port is the http port for doing actual work passed as a parameter
static void udp_server_task(void *port)
{
  char response[64]; sprintf(response, "{\"AlpacaPort\":%d}", (int)(size_t)port); // prep response packets (always the same)
  size_t const response_len = strlen(response);

  while (true)
  {
    // create udp socket...
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) { ESP_LOGE(TAG, "UDP Unable to create  socket: errno %d", errno); vTaskDelay(1000); continue; }
    ESP_LOGI(TAG, "UDP listener Socket created");
    // make it listen to broadcasts...
    struct sockaddr_in dest_addr; memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(32227);  // By definition!
    int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err < 0) { ESP_LOGE(TAG, "UDP Socket unable to bind: errno %d", errno); vTaskDelay(1000); continue; }
    ESP_LOGI(TAG, "UDP Socket bound, port %d", htons(dest_addr.sin_port));

    while (true)
    {
      char rx_buffer[64]; // 64 by alpaca standard
      struct sockaddr_storage source_addr; socklen_t socklen= sizeof(source_addr); // Large enough for both IPv4 or IPv6
      // get data from anyone...
      int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer), 0, (struct sockaddr *)&source_addr, &socklen);
      if (len < 0) { ESP_LOGE(TAG, "UDP recvfrom failed: errno %d len:%d", errno, len); break; }
      rx_buffer[len]=0;
      ESP_LOGI(TAG, "UDP received %d %s", len, rx_buffer);
      // Data received
#ifdef LOCALHOSTONLY // for local testing, this only gets messages from localhost for easier testing...
      if (source_addr.__ss_pad1[2]!=127) continue;
#endif
      if (len>=16 && memcmp(rx_buffer, "alpacadiscovery1", 16)==0) // known discovery message?
      {
        err = sendto(sock, response, (int)response_len, 0, (sockaddr*)&source_addr, sizeof(source_addr)); // reply
        if (err < 0) { ESP_LOGE(TAG, "UDP Socket unable to sendto: errno %d", errno); continue; }
        ESP_LOGI(TAG, "UDP sent %s", response);
      }
    }

    ESP_LOGI(TAG, "UDP Shutting down socket and restarting..."); 
    shutdown(sock, 0); close(sock);
  }
}

// handles ONE HTTP client. Sock is open through an accept and passed through the alpaca object...
// This function received a well formed http request, divides it into a GET/PUT, url and data before calling alpaca to handle it...
static void HTTPClient(void *P)
{
    CAlpaca *alpaca= (CAlpaca *)P;
    SOCKET sock= alpaca->newClientSocket; alpaca->newClientSocket= -1; // get sock
    #ifndef _WIN32
    timeval timeout= { .tv_sec=120, .tv_usec=0 }; setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout); // 2 minutes timeout on sock receve (the system is different on windows... but not needed for test, so... not there)
    #endif
    ESP_LOGI(TAG, "HTTP new client");
    int keepAlive= 1; setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char*)&keepAlive, sizeof(int)); // Set tcp keepalive option. I don't honneslty know what that means!!!
    char buf[1024]; size_t bs= 0; // buffer and buffer size./ max 1024 bytes read.. Assumes all ascom messages are smaller... Will need to check!
    while (true)
    {
        int rlen= recv(sock, buf+bs, (int)(sizeof(buf)-bs), 0); // receive data. rlen is received len or error
        if (rlen<=0) { ESP_LOGE(TAG, "HTTP recv failed: %d", rlen); break; }
        bs+= rlen;
        if (bs>=sizeof(buf)-1) break; // We have a problem here! stop...
        if (bs<4) continue;           // not enough data for anything...
        bool get= true;               // by default in get mode
        buf[bs]= 0; char *end, *data; // null terminate to use str funcitons, end will be end of packet, data will be data part..
        if (memcmp(buf, "GET ", 4)==0) // Get case. ends with double new line; Data is after url and starts with "?" if is there...
        { 
            end= strstr(buf, "\r\n\r\n"); if (end==nullptr) continue; end+= 4; // we don't have the end yet
            // find a '?' or a space. If '?', replace by ' ' and return the char after (first char of data).. else, no data...
            data= buf+4; while (true) if (*data=='?') { *data++= ' '; break; } else if (*data<=' ') { data= nullptr; break; } else data++; 
        }
        else if (memcmp(buf, "PUT ", 4)==0) // PUT type. data is at the end of http request...
        { 
            get= false;
            end= strstr(buf, "Content-Length:"); if (end==nullptr) continue; // find length of data marker
            end+= 15; while (*end==' ' || *end=='\t') end++; // skip lengh marker
            int l= readInt(end);                             // get length
            end= strstr(end, "\r\n\r\n"); if (end==nullptr) continue; end+=4; // find end of header
            data= end; end+= l;                     // end of header is start of data. end of buffer is l bytes later...
            if (end>buf+bs) continue;               // if we don't have all the data... wait some more...
        }
        else { char const er[]= "HTTP/1.0 400 Bad Request\r\n\r\n"; send(sock, er, sizeof(er)-1, 0); break; } // error...

        // packdebug(buf);
        char savedChar, *savedCharPos= nullptr; // save a character that we will change and that might be the first char of the next message!
        if (data!=nullptr) // 0 terminate data. Since data CAN be at end of message, save what was under the 0 in case it was important...
        {
            char *t= data; while (*t>' ') t++; // find a separator...
            savedCharPos= t; savedChar= *savedCharPos; *savedCharPos= 0; // save it and put a 0!
        }
        { char *t= strchr(buf+4, ' '); if (t!=nullptr) *t= 0; } // 0 terminate the URL... no need to save the char there as URL can NOT be at end of message...
        ESP_LOGI(TAG, "HTTP new http %s %s", buf, data!=nullptr?data:"");
        packdebug(buf); packdebug("\r\n"); if (data) { packdebug(data); packdebug("\r\n"); }
        packdebug("\r\n<-----------------------------------\r\n");
        if (!alpaca->execRequest(sock, get, buf+4, data)) break; // Have alpaca handle the request! Do the actual work...
        packdebug("\r\n-----------------------------------\r\n");
        // Assumes keep alive... client can close if it wants!!!!
        if (savedCharPos) *savedCharPos= savedChar;  // restore removed char
        bs-= end-buf; memcpy(buf, end, bs);          // and shift buffer down for next message...
    }
    ESP_LOGI(TAG, "HTTP close");
    shutdown(sock, 0); close(sock); // cleanup
    vTaskDelete(nullptr);
}

// This is the HTTP server task. listen on the given port and accept clients.
// starts a new task for each of them...
static void HTTPTask(void *P)
{
    CAlpaca *alpaca= (CAlpaca *)P;
    SOCKET ListenSocket; int opt, iResult;

    ESP_LOGI(TAG, "HTTPServer start %d", alpaca->httpport);

    // binding address
    struct sockaddr_storage dest_addr; struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
    dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr_ip4->sin_family = AF_INET;
    dest_addr_ip4->sin_port = htons(alpaca->httpport);

    // Create a SOCKET for the server to listen for client connections.
    ListenSocket= socket(dest_addr_ip4->sin_family, SOCK_STREAM, IPPROTO_IP);
    if (ListenSocket==INVALID_SOCKET) { ESP_LOGE(TAG, "HTTPServer socket failed: %d", errno); goto er; }
    #ifndef _WIN32
        opt= 1; setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    #endif
    // setsockopt(ListenSocket, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));
    // Setup the TCP listening socket
    iResult = bind(ListenSocket, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (iResult<0) { ESP_LOGE(TAG, "HTTPServer bind failed: %d", errno); goto er; }
    alpaca->newClientSocket= -1;

    //Now, await clients...
    while (true)
    {
        ESP_LOGI(TAG, "HTTPServer listen");
        iResult = listen(ListenSocket, 5);
        if (iResult<0) { ESP_LOGE(TAG, "HTTPServer listen failed: %d", errno); goto er; }
        // Accept a client socket
        SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET) { ESP_LOGE(TAG, "HTTPServer accept failed: %d", errno); goto er; }
        ESP_LOGI(TAG, "HTTPServer accepted %d", alpaca->newClientSocket);
        while (alpaca->newClientSocket!=-1) vTaskDelay(1); // will be set to -1 by the HTTPClient as soon as it picks it! which should be almost instantaneous...
        // pass the ClientSocket to HTTPClient and run it...
        alpaca->newClientSocket= ClientSocket; 
        xTaskCreate(HTTPClient, "http_client", 8192, alpaca, 2, nullptr);
    }
    er:
    ESP_LOGE(TAG, "HTTPServer stopped");
    vTaskDelete(nullptr);
}

void CAlpaca::start(int port)
{
    osinit(); // unique identificator plus other stuff as needed...
    httpport= port;
    xTaskCreate(udp_server_task, "udp_server_task", 2048, (void*)(size_t)port, 2, nullptr);
    xTaskCreate(HTTPTask, "http_server_task", 2048, this, 2, nullptr);
}

CAlpaca::CAlpaca(char const *Manufacturer, char const *ManufacturerVersion, char const *DefaultServerName, char const *DefaultLocation) 
{
    strncpy(this->ServerName, DefaultServerName ,sizeof(this->ServerName)-1);
    strncpy(this->Manufacturer, Manufacturer ,sizeof(this->Manufacturer)-1);
    strncpy(this->ManufacturerVersion, ManufacturerVersion ,sizeof(this->ManufacturerVersion)-1);
    strncpy(this->Location, DefaultLocation , sizeof(this->Location)-1);
    wifi[0]= wifip[0]= 0;
    uniqueid[0]= 0; 
    saveLoadBegin();
    char t[sizeof(this->Location)];
    strcpy(this->ServerName, load("ServerName", this->ServerName, t, sizeof(this->ServerName)));
    strcpy(this->Location, load("Location", this->Location, t, sizeof(this->Location)));
    strcpy(this->wifi, load("wifi", "", t, sizeof(this->wifi)));
    strcpy(this->wifip, load("wifip", "", t, sizeof(this->wifip)));
    saveLoadEnd();
}

void CAlpaca::addDevice(CAlpacaDevice *a)
{ 
    if (nbDevices==maxDevices) return;
    memcpy(a->keyHeader, a->get_type(), 2); _itoa(a->id, a->keyHeader+2, 10); int tl= strlen(a->keyHeader); // create the "start of the handle" as type/id
    saveLoadBegin();
    char t[30]; strcpy(t, a->keyHeader);
    strcpy(t+tl, "Name"); load(t, a->Name, a->Name, sizeof(a->Name));
    strcpy(t+tl, "Description"); load(t, a->Description, a->Description, sizeof(a->Description));
    a->subLoad(this, t, tl);
    saveLoadEnd();
    // generate json
    t[tl]=0;
    snprintf(a->_jsonDesc, sizeof(a->_jsonDesc)-1, "{\"DeviceName\":\"%s\",\"DeviceType\":\"%s\",\"DeviceNumber\":%d,\"UniqueID\":\"%s%s\"},", a->get_name(), a->get_type(), a->id, uniqueid, t);
    // and finally, add...
    devices[nbDevices++]= a; 
}

void CFilterWheel::subLoad(CAlpaca *alpaca, char *t, int tl)
{
    strcpy(t+tl, "names"); alpaca->load(t, names, names, sizeof(names));
    strcpy(t+tl, "focusoffsets"); alpaca->load(t, focusoffsets, focusoffsets, sizeof(focusoffsets));
}

static std::atomic<int> transactionsId= 0; // transactio counter....

static char const httpokheader[]= "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nTransfer-Encoding: chunked\r\n\r\n";
//static char const httpokheader[]= "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nDate: Fri, 24 Jan 2025 19:50:00 GMT\r\nServer: Kestrel\r\nTransfer-Encoding: chunked\r\n\r\n";
static char const http404header[]= "HTTP/1.1 404 Page Not Found\r\n\r\n";
static char const http400header[]= "HTTP/1.1 400 Bad Request\r\n\r\n";
// These are for debug purpose...
static void inline send2(SOCKET s, char *t, int l, int f) { t[l]= 0; packdebug(t); send(s, t, l, f); }
static void inline send2(SOCKET s, char const *t, int l, int f) { packdebug(t); send(s, t, l, f); }

// return the device from a url that points to Focuser/0 or something like that. moves url to the next "word" in the url for further processing (skipping '/' if needed)
CAlpacaDevice *CAlpaca::deviceFromURL(char *&url)
{
    for (int i=0; i<nbDevices; i++)
    {
        CAlpacaDevice *d= devices[i];
        char const *type= d->get_type(); 
        if (!startsWithNonCase(url, type)) continue;       // does device type match api request???
        char *t= url+strlen(type); if (*t!='/') continue; t++;// yes, skip it and /
        int id= readInt(t); if (d->id!=id) continue;       // does id match api request?
        if (*t!='/') continue;                           // yes, skip that from url (and/)
        url= t+1; return d;
    }
    return nullptr;
}

// Main alpaca http request handleing!
bool CAlpaca::execRequest(int sock, bool get, char *url, char *data)
{
    // Find the client transaciton id if present in data!
    char ClientTransactionId[9]= "0";
    if (data!=nullptr)
    {
        char *q= strstr(data, "ClientTransactionID=");
        if (q!=nullptr)
        {
            q+=20;
            int i=0; while (i<8 && q[i]>='0' && q[i]<='9') { ClientTransactionId[i]= q[i]; i++; }
            ClientTransactionId[i]= 0;
        }
    }
    CMyStr s; s+="        \r\n"; // space for size! see sendChunk
    s.printf("{\"ClientTransactionID\":%s,\"ServerTransactionID\":%d,", ClientTransactionId, transactionsId++); // start with known needed data!

    // Generic apis...
    if (memcmp(url, "/management", 11)==0)
    {
        if (get && strcmp(url, "/management/apiversions")==0) s+= "\"ErrorNumber\":0,\"ErrorMessage\":\"\",\"Value\":[1]}";
        else if (get && strcmp(url, "/management/v1/description")==0) s.printf("\"Value\":{\"ServerName\":\"%s\",\"Manufacturer\":\"%s\",\"ManufacturerVersion\":\"%s\",\"Location\":\"%s\"},\"ErrorNumber\":0,\"ErrorMessage\":\"\"}", ServerName, Manufacturer, ManufacturerVersion, Location);
        else if (get && strcmp(url, "/management/v1/configureddevices")==0)
        {   // device enumeration
            s+= "\"ErrorNumber\":0,\"ErrorMessage\":\"\",\"Value\":[";
            for (int i=0; i<nbDevices; i++) s+= devices[i]->_jsonDesc;
            if (s.c[s.w-1]==',') s.w--; // remove final ',' if there is one...
            s+= "]}";
        }
    }
    else if (memcmp(url, "/api/v1/", 8)==0) // device apis...
    {
        url+= 8; CAlpacaDevice *d= deviceFromURL(url);
        if (d==nullptr) goto er400;
        // url now points on API name/command and we have the right device! dispatch on virtual fuction
        bool ok= d->dispatch(get, url, data, &s);
        if (!ok) goto er400; // not ok, return error....
        if (s.c[s.w-1]==',') s.w--; s+= "}"; // close object
    } else if (memcmp(url, "/setup", 6)==0)
    {
        url+= 6; if (*url!='/') return setup(sock, get, data);
        if (memcmp(url, "/v1/",4)!=0) goto er400; url+= 4;
        CAlpacaDevice *d= deviceFromURL(url);
        if (d==nullptr) goto er400; // device not known! Error
        if (strcmp(url, "setup")!=0) goto er400;
        return d->setup(this, sock, get, data);
    } else { return setup(sock, get, data);
        er400:
        send2(sock, http400header, sizeof(http400header)-1, 0); // api not know.. error...
        return false;
    }

    // default case, send s (a json data) to the server
    send(sock, httpokheader, sizeof(httpokheader)-1, 0); // sendHTTP header...
    // s was started by 8 spaces and a crlf. Will write the chunk lengh in that space and add a crfl0 after that and then send it
    // This is the normal structure of a http response in chuck encoded format...
    size_t l= s.w-10; // This is to account for the header!
    if (l==0) { send(sock, "0\r\n", 3, 0); return true; }
    static char const hex[]="0123456789abcdef";
    for (int i=0; i<8; i++) { s.c[i]=hex[(l>>28)&0xf]; l<<=4; }
    s.append("\r\n0\r\n\r\n"); // Add terminator
    int skip= 0; while (s.c[skip]=='0') skip++; // skip the initial zeros in size
    send2(sock, s.c+skip, int(s.w-skip), 0); // and send!
    return true;
}

// error to text...
static char const *msgFromEr(TAlpacaErr er)
{
    switch (er)
    {
    case ALPACA_OK: return "";
    case ALPACA_ERR_NOT_IMPLEMENTED: return "ALPACA_ERR_NOT_IMPLEMENTED";
    case ALPACA_ERR_INVALID_VALUE: return "ALPACA_ERR_INVALID_VALUE";
    case ALPACA_ERR_VALUE_NOT_SET: return "ALPACA_ERR_VALUE_NOT_SET";
    case ALPACA_ERR_NOT_CONNECTED: return "ALPACA_ERR_NOT_CONNECTED";
    case ALPACA_ERR_INVALID_WHILE_PARKED: return "ALPACA_ERR_INVALID_WHILE_PARKED";
    case ALPACA_ERR_INVALID_WHILE_SLAVED: return "ALPACA_ERR_INVALID_WHILE_SLAVED";
    case ALPACA_ERR_INVALID_OPERATION: return "ALPACA_ERR_INVALID_OPERATION";
    case ALPACA_ERR_ACTION_NOT_IMPLEMENTED: return "ALPACA_ERR_ACTION_NOT_IMPLEMENTED";
    }
    return "ALPACA Unknown Error";
}

// A series of functions that will add standard stuff in json format (depending on type of course)
// They all return true as this is the generic all went well return for the dispatch functions...
// The only difference between them if the value type (if there is one)...
static bool putEr(CMyStr *s, TAlpacaErr er)
{
    s->printf("\"ErrorNumber\":%d,\"ErrorMessage\":\"%s\",", er, msgFromEr(er));
    return true;
}
static bool putErVal(CMyStr *s, TAlpacaErr er, int v)
{
    s->printf("\"ErrorNumber\":%d,\"ErrorMessage\":\"%s\",\"Value\":%d", er, msgFromEr(er), v);
    return true;
}
static bool putErVal(CMyStr *s, TAlpacaErr er, uint32_t v)
{
    s->printf("\"ErrorNumber\":%d,\"ErrorMessage\":\"%s\",\"Value\":%d", er, msgFromEr(er), v);
    return true;
}
#ifndef _WIN32
static bool putErVal(CMyStr *s, TAlpacaErr er, int32_t v)
{
    s->printf("\"ErrorNumber\":%d,\"ErrorMessage\":\"%s\",\"Value\":%d", er, msgFromEr(er), v);
    return true;
}
#endif
static bool putErVal(CMyStr *s, TAlpacaErr er, double v)
{
    s->printf("\"ErrorNumber\":%d,\"ErrorMessage\":\"%s\",\"Value\":%f", er, msgFromEr(er), v);
    return true;
}
static bool putErVal(CMyStr *s, TAlpacaErr er, bool v)
{
    s->printf("\"ErrorNumber\":%d,\"ErrorMessage\":\"%s\",\"Value\":%s", er, msgFromEr(er), v?"true":"false");
    return true;
}
static bool putErVal(CMyStr *s, TAlpacaErr er, char const * v, bool transformquotes=false)
{
    s->printf("\"ErrorNumber\":%d,\"ErrorMessage\":\"%s\",\"Value\":\"%s\"", er, msgFromEr(er), v);
    return true;
}
static bool putErValRaw(CMyStr *s, TAlpacaErr er, char const * v, bool transformquotes=false)
{
    char *c= s->c+s->w;
    s->printf("\"ErrorNumber\":%d,\"ErrorMessage\":\"%s\",\"Value\":%s", er, msgFromEr(er), v);
    if (!transformquotes) return true;
    while (*c!=0) if (*c++=='\'') c[-1]='"';
    return true;
}

// Series of functions that will look for the value for a given parameter in the input...
// They differ by the type returned...
// note that if you want to use a string stuff directly, you better verify and sanitize it (there is a function for that, see lower)
char const *getStrData(char *m, char const *parameter)
{
    if (m==nullptr) return nullptr;
    size_t plen= strlen(parameter);
    while (true) // search parameter followed by "=" in string...
    {
        m= strstr(m, parameter); if (m==nullptr) return nullptr; 
        m+= plen; // skip parameter...
        if (*m=='=') return m+1; // and an '=', we got it!!!!
    }
}
// return 0 or 1 (false/true) or 2 for neither if you care!
int getBoolData(char *m, char const *parameter)
{
    char const *r= getStrData(m, parameter); if (r==nullptr) return 2;
    if (memcmp(r, "True",4)==0) return 1;
    if (memcmp(r, "False",5)==0) return 0;
    if (memcmp(r, "true",4)==0) return 1;
    if (memcmp(r, "false",5)==0) return 0;
    return 2;
}
int getIntData(char *m, char const *parameter)
{
    char const *r= getStrData(m, parameter); if (r==nullptr) return -1;
    return readInt(r);
}

// Dispatch for common functions to all devices...
bool CAlpacaDevice::dispatch(bool get, char const *url, char *data, CMyStr *s)
{
    if (!get && strcmp(url, "action") == 0) return putEr(s, action(getStrData(data, "Action"), getStrData(data,"Parameters")));
    if (!get && strcmp(url, "commandblind") == 0) return putEr(s, commandblind(getStrData(data, "Command")));
    if (!get && strcmp(url, "commandbool") == 0)
    {
        bool resp= true;
        return putErVal(s, commandbool(getStrData(data, "Command"), &resp), resp);
    }
    if (!get && strcmp(url, "commandstring") == 0)
    {
        char res[256]= "";
        return putErVal(s, commandstring(getStrData(data, "Command"), res, sizeof(res)), res);
    }
    if (!get && strcmp(url, "connect") == 0) return putEr(s, set_connected(true));
    if (!get && strcmp(url, "disconnect") == 0) return putEr(s, set_connected(false));
    if (get && strcmp(url, "connected") == 0) return putErVal(s, ALPACA_OK, get_connected());
    if (!get && strcmp(url, "connected") == 0) return putEr(s, set_connected(getBoolData(data,"Connected")));
    if (get && strcmp(url, "connecting") == 0) return putErVal(s, ALPACA_OK, get_connecting());
    if (get && strcmp(url, "driverinfo") == 0) return putErVal(s, ALPACA_OK, get_driverinfo());
    if (get && strcmp(url, "description") == 0) return putErVal(s, ALPACA_OK, get_description());
    if (get && strcmp(url, "driverversion") == 0) return putErVal(s, ALPACA_OK, get_driverversion());
    if (get && strcmp(url, "name") == 0) return putErVal(s, ALPACA_OK, get_name());
    if (get && strcmp(url, "interfaceversion") == 0) return putErVal(s, ALPACA_OK, get_interfaceversion());
    if (get && strcmp(url, "supportedactions") == 0) return putErVal(s, ALPACA_OK, get_supportedactions());
    return putEr(s, ALPACA_ERR_INVALID_OPERATION);
}

bool CFocuser::dispatch(bool get, char const *url, char *data, CMyStr *s)
{
    if (get && strcmp(url, "absolute") == 0) return putErVal(s, ALPACA_OK, get_absolute());
    if (get && strcmp(url, "maxincrement") == 0) return putErVal(s, ALPACA_OK, get_maxincrement());
    if (get && strcmp(url, "maxstep") == 0) return putErVal(s, ALPACA_OK, get_maxstep());
    if (get && strcmp(url, "position") == 0) return putErVal(s, ALPACA_OK, get_position());
    if (get && strcmp(url, "stepsize") == 0) return putErVal(s, ALPACA_OK, get_stepsize());
    if (get && strcmp(url, "ismoving") == 0) return putErVal(s, ALPACA_OK, get_ismoving());
    if (get && strcmp(url, "tempcomp") == 0) { bool comp= false; return putErVal(s, get_tempcomp(&comp), comp); }
    if (get && strcmp(url, "tempcompavailable") == 0) return putErVal(s, ALPACA_OK, get_tempcompavailable());
    if (get && strcmp(url, "temperature") == 0) { double temp= 0.0f; return putErVal(s, get_temperature(&temp), temp); }

    if (!get && strcmp(url, "tempcomp") == 0) return putEr(s, put_tempcomp(getBoolData(data, "TempComp")));
    if (!get && strcmp(url, "halt") == 0) return putEr(s, put_halt());
    if (!get && strcmp(url, "move") == 0) return putEr(s, put_move(getIntData(data, "Position")));
    return CAlpacaDevice::dispatch(get, url, data, s);
}

bool CFilterWheel::dispatch(bool get, char const *url, char *data, CMyStr *s)
{
    if (get && strcmp(url, "focusoffsets") == 0) return putErValRaw(s, ALPACA_OK, get_focusoffsets());
    if (get && strcmp(url, "names") == 0) return putErValRaw(s, ALPACA_OK, get_names(), true);

    if (get && strcmp(url, "position") == 0) return putErVal(s, ALPACA_OK, get_position());
    if (!get && strcmp(url, "position") == 0) return putEr(s, put_position(getIntData(data, "Position")));
    if (!get && strcmp(url, "move") == 0) return putEr(s, put_position(getIntData(data, "Position")));
    return CAlpacaDevice::dispatch(get, url, data, s);
}

/////////////////////////////////
// html data sanitation...
// Gets a pointer to date in an html data packet (typically, just after the '=')
// And will decode and place the data in buf, limiting itself to buflen...
// return buf for convinience...
// Here are example of input...
// wifi=hpmad2&wifip=aaa&fname=FocusServer&location=Mars
// wifi=aa&wifip=a%3Db&fname=c%26d&location=e%22%22
    static int getHex(char c)
    {
        if (c>='0' && c<='9') return c-'0';
        if (c>='A' && c<='F') return c-'A'+10;
        if (c>='a' && c<='f') return c-'a'+10;
        return -1;
    }
char *getHtmlString(char const *in, char *buf, size_t buflen)
{
    int i=0;
    while (i<buflen)
    {
        if (*in<=' ' || *in=='&' || *in=='=') break;
        if (*in=='+') { buf[i++]= ' '; in++; continue; }
        if (*in!='%') { buf[i++]= *in++; continue; }
        in++;
        int vh= getHex(*in++);
        int vl= getHex(*in++);
        if (vh<0 || vl<0) break;
        buf[i++]= (vh<<4)+vl;
    }
    buf[i]= 0;
    return buf;
}

///////////////////////////////////////////////
// Setup system...
static const char httpHtmlHeader[]="HTTP/1.0 200 OK\r\nContent-type: text/html; charset=UTF-8\r\nContent-Length: ";
static const char basicSetupPage[]=
"<html>"
"	<head>"
"		<meta name=\"viewport\" http-equiv=\"content-type\" content=\"text/html; charset=UTF-8,width=device-width, initial-scale=1.0\">"
"		<title>Alpaca server setup</title>"
"	</head>"
"	<body>"
"		<h1>Alpaca server setup</h1>"
"		<form action=\"/setup\">"
"			<table align=\"center\">"
"				<tr><td align=\"right\"><label for=\"wifi\">WiFi network name:</label></td>"
"					<td><input type=\"text\" id=\"wifi\" name=\"wifi\" value=\"%s\"></td></tr>"
"				<tr><td align=\"right\"><label for=\"wifip\">WiFi password:</label></td>"
"					<td><input type=\"text\" id=\"wifip\" name=\"wifip\"></td></tr>"
"				<tr><td align=\"right\"><label for=\"fname\">Device name:</label></td>"
"					<td><input type=\"text\" id=\"fname\" name=\"fname\" value=\"%s\"></td></tr>"
"				<tr><td align=\"right\"><label for=\"location\">Location:</label></td>"
"					<td><input type=\"text\" id=\"location\" name=\"location\" value=\"%s\"></td></tr>"
"			</table><br><input type=\"submit\" value=\"Submit\">"
"		</form>"
"		<h1>Alpaca devices setup</h1>";

static const char closeBodyStyleCloseHtml[]="</body>"
        "<style>"
        "	body{ background-color: black; color:white; text-align: center; }"
        "	h1{ margin-bottom: 40px; margin-top: 50px; text-align: center; }"
        "	a{ color: lightgreen; }"
        "</style>"
        "</html>";
bool CAlpaca::setup(int sock, bool get, char *data)
{
    if (data!=nullptr)
    {   // wifi=hpmad2&wifip=aaa&fname=FocusServer&location=Mars
        char const *d;
        saveLoadBegin();
        d= getStrData(data, "wifi"); if (d!=nullptr) save("wifi", getHtmlString(d, wifi, sizeof(wifi)));
        d= getStrData(data, "wifip"); if (d!=nullptr) save("wifip", getHtmlString(d, wifip, sizeof(wifip)));
        d= getStrData(data, "fname"); if (d!=nullptr) save("ServerName", getHtmlString(d, ServerName, sizeof(ServerName)));
        d= getStrData(data, "location"); if (d!=nullptr) save("Location", getHtmlString(d, Location, sizeof(Location)));
        saveLoadEnd();
    }
    CMyStr s;
    s.printf(basicSetupPage, wifi, ServerName, Location);
    for (int i=0; i<nbDevices; i++)
        s.printf("<h2><a href=\"setup/v1/%s/%d/setup\">%s %d:%s %s</a></h2>", devices[i]->get_type(), devices[i]->id, devices[i]->get_type(), devices[i]->id, devices[i]->get_name(), devices[i]->get_description());
    s+="</form>"; s+= closeBodyStyleCloseHtml;
    char t[sizeof(httpHtmlHeader)+10]; strcpy(t, httpHtmlHeader);
    char *e= t+strlen(t); _itoa(s.w, e, 10); strcat(e,"\r\n\r\n");
    send(sock, t, strlen(t), 0);
    send(sock, s.c, s.w, 0);
    return true;
}

static const char basicDeviceSetupPage[]=
"<html><head><meta name=\"viewport\" http-equiv=\"content-type\" content=\"text/html; charset=UTF-8,width=device-width, initial-scale=1.0\"><title>Alpaca %s setup</title></head><body>"
"<h1><a href=\"/setup\">Main setup</a></h1>"
"<h1>Alpaca %s %d:%s %s setup</h1>"
"<form action=\"/setup/v1/%s/%d/setup\">"
"<table align=\"center\">"
"  <tr><td align=\"right\"><label for=\"name\">name:</label></td>"
"      <td><input type=\"text\" id=\"name\" name=\"name\" value=\"%s\"></td></tr>"
"  <tr><td align=\"right\"><label for=\"description\">description:</label></td>"
"      <td><input type=\"text\" id=\"description\" name=\"description\" value=\"%s\"></td></tr></table>"
"  <input type=\"submit\" value=\"Submit\">"
"</form>";
bool CAlpacaDevice::setup(CAlpaca *Alpaca, int sock, bool get, char *data)
{
    if (data!=nullptr)
    {   
        char const *d; char t[30]; 
        strcpy(t, keyHeader); strcat(t, "Name");        d= getStrData(data, "name"); if (d!=nullptr) Alpaca->save(t, getHtmlString(d, Name, sizeof(Name)));
        strcpy(t, keyHeader); strcat(t, "Description"); d= getStrData(data, "description"); if (d!=nullptr) Alpaca->save(t, getHtmlString(d, Description, sizeof(Description)));
    }
    CMyStr s;
    s.printf(basicDeviceSetupPage, get_type(), get_type(), id, get_name(), get_description(), get_type(), id, get_name(), get_description());
    subSetup(Alpaca, sock, get, data, s);
    s+= closeBodyStyleCloseHtml;
    char t[sizeof(httpHtmlHeader)+10]; strcpy(t, httpHtmlHeader);
    char *e= t+strlen(t); _itoa(s.w, e, 10); strcat(e,"\r\n\r\n");
    send(sock, t, strlen(t), 0);
    send(sock, s.c, s.w, 0);
    return true;
}

void CFocuser::subSetup(CAlpaca *Alpaca, int sock, bool get, char *data, CMyStr &s)  // This allows you to add stuff in the HTML or handle inputs...
{
    if (data!=nullptr)
    {   
        char const *d= getStrData(data, "position");
        if (d!=nullptr && *d>='0' && *d<='9') put_move(readInt(d));
        d= getStrData(data, "Distance");
        if (d!=nullptr && *d>='0' && *d<='9') 
        {
            float f= readFloat(d);
            f=f*1000.0f/get_stepsize();
            put_move(int(f));
        }
    }
    if (data!=nullptr && data[0]==0) put_halt();
    s.printf("<h1>Position</h1>"
            "<table align=\"center\">"
            "<form action=\"/setup/v1/%s/%d/setup\">"
            "  <tr><td align=\"right\"><label for=\"position\">position:</label></td>"
            "      <td><input type=\"text\" id=\"position\" name=\"position\" value=\"%d\"></td>"
            "      <td><input type=\"submit\" value=\"GoTo\"></td></tr>"
            "</form>"
            "<form action=\"/setup/v1/%s/%d/setup\">"
            "  <tr><td align=\"right\"><label for=\"Distance\">Distance(mm):</label></td>"
            "      <td><input type=\"text\" id=\"Distance\" name=\"Distance\" value=\"%3f\"></td>"
            "      <td><input type=\"submit\" value=\"GoTo\"></td></tr>"
            "</form>"
            "</table>"
            "<form action=\"/setup/v1/%s/%d/setup\">"
            "  <input type=\"submit\" value=\"Stop\">"
            "</form>"
        , get_type(), id, get_position(), get_type(), id, float(get_position()*get_stepsize()/1000.0f), get_type(), id);
        if (get_ismoving())
            s.printf("<script> function autoRefresh() { window.location = \"/setup/v1/%s/%d/setup\"; } setInterval('autoRefresh()', 5000);</script>", get_type(), id );
 }

void CFilterWheel::subSetup(CAlpaca *Alpaca, int sock, bool get, char *data, CMyStr &s)  // This allows you to add stuff in the HTML or handle inputs...
{
    if (data!=nullptr)
    {   
        char const *d= getStrData(data, "position");
        if (d!=nullptr && *d>='0' && *d<='9') put_position(readInt(d));
        char t[30]; 
        strcpy(t, keyHeader); strcat(t, "names");        d= getStrData(data, "names"); if (d!=nullptr) Alpaca->save(t, getHtmlString(d, names, sizeof(names)));
        strcpy(t, keyHeader); strcat(t, "focusoffsets"); d= getStrData(data, "focusoffsets"); if (d!=nullptr) Alpaca->save(t, getHtmlString(d, focusoffsets, sizeof(focusoffsets)));
    }
    s.printf("<h1>Setup</h1>"
            "<form action=\"/setup/v1/%s/%d/setup\">"
            "<table align=\"center\">"
            "  <tr><td align=\"right\"><label for=\"names\">names (['name1','name2'...]):</label></td>"
            "  <td><input type=\"text\" id=\"names\" name=\"names\" value=\"%s\"></td></tr>"
            "  <tr><td align=\"right\"><label for=\"focusoffsets\">focus offsets ([0,0...]):</label></td>"
            "  <td><input type=\"text\" id=\"focusoffsets\" name=\"focusoffsets\" value=\"%s\"></td></tr>"
            "</table>"
            "  <input type=\"submit\" value=\"Submit\">"
            "</form>"
            "<h1>Set Filter</h1>"
            "<form action=\"/setup/v1/%s/%d/setup\">"
            "  <label for=\"position\">position:</label>"
            "  <input type=\"text\" id=\"position\" name=\"position\" value=\"%d\">"
            "  <input type=\"submit\" value=\"Set Wheel\">"
            "</form>"
            "<h1><a href=\"/setup/v1/%s/%d/setup\">Refresh</a></h1>"
        , get_type(), id, names, focusoffsets, get_type(), id, get_position(), get_type(), id);
}


//////////////////////////////////////
// Persistance stuff...
#ifndef _WIN32
void CAlpaca::save(char const *key, char const *v)
{
    saveLoadBegin();
    nvs_set_blob(saveLoadHandle, key, v, strlen(v)+1); saveLoadHandleDirty= true;
    saveLoadEnd();
}
void CAlpaca::save(char const *key, int32_t v)
{
    saveLoadBegin();
    nvs_set_i32(saveLoadHandle, key, v); saveLoadHandleDirty= true;
    saveLoadEnd();
}
void CAlpaca::save(char const *key, float v)
{
    int32_t t; memcpy(&t, &v, 4); save(key, t);
}
char *CAlpaca::load(char const *key, char const *def, char *buf, size_t buflen)
{
    saveLoadBegin();
    strncpy(buf, def, buflen); // copy default in result.
    esp_err_t err= nvs_get_blob(saveLoadHandle, key, buf, &buflen); // does NOT modify buf if error....
    saveLoadEnd();
    return buf;
}
int32_t CAlpaca::load(char const *key, int32_t def)
{
    saveLoadBegin();
    int32_t v;
    esp_err_t err= nvs_get_i32(saveLoadHandle, key, &v);
    if (err!=ESP_OK) v= def;
    saveLoadEnd(); return v;
}
float CAlpaca::load(char const *key, float def)
{
    int32_t d; memcpy(&d, &def, 4);
    int32_t t= load(key, d);
    memcpy(&def, &t, 4); return def;
}
void CAlpaca::saveLoadBegin()
{
    if (saveLoadCount++!=0) return;
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) { ESP_ERROR_CHECK(nvs_flash_erase()); err = nvs_flash_init(); }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(nvs_open("Alpaca", NVS_READWRITE, &saveLoadHandle));
    saveLoadHandleDirty= false;
}
void CAlpaca::saveLoadEnd()
{
    if (saveLoadCount==0) return; if (--saveLoadCount!=0) return;
    if (saveLoadHandle==0xffffffff) return;
    if (saveLoadHandleDirty) ESP_ERROR_CHECK(nvs_commit(saveLoadHandle));
    nvs_close(saveLoadHandle);
    saveLoadHandle= 0xffffffff;
}
#else
CPreference* Preference = nullptr;
template <typename T1> static int get(T1 *l, int s, char const *k)
{
    for (int i= 0; i<s; i++) if (strcmp(l[i].k, k)==0) return i;
    strcpy(l[s].k, k);
    return -1;
}
void CAlpaca::save(char const* key, char const* v)
{
    saveLoadBegin();
    int i= get(Preference->texts, Preference->usedText, key); if (i<0) i= Preference->usedText++;
    strcpy(Preference->texts[i].v, v);
    saveLoadHandleDirty = true;
    saveLoadEnd();
}
void CAlpaca::save(char const* key, int32_t v)
{
    saveLoadBegin();
    int i= get(Preference->ints, Preference->usedInts, key); if (i<0) i= Preference->usedInts++;
    Preference->ints[i].v= v;
    saveLoadEnd();
}
void CAlpaca::save(char const* key, float v) { int32_t t; memcpy(&t, &v, 4); save(key, t); }
char* CAlpaca::load(char const* key, char const* def, char* buf, size_t buflen)
{
    saveLoadBegin(); strcpy(buf, def);
    int i= get(Preference->texts, Preference->usedText, key); 
    if (i>=0) strcpy(buf, Preference->texts[i].v);
    saveLoadEnd();
    return buf;
}
int32_t CAlpaca::load(char const* key, int32_t def)
{
    saveLoadBegin();
    int i= get(Preference->ints, Preference->usedInts, key); 
    if (i>=0) def= Preference->ints[i].v;
    saveLoadEnd(); return def;
}
float CAlpaca::load(char const* key, float def)
{
    int32_t d; memcpy(&d, &def, 4);
    int32_t t = load(key, d);
    memcpy(&def, &t, 4); return def;
}
void CAlpaca::saveLoadBegin()
{
    if (saveLoadCount++ != 0) return;
    if (!Preference) Preference = new CPreference;
    Preference->load("ALpaca");
}
void CAlpaca::saveLoadEnd()
{
    if (saveLoadCount == 0) return; if (--saveLoadCount != 0) return;
    if (!saveLoadHandleDirty) return;
    Preference->save("Alpaca");
}
void CPreference::load(char const* fn)
{
    HANDLE h= CreateFileA(fn, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL); if (h==INVALID_HANDLE_VALUE) return;
    DWORD size= GetFileSize(h, NULL); 
    if (size==sizeof(*this))
    {
        uint8_t *data= (uint8_t*)this;
        size_t pos= 0;
        while (size>0) { DWORD read= 0; if (!ReadFile(h, data+pos, size, &read, NULL)) { CloseHandle(h); return; }; size-= read; pos+= read; }
    }
    CloseHandle(h);
}
void CPreference::save(char const* fn)
{
    HANDLE h= CreateFileA(fn, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL); if (h==INVALID_HANDLE_VALUE) return;
    DWORD write; WriteFile(h, this, sizeof(*this), &write, NULL);
    CloseHandle(h);
}
#endif