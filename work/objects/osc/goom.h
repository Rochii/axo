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
};

//-----------------------------------------------------------------------------

static inline void goom_init(struct goom_state *s) {
	memset(s, 0, sizeof(staruct goom_state));
}

//-----------------------------------------------------------------------------

static inline void goom_wave(struct goom_state *s, const int32_t inlet_pitch, const int32buffer inlet_freq, const int32buffer inlet_phase, int32buffer & outlet_wave, int param_pitch, int param_duty, int param_slope) {
	int32_t freq;
	MTOFEXTENDED(param_pitch + inlet_pitch, freq);
	for (int i = 0; i < BUFSIZE; i++) {
		int32_t r;
		s->phase += freq + inlet_freq[i];
		int32_t p2 = s->phase + (inlet_phase[i] << 4);
		SINE2TINTERP(p2, r) outlet_wave[i] = (r >> 4);
	}
}

//-----------------------------------------------------------------------------

#endif				// DEADSY_GOOM_H

//-----------------------------------------------------------------------------
