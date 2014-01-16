#include <iostream>
#include "Controller.h"
#include "RTException.h"
#include "Command.h"
#include "WebSocketServer.h"
using namespace std;
using namespace RaspiTank;

int main(int argc, char *argv[])
{
	cout << "RaspiTank Version 1.0.0" << endl;
	cout << "LUGAND Alexandre, January 2014" << endl;

	try
	{		
		Controller& ctrl = Controller::GetInstance();
		ctrl.Initialize();
		for (;;)
		{
			string strCmd, strCount;
			int cmd = 0, count = 0;

			printf("\t*** Enter command:\n");
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

			cmd = strtoul(strCmd.c_str(), NULL, 16);
			printf("\t*** Cmd: 0x%08X\n", cmd);
			printf("\t*** Enter frame count:\n");
			cin >> strCount;
			count = strtoul(strCount.c_str(), NULL, 10);
			ctrl.AddCmd(new Command(cmd, count));
		}
		
		ctrl.Dispose();
	}
	catch (RTException& ex)
	{
		cout << ex.what() << endl;
	}
	catch (exception& ex)
	{
		cout << ex.what() << endl;
	}
	catch (...)
	{
		cout << "[Unknown exception]\n" << endl;
	}
	
	return 0;
}



