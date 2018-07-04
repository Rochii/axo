//-----------------------------------------------------------------------------
/*

Goom Wave Oscillator

A Goom Wave is a wave shape with the following segments:

1) s0: A falling (1 to -1) sine curve
2) f0: A flat piece at the bottom
3) s1: A rising (-1 to 1) sine curve
4) f1: A flat piece at the top

Shape is controlled by two parameters:
duty = split the total period between s0,f0 and s1,f1
slope = split s0f0 and s1f1 beween slope and flat.

The idea for goom waves comes from: https://www.quinapalus.com/goom.html

*/
//-----------------------------------------------------------------------------

#ifndef DEADSY_GOOM_H
#define DEADSY_GOOM_H

//-----------------------------------------------------------------------------

// Limit how close the duty cycle can get to 0/100%.
#define TP_MIN 0.1f

// Limit how fast the slope can rise.
#define SLOPE_MIN 0.1f

#define FULL_CYCLE ((float)(1ULL << 32))
#define CYCLE_1_2 ((uint32_t)(1U << 31))
#define CYCLE_1_4 ((uint32_t)(1U << 30))
#define CYCLE_3_4 (CYCLE_1_2 + CYCLE_1_4)

//-----------------------------------------------------------------------------

// map a 0..127 midi control value from a..b
float midi_map(uint8_t val, float a, float b) {
	return a + ((b - a) / 127.f) * (float)(val & 0x7f);
}

//-----------------------------------------------------------------------------

struct goom_state {
	uint8_t duty;		// duty cycle 0..127
	uint8_t slope;		// slope 0..127
	float k0;		// scaling factor for slope 0
	float k1;		// scaling factor for slope 1
	uint32_t tp;		// s0f0 to s1f1 transition point
	uint32_t x;		// phase position
	uint32_t xstep;		// phase step per sample
};

//-----------------------------------------------------------------------------

static void goom_init(struct goom_state *s) {
	memset(s, 0, sizeof(struct goom_state));
	// force an initial update
	s->duty = 0xff;
}

//-----------------------------------------------------------------------------

static void goom_krate(struct goom_state *s,	// state
		       int32_t pitch,	// inlet q11.21
		       const int32_t * freq,	// inlet q5.27
		       const int32_t * phase,	// inlet q5.27
		       int32_t duty,	// inlet 0..127
		       int32_t slope,	// inlet 0..127
		       int32_t * out	// outlet q5.27
    ) {

	// do we need to change the wave shape?
	if ((duty != s->duty) || (slope != s->slope)) {
		s->duty = duty;
		s->slope = slope;
		// map midi control values to the range limits
		float fduty = midi_map(duty, TP_MIN, 1.f - TP_MIN);
		float fslope = midi_map(slope, SLOPE_MIN, 1.f);
		// This is where we transition from s0f0 to s1f1.
		s->tp = (uint32_t) (FULL_CYCLE * fduty);
		// scaling constant for s0, map the slope to the LUT.
		s->k0 = 1.f / ((float)s->tp * fslope);
		// scaling constant for s1, map the slope to the LUT.
		s->k1 = 1.f / ((FULL_CYCLE - (float)s->tp) * fslope);
	}

	s->xstep = mtof48k_ext_q31(pitch);

	for (size_t i = 0; i < BUFSIZE; i++) {
		uint32_t ofs;
		float x;
		// what portion of the goom wave are we in?
		if (s->x < s->tp) {
			// we are in the s0/f0 portion
			x = (float)s->x * s->k0;
			ofs = CYCLE_1_4;
		} else {
			// we are in the s1/f1 portion
			x = (float)(s->x - s->tp) * s->k1;
			ofs = CYCLE_3_4;
		}
		x = (x > 1.f) ? 1.f : x;
		out[i] = sin_q31((uint32_t) (x * (float)CYCLE_1_2) + ofs) >> 4;
		// step the phase
		s->x += s->xstep;
	}

}

//-----------------------------------------------------------------------------

#endif				// DEADSY_GOOM_H

//-----------------------------------------------------------------------------
