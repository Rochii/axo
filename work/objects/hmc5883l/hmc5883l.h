//-----------------------------------------------------------------------------
/*

HMC5883L - 3 axis digital compass
Author: Jason Harris (https://github.com/deadsy)

*/
//-----------------------------------------------------------------------------

#ifndef DEADSY_HMC5883L_H
#define DEADSY_HMC5883L_H

//-----------------------------------------------------------------------------

#if CH_KERNEL_MAJOR == 2
#define THD_WORKING_AREA_SIZE THD_WA_SIZE
#define MSG_OK RDY_OK
#define THD_FUNCTION(tname, arg) msg_t tname(void *arg)
#endif

//-----------------------------------------------------------------------------
// registers

#define HMC5883L_CFG_REG_A    0x0	// R/W Configuration Register A
#define HMC5883L_CFG_REG_B    0x1	// R/W Configuration Register B
#define HMC5883L_MODE_REG     0x2	// R/W Mode Register
#define HMC5883L_DOUT_X_MSB   0x3	// R   Data Output X MSB Register
#define HMC5883L_DOUT_X_LSB   0x4	// R   Data Output X LSB Register
#define HMC5883L_DOUT_Z_MSB   0x5	// R   Data Output Z MSB Register
#define HMC5883L_DOUT_Z_LSB   0x6	// R   Data Output Z LSB Register
#define HMC5883L_DOUT_Y_MSB   0x7	// R   Data Output Y MSB Register
#define HMC5883L_DOUT_Y_LSB   0x8	// R   Data Output Y LSB Register
#define HMC5883L_STATUS_REG   0x9	// R   Status Register
#define HMC5883L_ID_REG_A     0xa	// R   Identification Register A
#define HMC5883L_ID_REG_B     0xb	// R   Identification Register B
#define HMC5883L_ID_REG_C     0xc	// R   Identification Register C

#define COMPASS_RATE_0_75   0
#define COMPASS_RATE_1_5    1
#define COMPASS_RATE_3      2
#define COMPASS_RATE_7_5    3
#define COMPASS_RATE_15     4
#define COMPASS_RATE_30     5
#define COMPASS_RATE_75     6

#define HMC5883L_I2C_TIMEOUT 30	// chibios ticks

#define HMC5883L_I2C_ADR 0x1e

//-----------------------------------------------------------------------------

// hmc5883l configuration
struct hmc5883l_cfg {
	uint8_t reg;
	uint8_t val;
};

// hmc5883l state variables
struct hmc5883l_state {
	stkalign_t thd_wa[THD_WORKING_AREA_SIZE(512) / sizeof(stkalign_t)];	// thread working area
	Thread *thd;		// thread pointer
	const struct hmc5883l_cfg *cfg;	// register configuration
	I2CDriver *dev;		// i2c bus driver
	i2caddr_t adr;		// i2c device address
	uint8_t *tx;		// i2c tx buffer
	uint8_t *rx;		// i2c rx buffer
	int32_t x, y, z;	// acceleration vector (shared across dsp/hmc5883l thread)
};

//-----------------------------------------------------------------------------

// Allocate a 32-bit aligned buffer of size bytes from sram2.
// The memory pool is big enough for 4 concurrent devices.
static void *hmc5883l_malloc(size_t size) {
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
static int hmc5883l_rd8(struct hmc5883l_state *s, uint8_t reg, uint8_t * val) {
	s->tx[0] = reg;
	i2cAcquireBus(s->dev);
	msg_t rc = i2cMasterTransmitTimeout(s->dev, s->adr, s->tx, 1, s->rx, 1, HMC5883L_I2C_TIMEOUT);
	i2cReleaseBus(s->dev);
	*val = *(uint8_t *) s->rx;
	return (rc == MSG_OK) ? 0 : -1;
}

// write an 8 bit value to a register
static int hmc5883l_wr8(struct hmc5883l_state *s, uint8_t reg, uint8_t val) {
	s->tx[0] = reg;
	s->tx[1] = val;
	i2cAcquireBus(s->dev);
	msg_t rc = i2cMasterTransmitTimeout(s->dev, s->adr, s->tx, 2, NULL, 0, HMC5883L_I2C_TIMEOUT);
	i2cReleaseBus(s->dev);
	return (rc == MSG_OK) ? 0 : -1;
}

//-----------------------------------------------------------------------------

// read the compass data
static int hmc5883l_rd_compass(struct hmc5883l_state *s) {
	// read 6 bytes starting at the DOUT_X_MSB register.

	s->tx[0] = HMC5883L_DOUT_X_MSB;
	i2cAcquireBus(s->dev);
	msg_t rc = i2cMasterTransmitTimeout(s->dev, s->adr, s->tx, 1, s->rx, 6, HMC5883L_I2C_TIMEOUT);
	i2cReleaseBus(s->dev);
	if (rc != MSG_OK) {
		return -1;
	}
	// TODO unit conversion
	chSysLock();
	s->x = *(uint16_t *) & s->rx[0];
	s->y = *(uint16_t *) & s->rx[2];
	s->z = *(uint16_t *) & s->rx[4];
	chSysUnlock();

	return 0;
}

// return non-zero if a new accelerometer sample is available
static int hmc5883l_poll(struct hmc5883l_state *s) {
	uint8_t val;
	// TODO this isn't right, the datasheet description of this register appears to be incorrect.
	hmc5883l_rd8(s, HMC5883L_STATUS_REG, &val);
	return val;
}

//-----------------------------------------------------------------------------

static void hmc5883l_info(struct hmc5883l_state *s, const char *msg) {
	LogTextMessage("hmc5883l(0x%x) %s", s->adr, msg);
}

static void hmc5883l_error(struct hmc5883l_state *s, const char *msg) {
	hmc5883l_info(s, msg);
	// wait for the parent thread to kill us
	while (!chThdShouldTerminate()) {
		chThdSleepMilliseconds(100);
	}
}

static THD_FUNCTION(hmc5883l_thread, arg) {
	struct hmc5883l_state *s = (struct hmc5883l_state *)arg;
	int idx = 0;
	int rc = 0;

	hmc5883l_info(s, "starting thread");

	// allocate i2c buffers
	s->tx = (uint8_t *) hmc5883l_malloc(2);
	s->rx = (uint8_t *) hmc5883l_malloc(6);
	if (s->rx == NULL || s->tx == NULL) {
		hmc5883l_error(s, "out of memory");
		goto exit;
	}
	// read and check the device id
	uint8_t id_a, id_b, id_c;
	rc = hmc5883l_rd8(s, HMC5883L_ID_REG_A, &id_a);
	if (rc < 0) {
		hmc5883l_error(s, "i2c error");
		goto exit;
	}
	hmc5883l_rd8(s, HMC5883L_ID_REG_B, &id_b);
	hmc5883l_rd8(s, HMC5883L_ID_REG_C, &id_c);
	if (id_a != 'H' || id_b != '4' || id_c != '3') {
		hmc5883l_error(s, "bad device id");
		goto exit;
	}
	// apply the per-object register configuration
	while (s->cfg[idx].reg != 0xff) {
		hmc5883l_wr8(s, s->cfg[idx].reg, s->cfg[idx].val);
		idx += 1;
	}

	// poll for the changing compass status
	while (!chThdShouldTerminate()) {
		if (hmc5883l_poll(s)) {
			hmc5883l_rd_compass(s);
		}
		// TODO - tune loop time
		chThdSleepMilliseconds(10);
	}

 exit:
	hmc5883l_info(s, "stopping thread");
	chThdExit((msg_t) 0);
}

//-----------------------------------------------------------------------------

static void hmc5883l_init(struct hmc5883l_state *s, const struct hmc5883l_cfg *cfg) {
	// initialise the state
	memset(s, 0, sizeof(struct hmc5883l_state));
	s->cfg = cfg;
	s->dev = &I2CD1;
	s->adr = HMC5883L_I2C_ADR;
	// create the polling thread
	s->thd = chThdCreateStatic(s->thd_wa, sizeof(s->thd_wa), NORMALPRIO, hmc5883l_thread, (void *)s);
}

static void hmc5883l_dispose(struct hmc5883l_state *s) {
	// stop thread
	chThdTerminate(s->thd);
	chThdWait(s->thd);
}

// return the current acceleration vector
static void hmc5883l_krate(struct hmc5883l_state *s, int32_t * x, int32_t * y, int32_t * z) {
	chSysLock();
	*x = s->x;
	*y = s->y;
	*z = s->z;
	chSysUnlock();
}

//-----------------------------------------------------------------------------

#endif				// DEADSY_HMC5883L_H

//-----------------------------------------------------------------------------
