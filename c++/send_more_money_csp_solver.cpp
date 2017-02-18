#include <cassert>
#include <cstdio>
#include <cstdint>
#include <vector>

#if 1
#define print(...) printf(__VA_ARGS__)
#else
#define print(...)
#endif



static const uint32_t MID_DEPTH = 4;
static const uint32_t MAX_DEPTH = 8;

static const uint32_t POWER_LUT[5] = {1, 10, 100, 1000, 10000};



static uint32_t calc_value(const std::vector<uint32_t>& digits) {
	uint32_t s = 0;

	for (size_t n = 0; n < digits.size(); n++) {
		s += (digits[n] * POWER_LUT[n]);
	}

	return s;
}

static bool solve_csp(
	std::vector<    bool>& lhs_dgt_mask,
	std::vector<    bool>& rhs_dgt_mask,
	std::vector<uint32_t>& lhs_dgt_vals,
	std::vector<uint32_t>& rhs_dgt_vals,
	const uint32_t cur_depth,
	const uint32_t mid_depth,
	const uint32_t max_depth
) {
	// select operand (LHS=SEND or RHS=MORE)
	//
	// if depth <  MID_DEPTH, manipulate digits of LHS
	// if depth >= MID_DEPTH, manipulate digits of RHS
	//
	std::vector<    bool>& dgt_mask = (cur_depth < mid_depth)? lhs_dgt_mask: rhs_dgt_mask;
	std::vector<uint32_t>& dgt_vals = (cur_depth < mid_depth)? lhs_dgt_vals: rhs_dgt_vals;

	// convert depth to an LHS or RHS digit-index (0=3, 1=2, ..., 4=3, 5=2, ...)
	const size_t dgt_idx = (mid_depth - 1) - (cur_depth % mid_depth);

	if (cur_depth >= max_depth) {
		// all digits have a concrete assignment, check constraints
		const uint32_t sum_value = calc_value(lhs_dgt_vals) + calc_value(rhs_dgt_vals);
		const uint32_t sum_array[] = {
			((sum_value / POWER_LUT[0]) % 10), // 1234[5]
			((sum_value / POWER_LUT[1]) % 10), // 123[4]5
			((sum_value / POWER_LUT[2]) % 10), // 12[3]45
			((sum_value / POWER_LUT[3]) % 10), // 1[2]345
			((sum_value / POWER_LUT[4]) % 10), // [1]2345
		};

		if (sum_value < POWER_LUT[4])
			return false;

		#if 0
		// [M]ORE == 1, ensured by caller
		if (rhs_dgt_vals[3] != 1)
			return false;
		#endif

		// S[E]ND == MOR[E]
		if (lhs_dgt_vals[2] != rhs_dgt_vals[0])
			return false;
		// SEN[D] != MO[R]E
		if (lhs_dgt_vals[0] == rhs_dgt_vals[1])
			return false;
		// [S]END != MOR[E]
		if (lhs_dgt_vals[3] == rhs_dgt_vals[0])
			return false;
		// [S]END != MO[R]E
		if (lhs_dgt_vals[3] == rhs_dgt_vals[1])
			return false;
		// SE[N]D != MO[R]E
		if (lhs_dgt_vals[1] == rhs_dgt_vals[1])
			return false;

		// SE[N]D == MO[N]EY
		if (lhs_dgt_vals[1] != sum_array[2])
			return false;
		// MOR[E] == MON[E]Y
		if (rhs_dgt_vals[0] != sum_array[1])
			return false;
		// M[O]RE == M[O]NEY
		if (rhs_dgt_vals[2] != sum_array[3])
			return false;

		// [M]ONEY != MONE[Y]
		if (sum_array[4] == sum_array[0])
			return false;
		// M[O]NEY != MONE[Y]
		if (sum_array[3] == sum_array[0])
			return false;
		// MO[N]EY != MONE[Y]
		if (sum_array[2] == sum_array[0])
			return false;
		// MON[E]Y != MONE[Y]
		if (sum_array[1] == sum_array[0])
			return false;

		// found a solution satisfying all constraints
		print("[%s]\n", __FUNCTION__);
		print("\tidx | 4 3 2 1 0   4 3 2 1 0\n");
		print("\t----+----------------------\n");
		print("\tlhs |   S E N D = %s %d %d %d %d\n", " ", lhs_dgt_vals[3], lhs_dgt_vals[2], lhs_dgt_vals[1], lhs_dgt_vals[0]);
		print("\trhs |   M O R E = %s %d %d %d %d\n", " ", rhs_dgt_vals[3], rhs_dgt_vals[2], rhs_dgt_vals[1], rhs_dgt_vals[0]);
		print("\tsum | M O N E Y = %d %d %d %d %d\n", sum_array[4], sum_array[3], sum_array[2], sum_array[1], sum_array[0]);
		print("\n");

		return true;
	} else {
		// the value of 'M' (at depth 4) is forced to 1, so 'S' (at depth 0) must be at least 8
		const uint32_t min_dgt_val = 8 * (cur_depth ==         0) + 1 * (cur_depth == mid_depth);
		const uint32_t max_dgt_val = 9 * (cur_depth != mid_depth) + 1 * (cur_depth == mid_depth);

		// set each digit to a value we have not used yet
		for (uint32_t dgt_val = min_dgt_val; dgt_val <= max_dgt_val; dgt_val++) {
			if (dgt_mask[dgt_val])
				continue;

			dgt_mask[dgt_val] = true;
			dgt_vals[dgt_idx] = dgt_val;

			if (solve_csp(lhs_dgt_mask, rhs_dgt_mask,  lhs_dgt_vals, rhs_dgt_vals,  cur_depth + 1, mid_depth, max_depth))
				return true;

			dgt_mask[dgt_val] = false;
		}
	}

	return false;
}



int main() {
	std::vector<bool> lhs_dgt_mask = {false, false, false, false, false, false, false, false, false, false};
	std::vector<bool> rhs_dgt_mask = {false, false, false, false, false, false, false, false, false, false};

	std::vector<uint32_t> lhs_dgt_vals = {0, 0, 0, 0};
	std::vector<uint32_t> rhs_dgt_vals = {0, 0, 0, 0};

	assert(MID_DEPTH == lhs_dgt_vals.size());
	assert(MID_DEPTH == rhs_dgt_vals.size());
	assert(MAX_DEPTH == MID_DEPTH * 2);

	solve_csp(lhs_dgt_mask, rhs_dgt_mask,  lhs_dgt_vals, rhs_dgt_vals,  0, MID_DEPTH, MAX_DEPTH);
	return 0;
}

