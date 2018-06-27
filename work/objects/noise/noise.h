//-----------------------------------------------------------------------------
/*

Noise Functions

https://noisehack.com/generate-noise-web-audio-api/
http://www.musicdsp.org/files/pink.txt
https://en.wikipedia.org/wiki/Pink_noise
https://en.wikipedia.org/wiki/White_noise
https://en.wikipedia.org/wiki/Brownian_noise

*/
//-----------------------------------------------------------------------------

#ifndef DEADSY_NOISE_H
#define DEADSY_NOISE_H

//-----------------------------------------------------------------------------

// convert a q1.31 to a float32
static float q31_to_float(int32_t op1) {
	float fop1 = *(float *)(&op1);
	__ASM volatile ("VCVT.F32.S32 %0, %0, 31":"+w" (fop1));
	return fop1;
}

// return a float [-1, 1)
static float rand_float(void) {
	return q31_to_float(rand_s32());
}

//-----------------------------------------------------------------------------

struct noise_state {
	float b0, b1, b2, b3, b4, b5, b6;
};

static void noise_init(struct noise_state *s) {
	memset(s, 0, sizeof(struct noise_state));
}

//-----------------------------------------------------------------------------

// white noise (spectral density = k)
static void white_noise(int32_t * out) {
	for (size_t i = 0; i < BUFSIZE; i++) {
		out[i] = rand_s32() >> 4;
	}
}

// brown noise (spectral density = k/f*f
static void brown_noise(struct noise_state *s, int32_t * out) {
	float b0 = s->b0;
	for (size_t i = 0; i < BUFSIZE; i++) {
		float white = rand_float();
		b0 = (b0 + (0.02f * white)) * (1.f / 1.02f);
		out[i] = float_to_q27(b0 * (1.f / 0.38f));
	}
	s->b0 = b0;
}

// pink noise (spectral density = k/f): fast, inaccurate version
static void pink_noise1(struct noise_state *s, int32_t * out) {
	float b0 = s->b0;
	float b1 = s->b1;
	float b2 = s->b2;
	for (size_t i = 0; i < BUFSIZE; i++) {
		float white = rand_float();
		b0 = 0.99765f * b0 + white * 0.0990460f;
		b1 = 0.96300f * b1 + white * 0.2965164f;
		b2 = 0.57000f * b2 + white * 1.0526913f;
		float pink = b0 + b1 + b2 + white * 0.1848f;
		out[i] = float_to_q27(pink * (1.f / 10.4f));
	}
	s->b0 = b0;
	s->b1 = b1;
	s->b2 = b2;
}

// pink noise (spectral density = k/f): slow, accurate version
static void pink_noise2(struct noise_state *s, int32_t * out) {
	float b0 = s->b0;
	float b1 = s->b1;
	float b2 = s->b2;
	float b3 = s->b3;
	float b4 = s->b4;
	float b5 = s->b5;
	float b6 = s->b6;
	for (size_t i = 0; i < BUFSIZE; i++) {
		float white = rand_float();
		b0 = 0.99886f * b0 + white * 0.0555179f;
		b1 = 0.99332f * b1 + white * 0.0750759f;
		b2 = 0.96900f * b2 + white * 0.1538520f;
		b3 = 0.86650f * b3 + white * 0.3104856f;
		b4 = 0.55000f * b4 + white * 0.5329522f;
		b5 = -0.7616f * b5 - white * 0.0168980f;
		float pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f;
		b6 = white * 0.115926f;
		out[i] = float_to_q27(pink * (1.f / 10.2f));
	}
	s->b0 = b0;
	s->b1 = b1;
	s->b2 = b2;
	s->b3 = b3;
	s->b4 = b4;
	s->b5 = b5;
	s->b6 = b6;
}

//-----------------------------------------------------------------------------

#endif				// DEADSY_NOISE_H

//-----------------------------------------------------------------------------
