#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEQUE_BLOCK_SIZE 16
#define DEQUE_VALUE_TYPE int

#define MAX(a, b) (((a) > (b))? (a): (b))

#ifndef NDEBUG
#define ASSERT(cond, desc)                                                                            \
	do {                                                                                              \
		if (!cond) {                                                                                  \
			printf("[file=%s][line=%d][func=%s][desc=\"%s\"]\n", __FILE__, __LINE__, __func__, desc); \
			assert(cond);                                                                             \
		}                                                                                             \
	} while (false);
#else
#define ASSERT(cond, desc) assert(cond);
#endif


typedef
	struct deque_block_s {
		DEQUE_VALUE_TYPE data[DEQUE_BLOCK_SIZE];
	}
deque_block_t;

typedef
	struct deque_s {
		// pointers s.t. resizing does not invalidate iterators
		deque_block_t** block_vector;

		size_t cur_num_values; // number of values currently contained
		size_t max_num_blocks; // number of blocks this deque can hold

		size_t min_write_idx; // index where push_front will insert next (first actual stored value is at min+1)
		size_t max_write_idx; // index where push_back will insert next (last actual stored value is at max-1)
	}
deque_t;



void deque_clear(deque_t* dq);
size_t deque_max_size(const deque_t* dq);

void deque_debug_print(const deque_t* dq) {
	const size_t min_value_idx = dq->min_write_idx + 1;
	const size_t max_value_idx = dq->max_write_idx - 1;

	printf("[%s][size=%lu][{min,max}_val_idx={%lu,%lu}]\n", __func__, dq->cur_num_values, min_value_idx, max_value_idx);
	printf("\t[");

	for (size_t cur_idx = min_value_idx; cur_idx <= max_value_idx; cur_idx++) {
		const size_t blk_idx = cur_idx / DEQUE_BLOCK_SIZE;
		const size_t val_idx = cur_idx % DEQUE_BLOCK_SIZE;

		assert(dq->block_vector[blk_idx] != NULL);

		if (val_idx == 0)
			printf("][");

		printf(" %3d ", dq->block_vector[blk_idx]->data[val_idx]);
	}

	printf("]\n");
}



deque_block_t* deque_alloc_block() {
	deque_block_t* block = (deque_block_t*) malloc(sizeof(deque_block_t));
	memset(&block->data[0], 0, DEQUE_BLOCK_SIZE * sizeof(DEQUE_VALUE_TYPE));
	return block;
}

deque_t* deque_alloc(size_t num_values) {
	deque_t* dq = (deque_t*) malloc(sizeof(deque_t));

	dq->max_num_blocks = MAX(2, num_values / DEQUE_BLOCK_SIZE);
	dq->block_vector = (deque_block_t**) malloc(dq->max_num_blocks * sizeof(deque_block_t*));

	deque_clear(dq);

	for (size_t n = 0; n < dq->max_num_blocks; n++)
		dq->block_vector[n] = NULL;

	// allocate one initial block (which min and max both map to)
	dq->block_vector[dq->min_write_idx / DEQUE_BLOCK_SIZE] = deque_alloc_block();

	return dq;
}

void deque_free(deque_t* dq) {
	for (size_t n = 0; n < dq->max_num_blocks; n++)
		free(dq->block_vector[n]);

	free(dq->block_vector);
	free(dq);
}

void deque_clear(deque_t* dq) {
	dq->cur_num_values = 0;
	dq->min_write_idx = (DEQUE_BLOCK_SIZE >> 1) + (dq->max_num_blocks >> 1) * DEQUE_BLOCK_SIZE - 1;
	dq->max_write_idx = (DEQUE_BLOCK_SIZE >> 1) + (dq->max_num_blocks >> 1) * DEQUE_BLOCK_SIZE;
}

void deque_resize(deque_t* dq) {
	deque_block_t** block_vector = dq->block_vector;

	const size_t old_num_blocks = dq->max_num_blocks;
	const size_t new_num_blocks = dq->max_num_blocks << 1;

	// only the number of *blocks* changes on resize
	dq->max_num_blocks = new_num_blocks;
	dq->block_vector = (deque_block_t**) malloc(new_num_blocks * sizeof(deque_block_t*));

	for (size_t n = 0; n < new_num_blocks; n++)
		dq->block_vector[n] = NULL;

	// place old blocks in middle of new array
	for (size_t n = 0; n < old_num_blocks; n++)
		dq->block_vector[(old_num_blocks >> 1) + n] = block_vector[n];

	// NOTE:
	//   if old_num_blocks == 1, a push_front() underflow will
	//   cause min_idx to remain at -1u instead of shifting it
	//   properly
	assert(old_num_blocks > 1);

	dq->min_write_idx += ((old_num_blocks >> 1) * DEQUE_BLOCK_SIZE);
	dq->max_write_idx += ((old_num_blocks >> 1) * DEQUE_BLOCK_SIZE);

	free(block_vector);
}

size_t deque_size(const deque_t* dq) { return (dq->cur_num_values); }
size_t deque_max_size(const deque_t* dq) { return (dq->max_num_blocks * DEQUE_BLOCK_SIZE); }

bool deque_empty(const deque_t* dq) { return (deque_size(dq) == 0); }



void deque_set(deque_t* dq, size_t idx, DEQUE_VALUE_TYPE v) {
	assert(idx < deque_size(dq));

	const size_t min_idx = dq->min_write_idx + 1;
	const size_t blk_idx = (min_idx + idx) / DEQUE_BLOCK_SIZE;
	const size_t val_idx = (min_idx + idx) % DEQUE_BLOCK_SIZE;

	assert(dq->block_vector[blk_idx] != NULL);
	dq->block_vector[blk_idx]->data[val_idx] = v;
}

DEQUE_VALUE_TYPE deque_get(const deque_t* dq, size_t idx) {
	assert(idx < deque_size(dq));

	const size_t min_idx = dq->min_write_idx + 1;
	const size_t blk_idx = (min_idx + idx) / DEQUE_BLOCK_SIZE;
	const size_t val_idx = (min_idx + idx) % DEQUE_BLOCK_SIZE;

	assert(dq->block_vector[blk_idx] != NULL);
	return (dq->block_vector[blk_idx]->data[val_idx]);
}

DEQUE_VALUE_TYPE deque_front(const deque_t* dq) {
	assert(!deque_empty(dq));
	return (deque_get(dq, 0));
}

DEQUE_VALUE_TYPE deque_back(const deque_t* dq) {
	assert(!deque_empty(dq));
	return (deque_get(dq, deque_size(dq) - 1));
}



void deque_push_front(deque_t* dq, DEQUE_VALUE_TYPE v) {
	const size_t blk_idx = dq->min_write_idx / DEQUE_BLOCK_SIZE;
	const size_t val_idx = dq->min_write_idx % DEQUE_BLOCK_SIZE;

	deque_block_t* block = dq->block_vector[blk_idx];

	if (block == NULL)
		dq->block_vector[blk_idx] = (block = deque_alloc_block());

	// write, then move index
	block->data[val_idx] = v;

	dq->min_write_idx -= 1;
	dq->cur_num_values += 1;

	// check for underflow
	if (dq->min_write_idx < dq->max_write_idx)
		return;

	deque_resize(dq);
}

void deque_push_back(deque_t* dq, DEQUE_VALUE_TYPE v) {
	const size_t blk_idx = dq->max_write_idx / DEQUE_BLOCK_SIZE;
	const size_t val_idx = dq->max_write_idx % DEQUE_BLOCK_SIZE;

	deque_block_t* block = dq->block_vector[blk_idx];

	if (block == NULL)
		dq->block_vector[blk_idx] = (block = deque_alloc_block());

	// write, then move index
	block->data[val_idx] = v;

	dq->max_write_idx += 1;
	dq->cur_num_values += 1;

	// check for overflow
	if (dq->max_write_idx < deque_max_size(dq))
		return;

	deque_resize(dq);
}



DEQUE_VALUE_TYPE deque_pop_front(deque_t* dq) {
	// asserts !empty
	const DEQUE_VALUE_TYPE ret = deque_front(dq);

	dq->min_write_idx += 1;
	dq->cur_num_values -= 1;

	return ret;
}

DEQUE_VALUE_TYPE deque_pop_back(deque_t* dq) {
	// asserts !empty
	const DEQUE_VALUE_TYPE ret = deque_back(dq);

	dq->max_write_idx -= 1;
	dq->cur_num_values -= 1;

	return ret;
}



int main(int argc, char** argv) {
	srand((argc > 1)? atoi(argv[1]): time(NULL));

	// start with a two-block queue
	deque_t* dq = deque_alloc(16);

	for (int i = 1; i <= 32; i++) {
		deque_push_front(dq, -i);
		deque_push_back(dq, i);
		deque_debug_print(dq);
	}

	#if 0
	deque_debug_print(dq);
	deque_clear(dq);
	assert(deque_empty(dq));
	assert(deque_size(dq) == 0);
	#endif

	while (!deque_empty(dq)) {
		assert(deque_front(dq) == deque_get(dq, 0));
		assert(deque_back(dq) == deque_get(dq, deque_size(dq) - 1));

		if ((rand() & 1) == 1) {
			deque_pop_front(dq);
		} else {
			deque_pop_back(dq);
		}
	}

	deque_debug_print(dq);
	deque_push_back(dq, 123);
	deque_debug_print(dq);
	deque_free(dq);
	return 0;
}

