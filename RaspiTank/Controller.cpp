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
	cout << "Initialize controller..." << endl;

	cmdSenderThread = thread(&Controller::CommandSender, this);
	sched_param sch;
	int policy;
	pthread_getschedparam(cmdSenderThread.native_handle(), &policy, &sch);
	sch.sched_priority = 20;
	if (pthread_setschedparam(cmdSenderThread.native_handle(), SCHED_FIFO, &sch)) 
	{
		cout << "Failed to setschedparam: " << strerror(errno) << endl;
	}

	WebSocketServer& wss = WebSocketServer::GetInstance();
	wss.Initialize();
	wss.Listen();

	cout << "Controller initialized" << endl;
}

void Controller::SetupIO()
{
	cout << "Setup IO in progress..." << endl;

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

	cout << "Setup IO finished" << endl;
}

void Controller::Dispose()
{
	if (engineStarted)
		StopEngine();

	WebSocketServer::GetInstance().Stop();
	WebSocketServer::GetInstance().Kill();

	cout << "Dispose controller..." << endl;
	StopThread = true;
	if (cmdSenderThread.joinable())
		cmdSenderThread.join();
	cout << "Controller disposed" << endl;
	Kill();
}

void Controller::CommandSender(Controller* ctrl)
{		
	ctrl->SetupIO();

	while (!ctrl->StopThread)
	{
		int cmd = 0;
		int repeat = 1;
		string msg;

		if (!ctrl->cmdQueue.empty())
		{
			try
			{
				ctrl->queueLock.lock();
				shared_ptr<Command> pCmd = ctrl->cmdQueue.front();
				ctrl->cmdQueue.pop();
				cmd = pCmd.get()->GetCmd();
				repeat = pCmd.get()->GetRepeat();
				msg = pCmd.get()->GetMessage();
				if (pCmd.get()->IsEngineStart())
					ctrl->engineStarted = true;
				else if(pCmd.get()->IsEngineStop())
					ctrl->engineStarted = false;
			}
			catch (...) {}
			ctrl->queueLock.unlock();
		}
		else if (ctrl->engineStarted)
		{
			Command pCmd(CmdType::neutral);
			cmd = pCmd.GetCmd();				
			repeat = 1;
			msg = "";
		}
		else
		{
			continue;
			/*Command pCmd(CmdType::idle);
			cmd = pCmd.GetCmd();				
			repeat = 1;
			msg = "";*/
		}
				
		if (!msg.empty())
			cout << msg << endl;

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
	AddCmd(new Command(CmdType::idle, 40));
	AddCmd(new Command(CmdType::ignition, 40));
	AddCmd(new Command(CmdType::neutral, 400, "Waiting for ignition..."));
	AddCmd(new Command(CmdType::ignition, 1, "Ignition done"));
	AddCmd(new Command(CmdType::engine_start , 10, "Warm up"));
	queueLock.unlock();
//	AddCmd(new Command(CmdType::neutral, 100, "Warm up"));
//	sleep(10);		
//	AddCmd(new Command(CmdType::neutral, 1, "Engine started"));		
	while (!engineStarted)
		usleep(10000);

	sleep(2);

	cout << "Engine started" << endl;
}

void Controller::StopEngine()
{
	queueLock.lock();
	AddCmd(new Command(CmdType::neutral, 40));
	AddCmd(new Command(CmdType::ignition, 5));		
	AddCmd(new Command(CmdType::engine_stop, 1, "Stopping engine..."));
	queueLock.unlock();

	while (engineStarted)
		usleep(10000);

	sleep(2);

	//AddCmd(new Command(CmdType::idle, 40));

	cout << "Engine stopped" << endl;
}

void Controller::AddCmd(Command* cmd)
{
	shared_ptr<Command> pCmd(cmd);
	try
	{
		//queueLock.lock();
		cmdQueue.push(pCmd);
	}
	catch (...) {}
	//queueLock.unlock();
}
