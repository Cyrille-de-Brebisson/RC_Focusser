using ASCOM.Astrometry.NOVASCOM;
using ASCOM.BrebissonV1.Focuser;
using ASCOM.BrebissonRC600_filter_wheel.FilterWheel;
using ASCOM.DeviceInterface;
using ASCOM.Utilities;
using System;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Windows.Forms;
using static ASCOM.LocalServer.SharedResources;
using static System.Net.WebRequestMethods;
using static System.Windows.Forms.LinkLabel;
//using Ngcs;

namespace ASCOM.LocalServer
{
    public partial class FrmMain : Form, SharedResources.ILog
    {
        private delegate void SetTextCallback(string text);

        Focuser f = new Focuser();
        FilterWheel fw = new FilterWheel();
        private System.Timers.Timer timerPos;
        private bool init = true;
        public FrmMain()
        {
            InitializeComponent();
            this.ShowInTaskbar = true;
            this.Visible = true;
            label1.Text = label1.Text + " " + FocuserHardware.DriverVersion;
            button2.Text = f.Connected ? "Disconnect" : "Connect";
            rescanPort();
            timerPos = new System.Timers.Timer(500);
            timerPos.Elapsed += (source, e) => { updateConnectedLabel(); };
            timerPos.Enabled = true;
            StepSize.Text= f.StepSize.ToString();
            textBox2.Text = FocuserHardware.fastSpeed.ToString();
            textBox5.Text = FocuserHardware.slowSpeed.ToString();
            SharedResources.log= this;
            Width= groupBox2.Width+groupBox2.Left*2+Width-ClientSize.Width;
            //Height= groupBox2.Height+groupBox2.Top+groupBox2.Left+Height-ClientSize.Height;
            setWheelNames();
            for (int i = 0; i < fw.Names.Length; i++)
            {
                textBox3.Text += fw.Names[i];
                if (i < fw.Names.Length - 1) textBox3.Text += Environment.NewLine;
            }
            for (int i = 0; i < fw.FocusOffsets.Length; i++)
            {
                textBox4.Text += fw.FocusOffsets[i].ToString();
                if (i < fw.FocusOffsets.Length - 1) textBox4.Text += Environment.NewLine;
            }
            init = false;
            updateSavedPosList();
        }
        ~FrmMain() { SharedResources.log= null; }
        private int lastFocusPos= -1;
        bool hadwifi = false;
        private void updateConnectedLabel() // every 500ms, update UI based on driver's data
        {
            try
            {
                BeginInvoke((MethodInvoker)delegate ()
                {
                    labelCom.Text = SharedResources.comPort;
                    if (SharedResources.Connected)
                    {
                        button2.Text = "Disconnect";
                        if (lastFocusPos!=SharedResources.FocusserPosition) 
                        { 
                            lastFocusPos= SharedResources.FocusserPosition; 
                            textBox1.Text = SharedResources.FocusserPosition.ToString(); 
                        }
                        button1.Text = SharedResources.FocusMoving ? "Stop" : "Goto"; button1.Enabled= true;
                        textBox1.Enabled = !SharedResources.FocusMoving;
                        button8.Enabled = true;
                        button9.Enabled = true;
                        button10.Enabled = true;
                        button11.Enabled = true;
                        button19.Enabled = true;
                        button20.Enabled = SharedResources.buteeBassePos!=-1;
                        button21.Enabled = SharedResources.buteeBassePos!=-1;
                        textBox9.Enabled = SharedResources.buteeBassePos!=-1;
                        if (!comboBox1.DroppedDown)
                            comboBox1.SelectedIndex = FilterHardware.Position;
                        if (SharedResources.hadssid != hadwifi)
                        {
                            textBox6.Text = SharedResources.wifi;
                            textBox7.Text = SharedResources.wifipass;
                            hadwifi = SharedResources.hadssid;
                        }
                    }
                    else
                    {
                        button2.Text = "Connect";
                        textBox1.Text = ""; textBox1.Enabled = false;
                        button1.Text = "Goto"; button1.Enabled = false;
                        button8.Enabled = false;
                        button9.Enabled = false;
                        button10.Enabled = false;
                        button11.Enabled = false;
                        lastFocusPos= -1;
                        button19.Enabled = false;
                        button20.Enabled = false;
                        button21.Enabled = false;
                        textBox9.Enabled = false;
                    }
                });
            } 
            catch (Exception ex) { }
        }
        private void button2_Click(object sender, EventArgs e)
        {
            if (SharedResources.Connected) SharedResources.Disconnect();
            else SharedResources.Connected = true;
        }
        private void focusTo()
        {
            int v; if (int.TryParse(textBox1.Text, out v)) f.Move(v);
        }
        private void reeanbleMotors()
        {
            if (!reenableAllMotors) return;
            reenableAllMotors = false;
            SharedResources.SendSerialCommand(":en7#");
        }
        private void button1_Click(object sender, EventArgs e) // focus to
        {
            reeanbleMotors();
            if (SharedResources.FocusMoving) f.Stop();
            else focusTo();
        }

        private void comboBoxComPort_SelectedIndexChanged(object sender, EventArgs e)
        {
            SharedResources.comPort= comboBoxComPort.SelectedItem.ToString();
            if (init) return;
            FocuserHardware.saveProfile();
        }
        private bool setupVisible = false;
        private void rewidth(bool grow)
        {
            if (grow)
                Width= groupBox7.Width+groupBox7.Left+groupBox2.Left+Width-ClientSize.Width;
            else
                Width= groupBox2.Width+groupBox2.Left*2+Width-ClientSize.Width;
        }
        private void button15_Click(object sender, EventArgs e)
        {
            setupVisible = !setupVisible; rewidth(setupVisible); 
            groupBox7.Visible= setupVisible;
        }

        private void rescan_Click(object sender, EventArgs e) { rescanPort(); }
        void rescanPort()
        {
            // set the list of COM ports to those that are currently available
            comboBoxComPort.Items.Clear(); // Clear any existing entries
            using (Serial serial = new Serial()) // User the Se5rial component to get an extended list of COM ports
                comboBoxComPort.Items.AddRange(serial.AvailableCOMPorts);
            // select the current port if possible
            if (comboBoxComPort.Items.Contains(SharedResources.comPort))
                comboBoxComPort.SelectedItem = SharedResources.comPort;
        }

        private void StepSize_TextChanged(object sender, EventArgs e)
        {
            if (init) return;
            double v; if (double.TryParse(StepSize.Text, out v)) { f.StepSize = v; FocuserHardware.saveProfile(); }
            }

        // fast in
        private void button8_MouseDown(object sender, MouseEventArgs e) { reeanbleMotors(); FocuserHardware.moveIn(FocuserHardware.fastSpeed); }
        private void button8_MouseLeave(object sender, EventArgs e) { FocuserHardware.Stop(); }
        private void button8_MouseUp(object sender, MouseEventArgs e) { FocuserHardware.Stop(); }
        // fase out
        private void button9_MouseDown(object sender, MouseEventArgs e) { reeanbleMotors(); FocuserHardware.moveOut(FocuserHardware.fastSpeed); }
        private void button9_MouseLeave(object sender, EventArgs e) { FocuserHardware.Stop(); }
        private void button9_MouseUp(object sender, MouseEventArgs e) { FocuserHardware.Stop(); }

        private void button11_MouseDown(object sender, MouseEventArgs e) { reeanbleMotors(); FocuserHardware.moveIn(FocuserHardware.slowSpeed); }
        private void button11_MouseLeave(object sender, EventArgs e) { FocuserHardware.Stop(); }
        private void button11_MouseUp(object sender, MouseEventArgs e) { FocuserHardware.Stop(); }
        private void button10_MouseDown(object sender, MouseEventArgs e) { reeanbleMotors(); FocuserHardware.moveOut(FocuserHardware.slowSpeed); }
        private void button10_MouseLeave(object sender, EventArgs e) { FocuserHardware.Stop(); }
        private void button10_MouseUp(object sender, MouseEventArgs e) { FocuserHardware.Stop(); }

        private void textBox2_TextChanged(object sender, EventArgs e)
        {
            if (init) return;
            int i;
            if (int.TryParse(textBox2.Text, out i)) if (FocuserHardware.fastSpeed!=i) { FocuserHardware.fastSpeed = i; FocuserHardware.saveProfile(); }
        }

        private void textBox5_TextChanged(object sender, EventArgs e)
        {
            if (init) return;
            int i;
            if (int.TryParse(textBox5.Text, out i)) if (FocuserHardware.slowSpeed != i) { FocuserHardware.slowSpeed = i; FocuserHardware.saveProfile(); }
        }

        private void textBox4_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (e.KeyChar == (char)Keys.Enter) focusTo();
        }
        public void log(string message, int source)
        {
            if (groupBox7.Visible)
            try
            {
               BeginInvoke((MethodInvoker)delegate (){ logBox.AppendText(message+"\r\n"); });
            } catch (Exception ex) { }
            
        }

        private void button14_Click(object sender, EventArgs e)
        {
            logBox.Text= "";
        }
        private void setWheelNames()
        {
            comboBox1.Items.Clear();
            if (fw.Names == null) return;
            for (int i = 0; i < fw.Names.Length; i++)
                comboBox1.Items.Add(fw.Names[i]);
        }

        private void textBox3_TextChanged(object sender, EventArgs e)
        {

        }

        private void comboBox1_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (FilterHardware.Position!= comboBox1.SelectedIndex) FilterHardware.Position= comboBox1.SelectedIndex;
        }

        private void button3_Click(object sender, EventArgs e)
        {
            FilterHardware.Names= textBox3.Lines;
            string[] d = textBox4.Lines;
            int[]l= new int[d.Length];
            for (int i=0; i<d.Length; i++)
            {
                int j; if (int.TryParse(d[i], out j)) l[i] = j;
            }
            FilterHardware.FocusOffsets = l;
            FilterHardware.saveProfile();
            setWheelNames();
        }
        
        private void button4_Click(object sender, EventArgs e)
        {
            SharedResources.SendSerialCommand(":W" + (char)(((int)'A') + textBox6.Text.Length) + textBox6.Text + textBox7.Text + '#');
            SharedResources.hadssid = false;
        }

        int TM1Cnt = 0, TM2Cnt = 0, TM3Cnt = 0;

        private void button18_Click(object sender, EventArgs e)
        {
            SharedResources.SendSerialCommand(":en7#");
        }
        private int tiltSteps()
        {
            int steps = 0;
            if (int.TryParse(textBox8.Text, out steps)) return steps;
            return 0;
        }
        private bool reenableAllMotors = false;
        private void tiltMove(int dir, char motor, ref int cnt)
        {
            int steps = tiltSteps(); if (steps == 0) return;
            steps = (int)(steps/f.StepSize);
            reenableAllMotors= true;
            SharedResources.SendSerialCommand(":en"+motor+'#');
            f.Move(f.Position + steps*dir);
            cnt += steps * dir;
            updateTiltUI();
        }
        private void button5_Click(object sender, EventArgs e)
        {
            tiltMove(1, '1', ref TM1Cnt);
        }

        private void button6_Click(object sender, EventArgs e)
        {
            tiltMove(-1, '1', ref TM1Cnt);
        }

        private void button12_Click(object sender, EventArgs e)
        {
            tiltMove(1, '2', ref TM2Cnt);
        }

        private void button7_Click(object sender, EventArgs e)
        {
            tiltMove(-1, '2', ref TM2Cnt);
        }

        private void button16_Click(object sender, EventArgs e)
        {
            tiltMove(1, '3', ref TM3Cnt);
        }

        private void button13_Click(object sender, EventArgs e)
        {
            tiltMove(-1, '3', ref TM3Cnt);
        }

        private void textBox1_TextChanged(object sender, EventArgs e)
        {

        }


        private void button19_Click(object sender, EventArgs e)
        {
            f.Move(0);
        }

        private void button20_Click(object sender, EventArgs e)
        {
            foreach (FocuserHardware.CSavedPos sp in FocuserHardware.savedPos)
                if (sp.name == textBox9.Text) { sp.pos = f.Position - SharedResources.buteeBassePos; FocuserHardware.saveProfile(); updateSavedPosList(); return; }
            FocuserHardware.CSavedPos sp2 = new FocuserHardware.CSavedPos();
            sp2.name = textBox9.Text; sp2.pos= f.Position - SharedResources.buteeBassePos;
            FocuserHardware.savedPos.Add(sp2);
            FocuserHardware.saveProfile(); 
            updateSavedPosList();
        }
        private void updateSavedPosList()
        {
            listBox1.Items.Clear();
            foreach (FocuserHardware.CSavedPos sp in FocuserHardware.savedPos)
                listBox1.Items.Add(sp.name + " " + (sp.pos * f.StepSize / 1000.0f).ToString("N3"));
        }
        private void button21_Click(object sender, EventArgs e)
        {
            int i= listBox1.SelectedIndex;
            if (i == -1 || i > FocuserHardware.savedPos.Count) return;
            f.Move(SharedResources.buteeBassePos + FocuserHardware.savedPos[i].pos);
        }
        private void button17_Click(object sender, EventArgs e)
        {
            TM1Cnt = 0; TM2Cnt = 0; TM3Cnt = 0;
            updateTiltUI();
        }
        private void updateTiltUI()
        {
            TMl1.Text = "M1 " + TM1Cnt.ToString();
            TMl2.Text = "M2 " + TM2Cnt.ToString();
            TMl3.Text = "M3 " + TM3Cnt.ToString();
        }
    }
}
