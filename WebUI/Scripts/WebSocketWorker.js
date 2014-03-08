var command = "";
var socket = null;

setInterval(function () {
    if (command != "") {
        if (socket != null && socket.readyState == 1) {
            socket.send(command);
            command = "";
        }
    }
    var data = { Action: "RequestCmd" };
    postMessage(data);
}, 100);

function OnErrorReceive(e) {
    socket = null;
    var msg = "There was a problem with the WebSocket: " + e;
    var data = { Action: "ErrorReceived", Msg: msg };
    postMessage(data);
}

function OnMessageReceive(e) {
    var msg = e.data;
    if (msg != null && msg.length > 0) {
        if (msg.charAt(0) == "$") {
            var enddelim = msg.indexOf("$", 1);
            var action = msg.substring(1, enddelim);
            var strmsg = msg.substring(enddelim + 2, msg.length);
            var data = { Action: action, Msg: strmsg };
            postMessage(data);
        }
    }
}

self.onmessage = function (ev) {
    if (socket == null) {
        socket = new WebSocket("ws://" + ev.data.IP + ":" + ev.data.Port);
        socket.onerror = OnErrorReceive;
        socket.onmessage = OnMessageReceive;
    }
    command = ev.data.Cmd;    
};
