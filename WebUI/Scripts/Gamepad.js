/// <reference path="Command.js" /> 
/// <reference path="javascript.js" /> 

//var canvas;
//var context;
//var BUFFER;
//var WIDTH;
//var HEIGHT;

//var image;
//var limitSize;
//var inputSize;
//var lastTime;
//var stick;
//var threshold;
//var point;

function GamePad(ctrl, imgsrc, inputSize, limitSize, threshold, pointRadius, pointSpeed, knobe, debug)
{
    this.ctrl = ctrl;
    this.context = ctrl[0].getContext("2d");    
    this.width = ctrl[0].offsetWidth;
    this.height = ctrl[0].offsetHeight;
    this.buffer = parseInt(this.width / 2 );

    this.image = new Image();
    this.limitSize = limitSize;
    this.inputSize = inputSize;
    this.lastTime = Date.now();
    this.stick = new Stick(inputSize, false);
    this.threshold = threshold;
    this.imgsrc = imgsrc;
    this.point = {
        radius: pointRadius,
        speed: pointSpeed,
        x: (this.width / 2),
        y: (this.height / 2)
    };
    this.knobe = knobe;
    this.debug = debug;
    if (this.debug) {
        this.DebugCtrl = "Label" + (1 + Math.floor(Math.random() * 10));    
        $('<p id="' + this.DebugCtrl + '"></p>').insertAfter(this.ctrl);
    }

    this.Init();
}

GamePad.prototype.Draw = function() {
	this.context.clearRect(0, 0, this.width, this.height);
	this.DrawStick();
}

GamePad.prototype.DrawStick = function() {
	this.context.save();

	// Limit
	this.context.drawImage(
		this.image,
		0, 0,
		this.width, this.width,
        0, 0,
        this.width, this.width
    );

	// Input
	this.context.drawImage(
		this.image,
		this.knobe.X, this.knobe.Y,
		this.knobe.Size, this.knobe.Size,
		this.stick.input.x - (this.knobe.Size / 2), this.stick.input.y - (this.knobe.Size / 2),
		this.knobe.Size, this.knobe.Size
	);

	this.context.restore();
}

GamePad.prototype.Init = function () {
    this.stick.setLimitXY(this.buffer, (this.height - this.buffer));
	this.stick.setInputXY(this.buffer, (this.height - this.buffer));

	var that = this;
	this.ctrl.mouseover(function (e) {
	    that.stick.enabled = true;
    });

	this.ctrl.mouseout(function (e) {
	    that.stick.enabled = false;
    });

	$(document).mousedown(function (e) {
	    e.preventDefault();
	    if (that.stick.enabled) {
	        that.stick.setInputXY(e.pageX - that.ctrl[0].offsetLeft, e.pageY - that.ctrl[0].offsetTop);
	        that.stick.active = true;
	    }
	});

	$(document).mousemove(function (e) {
		e.preventDefault();
		if (that.stick.active) {
		    that.stick.setInputXY(e.pageX - that.ctrl[0].offsetLeft, e.pageY - that.ctrl[0].offsetTop);
		}
	});

	$(document).mouseup(function (e) {
	    that.stick.active = false;
	    that.stick.setInputXY(that.stick.limit.x, that.stick.limit.y);
	});

	this.image.src = this.imgsrc;
    this.image.onload = function () {
        setInterval(function() {
	        var now = Date.now();
            var elapsed = (now - that.lastTime);

            that.Update(elapsed);
            that.Draw();

            that.lastTime = now;
        }, 20);
	};
}

GamePad.prototype.Update = function (elapsed) {
	this.stick.update();

	if (this.stick.active) {
	    this.point.x = this.stick.input.x;
	    this.point.y = this.stick.input.y;

	    if (this.point.x < 0) {
	        this.point.x = 0;
	    } else if (this.point.x > this.width) {
	        this.point.x = this.width;
	    }

	    if (this.point.y < 0) {
	        this.point.y = 0;
	    } else if (this.point.y > this.height) {
	        this.point.y = this.height;
	    }
	}
	else
	{
	    this.point.x = this.stick.limit.x;
	    this.point.y = this.stick.limit.y;
	}

	var X = ((this.point.x - this.stick.limit.x) / this.inputSize) * 100;
	var Y = ((this.point.y - this.stick.limit.y) / this.inputSize) * 100;

	var hint = 2;

	var cmd = null;
	if (X > 50)
	{
	    if (command == null)
	        command = new Command();
	    command.repeat = hint;
	    command.turrelRotation = 2;
	}
	else if (X < -50)
	{
	    if (command == null)
	        command = new Command();
	    command.repeat = hint;
	    command.turrelRotation = 1;
	}
    
	if (Y > 50) {
	    if (command == null)
	        command = new Command();
	    command.repeat = hint * 2;
	    command.canonElevation = true;
	}

	if (this.debug) {
	    $("#" + this.DebugCtrl).html("<h1>X: " + Number(X).toFixed(1) + "</h1><h1>Y: " + Number(Y).toFixed(1) + "</h1>");
	}	
}
