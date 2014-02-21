/// <reference path="~/Scripts/jquery-2.1.0.js" />

/* Raspberry Tank Web UI JavaScript
   Written by Ian Renton (http://ianrenton.com), February 2013
   Released into the public domain without licence. */

// All commands that could be sent to the vehicle.
var command = {
    'forward' : false,
    'reverse' : false,
    'left' : false,
    'right' : false,
    'turret_left' : false,
    'turret_right' : false,
    'turret_elev' : false,
    'fire' : false,
    'autonomy' : false
}

// Port on which the tank's control server runs
var CONTROL_PORT = 3000;

// Port on which the mjpg-streamer webcam server runs
var WEBCAM_PORT = 8080;
var WEBSOCKET_PORT = 3000;

var webcamMng;
var websocketMng;
var command;
var turrelGmPd;

function RequestCmd() {
    if (command != null) {
        var cmd = command;
        command = null;
        return cmd;
    }      
    return null;
}

// Executes on page load.
$(function () {
    //webcamMng = new WebCamMng(window.location.host, WEBCAM_PORT);
    webcamMng = new WebCamMng("192.168.0.10", WEBCAM_PORT, "#webcam");
    webcamMng.Start();

    //websocketMng = new websocketMng(window.location.host, WEBSOCKET_PORT);
    websocketMng = new WebSocketMng("192.168.0.10", WEBSOCKET_PORT, RequestCmd);
    websocketMng.Start();

    turrelGmPd = new GamePad($("#controller"), "../img/left_stick.png", 60, 60, 2, 20, 15, { X: 337, Y: 83, Size: 91 }, true);
    
    //setInterval(updateSensorData, 1000);
});


// Sets a command to either true or false by name, e.g. to go forwards use
// set('forwards', true) and to stop going forwards, use set('forwards', false).
function set(name, value) {
    command[name] = value;
    send();
    return true;
}

// Toggles the state of autonomy.
function toggleAutonomy() {
    if (command['autonomy'] == true) {
        command['autonomy'] = false;
        $('span.autonomystate').html("OFF");
        $('span.autonomybutton').html("Switch ON");
    } else {
        command['autonomy'] = true;
        $('span.autonomystate').html("ON");
        $('span.autonomybutton').html("Switch OFF");
    }
    send();
}

// Set all commands to false, in case there's been a glitch and something is
// stuck on.
function stop() {
    for (var name in command) {
        command[name] = false;
    }
    send();
}

// Send the current command set to the vehicle.
function send() {
    var commandBits = "";
    for (var name in command) {
        commandBits = commandBits + (command[name] ? "1" : "0");
    }
    $.get("http://192.168.0.10:3000?set" + commandBits);
    /*$.get(window.location.protocol+'//'+window.location.host + ':' + CONTROL_PORT + "?set" + commandBits);*/

}

// Gets the sensor data
function updateSensorData() {
    $.get(window.location.protocol+'//'+window.location.host + "/sensordata.txt", "", function(data){
        if (data != "") {
            $('div.data').html("<h1>" + data + "</h1>");
        }
        else {
            $('div.data').html("<h1>-</h1>");
        }
    }, "html");
}