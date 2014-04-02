#include "Command.h"
#include "log.h"
#include "Utils.h"
#include <algorithm>
#include <string> 
#include <sys/time.h>

using namespace std;
using namespace RaspiTank;

int Command::IdleCmd = 0xFE40121C;
int Command::IgnitionCmd = 0xFE401294;
int Command::NeutralCmd = 0xFE3C0F00;

Command::Command()
{
	Init();
}

Command::Command(CmdType cmdtype)
{
	Init(cmdtype);
}

Command::Command(CmdType cmdtype, string msg)
{
	Init(cmdtype, msg);
}

Command::Command(int cmdCode)
{
	Init(CmdType::idle);
	cmd = cmdCode;
}

Command::Command(int cmdCode, string msg)
{
	Init(CmdType::idle, msg);
	cmd = cmdCode;
}

Command::Command(json_object* jobj)
{		
	ParseJSON(json_object_object_get(jobj, "Command"));
}

Command::~Command()
{
}

void Command::Init(CmdType cmdtype /*= CmdType::neutral*/, string msg /*= ""*/)
{
	idle = false;
	ignition = false;
	neutral = false;
	direction = Direction::Backward;
	dirspeed = Speed::VerySlow;
	rotation = Rotation::None;
	rotspeed = Speed::VerySlow;
	turrelRotation = Rotation::None;
	canonElevation = false;
	fire = false;
	gun = false;
	recoil = false;
	engineStart = false;
	engineStop = false;
	external = false;	
	gettimeofday(&timestamp, NULL);

	switch (cmdtype)
	{
	case RaspiTank::idle:
		idle = true;
		break;
	case RaspiTank::ignition:
		ignition = true;
		break;
	case RaspiTank::neutral:
		neutral = true;
		break;
	case RaspiTank::left_slow:
		SetRotation(Rotation::Left, Speed::Slow);
		break;
	case RaspiTank::left_fast:
		SetRotation(Rotation::Left, Speed::Fast);
		break;
	case RaspiTank::right_slow:
		SetRotation(Rotation::Right, Speed::Slow);
		break;
	case RaspiTank::right_fast:
		SetRotation(Rotation::Right, Speed::Fast);
		break;
	case RaspiTank::fwd_slow:
		SetDirection(Direction::Forward, Speed::Slow);
		break;
	case RaspiTank::fwd_fast:
		SetDirection(Direction::Forward, Speed::Fast);
		break;
	case RaspiTank::back_slow:
		SetDirection(Direction::Backward, Speed::Slow);
		break;
	case RaspiTank::back_fast:
		SetDirection(Direction::Backward, Speed::Fast);
		break;
	case RaspiTank::turret_left:
		SetTurrelRotation(Rotation::Left);
		break;
	case RaspiTank::turret_right:
		SetTurrelRotation(Rotation::Right);
		break;
	case RaspiTank::canon_elev:
		CanonElevation();
		break;
	case RaspiTank::machine_gun:
		Gun();
		break;
	case RaspiTank::fire:
		Fire();
		break;
	case RaspiTank::engine_start:
		engineStart = true;
		neutral = true;
		break;
	case RaspiTank::engine_stop:
		engineStop = true;
		idle = true;
		break;
	case RaspiTank::recoil:
		Recoil();
		break;
	default:
		break;
	}

	cmd = UNASSIGNED_CMD; //Préambule et postambule, CRC à 0
}

void Command::ParseJSON(json_object* jobj)
{
	if (jobj != NULL)
	{
		json_object_object_foreach(jobj, key, val)
		{
			string strAttr(key);
			transform(strAttr.begin(), strAttr.end(), strAttr.begin(), ::tolower);
			if (strAttr.compare("idle") == 0)
			{
				idle = json_object_get_boolean(val) != 0;
			}
			else if (strAttr.compare("ignition") == 0)
			{
				ignition = json_object_get_boolean(val) != 0;
			}
			else if (strAttr.compare("neutral") == 0)
			{
				neutral = json_object_get_boolean(val) != 0;
			}
			else if (strAttr.compare("direction") == 0)
			{
				direction = (Direction)json_object_get_int(val);
			}
			else if (strAttr.compare("dirspeed") == 0)
			{
				dirspeed = (Speed)json_object_get_int(val);
			}
			else if (strAttr.compare("rotation") == 0)
			{
				rotation = (Rotation)json_object_get_int(val);
			}
			else if (strAttr.compare("rotspeed") == 0)
			{
				rotspeed = (Speed)json_object_get_int(val);
			}
			else if (strAttr.compare("turrelrotation") == 0)
			{
				turrelRotation = (Rotation)json_object_get_int(val);
			}
			else if (strAttr.compare("canonelevation") == 0)
			{
				canonElevation = json_object_get_boolean(val) != 0;
			}
			else if (strAttr.compare("fire") == 0)
			{
				fire = json_object_get_boolean(val) != 0;
			}
			else if (strAttr.compare("gun") == 0)
			{
				gun = json_object_get_boolean(val) != 0;
			}
			else if (strAttr.compare("enginestart") == 0)
			{
				engineStart = json_object_get_boolean(val) != 0;
				if (engineStart)
					neutral = true;
			}
			else if (strAttr.compare("enginestop") == 0)
			{
				engineStop = json_object_get_boolean(val) != 0;
				if (engineStop)
					idle = true;
			}
			else if (strAttr.compare("recoil") == 0)
			{
				recoil = json_object_get_boolean(val) != 0;
			}
		}
	}
	external = true;
	gettimeofday(&timestamp, NULL);
	cmd = UNASSIGNED_CMD; //Préambule et postambule, CRC à 0
}

const int Command::GetCmd()
{
	string msg;
	if (cmd != UNASSIGNED_CMD)
	{
		CRC(cmd);
		return cmd;
	}

	if (idle) //Dans ce cas de figure, le controleur démarre une procédure spéciale
	{
		return IdleCmd;
	}

	if (ignition) //Dans ce cas de figure, le controleur démarre une procédure spéciale
	{
		msg += "Ignition / ";
		return IgnitionCmd;
	}

	if (neutral)
	{
		msg += "Neutral / ";
		return NeutralCmd;
	}

	//Calcul de la direction
	if (direction == Direction::Backward)
	{
		if (dirspeed > 0)
			msg += string_format("Direction: Backward / Dir Speed: %d / ", dirspeed);
		cmd |= 0x00400000; //Lève le bit 22 à 1
	}
	else
	{
		if (dirspeed > 0)
			msg += string_format("Direction: Forward / Dir Speed: %d / ", dirspeed);
	}
		

	//Calcul la vitesse de déplacement
	cmd |= (((direction == Direction::Forward) ? ~dirspeed : dirspeed) & 0x00000007) << 19; //Positionne les bits 21 à 19
	
	//Doit-on tirer avec le canon ?
	if (fire)
	{
		msg += "Fire / ";
		cmd |= 0x00020000; //Lève le bit 17 à 1
	}

	//Doit-on tourner la tourelle ?
	switch (turrelRotation)
	{
	case Rotation::Right:
		msg += "Turrel Rotation: Right / ";
		cmd |= 0x00010000; //Lève le bit 16 à 1
		break;
	case Rotation::Left:
		msg += "Turrel Rotation: Left / ";
		cmd |= 0x00008000; //Lève le bit 15 à 1
		break;		
	default:
		break;
	}

	//Doit-on incliner le canon ?
	if (canonElevation)
	{
		msg += "Canon Elev / ";
		cmd |= 0x00004000; //Lève le bit 14 à 1
	}

	//Calcul de la direction de rotation
	if (rotation == Rotation::Right)
		cmd |= 0x00001000; //Lève le bit 12 à 1

	//Calcul la vitesse de rotation
	if (rotation == Rotation::Left)
	{
		if (rotspeed > 0)
			msg += string_format("Rotation: Left / Rot Speed: %d / ", rotspeed);
		cmd |= (~rotspeed & 0x00000007) << 8; //Positionne les bits 11 à 8
	}
	else if (rotation == Rotation::Right)
	{
		if (rotspeed > 0)
			msg += string_format("Rotation: Right / Rot Speed: %d / ", rotspeed);
		cmd |= ((rotspeed & 0x00000007) << 8) + 0x00000700; //Positionne les bits 11 à 8
	}
	else
	{
		cmd |= 0x00000F00; //Positionne le bit 12 à 0 et les bits 11 à 8 à 1
	}

	//Doit-on tirer avec la mitrailleuse ?
	if (gun)
	{
		msg += "Gun / ";
		cmd |= 0x00000040; //Lève le bit 6 à 1
	}

	if (recoil)
	{
		msg += "Recoil / ";
		cmd |= 0x00002000; //Lève le bit 13 à 1
	}

	if (!msg.empty())
	{
		msg = msg.substr(0, msg.length() - 3);
		INFO(msg.c_str());
	}

	CRC(cmd);

	return cmd;
}

void Command::CRC(int& cmd)
{
	unsigned char crc = 0x00;

	//XOR bits à bits par mot de 4 bits;
	int word = cmd & 0x00F00000;
	crc ^= word >> 20;

	word = cmd & 0x000F0000;
	crc ^= word >> 16;

	word = cmd & 0x0000F000;
	crc ^= word >> 12;

	word = cmd & 0x00000F00;
	crc ^= word >> 8;

	//Le dernier groupe de 4 bits est constitué de deux derniers bits de la trame en poids faible
	word = cmd & 0x000000C0;
	crc ^= word >> 6;

	//Replacement sur les bits 2 à 5 de la trame
	crc = crc << 2;

	//Place le CRC dans la trame
	cmd = cmd | crc;
}