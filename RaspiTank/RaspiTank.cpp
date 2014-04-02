#include <iostream>
#include "Controller.h"
#include "RTException.h"
#include "Command.h"
#include "WebSocketServer.h"
#include "log.h"

using namespace std;
using namespace RaspiTank;

int main(int argc, char *argv[])
{
	WebSocketServer& wss = WebSocketServer::GetInstance();

	INFO("RaspiTank Version 1.0.0");
	INFO("LUGAND Alexandre, January 2014");

	try
	{		
		Controller& ctrl = Controller::GetInstance();
		ctrl.Initialize();
		for (;;)
		{
			string strCmd, strCount;
			CmdType cmd = CmdType::neutral;
			int count = 0;

			WARNING("Wait command:");
			cin >> strCmd;
			if (strCmd.compare("exit") == 0)
				break;

			if (strCmd.compare("start") == 0)
			{
				ctrl.StartEngine();
				continue;
			}

			if (strCmd.compare("stop") == 0)
			{
				ctrl.StopEngine();
				continue;
			}

			cmd = (CmdType)strtoul(strCmd.c_str(), NULL, 16);
			WARNING("Process command: 0x%08X", cmd);
			WARNING("Enter frame count:");
			cin >> strCount;
			count = strtoul(strCount.c_str(), NULL, 10);
			ctrl.AddCmd(cmd, count);
		}
		
		ctrl.Dispose();
	}
	catch (RTException& ex)
	{
		ERROR("RaspiTank exception: %s", ex.what());
	}
	catch (exception& ex)
	{
		ERROR("Exception: %s", ex.what());
	}
	catch (...)
	{
		ERROR("[Unknown exception]");
	}
	
	return 0;
}



