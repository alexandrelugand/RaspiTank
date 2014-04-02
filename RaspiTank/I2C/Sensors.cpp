#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "Sensors.h"
#include "MPU6050.h"
#include "HMC5883L.h"
#include "../log.h"

MPU6050 mpu6050;
HMC5883L hmc5883l;
heading_t heading;

// this method is just used to collect different setSlave operations
void setSlaveControl(uint8_t slaveID){
	mpu6050.setSlaveEnabled(slaveID, true);
	mpu6050.setSlaveWordByteSwap(slaveID, false);
	mpu6050.setSlaveWriteMode(slaveID, false);
	mpu6050.setSlaveWordGroupOffset(slaveID, false);
	mpu6050.setSlaveDataLength(slaveID, 2);
}

void SetupSensors() {

	// initialize device
	INFO("Initializing I2C devices...");
	mpu6050.initialize();
	if (mpu6050.testConnection()){
		INFO("MPU6050 connection successful");
	}
	else {
		INFO("MPU6050 connection failed");
	}
	
	// configuration of the compass module
	// activate the I2C bypass to directly access the Sub I2C 
	mpu6050.setI2CMasterModeEnabled(0);
	mpu6050.setI2CBypassEnabled(1);

	if (hmc5883l.testConnection()) {
		INFO("HMC5883l connection successful");
		hmc5883l.initialize();		
		hmc5883l.SelfTest();
		

/*		// unfourtunally 
		// hmc5883l.setMode(HMC5883L_MODE_CONTINUOUS); 
		// does not work correctly. I used the following command to 
		// "manually" switch on continouse measurements
		I2Cdev::writeByte(HMC5883L_DEFAULT_ADDRESS,
			HMC5883L_RA_MODE,
			HMC5883L_MODE_CONTINUOUS);

		// the HMC5883l is configured now, we switch back to the MPU 6050
		mpu6050.setI2CBypassEnabled(0);

		// X axis word
		mpu6050.setSlaveAddress(0, HMC5883L_DEFAULT_ADDRESS | 0x80);
		mpu6050.setSlaveRegister(0, HMC5883L_RA_DATAX_H);
		setSlaveControl(0);

		// Y axis word
		mpu6050.setSlaveAddress(1, HMC5883L_DEFAULT_ADDRESS | 0x80);
		mpu6050.setSlaveRegister(1, HMC5883L_RA_DATAY_H);
		setSlaveControl(1);

		// Z axis word
		mpu6050.setSlaveAddress(2, HMC5883L_DEFAULT_ADDRESS | 0x80);
		mpu6050.setSlaveRegister(2, HMC5883L_RA_DATAZ_H);
		setSlaveControl(2);

		mpu6050.setI2CMasterModeEnabled(1);*/
	}
	else {
		INFO("HMC5883l connection failed");
	}

	// activate temperature MPU 6050 sensor
	mpu6050.setTempSensorEnabled(true);
}

void ReadAcceleration(int16_t* ax, int16_t* ay, int16_t* az) {
	mpu6050.getAcceleration(ax, ay, az);
}

void ReadGyroscope(int16_t* gx, int16_t* gy, int16_t* gz) {
	mpu6050.getRotation(gx, gy, gz);
}

void ReadTemp(double* temp)
{
	// see MPU 6050 datasheet page 31 of 47
	*temp = ((double)mpu6050.getTemperature()) / 340.0 + 36.53;
}

void ReadCompas(double* head, double* mx, double* my, double* mz) {

	// read raw heading measurements from device
	/**mx = mpu6050.getExternalSensorWord(0);
	*my = mpu6050.getExternalSensorWord(2);*/

	hmc5883l.getHeading(&heading);
	//hmc5883l.compensate(&heading);
	
	//*mz = mpu6050.getExternalSensorWord(4);

	//*mx = *mx / 442;
	//*my = *my / 442;

	*mx = heading.X;
	*my = heading.Y;
	*mz = heading.Z;
	// To calculate heading in degrees. 0 degree indicates North
	*head = atan2(heading.X, heading.Y);
	if (heading.X < 0)
		*head += 2 * M_PI;
}
