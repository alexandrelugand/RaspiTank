#include "Command.h"
#include "log.h"
#include "Utils.h"

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

Command::Command(CmdType cmdtype, int repeat)
{
	Init(cmdtype, repeat);
}

Command::Command(CmdType cmdtype, int repeat, string msg)
{
	Init(cmdtype, repeat, msg);
}

Command::Command(int cmdCode)
{
	Init(CmdType::idle);
	cmd = cmdCode;
}

Command::Command(int cmdCode, int repeat)
{
	Init(CmdType::idle, repeat);
	cmd = cmdCode;
}

Command::Command(int cmdCode, int repeat, string msg)
{
	Init(CmdType::idle, repeat, msg);
	cmd = cmdCode;
}

Command::Command(json_object* jobj)
{
	enum json_type type;
	json_object_object_foreach(jobj, key, val)
	{
		string str = string_format("type: ");
		type = json_object_get_type(val);
		switch (type)
		{
		case json_type_null: str += "json_type_null";
			break;
		case json_type_boolean: str += "json_type_boolean";
			break;
		case json_type_double: str += "json_type_double";
			break;
		case json_type_int: str += "json_type_int";
			break;
		case json_type_object: str += "json_type_object";
			break;
		case json_type_array: str += "json_type_array";
			break;
		case json_type_string: str += "json_type_string";
			break;
		}
		INFO(str.c_str());
	}
}

Command::~Command()
{
}

void Command::Init(CmdType cmdtype /*= CmdType::neutral*/, int rep /*= 1*/, string msg /*= ""*/)
{
	idle = false;
	ignition = false;
	neutral = false;
	repeat = rep;
	direction = Direction::Backward;
	dirspeed = Speed::VerySlow;
	rotation = Rotation::None;
	rotspeed = Speed::VerySlow;
	turrelRotation = Rotation::None;
	canonElevation = false;
	fire = false;
	gun = false;
	engineStart = false;
	engineStop = false;

	switch (cmdtype)
	{
	case RaspiTank::idle:
		idle = true;
		message = "Idle";
		break;
	case RaspiTank::ignition:
		ignition = true;
		message = "Ignition";
		break;
	case RaspiTank::neutral:
		neutral = true;
		message = "Neutral";
		break;
	case RaspiTank::left_slow:
		SetRotation(Rotation::Left, Speed::Slow);
		message = "Left slow";
		break;
	case RaspiTank::left_fast:
		SetRotation(Rotation::Left, Speed::Fast);
		message = "Left fast";
		break;
	case RaspiTank::right_slow:
		SetRotation(Rotation::Right, Speed::Slow);
		message = "Right slow";
		break;
	case RaspiTank::right_fast:
		SetRotation(Rotation::Right, Speed::Fast);
		message = "Right fast";
		break;
	case RaspiTank::fwd_slow:
		SetDirection(Direction::Forward, Speed::Slow);
		message = "Forward slow";
		break;
	case RaspiTank::fwd_fast:
		SetDirection(Direction::Forward, Speed::Fast);
		message = "Forward fast";
		break;
	case RaspiTank::back_slow:
		SetDirection(Direction::Backward, Speed::Slow);
		message = "Backward slow";
		break;
	case RaspiTank::back_fast:
		SetDirection(Direction::Backward, Speed::Fast);
		message = "Backward fast";
		break;
	case RaspiTank::turret_left:
		SetTurrelRotation(Rotation::Left);
		message = "Turret left";
		break;
	case RaspiTank::turret_right:
		SetTurrelRotation(Rotation::Right);
		message = "Turret right";
		break;
	case RaspiTank::canon_elev:
		CanonElevation();
		message = "Canon elevation";
		break;
	case RaspiTank::machine_gun:
		Gun();
		message = "Machine gun";
		break;
	case RaspiTank::fire:
		Fire();
		message = "Fire";
		break;
	case RaspiTank::engine_start:
		engineStart = true;
		neutral = true;
		message = "Engine started";
		break;
	case RaspiTank::engine_stop:
		engineStop = true;
		idle = true;
		message = "Engine stopped";
		break;
	default:
		break;
	}

	cmd = UNASSIGNED_CMD; //Préambule et postambule, CRC à 0
	if (!msg.empty())
		message = msg;
}

const int Command::GetCmd()
{
	if (cmd != UNASSIGNED_CMD)
	{
		CRC(cmd);
		return cmd;
	}

	if (idle) //Dans ce cas de figure, le controleur démarre une procédure spéciale
		return IdleCmd;

	if (ignition) //Dans ce cas de figure, le controleur démarre une procédure spéciale
		return IgnitionCmd;

	if (neutral)
		return NeutralCmd;

	//Calcul de la direction
	if (direction == Direction::Backward)
		cmd |= 0x00400000; //Lève le bit 22 à 1

	//Calcul la vitesse de déplacement
	cmd |= (((direction == Direction::Forward) ? ~dirspeed : dirspeed) & 0x00000007) << 19; //Positionne les bits 21 à 19

	//Doit-on tirer avec le canon ?
	if (fire)
		cmd |= 0x00020000; //Lève le bit 17 à 1

	//Doit-on tourner la tourelle ?
	switch (turrelRotation)
	{
	case Rotation::Right:
		cmd |= 0x00010000; //Lève le bit 16 à 1
		break;
	case Rotation::Left:
		cmd |= 0x00008000; //Lève le bit 15 à 1
		break;		
	default:
		break;
	}

	//Doit-on incliner le canon ?
	if (canonElevation)
		cmd |= 0x00004000; //Lève le bit 14 à 1

	//Calcul de la direction de rotation
	if (rotation == Rotation::Right)
		cmd |= 0x00001000; //Lève le bit 12 à 1

	//Calcul la vitesse de rotation
	if (rotation != Rotation::None)
		cmd |= (((rotation == Rotation::Left) ? ~rotspeed : rotspeed) & 0x00000007) << 8; //Positionne les bits 11 à 8
	else
		cmd |= 0x00000F00; //Positionne le bit 12 à 0 et les bits 11 à 8 à 1

	//Doit-on tirer avec la mitrailleuse ?
	if (gun)
		cmd |= 0x00000040; //Lève le bit 6 à 1

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