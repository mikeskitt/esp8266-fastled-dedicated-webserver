using System;
using System.Drawing;
using System.Windows.Forms;
using System.Net;
using System.Net.Sockets;
using System.Linq;
using NAudio.Wave;
using System.Timers;
using System.Text;
using Newtonsoft.Json;
using ScottPlotMicrophoneFFT;

namespace ScottPlotMicrophoneFFT
{
    public partial class Form1 : Form
    {

        // MICROPHONE ANALYSIS SETTINGS
        private int RATE = 40960; // sample rate of the sound card, for 40 hz increments, use 40960
        private int BUFFERSIZE = (int)Math.Pow(2, 11); // must be a multiple of 2 for 40 hz increments, use 2^11
		private byte[] udpBuffer = null;
        private static string multicastIP = "239.0.0.222";//2";
        private static int multicastPort = 2222;
        private static int udpBroadcastRate = 30;
        private static int udpStandbyThreshold = 200;
		private int frameCounter = 0;
        private int packetCounter = 0;
        private int silenceCounter = 0;
        private double MAX_GRAPH1 = 0;
        private double MAX_GRAPH2 = 0;
		double[] fftReal;

        UdpClient udpclient = new UdpClient();
        private static IPAddress multicastaddress = IPAddress.Parse(multicastIP);
        private static IPEndPoint remoteEndPoint = new IPEndPoint(multicastaddress, multicastPort);

        public static void DisplayTimeEvent(object source, ElapsedEventArgs e)
        {
            // code here will run every second
        }

        // prepare class objects
        public BufferedWaveProvider bwp;

        public Form1()
        {
            InitializeComponent();
            SetupUDPServer();
            SetupGraphLabels();
            StartListeningToMicrophone(0);
            timerReplot.Enabled = true;
        }

        void AudioDataAvailable(object sender, WaveInEventArgs e)
        {
            bwp.AddSamples(e.Buffer, 0, e.BytesRecorded);
        }

        private void Form1_Load(object sender, EventArgs e)
        {
        }

        public void SetupUDPServer()
        {
            System.Timers.Timer udpTimer = new System.Timers.Timer();
            udpTimer.Elapsed += new ElapsedEventHandler(sendUdpData);
            udpTimer.Interval = udpBroadcastRate;
            udpTimer.Start();
        }

        public void sendUdpData(object source, ElapsedEventArgs e)
		{
            if (fftReal != null)
            {
                //audio packet
                if (fftReal.Max() > 0.15)
                {
                    silenceCounter = 0;

                    AudioPacket audioPacket = new AudioPacket();
                    audioPacket.low40hz = Math.Round(fftReal[1] * 2 * 10, 4);
                    audioPacket.low80hz = Math.Round(fftReal[2] * 2 * 10, 4);
                    audioPacket.low120hz = Math.Round(fftReal[3] * 2 * 10, 4);
                    audioPacket.low160hz = Math.Round(fftReal[4] * 2 * 10, 4);
                    audioPacket.mid = Math.Round(Enumerable.Range(fftReal.Length / 2, fftReal.Length / 40).Select(i => fftReal[i]).Average(), 4) * 6 * 8;
                    audioPacket.high = Math.Round(Enumerable.Range(fftReal.Length / 2, fftReal.Length / 6).Select(i => fftReal[i]).Average(), 4) * 0 * 8;

                    string jsonAudio = JsonConvert.SerializeObject(audioPacket, Formatting.None);
                    udpBuffer = Encoding.UTF8.GetBytes(jsonAudio);
                    udpclient.Send(udpBuffer, udpBuffer.Length, remoteEndPoint);
                    packetCounter++;
                }
                else
                {
                    if (silenceCounter < udpStandbyThreshold)
                    {
                        AudioPacket audioPacket = new AudioPacket();

                        string jsonAudio = JsonConvert.SerializeObject(audioPacket, Formatting.None);
                        udpBuffer = Encoding.UTF8.GetBytes(jsonAudio);
                        udpclient.Send(udpBuffer, udpBuffer.Length, remoteEndPoint);
                        packetCounter++;
                        silenceCounter++;
                    }
                }
            }
        }

        public void SetupGraphLabels()
        {
            Color figureBgColor = ColorTranslator.FromHtml("#001021");
            Color dataBgColor = ColorTranslator.FromHtml("#021d38");
            scottPlotUC1.plt.Style(figBg: figureBgColor, dataBg: dataBgColor);
            scottPlotUC1.plt.Grid(color: ColorTranslator.FromHtml("#273c51"));
            scottPlotUC1.plt.Ticks(color: Color.LightGray);
            scottPlotUC1.plt.Title("Microphone PCM Data", Color.White);
            scottPlotUC1.plt.XLabel("Time (ms)", Color.LightGray);
            scottPlotUC1.plt.YLabel("Amplitude (PCM)", Color.LightGray);
            //scottPlotUC1.plt.Title("Microphone PCM Data");
            //scottPlotUC1.plt.YLabel("Amplitude (PCM)");
           // scottPlotUC1.plt.XLabel("Time (ms)");
            scottPlotUC1.Render();
            scottPlotUC2.plt.Style(figBg: figureBgColor, dataBg: dataBgColor);
            scottPlotUC2.plt.Grid(color: ColorTranslator.FromHtml("#273c51"));
            scottPlotUC2.plt.Ticks(color: Color.LightGray);

            scottPlotUC2.plt.Title("Microphone FFT Data", Color.White);
            scottPlotUC2.plt.YLabel("Power (raw)", Color.LightGray);
            scottPlotUC2.plt.XLabel("Frequency (Hz)", Color.LightGray);
            scottPlotUC2.Render();
        }

        public void StartListeningToMicrophone(int audioDeviceNumber = 0)
        {
            WaveIn wi = new WaveIn();
            wi.DeviceNumber = audioDeviceNumber;
            wi.WaveFormat = new NAudio.Wave.WaveFormat(RATE, 1);
            wi.BufferMilliseconds = (int)((double)BUFFERSIZE / (double)RATE * 1000.0);
            wi.DataAvailable += new EventHandler<WaveInEventArgs>(AudioDataAvailable);
            bwp = new BufferedWaveProvider(wi.WaveFormat);
            bwp.BufferLength = BUFFERSIZE * 2;
            bwp.DiscardOnBufferOverflow = true;
            try
            {
                wi.StartRecording();
            }
            catch
            {
                string msg = "Could not record from audio device!\n\n";
                msg += "Is your microphone plugged in?\n";
                msg += "Is it set as your default recording device?";
                MessageBox.Show(msg, "ERROR");
            }
        }

        private void Timer_Tick(object sender, EventArgs e)
        {
            // turn off the timer, take as long as we need to plot, then turn the timer back on
            timerReplot.Enabled = false;
            PlotLatestData();
			++frameCounter;
			lblAudioFrames.Text = $"Audio Frames: {numberOfDraws}";
            lblUdpPackets.Text = $"Packets Sent: {packetCounter}";
            if (silenceCounter >= udpStandbyThreshold)
            {
                lblStatus.Text = "Standby";
            }
            else if (silenceCounter > 10)
            {
                lblStatus.Text = $"Entering Standby: {Math.Ceiling((double)((udpStandbyThreshold - silenceCounter) / udpBroadcastRate))}";
            }
            else
            {
                lblStatus.Text = "Broadcasting Music Data";
            }
            timerReplot.Enabled = true;
        }

        public int numberOfDraws = 0;
        public bool needsAutoScalingGraph1 = true;
        public bool needsAutoScalingGraph2 = true;
        public void PlotLatestData()
		{
			// check the incoming microphone audio
			int frameSize = BUFFERSIZE;
			var audioBytes = new byte[frameSize];
			bwp.Read(audioBytes, 0, frameSize);

			// return if there's nothing new to plot
			if (audioBytes.Length == 0)
				return;
			if (audioBytes[frameSize - 2] == 0)
				return;

			// incoming data is 16-bit (2 bytes per audio point)
			int BYTES_PER_POINT = 2;

			// create a (32-bit) int array ready to fill with the 16-bit data
			int graphPointCount = audioBytes.Length / BYTES_PER_POINT;

			// create double arrays to hold the data we will graph
			double[] pcm = new double[graphPointCount];
			double[] fft = new double[graphPointCount];
            double[] fftPlot = new double[graphPointCount / 2];
            fftReal = new double[graphPointCount / 2];

			// populate Xs and Ys with double data
			for (int i = 0; i < graphPointCount; i++)
			{
				// read the int16 from the two bytes
				Int16 val = BitConverter.ToInt16(audioBytes, i * 2);

				// store the value in Ys as a percent (+/- 100% = 200%)
				pcm[i] = (double)(val) / Math.Pow(2, 16) * 200.0;
			}

			// calculate the full FFT
			fft = FFT(pcm);

			// determine horizontal axis units for graphs
			double pcmPointSpacingMs = RATE / 1000;
			double fftMaxFreq = RATE / 2;
			double fftPointSpacingHz = fftMaxFreq / graphPointCount;

			// just keep the real half (the other half imaginary)
			Array.Copy(fft, fftReal, fftReal.Length);
            Array.Copy(fft, fftPlot, fftPlot.Length);
            for (int i = 0; i < fftPlot.Length; i++)
            {
                fftPlot[i] = fftReal[i] * (i + 50) / 4;
            }
            if (pcm.Max() > MAX_GRAPH1)
            {
                needsAutoScalingGraph1 = true;
                MAX_GRAPH1 = pcm.Max();
            }
            if (fftPlot.Max() > MAX_GRAPH2)
            {
                needsAutoScalingGraph2 = true;
                MAX_GRAPH2 = fftPlot.Max();
            }
            // plot the Xs and Ys for both graphs
            if (numberOfDraws % 3 == 0)
            {

                scottPlotUC1.plt.Clear();
				scottPlotUC1.plt.PlotSignal(pcm, pcmPointSpacingMs, color: System.Drawing.Color.Red, markerSize: 1);
				scottPlotUC2.plt.Clear();
				scottPlotUC2.plt.PlotSignal(fftPlot, fftPointSpacingHz, color: System.Drawing.Color.DeepSkyBlue, markerSize: 1);
                scottPlotUC1.Render();
                scottPlotUC2.Render();
			}

            // optionally adjust the scale to automatically fit the data
            if (needsAutoScalingGraph1)
            {
                scottPlotUC1.plt.Clear();
                scottPlotUC1.plt.PlotSignal(pcm, pcmPointSpacingMs, color: System.Drawing.Color.Red, markerSize: 1);
                scottPlotUC1.Render();
                scottPlotUC1.plt.AxisAuto(0, 0);
                needsAutoScalingGraph1 = false;
            }

            // optionally adjust the scale to automatically fit the data
            if (needsAutoScalingGraph2)
            {
                scottPlotUC2.plt.Clear();
                scottPlotUC2.plt.PlotSignal(fftPlot, fftPointSpacingHz, color: System.Drawing.Color.DeepSkyBlue, markerSize: 1);
                scottPlotUC2.Render();
                scottPlotUC2.plt.AxisAuto(0.01, 0.1);
                //scottPlotUC2.plt.Axis(0, 24, -.05 * (MAX_GRAPH2 / 2), (MAX_GRAPH2 / 2));
                needsAutoScalingGraph2 = false;
            }

            //scottPlotUC1.PlotSignal(Ys, RATE);

            numberOfDraws += 1;

            // this reduces flicker and helps keep the program responsive
            Application.DoEvents(); 

        }

        private void autoScaleToolStripMenuItem_Click(object sender, EventArgs e)
        {
            needsAutoScalingGraph1 = true;
            needsAutoScalingGraph2 = true;
            MAX_GRAPH1 = 0;
            MAX_GRAPH2 = 0;
        }

        private void infoMessageToolStripMenuItem_Click(object sender, EventArgs e)
        {
            string msg = "";
            msg += "left-click-drag to pan\n";
            msg += "right-click-drag to zoom\n";
            msg += "middle-click to auto-axis\n";
            msg += "double-click for graphing stats\n";
            MessageBox.Show(msg);
        }

        private void websiteToolStripMenuItem_Click(object sender, EventArgs e)
        {
            System.Diagnostics.Process.Start("https://github.com/swharden/Csharp-Data-Visualization");
        }

        public double[] FFT(double[] data)
        {
            double[] fft = new double[data.Length];
            System.Numerics.Complex[] fftComplex = new System.Numerics.Complex[data.Length];
            for (int i = 0; i < data.Length; i++)
                fftComplex[i] = new System.Numerics.Complex(data[i], 0.0);
            Accord.Math.FourierTransform.FFT(fftComplex, Accord.Math.FourierTransform.Direction.Forward);
            for (int i = 0; i < data.Length; i++)
                fft[i] = fftComplex[i].Magnitude;
            return fft;
        }

        private void toolStripStatusLabel1_Click(object sender, EventArgs e)
        {

        }

        private void ScottPlotUC1_Load(object sender, EventArgs e)
        {

        }

        private void ScottPlotUC2_Load(object sender, EventArgs e)
        {

        }

        private void ScottPlotUC1_Load_1(object sender, EventArgs e)
        {

        }

        private void ToolStripStatusLabel1_Click_1(object sender, EventArgs e)
        {

        }
    }
}
