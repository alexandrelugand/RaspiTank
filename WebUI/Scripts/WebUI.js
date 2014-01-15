/// <reference path="~/Scripts/jquery-2.1.0.js" />

/* RaspiTank Web UI JavaScript
   Written by Alexandre Lugand, January 2014
   Released into the public domain without licence. */


// Port on which the tank's control server runs
var CONTROL_PORT = 3000;

// Port on which the mjpg-streamer webcam server runs
var WEBCAM_PORT = 8080;
var WEBSOCKET_PORT = 3000;

var webcamMng;
var websocketMng;
var turrelGmPd;
var moveGmPd;
var engineState;
var recoil;
var alertsTimer;
var rangeTimer;
var sensors;
var canvas;
var redRetcule1;
var redRetcule2;
var greenRetcule1;
var greenRetcule2;
var reticuleNum;

function RequestCmd(data) {
    if (data.Action == "LOG") {
        Log(data.Msg);
    }
    else if (data.Action == "ENGINE_STATUS") {
        engineState = data.Msg;
    }
    else if (data.Action == "SENSORS") {
        sensors = JSON.parse(data.Msg);
    }
    return null;
}

// Executes on page load.
$(function () {

    redRetcule1 = new Image();
    redRetcule1.src = "../img/reticule_rouge_1.png";
    
    redRetcule2 = new Image();
    redRetcule2.src = "../img/reticule_rouge_2.png";
    
    greenRetcule1 = new Image();
    greenRetcule1.src = "../img/reticule_vert_1.png";
    
    greenRetcule2 = new Image();
    greenRetcule2.src = "../img/reticule_vert_2.png";
    
    canvas = $("#webcamCanvas")[0];
    reticuleNum = 0;

    //webcamMng = new WebCamMng(window.location.host, WEBCAM_PORT);
    webcamMng = new WebCamMng("192.168.0.10", WEBCAM_PORT, "#webcam");
    webcamMng.Start();

    //websocketMng = new websocketMng(window.location.host, WEBSOCKET_PORT);
    websocketMng = new WebSocketMng("192.168.0.10", WEBSOCKET_PORT, RequestCmd);

    turrelGmPd = new GamePad(0, $("#turrel-ctrl"), "../img/left_stick.png", 60, 60, 2, 20, 15, { X: 337, Y: 83, Size: 91 }, UpdateTurrelGmPd);
    moveGmPd = new GamePad(1, $("#move-ctrl"), "../img/right_stick.png", 60, 60, 2, 20, 15, { X: 337, Y: 83, Size: 91 }, UpdateMoveGmPd);

    $("#main").on("touchmove", function (e) {
        e.preventDefault();
    });

    engineState = "stop";
    recoil = false;
    var startbt = $("#startbutton");
    var warmupTimer = 0;
    startbt.on("click", function (e) {
        if (engineState == "stop") {
            clearInterval(warmupTimer);
            var command = new Command();
            command.engineStart = true;
            websocketMng.SendCmd(command, 1);
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
            var command = new Command();
            command.engineStop = true;
            websocketMng.SendCmd(command, 1);
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
        var command = new Command();
        command.engineStop = true;
        websocketMng.SendCmd(command, 1);
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
        var command = new Command();
        var repeat;
        if (!recoil) {
            command.fire = true;
            repeat = 10;
        } else {
            command.recoil = true;
            repeat = 50;
        }
        websocketMng.SendCmd(command, repeat);
    });
    
    var gunbt = $("#gunbutton");
    gunbt.on("click", function (e) {
        gunbt.css("background-image", "url('../img/gun_d.png')");
        setTimeout(function () {
            gunbt.css("background-image", "url('../img/gun.png')");
        }, 100);
        var command = new Command();
        var repeat = 40;
        command.gun = true;
        websocketMng.SendCmd(command, repeat);
    });
    
    
    var recoilbt = $("#recoilbutton");
    recoilbt.on("click", function (e) {
        if (!recoil) {
            recoilbt.css("background-image", "url('../img/recoil_on.png')");
            recoil = true;
            var show = true;
            alertsTimer = setInterval(function () {
                show = drawAlerts(show);
            }, 500);
            rangeTimer = setInterval(function () {
                drawRangeInfo();
            }, 500);
            showReticule(-1);
        } else {
            recoilbt.css("background-image", "url('../img/recoil_off.png')");
            recoil = false;
            clearInterval(alertsTimer);
            clearInterval(rangeTimer);
            clearAllCanvas();
            showReticule(0);
        }
    });
    
    var reticulebt = $("#reticuleSelect");
    reticulebt.on("click", function (e) {
        if (!recoil)
            return false;
        
        if (reticuleNum > -2) {
            reticuleNum--;
            showReticule(reticuleNum);
        } else {
            showReticule(2);
        }
        return true;
    })
    .on("contextmenu", function (e) {
        if (!recoil)
            return false;
        
        if (reticuleNum < 2) {
            reticuleNum++;
            showReticule(reticuleNum);
        } else {
            showReticule(-2);
        }
        return false;
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

$(window).unload(function () {
    alert("Bye now!");
});

function clearAllCanvas() {
    if (!canvas)
        return;
    
    var ctx = canvas.getContext("2d");
    ctx.clearRect(0, 0, 640, 480);
}

function drawRangeInfo() {
    if (!canvas)
        return;

    if (sensors == null)
        return;
    
    var ctx = canvas.getContext("2d");
    ctx.strokeStyle = "red"; // Définition de la couleur de contour
    ctx.lineWidth = 1;
    
    ctx.clearRect(20, 440, 200, 480);
    ctx.font = "18pt Verdana";
    ctx.textAlign = "left";
    ctx.textBaseline = "top";
    ctx.fillStyle = "red";
    var text = "Range: " + sensors.Range;
    ctx.fillText(text, 20, 440);

    ctx.stroke();
}

function drawAlerts(show) {
    if (!canvas)
        return false;

    var ctx = canvas.getContext("2d");
    ctx.strokeStyle = "red"; // Définition de la couleur de contour
    ctx.lineWidth = 1;

    ctx.clearRect(420, 10, 640, 30);
    ctx.font = "18pt Verdana";
    ctx.textAlign = "left";
    ctx.textBaseline = "top";
    ctx.fillStyle = "red";
    if (show) {
        var text = "Engage activated";
        ctx.fillText(text, 420, 10);
    }
    
    ctx.stroke();
    
    return !show;
}

function showReticule(num) {
    reticuleNum = num;
    var img = null;
    var deg = 0;
    switch (num) {
        case -1:
            img = redRetcule1;
            deg = -60;
            break;
        case -2:
            img = redRetcule2;
            deg = -120;
            break;
        case 1:
            img = greenRetcule1;
            deg = 60;
            break;
        case 2:
            img = greenRetcule2;
            deg = 120;
            break;
        default:
            break;
    }
    
    $("#reticuleButton").css({
        "webkitTransform": "rotate(" + deg + "deg)",
        "MozTransform": "rotate(" + deg + "deg)",
        "msTransform": "rotate(" + deg + "deg)",
        "OTransform": "rotate(" + deg + "deg)",
        "transform": "rotate(" + deg + "deg)"
    });

    drawReticule(img);
}

function drawReticule(img) {
    if (!canvas)
        return;
    
    var ctx = canvas.getContext("2d");
  
    ctx.clearRect(176, 96, 464, 384);
    if (img) {
        ctx.drawImage(
            img,
            0, 0,
            288, 288,
            176, 96,
            288, 288
        );
    }

    ctx.stroke();
}

function UpdateTurrelGmPd(event) {
    var repeat = 1;
    var command;
    if (event.X > 50) {
        command = new Command();
        command.turrelRotation = 2;
        websocketMng.SendCmd(command, repeat);
    }
    else if (event.X < -50) {
        command = new Command();
        command.turrelRotation = 1;
        websocketMng.SendCmd(command, repeat);
    }

    if (event.Y > 50 || event.Y < -50) {
        command = new Command();
        command.canonElevation = true;
        websocketMng.SendCmd(command, repeat);
    }
}

function UpdateMoveGmPd(event) {
    var repeat = 1;
    var command;
    if (event.X > 15 && event.X < 50) {
        command = new Command();
        command.rotation = 2; //right
        command.rotspeed = 1; 
        websocketMng.SendCmd(command, repeat);
    }
    else if (event.X > 50 && event.X < 80) {
        command = new Command();
        command.rotation = 2; //right
        command.rotspeed = 2;
        websocketMng.SendCmd(command, repeat);
    }
    else if (event.X < -15 && event.X > -50) {
        command = new Command();
        command.rotation = 1; //left
        command.rotspeed = Math.abs(Math.round(event.X / 40));
        websocketMng.SendCmd(command, repeat);
    }
    else if (event.X < -50 && event.X > -80) {
        command = new Command();
        command.rotation = 1; //left
        command.rotspeed = 2;
        websocketMng.SendCmd(command, repeat);
    }

    if (event.Y < -15 && event.Y > -35) {
        command = new Command();
        command.direction = 0; //Forward
        command.dirspeed = 2;
        websocketMng.SendCmd(command, repeat);
    }
    else if (event.Y < -35) {
        command = new Command();
        command.direction = 0; //Forward
        command.dirspeed = 3;
        websocketMng.SendCmd(command, repeat);
    }
    else if (event.Y > 15 && event.Y < 35) {
        command = new Command();
        command.direction = 1; //Reverse
        command.dirspeed = 2;
        websocketMng.SendCmd(command, repeat);
    }
    else if (event.Y > 35) {
        command = new Command();
        command.direction = 1; //Reverse
        command.dirspeed = 3;
        websocketMng.SendCmd(command, repeat);
    }
}

