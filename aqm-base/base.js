const SerialPort = require('serialport');
const Readline = require('@serialport/parser-readline')

var port = new SerialPort('COM9', {baudRate: 115200}, function(e) {
    if (e) console.log('ERROR: ', e.message);
});

function readData(raw) {
    let r = raw.split('>');
    let data = r[0].replace('<', '').split(',');
    let chk = r[1];
    console.log(data, chk);
    switch (data[2]) {
        case '0':
            console.log("REQ recieved");
            break;
    }
}

var parser = port.pipe(new Readline({ delimiter: '\n' }));
parser.on('data', readData);
parser.on('error', console.log)

setInterval(() => port.write('<1,0,0>\n'), 500);