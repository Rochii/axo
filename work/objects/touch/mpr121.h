//-----------------------------------------------------------------------------
/*

MPR121 Capacitive Touch Sensor (I2C).
Author: Jason Harris (https://github.com/deadsy)

The touch output is the bit-wise status of the touch plates (bits 0..11)
0 == not touched
1 == touched

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
// #define MPR121_TOUCHSTATUS_L 0x00
// #define MPR121_TOUCHSTATUS_H 0x01
// #define MPR121_FILTDATA_0L   0x04
// #define MPR121_FILTDATA_0H   0x05
// #define MPR121_BASELINE_0    0x1E

// #define MPR121_FDLR          0x2E

// #define MPR121_FDLF          0x32
// #define MPR121_NHDT          0x33
// #define MPR121_NCLT          0x34
// #define MPR121_FDLT          0x35
// #define MPR121_DEBOUNCE      0x5B

// #define MPR121_CHARGECURR_0  0x5F
// #define MPR121_CHARGETIME_1  0x6C
// #define MPR121_AUTOCONFIG0   0x7B
// #define MPR121_AUTOCONFIG1   0x7C
// #define MPR121_UPLIMIT       0x7D
// #define MPR121_LOWLIMIT      0x7E
// #define MPR121_TARGETLIMIT   0x7F
// #define MPR121_GPIODIR       0x76
// #define MPR121_GPIOEN        0x77
// #define MPR121_GPIOSET       0x78
// #define MPR121_GPIOCLR       0x79
// #define MPR121_GPIOTOGGLE    0x7A

#define MPR121_MHDR 0x2B
#define MPR121_NHDR 0x2C
#define MPR121_NCLR 0x2D
#define MPR121_MHDF 0x2F
#define MPR121_NHDF 0x30
#define MPR121_NCLF 0x31
#define MPR121_E0TTH 0x41
#define MPR121_E0RTH 0x42
#define MPR121_CONFIG1 0x5C
#define MPR121_CONFIG2 0x5D
#define MPR121_ECR 0x5E
#define MPR121_SOFTRESET 0x80

#define I2C_TIMEOUT 30		// chibios ticks

//-----------------------------------------------------------------------------

struct mpr121_state {
	stkalign_t thd_wa[THD_WA_SIZE(1024) / sizeof(stkalign_t)];	// thread working area
	Thread *thd;		// thread pointer
	i2caddr_t adr;		// i2c device address
	uint8_t tth;		// touch threshold
	uint8_t rth;		// release threshold
	uint8_t *tx;		// i2c tx buffer
	uint8_t *rx;		// i2c rx buffer
	int32_t touch;		// touch status (shared across dsp/mpr121 thread)
};

//-----------------------------------------------------------------------------
// i2c read/write routines

// read an 8 bit value from a register
static int mpr121_rd8(struct mpr121_state *s, uint8_t reg, uint8_t * val) {
	s->tx[0] = reg;
	msg_t rc = i2cMasterTransmitTimeout(&I2CD1, s->adr, s->tx, 1, s->rx, 1, I2C_TIMEOUT);
	*val = *(uint8_t *) s->rx;
	return (rc == RDY_OK) ? 0 : -1;
}

// read a 16 bit value from a register
static int mpr121_rd16(struct mpr121_state *s, uint8_t reg, uint16_t * val) {
	s->tx[0] = reg;
	msg_t rc = i2cMasterTransmitTimeout(&I2CD1, s->adr, s->tx, 1, s->rx, 2, I2C_TIMEOUT);
	*val = *(uint16_t *) s->rx;
	return (rc == RDY_OK) ? 0 : -1;
}

// write an 8 bit value to a register
static int mpr121_wr8(struct mpr121_state *s, uint8_t reg, uint8_t val) {
	s->tx[0] = reg;
	s->tx[1] = val;
	msg_t rc = i2cMasterTransmitTimeout(&I2CD1, s->adr, s->tx, 2, NULL, 0, I2C_TIMEOUT);
	return (rc == RDY_OK) ? 0 : -1;
}

/*

static uint16_t mpr121_rd_filtered(struct mpr121_state *s, int t) {
	if (t > 12) {
		return 0;
	}
	return mpr121_rd16(s, MPR121_FILTDATA_0L + (t * 2));
}

static uint16_t mpr121_rd_baseline(struct mpr121_state *s, int t) {
	if (t > 12) {
		return 0;
	}
	return (uint16_t) mpr121_rd8(s, MPR121_BASELINE_0 + t) << 2;
}

static uint16_t mpr121_touched(struct mpr121_state *s) {
	return mpr121_rd16(s, MPR121_TOUCHSTATUS_L) & 0x0fff;
}

*/

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
	// i2c buffers in sram2
	static uint8_t txbuf[2] __attribute__ ((section(".sram2")));
	static uint8_t rxbuf[2] __attribute__ ((section(".sram2")));
	struct mpr121_state *s = (struct mpr121_state *)arg;
	s->tx = txbuf;
	s->rx = rxbuf;

	mpr121_info(s, "starting thread");

	// reset the device
	int rc = mpr121_wr8(s, MPR121_SOFTRESET, 0x63);
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
	// set touch/release thresholds
	for (int i = 0; i < 12; i++) {
		mpr121_wr8(s, MPR121_E0TTH + (2 * i), s->tth);
		mpr121_wr8(s, MPR121_E0RTH + (2 * i), s->rth);
	}
	mpr121_wr8(s, MPR121_MHDR, 0x01);
	mpr121_wr8(s, MPR121_NHDR, 0x01);
	mpr121_wr8(s, MPR121_NCLR, 0x0E);
	mpr121_wr8(s, MPR121_MHDF, 0x01);
	mpr121_wr8(s, MPR121_NHDF, 0x05);
	mpr121_wr8(s, MPR121_NCLF, 0x01);
	mpr121_wr8(s, MPR121_CONFIG1, 0x10);	// default, 16uA charge current
	mpr121_wr8(s, MPR121_CONFIG2, 0x20);	// 0.5uS encoding, 1ms period
	mpr121_wr8(s, MPR121_ECR, 0x8F);	// start with first 5 bits of baseline tracking

	while (!chThdShouldTerminate()) {
		chSysLock();
		s->touch += 1;
		chSysUnlock();
		chThdSleepMilliseconds(500);
	}

 exit:
	mpr121_info(s, "stopping thread");
	chThdExit((msg_t) 0);
}

//-----------------------------------------------------------------------------

static void mpr121_init(struct mpr121_state *s, i2caddr_t adr) {
	// initialise the state
	memset(s, 0, sizeof(struct mpr121_state));
	s->adr = adr;
	s->tth = 12;		// touch threshold
	s->rth = 6;		// release threshold

	// initialise the pins
	palSetPadMode(GPIOB, 8, PAL_MODE_ALTERNATE(4) | PAL_STM32_PUDR_PULLUP | PAL_STM32_OTYPE_OPENDRAIN);	// SCL
	palSetPadMode(GPIOB, 9, PAL_MODE_ALTERNATE(4) | PAL_STM32_PUDR_PULLUP | PAL_STM32_OTYPE_OPENDRAIN);	// SDA

	// start i2c
	I2CConfig i2cfg = {
		OPMODE_I2C,
		400000,
		STD_DUTY_CYCLE,
	};
	i2cStart(&I2CD1, &i2cfg);

	// create the mpr121 polling thread
	s->thd = chThdCreateStatic(s->thd_wa, sizeof(s->thd_wa), NORMALPRIO, mpr121_thread, (void *)s);
}

static void mpr121_dispose(struct mpr121_state *s) {
	// stop thread
	chThdTerminate(s->thd);
	chThdWait(s->thd);

	// stop i2c
	i2cStop(&I2CD1);

	// restore the pins
	palSetPadMode(GPIOB, 8, PAL_MODE_INPUT_ANALOG);
	palSetPadMode(GPIOB, 9, PAL_MODE_INPUT_ANALOG);
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
