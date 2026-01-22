/***************************************************************
* Cyrille.de.Brebisson@gmail.com implementation of Ascom Alpaca device side for esp32 or similar
* To use, you first need to be connected to a network...
* 
* And you will have to create your devices, inheriting from the base device types...
* class CMyFilterWheel : public CFilterWheel { public:
*   CMyFilterWheel(int id): CFilterWheel(id, "CdB filters Driver", "1", "CdB filters", "filters for RC600") { }
*   char const *get_focusoffsets() override { return "[0,0,0]"; } // returns a json formated array of integer values... "[1, 2, 3]" for example
*   char const *get_names() override { return "[\"S\",\"H\",\"O\"]"; }  // returns a json formated array of string values... "["S", "H", "O"]" for example
*   int position= 0;
*   int get_position() override { return position; }
*   TAlpacaErr put_position(int32_t position) override { if (position>=3) return ALPACA_ERR_INVALID_VALUE; moveMotor(this->position= position); return ALPACA_OK; }
* };
* and initialize, then start the system in main...
*  CAlpaca *alpaca= new CAlpaca("CdB", "1", "FocusServer", "Mars");
*  alpaca->addDevice(new CMyFilterWheel(0)); // Add as many as you want. But the device ID (one counter per type), has to be correct...
*  alpaca->start();
* 
* At this point in time, I have mostly done the Focuser and FilterWheel.
* Dispatch will need to be created for any other device type... PRobably a 20 mn job per device...
* Have fun
* 
* Note: NO HTTP INPUT IS DEFENSIVELY VALIDATED!!!! NONE of the string putput from device is verified either (but this is more your fault)
* One could argue that this is "bad" and that I am a shitty programmer that does not understand security...
* I would argue back that Alpaca allows ANYONE with net access to manipulate HW which cna be worth thousand
* and that this is WAY more risky than a "bad actor" crashing the system! If I was a bad actor, I would 
* use the system to open domes when it rains or some other crap.... which is way worth than a FW bug...
* Anyway, if you plan to use this code in a "serious" setting. it won't be too hard to fix (probably a day's work).
* 
* For testing, you can of course compile and run this under windows!
* You will see some #ifndef _WIN32 throughout these files which are designed exactly for that...
****************************************************************/

#define _CRT_SECURE_NO_WARNINGS // allows windows compilation
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>
#ifndef _WIN32
#include "nvs_flash.h" // esp32 data storage...
#include "nvs.h"
#else
class CPreference { public: // A bad preference class for use in windows, limited to 200 keys...
    struct { char k[32]; int32_t v; } ints[100]; int usedInts= 0;
    struct { char k[32], v[32]; } texts[100]; int usedText= 0;
    void load(char const* fn); void save(char const* fn);
};
typedef int nvs_handle_t;
#endif

class CAlpacaDevice; // Generic class for all Alpaca devices..

//////////////////////////////////////////////////////
// The MAIN class!!!!
// change maxDevices if you need
// in order, create, call addDevice n times, and then start
class CAlpaca { public:
    static int const maxDevices=10;
    // Note that DefaultServerName and DefaultLocation can be overriden by user changing these in the setup. They will then be saved and
    // reloaded by the system...
    CAlpaca(char const *Manufacturer, char const *ManufacturerVersion, char const *DefaultServerName, char const *DefaultLocation);

    void addDevice(CAlpacaDevice *a);
    void start(int httpPort=80);

    // call to the setup api. return false to have the system return err404. Else send what you want through sock and return true to continue the connection...
    // get is true if get, false if put. data is the data passed to the server
    virtual bool setup(int sock, bool get, char *data);

    // this uses the nvf API to load/save data...
    // You can just call load or save, but if you have a number of load/save to do ina row
    // you can call saveLoadBegin first, then all your operations, then saveLoadEnd to commit. Will be faster...
    // all load operations have a "default" value should you need it...
    // This is there because it is used by the default setup stuff...
    void saveLoadBegin();
    void saveLoadEnd();
    void save(char const *key, char const *v);
    void save(char const *key, int32_t v);
    void save(char const *key, float v);
    // all the loads have a default value returned if error...
    char *load(char const *key, char const *def, char *buf, size_t buflen); // put data in buf. return it also for convinience... buf MUST be large enough for def
    int32_t load(char const *key, int32_t def);
    float load(char const *key, float def);



    // These should be private with a friend declaration... but I am too lazy!
    int volatile newClientSocket; // will be set to the next client socket to listen to...
    int httpport;
    // This one is called by the http client to execute requests...
    // return true if no errors and connection can stay alive
    bool execRequest(int sock, bool put, char *url, char *data);

    char ServerName[32], Manufacturer[32], ManufacturerVersion[32], Location[32], wifi[32], wifip[32];
private:
    CAlpacaDevice *devices[maxDevices]; int nbDevices= 0;
    char uniqueid[13]; // designed for mac address...
    void osinit(); // os specific intialisation
    // if url points to a device identifier (Focuser/0/ for example), return device and url after the id and the last '/'
    // else return nullprt. url will not have moved...
    CAlpacaDevice *deviceFromURL(char *&url); 

    // for data saving...
    nvs_handle_t saveLoadHandle= 0xffffffff; bool saveLoadHandleDirty= false; int saveLoadCount= 0;
};


enum TAlpacaErr { // Alpaca error code for returns if you need them...
    ALPACA_OK                                 = 0x000,
    ALPACA_ERR_NOT_IMPLEMENTED                = 0x400,
    ALPACA_ERR_INVALID_VALUE                  = 0x401,
    ALPACA_ERR_VALUE_NOT_SET                  = 0x402,
    ALPACA_ERR_NOT_CONNECTED                  = 0x407,
    ALPACA_ERR_INVALID_WHILE_PARKED           = 0x408,
    ALPACA_ERR_INVALID_WHILE_SLAVED           = 0x409,
    ALPACA_ERR_INVALID_OPERATION              = 0x40B,
    ALPACA_ERR_ACTION_NOT_IMPLEMENTED         = 0x40C};

// Used by dispatch. This is a sting class that grows by packs of 1K... allows to not have any dependencies...
// string which is allocated by 1k blocks and has a printf... Used to contruct responses...
class CMyStr { public:
    char *c= nullptr; size_t csize= 0, w= 0;
    CMyStr() { c= (char*)malloc(csize= 1024); }
    ~CMyStr() { free(c); }
    void grow() { c= (char*)realloc(c, csize= csize+1024); }
    void append(char const *s, size_t l=-1)
    {
        if (l==-1) l=strlen(s);
        if (w+l>csize-1) grow();
        memcpy(c+w, s, l); w+= l;
    }
    CMyStr &operator +=(char const *s) { append(s); return *this; }
    void printf(const char* format, ...)
    {
        if (csize-w<750) grow();
        va_list args; va_start(args, format); vsprintf(c+w, format, args); va_end(args);
        w+= strlen(c+w);
    }
};

// This is the generic Alpaca device class. All device types inherits from it...
class CAlpacaDevice { public:
    CAlpacaDevice(int id, char const *driverInfo, char const *driverVersion, char const *defaultName, char const *defaultDescription):id(id), driverInfo(driverInfo), driverVersion(driverVersion)
    { 
        strncpy(Name, defaultName, sizeof(Name)-1);
        strncpy(Description, defaultDescription, sizeof(Description)-1);
        _jsonDesc[0]= 0; // Can not generate here as C/C++ does not allow for virtual function calls in constructor (bad design! Pascal can!)
    }

    ///////////////////////////////////
    // Here you have a bunch of virtual functions that you COULD override if you wanted to... But in 99% of cases the default impementation is good enough...
        bool connected= false;
    virtual bool get_connected() { return connected; return ALPACA_OK; }
    virtual TAlpacaErr set_connected(bool connected) { this->connected= connected; return ALPACA_OK; }
    virtual bool get_connecting() { return false; return ALPACA_OK; }

    // If you want to use these, GO SEE THE DISPATCH and now action/command/parameters is handled because I am 100% certain that it will not work (as I have no use for it, I never checked)...
    // Also, all char data here is RAW data from the htlp request.
    // It is a pointer on the start of the text, and can continue past the real end...
    // Basically, this is what I get as input: Action=sddf%25%25&Parameters=sdfssdfsdf 
    // And action will be a pointer after Action= while parameters will be a pointer after Parameters=. Parse and sanitize as needed!
    // Look at the getHtmlString as it is mostly what you need/want...
    virtual TAlpacaErr action(const char *action, const char *parameters) { return ALPACA_ERR_ACTION_NOT_IMPLEMENTED; }
    virtual TAlpacaErr commandblind(const char *command) { return ALPACA_ERR_ACTION_NOT_IMPLEMENTED; }
    virtual TAlpacaErr commandbool(const char *command, bool *resp) { return ALPACA_ERR_ACTION_NOT_IMPLEMENTED; }
    virtual TAlpacaErr commandstring(const char *action, char *buf, size_t len) { return ALPACA_ERR_ACTION_NOT_IMPLEMENTED; }

    virtual char const *get_description() { return Description; }
    virtual char const *get_driverinfo() { return driverInfo; }
    virtual char const *get_driverversion() { return driverVersion; }
    virtual char const *get_name() { return Name; }
    virtual uint32_t get_interfaceversion() = 0;
    virtual char const *get_supportedactions() { return "[]"; } // return json OK array of string....
    ///////////////////////////////////


    // All classes will have a dispatch which will transform http requests into function calls...
    // As a general rule, inheriting classes will first test their known API and then default to the upperclass one...
    // get is true for http GET an false for http PUT
    // url is the url. Usually points on the command, or at least after the api/v1/type/id/ stuff
    // data would be any parameters raw from http
    // s is the json file that will be returned
    // return false to have the system return an error 400...
    virtual bool dispatch(bool get, char const *url, char *data, CMyStr *s);

    // call to the setup api. return false to have the system return err404. else send what you want through sock and return true to continue the connection...
    // get is true if get, false if put. data is the data passed to the server
    virtual bool setup(CAlpaca *Alpaca, int sock, bool get, char *data); 
    virtual void subSetup(CAlpaca *Alpaca, int sock, bool get, char *data, CMyStr &s) { } // This allows you to add stuff in the HTML or handle inputs...

    // Allows you to load stuff from persistance if you want...
    // t[0..tl-1] is the device specific key start and t is 30 chr long, so you can strcpy(t+tl, "keyName") and do a load on t...
    // see example in FilterWheel
    virtual void subLoad(CAlpaca *alpaca, char *t, int tl) { }


    // You should NOT have to touch any of this as this is internal stuff
    // most of stuff here should be private, but it's only used by Alpaca in one spot... and I can't be bother to friend it...
    int id;
    char const *driverInfo, *driverVersion;
    char Name[32], Description[32];
    virtual char const *get_type()= 0; 
    char _jsonDesc[512];          // Will hold afer the addition of the device to alpaca the device description for telling clients about who you are... initialized once...
    char keyHeader[10];           // Will contain some text used to prefix any key for load/save... see load/save in addDevice and setup...
};

//////////////////////////////////
// now, you have all the supported device types!
// see alpaca documention to know what every function does...

// This one works...
class CFilterWheel : public CAlpacaDevice { public: 
    CFilterWheel(int id, char const *driverInfo, char const *driverVersion, char const *defaultName, char const *defaultDescription): CAlpacaDevice(id, driverInfo, driverVersion, defaultName, defaultDescription) 
    { strcpy(focusoffsets, "[]"); strcpy(names, "[]"); }
    uint32_t get_interfaceversion() override { return 3; }
    virtual char const *get_focusoffsets() { return focusoffsets; }; // returns a json formated array of integer values... "[1, 2, 3]" for example
    virtual char const *get_names() {return names; };  // returns a json formated array of string values... "["S", "H", "O"]" for example
    virtual int get_position() = 0;
    virtual TAlpacaErr put_position(int32_t position) = 0;
    void subLoad(CAlpaca *alpaca, char *t, int tl) override;

    bool dispatch(bool get, char const *url, char *data, CMyStr *s);
    void subSetup(CAlpaca *Alpaca, int sock, bool get, char *data, CMyStr &s);  // This allows you to add stuff in the HTML or handle inputs...
protected:
    char const *get_type() override { return "FilterWheel"; }
    char focusoffsets[128], names[128];
};

// This one works...
class CFocuser : public CAlpacaDevice { public: CFocuser(int id, char const *driverInfo, char const *driverVersion, char const *defaultName, char const *defaultDescription): CAlpacaDevice(id, driverInfo, driverVersion, defaultName, defaultDescription) { }
    uint32_t get_interfaceversion() override { return 4; }
    virtual bool get_absolute() = 0;
    virtual bool get_ismoving() = 0;
    virtual int32_t get_maxincrement() = 0;
    virtual int32_t get_maxstep() = 0;
    virtual int32_t get_position() = 0;
    virtual int32_t get_stepsize() = 0;
    virtual TAlpacaErr put_halt() = 0;
    virtual TAlpacaErr put_move(int32_t position) = 0;
    virtual TAlpacaErr get_tempcomp(bool *tempcomp) { return ALPACA_ERR_ACTION_NOT_IMPLEMENTED; }
    virtual TAlpacaErr put_tempcomp(bool tempcomp) { return ALPACA_ERR_ACTION_NOT_IMPLEMENTED; }
    virtual bool get_tempcompavailable() { return false; }
    virtual TAlpacaErr get_temperature(double *temperature) { return ALPACA_ERR_ACTION_NOT_IMPLEMENTED; }

    bool dispatch(bool get, char const *url, char *m, CMyStr *s) override;
    void subSetup(CAlpaca *Alpaca, int sock, bool get, char *data, CMyStr &s) override; // This allows you to add stuff in the HTML or handle inputs...
protected:
    char const *get_type() override { return "Focuser"; }
};

// This has never been tested and will not work as I have not written the dispatch!
class CCamera : public CAlpacaDevice { public: CCamera(int id, char const *driverInfo, char const *driverVersion, char const *defaultName, char const *defaultDescription): CAlpacaDevice(id, driverInfo, driverVersion, defaultName, defaultDescription) { }
protected:
    uint32_t get_interfaceversion() override { return 4; }
    char const *get_type() override { return "Camera"; }
};

// This has never been tested and will not work as I have not written the dispatch!
class CCoverCalibrator : public CAlpacaDevice { public: CCoverCalibrator(int id, char const *driverInfo, char const *driverVersion, char const *defaultName, char const *defaultDescription): CAlpacaDevice(id, driverInfo, driverVersion, defaultName, defaultDescription) { }
    uint32_t get_interfaceversion() override { return 2; }
    virtual TAlpacaErr get_brightness(uint32_t *brightness) = 0;
      enum class CalibratorState { NotPresent, Off, NotReady, Ready, Unknown, Error };
    virtual TAlpacaErr get_calibratorstate(CalibratorState *state) = 0;
      enum CoverState { NotPresent, Closed, Moving, Open, Unknown, Error };
    virtual TAlpacaErr get_coverstate(CoverState *state) = 0;
    virtual TAlpacaErr get_maxbrightness(uint32_t *max) = 0;
    virtual TAlpacaErr turn_calibratoroff() = 0;
    virtual TAlpacaErr turn_calibratoron(int32_t brightness) = 0;
    virtual TAlpacaErr closecover() = 0;
    virtual TAlpacaErr opencover() = 0;
    virtual TAlpacaErr haltcover() = 0;
protected:
    char const *get_type() override { return "CoverCalibrator"; }
};

// This has never been tested and will not work as I have not written the dispatch!
class CDome : public CAlpacaDevice { public: CDome(int id, char const *driverInfo, char const *driverVersion, char const *defaultName, char const *defaultDescription): CAlpacaDevice(id, driverInfo, driverVersion, defaultName, defaultDescription) { }
    uint32_t get_interfaceversion() override { return 3; }
    virtual TAlpacaErr get_altitude(float *altitude) = 0;
    virtual TAlpacaErr get_athome(bool *athome) = 0;
    virtual TAlpacaErr get_atpark(bool *atpark) = 0;
    virtual TAlpacaErr get_azimuth(float *azimuth) = 0;
    virtual TAlpacaErr get_canfindhome(bool *canfindhome) = 0;
    virtual TAlpacaErr get_canpark(bool *canpark) = 0;
    virtual TAlpacaErr get_cansetaltitude(bool *cansetaltitude) = 0;
    virtual TAlpacaErr get_cansetazimuth(bool *cansetazimuth) = 0;
    virtual TAlpacaErr get_cansetpark(bool *cansetpark) = 0;
    virtual TAlpacaErr get_cansetshutter(bool *cansetshutter) = 0;
    virtual TAlpacaErr get_canslave(bool *canslave) = 0;
    virtual TAlpacaErr get_cansyncazimuth(bool *cansyncazimuth) = 0;
      enum ShutterState { Open, Closed, Opening, Closing, Error };
    virtual TAlpacaErr get_shutterstatus(ShutterState *shutterstatus) = 0;
    virtual TAlpacaErr get_slaved(bool *slaved) = 0;
    virtual TAlpacaErr put_slaved(bool slaved) = 0;
    virtual TAlpacaErr get_slewing(bool *slewing) = 0;
    virtual TAlpacaErr put_abortslew() = 0;
    virtual TAlpacaErr put_closeshutter() = 0;
    virtual TAlpacaErr put_findhome() = 0;
    virtual TAlpacaErr put_openshutter() = 0;
    virtual TAlpacaErr put_park() = 0;
    virtual TAlpacaErr put_setpark() = 0;
    virtual TAlpacaErr put_slewtoaltitude(float altitude) = 0;
    virtual TAlpacaErr put_slewtoazimuth(float azimuth) = 0;
    virtual TAlpacaErr put_synctoazimuth(float azimuth) = 0;
protected:
    char const *get_type() override { return "Dome"; }
};

// This has never been tested and will not work as I have not written the dispatch!
class CObservingConditions : public CAlpacaDevice { public: CObservingConditions(int id, char const *driverInfo, char const *driverVersion, char const *defaultName, char const *defaultDescription): CAlpacaDevice(id, driverInfo, driverVersion, defaultName, defaultDescription) { }
    uint32_t get_interfaceversion() override { return 2; }
    virtual TAlpacaErr get_averageperiod(double *averageperiod) = 0;
    virtual TAlpacaErr put_averageperiod(double averageperiod) = 0;
    virtual TAlpacaErr get_cloudcover(double *cloudcover) = 0;
    virtual TAlpacaErr get_dewpoint(double *dewpoint) = 0;
    virtual TAlpacaErr get_humidity(double *humidity) = 0;
    virtual TAlpacaErr get_pressure(double *pressure) = 0;
    virtual TAlpacaErr get_rainrate(double *rainrate) = 0;
    virtual TAlpacaErr get_skybrightness(double *skybrightness) = 0;
    virtual TAlpacaErr get_skyquality(double *skyquality) = 0;
    virtual TAlpacaErr get_skytemperature(double *skytemperature) = 0;
    virtual TAlpacaErr get_starfwhm(double *starfwhm) = 0;
    virtual TAlpacaErr get_temperature(double *temperature) = 0;
    virtual TAlpacaErr get_winddirection(double *winddirection) = 0;
    virtual TAlpacaErr get_windgust(double *windgust) = 0;
    virtual TAlpacaErr get_windspeed(double *windspeed) = 0;
    virtual TAlpacaErr put_refresh() = 0;
    virtual TAlpacaErr get_sensordescription(const char *sensorname, char *buf, size_t len) = 0;
    virtual TAlpacaErr get_timesincelastupdate(double *timesincelastupdate) = 0;
protected:
    char const *get_type() override { return "ObservingConditions"; }
};

// This has never been tested and will not work as I have not written the dispatch!
class CRotator : public CAlpacaDevice { public: CRotator(int id, char const *driverInfo, char const *driverVersion, char const *defaultName, char const *defaultDescription): CAlpacaDevice(id, driverInfo, driverVersion, defaultName, defaultDescription) { }
    uint32_t get_interfaceversion() override { return 4; }
    virtual TAlpacaErr get_canreverse(bool *canreverse) = 0;
    virtual TAlpacaErr get_ismoving(bool *ismoving) = 0;
    virtual TAlpacaErr get_mechanicalposition(double *mechanicalposition) = 0;
    virtual TAlpacaErr get_position(double *position) = 0;
    virtual TAlpacaErr get_reverse(bool *reverse) = 0;
    virtual TAlpacaErr put_reverse(bool reverse) = 0;
    virtual TAlpacaErr get_stepsize(double *stepsize) = 0;
    virtual TAlpacaErr get_targetposition(double *targetposition) = 0;
    virtual TAlpacaErr put_halt() = 0;
    virtual TAlpacaErr put_move(double position) = 0;
    virtual TAlpacaErr put_moveabsolute(double position) = 0;
    virtual TAlpacaErr put_movemechanical(double position) = 0;
    virtual TAlpacaErr put_sync(double position) = 0;
protected:
    char const *get_type() override { return "Rotator"; }
};

// This has never been tested
class CSafetyMonitor : public CAlpacaDevice { public: CSafetyMonitor(int id, char const *driverInfo, char const *driverVersion, char const *defaultName, char const *defaultDescription): CAlpacaDevice(id, driverInfo, driverVersion, defaultName, defaultDescription) { }
    uint32_t get_interfaceversion() override { return 3; }
    virtual TAlpacaErr get_issafe(bool *issafe) = 0;
protected:
    char const *get_type() override { return "SafetyMonitor"; }
};

// This has never been tested and will not work as I have not written the dispatch!
class CSwitch : public CAlpacaDevice { public: CSwitch(int id, char const *driverInfo, char const *driverVersion, char const *defaultName, char const *defaultDescription): CAlpacaDevice(id, driverInfo, driverVersion, defaultName, defaultDescription) { }
    uint32_t get_interfaceversion() override { return 3; }
    virtual TAlpacaErr get_maxswitch(int32_t *maxswitch) = 0;
    virtual TAlpacaErr get_canwrite(int32_t id, bool *canwrite) = 0;
    virtual TAlpacaErr get_getswitch(int32_t id, bool *getswitch) = 0;
    virtual TAlpacaErr get_getswitchdescription(int32_t id, char *buf, size_t len) = 0;
    virtual TAlpacaErr get_getswitchname(int32_t id, char *buf, size_t len) = 0;
    virtual TAlpacaErr get_getswitchvalue(int32_t id, double *value) = 0;
    virtual TAlpacaErr get_minswitchvalue(int32_t id, double *value) = 0;
    virtual TAlpacaErr get_maxswitchvalue(int32_t id, double *value) = 0;
    virtual TAlpacaErr put_setswitch(int32_t id, bool value) = 0;
    virtual TAlpacaErr put_setswitchname(int32_t id, const char *name) = 0;
    virtual TAlpacaErr put_setswitchvalue(int32_t id, double value) = 0;
    virtual TAlpacaErr get_switchstep(int32_t id, double *switchstep) = 0;
protected:
    char const *get_type() override { return "Switch"; }
};

// This has never been tested and will not work as I have not written the dispatch!
class CTelescope : public CAlpacaDevice { public: CTelescope(int id, char const *driverInfo, char const *driverVersion, char const *defaultName, char const *defaultDescription): CAlpacaDevice(id, driverInfo, driverVersion, defaultName, defaultDescription) { }
protected:
    uint32_t get_interfaceversion() override { return 4; }
    char const *get_type() override { return "Telescope"; }
};

// Series of functions that will look for the value for a given parameter an http form input...
// They differ by the type returned...
// note that if you want to use a string stuff directly, you better verify and sanitize it (there is a function for that, see lower)
char const *getStrData(char *m, char const *parameter);
int getBoolData(char *m, char const *parameter); // return 0 or 1 (false/true) or 2 for neither if you care!
int getIntData(char *m, char const *parameter);
