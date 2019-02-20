UDP Audio Server
=========
Included here are two pieces to getting the audio server for this project up and running.

Audio Server
------
![](https://github.com/Tiuipuv/esp8266-fastled-dedicated-webserver/blob/master/Audio%20Server/UDP%20Audio%20Server/screenshot.png)

Firstly, the ScottPlot based C# Audio server is located in [UDP Audio Server](https://github.com/Tiuipuv/esp8266-fastled-dedicated-webserver/tree/master/Audio%20Server/UDP%20Audio%20Server).

This server is designed to take in audio data from an audio source, due a live FFT (Fast Fourier Transform) on the data, and JSON encode the data over the network.
Currently, it broadcasts JSON data on an arbitrary IP address, 239.0.0.222 port 2222. This could be changed in the program to whatever works best for your network.

Please place this on the server pc with the [Web Server](https://github.com/Tiuipuv/esp8266-fastled-dedicated-webserver/tree/master/Web%20Server) to allow for audio animations to function. (You could, of course, have an independent audio server pc, but that seems like an excessive cost)

Audio Tester
------
![](https://github.com/Tiuipuv/esp8266-fastled-dedicated-webserver/blob/master/Audio%20Server/UDP%20Server%20Tester/screenshot.png)

Secondly, the UDP test tool is located in [UDP Server Tester](https://github.com/Tiuipuv/esp8266-fastled-dedicated-webserver/tree/master/Audio%20Server/UDP%20Server%20Tester).

This tester is just a console based C# application that takes in the JSON data on 239.0.0.222 port 2222 and converts it to a string, and finally prints it out.
This is useful to determine if your networking is configured properly.
