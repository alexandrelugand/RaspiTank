#pragma once
#include "Singleton.h"
#include "onion/onion.h"
#include <thread>

#define WS_BUFFER 2048

namespace RaspiTank
{
	class WebSocketServer : public Singleton<WebSocketServer>
	{
		friend class Singleton<WebSocketServer>;

	protected:
		WebSocketServer();
		virtual ~WebSocketServer();

		thread listenerThread;
		onion* o;
		onion_websocket* ws;
		static mutex wsLock;

		static void Listener();
		static void Shutdown(int signal);
		static onion_connection_status OnConnect(void *data, onion_request *req, onion_response *res);
		static onion_connection_status OnMessage(void *data, onion_websocket *ws, size_t data_ready_len);

	public:
		void Initialize();
		void Listen();
		void Stop();
		void Log(const string& log);
	};
}

