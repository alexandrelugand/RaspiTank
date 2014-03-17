#include "Controller.h"
#include <iostream>
#include <exception>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include "Utils.h"
#include <system_error>
#include "log.h"
#include "WebSocketServer.h"

using namespace std;
using namespace RaspiTank;

#define BCM2708_PERI_BASE        0x20000000
#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */

#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

// N.B. These have been reversed compared to Gert & Dom's original code!
// This is because the transistor board I use for talking to the Heng
// Long RX18 inverts the signal.  So the GPIO_SET pointer here actually
// sets the GPIO pin low - but that ends up as a high at the tank.
#define GPIO_CLR *(gpio+7)  // sets   bits which are 1, ignores bits which are 0
#define GPIO_SET *(gpio+10) // clears bits which are 1, ignores bits which are 0

// GPIO pin that connects to the Heng Long main board
// (Pin 7 is the top right pin on the Pi's GPIO, next to the yellow video-out)
//#define PIN 7
#define PIN 14


Controller::Controller()
{
	mem_fd = 0;
	gpio_mem = NULL;
	gpio_map = NULL;
	spi0_mem = NULL;
	spi0_map = NULL;
	gpio = NULL;
	frameInt = 4; //4 ms
	engineStarted = false;
}


Controller::~Controller()
{
}

void Controller::Initialize()
{
	INFO("Initialize controller...");

	cmdSenderThread = thread(&Controller::CommandSender, this);
	sched_param sch;
	int policy;
	pthread_getschedparam(cmdSenderThread.native_handle(), &policy, &sch);
	sch.sched_priority = 20;
	if (pthread_setschedparam(cmdSenderThread.native_handle(), SCHED_FIFO, &sch)) 
	{
		ERROR("Failed to increase Controller thread priority: %s", strerror(errno));
	}

	WebSocketServer& wss = WebSocketServer::GetInstance();
	wss.Initialize();
	wss.Listen();

	INFO("Controller initialized");
}

void Controller::SetupIO()
{
	INFO("Setup IO in progress...");

	/* open /dev/mem */
	if ((mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0)
		throw RTException("can't open /dev/mem");

	/* mmap GPIO */

	// Allocate MAP block
	if ((gpio_mem = (char*)malloc(BLOCK_SIZE + (PAGE_SIZE - 1))) == NULL)
		throw RTException("allocation error");

	// Make sure pointer is on 4K boundary
	if ((unsigned long)gpio_mem % PAGE_SIZE)
		gpio_mem += PAGE_SIZE - ((unsigned long)gpio_mem % PAGE_SIZE);

	// Now map it
	gpio_map = (char *)mmap(
		(caddr_t)gpio_mem,
		BLOCK_SIZE,
		PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_FIXED,
		mem_fd,
		GPIO_BASE
		);

	if ((long)gpio_map < 0)
		throw RTException(string_format("mmap error %d", (int)gpio_map));

	// Always use volatile pointer!
	gpio = (volatile unsigned *)gpio_map;

	// Switch the relevant GPIO pin to output mode
	INP_GPIO(PIN); // must use INP_GPIO before we can use OUT_GPIO
	OUT_GPIO(PIN);

	GPIO_CLR = 1 << PIN;

	INFO("Setup IO finished");
}

void Controller::Dispose()
{
	if (engineStarted)
		StopEngine();

	WebSocketServer::GetInstance().Stop();
	WebSocketServer::GetInstance().Kill();

	INFO("Dispose controller...");
	StopThread = true;
	if (cmdSenderThread.joinable())
		cmdSenderThread.join();
	INFO("Controller disposed");
	Kill();
}

void Controller::CommandSender(Controller* ctrl)
{		
	ctrl->SetupIO();

	while (!ctrl->StopThread)
	{
		int cmd = 0;
		int repeat = 1;
		//string msg;

		if (!ctrl->cmdQueue.empty())
		{
			try
			{
				ctrl->queueLock.lock();
				shared_ptr<Command> pCmd = ctrl->cmdQueue.front();
				ctrl->cmdQueue.pop();
				cmd = pCmd.get()->GetCmd();
				repeat = pCmd.get()->GetRepeat();
				//msg = pCmd.get()->GetMessage();
				if (pCmd.get()->IsEngineStart())
				{
					INFO("Starting engine ...");
					ctrl->engineStarted = true;
					WebSocketServer& wss = WebSocketServer::GetInstance();
					wss.SendMsg("ENGINE_STATUS", "start");
				}
				else if (pCmd.get()->IsEngineStop())
				{
					INFO("Stopping engine ...");
					ctrl->engineStarted = false;
					WebSocketServer& wss = WebSocketServer::GetInstance();
					wss.SendMsg("ENGINE_STATUS", "stop");
				}
			}
			catch (...) {}
			ctrl->queueLock.unlock();
		}
		else if (ctrl->engineStarted)
		{
			Command pCmd(CmdType::neutral);
			cmd = pCmd.GetCmd();				
			repeat = 1;
			//msg = "";
		}
		else
		{
			continue;
		}
				
		/*if (!msg.empty())
			INFO("Send command: %s", msg.c_str());*/

		for (int i = 0; i < repeat; i++)
			ctrl->SendCode(cmd);
	}
}

void Controller::SendCode(int code)
{
	// Send header "bit" (not a valid Manchester code)
	GPIO_SET = 1 << PIN;
	usleep(600);

	// Send the code itself, bit by bit using Manchester coding
	int i;
	for (i = 0; i<32; i++)
	{
		int bit = (code >> (31 - i)) & 0x1;
		SendBit(bit);
	}

	// Force a 4ms gap between messages
	GPIO_CLR = 1 << PIN;
	usleep(frameInt * 1000);
}

// Sends one individual bit using Manchester coding
// 1 = high-low, 0 = low-high
void Controller::SendBit(int bit)
{
	if (bit == 1)
	{
		GPIO_SET = 1 << PIN;
		usleep(300);
		GPIO_CLR = 1 << PIN;
		usleep(300);
	}
	else
	{
		GPIO_CLR = 1 << PIN;
		usleep(300);
		GPIO_SET = 1 << PIN;
		usleep(300);
	}
}

void Controller::StartEngine()
{
	queueLock.lock();
	AddCmd(CmdType::idle, 40, "", false);
	AddCmd(CmdType::ignition, 40, "", false);
	AddCmd(CmdType::neutral, 400, "Waiting for ignition...", false);
	AddCmd(CmdType::ignition, 1, "Ignition done", false);
	AddCmd(CmdType::engine_start, 10, "Warm up", false);
	AddCmd(CmdType::neutral, 20, "", false);
	queueLock.unlock();

	while (!engineStarted)
		usleep(10000);

	sleep(2);

	INFO("Engine started");
}

void Controller::StopEngine()
{
	queueLock.lock();
	AddCmd(CmdType::neutral, 40, "", false);
	AddCmd(CmdType::ignition, 20, "", false);
	AddCmd(CmdType::engine_stop, 1, "Stopping engine...", false);
	queueLock.unlock();

	while (engineStarted)
		usleep(10000);

	sleep(2);

	INFO("Engine stopped");
}

void Controller::AddCmd(Command* cmd, bool lock /*= true*/)
{
	shared_ptr<Command> pCmd(cmd);
	if (pCmd->IsEngineStart())
	{
		StartEngine();
		return;
	}

	if (pCmd->IsEngineStop())
	{
		StopEngine();
		return;
	}

	if (lock)
		queueLock.lock();
	
	cmdQueue.push(pCmd);

	if (lock)
		queueLock.unlock();
}

void Controller::AddCmd(CmdType cmdtype, int repeat/*=1*/, string msg/*=""*/, bool lock /*= true*/)
{
	shared_ptr<Command> pCmd(new Command(cmdtype, repeat, msg));

	if (lock)
		queueLock.lock();

	cmdQueue.push(pCmd);

	if (lock)
		queueLock.unlock();
}

void Controller::AddCmd(json_object* jobj, bool lock /*= true*/)
{
	shared_ptr<Command> pCmd(new Command(json_object_get(jobj)));
	if (pCmd->IsEngineStart())
	{
		StartEngine();
		return;
	}

	if (pCmd->IsEngineStop())
	{
		StopEngine();
		return;
	}

	if (lock)
		queueLock.lock();

	cmdQueue.push(pCmd);

	if (lock)
		queueLock.unlock();
}