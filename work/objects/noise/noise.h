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

#ifndef NOISE_H
#define NOISE_H

//-----------------------------------------------------------------------------

struct noise {
	float b0, b1, b2, b3, b4, b5, b6;
};

static inline void noise_init(struct noise *ptr) {
	memset(ptr, 0, sizeof(struct noise));
}

//-----------------------------------------------------------------------------

// return a float from -1..1
static inline float rand_float(void) {
	uint32_t i = (uint32_t) rand_s32();
	i |= (i << 1) & 0x80000000;
	i = (i & 0x807fffff) | (126 << 23);
	return *(float *)&i;
}

//-----------------------------------------------------------------------------

// white noise (spectral density = k)
static inline void white_noise(int32_t * out) {
	for (size_t i = 0; i < BUFSIZE; i++) {
		out[i] = float_to_q27(rand_float());
	}
}

// brown noise (spectral density = k/f*f)
static inline void brown_noise(struct noise *ptr, int32_t * out) {
	float b0 = ptr->b0;
	for (size_t i = 0; i < BUFSIZE; i++) {
		float white = rand_float();
		b0 = (b0 + (0.02f * white)) * (1.f / 1.02f);
		out[i] = float_to_q27(b0 * (1.f / 0.38f));
	}
	ptr->b0 = b0;
}

// pink noise (spectral density = k/f): fast, inaccurate version
static inline void pink_noise1(struct noise *ptr, int32_t * out) {
	float b0 = ptr->b0;
	float b1 = ptr->b1;
	float b2 = ptr->b2;
	for (size_t i = 0; i < BUFSIZE; i++) {
		float white = rand_float();
		b0 = 0.99765f * b0 + white * 0.0990460f;
		b1 = 0.96300f * b1 + white * 0.2965164f;
		b2 = 0.57000f * b2 + white * 1.0526913f;
		float pink = b0 + b1 + b2 + white * 0.1848f;
		out[i] = float_to_q27(pink * (1.f / 10.4f));
	}
	ptr->b0 = b0;
	ptr->b1 = b1;
	ptr->b2 = b2;
}

// pink noise (spectral density = k/f): slow, accurate version
static inline void pink_noise2(struct noise *ptr, int32_t * out) {
	float b0 = ptr->b0;
	float b1 = ptr->b1;
	float b2 = ptr->b2;
	float b3 = ptr->b3;
	float b4 = ptr->b4;
	float b5 = ptr->b5;
	float b6 = ptr->b6;
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
	ptr->b0 = b0;
	ptr->b1 = b1;
	ptr->b2 = b2;
	ptr->b3 = b3;
	ptr->b4 = b4;
	ptr->b5 = b5;
	ptr->b6 = b6;
}

//-----------------------------------------------------------------------------

#endif				// NOISE_H

//-----------------------------------------------------------------------------
