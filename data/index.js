/**
 * ----------------------------------------------------------------------------
 * ESP32 Remote Control with WebSocket
 * ----------------------------------------------------------------------------
 * © 2020 Stéphane Calderoni
 * ----------------------------------------------------------------------------
 */

var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
var alarmeAc = 0;
var alarmeLow = 0;
var play = 0;

// ----------------------------------------------------------------------------
// Initialization
// ----------------------------------------------------------------------------

window.addEventListener('load', onLoad);

function onLoad(event) {
    initWebSocket();
    initButton();
}

// ----------------------------------------------------------------------------
// WebSocket handling
// ----------------------------------------------------------------------------

function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
    ler();
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

//function onMessage(event) {
//    let data = JSON.parse(event.data);
//    document.getElementById('led').className = data.status;
//}

function onMessage(event) {
    let data = JSON.parse(event.data);
    console.log(data);
    //document.getElementById('led_' + data.rank).className = data.status;

    document.getElementById('localizacao').innerText = data.local;
    document.getElementById('acfail').innerText = data.acfail;
    document.getElementById('lowbat').innerText = data.lowbat;


    document.getElementById('ld0').innerText = data.log.ld0;
    document.getElementById('lh0').innerText = data.log.lh0;
    document.getElementById('lm0').innerText = data.log.lm0;

    document.getElementById('ld1').innerText = data.log.ld1;
    document.getElementById('lh1').innerText = data.log.lh1;
    document.getElementById('lm1').innerText = data.log.lm1;

    document.getElementById('ld2').innerText = data.log.ld2;
    document.getElementById('lh2').innerText = data.log.lh2;
    document.getElementById('lm2').innerText = data.log.lm2;

    document.getElementById('ld3').innerText = data.log.ld3;
    document.getElementById('lh3').innerText = data.log.lh3;
    document.getElementById('lm3').innerText = data.log.lm3;

    document.getElementById('ld4').innerText = data.log.ld4;
    document.getElementById('lh4').innerText = data.log.lh4;
    document.getElementById('lm4').innerText = data.log.lm4;

    if (data.acfail === "Normal") {
        document.getElementById('acfail').style.color = 'black';
        document.getElementById("bt_ac").style.display = "none";
        alarmeAc = 0;
    } else if (data.acfail === "Falha") {
        document.getElementById('acfail').style.color = 'red';
        document.getElementById("bt_ac").style.display = "block";
        alarmeAc = 1;
    } else {
        document.getElementById('acfail').style.color = 'red';
        document.getElementById("bt_ac").style.display = "none";
        alarmeAc = 0;
    }

    if (data.lowbat === "Normal") {
        document.getElementById('lowbat').style.color = 'black';
        document.getElementById("bt_low").style.display = "none";
        alarmeLow = 0;
    } else if (data.lowbat === "Falha") {
        document.getElementById('lowbat').style.color = 'red';
        document.getElementById("bt_low").style.display = "block";
        alarmeLow = 1;
    } else {
        document.getElementById('lowbat').style.color = 'red';
        document.getElementById("bt_low").style.display = "none";
        alarmeLow = 0;
    }
    //console.log(alarmeAc);
    //console.log(alarmeLow);
}
// ----------------------------------------------------------------------------
// Button handling
// ----------------------------------------------------------------------------

function initButton() {
    document.getElementById('bt_ac').addEventListener('click', clickacfail);
    document.getElementById('bt_low').addEventListener('click', clickLowbat);
}

function clickacfail(event) {
    websocket.send(JSON.stringify({ 'action': 'reconhecerAcfail' }));
}
function clickLowbat(event) {
    websocket.send(JSON.stringify({ 'action': 'reconhecerLowbat' }));
}

//-----------------------------------------------------------------------------
//SOM
//-----------------------------------------------------------------------------
let context = new AudioContext(),
    oscillator = context.createOscillator();
let i = 1;

setInterval(function run() {
    if (alarmeAc === 1 || alarmeLow === 1) {
        let resto = i % 2;
        //console.log(resto);
        if (resto) {
            context = new AudioContext();
            oscillator = context.createOscillator();
            oscillator.type = "sine";
            oscillator.connect(context.destination);
            oscillator.start();
            play = 1;
            //console.log("Play");
        } else {
            oscillator.stop(context.currentTime + 0);
            //console.log("Pause");
        }
        i++
    } else {
        if (play === 1) {
            oscillator.stop(context.currentTime + 0);
            play = 0;
        }
    }
}, 1000);

setInterval(() => {
    websocket.send(JSON.stringify({ 'action': 'obterleitura' }));
    //datacao();
    //ocultar();
}, 20000);

function ler() {
    websocket.send(JSON.stringify({ 'action': 'obterleitura' }));
}
//let lowBat = document.getElementById('lowbat');
////document.getElementById('bt_ac').style.display = 'none';
//document.getElementById('bt_low').style.display = 'inline';

function ocultar() {
    document.getElementById("bt_ac").style.display = "none";
    document.getElementById("bt_low").style.display = "none";
}

function datacao() {
    now = new Date
    let dia = now.getDate() + "/" + now.getMonth() + "/" + now.getFullYear();
    let hora = now.getHours() + ":" + now.getMinutes() + ":" + now.getSeconds();
    console.log(dia);
    console.log(hora);
}

