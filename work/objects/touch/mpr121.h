//-----------------------------------------------------------------------------
/*

MPR121 Capacitive Touch Sensor (I2C).
https://www.adafruit.com/product/1982

Pins:
Vin - voltage input 3.3v/5v (to axoloti)
3Vo - voltage output (nc)
GND - ground (to axoloti gnd)
ADDR - address select (nc == 0x5a)
SDA - serial data (to PB9)
SCL - serial clock (to PB8)
IRQ - interrupt request (nc - we poll it)

*/
//-----------------------------------------------------------------------------

#ifndef DEADSY_MPR121_H
#define DEADSY_MPR121_H

//-----------------------------------------------------------------------------

// registers
#define MPR121_TOUCHSTATUS_L 0x00
#define MPR121_TOUCHSTATUS_H 0x01
#define MPR121_FILTDATA_0L   0x04
#define MPR121_FILTDATA_0H   0x05
#define MPR121_BASELINE_0    0x1E
#define MPR121_MHDR          0x2B
#define MPR121_NHDR          0x2C
#define MPR121_NCLR          0x2D
#define MPR121_FDLR          0x2E
#define MPR121_MHDF          0x2F
#define MPR121_NHDF          0x30
#define MPR121_NCLF          0x31
#define MPR121_FDLF          0x32
#define MPR121_NHDT          0x33
#define MPR121_NCLT          0x34
#define MPR121_FDLT          0x35
#define MPR121_TOUCHTH_0     0x41
#define MPR121_RELEASETH_0   0x42
#define MPR121_DEBOUNCE      0x5B
#define MPR121_CONFIG1       0x5C
#define MPR121_CONFIG2       0x5D
#define MPR121_CHARGECURR_0  0x5F
#define MPR121_CHARGETIME_1  0x6C
#define MPR121_ECR           0x5E
#define MPR121_AUTOCONFIG0   0x7B
#define MPR121_AUTOCONFIG1   0x7C
#define MPR121_UPLIMIT       0x7D
#define MPR121_LOWLIMIT      0x7E
#define MPR121_TARGETLIMIT   0x7F
#define MPR121_GPIODIR       0x76
#define MPR121_GPIOEN        0x77
#define MPR121_GPIOSET       0x78
#define MPR121_GPIOCLR       0x79
#define MPR121_GPIOTOGGLE    0x7A
#define MPR121_SOFTRESET     0x80

//-----------------------------------------------------------------------------

struct mpr121_state {
	i2caddr_t adr;
};

//-----------------------------------------------------------------------------

static inline uint8_t mpr121_rd8(struct mpr121_state *s, uint8_t reg) {
	return 0;
}

static inline uint16_t mpr121_rd16(struct mpr121_state *s, uint8_t reg) {
	return 0;
}

static inline void mpr121_wr8(struct mpr121_state *s, uint8_t reg, uint8_t val) {
}

static inline void mpr121_set_threshold(struct mpr121_state *s, uint8_t touch, uint8_t release) {
	for (int i = 0; i < 12; i++) {
		mpr121_wr8(s, MPR121_TOUCHTH_0 + (2 * i), touch);
		mpr121_wr8(s, MPR121_RELEASETH_0 + (2 * i), release);
	}
}

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

//-----------------------------------------------------------------------------

static void mpr121_thread_init(struct mpr121_state *s) {
	// soft reset
	mpr121_wr8(s, MPR121_SOFTRESET, 0x63);
	chThdSleepMilliseconds(1);

	mpr121_wr8(s, MPR121_ECR, 0);
	mpr121_set_threshold(s, 12, 6);
	mpr121_wr8(s, MPR121_MHDR, 0x01);
	mpr121_wr8(s, MPR121_NHDR, 0x01);
	mpr121_wr8(s, MPR121_NCLR, 0x0E);
	mpr121_wr8(s, MPR121_FDLR, 0);
	mpr121_wr8(s, MPR121_MHDF, 0x01);
	mpr121_wr8(s, MPR121_NHDF, 0x05);
	mpr121_wr8(s, MPR121_NCLF, 0x01);
	mpr121_wr8(s, MPR121_FDLF, 0);
	mpr121_wr8(s, MPR121_NHDT, 0);
	mpr121_wr8(s, MPR121_NCLT, 0);
	mpr121_wr8(s, MPR121_FDLT, 0);
	mpr121_wr8(s, MPR121_DEBOUNCE, 0);
	mpr121_wr8(s, MPR121_CONFIG1, 0x10);	// default, 16uA charge current
	mpr121_wr8(s, MPR121_CONFIG2, 0x20);	// 0.5uS encoding, 1ms period

	// enable all electrodes
	mpr121_wr8(s, MPR121_ECR, 0x8f);	// start with first 5 bits of baseline tracking
}

static void mpr121_thread_loop(void) {
}

static msg_t mpr121_thread(void *arg) {
	mpr121_thread_init(0 /*TODO*/);
	while (!chThdShouldTerminate()) {
		mpr121_thread_loop();
		chThdSleepMilliseconds(1);
	}
	chThdExit((msg_t) 0);
}

//-----------------------------------------------------------------------------

static inline void mpr121_init(struct mpr121_state *s, i2caddr_t adr) {
	// initialise the state
	memset(s, 0, sizeof(struct mpr121_state));
	s->adr = adr;
	// initialise the pins
	palSetPadMode(GPIOB, 8, PAL_MODE_ALTERNATE(4) | PAL_STM32_PUDR_PULLUP | PAL_STM32_OTYPE_OPENDRAIN);	// SCL
	palSetPadMode(GPIOB, 9, PAL_MODE_ALTERNATE(4) | PAL_STM32_PUDR_PULLUP | PAL_STM32_OTYPE_OPENDRAIN);	// SDA

	// start i2c
	I2CConfig i2cfg = {
		OPMODE_I2C,
		400000,
		FAST_DUTY_CYCLE_2,
	};
	i2cStart(&I2CD1, &i2cfg);

	// TODO create thread

}

static inline void mpr121_dispose(struct mpr121_state *s) {

	// TODO terminate thread

	// stop i2c
	i2cStop(&I2CD1);

	/// restore the pins
	palSetPadMode(GPIOB, 8, PAL_MODE_INPUT_ANALOG);
	palSetPadMode(GPIOB, 9, PAL_MODE_INPUT_ANALOG);
}

//-----------------------------------------------------------------------------

static inline void mpr121_krate(struct mpr121_state *s) {
}

//-----------------------------------------------------------------------------

#endif				// DEADSY_MPR121_H

//-----------------------------------------------------------------------------
