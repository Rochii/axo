//-----------------------------------------------------------------------------
/*

Autoharp Strummer

*/
//-----------------------------------------------------------------------------

#ifndef DEADSY_STRUM_H
#define DEADSY_STRUM_H

//-----------------------------------------------------------------------------

// strum configuration
struct strum_cfg {
	int touch_bits;		// number of touch bits
};

// strum state variables
struct strum_state {
	uint32_t touch_mask;
};

//-----------------------------------------------------------------------------

static void strum_init(struct strum_state *s, const struct strum_cfg *cfg) {
	// initialise the state
	memset(s, 0, sizeof(struct strum_state));
	s->touch_mask = (1ULL << cfg->touch_bits) - 1;
}

//-----------------------------------------------------------------------------

static void strum_krate(struct strum_state *s, int32_t key, int32_t touch) {
	touch &= s->touch_mask;
}

//-----------------------------------------------------------------------------

#endif				// DEADSY_STRUM_H

//-----------------------------------------------------------------------------
