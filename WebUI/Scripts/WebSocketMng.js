function WebSocketMng(ip, port, callback) {
    this.IP = ip;
    this.Port = port;
    this.Callback = callback;
    this.Socket = null;
    this.CreateSocket();
};


WebSocketMng.prototype.CreateSocket = function(cmd) {
    if (this.Socket == null) {
        this.Socket = new WebSocket("ws://" + this.IP + ":" + this.Port);
        var wsm = this;
        this.Socket.onerror = function(e) {
            wsm.Socket = null;
            wsm.CreateSocket();
            var msg = "There was a problem with the WebSocket: " + e;
            var data = { data: { Action: "ErrorReceived", Msg: msg } };
            if (wsm.Callback != null) {
                wsm.Callback(data);
            }
        };

        this.Socket.onmessage = function(e) {
            var msg = e.data;
            if (msg != null && msg.length > 0) {
                if (msg.charAt(0) == "$") {
                    var enddelim = msg.indexOf("$", 1);
                    var action = msg.substring(1, enddelim);
                    var strmsg = msg.substring(enddelim + 2, msg.length);
                    var data = { Action: action, Msg: strmsg };
                    if (wsm.Callback != null) {
                        wsm.Callback(data);
                    }
                }
            }
        };
    }
};

WebSocketMng.prototype.SendCmd = function(cmd, repeat) {
    var jcmd = cmd.toJSON();
    if (jcmd != "") {
        if (this.Socket != null && this.Socket.readyState == 1) {
            for(var i = 0; i < repeat; i++)
                this.Socket.send(jcmd);
        }
    }
};