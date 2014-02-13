/// <reference path="Command.js" /> 
/// <reference path="javascript.js" /> 

var canvas;
var context;
var BUFFER;
var WIDTH;
var HEIGHT;

var image;
var limitSize;
var inputSize;
var lastTime;
var stick;
var threshold;
var point;

function draw() {
	context.clearRect(0, 0, WIDTH, HEIGHT);
	drawStick();
	drawPoint();
};

function drawPoint () {
	context.save();

	context.beginPath();
	context.arc(point.x, point.y, point.radius, 0, (Math.PI * 2), true);

	context.lineWidth = 3;
	context.strokeStyle = "rgb(0, 200, 0)";
	context.stroke();

	context.restore();
};

function drawStick () {
	context.save();

	// Limit
	context.drawImage(
		image,
		0, 0,
		88, 88,
		stick.limit.x - (limitSize / 2), stick.limit.y - (limitSize / 2),
		limitSize, limitSize
	);

	// Input
	var knobSize = 60;
	context.drawImage(
		image,
		90, 14,
		knobSize, knobSize,
		stick.input.x - (knobSize / 2), stick.input.y - (knobSize / 2),
		knobSize, knobSize
	);

	context.restore();
};

function init () {
	stick.setLimitXY(BUFFER, (HEIGHT - BUFFER));
	stick.setInputXY(BUFFER, (HEIGHT - BUFFER));

	$(document).mousedown(function (e) {
		e.preventDefault();
		stick.setInputXY(e.pageX - canvas.offsetLeft, e.pageY - canvas.offsetTop);
		stick.active = true;
	});

	$(document).mousemove(function (e) {
		e.preventDefault();
		if (stick.active) {
		    stick.setInputXY(e.pageX - canvas.offsetLeft, e.pageY - canvas.offsetTop);
		}
	});

	$(document).mouseup(function (e) {
		stick.active = false;
		stick.setInputXY(stick.limit.x, stick.limit.y);
	});

	image.src = "../img/stick.png";
	image.onload = function () {
		setInterval(main, 500);
	};
};

function main () {
	var now = Date.now();
	var elapsed = (now - lastTime);

	update(elapsed);
	draw();

	lastTime = now;
};

function update (elapsed) {
	stick.update();

	if (stick.active) {
	//if (stick.active && (stick.length > threshold)) {
		//point.x += (
		//	(stick.length * stick.normal.x)
		//	* point.speed
		//	* (elapsed / 1000)
		//);
	    point.x = stick.input.x;
		//point.y += (
		//	(stick.length * stick.normal.y)
		//	* point.speed
		//	* (elapsed / 1000)
		//);
	    point.y = stick.input.y;

	    if (point.x < 0) {
	        point.x = 0;
	    } else if (point.x > WIDTH) {
	        point.x = WIDTH;
	    }

	    if (point.y < 0) {
	        point.y = 0;
	    } else if (point.y > HEIGHT) {
	        point.y = HEIGHT;
	    }

		//if (point.x < point.radius) {
		//	point.x = point.radius;
		//} else if (point.x > (WIDTH - point.radius)) {
		//	point.x = (WIDTH - point.radius);
		//}
		//if (point.y < point.radius) {
		//	point.y = point.radius;
		//} else if (point.y > (HEIGHT - point.radius)) {
		//	point.y = (HEIGHT - point.radius);
		//}
	}
	else
	{
	    point.x = stick.limit.x;
	    point.y = stick.limit.y;
	}

	var X = ((point.x - stick.limit.x) / inputSize) * 100;
	var Y = ((point.y - stick.limit.y) / inputSize) * 100;

	var cmd = null;
	if (X > 50)
	{
	    if (cmd == null)
	        cmd = new Command();
	    cmd.repeat = 20;
	    cmd.turrelRotation = 2;
	}
	else if (X < -50)
	{
	    if (cmd == null)
	        cmd = new Command();
	    cmd.repeat = 20;
	    cmd.turrelRotation = 1;
	}
    
	if (Y > 50) {
	    if (cmd == null)
	        cmd = new Command();
	    cmd.repeat = 20;
	    cmd.canonElevation = true;
	}
    
	if (ws != null && cmd != null)
	{
	    var jsonCmd = cmd.toJSON();
	    ws.send(jsonCmd);
	    $('#CmdInput').text(jsonCmd);
	}
	else
	    $('#CmdInput').text("");

	$("#pointLabel").html("<h1>X: " + X + "</h1><h1>Y: " + Y + "</h1>");
};

$(function() {
    canvas = $("#controller")[0];
    context = canvas.getContext("2d");
    BUFFER = 65;
    WIDTH = canvas.offsetWidth;
    HEIGHT = canvas.offsetHeight;

    image = new Image();
    limitSize = 88;
    inputSize = 20;
    lastTime = Date.now();
    stick = new Stick(inputSize);
    threshold = 2;
    point = {
        radius: 20,
        speed: 15,
        x: (WIDTH / 2),
        y: (HEIGHT / 2)
    };

    init();
});
