//-----------------------------------------------------------------------------
/*

I2C Rotary Encoder V2 Driver
Author: Jason Harris (https://github.com/deadsy)

https://github.com/Fattoresaimon/I2CEncoderV2
https://www.kickstarter.com/projects/1351830006/i2c-encoder-v2

*/
//-----------------------------------------------------------------------------

#ifndef DEADSY_REI2C_H
#define DEADSY_REI2C_H

//-----------------------------------------------------------------------------
// registers

#define REI2C_GCONF      0x00	// General Configuration
#define REI2C_GP1CONF    0x01	// GP 1 Configuration
#define REI2C_GP2CONF    0x02	// GP 2 Configuration
#define REI2C_GP3CONF    0x03	// GP 3 Configuration
#define REI2C_INTCONF    0x04	// INT pin Configuration
#define REI2C_ESTATUS    0x05	// Encoder Status
#define REI2C_GPSTATUS   0x07	// GP Status
#define REI2C_CVAL(x)    (0x07 + (x))	// Counter Value
#define REI2C_CMAX(x)    (0x0B + (x))	// Counter Max value
#define REI2C_CMIN(x)    (0x0F + (x))	// Counter Min value
#define REI2C_ISTEP      (0x13 + (x))	// Increment step value
#define REI2C_RLED LED   0x17	// red color intensity
#define REI2C_GLED LED   0x18	// green color intensity
#define REI2C_BLED LED   0x19	// blue color intensity
#define REI2C_GP1REG     0x1A	// I/O GP1 register
#define REI2C_GP2REG     0x1B	// I/O GP2 register
#define REI2C_GP3REG     0x1C	// I/O GP3 register
#define REI2C_EEPROM(x)  (0x80 +(x))	// EEPROM memory

//-----------------------------------------------------------------------------

#define REI2C_I2C_TIMEOUT 30	// chibios ticks

//-----------------------------------------------------------------------------

// rotenc configuration
struct rei2c_cfg {
};

// rotenc state variables
struct rei2c_state {
	stkalign_t thd_wa[THD_WA_SIZE(512) / sizeof(stkalign_t)];	// thread working area
	Thread *thd;		// thread pointer
	const struct rei2c_cfg *cfg;	// driver configuration
	I2CDriver *dev;		// i2c bus driver
	i2caddr_t adr;		// i2c device address
	uint8_t *tx;		// i2c tx buffer
	uint8_t *rx;		// i2c rx buffer
};

//-----------------------------------------------------------------------------

#endif				// DEADSY_REI2C_H

//-----------------------------------------------------------------------------
