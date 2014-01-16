#pragma once
#include <string>
#include <json/json.h>

using namespace std;

#define UNASSIGNED_CMD 0xFE000000

namespace RaspiTank
{
	enum Direction
	{
		Forward = 0,
		Backward = 1
	};

	enum Rotation : int
	{
		None = 0,
		Left = 1,
		Right = 2		
	};

	enum Speed : int
	{
		VerySlow = 0,
		Slow = 1,
		MediumSlow = 2,
		Intermidiate = 3,
		Medium = 4,
		MediumFast = 5,
		Fast = 6,
		VeryFast = 7
	};

	enum CmdType : int
	{
		idle = 0x00000000,
		ignition = 0x00000001,
		neutral = 0x00000002,
		left_slow = 0x00000004,
		left_fast = 0x00000008,
		right_slow = 0x00000010,
		right_fast = 0x00000020,
		fwd_slow = 0x00000040,
		fwd_fast = 0x00000080,
		back_slow = 0x00000100,
		back_fast = 0x00000200,
		turret_left = 0x00000400,
		turret_right = 0x00000800,
		canon_elev = 0x00001000,
		machine_gun = 0x00002000,
		fire = 0x00004000,
		engine_start = 0x00008000,
		engine_stop = 0x00010000
	};

	class Command
	{
	protected:
		bool idle;
		bool ignition;
		bool neutral;
		unsigned int repeat;
		Direction direction;
		Speed dirspeed;
		Rotation rotation;
		Speed rotspeed;
		Rotation turrelRotation;
		bool canonElevation;
		bool fire;
		bool gun;
		int cmd;
		string message;
		bool engineStart;
		bool engineStop;

		void CRC(int& cmd);

		static int IdleCmd;
		static int IgnitionCmd;
		static int NeutralCmd;

		void Init(CmdType cmdtype = CmdType::neutral, int rep = 1, string msg = "");

	public:
		Command();
		Command(CmdType cmdtype);
		Command(CmdType cmdtype, int repeat);
		Command(CmdType cmdtype, int repeat, string msg);
		Command(int cmdCode);
		Command(int cmd, int repeat);
		Command(int cmdCode, int repeat, string msg);
		Command(json_object* jobj);
		virtual ~Command();
		
		void Idle() { idle = true; }
		void Ignition() { ignition = true; }

		void SetRepeat(unsigned int count) { repeat = count; }
		const int GetRepeat() { return repeat;  }
		void SetDirection(Direction dir, Speed speed = Intermidiate) { direction = dir; if (speed < VerySlow) speed = VerySlow; if (speed > VeryFast) speed = VeryFast; dirspeed = speed; }
		void SetRotation(Rotation rot, Speed speed = Intermidiate) { rotation = rot; if (speed < VerySlow) speed = VerySlow; if (speed > VeryFast) speed = VeryFast; rotspeed = speed; }
		void SetTurrelRotation(Rotation rot) { turrelRotation = rot; }
		
		void CanonElevation() { canonElevation = true; }
		void Fire() { fire = true; }
		void Gun() { gun = true; }

		const int GetCmd();
		const string& GetMessage() { return message; }

		const bool IsEngineStart() { return engineStart; }
		const bool IsEngineStop() { return engineStop; }
	};
}

