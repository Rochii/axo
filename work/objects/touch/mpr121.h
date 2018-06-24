//-----------------------------------------------------------------------------
/*

MPR121 Capacitive Touch Sensor (I2C).
Author: Jason Harris (https://github.com/deadsy)

The touch output is the bit-wise status of the touch plates (bits 0..11)
0 == not touched
1 == touched

Note:
This object requires a single instance of the factory/gpio/i2c/config object.
This allows multiple mpr121 devices (each with a unique i2c address) to work concurrently.
I2C Bus: I2C1, SCL=PB8, SDA=PB9 (these are the config defaults)

Tested with: https://www.adafruit.com/product/1982
Vin - voltage input 3.3v/5v (to axoloti)
3Vo - voltage output (nc)
GND - ground (to axoloti gnd)
ADDR - address select (nc == 0x5a)
SDA - serial data (to i2c1/pb9)
SCL - serial clock (to i2c1/pb8)
IRQ - interrupt request (nc - we poll it)

*/
//-----------------------------------------------------------------------------

#ifndef DEADSY_MPR121_H
#define DEADSY_MPR121_H

//-----------------------------------------------------------------------------

// registers
#define MPR121_TSTATUS_L    0x00
#define MPR121_TSTATUS_H    0x01
#define MPR121_FDATA_L(x)   (0x04 + (2 * (x)))
#define MPR121_FDATA_H(x)   (0x05 + (2 * (x)))
#define MPR121_BASELINE(x)  (0x1E + (x))
#define MPR121_MHDR         0x2B
#define MPR121_NHDR         0x2C
#define MPR121_NCLR         0x2D
#define MPR121_FDLR         0x2E
#define MPR121_MHDF         0x2F
#define MPR121_NHDF         0x30
#define MPR121_NCLF         0x31
#define MPR121_FDLF         0x32
#define MPR121_NHDT         0x33
#define MPR121_NCLT         0x34
#define MPR121_FDLT         0x35
#define MPR121_TTH(x)       (0x41 + (2 * (x)))
#define MPR121_RTH(x)       (0x42 + (2 * (x)))
#define MPR121_DEBOUNCE     0x5B
#define MPR121_CONFIG1      0x5C
#define MPR121_CONFIG2      0x5D
#define MPR121_ECR          0x5E
#define MPR121_CDC(x)       (0x5F + (x))
#define MPR121_CDT          0x6C
#define MPR121_GPIODIR      0x76
#define MPR121_GPIOEN       0x77
#define MPR121_GPIOSET      0x78
#define MPR121_GPIOCLR      0x79
#define MPR121_GPIOTOGGLE   0x7A
#define MPR121_AUTOCONFIG0  0x7B
#define MPR121_AUTOCONFIG1  0x7C
#define MPR121_UPLIMIT      0x7D
#define MPR121_LOWLIMIT     0x7E
#define MPR121_TARGETLIMIT  0x7F
#define MPR121_SOFTRESET    0x80

#define I2C_TIMEOUT 30		// chibios ticks

//-----------------------------------------------------------------------------

// mpr121 configuration
struct mpr121_cfg {
	uint8_t reg;
	uint8_t val;
};

// mpr121 state variables
struct mpr121_state {
	stkalign_t thd_wa[THD_WA_SIZE(1024) / sizeof(stkalign_t)];	// thread working area
	Thread *thd;		// thread pointer
	const struct mpr121_cfg *cfg;	// register configuration
	I2CDriver *dev;		// i2c bus driver
	i2caddr_t adr;		// i2c device address
	uint8_t *tx;		// i2c tx buffer
	uint8_t *rx;		// i2c rx buffer
	int32_t touch;		// touch status (shared across dsp/mpr121 thread)
};

//-----------------------------------------------------------------------------

// Allocate a 32-bit aligned buffer of size bytes from sram2.
// The memory pool is big enough for 4 concurrent devices.
static void *mpr121_malloc(size_t size) {
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
static int mpr121_rd8(struct mpr121_state *s, uint8_t reg, uint8_t * val) {
	s->tx[0] = reg;
	msg_t rc = i2cMasterTransmitTimeout(s->dev, s->adr, s->tx, 1, s->rx, 1, I2C_TIMEOUT);
	*val = *(uint8_t *) s->rx;
	return (rc == RDY_OK) ? 0 : -1;
}

// read a 16 bit value from a register
static int mpr121_rd16(struct mpr121_state *s, uint8_t reg, uint16_t * val) {
	s->tx[0] = reg;
	msg_t rc = i2cMasterTransmitTimeout(s->dev, s->adr, s->tx, 1, s->rx, 2, I2C_TIMEOUT);
	*val = *(uint16_t *) s->rx;
	return (rc == RDY_OK) ? 0 : -1;
}

// write an 8 bit value to a register
static int mpr121_wr8(struct mpr121_state *s, uint8_t reg, uint8_t val) {
	s->tx[0] = reg;
	s->tx[1] = val;
	msg_t rc = i2cMasterTransmitTimeout(s->dev, s->adr, s->tx, 2, NULL, 0, I2C_TIMEOUT);
	return (rc == RDY_OK) ? 0 : -1;
}

//-----------------------------------------------------------------------------

static void mpr121_info(struct mpr121_state *s, const char *msg) {
	LogTextMessage("mpr121(0x%x) %s", s->adr, msg);
}

static void mpr121_error(struct mpr121_state *s, const char *msg) {
	mpr121_info(s, msg);
	// wait for the parent thread to kill us
	while (!chThdShouldTerminate()) {
		chThdSleepMilliseconds(100);
	}
}

static msg_t mpr121_thread(void *arg) {
	struct mpr121_state *s = (struct mpr121_state *)arg;
	int rc = 0;
	int idx = 0;

	//mpr121_info(s, "starting thread");

	// allocate i2c buffers
	s->tx = (uint8_t *) mpr121_malloc(2);
	s->rx = (uint8_t *) mpr121_malloc(2);
	if (s->rx == NULL || s->tx == NULL) {
		mpr121_error(s, "out of memory");
		goto exit;
	}
	// reset the device
	rc = mpr121_wr8(s, MPR121_SOFTRESET, 0x63);
	if (rc < 0) {
		mpr121_error(s, "i2c error");
		goto exit;
	}
	chThdSleepMilliseconds(1);

	// check the expected default values for the config registers
	uint8_t cfg1, cfg2;
	mpr121_rd8(s, MPR121_CONFIG1, &cfg1);
	mpr121_rd8(s, MPR121_CONFIG2, &cfg2);
	if (cfg1 != 0x10 || cfg2 != 0x24) {
		mpr121_error(s, "bad register values");
		goto exit;
	}
	// apply the per-object register configuration
	while (s->cfg[idx].reg != 0xff) {
		mpr121_wr8(s, s->cfg[idx].reg, s->cfg[idx].val);
		idx += 1;
	}

	// poll for the changing touch status
	while (!chThdShouldTerminate()) {
		uint16_t val;
		// read the touch status
		mpr121_rd16(s, MPR121_TSTATUS_L, &val);
		val &= 0xfff;
		chSysLock();
		s->touch = val;
		chSysUnlock();
		chThdSleepMilliseconds(10);
	}

 exit:
	//mpr121_info(s, "stopping thread");
	chThdExit((msg_t) 0);
}

//-----------------------------------------------------------------------------

static void mpr121_init(struct mpr121_state *s, const struct mpr121_cfg *cfg, i2caddr_t adr) {
	// initialise the state
	memset(s, 0, sizeof(struct mpr121_state));
	s->cfg = cfg;
	s->dev = &I2CD1;
	s->adr = adr;
	// create the polling thread
	s->thd = chThdCreateStatic(s->thd_wa, sizeof(s->thd_wa), NORMALPRIO, mpr121_thread, (void *)s);
}

static void mpr121_dispose(struct mpr121_state *s) {
	// stop thread
	chThdTerminate(s->thd);
	chThdWait(s->thd);
}

static void mpr121_krate(struct mpr121_state *s, int32_t * touch) {
	// get the current touch status
	chSysLock();
	*touch = s->touch;
	chSysUnlock();
}

//-----------------------------------------------------------------------------

#endif				// DEADSY_MPR121_H

//-----------------------------------------------------------------------------
