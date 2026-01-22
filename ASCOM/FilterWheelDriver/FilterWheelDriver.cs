using ASCOM.BrebissonRC600_filter_wheel.FilterWheel;
using ASCOM.DeviceInterface;
using ASCOM.LocalServer;
using System;
using System.Collections;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace ASCOM.BrebissonRC600_filter_wheel.FilterWheel
{
    // This code is mostly a presentation layer for the functionality in the FilterHardware class. You should not need to change the contents of this file very much, if at all.
    // Most customisation will be in the FilterHardware class, which is shared by all instances of the driver, and which must handle all aspects of communicating with your device.
    [ComVisible(true)]
    [Guid("79f68dd4-8ff6-4544-b2ba-2b764be387c1")]
    [ProgId("ASCOM.BrebissonV1.Filters")]
    [ServedClassName("ASCOM Filters Driver for BrebissonV1")] // Driver description that appears in the Chooser, customise as required
    [ClassInterface(ClassInterfaceType.None)]
    public class FilterWheel : ReferenceCountedObjectBase, IFilterWheelV2, IDisposable
    {
        internal static string DriverProgId; // ASCOM DeviceID (COM ProgID) for this driver, the value is retrieved from the ServedClassName attribute in the class initialiser.
        internal static string DriverDescription; // The value is retrieved from the ServedClassName attribute in the class initialiser.
        // connectedState holds the connection state from this driver instance's perspective, as opposed to the local server's perspective, which may be different because of other client connections.
        internal bool connectedState = false; // The connected state from this driver's perspective
        public FilterWheel()
        {
            try
            {
                // Pull the ProgID from the ProgID class attribute.
                Attribute attr = Attribute.GetCustomAttribute(this.GetType(), typeof(ProgIdAttribute));
                DriverProgId = ((ProgIdAttribute)attr).Value ?? "PROGID NOT SET!";  // Get the driver ProgIDfrom the ProgID attribute.
                // Pull the display name from the ServedClassName class attribute.
                attr = Attribute.GetCustomAttribute(this.GetType(), typeof(ServedClassNameAttribute));
                DriverDescription = ((ServedClassNameAttribute)attr).DisplayName ?? "DISPLAY NAME NOT SET!";  // Get the driver description that displays in the ASCOM Chooser from the ServedClassName attribute.
            }
            catch (Exception ex)
            {
                MessageBox.Show($"{ex.Message}", "Exception creating ASCOM.BrebissonV1.Filters", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
        ~FilterWheel() { }
        public void Dispose() { }
        public void SetupDialog() { FilterHardware.SetupDialog(); }
        public ArrayList SupportedActions { get { return new ArrayList(); } }
        public string Action(string actionName, string actionParameters) { return ""; }
        public void CommandBlind(string command, bool raw) { }
        public bool CommandBool(string command, bool raw) { return false; }
        public string CommandString(string command, bool raw) { return ""; }
        public bool Connected
        {
            get { return connectedState; }
            set { if (connectedState == value) return; FilterHardware.Connected = value; if (!value) connectedState = false; else connectedState = FilterHardware.Connected; }
        }
        public void connect() {  Connected = true; }
        public string Description { get { return FilterHardware.Description; } }
        public string DriverInfo { get { return FilterHardware.DriverInfo; } }
        public string DriverVersion { get { return FilterHardware.DriverVersion; } }
        public short InterfaceVersion { get { return FilterHardware.InterfaceVersion; } }
        public string Name { get { return FilterHardware.Name; } }
        public bool Link { get { return Connected; } set { Connected = value; } }
        public short Position { get { return (short)FilterHardware.Position; } set { FilterHardware.Position = value; } }
        public int[] FocusOffsets { get { return FilterHardware.FocusOffsets; } }
        public string[] Names { get { return FilterHardware.Names; } }

    }
}
