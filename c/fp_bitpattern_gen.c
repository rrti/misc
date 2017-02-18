#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

typedef      unsigned char uint8;
typedef      unsigned  int uint32;
typedef long           int uint64;



// single-precision (32-bits) bit-pattern layout
//   [ SIGN | EXPONENT | MANTISSA]
//   [ N=31 | 30 :: 23 |  22 :: 0]
static uint32 fp32_sign_shift() { return 31u; }
static uint32 fp32_expo_shift() { return 23u; }
static uint32 fp32_mant_shift() { return  0u; }

static uint32 fp32_max_sign_val() { return ((1u <<  1u) - 1u); }
static uint32 fp32_max_expo_val() { return ((1u <<  8u) - 1u); }
static uint32 fp32_max_mant_val() { return ((1u << 23u) - 1u); }

static uint32 fp32_expo_bias_val() { return 127u; }

// double-precision (64-bits) bit-pattern layout
//   [ SIGN | EXPONENT | MANTISSA]
//   [ N=63 | 62 :: 52 |  51 :: 0]
static uint64 fp64_sign_shift() { return 63lu; }
static uint64 fp64_expo_shift() { return 52lu; }
static uint64 fp64_mant_shift() { return  0lu; }

static uint64 fp64_max_sign_val() { return ((1lu <<  1lu) - 1lu); }
static uint64 fp64_max_expo_val() { return ((1lu << 11lu) - 1lu); }
static uint64 fp64_max_mant_val() { return ((1lu << 52lu) - 1lu); }

static uint64 fp64_expo_bias_val() { return 1023lu; }



// NOTE: should *subtract* bias from exponent (unsigned format)
//
// Exponent is either an 8-bit signed integer from -128 to +127 (2's complement)
// or an 8-bit unsigned integer from 0 to 255 which is the accepted biased form
// in IEEE 754 binary32 definition. If the unsigned integer format is used, the
// exponent value used in the arithmetic is the exponent shifted by a bias - for
// the IEEE 754 binary32 case, an exponent value of 127 represents the actual zero
// (i.e. for 2^{e - 127} to be one, e must be 127)
//
// The real value assumed by a given 32-bit pattern with a given biased exponent
// e (the 8-bit unsigned integer) and a 23-bit fraction is
//
//   (-1)^{bit{31}} * (1.bit{22}bit{21}...bit{0})_base2 * 2^{bit{30}bit{29}...bit{23} - 127}
//   (-1)^{sign}    * (1 + SUM{i=1,23} bit{23-i} * 2^(-i)) * 2^{exp - 127}
//
static uint32 fp32_bit_pattern(uint32 sign, uint32 exponent, uint32 mantissa) {
	return ((sign << fp32_sign_shift()) | ((exponent + fp32_expo_bias_val()) << fp32_expo_shift()) | (mantissa << fp32_mant_shift()));
}

static uint64 fp64_bit_pattern(uint64 sign, uint64 exponent, uint64 mantissa) {
	return ((sign << fp64_sign_shift()) | ((exponent + fp64_expo_bias_val()) << fp64_expo_shift()) | (mantissa << fp64_mant_shift()));
}



float cast_fp32_bytes(uint32 pattern, uint8 bytes[4]) {
	// note: assumes little-endian order
	bytes[3] = (pattern >> 24u) & 255u;
	bytes[2] = (pattern >> 16u) & 255u;
	bytes[1] = (pattern >>  8u) & 255u;
	bytes[0] = (pattern >>  0u) & 255u;

	// this is technically an undefined operation due to aliasing
	// let the FPU figure out what this bitpattern actually means
	// return (*(float*) &pattern);
	return (*((float*) &bytes[0]));
}

double cast_fp64_bytes(uint64 pattern, uint8 bytes[8]) {
	bytes[7] = (pattern >> 56lu) & 255lu;
	bytes[6] = (pattern >> 48lu) & 255lu;
	bytes[5] = (pattern >> 40lu) & 255lu;
	bytes[4] = (pattern >> 32lu) & 255lu;

	bytes[3] = (pattern >> 24lu) & 255lu;
	bytes[2] = (pattern >> 16lu) & 255lu;
	bytes[1] = (pattern >>  8lu) & 255lu;
	bytes[0] = (pattern >>  0lu) & 255lu;

	// return (*(double*) &pattern);
	return (*((double*) &bytes[0]));
}



void generate_floating_point_bitpatterns(unsigned int bits) {
	assert(sizeof(uint32) == 4);
	assert(sizeof(uint64) == 8);

	uint8 bytes[bits / 8];
	uint32 counts[8] = {0, 0, 0, 0, 0, 0, 0, 0};

	switch (bits) {
		case 32: {
			// mantissa is a binary fraction
			const uint32 max_sign = fp32_max_sign_val();
			const uint32 max_expo = fp32_max_expo_val();
			const uint32 max_mant = fp32_max_mant_val();

			// generate all possible floating-point patterns
			for (uint32 sign = 0; sign <= max_sign; sign++) {
				for (uint32 expo = 0; expo <= max_expo; expo++) {
					for (uint32 mant = 0; mant <= max_mant; mant++) {
						const uint32 fp32_pattern = fp32_bit_pattern(sign, expo, mant);
						const float fp32_value = cast_fp32_bytes(fp32_pattern, bytes);

						// fp32 numbers have log10(2^24) decimal digits
						// worth of precision (at least  6, at most  9)
						printf("[fp32] value=%.10f (pattern=%u :: sign=%u expo=%u mant=%u)\n", fp32_value, fp32_pattern, sign, expo, mant);
					}
				}
			}
		} break;

		case 64: {
			const uint64 max_sign = fp64_max_sign_val();
			const uint64 max_expo = fp64_max_expo_val();
			const uint64 max_mant = fp64_max_mant_val();

			for (uint64 sign = 0; sign <= max_sign; sign++) {
				for (uint64 expo = 0; expo <= max_expo; expo++) {
					for (uint64 mant = 0; mant <= max_mant; mant++) {
						const uint64 fp64_pattern = fp64_bit_pattern(sign, expo, mant);
						const double fp64_value = cast_fp64_bytes(fp64_pattern, bytes);

						// fp64 numbers have log10(2^54) decimal digits
						// worth of precision (at least 15, at most 17)
						printf("[fp64] value=%.20g (pattern=%lu :: sign=%lu expo=%lu mant=%lu)\n", fp64_value, fp64_pattern, sign, expo, mant);
					}
				}
			}
		} break;
	}
}

int main(int argc, char** argv) {
	if (argc > 1)
		generate_floating_point_bitpatterns(atoi(argv[1]));

	return 0;
}

