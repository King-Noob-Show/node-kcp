## Build:

```shell
npm install -g node-gyp
git clone https://github.com/King-Noob-Show/node-kcp.git
cd node-kcp
node-gyp configure
node-gyp build
```

## Example:

### Install by npm

```shell
npm install github:king-noob-show/node-kcp
```

### udpserver.js

```js
var kcp = require('node-kcp-token');
var dgram = require('dgram');
var server = dgram.createSocket('udp4');
var clients = {};
var interval = 200;

var output = function(data, size, context) {
    server.send(data, 0, size, context.port, context.address);
};

server.on('error', (err) => {
    console.log(`server error:\n${err.stack}`);
    server.close();
});

server.on('message', (msg, rinfo) => {
    var k = rinfo.address+'_'+rinfo.port;
    if (undefined === clients[k]) {
        var context = {
            address : rinfo.address,
            port : rinfo.port
        };
        var kcpobj = new kcp.KCP(123, 1, context);
        kcpobj.nodelay(0, interval, 0, 0);
        kcpobj.output(output);
        clients[k] = kcpobj;
    }
    var kcpobj = clients[k];
    kcpobj.input(msg);
});

server.on('listening', () => {
    var address = server.address();
    console.log(`server listening ${address.address} : ${address.port}`);
    setInterval(() => {
        for (var k in clients) {
            var kcpobj = clients[k];
        	kcpobj.update(Date.now());
        	var recv = kcpobj.recv();
        	if (recv) {
            	console.log(`server recv ${recv} from ${kcpobj.context().address}:${kcpobj.context().port}`);
           		kcpobj.send('RE-'+recv);
       	 	}
       	}
    }, interval);
});

server.bind(41234);

```

### udpclient.js

```js
var kcp = require('node-kcp');
var kcpobj = new kcp.KCP(123, 1, {address: '127.0.0.1', port: 41234});
var dgram = require('dgram');
var client = dgram.createSocket('udp4');
var msg = 'hello world';
var idx = 1;
var interval = 200;

kcpobj.nodelay(0, interval, 0, 0);

kcpobj.output((data, size, context) => {
    client.send(data, 0, size, context.port, context.address);
});

client.on('error', (err) => {
    console.log(`client error:\n${err.stack}`);
    client.close();
});

client.on('message', (msg, rinfo) => {
    kcpobj.input(msg);
});

setInterval(() => {
    kcpobj.update(Date.now());
    var recv = kcpobj.recv();
    if (recv) {
        console.log(`client recv ${recv}`);
        kcpobj.send(msg+(idx++));
    }
}, interval);

kcpobj.send(msg+(idx++));

```