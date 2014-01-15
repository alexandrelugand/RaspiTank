#pragma once
#include "Singleton.h"
#include <thread>
#include <memory>
#include <atomic>
#include <queue>
#include <mutex>
#include "RTException.h"
#include "Command.h"

using namespace std;

namespace RaspiTank
{
	class Controller : public Singleton<Controller>
	{
		friend class Singleton<Controller>;

	protected:
		Controller();
		virtual ~Controller();

		thread cmdSenderThread;
		int  mem_fd;
		char *gpio_mem, *gpio_map;
		char *spi0_mem, *spi0_map;
		int frameInt;
		bool engineStarted;
		
		// I/O access
		volatile unsigned *gpio;

		void SetupIO();
		void SendCode(int code);
		void SendBit(int bit);

		atomic<bool> StopThread;
		queue<shared_ptr<Command> > cmdQueue;
		mutex queueLock;

		static void CommandSender(Controller* ctrl);
	
	public:
		void Initialize();
		void Dispose();

		void SetFrameInt(int interval) { frameInt = interval; }
		void AddCmd(Command* cmd, bool lock = true);
		void AddCmd(CmdType cmdtype, int repeat = 1, string msg = "", bool lock = true);
		void AddCmd(json_object* jobj, bool lock = true);

		void StartEngine();
		void StopEngine();
	};
}
