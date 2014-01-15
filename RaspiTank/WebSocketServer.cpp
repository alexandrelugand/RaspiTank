#include "WebSocketServer.h"
#include "websocket.h"
#include "log.h"
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <iostream>

using namespace RaspiTank;

WebSocketServer::WebSocketServer()
{
	o = NULL;
}

WebSocketServer::~WebSocketServer()
{
}

void WebSocketServer::Initialize()
{
	cout << "Initialize web socket server..." << endl;

	signal(SIGINT, WebSocketServer::Shutdown);
	signal(SIGTERM, WebSocketServer::Shutdown);
		
	o = onion_new(O_THREADED);
	onion_set_port(o, "3000");
	onion_url *urls = onion_root_url(o);
	onion_url_add(urls, "", (void*)WebSocketServer::OnConnect);	

	cout << "web socket server initialized" << endl;
}

void WebSocketServer::Listen()
{
	listenerThread = thread(&WebSocketServer::Listener);
}

void WebSocketServer::Listener()
{
	cout << "Start web socket listener" << endl;
	WebSocketServer& wss = WebSocketServer::GetInstance();
	onion_listen(wss.o);
}

void WebSocketServer::Shutdown(int signal)
{
	WebSocketServer& wss = WebSocketServer::GetInstance();
	onion_listen_stop(wss.o);
}

void WebSocketServer::Stop()
{
	Shutdown(0);
}

onion_connection_status WebSocketServer::OnConnect(void *data, onion_request *req, onion_response *res)
{
	onion_websocket *ws = onion_websocket_new(req, res);
	//if (!ws)
	//{
	//	onion_response_write0(res,
	//		"<html><body><h1>Easy echo</h1><pre id=\"chat\"></pre>"
	//	/*	" <script>\ninit = function(){\nmsg=document.getElementById('msg');\nmsg.focus();\n\nws=new WebSocket('ws://'+window.location.host);\nws.onmessage=function(ev){\n document.getElementById('chat').textContent+=ev.data+'\\n';\n};}\n"
	//		"window.addEventListener('load', init, false);\n</script>"*/
	//		"<input type=\"text\" id=\"msg\" onchange=\"javascript:ws.send(msg.value); msg.select(); msg.focus();\"/>\n"
	//		"</body></html>");

	//	return OCS_PROCESSED;
	//}

	onion_websocket_printf(ws, "Hello from server. Write something to echo it");
	onion_websocket_set_callback(ws, WebSocketServer::OnMessage);

	return OCS_WEBSOCKET;
}

onion_connection_status WebSocketServer::OnMessage(void *data, onion_websocket *ws, size_t data_ready_len)
{
	char tmp[256];
	if (data_ready_len>sizeof(tmp))
		data_ready_len = sizeof(tmp)-1;

	int len = onion_websocket_read(ws, tmp, data_ready_len);
	if (len == 0)
	{
		ONION_ERROR("Error reading data: %d: %s (%d)", errno, strerror(errno), data_ready_len);
		sleep(1);
	}
	tmp[len] = 0;
	onion_websocket_printf(ws, "Echo: %s", tmp);

	ONION_INFO("Read from websocket: %d: %s", len, tmp);

	return OCS_NEED_MORE_DATA;
}
