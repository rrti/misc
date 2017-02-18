#include <stdio.h>

typedef int bool;

#define true  1
#define false 0

// number of queens to solve for (on an NxN grid)
#define N 8

#define min(a, b) ((a) < (b))? (a): (b);
#define max(a, b) ((a) > (b))? (a): (b);



void nqueens_print_grid_div() {
	printf("\t+");

	for (int col = 0; col < N; col++)
		printf("---+");

	printf("\n");
}

void nqueens_print_grid_row(bool arr[][N], int row) {
	printf("\t|");

	for (int col = 0; col < N; col++) {
		printf("%s", arr[row][col]? " Q |": "   |");
	}

	printf("\n");
}

void nqueens_print_grid(bool arr[][N]) {
	for (int row = 0; row < N; row++) {
		nqueens_print_grid_div();
		nqueens_print_grid_row(arr, row);
	}

	nqueens_print_grid_div();
}



void nqueens_init_grid(bool arr[][N]) {
	for (int row = 0; row < N; row++) {
		for (int col = 0; col < N; col++) {
			arr[row][col] = false;
		}
	}
}



bool nqueens_check_diagonals(bool arr[][N], int row, int col) {
	bool diagonals_free = true;

	for (int k = 0; (k < N) && diagonals_free; k++) {
		// TODO's
		//   1. if row reaches 0 or n-1 before col does, stop incr./decr.'ing col
		//   2. if col reaches 0 or n-1 before row does, stop incr./decr.'ing row
		const int min_row = max(    0, row - k);
		const int min_col = max(    0, col - k);
		const int max_row = min(N - 1, row + k);
		const int max_col = min(N - 1, col + k);

		const bool b0 = arr[max_row][max_col];
		const bool b1 = arr[max_row][min_col];
		const bool b2 = arr[min_row][min_col];
		const bool b3 = arr[min_row][max_col];

		diagonals_free &= !(b0 || b1 || b2 || b3);
	}

	return diagonals_free;
}



bool nqueens_solve_grid_rec(bool arr[][N], bool occ_rows[N], bool occ_cols[N], int num_queens) {
	bool ret = false;

	if (num_queens == N)
		return !ret;

	for (int row = 0; row < N; row++) {
		for (int col = 0; col < N; col++) {
			if (occ_rows[row])
				continue;
			if (occ_cols[col])
				continue;

			if (!nqueens_check_diagonals(arr, row, col))
				continue;

			arr[row][col] = true;
			occ_rows[row] = true;
			occ_cols[col] = true;

			if ((ret = nqueens_solve_grid_rec(arr, occ_rows, occ_cols, num_queens + 1)))
				continue;

			arr[row][col] = false;
			occ_rows[row] = false;
			occ_cols[col] = false;
		}
	}

	return ret;
}

void nqueens_solve_grid(bool arr[][N]) {
	bool occ_rows[N] = {false};
	bool occ_cols[N] = {false};

	if (nqueens_solve_grid_rec(arr, occ_rows, occ_cols, 0)) {
		printf("[%s][N=%u] solution:\n", __FUNCTION__, N);
		nqueens_print_grid(arr);
	}
}



int main(void) {
	bool arr[N][N];

	nqueens_init_grid(arr);
	nqueens_solve_grid(arr);
	return 0;
}

