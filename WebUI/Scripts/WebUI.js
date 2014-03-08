/// <reference path="~/Scripts/jquery-2.1.0.js" />

/* Raspberry Tank Web UI JavaScript
   Written by Ian Renton (http://ianrenton.com), February 2013
   Released into the public domain without licence. */

// All commands that could be sent to the vehicle.
var command = null;

// Port on which the tank's control server runs
var CONTROL_PORT = 3000;

// Port on which the mjpg-streamer webcam server runs
var WEBCAM_PORT = 8080;
var WEBSOCKET_PORT = 3000;

var webcamMng;
var websocketMng;
var command;
var turrelGmPd;
var moveGmPd;
var engineState;

function RequestCmd(e) {
    if (e.data.Action == "RequestCmd") {
        if (command != null) {
            var cmd = command;
            command = null;
            return cmd;
        }
    } else if (e.data.Action == "LOG") {
        $("#Logger").append(e.data.Msg + "\n");
        $('#Logger').scrollTop($('#Logger')[0].scrollHeight);
    }
    else if (e.data.Action == "ENGINE_STATUS") {
        $("#Logger").append("Engine status changed: " + e.data.Msg + "\n");
        $('#Logger').scrollTop($('#Logger')[0].scrollHeight);
        engineState = e.data.Msg;
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

    turrelGmPd = new GamePad($("#turrel-ctrl"), "../img/left_stick.png", 60, 60, 2, 20, 15, { X: 337, Y: 83, Size: 91 }, UpdateTurrelGmPd);
    moveGmPd = new GamePad($("#move-ctrl"), "../img/right_stick.png", 60, 60, 2, 20, 15, { X: 337, Y: 83, Size: 91 }, UpdateMoveGmPd);

    $("#main").on("touchmove", function (e) {
        e.preventDefault();
    });

    engineState = "stop";
    var startbt = $("#startbutton");
    var warmupTimer = 0;
    startbt.on("click", function (e) {
        if (engineState == "stop") {
            clearInterval(warmupTimer);
            command = new Command();
            command.engineStart = true;
            engineState = "warmup";
            startbt.attr("src", "../img/StartStop_Off_d.png");
            setTimeout(function() {
                startbt.attr("src", "../img/StartStop_WarmUp.png");
            }, 100);
            warmupTimer = setInterval(function () {
                if (engineState == "start") {
                    clearInterval(warmupTimer);
                    startbt.attr("src", "../img/StartStop_On.png");
                } 
            }, 1000);      
        } else if (engineState == "start") {
            clearInterval(warmupTimer);
            command = new Command();
            command.engineStop = true;
            startbt.attr("src", "../img/StartStop_On_d.png");
            setTimeout(function () {
                startbt.attr("src", "../img/StartStop_WarmUp.png");
            }, 100);
            engineState = "warmup";
            warmupTimer = setInterval(function () {
                if (engineState == "stop") {
                    clearInterval(warmupTimer);
                    startbt.attr("src", "../img/StartStop_Off.png");
                }
            }, 1000);
        }
    });

    var emergencybt = $("#emergencybutton");
    emergencybt.on("click", function(e) {
        clearInterval(warmupTimer);
        engineState = "stop";
        command = new Command();
        command.engineStop = true;
        startbt.attr("src", "../img/StartStop_Off.png");
        emergencybt.attr("src", "../img/emergency_d.png");
        setTimeout(function () {
            emergencybt.attr("src", "../img/emergency.png");
        }, 100);
    });
    
    var firebt = $("#firebutton");
    firebt.on("click", function (e) {
        firebt.css("background-image", "url('../img/fire_d.png')");
        setTimeout(function () {
            firebt.css("background-image", "url('../img/fire.png')");
        }, 100);
        if (command == null)
            command = new Command();
        command.repeat = 10;
        command.fire = true;
    });
    
    var gunbt = $("#gunbutton");
    gunbt.on("click", function (e) {
        gunbt.css("background-image", "url('../img/gun_d.png')");
        setTimeout(function () {
            gunbt.css("background-image", "url('../img/gun.png')");
        }, 100);
        if (command == null)
            command = new Command();
        command.repeat = 40;
        command.gun = true;
    });

    var console = $("#log_panel");
    console.hammer().on('swipeup', function (event) {
        event.preventDefault();
        console.animate({ top: "481px" }, 500);
    })
    .on('dragup', function (event) {
        event.preventDefault();
        console.animate({ top: "481px" }, 500);
    })
    .on('swipedown', function (event) {
        event.preventDefault();
        console.animate({ top: "625px" }, 500);
    })
    .on('dragdown', function (event) {
        event.preventDefault();
        console.animate({ top: "625px" }, 500);
    })
    .on('doubletap', function (event) {
        event.preventDefault();
        if (console.css('top') == "481px")
            console.animate({ top: "625px" }, 500);
        else
            console.animate({ top: "481px" }, 500);
    });
});

function UpdateTurrelGmPd(event) {
    var hint = 2;

    var cmd = null;
    if (event.X > 50) {
        if (command == null)
            command = new Command();
        command.repeat = hint;
        command.turrelRotation = 2;
    }
    else if (event.X < -50) {
        if (command == null)
            command = new Command();
        command.repeat = hint;
        command.turrelRotation = 1;
    }

    if (event.Y > 50) {
        if (command == null)
            command = new Command();
        command.repeat = hint * 2;
        command.canonElevation = true;
    }

    //if (this.debug) {
    //    $("#" + this.DebugCtrl).html("<h1>X: " + Number(X).toFixed(1) + "</h1><h1>Y: " + Number(Y).toFixed(1) + "</h1>");
    //}
}

function UpdateMoveGmPd(event) {
    var hint = 3;

    var cmd = null;
    if (event.X > 15) {
        if (command == null)
            command = new Command();
        command.repeat = hint;
        command.rotation = 2; //right
        command.rotspeed = Math.abs(Math.round(event.X / 15));
    }
    else if (event.X < -15) {
        if (command == null)
            command = new Command();
        command.repeat = hint;
        command.rotation = 1; //left
        command.rotspeed = Math.abs(Math.round(event.X / 15));
    }

    if (event.Y < -15) {
        if (command == null)
            command = new Command();
        command.repeat = hint;
        command.direction = 0; //Forward
        command.dirspeed = Math.abs(Math.round(event.Y / 15));
    }
    else if (event.Y > 15) {
        if (command == null)
            command = new Command();
        command.repeat = hint;
        command.direction = 1; //Reverse
        command.dirspeed = Math.abs(Math.round(event.Y / 15));
    }
}

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
