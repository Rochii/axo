//-----------------------------------------------------------------------------
/*

Goom Wave Oscillator

*/
//-----------------------------------------------------------------------------

#ifndef DEADSY_GOOM_H
#define DEADSY_GOOM_H

//-----------------------------------------------------------------------------

struct goom_state {
	uint32_t phase;
	int32_t duty;
};

//-----------------------------------------------------------------------------

static inline void goom_init(struct goom_state *s) {
	memset(s, 0, sizeof(struct goom_state));
}

//-----------------------------------------------------------------------------

static inline void goom_wave(struct goom_state *s,	// state
			     int32_t * out,	// frac32buffer.bipolar q5.27 [-1,1]
			     int32_t duty,	// duty cycle
			     int32_t slope	// slope
    ) {

	if (duty != s->duty) {
		LogTextMessage("duty: %08x", (uint32_t) duty);
		s->duty = duty;
	}

	for (size_t i = 0; i < BUFSIZE; i++) {
		out[i] = 0;
	}

	/*
	   int32_t freq;
	   MTOFEXTENDED(param_pitch + inlet_pitch, freq);
	   for (int i = 0; i < BUFSIZE; i++) {
	   int32_t r;
	   s->phase += freq + inlet_freq[i];
	   int32_t p2 = s->phase + (inlet_phase[i] << 4);
	   SINE2TINTERP(p2, r) outlet_wave[i] = (r >> 4);
	   }
	 */
}

//-----------------------------------------------------------------------------

#endif				// DEADSY_GOOM_H

//-----------------------------------------------------------------------------
