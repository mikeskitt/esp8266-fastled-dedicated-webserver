const express = require('express');
// you can pass the parameter in the command line. e.g. node static_server.js 3000
const httpPort = process.argv[2] || 80;
const app = express();

const options = { index: ['index.html', 'index.htm', 'default.html', 'default.htm'] };
app.use(express.static('data', options));
// eslint-disable-next-line no-console
app.listen(httpPort, () => console.log(`app listening on port ${httpPort}`));
