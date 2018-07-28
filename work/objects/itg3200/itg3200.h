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

#if CH_KERNEL_MAJOR == 2
#define THD_WORKING_AREA_SIZE THD_WA_SIZE
#define MSG_OK RDY_OK
#define THD_FUNCTION(tname, arg) msg_t tname(void *arg)
#endif

//-----------------------------------------------------------------------------
// registers

#define ITG3200_WHO_AM_I    0x00	// R/W
#define ITG3200_SMPLRT_DIV  0x15	// R/W
#define ITG3200_DLPF_FS     0x16	// R/W
#define ITG3200_INT_CFG     0x17	// R/W
#define ITG3200_INT_STATUS  0x1A	// R
#define ITG3200_TEMP_OUT_H  0x1B	// R
#define ITG3200_TEMP_OUT_L  0x1C	// R
#define ITG3200_GYRO_XOUT_H 0x1D	// R
#define ITG3200_GYRO_XOUT_L 0x1E	// R
#define ITG3200_GYRO_YOUT_H 0x1F	// R
#define ITG3200_GYRO_YOUT_L 0x20	// R
#define ITG3200_GYRO_ZOUT_H 0x21	// R
#define ITG3200_GYRO_ZOUT_L 0x22	// R
#define ITG3200_PWR_MGM     0x3E	// R/W

#define SMPLRT(x, y) (((x)/(y)) - 1)

#define DLP_CFG_256Hz_8kHz  0
#define DLP_CFG_188Hz_1kHz  1
#define DLP_CFG_98Hz_1kHz   2
#define DLP_CFG_42Hz_1kHz   3
#define DLP_CFG_20Hz_1kHz   4
#define DLP_CFG_10Hz_1kHz   5
#define DLP_CFG_5Hz_1kHz    6

#define ITG3200_I2C_TIMEOUT 30	// chibios ticks

//-----------------------------------------------------------------------------

// itg3200 configuration
struct itg3200_cfg {
	uint8_t reg;
	uint8_t val;
};

// itg3200 state variables
struct itg3200_state {
	stkalign_t thd_wa[THD_WORKING_AREA_SIZE(512) / sizeof(stkalign_t)];	// thread working area
	Thread *thd;		// thread pointer
	const struct itg3200_cfg *cfg;	// register configuration
	I2CDriver *dev;		// i2c bus driver
	i2caddr_t adr;		// i2c device address
	uint8_t *tx;		// i2c tx buffer
	uint8_t *rx;		// i2c rx buffer
	int32_t x, y, z;	// gyro rate vector (shared across dsp/itg3200 thread)
};

//-----------------------------------------------------------------------------

// Allocate a 32-bit aligned buffer of size bytes from sram2.
// The memory pool is big enough for 2 concurrent devices.
static void *itg3200_malloc(size_t size) {
	static uint8_t pool[32] __attribute__ ((section(".sram2")));
	static uint32_t free = 0;
	void *ptr = NULL;
	// round up to 32-bit alignment
	size = (size + 3) & ~3;
	chSysLock();
	if ((free + size) <= sizeof(pool)) {
		ptr = &pool[free];
		free += size;
	}
	chSysUnlock();
	return ptr;
}

//-----------------------------------------------------------------------------
// i2c read/write routines

// read an 8 bit value from a register
static int itg3200_rd8(struct itg3200_state *s, uint8_t reg, uint8_t * val) {
	s->tx[0] = reg;
	i2cAcquireBus(s->dev);
	msg_t rc = i2cMasterTransmitTimeout(s->dev, s->adr, s->tx, 1, s->rx, 1, ITG3200_I2C_TIMEOUT);
	i2cReleaseBus(s->dev);
	*val = *(uint8_t *) s->rx;
	return (rc == MSG_OK) ? 0 : -1;
}

// write an 8 bit value to a register
static int itg3200_wr8(struct itg3200_state *s, uint8_t reg, uint8_t val) {
	s->tx[0] = reg;
	s->tx[1] = val;
	i2cAcquireBus(s->dev);
	msg_t rc = i2cMasterTransmitTimeout(s->dev, s->adr, s->tx, 2, NULL, 0, ITG3200_I2C_TIMEOUT);
	i2cReleaseBus(s->dev);
	return (rc == MSG_OK) ? 0 : -1;
}

//-----------------------------------------------------------------------------

// read the gyro data
static int itg3200_rd_gyro(struct itg3200_state *s) {
	// read 8 bytes starting at the TEMP_OUT_H register.

	s->tx[0] = ITG3200_TEMP_OUT_H;
	i2cAcquireBus(s->dev);
	msg_t rc = i2cMasterTransmitTimeout(s->dev, s->adr, s->tx, 1, s->rx, 8, ITG3200_I2C_TIMEOUT);
	i2cReleaseBus(s->dev);

	if (rc != MSG_OK) {
		return -1;
	}
	// TODO unit conversion
	chSysLock();
	s->x = *(uint16_t *) & s->rx[2];
	s->y = *(uint16_t *) & s->rx[4];
	s->z = *(uint16_t *) & s->rx[6];
	// TODO use temperature
	chSysUnlock();

	return 0;
}

// return non-zero if a new gyro sample is available
static int itg3200_poll(struct itg3200_state *s) {
	uint8_t val;
	itg3200_rd8(s, ITG3200_INT_STATUS, &val);
	return val & 1;
}

//-----------------------------------------------------------------------------

static void itg3200_info(struct itg3200_state *s, const char *msg) {
	LogTextMessage("itg3200(0x%x) %s", s->adr, msg);
}

static void itg3200_error(struct itg3200_state *s, const char *msg) {
	itg3200_info(s, msg);
	// wait for the parent thread to kill us
	while (!chThdShouldTerminate()) {
		chThdSleepMilliseconds(100);
	}
}

static THD_FUNCTION(itg3200_thread, arg) {
	struct itg3200_state *s = (struct itg3200_state *)arg;
	int idx = 0;
	int rc = 0;

	itg3200_info(s, "starting thread");

	// allocate i2c buffers
	s->tx = (uint8_t *) itg3200_malloc(2);
	s->rx = (uint8_t *) itg3200_malloc(8);
	if (s->rx == NULL || s->tx == NULL) {
		itg3200_error(s, "out of memory");
		goto exit;
	}
	// reset the gyro
	rc = itg3200_wr8(s, ITG3200_PWR_MGM, (1 << 7 /*H_RESET */ ));
	if (rc < 0) {
		itg3200_error(s, "i2c error");
		goto exit;
	}
	// read and check the "who am i" register
	uint8_t val;
	itg3200_rd8(s, ITG3200_WHO_AM_I, &val);
	if ((val >> 1) != s->adr) {
		itg3200_error(s, "bad device id");
		goto exit;
	}
	// apply the per-object register configuration
	while (s->cfg[idx].reg != 0xff) {
		itg3200_wr8(s, s->cfg[idx].reg, s->cfg[idx].val);
		idx += 1;
	}

	// poll for the changing touch status
	while (!chThdShouldTerminate()) {
		if (itg3200_poll(s)) {
			itg3200_rd_gyro(s);
		}
		// 50Hz polling interval
		chThdSleepMilliseconds(20);
	}

 exit:
	itg3200_info(s, "stopping thread");
	chThdExit((msg_t) 0);
}

//-----------------------------------------------------------------------------

static void itg3200_init(struct itg3200_state *s, const struct itg3200_cfg *cfg, i2caddr_t adr) {
	// initialise the state
	memset(s, 0, sizeof(struct itg3200_state));
	s->cfg = cfg;
	s->dev = &I2CD1;
	s->adr = adr;
	// create the polling thread
	s->thd = chThdCreateStatic(s->thd_wa, sizeof(s->thd_wa), NORMALPRIO, itg3200_thread, (void *)s);
}

static void itg3200_dispose(struct itg3200_state *s) {
	// stop thread
	chThdTerminate(s->thd);
	chThdWait(s->thd);
}

// return the current acceleration vector
static void itg3200_krate(struct itg3200_state *s, int32_t * x, int32_t * y, int32_t * z) {
	chSysLock();
	*x = s->x;
	*y = s->y;
	*z = s->z;
	chSysUnlock();
}

//-----------------------------------------------------------------------------

#endif				// DEADSY_ITG3200_H

//-----------------------------------------------------------------------------
