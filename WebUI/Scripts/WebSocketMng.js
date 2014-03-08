function WebSocketMng(ip, port, callback) {
    this.IP = ip;
    this.Port = port;
    this.Callback = callback;
    this.Worker = null;
};

WebSocketMng.prototype.Start = function () {
    if (this.Worker != null)
        return;

    this.Worker = new Worker("../Scripts/WebSocketWorker.js");
    var wsm = this;

    this.Worker.onerror = function (e) {
        console.log("There was a problem with the WebSocket WebWorker: " + e);
    };

    this.Worker.onmessage = function (e) {
        if (wsm.Callback != null) {
            var cmd = wsm.Callback(e);
            if (cmd != null) {
                var jcmd = cmd.toJSON();
                var data = { IP: wsm.IP, Port: wsm.Port, Cmd: jcmd };
                this.postMessage(data);
            }
        }
    };
}

WebSocketMng.prototype.Stop = function () {
    if (this.Worker == null)
        return

    this.Worker.terminate();
    this.Worker = null;
}

WebSocketMng.prototype.SendCmd = function (cmd) {
    if (this.Worker == null)
        return

    if (cmd != null) {
        var jcmd = cmd.toJSON();
        var data = { IP: wsm.IP, Port: wsm.Port, Cmd: jcmd };
        this.postMessage(data);
    }
}