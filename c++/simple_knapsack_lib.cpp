#include <cstdio>
#include <cstdlib>
#include <ctime>

#include <limits>
#include <vector>

typedef unsigned int uint_type;

static const uint_type NUM_TEST_OBJECTS  =   20;
static const uint_type MAX_SACK_WEIGHT   =  100;
static const uint_type MAX_OBJECT_WEIGHT =   50;
static const uint_type MAX_OBJECT_VALUE  = 1000;



struct t_object {
public:
	t_object(uint_type id): m_id(id) {}

	virtual uint_type get_id() const { return m_id; }
	virtual uint_type get_weight() const = 0;
	virtual uint_type get_value() const = 0;

protected:
	uint_type m_id;
};

struct t_sack_object: public t_object {
public:
	t_sack_object(uint_type id, uint_type wgt, uint_type val): t_object(id), m_weight(wgt), m_value(val) {
	}

	uint_type get_weight() const { return m_weight; }
	uint_type get_value() const { return m_value; }

private:
	uint_type m_weight;
	uint_type m_value;
};



// solves the 0-1 knapsack problem via dynamic-programming
//
// sack is filled with the optimal combination of objects
// (ie. the combination which maximizes the sum of values
// and gets closest to <max_total_weight>) through greedy
// selection
//
uint_type knapsack_fill(
	const uint_type max_total_weight,
	const uint_type min_object_value,
	const uint_type max_object_value,
	const std::vector<t_object*>& object_list,
	      std::vector<t_object*>& object_sack
) {
	if (max_total_weight == 0)
		return 0;
	if (min_object_value > max_object_value)
		return 0;
	if (object_list.empty())
		return 0;

	std::vector<uint_type> object_weights(object_list.size(), std::numeric_limits<uint_type>::max());
	std::vector<uint_type> object_values(object_list.size(), 0);
	std::vector< std::vector<uint_type> > object_matrix(object_weights.size() + 1, std::vector<uint_type>(max_total_weight, 0));

	for (uint_type n = 0; n < object_list.size(); n++) {
		const t_object* o = object_list[n];

		// only consider objects within our value-bounds
		if (o->get_value() >= min_object_value && o->get_value() <= max_object_value) {
			object_weights[n] = o->get_weight();
			object_values[n] = o->get_value();
		}
	}

	for (uint_type k = 1; k <= object_weights.size(); k++) {
		for (uint_type y = 1; y <= max_total_weight; y++) {
			const uint_type wgt = object_weights[k - 1];
			const uint_type val = object_matrix[k - 1][y - 1];

			if (y < wgt) {
				object_matrix[k][y - 1] = val;
			} else if (y > wgt) {
				object_matrix[k][y - 1] = std::max(val,  object_matrix[k - 1][y - 1 - wgt] + object_values[k - 1]);
			} else {
				object_matrix[k][y - 1] = std::max(val,                                      object_values[k - 1]);
			}
		}
	}

	// get the objects in the solution
	uint_type idx = object_weights.size();
	uint_type wgt = max_total_weight - 1;
	uint_type val = 0;

	while (idx > 0) {
		const bool b0 = ((idx == 0) && (object_matrix[idx][wgt] > 0));
		const bool b1 = ((idx >  0) && (object_matrix[idx][wgt] != object_matrix[idx - 1][wgt]));

		if (b0 || b1) {
			object_sack.push_back(object_list[idx - 1]);

			wgt -= object_weights[idx - 1];
			val += object_list[idx - 1]->get_value();
		}

		if (static_cast<int>(wgt) <= 0) {
			break;
		}

		idx--;
	}

	// return the total filled sack value
	return val;
}



int main() {
	srandom(time(NULL));

	std::vector<t_object*> object_list;
	std::vector<t_object*> object_sack;

	object_list.reserve(NUM_TEST_OBJECTS);
	object_sack.reserve(NUM_TEST_OBJECTS);

	for (uint_type n = 0; n < NUM_TEST_OBJECTS; n++) {
		object_list.push_back(new t_sack_object(n, (random() % MAX_OBJECT_WEIGHT) + 1, (random() % MAX_OBJECT_VALUE) + 1));
	}

	if (knapsack_fill(MAX_SACK_WEIGHT, 0, std::numeric_limits<uint_type>::max(), object_list, object_sack) > 0) {
		uint_type weight_sum = 0;
		uint_type value_sum = 0;

		for (uint_type n = 0; n < object_sack.size(); n++) {
			weight_sum += object_sack[n]->get_weight();
			value_sum += object_sack[n]->get_value();
		}

		printf("[%s] |sack|=%lu weight=%u value=%u\n", __FUNCTION__, object_sack.size(), weight_sum, value_sum);
	}

	return 0;
}

