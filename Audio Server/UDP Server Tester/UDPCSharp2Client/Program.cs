using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Text;

internal class Program
{
    private static void Main(string[] args)
    {
        UdpClient client = new UdpClient();

        client.ExclusiveAddressUse = false;
        IPEndPoint localEp = new IPEndPoint(IPAddress.Any, 2222);

        client.Client.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReuseAddress, true);
        client.ExclusiveAddressUse = false;

        client.Client.Bind(localEp);

        IPAddress multicastaddress = IPAddress.Parse("239.0.0.222");
        client.JoinMulticastGroup(multicastaddress);

        Console.WriteLine("Listening this will never quit so you will need to ctrl-c it");
		int count = 0;
        while (true)
        {
			count++;
            Byte[] data = client.Receive(ref localEp);
			string strData = Encoding.Unicode.GetString(data);
			Console.SetCursorPosition(0, 0);
			if (count % 30 == 0)
			{
				Console.Clear();
			}

			Console.WriteLine(strData);
		}
    }
}