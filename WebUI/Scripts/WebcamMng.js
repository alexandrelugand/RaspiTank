function WebCamMng(ip, port, placeholder) {
    this.IP = ip;
    this.Port = port;  
    this.PlaceHolder = placeholder;
};

WebCamMng.prototype.Start = function() {
    this.CreateImageLayer();
};

WebCamMng.prototype.Stop = function() {
    this.DeleteImageLayer();
};

WebCamMng.prototype.CreateImageLayer = function() {
    var img = new Image();
    img.id = "webcamRender";
    img.zIndex = 1;
    img.style.position = "absolute";
    img.src = window.location.protocol + '//' + this.IP + ':' + this.Port + "/?action=stream";
    var webcam = $(this.PlaceHolder)[0];
    webcam.insertBefore(img, webcam.firstChild);
};

WebCamMng.prototype.DeleteImageLayer = function() {
    var img = $("#webcamRender")[0];
    var webcam = $(this.PlaceHolder)[0];
    webcam.removeChild(img);
};