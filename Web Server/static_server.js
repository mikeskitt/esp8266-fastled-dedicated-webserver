
const webSocket = require('ws');
const express = require('express');

// you can pass the parameter in the command line. e.g. node static_server.js 3000
const httpPort = process.argv[2] || 80;
//const webSocketPort = process.argv[3] || 81;

const app = express();

var options = {
  index: ["index.html", "index.htm", "default.html", "default.htm"]
};

app.use(express.static('data', options));
app.listen(httpPort, () => console.log(`app listening on port ${httpPort}`));

/*const webSocketServer = new webSocket.Server({ port: webSocketPort });

webSocketServer.on('connection', function connection(ws) {
  console.log('Connected to web socket')
  ws.on('message', function incoming(message) {
    console.log('received: %s', message);
  });
});

exports.broadcastInt = function(name, value) {
  let json = "{\"name\":\"" + name + "\",\"value\":" + String(value) + "}";
  webSocketServer.broadcast(json);
}

exports.broadcastString = function(name, value) {
  let json = "{\"name\":\"" + name + "\",\"value\":\"" + String(value) + "\"}";
  webSocketServer.broadcast(json);
}

webSocketServer.broadcast = function broadcast(data) {
  console.log(`broadcasting ${data} on web socket`);
  webSocketServer.clients.forEach(function each(client) {
    if (client.readyState === webSocket.OPEN) {
      client.send(data);
    }
  });
};*/