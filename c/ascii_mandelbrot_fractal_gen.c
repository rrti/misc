#include <stdio.h>

#define NUM_CHARS_X 80
#define NUM_CHARS_Y 60

#define MIN_X_COOR -2.5
#define MIN_Y_COOR -1.5
#define MAX_X_COOR  1.5
#define MAX_Y_COOR  1.5

#define X_COOR_EXT (MAX_X_COOR - MIN_X_COOR)
#define Y_COOR_EXT (MAX_Y_COOR - MIN_Y_COOR)

#define NUM_LUT_CHARS 32
#define MAX_ITERATION 512

// missing elements become zeros (non-printable)
static const char CHAR_LOOKUP_TABLE[NUM_LUT_CHARS] = " ..^^:://II&&@@@***%%%$$$###";

unsigned int iterate_point(const double r, const double i) {
	unsigned int n = 0;

	double rr = 0.0, ii = 0.0;
	double zr = 0.0, zi = 0.0;

	while ((n < MAX_ITERATION) && ((rr + ii) < 4.0)) {
		zi = 2.0 * zr * zi + i;
		zr = rr - ii + r; 
		rr = zr * zr;
		ii = zi * zi;
		n += 1;
	}

	return n;
}

void generate_fractal(int num_chars_x, int num_chars_y) {
	for (int y = 0; y <= num_chars_y; y++){
		for (int x = 0; x < num_chars_x; x++) {
			const double xp = ((y * X_COOR_EXT) / num_chars_y) + MIN_X_COOR;
			const double yp = ((x * Y_COOR_EXT) / num_chars_x) + MIN_Y_COOR;

			printf("%c", CHAR_LOOKUP_TABLE[ iterate_point(xp, yp) % sizeof(CHAR_LOOKUP_TABLE) ]);
		}

		printf("\n");
	}
}

int main(void) {
	generate_fractal(NUM_CHARS_X, NUM_CHARS_Y);
	return 0;
}

