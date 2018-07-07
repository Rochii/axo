//-----------------------------------------------------------------------------
/*

SX1509 GPIO/PWM/Keyboard Driver
Author: Jason Harris (https://github.com/deadsy)

https://www.semtech.com/uploads/documents/sx150x_89b.pdf
https://www.sparkfun.com/products/13601
http://cdn.sparkfun.com/datasheets/BreakoutBoards/sx1509.pdf

Note:
This object requires a single instance of the factory/gpio/i2c/config object.
This allows multiple devices (each with a unique i2c address) to work concurrently.
Tested with I2C1, SCL=PB8, SDA=PB9 (these are the config defaults)

*/
//-----------------------------------------------------------------------------

#ifndef DEADSY_SX1509_H
#define DEADSY_SX1509_H

//-----------------------------------------------------------------------------
// registers

// Device and IO Banks
#define SX1509_INPUT_DISABLE_B      0x00	// RegInputDisableB Input buffer disable register _ I/O[15_8] (Bank B) 0000 0000
#define SX1509_INPUT_DISABLE_A      0x01	// RegInputDisableA Input buffer disable register _ I/O[7_0] (Bank A) 0000 0000
#define SX1509_LONG_SLEW_B          0x02	// RegLongSlewB Output buffer long slew register _ I/O[15_8] (Bank B) 0000 0000
#define SX1509_LONG_SLEW_A          0x03	// RegLongSlewA Output buffer long slew register _ I/O[7_0] (Bank A) 0000 0000
#define SX1509_LOW_DRIVE_B          0x04	// RegLowDriveB Output buffer low drive register _ I/O[15_8] (Bank B) 0000 0000
#define SX1509_LOW_DRIVE_A          0x05	// RegLowDriveA Output buffer low drive register _ I/O[7_0] (Bank A) 0000 0000
#define SX1509_PULL_UP_B            0x06	// RegPullUpB Pull_up register _ I/O[15_8] (Bank B) 0000 0000
#define SX1509_PULL_UP_A            0x07	// RegPullUpA Pull_up register _ I/O[7_0] (Bank A) 0000 0000
#define SX1509_PULL_DOWN_B          0x08	// RegPullDownB Pull_down register _ I/O[15_8] (Bank B) 0000 0000
#define SX1509_PULL_DOWN_A          0x09	// RegPullDownA Pull_down register _ I/O[7_0] (Bank A) 0000 0000
#define SX1509_OPEN_DRAIN_B         0x0A	// RegOpenDrainB Open drain register _ I/O[15_8] (Bank B) 0000 0000
#define SX1509_OPEN_DRAIN_A         0x0B	// RegOpenDrainA Open drain register _ I/O[7_0] (Bank A) 0000 0000
#define SX1509_POLARITY_B           0x0C	// RegPolarityB Polarity register _ I/O[15_8] (Bank B) 0000 0000
#define SX1509_POLARITY_A           0x0D	// RegPolarityA Polarity register _ I/O[7_0] (Bank A) 0000 0000
#define SX1509_DIR_B                0x0E	// RegDirB Direction register _ I/O[15_8] (Bank B) 1111 1111
#define SX1509_DIR_A                0x0F	// RegDirA Direction register _ I/O[7_0] (Bank A) 1111 1111
#define SX1509_DATA_B               0x10	// RegDataB Data register _ I/O[15_8] (Bank B) 1111 1111*
#define SX1509_DATA_A               0x11	// RegDataA Data register _ I/O[7_0] (Bank A) 1111 1111*
#define SX1509_INTERRUPT_MASK_B     0x12	// RegInterruptMaskB Interrupt mask register _ I/O[15_8] (Bank B) 1111 1111
#define SX1509_INTERRUPT_MASK_A     0x13	// RegInterruptMaskA Interrupt mask register _ I/O[7_0] (Bank A) 1111 1111
#define SX1509_SENSE_HIGH_B         0x14	// RegSenseHighB Sense register for I/O[15:12] 0000 0000
#define SX1509_SENSE_LOW_B          0x15	// RegSenseLowB Sense register for I/O[11:8] 0000 0000
#define SX1509_SENSE_HIGH_A         0x16	// RegSenseHighA Sense register for I/O[7:4] 0000 0000
#define SX1509_SENSE_LOW_A          0x17	// RegSenseLowA Sense register for I/O[3:0] 0000 0000
#define SX1509_INTERRUPT_SOURCE_B   0x18	// RegInterruptSourceB Interrupt source register _ I/O[15_8] (Bank B) 0000 0000
#define SX1509_INTERRUPT_SOURCE_A   0x19	// RegInterruptSourceA Interrupt source register _ I/O[7_0] (Bank A) 0000 0000
#define SX1509_EVENT_STATUS_B       0x1A	// RegEventStatusB Event status register _ I/O[15_8] (Bank B) 0000 0000
#define SX1509_EVENT_STATUS_A       0x1B	// RegEventStatusA Event status register _ I/O[7_0] (Bank A) 0000 0000
#define SX1509_LEVEL_SHIFTER_1      0x1C	// RegLevelShifter1 Level shifter register 0000 0000
#define SX1509_LEVEL_SHIFTER_2      0x1D	// RegLevelShifter2 Level shifter register 0000 0000
#define SX1509_CLOCK                0x1E	// RegClock Clock management register 0000 0000
#define SX1509_MISC                 0x1F	// RegMisc Miscellaneous device settings register 0000 0000
#define SX1509_LED_DRIVER_ENABLE_B  0x20	// RegLEDDriverEnableB LED driver enable register _ I/O[15_8] (Bank B) 0000 0000
#define SX1509_LED_DRIVER_ENABLE_A  0x21	// RegLEDDriverEnableA LED driver enable register _ I/O[7_0] (Bank A) 0000 0000
// Debounce and Keypad Engine
#define SX1509_DEBOUNCE_CONFIG      0x22	// RegDebounceConfig Debounce configuration register 0000 0000
#define SX1509_DEBOUNCE_ENABLE_B    0x23	// RegDebounceEnableB Debounce enable register _ I/O[15_8] (Bank B) 0000 0000
#define SX1509_DEBOUNCE_ENABLE_A    0x24	// RegDebounceEnableA Debounce enable register _ I/O[7_0] (Bank A) 0000 0000
#define SX1509_KEY_CONFIG_1         0x25	// RegKeyConfig1 Key scan configuration register 0000 0000
#define SX1509_KEY_CONFIG_2         0x26	// RegKeyConfig2 Key scan configuration register 0000 0000
#define SX1509_KEY_DATA_1           0x27	// RegKeyData1 Key value (column) 1111 1111
#define SX1509_KEY_DATA_2           0x28	// RegKeyData2 Key value (row) 1111 1111
// LED Driver (PWM, blinking, breathing)
#define SX1509_T_ON_0               0x29	// RegTOn0 ON time register for I/O[0] 0000 0000
#define SX1509_I_ON_0               0x2A	// RegIOn0 ON intensity register for I/O[0] 1111 1111
#define SX1509_OFF_0                0x2B	// RegOff0 OFF time/intensity register for I/O[0] 0000 0000
#define SX1509_T_ON_1               0x2C	// RegTOn1 ON time register for I/O[1] 0000 0000
#define SX1509_I_ON_1               0x2D	// RegIOn1 ON intensity register for I/O[1] 1111 1111
#define SX1509_OFF_1                0x2E	// RegOff1 OFF time/intensity register for I/O[1] 0000 0000
#define SX1509_T_ON_2               0x2F	// RegTOn2 ON time register for I/O[2] 0000 0000
#define SX1509_I_ON_2               0x30	// RegIOn2 ON intensity register for I/O[2] 1111 1111
#define SX1509_OFF_2                0x31	// RegOff2 OFF time/intensity register for I/O[2] 0000 0000
#define SX1509_T_ON_3               0x32	// RegTOn3 ON time register for I/O[3] 0000 0000
#define SX1509_I_ON_3               0x33	// RegIOn3 ON intensity register for I/O[3] 1111 1111
#define SX1509_OFF_3                0x34	// RegOff3 OFF time/intensity register for I/O[3] 0000 0000
#define SX1509_T_ON_4               0x35	// RegTOn4 ON time register for I/O[4] 0000 0000
#define SX1509_I_ON_4               0x36	// RegIOn4 ON intensity register for I/O[4] 1111 1111
#define SX1509_OFF_4                0x37	// RegOff4 OFF time/intensity register for I/O[4] 0000 0000
#define SX1509_T_RISE_4             0x38	// RegTRise4 Fade in register for I/O[4] 0000 0000
#define SX1509_T_FALL_4             0x39	// RegTFall4 Fade out register for I/O[4] 0000 0000
#define SX1509_T_ON_5               0x3A	// RegTOn5 ON time register for I/O[5] 0000 0000
#define SX1509_I_ON_5               0x3B	// RegIOn5 ON intensity register for I/O[5] 1111 1111
#define SX1509_OFF_5                0x3C	// RegOff5 OFF time/intensity register for I/O[5] 0000 0000
#define SX1509_T_RISE_5             0x3D	// RegTRise5 Fade in register for I/O[5] 0000 0000
#define SX1509_T_FALL_5             0x3E	// RegTFall5 Fade out register for I/O[5] 0000 0000
#define SX1509_T_ON_6               0x3F	// RegTOn6 ON time register for I/O[6] 0000 0000
#define SX1509_I_ON_6               0x40	// RegIOn6 ON intensity register for I/O[6] 1111 1111
#define SX1509_OFF_6                0x41	// RegOff6 OFF time/intensity register for I/O[6] 0000 0000
#define SX1509_T_RISE_6             0x42	// RegTRise6 Fade in register for I/O[6] 0000 0000
#define SX1509_T_FALL_6             0x43	// RegTFall6 Fade out register for I/O[6] 0000 0000
#define SX1509_T_ON_7               0x44	// RegTOn7 ON time register for I/O[7] 0000 0000
#define SX1509_I_ON_7               0x45	// RegIOn7 ON intensity register for I/O[7] 1111 1111
#define SX1509_OFF_7                0x46	// RegOff7 OFF time/intensity register for I/O[7] 0000 0000
#define SX1509_T_RISE_7             0x47	// RegTRise7 Fade in register for I/O[7] 0000 0000
#define SX1509_T_FALL_7             0x48	// RegTFall7 Fade out register for I/O[7] 0000 0000
#define SX1509_T_ON_8               0x49	// RegTOn8 ON time register for I/O[8] 0000 0000
#define SX1509_I_ON_8               0x4A	// RegIOn8 ON intensity register for I/O[8] 1111 1111
#define SX1509_OFF_8                0x4B	// RegOff8 OFF time/intensity register for I/O[8] 0000 0000
#define SX1509_T_ON_9               0x4C	// RegTOn9 ON time register for I/O[9] 0000 0000
#define SX1509_I_ON_9               0x4D	// RegIOn9 ON intensity register for I/O[9] 1111 1111
#define SX1509_OFF_9                0x4E	// RegOff9 OFF time/intensity register for I/O[9] 0000 0000
#define SX1509_T_ON_10              0x4F	// RegTOn10 ON time register for I/O[10] 0000 0000
#define SX1509_I_ON_10              0x50	// RegIOn10 ON intensity register for I/O[10] 1111 1111
#define SX1509_OFF_10               0x51	// RegOff10 OFF time/intensity register for I/O[10] 0000 0000
#define SX1509_T_ON_11              0x52	// RegTOn11 ON time register for I/O[11] 0000 0000
#define SX1509_I_ON_11              0x53	// RegIOn11 ON intensity register for I/O[11] 1111 1111
#define SX1509_OFF_11               0x54	// RegOff11 OFF time/intensity register for I/O[11] 0000 0000
#define SX1509_T_ON_12              0x55	// RegTOn12 ON time register for I/O[12] 0000 0000
#define SX1509_I_ON_12              0x56	// RegIOn12 ON intensity register for I/O[12] 1111 1111
#define SX1509_OFF_12               0x57	// RegOff12 OFF time/intensity register for I/O[12] 0000 0000
#define SX1509_T_RISE_12            0x58	// RegTRise12 Fade in register for I/O[12] 0000 0000
#define SX1509_T_FALL_12            0x59	// RegTFall12 Fade out register for I/O[12] 0000 0000
#define SX1509_T_ON_13              0x5A	// RegTOn13 ON time register for I/O[13] 0000 0000
#define SX1509_I_ON_13              0x5B	// RegIOn13 ON intensity register for I/O[13] 1111 1111
#define SX1509_OFF_13               0x5C	// RegOff13 OFF time/intensity register for I/O[13] 0000 0000
#define SX1509_T_RISE_13            0x5D	// RegTRise13 Fade in register for I/O[13] 0000 0000
#define SX1509_T_FALL_13            0x5E	// RegTFall13 Fade out register for I/O[13] 0000 0000
#define SX1509_T_ON_14              0x5F	// RegTOn14 ON time register for I/O[14] 0000 0000
#define SX1509_I_ON_14              0x60	// RegIOn14 ON intensity register for I/O[14] 1111 1111
#define SX1509_OFF_14               0x61	// RegOff14 OFF time/intensity register for I/O[14] 0000 0000
#define SX1509_T_RISE_14            0x62	// RegTRise14 Fade in register for I/O[14] 0000 0000
#define SX1509_T_FALL_14            0x63	// RegTFall14 Fade out register for I/O[14] 0000 0000
#define SX1509_T_ON_15              0x64	// RegTOn15 ON time register for I/O[15] 0000 0000
#define SX1509_I_ON_15              0x65	// RegIOn15 ON intensity register for I/O[15] 1111 1111
#define SX1509_OFF_15               0x66	// RegOff15 OFF time/intensity register for I/O[15] 0000 0000
#define SX1509_T_RISE_15            0x67	// RegTRise15 Fade in register for I/O[15] 0000 0000
#define SX1509_T_FALL_15            0x68	// RegTFall15 Fade out register for I/O[15] 0000 0000
// Miscellaneous
#define SX1509_HIGH_INPUT_B         0x69	// RegHighInputB High input enable register _ I/O[15_8] (Bank B) 0000 0000
#define SX1509_HIGH_INPUT_A         0x6A	// RegHighInputA High input enable register _ I/O[7_0] (Bank A) 0000 0000
// Software Reset
#define SX1509_RESET                0x7D	// RegReset Software reset register 0000 0000
// Test
#define SX1509_TEST_1               0x7E	// RegTest1 Test register 0000 0000
#define SX1509_TEST_2               0x7F	// RegTest2 Test register 0000 0000

//-----------------------------------------------------------------------------

#define SX1509_I2C_TIMEOUT 30	// chibios ticks

//-----------------------------------------------------------------------------

// sx1509 configuration
struct sx1509_cfg {
	uint8_t reg;
	uint8_t val;
};

// sx1509 state variables
struct sx1509_state {
	stkalign_t thd_wa[THD_WA_SIZE(512) / sizeof(stkalign_t)];	// thread working area
	Thread *thd;		// thread pointer
	const struct sx1509_cfg *cfg;	// driver configuration
	I2CDriver *dev;		// i2c bus driver
	i2caddr_t adr;		// i2c device address
	uint8_t *tx;		// i2c tx buffer
	uint8_t *rx;		// i2c rx buffer
};

//-----------------------------------------------------------------------------

// TODO - size properly

// Allocate a 32-bit aligned buffer of size bytes from sram2.
// The memory pool is big enough for 4 concurrent devices.
static void *sx1509_malloc(size_t size) {
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
static int sx1509_rd8(struct sx1509_state *s, uint8_t reg, uint8_t * val) {
	s->tx[0] = reg;
	i2cAcquireBus(s->dev);
	msg_t rc = i2cMasterTransmitTimeout(s->dev, s->adr, s->tx, 1, s->rx, 1, SX1509_I2C_TIMEOUT);
	i2cReleaseBus(s->dev);
	*val = *(uint8_t *) s->rx;
	return (rc == RDY_OK) ? 0 : -1;
}

// read a 16 bit value from a register
static int sx1509_rd16(struct sx1509_state *s, uint8_t reg, uint16_t * val) {
	s->tx[0] = reg;
	i2cAcquireBus(s->dev);
	msg_t rc = i2cMasterTransmitTimeout(s->dev, s->adr, s->tx, 1, s->rx, 2, SX1509_I2C_TIMEOUT);
	i2cReleaseBus(s->dev);
	*val = *(uint16_t *) s->rx;
	return (rc == RDY_OK) ? 0 : -1;
}

// write an 8 bit value to a register
static int sx1509_wr8(struct sx1509_state *s, uint8_t reg, uint8_t val) {
	s->tx[0] = reg;
	s->tx[1] = val;
	i2cAcquireBus(s->dev);
	msg_t rc = i2cMasterTransmitTimeout(s->dev, s->adr, s->tx, 2, NULL, 0, SX1509_I2C_TIMEOUT);
	i2cReleaseBus(s->dev);
	return (rc == RDY_OK) ? 0 : -1;
}

//-----------------------------------------------------------------------------

// reset the device
static int sx1509_reset(struct sx1509_state *s) {
	int rc = 0;
	rc |= sx1509_wr8(s, SX1509_RESET, 0x12);
	rc |= sx1509_wr8(s, SX1509_RESET, 0x34);
	return rc;
}

// read the current key data
static int sx1509_rd_key(struct sx1509_state *s, uint16_t * val) {
	int rc = sx1509_rd16(s, SX1509_KEY_DATA_1, val);
	*val ^= 0xffff;
	return rc;
}

//-----------------------------------------------------------------------------

static void sx1509_info(struct sx1509_state *s, const char *msg) {
	LogTextMessage("sx1509(0x%x) %s", s->adr, msg);
}

static void sx1509_error(struct sx1509_state *s, const char *msg) {
	sx1509_info(s, msg);
	// wait for the parent thread to kill us
	while (!chThdShouldTerminate()) {
		chThdSleepMilliseconds(100);
	}
}

//-----------------------------------------------------------------------------

/*

static const EXTConfig extcfg = {
  {
    {EXT_CH_MODE_BOTH_EDGES | EXT_CH_MODE_AUTOSTART | EXT_MODE_GPIOA, extcb1},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL}
  }
};

#define STM32_EXTI0_HANDLER Vector58

CH_IRQ_HANDLER(STM32_EXTI0_HANDLER) {
	CH_IRQ_PROLOGUE();
	chSysLockFromIsr();
	chEvtSignalI(NULL, (eventmask_t) 1);
	chSysUnlockFromIsr();
	CH_IRQ_EPILOGUE();
}

*/

//-----------------------------------------------------------------------------

static msg_t sx1509_thread(void *arg) {
	struct sx1509_state *s = (struct sx1509_state *)arg;
	int rc = 0;
	int idx = 0;

	sx1509_info(s, "starting thread");

	// allocate i2c buffers
	s->tx = (uint8_t *) sx1509_malloc(2);
	s->rx = (uint8_t *) sx1509_malloc(2);
	if (s->rx == NULL || s->tx == NULL) {
		sx1509_error(s, "out of memory");
		goto exit;
	}
	// reset the device
	rc = sx1509_reset(s);
	if (rc < 0) {
		sx1509_error(s, "i2c error");
		goto exit;
	}
	// check the expected default values for some registers
	uint8_t val0, val1;
	sx1509_rd8(s, SX1509_INTERRUPT_MASK_A, &val0);
	sx1509_rd8(s, SX1509_SENSE_HIGH_B, &val1);
	if (val0 != 0xff || val1 != 0) {
		sx1509_error(s, "bad register values");
		goto exit;
	}
	// apply the per-object register configuration
	while (s->cfg[idx].reg != 0xff) {
		sx1509_wr8(s, s->cfg[idx].reg, s->cfg[idx].val);
		idx += 1;
	}

	//extStart(&EXTD1, &extcfg);

	uint16_t oldval;

	while (!chThdShouldTerminate()) {
		uint16_t val;
		sx1509_rd_key(s, &val);
		if (val != oldval && val != 0) {
			LogTextMessage("%04x", val);
		}
		oldval = val;
		chThdSleepMilliseconds(20);
	}

 exit:
	sx1509_info(s, "stopping thread");
	chThdExit((msg_t) 0);
}

//-----------------------------------------------------------------------------

static void sx1509_init(struct sx1509_state *s, const struct sx1509_cfg *cfg, i2caddr_t adr) {
	// initialise the state
	memset(s, 0, sizeof(struct sx1509_state));
	s->cfg = cfg;
	s->dev = &I2CD1;
	s->adr = adr;
	// create the polling thread
	s->thd = chThdCreateStatic(s->thd_wa, sizeof(s->thd_wa), NORMALPRIO, sx1509_thread, (void *)s);
}

static void sx1509_dispose(struct sx1509_state *s) {
	// stop thread
	chThdTerminate(s->thd);
	chThdWait(s->thd);
}

// krate key function (they are the same for all object variants)
static void sx1509_key(struct sx1509_state *s, int32_t * key) {
}

//-----------------------------------------------------------------------------

#endif				// DEADSY_SX1509_H

//-----------------------------------------------------------------------------
