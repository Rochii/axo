//-----------------------------------------------------------------------------
/*

MPR121 Capacitive Touch Sensor (I2C).
https://www.adafruit.com/product/1982

*/
//-----------------------------------------------------------------------------

#ifndef DEADSY_MPR121_H
#define DEADSY_MPR121_H

//-----------------------------------------------------------------------------

struct mpr121_state {
	i2caddr_t adr;
};

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
