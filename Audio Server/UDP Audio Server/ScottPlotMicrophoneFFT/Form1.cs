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

namespace ScottPlotMicrophoneFFT
{
    public partial class Form1 : Form
    {

        // MICROPHONE ANALYSIS SETTINGS
        private int RATE = 40960; // sample rate of the sound card, for 40 hz increments, use 40960
        private int BUFFERSIZE = (int)Math.Pow(2, 11); // must be a multiple of 2 for 40 hz increments, use 2^11
		private byte[] udpBuffer = null;
        private string multicastIP = "239.0.0.222";
        private int multicastPort = 2222;
        private int udpBroadcastRate = 30;
		private int frameCounter = 0;
		double[] fftReal;

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
            lblStatus.Text = $"UDP multicast server started on {multicastIP}, port {multicastPort}";
            System.Timers.Timer udpTimer = new System.Timers.Timer();
            udpTimer.Elapsed += new ElapsedEventHandler(sendUdpData);
            udpTimer.Interval = udpBroadcastRate;
            udpTimer.Start();
        }

        public void sendUdpData(object source, ElapsedEventArgs e)
		{
			UdpClient udpclient = new UdpClient();
			IPAddress multicastaddress = IPAddress.Parse(multicastIP);
            IPEndPoint remoteEndPoint = new IPEndPoint(multicastaddress, multicastPort);
            Random rnd = new Random();
            int num = rnd.Next();
			//audio packet
			AudioPacket audio = new AudioPacket();
			//BUFFERSIZE / BYTES_PER_POINT / 2;
			/*audio.low80hz = Math.Round(Enumerable.Range(0, fftReal.Length / 80).Select(i => fftReal[i]).Average(), 4) * 1;
			audio.mid = Math.Round(Enumerable.Range(fftReal.Length / 80, fftReal.Length / 4).Select(i => fftReal[i]).Average(), 4) * 2;
			audio.high = Math.Round(Enumerable.Range(fftReal.Length / 4, 3 * (fftReal.Length / 4)).Select(i => fftReal[i]).Average(), 4) * 4;
			string jsonAudio = JsonConvert.SerializeObject(audio, Formatting.None);
			udpBuffer = Encoding.UTF8.GetBytes(jsonAudio);*/
			audio.low40hz = Math.Round(fftReal[1] * 2 * 10, 4);
			audio.low80hz = Math.Round(fftReal[2] * 2 * 10, 4);
			audio.low120hz = Math.Round(fftReal[3] * 2 * 10, 4);
			audio.low160hz = Math.Round(fftReal[4] * 2 * 10, 4);
			audio.mid = Math.Round(Enumerable.Range(fftReal.Length / 2, fftReal.Length / 40).Select(i => fftReal[i]).Average(), 4) * 6 * 8;
			audio.high = Math.Round(Enumerable.Range(fftReal.Length / 2, fftReal.Length / 6).Select(i => fftReal[i]).Average(), 4) * 0 * 8;
			string jsonAudio = JsonConvert.SerializeObject(audio, Formatting.None);
			udpBuffer = Encoding.UTF8.GetBytes(jsonAudio);
			udpclient.Send(udpBuffer, udpBuffer.Length, remoteEndPoint);
        }

        public void SetupGraphLabels()
        {
            scottPlotUC1.fig.labelTitle = "Microphone PCM Data";
            scottPlotUC1.fig.labelY = "Amplitude (PCM)";
            scottPlotUC1.fig.labelX = "Time (ms)";
            scottPlotUC1.Redraw();

            scottPlotUC2.fig.labelTitle = "Microphone FFT Data";
            scottPlotUC2.fig.labelY = "Power (raw)";
            scottPlotUC2.fig.labelX = "Frequency (Hz)";
            scottPlotUC2.Redraw();
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
			lblStatus.Text = $"Frame counter: {numberOfDraws}";

			timerReplot.Enabled = true;
        }

        public int numberOfDraws = 0;
        public bool needsAutoScaling = true;
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

			// plot the Xs and Ys for both graphs
			if (numberOfDraws % 5 == 0) { 
				scottPlotUC1.Clear();
				scottPlotUC1.PlotSignal(pcm, pcmPointSpacingMs, Color.Blue);
				scottPlotUC2.Clear();
				scottPlotUC2.PlotSignal(fftReal, fftPointSpacingHz, Color.Blue);
			}

            // optionally adjust the scale to automatically fit the data
            if (needsAutoScaling)
            {
                scottPlotUC1.AxisAuto();
                scottPlotUC2.AxisAuto();
                //scottPlotUC1.AxisManual(0, 1, -1, 1);
                //scottPlotUC2.AxisManual(0, .35, 0, 1);
                needsAutoScaling = false;
            }

            //scottPlotUC1.PlotSignal(Ys, RATE);
			
            numberOfDraws += 1;

            // this reduces flicker and helps keep the program responsive
            Application.DoEvents(); 

        }

        private void autoScaleToolStripMenuItem_Click(object sender, EventArgs e)
        {
            needsAutoScaling = true;
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
    }
}
