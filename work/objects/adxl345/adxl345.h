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

#if CH_KERNEL_MAJOR == 2
#define THD_WORKING_AREA_SIZE THD_WA_SIZE
#define MSG_OK RDY_OK
#define THD_FUNCTION(tname, arg) msg_t tname(void *arg)
#endif

//-----------------------------------------------------------------------------
// registers

#define ADXL345_DEVID             0x00	// R   Device ID
#define ADXL345_TAP_THRESH        0x1D	// R/W Tap threshold
#define ADXL345_OFSX              0x1E	// R/W X-axis offset
#define ADXL345_OFSY              0x1F	// R/W Y-axis offset
#define ADXL345_OFSZ              0x20	// R/W Z-axis offset
#define ADXL345_TAP_DUR           0x21	// R/W Tap duration
#define ADXL345_TAP_LATENT        0x22	// R/W Tap latency
#define ADXL345_TAP_WINDOW        0x23	// R/W Tap window
#define ADXL345_THRESH_ACT        0x24	// R/W Activity threshold
#define ADXL345_THRESH_INACT      0x25	// R/W Inactivity threshold
#define ADXL345_TIME_INACT        0x26	// R/W Inactivity time
#define ADXL345_ACT_INACT_CTL     0x27	// R/W Axis enable control for activity and inactivity detection
#define ADXL345_THRESH_FF         0x28	// R/W Free-fall threshold
#define ADXL345_TIME_FF           0x29	// R/W Free-fall time
#define ADXL345_TAP_AXES          0x2A	// R/W Axis control for tap/double tap
#define ADXL345_ACT_TAP_STATUS    0x2B	// R   Source of tap/double tap
#define ADXL345_BW_RATE           0x2C	// R/W Data rate and power mode control
#define ADXL345_POWER_CTL         0x2D	// R/W Power-saving features control
#define ADXL345_INT_ENABLE        0x2E	// R/W Interrupt enable control
#define ADXL345_INT_MAP           0x2F	// R/W Interrupt mapping control
#define ADXL345_INT_SOURCE        0x30	// R   Source of interrupts
#define ADXL345_DATA_FORMAT       0x31	// R/W Data format control
#define ADXL345_DATAX0            0x32	// R   X-Axis Data 0
#define ADXL345_DATAX1            0x33	// R   X-Axis Data 1
#define ADXL345_DATAY0            0x34	// R   Y-Axis Data 0
#define ADXL345_DATAY1            0x35	// R   Y-Axis Data 1
#define ADXL345_DATAZ0            0x36	// R   Z-Axis Data 0
#define ADXL345_DATAZ1            0x37	// R   Z-Axis Data 1
#define ADXL345_FIFO_CTL          0x38	// R/W FIFO control
#define ADXL345_FIFO_STATUS       0x39	// R   FIFO status

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

#define ADXL345_I2C_TIMEOUT 30	// chibios ticks

//-----------------------------------------------------------------------------

// adxl345 configuration
struct adxl345_cfg {
	uint8_t reg;
	uint8_t val;
};

// adxl345 state variables
struct adxl345_state {
	stkalign_t thd_wa[THD_WORKING_AREA_SIZE(512) / sizeof(stkalign_t)];	// thread working area
	Thread *thd;		// thread pointer
	const struct adxl345_cfg *cfg;	// register configuration
	I2CDriver *dev;		// i2c bus driver
	i2caddr_t adr;		// i2c device address
	uint8_t *tx;		// i2c tx buffer
	uint8_t *rx;		// i2c rx buffer
	int32_t x, y, z;	// acceleration vector (shared across dsp/adxl345 thread)
};

//-----------------------------------------------------------------------------

// Allocate a 32-bit aligned buffer of size bytes from sram2.
// The memory pool is big enough for 4 concurrent devices.
static void *adxl345_malloc(size_t size) {
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
static int adxl345_rd8(struct adxl345_state *s, uint8_t reg, uint8_t * val) {
	s->tx[0] = reg;
	i2cAcquireBus(s->dev);
	msg_t rc = i2cMasterTransmitTimeout(s->dev, s->adr, s->tx, 1, s->rx, 1, ADXL345_I2C_TIMEOUT);
	i2cReleaseBus(s->dev);
	*val = *(uint8_t *) s->rx;
	return (rc == MSG_OK) ? 0 : -1;
}

// write an 8 bit value to a register
static int adxl345_wr8(struct adxl345_state *s, uint8_t reg, uint8_t val) {
	s->tx[0] = reg;
	s->tx[1] = val;
	i2cAcquireBus(s->dev);
	msg_t rc = i2cMasterTransmitTimeout(s->dev, s->adr, s->tx, 2, NULL, 0, ADXL345_I2C_TIMEOUT);
	i2cReleaseBus(s->dev);
	return (rc == MSG_OK) ? 0 : -1;
}

//-----------------------------------------------------------------------------

// read the accelerometer data
static int adxl345_rd_accel(struct adxl345_state *s) {
	// read 6 bytes starting at the DATAX0 register.

	s->tx[0] = ADXL345_DATAX0;
	i2cAcquireBus(s->dev);
	msg_t rc = i2cMasterTransmitTimeout(s->dev, s->adr, s->tx, 1, s->rx, 6, ADXL345_I2C_TIMEOUT);
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
static int adxl345_poll(struct adxl345_state *s) {
	uint8_t val;
	adxl345_rd8(s, ADXL345_FIFO_STATUS, &val);
	return val & 0x3f;
}

//-----------------------------------------------------------------------------

static void adxl345_info(struct adxl345_state *s, const char *msg) {
	LogTextMessage("adxl345(0x%x) %s", s->adr, msg);
}

static void adxl345_error(struct adxl345_state *s, const char *msg) {
	adxl345_info(s, msg);
	// wait for the parent thread to kill us
	while (!chThdShouldTerminate()) {
		chThdSleepMilliseconds(100);
	}
}

static THD_FUNCTION(adxl345_thread, arg) {
	struct adxl345_state *s = (struct adxl345_state *)arg;
	int idx = 0;
	int rc = 0;

	adxl345_info(s, "starting thread");

	// allocate i2c buffers
	s->tx = (uint8_t *) adxl345_malloc(2);
	s->rx = (uint8_t *) adxl345_malloc(6);
	if (s->rx == NULL || s->tx == NULL) {
		adxl345_error(s, "out of memory");
		goto exit;
	}

	uint8_t val;
	rc = adxl345_rd8(s, ADXL345_DEVID, &val);
	if (rc < 0) {
		adxl345_error(s, "i2c error");
		goto exit;
	}
	if (val != 0xe5) {
		adxl345_error(s, "bad device id");
		goto exit;
	}
	// apply the per-object register configuration
	while (s->cfg[idx].reg != 0xff) {
		adxl345_wr8(s, s->cfg[idx].reg, s->cfg[idx].val);
		idx += 1;
	}

	// poll for the changing accelerometer status
	while (!chThdShouldTerminate()) {
		if (adxl345_poll(s)) {
			adxl345_rd_accel(s);
		}
		// TODO - tune loop time
		chThdSleepMilliseconds(10);
	}

 exit:
	adxl345_info(s, "stopping thread");
	chThdExit((msg_t) 0);
}

//-----------------------------------------------------------------------------

static void adxl345_init(struct adxl345_state *s, const struct adxl345_cfg *cfg, i2caddr_t adr) {
	// initialise the state
	memset(s, 0, sizeof(struct adxl345_state));
	s->cfg = cfg;
	s->dev = &I2CD1;
	s->adr = adr;
	// create the polling thread
	s->thd = chThdCreateStatic(s->thd_wa, sizeof(s->thd_wa), NORMALPRIO, adxl345_thread, (void *)s);
}

static void adxl345_dispose(struct adxl345_state *s) {
	// stop thread
	chThdTerminate(s->thd);
	chThdWait(s->thd);
}

// return the current acceleration vector
static void adxl345_krate(struct adxl345_state *s, int32_t * x, int32_t * y, int32_t * z) {
	chSysLock();
	*x = s->x;
	*y = s->y;
	*z = s->z;
	chSysUnlock();
}

//-----------------------------------------------------------------------------

#endif				// DEADSY_ADXL345_H

//-----------------------------------------------------------------------------
