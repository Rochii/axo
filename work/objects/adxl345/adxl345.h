//-----------------------------------------------------------------------------
/*

ADXL345 - 3 axis accelerometer
Author: Jason Harris (https://github.com/deadsy)
http://www.sparkfun.com/products/10724
http://www.analog.com/media/en/technical-documentation/data-sheets/ADXL345.pdf

*/
//-----------------------------------------------------------------------------

#ifndef DEADSY_ADXL345_H
#define DEADSY_ADXL345_H

//-----------------------------------------------------------------------------
// registers

#define I2C_ADR_ADXL345 0x53

#define DEVID             0x00	// R   Device ID
#define TAP_THRESH        0x1D	// R/W Tap threshold
#define OFSX              0x1E	// R/W X-axis offset
#define OFSY              0x1F	// R/W Y-axis offset
#define OFSZ              0x20	// R/W Z-axis offset
#define TAP_DUR           0x21	// R/W Tap duration
#define TAP_LATENT        0x22	// R/W Tap latency
#define TAP_WINDOW        0x23	// R/W Tap window
#define THRESH_ACT        0x24	// R/W Activity threshold
#define THRESH_INACT      0x25	// R/W Inactivity threshold
#define TIME_INACT        0x26	// R/W Inactivity time
#define ACT_INACT_CTL     0x27	// R/W Axis enable control for activity and inactivity detection
#define THRESH_FF         0x28	// R/W Free-fall threshold
#define TIME_FF           0x29	// R/W Free-fall time
#define TAP_AXES          0x2A	// R/W Axis control for tap/double tap
#define ACT_TAP_STATUS    0x2B	// R   Source of tap/double tap
#define BW_RATE           0x2C	// R/W Data rate and power mode control
#define POWER_CTL         0x2D	// R/W Power-saving features control
#define INT_ENABLE        0x2E	// R/W Interrupt enable control
#define INT_MAP           0x2F	// R/W Interrupt mapping control
#define INT_SOURCE        0x30	// R   Source of interrupts
#define DATA_FORMAT       0x31	// R/W Data format control
#define DATAX0            0x32	// R   X-Axis Data 0
#define DATAX1            0x33	// R   X-Axis Data 1
#define DATAY0            0x34	// R   Y-Axis Data 0
#define DATAY1            0x35	// R   Y-Axis Data 1
#define DATAZ0            0x36	// R   Z-Axis Data 0
#define DATAZ1            0x37	// R   Z-Axis Data 1
#define FIFO_CTL          0x38	// R/W FIFO control
#define FIFO_STATUS       0x39	// R   FIFO status

#define BW_RATE_3200   0xf
#define BW_RATE_1600   0xe
#define BW_RATE_800    0xd
#define BW_RATE_400    0xc
#define BW_RATE_200    0xb
#define BW_RATE_100    0xa
#define BW_RATE_50     0x9
#define BW_RATE_25     0x8
#define BW_RATE_12_5   0x7
#define BW_RATE_6_25   0x6

//-----------------------------------------------------------------------------

// read/write a device register
static uint8_t accel_reg_rd(uint8_t reg) {
	return i2c_reg_rd(I2C_ADR_ADXL345, reg);
}
static void accel_reg_wr(uint8_t reg, uint8_t val) {
	i2c_reg_wr(I2C_ADR_ADXL345, reg, val);
}

// read the accelerometer data
int accel_rd(IMU_RAW * ptr) {
	// read 6 bytes starting at the DATAX0 register.
	uint8_t reg = DATAX0;
	if (i2c_write(I2C_ADR_ADXL345, &reg, 1) != 0) {
		return -1;
	}
	if (i2c_read(I2C_ADR_ADXL345, (uint8_t *) ptr, sizeof(IMU_RAW)) != 0) {
		return -1;
	}
	return 0;
}

int accel_init(void) {
	// do we have a device on the bus?
	uint8_t tmp;
	if (i2c_read(I2C_ADR_ADXL345, &tmp, 1) != 0) {
		return -1;
	}
	// read and check the device id register
	if (accel_reg_rd(DEVID) != 0xe5) {
		return -1;
	}

	accel_reg_wr(TAP_THRESH, 0);
	accel_reg_wr(OFSX, 0);
	accel_reg_wr(OFSY, 0);
	accel_reg_wr(OFSZ, 0);
	accel_reg_wr(TAP_DUR, 0);
	accel_reg_wr(TAP_LATENT, 0);
	accel_reg_wr(TAP_WINDOW, 0);
	accel_reg_wr(THRESH_ACT, 0);
	accel_reg_wr(THRESH_INACT, 0);
	accel_reg_wr(TIME_INACT, 0);
	accel_reg_wr(ACT_INACT_CTL, 0);
	accel_reg_wr(THRESH_FF, 0);
	accel_reg_wr(TIME_FF, 0);
	accel_reg_wr(TAP_AXES, 0);
	accel_reg_wr(BW_RATE, BW_RATE_50);
	accel_reg_wr(POWER_CTL, (1 << 3 /*measure */ ));
	accel_reg_wr(INT_ENABLE, 0);
	accel_reg_wr(INT_MAP, 0);
	accel_reg_wr(DATA_FORMAT, 0);
	accel_reg_wr(FIFO_CTL, (2 << 6 /*stream */ ));

	return 0;
}

// return non-zero if a new accelerometer sample is available
int accel_poll(void) {
	return accel_reg_rd(FIFO_STATUS) & 0x3f;
}

//-----------------------------------------------------------------------------

#endif				// DEADSY_ADXL345_H

//-----------------------------------------------------------------------------
