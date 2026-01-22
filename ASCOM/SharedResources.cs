using ASCOM.Utilities;
using System;
using System.Linq;
using ASCOM.Astrometry.Transform;
using ASCOM.Astrometry.AstroUtils;
using System.IO.Compression;
using System.Windows.Forms;
using System.Runtime.CompilerServices;

namespace ASCOM.LocalServer
{
    /// Add and manage resources that are shared by all drivers served by this local server here.
    /// In this example it's a serial port with a shared SendMessage method an idea for locking the message and handling connecting is given.
    /// Multiple drivers means that several drivers connect to the same hardware device, aka a hub.
    /// Multiple devices means that there are more than one instance of the hardware, such as two focusers. In this case there needs to be multiple instances
    /// of the hardware connector, each with it's own connection count.
    ///	ALL DECLARATIONS MUST BE STATIC HERE!! INSTANCES OF THIS CLASS MUST NEVER BE CREATED!
    [HardwareClass]
    public static class SharedResources
    {
        private static readonly object lockObject = new object(); // Object used for locking to prevent multiple drivers accessing common code at the same time

        public interface ILog { void log(string message, int source); };
        public static ILog log= null;
        public static void doLog(string msg, int source=0) 
        { 
            if (log==null) return; log.log(msg, source);
        }
        // Shared serial port. This will allow multiple drivers to use one single serial port.
        private static Serial SharedSerial = new Serial(); // Shared serial port
        public static string _comPort= "COM1";
        internal static Util utilities = new Util(); // ASCOM Utilities object for use as required
        internal static AstroUtils astroUtilities = new AstroUtils(); // ASCOM AstroUtilities object for use as required
        public static void Dispose()
        {
            Disconnect();
            try { if (SharedSerial != null) SharedSerial.Dispose(); } catch { }
        }
        public static int FocusserPosition
        {
            get { return _FocusserPosition; }
            //set { _FocusserPosition = value; doLog("Foc sync to "+value.ToString()); }
        }
        public static bool FocusMoving { get { return _FocusMoving; } }

        private static System.Timers.Timer timerPos;
        private static bool connectionLive = false;
        private static bool _FocusMoving = false;
        private static int _FocusserPosition = 0;
        public static bool dataDisplayed = false;
        public static int Filter = 0;
        public static bool buteeHaute = false, buteeBasse= false;
        public static int buteeHautePos = -1, buteeBassePos = -1;
        public static void Disconnect() // force disconnect. setting connected to false will NOT disconnect as multiple clients might be asking for a disconnection
        {
            lock (lockObject)
            {
                if (timerPos != null) { timerPos.Dispose(); timerPos = null; }
                connectionLive = false; dataDisplayed= false;
                if (SharedSerial!=null) SharedSerial.Connected = false;
                doLog("Disconnect");
            }
        }
        public static string comPort
        {
            get { return _comPort;  }
            set
            {
                if (value == _comPort) return;
                _comPort = value;
                if (SharedSerial == null) return;
                bool conn = connectionLive;
                Disconnect();
                if (conn) Connected = true;
                doLog("Set com to "+value);
            }
        }
        static public bool hadssid= false;
        static public string wifi = "", wifipass = "";
        public static bool Connected
        {
            set
            {
                lock (lockObject)
                {
                    doLog("Connect");
                    if (!value) return; // We actually do NOT disconnect when asked by a client... just when asked by the main app!
                    if (SharedSerial.Connected) return; // already connected...
                    SharedSerial.PortName = comPort;
                    SharedSerial.Speed = ASCOM.Utilities.SerialSpeed.ps115200;
                    SharedSerial.Connected = true;
                    SharedSerial.ReceiveTimeout = 1;
                    timerPos = new System.Timers.Timer(500);
                    timerPos.Elapsed += (source, e) =>
                    {  // update status every 1/2 second
                        try
                        {
                            string v = SendSerialCommand("!");
                            if (v.Length != 38) return; // invalid...
                            int i= 0;
                            i += 12; // skip 2 other focusser pos...
                            _FocusserPosition = readHex(v, ref i, i+6);
                            int bits= readHex(v, ref i, i+2);
                            _FocusMoving = (bits & 2) != 0;
                            buteeHaute = (bits & 8) != 0; buteeBasse = (bits & 4) != 0;
                            if (buteeHaute) buteeHautePos = _FocusserPosition;
                            if (buteeBasse) buteeBassePos = _FocusserPosition;
                            Filter = (int)(v[37]) - (int)'0'; // get filter wheel position
                            connectionLive = true;

                            if (!hadssid)
                            {
                                v = SendSerialCommand(":w#");
                                if (v.Length >= 1)
                                {
                                    string t = "";
                                    int ssidl = (int)v[0] - (int)'A';
                                    if (ssidl>0 && ssidl + 1 <= v.Length)
                                    {
                                        wifi = v.Substring(1, ssidl);
                                        wifipass = v.Substring(ssidl + 1);
                                        hadssid = true;
                                    }
                                }
                            }
                        }
                        catch (NotConnectedException)
                        {
                            SharedSerial.Connected= false;
                            timerPos.Dispose();
                            timerPos= null;
                            connectionLive= false;
                        }
                    };
                    timerPos.Enabled = true;
                }
            }
            get { return connectionLive; }
        }

        public static int readHex(string s, ref int i, int l=0)
        {
            int v = 0;
            if (l == 0) l = s.Length;
            while (i < l)
                if (s[i] >= '0' && s[i] <= '9') v = v * 16 + s[i++] - '0';
                else if (s[i] >= 'a' && s[i] <= 'f') v = v * 16 + s[i++] - 'a' + 10;
                else if (s[i] >= 'A' && s[i] <= 'F') v = v * 16 + s[i++] - 'A' + 10;
                else break;
            return v;
        }
        public static int readDec(string s, ref int i)
        {
            int v = 0;
            while (i < s.Length)
                if (s[i] >= '0' && s[i] <= '9') v = v * 10 + s[i++] - '0';
                else break;
            return v;
        }
        public static string SendSerialCommand(string command, int waitReturn=1) // 0: is no wait return, 1 is wait for '#' and 2 is get 1 char
        {
            if (!SharedSerial.Connected) return String.Empty;
            lock (lockObject)
            {
                try
                {
                    SharedSerial.ClearBuffers();
                    SharedSerial.Transmit(command);
                }
                catch (Exception) { Disconnect(); return String.Empty; } // end of work here...
                if (waitReturn==0) return String.Empty;
                try
                {
                    if (waitReturn == 1) return SharedSerial.ReceiveTerminated("#").Replace("#", String.Empty);
                    else return Convert.ToChar(SharedSerial.ReceiveByte()).ToString();
                }
                catch (Exception) { return String.Empty; } // timeout... not a deadly error
            }
        }
    }

}
