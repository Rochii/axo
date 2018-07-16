//-----------------------------------------------------------------------------
/*

Autoharp Strummer

*/
//-----------------------------------------------------------------------------

#ifndef DEADSY_STRUM_H
#define DEADSY_STRUM_H

//-----------------------------------------------------------------------------

#define STRUM_SIZE 16

//-----------------------------------------------------------------------------

// strum configuration
struct strum_cfg {
	int touch_bits;		// number of touch bits
};

// strum state variables
struct strum_state {
	uint32_t touch_mask;
	int root;		// current root note
	int chord;		// current chord shape
	uint8_t notes[STRUM_SIZE];	// current strum notes to be played
	uint8_t drone[STRUM_SIZE];	// current drone notes to be played
};

//-----------------------------------------------------------------------------

// root notes
enum {
	ROOT_NONE,
	ROOT_C,
	ROOT_C_SHARP,
	ROOT_D,
	ROOT_D_SHARP,
	ROOT_E,
	ROOT_F,
	ROOT_F_SHARP,
	ROOT_G,
	ROOT_G_SHARP,
	ROOT_A,
	ROOT_A_SHARP,
	ROOT_B,
};

// chord shapes
enum {
	CHORD_NONE,
	CHORD_MAJ,
	CHORD_MIN,
	CHORD_DOM7,
	CHORD_MAJ7,
	CHORD_MIN7,
	CHORD_AUG,
	CHORD_DIM,
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
