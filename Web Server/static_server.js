const express = require('express');
// you can pass the parameter in the command line. e.g. node static_server.js 3000
const httpPort = process.argv[2] || 80;
const app = express();

const options = { index: ['index.html', 'index.htm', 'default.html', 'default.htm'] };
app.use(express.static('data', options));
// eslint-disable-next-line no-console
app.listen(httpPort, () => console.log('\nServer started.'));
console.log("Server is starting...")
console.log('\nNetwork Interfaces:')

var os = require('os');
var ifaces = os.networkInterfaces();

Object.keys(ifaces).forEach(function (ifname) {
  var alias = 0;

  ifaces[ifname].forEach(function (iface) {
    if ('IPv4' !== iface.family || iface.internal !== false) {
      // skip over internal (i.e. 127.0.0.1) and non-ipv4 addresses
      return;
    }

    if (alias >= 1) {
      // this single interface has multiple ipv4 addresses
      console.log(`\t${ifname}:${alias} - ${iface.address}:${httpPort}`);
    } else {
      // this interface has only one ipv4 adress
      console.log(`\t${ifname} - ${iface.address}:${httpPort}`);
    }
    ++alias;
  });
});
