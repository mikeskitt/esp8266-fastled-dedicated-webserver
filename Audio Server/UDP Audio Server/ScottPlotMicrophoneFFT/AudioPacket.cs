using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ScottPlotMicrophoneFFT
{
	class AudioPacket
	{
		public double low40hz;
		public double low80hz;
		public double low120hz;
		public double low160hz;
		public double mid;
		public double high;

        public AudioPacket()
        {
            silence();
        }
        public void silence()
        {
            low40hz = 0.0;
            low80hz = 0.0;
            low120hz = 0.0;
            low160hz = 0.0;
            mid = 0.0;
            high = 0.0;
        }
    }
}
