#include "WebSocketServer.h"
#include "onion/onion.h"
#include "onion/websocket.h"
#include "log.h"
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <iostream>
#include "Command.h"
#include <memory>
#include "Controller.h"

using namespace std;
using namespace RaspiTank;

mutex WebSocketServer::wsLock;

WebSocketServer::WebSocketServer()
{
	o = NULL;
	ws = NULL;
}

WebSocketServer::~WebSocketServer()
{
}

void WebSocketServer::Initialize()
{
	INFO("Initialize web socket server...");

	signal(SIGINT, WebSocketServer::Shutdown);
	signal(SIGTERM, WebSocketServer::Shutdown);
		
	o = onion_new(O_THREADED);
	onion_set_port(o, "3000");
	onion_url *urls = onion_root_url(o);
	onion_url_add(urls, "", (void*)WebSocketServer::OnConnect);	

	INFO("web socket server initialized");
}

void WebSocketServer::Listen()
{
	listenerThread = thread(&WebSocketServer::Listener);
}

void WebSocketServer::Listener()
{
	INFO("Start web socket listener");
	WebSocketServer& wss = WebSocketServer::GetInstance();
	onion_listen(wss.o);
}

void WebSocketServer::Shutdown(int signal)
{
	WebSocketServer& wss = WebSocketServer::GetInstance();
	if (wss.ws != NULL)
	{
		onion_websocket_free(wss.ws);
		wss.ws = NULL;
	}
	onion_listen_stop(wss.o);
}

void WebSocketServer::Stop()
{
	Shutdown(0);
}

onion_connection_status WebSocketServer::OnConnect(void *data, onion_request *req, onion_response *res)
{
	WebSocketServer& wss = WebSocketServer::GetInstance();
	wss.ws = onion_websocket_new(req, res);
	if (!wss.ws)
		return OCS_PROCESSED;
	wss.Log("Connected to RaspiTank. Waiting command...");
	onion_websocket_set_callback(wss.ws, WebSocketServer::OnMessage);

	return OCS_WEBSOCKET;
}

void WebSocketServer::Log(const string& log)
{
	SendMsg("LOG", log);
}

void WebSocketServer::SendMsg(const string& action, const string& msg)
{
	if (ws != NULL)
	{
		wsLock.lock();
		string strMsg = "$" + action + "$_" + msg;
		onion_websocket_printf(ws, strMsg.c_str());
		wsLock.unlock();
	}
}

onion_connection_status WebSocketServer::OnMessage(void *data, onion_websocket *ws, size_t data_ready_len)
{
	if (data_ready_len > 0)
	{
		json_object* jobj = NULL;
		string cmd;

		wsLock.lock();

		char* tmp = new char[data_ready_len];
		int len = onion_websocket_read(ws, tmp, data_ready_len);
		if (len > 0)
		{
			tmp[len] = 0;
			jobj = json_tokener_parse(tmp);			
			cmd = string(tmp);
		}
		delete[] tmp;

		wsLock.unlock();

		if (len <= 0)
			ERROR("Error reading data: %d: %s (%d)", errno, strerror(errno), data_ready_len);
		else
			INFO("Read from websocket: %d:\n%s", len, cmd.c_str());

		if (jobj != NULL)
		{
			Controller& ctrl = Controller::GetInstance();
			ctrl.AddCmd(jobj);
			json_object_put(jobj); //Delete the object
		}
	}
	return OCS_NEED_MORE_DATA;
}
