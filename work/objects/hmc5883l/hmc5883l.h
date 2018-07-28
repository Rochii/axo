//-----------------------------------------------------------------------------
/*

HMC5883L - 3 axis digital compass
Author: Jason Harris (https://github.com/deadsy)
http://www.sparkfun.com/products/10724
http://www.farnell.com/datasheets/1683374.pdf

*/
//-----------------------------------------------------------------------------

#ifndef DEADSY_HMC5883L_H
#define DEADSY_HMC5883L_H

//-----------------------------------------------------------------------------
// registers

#define I2C_ADR_HMC5883L 0x1e

#define CFG_REG_A    0x0	// R/W Configuration Register A
#define CFG_REG_B    0x1	// R/W Configuration Register B
#define MODE_REG     0x2	// R/W Mode Register
#define DOUT_X_MSB   0x3	// R   Data Output X MSB Register
#define DOUT_X_LSB   0x4	// R   Data Output X LSB Register
#define DOUT_Z_MSB   0x5	// R   Data Output Z MSB Register
#define DOUT_Z_LSB   0x6	// R   Data Output Z LSB Register
#define DOUT_Y_MSB   0x7	// R   Data Output Y MSB Register
#define DOUT_Y_LSB   0x8	// R   Data Output Y LSB Register
#define STATUS_REG   0x9	// R   Status Register
#define ID_REG_A     0xa	// R   Identification Register A
#define ID_REG_B     0xb	// R   Identification Register B
#define ID_REG_C     0xc	// R   Identification Register C

#define COMPASS_RATE_0_75   0
#define COMPASS_RATE_1_5    1
#define COMPASS_RATE_3      2
#define COMPASS_RATE_7_5    3
#define COMPASS_RATE_15     4
#define COMPASS_RATE_30     5
#define COMPASS_RATE_75     6

//-----------------------------------------------------------------------------

// read/write a device register
static uint8_t compass_reg_rd(uint8_t reg) {
	return i2c_reg_rd(I2C_ADR_HMC5883L, reg);
}
static void compass_reg_wr(uint8_t reg, uint8_t val) {
	i2c_reg_wr(I2C_ADR_HMC5883L, reg, val);
}

// read the compass xyz data
int compass_rd(IMU_RAW * ptr) {
	// read 6 bytes starting at the DOUT_X_MSB register.
	uint8_t reg = DOUT_X_MSB;
	uint8_t data[6];

	if (i2c_write(I2C_ADR_HMC5883L, &reg, 1) != 0) {
		return -1;
	}
	if (i2c_read(I2C_ADR_HMC5883L, data, sizeof(data)) != 0) {
		return -1;
	}
	// pack the i2c data into the imu raw structure
	uint8_t *raw = (uint8_t *) ptr;
	raw[0] = data[1];	// x
	raw[1] = data[0];
	raw[2] = data[5];	// y
	raw[3] = data[4];
	raw[4] = data[3];	// z
	raw[5] = data[2];

	return 0;
}

int compass_init(void) {
	// do we have a device on the bus?
	uint8_t tmp;
	if (i2c_read(I2C_ADR_HMC5883L, &tmp, 1) != 0) {
		return -1;
	}
	// read and check the device id
	if ((compass_reg_rd(ID_REG_A) != 'H') || (compass_reg_rd(ID_REG_B) != '4') || (compass_reg_rd(ID_REG_C) != '3')) {
		return -1;
	}

	compass_reg_wr(CFG_REG_A, (COMPASS_RATE_15 << 2 /*Hz */ ));
	compass_reg_wr(CFG_REG_B, (1 << 5 /*gain */ ));
	compass_reg_wr(MODE_REG, (0 << 0 /*continuous mode */ ));

	return 0;
}

// return non-zero if a new compass sample is available
int compass_poll(void) {
	// todo: this isn't right, the datasheet description of this
	// register appears to be incorrect.
	return compass_reg_rd(STATUS_REG);
}

//-----------------------------------------------------------------------------

#endif				// DEADSY_HMC5883L_H

//-----------------------------------------------------------------------------
