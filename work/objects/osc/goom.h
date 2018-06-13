//-----------------------------------------------------------------------------
/*

Goom Wave Oscillator

*/
//-----------------------------------------------------------------------------

#ifndef DEADSY_GOOM_H
#define DEADSY_GOOM_H

//-----------------------------------------------------------------------------

struct goom_state {
	uint32_t phase;		// q12.20
	int32_t duty;
};

//-----------------------------------------------------------------------------

static inline void goom_init(struct goom_state *s) {
	memset(s, 0, sizeof(struct goom_state));
}

//-----------------------------------------------------------------------------

static inline void goom_wave(struct goom_state *s,	// state
			     int32_t * out,	// frac32buffer.bipolar q5.27 [-1,1]
			     int32_t duty,	// duty cycle q11.21 [0..64]
			     int32_t slope	// slope q11.21 [0..64]
    ) {

	if (duty != s->duty) {
		LogTextMessage("duty: %08x", (uint32_t) duty);
		s->duty = duty;
	}

	int32_t pitch = 16 << 21;
	int32_t freq = mtof48k_ext_q31(pitch);

	for (size_t i = 0; i < BUFSIZE; i++) {
		s->phase += freq;
		out[i] = sin_q31(s->phase) >> 4;	// q1.31 -> q5.27
	}

}

//-----------------------------------------------------------------------------

#endif				// DEADSY_GOOM_H

//-----------------------------------------------------------------------------
