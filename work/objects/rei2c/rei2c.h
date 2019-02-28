//-----------------------------------------------------------------------------
/*

I2C Rotary Encoder V2 Driver
Author: Jason Harris (https://github.com/deadsy)

https://github.com/Fattoresaimon/I2CEncoderV2
https://www.kickstarter.com/projects/1351830006/i2c-encoder-v2

*/
//-----------------------------------------------------------------------------

#ifndef DEADSY_REI2C_H
#define DEADSY_REI2C_H

//-----------------------------------------------------------------------------

#if CH_KERNEL_MAJOR == 2
#define THD_WORKING_AREA_SIZE THD_WA_SIZE
#define MSG_OK RDY_OK
#define THD_FUNCTION(tname, arg) msg_t tname(void *arg)
#endif

//-----------------------------------------------------------------------------
// registers

#define REI2C_GCONF    0x00	// General Configuration (1 byte)
#define REI2C_GP1CONF  0x01	// GP 1 Configuration (1 byte)
#define REI2C_GP2CONF  0x02	// GP 2 Configuration (1 byte)
#define REI2C_GP3CONF  0x03	// GP 3 Configuration (1 byte)
#define REI2C_INTCONF  0x04	// INT pin Configuration (1 byte)
#define REI2C_ESTATUS  0x05	// Encoder Status (1 byte)
#define REI2C_I2STATUS 0x06	// Secondary interrupt status (1 byte)
#define REI2C_FSTATUS  0x07	// Fade process status (1 byte)
#define REI2C_CVAL     0x08	// Counter Value (4 bytes)
#define REI2C_CMAX     0x0C	// Counter Max value (4 bytes)
#define REI2C_CMIN     0x10	// Counter Min value (4 bytes)
#define REI2C_ISTEP    0x14	// Increment step value (4 bytes)
#define REI2C_RLED     0x18	// LED red color intensity (1 byte)
#define REI2C_GLED     0x19	// LED green color intensity (1 byte)
#define REI2C_BLED     0x1A	// LED blue color intensity (1 byte)
#define REI2C_GP1REG   0x1B	// I/O GP1 Register (1 byte)
#define REI2C_GP2REG   0x1C	// I/O GP2 Register (1 byte)
#define REI2C_GP3REG   0x1D	// I/O GP3 Register (1 byte)
#define REI2C_ANTBOUNC 0x1E	// Anti-bouncing period (1 Byte)
#define REI2C_DPPERIOD 0x1F	// Double push period (1 Byte)
#define REI2C_FADERGB  0x20	// Fade timer RGB Encoder (1 Byte)
#define REI2C_FADEGP   0x21	// Fade timer GP ports (1 Byte)
#define REI2C_EEPROM   0x80	// EEPROM memory (128 bytes)

// REI2C_GCONF bits
#define REI2C_GCONF_DTYPE (1 << 0)	// Data type of the register: CVAL, CMAX, CMIN and ISTEP.
#define REI2C_GCONF_WRAPE (1 << 1)	// Enable counter wrap.
#define REI2C_GCONF_DIRE  (1 << 2)	// Direction of the encoder when increment.
#define REI2C_GCONF_IPUD  (1 << 3)	// Interrupt Pull-UP disable.
#define REI2C_GCONF_RMOD  (1 << 4)	// Reading Mode.
#define REI2C_GCONF_ETYPE (1 << 5)	// Set the encoder type (normal/illuminated)
#define REI2C_GCONF_MBANK (1 << 6)	// Select the EEPROM memory bank. Each bank is 128 bytes.
#define REI2C_GCONF_RESET (1 << 7)	// Reset the I2C Encoder V2

// REI2C_ESTATUS bits
#define REI2C_ESTATUS_PUSHR (1 << 0)	// push button has been released
#define REI2C_ESTATUS_PUSHP (1 << 1)	// push button has been pressed
#define REI2C_ESTATUS_PUSHD (1 << 2)	// push button has been double pushed
#define REI2C_ESTATUS_RINC  (1 << 3)	// rotated in the increase direction
#define REI2C_ESTATUS_RDEC  (1 << 4)	// rotated in the decrease direction
#define REI2C_ESTATUS_RMAX  (1 << 5)	// maximum counter value has been reached
#define REI2C_ESTATUS_RMIN  (1 << 6)	// minimum counter value has been reached
#define REI2C_ESTATUS_INT2  (1 << 7)	// Secondary interrupt status

//-----------------------------------------------------------------------------

#define REI2C_I2C_TIMEOUT 30	// chibios ticks

//-----------------------------------------------------------------------------

// rei2c configuration
struct rei2c_cfg {
	uint8_t reg;
	uint32_t val;
};

// rei2c state variables
struct rei2c_state {
	stkalign_t thd_wa[THD_WA_SIZE(512) / sizeof(stkalign_t)];	// thread working area
	Thread *thd;		// thread pointer
	const struct rei2c_cfg *cfg;	// driver configuration
	I2CDriver *dev;		// i2c bus driver
	i2caddr_t adr;		// i2c device address
	uint8_t *tx;		// i2c tx buffer
	uint8_t *rx;		// i2c rx buffer
};

//-----------------------------------------------------------------------------

// Allocate a 32-bit aligned buffer of size bytes from sram2.
// The memory pool is big enough for ? concurrent devices.
static void *rei2c_malloc(size_t size) {
	static uint8_t pool[32] __attribute__ ((section(".sram2.rei2c")));
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
static int rei2c_rd8(struct rei2c_state *s, uint8_t reg, uint8_t * val) {
	s->tx[0] = reg;
	i2cAcquireBus(s->dev);
	msg_t rc = i2cMasterTransmitTimeout(s->dev, s->adr, s->tx, 1, s->rx, 1, REI2C_I2C_TIMEOUT);
	i2cReleaseBus(s->dev);
	*val = *(uint8_t *) s->rx;
	return (rc == MSG_OK) ? 0 : -1;
}

// write an 8 bit value to a register
static int rei2c_wr8(struct rei2c_state *s, uint8_t reg, uint8_t val) {
	s->tx[0] = reg;
	s->tx[1] = val;
	i2cAcquireBus(s->dev);
	msg_t rc = i2cMasterTransmitTimeout(s->dev, s->adr, s->tx, 2, NULL, 0, REI2C_I2C_TIMEOUT);
	i2cReleaseBus(s->dev);
	return (rc == MSG_OK) ? 0 : -1;
}

// write an 32 bit value to a register
static int rei2c_wr32(struct rei2c_state *s, uint8_t reg, uint32_t val) {
	s->tx[0] = reg;
	s->tx[1] = val >> 24;
	s->tx[2] = val >> 16;
	s->tx[3] = val >> 8;
	s->tx[4] = val;

	i2cAcquireBus(s->dev);
	msg_t rc = i2cMasterTransmitTimeout(s->dev, s->adr, s->tx, 5, NULL, 0, REI2C_I2C_TIMEOUT);
	i2cReleaseBus(s->dev);
	return (rc == MSG_OK) ? 0 : -1;
}

//-----------------------------------------------------------------------------

// return non-zero if a new encoder value is available
static int rei2c_poll(struct rei2c_state *s) {
	uint8_t status;
	rei2c_rd8(s, REI2C_ESTATUS, &status);

	if (status & (REI2C_ESTATUS_RINC | REI2C_ESTATUS_RDEC)) {
	}

	if (status & REI2C_ESTATUS_PUSHR) {
	}

	if (status & REI2C_ESTATUS_PUSHP) {
	}

	if (status & REI2C_ESTATUS_PUSHD) {
	}

}

//-----------------------------------------------------------------------------

static void rei2c_info(struct rei2c_state *s, const char *msg) {
	LogTextMessage("rei2c(0x%x) %s", s->adr, msg);
}

static void rei2c_error(struct rei2c_state *s, const char *msg) {
	rei2c_info(s, msg);
	// wait for the parent thread to kill us
	while (!chThdShouldTerminate()) {
		chThdSleepMilliseconds(100);
	}
}

static THD_FUNCTION(rei2c_thread, arg) {
	struct rei2c_state *s = (struct rei2c_state *)arg;
	int idx = 0;
	int rc = 0;

	rei2c_info(s, "starting thread");

	// allocate i2c buffers
	s->tx = (uint8_t *) rei2c_malloc(5);
	s->rx = (uint8_t *) rei2c_malloc(4);
	if (s->rx == NULL || s->tx == NULL) {
		rei2c_error(s, "out of memory");
		goto exit;
	}
	// reset the chip
	rc = rei2c_wr8(s, REI2C_GCONF, REI2C_GCONF_RESET);
	if (rc < 0) {
		rei2c_error(s, "i2c error");
		goto exit;
	}
	// check some register values
	uint8_t val0, val1;
	rei2c_rd8(s, REI2C_GP1CONF, &val0);
	rei2c_rd8(s, REI2C_ANTBOUNC, &val1);
	if ((val0 != 0) || (val1 != 25)) {
		rei2c_error(s, "bad device values");
		goto exit;
	}
	// apply the per-object register configuration
	while (s->cfg[idx].reg != 0xff) {
		uint8_t reg = s->cfg[idx].reg;
		if ((reg == REI2C_CVAL) || (reg == REI2C_CMAX) || (reg == REI2C_CMIN) || (reg == REI2C_ISTEP)) {
			rei2c_wr32(s, reg, s->cfg[idx].val);
		} else {
			rei2c_wr8(s, reg, uint8_t(s->cfg[idx].val));
		}
		idx += 1;
	}

	// poll for the changes
	while (!chThdShouldTerminate()) {
		if (rei2c_poll(s)) {
			// TODO ...
		}
		// 50Hz polling interval
		chThdSleepMilliseconds(20);
	}

 exit:
	rei2c_info(s, "stopping thread");
	chThdExit((msg_t) 0);
}

//-----------------------------------------------------------------------------

static void rei2c_init(struct rei2c_state *s, const struct rei2c_cfg *cfg, i2caddr_t adr) {
	// initialise the state
	memset(s, 0, sizeof(struct rei2c_state));
	s->cfg = cfg;
	s->dev = &I2CD1;
	s->adr = adr;
	// create the polling thread
	s->thd = chThdCreateStatic(s->thd_wa, sizeof(s->thd_wa), NORMALPRIO, rei2c_thread, (void *)s);
}

static void rei2c_dispose(struct rei2c_state *s) {
	// stop thread
	chThdTerminate(s->thd);
	chThdWait(s->thd);
}

static void rei2c_krate(struct rei2c_state *s, int32_t * val) {
	chSysLock();
	chSysUnlock();
}

//-----------------------------------------------------------------------------

#endif				// DEADSY_REI2C_H

//-----------------------------------------------------------------------------
