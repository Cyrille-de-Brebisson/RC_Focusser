using ASCOM.LocalServer;
using ASCOM.Utilities;
using System;
using System.Globalization;
using System.Linq;
using System.Windows.Forms;

namespace ASCOM.BrebissonRC600_filter_wheel.FilterWheel
{
    [HardwareClass()] // Class attribute flag this as a device hardware class that needs to be disposed by the local server when it exits.
    internal static class FilterHardware
    {
        // Constants used for Profile persistence
        private static string DriverProgId = ""; // ASCOM DeviceID (COM ProgID) for this driver, the value is set by the driver's class initialiser.
        static FilterHardware()
        {
            DriverProgId = FilterWheel.DriverProgId; // Get this device's ProgID so that it can be used to read the Profile configuration values
            try
            {
                using (Profile driverProfile = new Profile())
                {
                    driverProfile.DeviceType = "FilterWheel";
                    string s = driverProfile.GetValue(DriverProgId, "names", "");
                    FilterHardware.Names = s.Split(';');
                    s = driverProfile.GetValue(DriverProgId, "offsets", "");
                    string []l = s.Split(';');
                    FocusOffsets = new int[l.Length];
                    for (int j = 0; j < l.Length; j++)
                    {
                        int t = 0;
                        if (int.TryParse(l[j], out t)) FocusOffsets[j] = t;
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"{ex.Message}", "Exception creating ASCOM.BrebissonV1.Filter", MessageBoxButtons.OK, MessageBoxIcon.Error);
                throw;
            }
        }

        public static void SetupDialog()
        {
            // Don't permit the setup dialogue if already connected
            MessageBox.Show("Please use main window to setup");
        }
        public static void saveProfile()
        {
            using (Profile driverProfile = new Profile())
            {
                driverProfile.DeviceType = "FilterWheel";
                string s = "";
                for (int j = 0; j < FilterHardware.Names.Length; j++)
                {
                    s += FilterHardware.Names[j]; if (j < FilterHardware.Names.Length - 1) s += ';';
                }
                driverProfile.WriteValue(DriverProgId, "names", s);
                s = "";
                for (int j = 0; j < FilterHardware.FocusOffsets.Length; j++)
                {
                    s += FilterHardware.FocusOffsets[j].ToString(); if (j < FilterHardware.FocusOffsets.Length - 1) s += ';';
                }
                driverProfile.WriteValue(DriverProgId, "offsets", s);
            }
        }
        public static void Dispose() { }
        public static bool Connected
        {
            get { return SharedResources.Connected; }
            set { SharedResources.Connected = value; }
        }
        internal static bool Link /// State of the connection to the focuser.
        {
            get { return Connected; } // Direct function to the connected method, the Link method is just here for backwards compatibility
            set { Connected = value; } // Direct function to the connected method, the Link method is just here for backwards compatibility
        }
        public static string Description { get { return "Brebisson RC600 Filter driver"; } }
        public static string DriverInfo { get { return "Information about the driver itself. Version: 1.0"; } }
        public static string DriverVersion { get { return "1.0"; } }
        public static short InterfaceVersion { get { return 2; } }
        public static string Name { get { return "Brebisson Combined Filter Focus V1"; } }
        internal static bool Absolute { get { return true; } } // This is an absolute focuser

        internal static int Position
        {
            get { return SharedResources.Filter; }
            set { string s = ":f" + (char)(48 + value) + '#'; SharedResources.SendSerialCommand(s, 0); }
        }
        internal static int[] FocusOffsets;
        internal static string[] Names;
    }
}

