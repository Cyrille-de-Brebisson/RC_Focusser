using System;

namespace ASCOM.LocalServer
{
    partial class FrmMain
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();  
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(FrmMain));
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.button1 = new System.Windows.Forms.Button();
            this.textBox1 = new System.Windows.Forms.TextBox();
            this.button2 = new System.Windows.Forms.Button();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.StepSize = new System.Windows.Forms.TextBox();
            this.button10 = new System.Windows.Forms.Button();
            this.label3 = new System.Windows.Forms.Label();
            this.button11 = new System.Windows.Forms.Button();
            this.textBox5 = new System.Windows.Forms.TextBox();
            this.label5 = new System.Windows.Forms.Label();
            this.button9 = new System.Windows.Forms.Button();
            this.button8 = new System.Windows.Forms.Button();
            this.textBox2 = new System.Windows.Forms.TextBox();
            this.label4 = new System.Windows.Forms.Label();
            this.comboBoxComPort = new System.Windows.Forms.ComboBox();
            this.labelCom = new System.Windows.Forms.Label();
            this.rescan = new System.Windows.Forms.Button();
            this.groupBox5 = new System.Windows.Forms.GroupBox();
            this.button3 = new System.Windows.Forms.Button();
            this.label7 = new System.Windows.Forms.Label();
            this.Filter = new System.Windows.Forms.Label();
            this.textBox4 = new System.Windows.Forms.TextBox();
            this.textBox3 = new System.Windows.Forms.TextBox();
            this.comboBox1 = new System.Windows.Forms.ComboBox();
            this.button15 = new System.Windows.Forms.Button();
            this.openFileDialog1 = new System.Windows.Forms.OpenFileDialog();
            this.saveFileDialog1 = new System.Windows.Forms.SaveFileDialog();
            this.groupBox7 = new System.Windows.Forms.GroupBox();
            this.button14 = new System.Windows.Forms.Button();
            this.logBox = new System.Windows.Forms.TextBox();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.button4 = new System.Windows.Forms.Button();
            this.textBox7 = new System.Windows.Forms.TextBox();
            this.textBox6 = new System.Windows.Forms.TextBox();
            this.label8 = new System.Windows.Forms.Label();
            this.label6 = new System.Windows.Forms.Label();
            this.groupBox3 = new System.Windows.Forms.GroupBox();
            this.button18 = new System.Windows.Forms.Button();
            this.textBox8 = new System.Windows.Forms.TextBox();
            this.label9 = new System.Windows.Forms.Label();
            this.button17 = new System.Windows.Forms.Button();
            this.TMl3 = new System.Windows.Forms.Label();
            this.button13 = new System.Windows.Forms.Button();
            this.button16 = new System.Windows.Forms.Button();
            this.TMl2 = new System.Windows.Forms.Label();
            this.button7 = new System.Windows.Forms.Button();
            this.button12 = new System.Windows.Forms.Button();
            this.TMl1 = new System.Windows.Forms.Label();
            this.button6 = new System.Windows.Forms.Button();
            this.button5 = new System.Windows.Forms.Button();
            this.button19 = new System.Windows.Forms.Button();
            this.button20 = new System.Windows.Forms.Button();
            this.textBox9 = new System.Windows.Forms.TextBox();
            this.listBox1 = new System.Windows.Forms.ListBox();
            this.button21 = new System.Windows.Forms.Button();
            this.label10 = new System.Windows.Forms.Label();
            this.groupBox2.SuspendLayout();
            this.groupBox5.SuspendLayout();
            this.groupBox7.SuspendLayout();
            this.groupBox1.SuspendLayout();
            this.groupBox3.SuspendLayout();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.Location = new System.Drawing.Point(24, 19);
            this.label1.Margin = new System.Windows.Forms.Padding(5, 0, 5, 0);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(452, 64);
            this.label1.TabIndex = 0;
            this.label1.Text = "Ascom driver for Brebisson RS232 focusser";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(27, 42);
            this.label2.Margin = new System.Windows.Forms.Padding(5, 0, 5, 0);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(89, 25);
            this.label2.TabIndex = 1;
            this.label2.Text = "Position";
            // 
            // button1
            // 
            this.button1.Location = new System.Drawing.Point(28, 116);
            this.button1.Margin = new System.Windows.Forms.Padding(5, 6, 5, 6);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(200, 44);
            this.button1.TabIndex = 3;
            this.button1.Text = "Goto";
            this.button1.UseVisualStyleBackColor = true;
            this.button1.Click += new System.EventHandler(this.button1_Click);
            // 
            // textBox1
            // 
            this.textBox1.Location = new System.Drawing.Point(28, 73);
            this.textBox1.Margin = new System.Windows.Forms.Padding(5, 6, 5, 6);
            this.textBox1.Name = "textBox1";
            this.textBox1.Size = new System.Drawing.Size(196, 31);
            this.textBox1.TabIndex = 4;
            this.textBox1.TextChanged += new System.EventHandler(this.textBox1_TextChanged);
            // 
            // button2
            // 
            this.button2.Location = new System.Drawing.Point(29, 106);
            this.button2.Margin = new System.Windows.Forms.Padding(5, 6, 5, 6);
            this.button2.Name = "button2";
            this.button2.Size = new System.Drawing.Size(196, 44);
            this.button2.TabIndex = 5;
            this.button2.Text = "Connect";
            this.button2.UseVisualStyleBackColor = true;
            this.button2.Click += new System.EventHandler(this.button2_Click);
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.label10);
            this.groupBox2.Controls.Add(this.button21);
            this.groupBox2.Controls.Add(this.listBox1);
            this.groupBox2.Controls.Add(this.textBox9);
            this.groupBox2.Controls.Add(this.button20);
            this.groupBox2.Controls.Add(this.button19);
            this.groupBox2.Controls.Add(this.StepSize);
            this.groupBox2.Controls.Add(this.button10);
            this.groupBox2.Controls.Add(this.label3);
            this.groupBox2.Controls.Add(this.button11);
            this.groupBox2.Controls.Add(this.textBox5);
            this.groupBox2.Controls.Add(this.label5);
            this.groupBox2.Controls.Add(this.button9);
            this.groupBox2.Controls.Add(this.button8);
            this.groupBox2.Controls.Add(this.textBox2);
            this.groupBox2.Controls.Add(this.label4);
            this.groupBox2.Controls.Add(this.textBox1);
            this.groupBox2.Controls.Add(this.button1);
            this.groupBox2.Controls.Add(this.label2);
            this.groupBox2.Location = new System.Drawing.Point(35, 174);
            this.groupBox2.Margin = new System.Windows.Forms.Padding(5, 6, 5, 6);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Padding = new System.Windows.Forms.Padding(5, 6, 5, 6);
            this.groupBox2.Size = new System.Drawing.Size(588, 407);
            this.groupBox2.TabIndex = 5;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Focuser";
            // 
            // StepSize
            // 
            this.StepSize.Location = new System.Drawing.Point(187, 364);
            this.StepSize.Margin = new System.Windows.Forms.Padding(5, 6, 5, 6);
            this.StepSize.Name = "StepSize";
            this.StepSize.Size = new System.Drawing.Size(240, 31);
            this.StepSize.TabIndex = 11;
            this.StepSize.TextChanged += new System.EventHandler(this.StepSize_TextChanged);
            // 
            // button10
            // 
            this.button10.Location = new System.Drawing.Point(493, 115);
            this.button10.Margin = new System.Windows.Forms.Padding(4);
            this.button10.Name = "button10";
            this.button10.Size = new System.Drawing.Size(63, 45);
            this.button10.TabIndex = 12;
            this.button10.Text = "out";
            this.button10.UseVisualStyleBackColor = true;
            this.button10.MouseDown += new System.Windows.Forms.MouseEventHandler(this.button10_MouseDown);
            this.button10.MouseLeave += new System.EventHandler(this.button10_MouseLeave);
            this.button10.MouseUp += new System.Windows.Forms.MouseEventHandler(this.button10_MouseUp);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(23, 364);
            this.label3.Margin = new System.Windows.Forms.Padding(5, 0, 5, 0);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(145, 25);
            this.label3.TabIndex = 10;
            this.label3.Text = "Step Size in µ";
            // 
            // button11
            // 
            this.button11.Location = new System.Drawing.Point(424, 115);
            this.button11.Margin = new System.Windows.Forms.Padding(4);
            this.button11.Name = "button11";
            this.button11.Size = new System.Drawing.Size(63, 45);
            this.button11.TabIndex = 11;
            this.button11.Text = "in";
            this.button11.UseVisualStyleBackColor = true;
            this.button11.MouseDown += new System.Windows.Forms.MouseEventHandler(this.button11_MouseDown);
            this.button11.MouseLeave += new System.EventHandler(this.button11_MouseLeave);
            this.button11.MouseUp += new System.Windows.Forms.MouseEventHandler(this.button11_MouseUp);
            // 
            // textBox5
            // 
            this.textBox5.Location = new System.Drawing.Point(423, 76);
            this.textBox5.Margin = new System.Windows.Forms.Padding(4);
            this.textBox5.Name = "textBox5";
            this.textBox5.Size = new System.Drawing.Size(132, 31);
            this.textBox5.TabIndex = 10;
            this.textBox5.TextChanged += new System.EventHandler(this.textBox5_TextChanged);
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(473, 43);
            this.label5.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(55, 25);
            this.label5.TabIndex = 9;
            this.label5.Text = "slow";
            // 
            // button9
            // 
            this.button9.Location = new System.Drawing.Point(340, 115);
            this.button9.Margin = new System.Windows.Forms.Padding(4);
            this.button9.Name = "button9";
            this.button9.Size = new System.Drawing.Size(63, 45);
            this.button9.TabIndex = 8;
            this.button9.Text = "out";
            this.button9.UseVisualStyleBackColor = true;
            this.button9.MouseDown += new System.Windows.Forms.MouseEventHandler(this.button9_MouseDown);
            this.button9.MouseLeave += new System.EventHandler(this.button9_MouseLeave);
            this.button9.MouseUp += new System.Windows.Forms.MouseEventHandler(this.button9_MouseUp);
            // 
            // button8
            // 
            this.button8.Location = new System.Drawing.Point(268, 115);
            this.button8.Margin = new System.Windows.Forms.Padding(4);
            this.button8.Name = "button8";
            this.button8.Size = new System.Drawing.Size(63, 45);
            this.button8.TabIndex = 7;
            this.button8.Text = "in";
            this.button8.UseVisualStyleBackColor = true;
            this.button8.MouseDown += new System.Windows.Forms.MouseEventHandler(this.button8_MouseDown);
            this.button8.MouseLeave += new System.EventHandler(this.button8_MouseLeave);
            this.button8.MouseUp += new System.Windows.Forms.MouseEventHandler(this.button8_MouseUp);
            // 
            // textBox2
            // 
            this.textBox2.Location = new System.Drawing.Point(267, 76);
            this.textBox2.Margin = new System.Windows.Forms.Padding(4);
            this.textBox2.Name = "textBox2";
            this.textBox2.Size = new System.Drawing.Size(132, 31);
            this.textBox2.TabIndex = 6;
            this.textBox2.TextChanged += new System.EventHandler(this.textBox2_TextChanged);
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(316, 42);
            this.label4.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(47, 25);
            this.label4.TabIndex = 5;
            this.label4.Text = "fast";
            // 
            // comboBoxComPort
            // 
            this.comboBoxComPort.FormattingEnabled = true;
            this.comboBoxComPort.Location = new System.Drawing.Point(356, 111);
            this.comboBoxComPort.Margin = new System.Windows.Forms.Padding(4);
            this.comboBoxComPort.Name = "comboBoxComPort";
            this.comboBoxComPort.Size = new System.Drawing.Size(160, 33);
            this.comboBoxComPort.TabIndex = 9;
            this.comboBoxComPort.SelectedIndexChanged += new System.EventHandler(this.comboBoxComPort_SelectedIndexChanged);
            // 
            // labelCom
            // 
            this.labelCom.AutoSize = true;
            this.labelCom.Location = new System.Drawing.Point(235, 115);
            this.labelCom.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.labelCom.Name = "labelCom";
            this.labelCom.Size = new System.Drawing.Size(70, 25);
            this.labelCom.TabIndex = 11;
            this.labelCom.Text = "label3";
            // 
            // rescan
            // 
            this.rescan.Location = new System.Drawing.Point(531, 106);
            this.rescan.Margin = new System.Windows.Forms.Padding(4);
            this.rescan.Name = "rescan";
            this.rescan.Size = new System.Drawing.Size(89, 42);
            this.rescan.TabIndex = 12;
            this.rescan.Text = "Scann";
            this.rescan.UseVisualStyleBackColor = true;
            this.rescan.Click += new System.EventHandler(this.rescan_Click);
            // 
            // groupBox5
            // 
            this.groupBox5.Controls.Add(this.button3);
            this.groupBox5.Controls.Add(this.label7);
            this.groupBox5.Controls.Add(this.Filter);
            this.groupBox5.Controls.Add(this.textBox4);
            this.groupBox5.Controls.Add(this.textBox3);
            this.groupBox5.Controls.Add(this.comboBox1);
            this.groupBox5.Location = new System.Drawing.Point(29, 591);
            this.groupBox5.Margin = new System.Windows.Forms.Padding(4);
            this.groupBox5.Name = "groupBox5";
            this.groupBox5.Padding = new System.Windows.Forms.Padding(4);
            this.groupBox5.Size = new System.Drawing.Size(585, 190);
            this.groupBox5.TabIndex = 37;
            this.groupBox5.TabStop = false;
            this.groupBox5.Text = "Filters";
            // 
            // button3
            // 
            this.button3.Location = new System.Drawing.Point(21, 116);
            this.button3.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.button3.Name = "button3";
            this.button3.Size = new System.Drawing.Size(224, 51);
            this.button3.TabIndex = 5;
            this.button3.Text = "Save names/Offsets";
            this.button3.UseVisualStyleBackColor = true;
            this.button3.Click += new System.EventHandler(this.button3_Click);
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(61, 89);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(197, 25);
            this.label7.TabIndex = 4;
            this.label7.Text = "Names an Offets ->";
            // 
            // Filter
            // 
            this.Filter.AutoSize = true;
            this.Filter.Location = new System.Drawing.Point(17, 34);
            this.Filter.Name = "Filter";
            this.Filter.Size = new System.Drawing.Size(60, 25);
            this.Filter.TabIndex = 3;
            this.Filter.Text = "Filter";
            // 
            // textBox4
            // 
            this.textBox4.Location = new System.Drawing.Point(423, 31);
            this.textBox4.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.textBox4.Multiline = true;
            this.textBox4.Name = "textBox4";
            this.textBox4.Size = new System.Drawing.Size(137, 136);
            this.textBox4.TabIndex = 2;
            // 
            // textBox3
            // 
            this.textBox3.Location = new System.Drawing.Point(264, 31);
            this.textBox3.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.textBox3.Multiline = true;
            this.textBox3.Name = "textBox3";
            this.textBox3.Size = new System.Drawing.Size(141, 136);
            this.textBox3.TabIndex = 1;
            this.textBox3.TextChanged += new System.EventHandler(this.textBox3_TextChanged);
            // 
            // comboBox1
            // 
            this.comboBox1.FormattingEnabled = true;
            this.comboBox1.Location = new System.Drawing.Point(93, 34);
            this.comboBox1.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.comboBox1.Name = "comboBox1";
            this.comboBox1.Size = new System.Drawing.Size(153, 33);
            this.comboBox1.TabIndex = 0;
            this.comboBox1.SelectedIndexChanged += new System.EventHandler(this.comboBox1_SelectedIndexChanged);
            // 
            // button15
            // 
            this.button15.Location = new System.Drawing.Point(533, 22);
            this.button15.Margin = new System.Windows.Forms.Padding(4);
            this.button15.Name = "button15";
            this.button15.Size = new System.Drawing.Size(89, 60);
            this.button15.TabIndex = 41;
            this.button15.Text = "Log";
            this.button15.UseVisualStyleBackColor = true;
            this.button15.Click += new System.EventHandler(this.button15_Click);
            // 
            // openFileDialog1
            // 
            this.openFileDialog1.FileName = "openFileDialog1";
            // 
            // groupBox7
            // 
            this.groupBox7.BackColor = System.Drawing.SystemColors.Control;
            this.groupBox7.Controls.Add(this.button14);
            this.groupBox7.Controls.Add(this.logBox);
            this.groupBox7.Location = new System.Drawing.Point(669, 22);
            this.groupBox7.Margin = new System.Windows.Forms.Padding(4);
            this.groupBox7.Name = "groupBox7";
            this.groupBox7.Padding = new System.Windows.Forms.Padding(4);
            this.groupBox7.Size = new System.Drawing.Size(765, 683);
            this.groupBox7.TabIndex = 40;
            this.groupBox7.TabStop = false;
            this.groupBox7.Text = "Log";
            // 
            // button14
            // 
            this.button14.Location = new System.Drawing.Point(611, 11);
            this.button14.Margin = new System.Windows.Forms.Padding(4);
            this.button14.Name = "button14";
            this.button14.Size = new System.Drawing.Size(135, 39);
            this.button14.TabIndex = 2;
            this.button14.Text = "Clear";
            this.button14.UseVisualStyleBackColor = true;
            this.button14.Click += new System.EventHandler(this.button14_Click);
            // 
            // logBox
            // 
            this.logBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.logBox.Location = new System.Drawing.Point(21, 58);
            this.logBox.Margin = new System.Windows.Forms.Padding(4);
            this.logBox.Multiline = true;
            this.logBox.Name = "logBox";
            this.logBox.Size = new System.Drawing.Size(736, 596);
            this.logBox.TabIndex = 0;
            this.logBox.WordWrap = false;
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.button4);
            this.groupBox1.Controls.Add(this.textBox7);
            this.groupBox1.Controls.Add(this.textBox6);
            this.groupBox1.Controls.Add(this.label8);
            this.groupBox1.Controls.Add(this.label6);
            this.groupBox1.Location = new System.Drawing.Point(33, 804);
            this.groupBox1.Margin = new System.Windows.Forms.Padding(4);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Padding = new System.Windows.Forms.Padding(4);
            this.groupBox1.Size = new System.Drawing.Size(587, 110);
            this.groupBox1.TabIndex = 42;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "wifi (Alpaca)";
            // 
            // button4
            // 
            this.button4.Location = new System.Drawing.Point(489, 28);
            this.button4.Margin = new System.Windows.Forms.Padding(4);
            this.button4.Name = "button4";
            this.button4.Size = new System.Drawing.Size(87, 41);
            this.button4.TabIndex = 4;
            this.button4.Text = "Save";
            this.button4.UseVisualStyleBackColor = true;
            this.button4.Click += new System.EventHandler(this.button4_Click);
            // 
            // textBox7
            // 
            this.textBox7.Location = new System.Drawing.Point(240, 62);
            this.textBox7.Margin = new System.Windows.Forms.Padding(4);
            this.textBox7.Name = "textBox7";
            this.textBox7.Size = new System.Drawing.Size(240, 31);
            this.textBox7.TabIndex = 3;
            // 
            // textBox6
            // 
            this.textBox6.Location = new System.Drawing.Point(25, 62);
            this.textBox6.Margin = new System.Windows.Forms.Padding(4);
            this.textBox6.Name = "textBox6";
            this.textBox6.Size = new System.Drawing.Size(185, 31);
            this.textBox6.TabIndex = 2;
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.Location = new System.Drawing.Point(241, 28);
            this.label8.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(106, 25);
            this.label8.TabIndex = 1;
            this.label8.Text = "Password";
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(25, 28);
            this.label6.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(60, 25);
            this.label6.TabIndex = 0;
            this.label6.Text = "SSID";
            // 
            // groupBox3
            // 
            this.groupBox3.Controls.Add(this.button18);
            this.groupBox3.Controls.Add(this.textBox8);
            this.groupBox3.Controls.Add(this.label9);
            this.groupBox3.Controls.Add(this.button17);
            this.groupBox3.Controls.Add(this.TMl3);
            this.groupBox3.Controls.Add(this.button13);
            this.groupBox3.Controls.Add(this.button16);
            this.groupBox3.Controls.Add(this.TMl2);
            this.groupBox3.Controls.Add(this.button7);
            this.groupBox3.Controls.Add(this.button12);
            this.groupBox3.Controls.Add(this.TMl1);
            this.groupBox3.Controls.Add(this.button6);
            this.groupBox3.Controls.Add(this.button5);
            this.groupBox3.Location = new System.Drawing.Point(669, 725);
            this.groupBox3.Name = "groupBox3";
            this.groupBox3.Size = new System.Drawing.Size(742, 189);
            this.groupBox3.TabIndex = 43;
            this.groupBox3.TabStop = false;
            this.groupBox3.Text = "Tilt";
            // 
            // button18
            // 
            this.button18.Location = new System.Drawing.Point(564, 88);
            this.button18.Name = "button18";
            this.button18.Size = new System.Drawing.Size(131, 71);
            this.button18.TabIndex = 12;
            this.button18.Text = "ALL ON";
            this.button18.UseVisualStyleBackColor = true;
            this.button18.Click += new System.EventHandler(this.button18_Click);
            // 
            // textBox8
            // 
            this.textBox8.Location = new System.Drawing.Point(490, 38);
            this.textBox8.Name = "textBox8";
            this.textBox8.Size = new System.Drawing.Size(205, 31);
            this.textBox8.TabIndex = 11;
            this.textBox8.Text = "50";
            // 
            // label9
            // 
            this.label9.AutoSize = true;
            this.label9.Location = new System.Drawing.Point(370, 41);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(114, 25);
            this.label9.TabIndex = 10;
            this.label9.Text = "Step in um";
            // 
            // button17
            // 
            this.button17.Location = new System.Drawing.Point(375, 97);
            this.button17.Name = "button17";
            this.button17.Size = new System.Drawing.Size(169, 59);
            this.button17.TabIndex = 9;
            this.button17.Text = "Reset Counts";
            this.button17.UseVisualStyleBackColor = true;
            this.button17.Click += new System.EventHandler(this.button17_Click);
            // 
            // TMl3
            // 
            this.TMl3.AutoSize = true;
            this.TMl3.Location = new System.Drawing.Point(270, 27);
            this.TMl3.Name = "TMl3";
            this.TMl3.Size = new System.Drawing.Size(42, 25);
            this.TMl3.TabIndex = 8;
            this.TMl3.Text = "M1";
            // 
            // button13
            // 
            this.button13.Location = new System.Drawing.Point(256, 116);
            this.button13.Name = "button13";
            this.button13.Size = new System.Drawing.Size(75, 43);
            this.button13.TabIndex = 7;
            this.button13.Text = "M3-";
            this.button13.UseVisualStyleBackColor = true;
            this.button13.Click += new System.EventHandler(this.button13_Click);
            // 
            // button16
            // 
            this.button16.Location = new System.Drawing.Point(256, 58);
            this.button16.Name = "button16";
            this.button16.Size = new System.Drawing.Size(75, 43);
            this.button16.TabIndex = 6;
            this.button16.Text = "M3+";
            this.button16.UseVisualStyleBackColor = true;
            this.button16.Click += new System.EventHandler(this.button16_Click);
            // 
            // TMl2
            // 
            this.TMl2.AutoSize = true;
            this.TMl2.Location = new System.Drawing.Point(163, 27);
            this.TMl2.Name = "TMl2";
            this.TMl2.Size = new System.Drawing.Size(42, 25);
            this.TMl2.TabIndex = 5;
            this.TMl2.Text = "M2";
            // 
            // button7
            // 
            this.button7.Location = new System.Drawing.Point(149, 116);
            this.button7.Name = "button7";
            this.button7.Size = new System.Drawing.Size(75, 43);
            this.button7.TabIndex = 4;
            this.button7.Text = "M2-";
            this.button7.UseVisualStyleBackColor = true;
            this.button7.Click += new System.EventHandler(this.button7_Click);
            // 
            // button12
            // 
            this.button12.Location = new System.Drawing.Point(149, 58);
            this.button12.Name = "button12";
            this.button12.Size = new System.Drawing.Size(75, 43);
            this.button12.TabIndex = 3;
            this.button12.Text = "M2+";
            this.button12.UseVisualStyleBackColor = true;
            this.button12.Click += new System.EventHandler(this.button12_Click);
            // 
            // TMl1
            // 
            this.TMl1.AutoSize = true;
            this.TMl1.Location = new System.Drawing.Point(52, 27);
            this.TMl1.Name = "TMl1";
            this.TMl1.Size = new System.Drawing.Size(42, 25);
            this.TMl1.TabIndex = 2;
            this.TMl1.Text = "M1";
            // 
            // button6
            // 
            this.button6.Location = new System.Drawing.Point(38, 116);
            this.button6.Name = "button6";
            this.button6.Size = new System.Drawing.Size(75, 43);
            this.button6.TabIndex = 1;
            this.button6.Text = "M1-";
            this.button6.UseVisualStyleBackColor = true;
            this.button6.Click += new System.EventHandler(this.button6_Click);
            // 
            // button5
            // 
            this.button5.Location = new System.Drawing.Point(38, 58);
            this.button5.Name = "button5";
            this.button5.Size = new System.Drawing.Size(75, 43);
            this.button5.TabIndex = 0;
            this.button5.Text = "M1+";
            this.button5.UseVisualStyleBackColor = true;
            this.button5.Click += new System.EventHandler(this.button5_Click);
            // 
            // button19
            // 
            this.button19.Location = new System.Drawing.Point(424, 220);
            this.button19.Name = "button19";
            this.button19.Size = new System.Drawing.Size(141, 74);
            this.button19.TabIndex = 13;
            this.button19.Text = "Find Home";
            this.button19.UseVisualStyleBackColor = true;
            this.button19.Click += new System.EventHandler(this.button19_Click);
            // 
            // button20
            // 
            this.button20.Enabled = false;
            this.button20.Location = new System.Drawing.Point(32, 316);
            this.button20.Name = "button20";
            this.button20.Size = new System.Drawing.Size(119, 45);
            this.button20.TabIndex = 14;
            this.button20.Text = "SavePos";
            this.button20.UseVisualStyleBackColor = true;
            this.button20.Click += new System.EventHandler(this.button20_Click);
            // 
            // textBox9
            // 
            this.textBox9.Enabled = false;
            this.textBox9.Location = new System.Drawing.Point(157, 323);
            this.textBox9.Name = "textBox9";
            this.textBox9.Size = new System.Drawing.Size(150, 31);
            this.textBox9.TabIndex = 15;
            // 
            // listBox1
            // 
            this.listBox1.FormattingEnabled = true;
            this.listBox1.ItemHeight = 25;
            this.listBox1.Location = new System.Drawing.Point(32, 208);
            this.listBox1.Name = "listBox1";
            this.listBox1.Size = new System.Drawing.Size(275, 104);
            this.listBox1.TabIndex = 16;
            // 
            // button21
            // 
            this.button21.Enabled = false;
            this.button21.Location = new System.Drawing.Point(313, 232);
            this.button21.Name = "button21";
            this.button21.Size = new System.Drawing.Size(104, 51);
            this.button21.TabIndex = 17;
            this.button21.Text = "GotoPos";
            this.button21.UseVisualStyleBackColor = true;
            this.button21.Click += new System.EventHandler(this.button21_Click);
            // 
            // label10
            // 
            this.label10.AutoSize = true;
            this.label10.Location = new System.Drawing.Point(27, 180);
            this.label10.Name = "label10";
            this.label10.Size = new System.Drawing.Size(114, 25);
            this.label10.TabIndex = 18;
            this.label10.Text = "Saved pos";
            // 
            // FrmMain
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(12F, 25F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.SystemColors.Control;
            this.ClientSize = new System.Drawing.Size(1476, 944);
            this.Controls.Add(this.groupBox3);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.groupBox5);
            this.Controls.Add(this.button15);
            this.Controls.Add(this.rescan);
            this.Controls.Add(this.labelCom);
            this.Controls.Add(this.comboBoxComPort);
            this.Controls.Add(this.groupBox2);
            this.Controls.Add(this.button2);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.groupBox7);
            this.ForeColor = System.Drawing.SystemColors.WindowText;
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Margin = new System.Windows.Forms.Padding(5, 6, 5, 6);
            this.MaximizeBox = false;
            this.Name = "FrmMain";
            this.Text = "ASCOM.BrebissonV1.Focuser Driver Server";
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            this.groupBox5.ResumeLayout(false);
            this.groupBox5.PerformLayout();
            this.groupBox7.ResumeLayout(false);
            this.groupBox7.PerformLayout();
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.groupBox3.ResumeLayout(false);
            this.groupBox3.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Button button1;
        private System.Windows.Forms.TextBox textBox1;
        private System.Windows.Forms.Button button2;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.ComboBox comboBoxComPort;
        private System.Windows.Forms.Label labelCom;
        private System.Windows.Forms.Button rescan;
        private System.Windows.Forms.GroupBox groupBox5;
        private System.Windows.Forms.TextBox StepSize;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Button button10;
        private System.Windows.Forms.Button button11;
        private System.Windows.Forms.TextBox textBox5;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.Button button9;
        private System.Windows.Forms.Button button8;
        private System.Windows.Forms.TextBox textBox2;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Button button15;
        private System.Windows.Forms.OpenFileDialog openFileDialog1;
        private System.Windows.Forms.SaveFileDialog saveFileDialog1;
        private System.Windows.Forms.GroupBox groupBox7;
        private System.Windows.Forms.Button button14;
        private System.Windows.Forms.TextBox logBox;
        private System.Windows.Forms.TextBox textBox4;
        private System.Windows.Forms.TextBox textBox3;
        private System.Windows.Forms.ComboBox comboBox1;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.Label Filter;
        private System.Windows.Forms.Button button3;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.TextBox textBox7;
        private System.Windows.Forms.TextBox textBox6;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.Button button4;
        private System.Windows.Forms.GroupBox groupBox3;
        private System.Windows.Forms.Label TMl1;
        private System.Windows.Forms.Button button6;
        private System.Windows.Forms.Button button5;
        private System.Windows.Forms.Button button18;
        private System.Windows.Forms.TextBox textBox8;
        private System.Windows.Forms.Label label9;
        private System.Windows.Forms.Button button17;
        private System.Windows.Forms.Label TMl3;
        private System.Windows.Forms.Button button13;
        private System.Windows.Forms.Button button16;
        private System.Windows.Forms.Label TMl2;
        private System.Windows.Forms.Button button7;
        private System.Windows.Forms.Button button12;
        private System.Windows.Forms.Button button19;
        private System.Windows.Forms.Button button21;
        private System.Windows.Forms.ListBox listBox1;
        private System.Windows.Forms.TextBox textBox9;
        private System.Windows.Forms.Button button20;
        private System.Windows.Forms.Label label10;
    }
}

