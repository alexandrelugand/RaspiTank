//var ImageNr = 1; // Serial number of current image
//var Finished = new Array(); // References to img objects which have finished downloading

function WebCamMng(ip, port, placeholder) {
    this.IP = ip;
    this.Port = port;  
    this.PlaceHolder = placeholder;
    this.Worker = null;
    this.ImageNr = 0; // Serial number of current image
    this.Finished = new Array(); // References to img objects which have finished downloading
    this.Started = false;
};

WebCamMng.prototype.Start = function () {
    this.Started = true;
    this.CreateImageLayer();
}

WebCamMng.prototype.Stop = function () {
    this.Started = false;
}

WebCamMng.prototype.CreateImageLayer = function() {
    var img = new Image();
    img.style.position = "absolute";
    img.style.zIndex = -1;
    var wcm = this;
    img.onload = function () {
        img.style.zIndex = wcm.ImageNr; // Image finished, bring to front!
        while (1 < wcm.Finished.length) {
            var del = wcm.Finished.shift(); // Delete old image(s) from document
            del.parentNode.removeChild(del);
        }
        wcm.Finished.push(this);
        if (wcm.Started)
            wcm.CreateImageLayer();
    };
    img.src = window.location.protocol+'//'+ this.IP + ':' + this.Port + "/?action=snapshot&n=" + (++this.ImageNr);
    var webcam = $(this.PlaceHolder)[0];
    webcam.insertBefore(img, webcam.firstChild);
}

//WebCamMng.prototype.Start = function () {
//    if (this.Worker != null)
//        return;

//    this.Worker = new Worker("../Scripts/WebCamWorker.js");
//    this.Worker.onerror = this.WorkerErrorReceiver;
//    this.Worker.onmessage = this.workerResultReceiver;
//    var data = { ip: this.IP, port: this.Port, imgNr: ImageNr, placeholder: this.PlaceHolder };
//    this.Worker.postMessage(data);
//};

//WebCamMng.prototype.Stop = function () {
//    if (this.Worker == null)
//        return
//    this.Worker.terminate();
//    this.Worker = null;
//};

//WebCamMng.prototype.WorkerErrorReceiver = function (e) {
//    console.log("there was a problem with the WebWorker " + e);
//}

//WebCamMng.prototype.workerResultReceiver = function (e) {

//    ImageNr++;
//    var img = new Image;
//   // img.src = "data:image/jpeg;base64," + Encode64(e.data.img);
//    img.src = e.data.img;
//    img.style.display = "";
//    var placeHolder = $(e.data.placeholder)[0];
//    placeHolder.insertBefore(img, placeHolder.firstChild);
//    img.style.zIndex = ImageNr; // Image finished, bring to front!
//    while (1 < Finished.length) {
//        var del = Finished.shift(); // Delete old image(s) from document
//        del.parentNode.removeChild(del);
//    }
//    Finished.push(img);
//    var data = { ip: this.IP, port: this.Port, imgNr: ImageNr, placeholder: e.data.placeholder };
//    this.postMessage(data);
//}
