//-----------------------------------------------------------------------------
/*

ITG-3200 - 3 axis gyroscope
Author: Jason Harris (https://github.com/deadsy)
http://www.sparkfun.com/products/10724
https://www.invensense.com/wp-content/uploads/2015/02/ITG-3200-Datasheet.pdf

*/
//-----------------------------------------------------------------------------

#ifndef DEADSY_ITG3200_H
#define DEADSY_ITG3200_H

//-----------------------------------------------------------------------------
// registers

#define I2C_ADR_ITG3200 0x68

#define WHO_AM_I    0x00	// R/W
#define SMPLRT_DIV  0x15	// R/W
#define DLPF_FS     0x16	// R/W
#define INT_CFG     0x17	// R/W
#define INT_STATUS  0x1A	// R
#define TEMP_OUT_H  0x1B	// R
#define TEMP_OUT_L  0x1C	// R
#define GYRO_XOUT_H 0x1D	// R
#define GYRO_XOUT_L 0x1E	// R
#define GYRO_YOUT_H 0x1F	// R
#define GYRO_YOUT_L 0x20	// R
#define GYRO_ZOUT_H 0x21	// R
#define GYRO_ZOUT_L 0x22	// R
#define PWR_MGM     0x3E	// R/W

#define SMPLRT(x, y) (((x)/(y)) - 1)

#define DLP_CFG_256Hz_8kHz  0
#define DLP_CFG_188Hz_1kHz  1
#define DLP_CFG_98Hz_1kHz   2
#define DLP_CFG_42Hz_1kHz   3
#define DLP_CFG_20Hz_1kHz   4
#define DLP_CFG_10Hz_1kHz   5
#define DLP_CFG_5Hz_1kHz    6

// read/write a device register
static uint8_t gyro_reg_rd(uint8_t reg) {
	return i2c_reg_rd(I2C_ADR_ITG3200, reg);
}
static void gyro_reg_wr(uint8_t reg, uint8_t val) {
	i2c_reg_wr(I2C_ADR_ITG3200, reg, val);
}

// read the gyro temperature and axis data
int gyro_rd(IMU_RAW * ptr) {
	// read 8 bytes starting at the TEMP_OUT_H register.
	uint8_t reg = TEMP_OUT_H;
	uint8_t data[8];

	if (i2c_write(I2C_ADR_ITG3200, &reg, 1) != 0) {
		return -1;
	}
	if (i2c_read(I2C_ADR_ITG3200, data, sizeof(data)) != 0) {
		return -1;
	}
	// pack the i2c data into the imu raw structure
	uint8_t *raw = (uint8_t *) ptr;
	raw[0] = data[3];	// x
	raw[1] = data[2];
	raw[2] = data[5];	// y
	raw[3] = data[4];
	raw[4] = data[7];	// z
	raw[5] = data[6];

	// todo: how to use temperature?
	// data[0..1]

	return 0;
}

int gyro_init(void) {
	// do we have a device on the bus?
	uint8_t tmp;
	if (i2c_read(I2C_ADR_ITG3200, &tmp, 1) != 0) {
		return -1;
	}
	// reset the gyro
	gyro_reg_wr(PWR_MGM, (1 << 7 /*H_RESET */ ));

	// read and check the "who am i" register
	uint8_t val = gyro_reg_rd(WHO_AM_I);
	if ((val >> 1) != (I2C_ADR_ITG3200 >> 2)) {
		return -1;
	}
	// configure the gyro
	// 50 Hz sample rate
	// 1000 Hz ADC sample rate
	// 98Hz low pass filter cutoff

	gyro_reg_wr(SMPLRT_DIV, SMPLRT(1000, 50) /*SMPLRT_DIV */ );
	gyro_reg_wr(DLPF_FS, (3 << 3 /*FS_SEL */ ) | (DLP_CFG_20Hz_1kHz << 0 /*DLPF_CFG */ ));
	gyro_reg_wr(INT_CFG, (1 << 0 /*RAW_RDY_EN */ ));
	gyro_reg_wr(PWR_MGM, (1 << 0 /*CLK_SEL */ ));

	return 0;
}

// return non-zero if a new gyro sample is available
int gyro_poll(void) {
	return gyro_reg_rd(INT_STATUS) & 1;
}

//-----------------------------------------------------------------------------

#endif				// DEADSY_ITG3200_H

//-----------------------------------------------------------------------------
