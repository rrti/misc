#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "template_linked_list.c"

#define HEAP_SIZE (1024 * 1024)
#define DEBUG_HEAP
// #define DEBUG_SPAM

#if (defined(DEBUG_HEAP) && defined(DEBUG_SPAM))
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif



struct s_memblock_node_data {
	void* node;
	void* addr;
	size_t size;
};

typedef
	struct s_memblock_node_data
t_memblock_node_data;



// DECLARE_LINKED_LIST(int, int)
// DECLARE_LINKED_LIST(voidp, void*)
DECLARE_LINKED_LIST(memblock, t_memblock_node_data*)



t_memblock_node_data* alloc_memblock_data(size_t size) {
	t_memblock_node_data* block = (t_memblock_node_data*) malloc(sizeof(t_memblock_node_data));

	if (block == NULL)
		return NULL;

	block->node = NULL;
	block->addr = block;
	block->size = size;
	return block;
}

void dealloc_memblock_data(t_memblock_node_data* block) {
	block->node = NULL;
	block->addr = NULL;
	block->size = 0;

	safe_free(block);
}



#if 0
void insert_sort_block_list_elem(t_memblock_list* list, t_memblock_list_elem* elem) {
	const t_memblock_list_elem* curr_elem = list->head;
	const t_memblock_list_elem* next_elem = NULL;

	const t_memblock_node_data* elem_data = elem->data;
	const t_memblock_node_data* curr_data = NULL;
	const t_memblock_node_data* next_data = NULL;

	while (curr_elem != NULL) {
		if (curr_elem->next == NULL)
			break;

		next_elem = curr_elem->next;
		next_data = next_elem->data;
		curr_data = curr_elem->data;

		if (elem_data->addr >= (curr_data->addr + curr_data->size)  &&  (elem_data->addr + elem_data->size) <= next_data->addr) {
			break;
		}

		curr_elem = next_elem;
	}
}
#endif


int heap_split_block(
	t_memblock_node_data* parent_block,
	t_memblock_node_data** l_child_block,
	t_memblock_node_data** r_child_block,
	size_t split_index
) {
	assert(split_index !=                  0);
	assert(split_index <= parent_block->size);

	// we do not want any NULL child-blocks
	if (split_index == parent_block->size) {
		*l_child_block = NULL;
		*r_child_block = NULL;
		return 0;
	}

	*l_child_block = alloc_memblock_data(split_index);
	*r_child_block = alloc_memblock_data(parent_block->size - split_index);

	dealloc_memblock_data(parent_block);

	assert((*l_child_block) != NULL);
	assert((*r_child_block) != NULL);
	return 1;
}




void heap_merge_blocks(t_memblock_list* free_list) {
	t_memblock_list_node* curr_elem = free_list->head;
	t_memblock_list_node* next_elem = curr_elem->next;

	// look for adjacent blocks that can be merged
	while (next_elem != NULL) {
		t_memblock_node_data* curr_data = curr_elem->data;
		t_memblock_node_data* next_data = next_elem->data;

		curr_elem = next_elem;
		next_elem = curr_elem->next;

		if ((curr_data->addr + curr_data->size) == next_data->addr) {
			curr_data->size += next_data->size;

			dealloc_memblock_data(curr_data);
			remove_memblock_list_node(free_list, curr_elem);
			dealloc_memblock_list_node(curr_elem);
		}
	}
}




// scan the list for a block of size <size> or larger
t_memblock_node_data* heap_alloc(t_memblock_list* free_list, size_t size) {
	t_memblock_list_node* iter = free_list->head;
	t_memblock_node_data* block = NULL;

	if (size == 0)
		return block;

	while (iter != NULL) {
		block = iter->data;

		// all elements in the free list must contain a payload
		assert(block != NULL);

		if (block->size >= size) {
			// if exactly equal then grab block directly, else split and grab left
			t_memblock_node_data* l_child_block = NULL;
			t_memblock_node_data* r_child_block = NULL;

			if (heap_split_block(block, &l_child_block, &r_child_block, size)) {
				block = l_child_block;

				// right child-block goes into free list and replaces its parent
				insert_memblock_list_node(free_list, iter, alloc_memblock_list_node(r_child_block));
			}

			remove_memblock_list_node(free_list, iter);
			dealloc_memblock_list_node(iter);
			break;
		}

		iter = iter->next;
		block = NULL;
	}

	return block;
}

t_memblock_node_data* heap_free(t_memblock_list* used_list, t_memblock_node_data* used_data) {
	t_memblock_list_node* node = used_list->head;
	t_memblock_node_data* data = NULL;

	// look up the used-list node containing this block
	// (much faster if blocks contain their owner node*)
	if (used_data->node == NULL) {
		while (node != NULL) {
			data = node->data;

			if (node->data == used_data) {
				remove_memblock_list_node(used_list, node);
				dealloc_memblock_list_node(node);
				break;
			}

			node = node->next;
		}
	} else {
		data = used_data;

		remove_memblock_list_node(used_list, (t_memblock_list_node*) used_data->node);
		dealloc_memblock_list_node((t_memblock_list_node*) used_data->node);
	}

	return data;
}

void heap_debug(t_memblock_list* free_list, t_memblock_list* used_list) {
	size_t free_size = 0;
	size_t used_size = 0;

	{
		t_memblock_list_node* node = free_list->head;
		t_memblock_node_data* data = NULL;

		DEBUG_PRINT("[%s][#free-list=%lu]\n", __FUNCTION__, free_list->size);

		while (node != NULL) {
			data = node->data;
			node = node->next;

			free_size += data->size;
			DEBUG_PRINT("\tdata={addr=%p :: size=%lu}\n", data->addr, data->size);
		}

		DEBUG_PRINT("\tFREE=%lu bytes\n", free_size);
	}

	{
		t_memblock_list_node* node = used_list->head;
		t_memblock_node_data* data = NULL;

		DEBUG_PRINT("[%s][#used-list=%lu]\n", __FUNCTION__, used_list->size);

		while (node != NULL) {
			data = node->data;
			node = node->next;

			used_size += data->size;
			DEBUG_PRINT("\tdata={addr=%p :: size=%lu}\n", data->addr, data->size);
		}

		DEBUG_PRINT("\tUSED=%lu bytes\n", used_size);
	}

	assert((free_size + used_size) == HEAP_SIZE);
}




int main(int argc, char** argv) {
	size_t max_iter = -1u;
	size_t cur_iter =  0;

	size_t free_bytes = HEAP_SIZE;
	size_t used_bytes =         0;

	size_t tot_allocs = 0;
	size_t bad_allocs = 0;

	if (argc > 1) {
		srandom(atoi(argv[1]));
	} else {
		srandom(time(NULL));
	}

	if (argc > 2) {
		max_iter = atoi(argv[2]);
	} else {
		max_iter = 1000;
	}


	t_memblock_list* free_blocks = alloc_memblock_list();
	t_memblock_list* used_blocks = alloc_memblock_list();

	assert(free_blocks != NULL);
	assert(used_blocks != NULL);

	// start with a single pool-block
	push_back_memblock_list_node(free_blocks, alloc_memblock_list_node(alloc_memblock_data(HEAP_SIZE)));

	{
		t_memblock_list_node* n = free_blocks->head;
		t_memblock_node_data* d = n->data;

		d->node = n;
	}

	while ((cur_iter++) < max_iter) {
		assert(free_bytes <= HEAP_SIZE);
		assert(used_bytes <= HEAP_SIZE);
		assert((free_bytes + used_bytes) == HEAP_SIZE);

		if ((random() & 1) != 0) {
			if (free_blocks->size != 0) {
				#ifdef DEBUG_HEAP
				DEBUG_PRINT("[%s][iter=%lu][  ALLOC1][free=%lu :: used=%lu][#free=%lu :: #used=%lu]\n", __FUNCTION__, cur_iter, free_bytes, used_bytes, free_blocks->size, used_blocks->size);
				heap_debug(free_blocks, used_blocks);
				#endif

				// simulate allocation request
				t_memblock_node_data* block = heap_alloc(free_blocks, random() % HEAP_SIZE);

				if (block != NULL) {
					// put claimed block at the back of the used-list
					push_back_memblock_list_node(used_blocks, alloc_memblock_list_node(block));

					// deep-link the block for O(1) reclamation
					block->node = used_blocks->tail;

					free_bytes -= block->size;
					used_bytes += block->size;

					#ifdef DEBUG_HEAP
					DEBUG_PRINT("[%s][iter=%lu][  ALLOC2][free=%lu :: used=%lu][#free=%lu :: #used=%lu][data={addr=%p :: size=%lu}]\n", __FUNCTION__, cur_iter, free_bytes, used_bytes, free_blocks->size, used_blocks->size, block->addr, block->size);
					heap_debug(free_blocks, used_blocks);
					#endif

					assert(free_bytes <= HEAP_SIZE);
					assert(used_bytes <= HEAP_SIZE);
					assert((free_bytes + used_bytes) == HEAP_SIZE);
				}

				tot_allocs += 1;
				bad_allocs += (block == NULL);
			}
		} else {
			if (used_blocks->size != 0) {
				#ifdef DEBUG_HEAP
				DEBUG_PRINT("[%s][iter=%lu][DEALLOC1][free=%lu :: used=%lu][#free=%lu :: #used=%lu]\n", __FUNCTION__, cur_iter, free_bytes, used_bytes, free_blocks->size, used_blocks->size);
				heap_debug(free_blocks, used_blocks);
				#endif

				// simulate deallocation request (will not always be the head)
				t_memblock_list_node* node = used_blocks->head;
				t_memblock_node_data* block = heap_free(used_blocks, node->data);

				assert(block != NULL);
				assert(block->node != NULL);

				// TODO: use insertion-sort instead
				push_back_memblock_list_node(free_blocks, alloc_memblock_list_node(block));

				free_bytes += block->size;
				used_bytes -= block->size;

				#ifdef DEBUG_HEAP
				DEBUG_PRINT("[%s][iter=%lu][DEALLOC2][free=%lu :: used=%lu][#free=%lu :: #used=%lu][data={addr=%p :: size=%lu}]\n", __FUNCTION__, cur_iter, free_bytes, used_bytes, free_blocks->size, used_blocks->size, block->addr, block->size);
				heap_debug(free_blocks, used_blocks);
				#endif

				assert(free_bytes <= HEAP_SIZE);
				assert(used_bytes <= HEAP_SIZE);
				assert((free_bytes + used_bytes) == HEAP_SIZE);
			}
		}

		if ((cur_iter % 100) == 0) {
			heap_merge_blocks(free_blocks);
			#ifdef DEBUG_HEAP
			heap_debug(free_blocks, used_blocks);
			#endif
		}

		assert(free_bytes <= HEAP_SIZE);
		assert(used_bytes <= HEAP_SIZE);
		assert((free_bytes + used_bytes) == HEAP_SIZE);
	}

	printf("[%s][free=%lu :: used=%lu][bad=%lu :: total=%lu]\n", __FUNCTION__, free_bytes, used_bytes, bad_allocs, tot_allocs);
	heap_debug(free_blocks, used_blocks);

	// NOTE: remaining data payloads will not be freed
	dealloc_memblock_list(free_blocks);
	dealloc_memblock_list(used_blocks);

	return 0;
}

