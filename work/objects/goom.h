//-----------------------------------------------------------------------------
/*

Goom Oscillator

*/
//-----------------------------------------------------------------------------

#ifndef GOOM_H
#define GOOM_H

//-----------------------------------------------------------------------------

struct goom_data {
  uint32_t phase;
};

//-----------------------------------------------------------------------------

static inline void
goom_init (struct goom_data *ptr) {
  ptr->phase = 0;
}

//-----------------------------------------------------------------------------

static inline void
goom_dsp (struct goom_data *ptr,	//
	  const int32_t inlet_pitch,	//
	  const int32buffer inlet_freq,	//
	  const int32buffer inlet_phase,	//
	  int32buffer & outlet_wave,	// []q5.27
	  int param_pitch,	//
	  int param_duty,	//
	  int param_slope	//
  ) {
  int32_t freq;
  MTOFEXTENDED (param_pitch + inlet_pitch, freq);
  for (int i = 0; i < BUFSIZE; i++) {
    int32_t r;
    ptr->phase += freq + inlet_freq[i];
    int32_t p2 = ptr->phase + (inlet_phase[i] << 4);
    SINE2TINTERP (p2, r) outlet_wave[i] = (r >> 4);
  }
}

//-----------------------------------------------------------------------------

#endif // GOOM_H

//-----------------------------------------------------------------------------
