function WebSocketMng(ip, port, callback) {
    this.IP = ip;
    this.Port = port;
    this.Callback = callback;
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
        if (e.data.Action == "RequestCmd") {
            if (wsm.Callback != null) {
                var cmd = wsm.Callback();
                if(cmd != null) {
                    var jcmd = cmd.toJSON();
                    var data = { IP: wsm.IP, Port: wsm.Port, Cmd: jcmd };
                    this.postMessage(data);
                }
            }
        }
        else {
            $("#Logger").append(e.data.Msg + "\n");
            $('#Logger').scrollTop($('#Logger')[0].scrollHeight);
        }
    };
}

WebSocketMng.prototype.Stop = function () {
    if (this.Worker == null)
        return

    this.Worker.terminate();
    this.Worker = null;
}