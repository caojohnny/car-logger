#ifndef OBD2_H
#define OBD2_H

#include <stdint.h>

enum Obd2Mode
{
	OBD2MODE_CURRENT_DATA = 0x01
};

enum Obd2Pid
{
	OBD2PID_PIDS_SUPPORTED_20 = 0x00,
	OBD2PID_ENGINE_SPEED = 0x0C,
	OBD2PID_VEHICLE_SPEED = 0x0D,
	OBD2PID_FUEL_TANK_LEVEL = 0x2F,
	OBD2PID_ENGINE_FUEL_RATE = 0x5E
};

struct Obd2Data
{
	enum Obd2Mode mode;
	enum Obd2Pid pid;
	uint8_t n_abcd;
	uint8_t abcd[4];
};

struct Obd2Data makeObd2Data(enum Obd2Mode mode, enum Obd2Pid pid);

void packObd2Data(uint8_t* packed, const struct Obd2Data* data, uint8_t packedLength);

#endif // OBD_H
