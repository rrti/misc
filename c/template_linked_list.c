#ifndef TEMPLATE_LINKED_LIST
#define TEMPLATE_LINKED_LIST

#define safe_free(p) { free(*((void**) &p)); *((void**) &p) = NULL; }



/* no templates, so resort to preprocessor magic */
#define DECLARE_LINKED_LIST(NAME, TYPE)            \
	struct s_## NAME ## _list_node {               \
		/* payload */                              \
		TYPE data;                                 \
                                                   \
		/* links */                                \
		struct s_## NAME ##_list_node* prev;       \
		struct s_## NAME ##_list_node* next;       \
	};                                             \
                                                   \
	struct s_## NAME ## _list {                    \
		struct s_## NAME ##_list_node* head;       \
		struct s_## NAME ##_list_node* tail;       \
                                                   \
		size_t size;                               \
	};                                             \
                                                   \
                                                   \
                                                   \
	typedef   struct s_## NAME ## _list_node   t_## NAME ## _list_node;                    \
	typedef   struct s_## NAME ## _list        t_## NAME ## _list;                         \
                                                                                           \
                                                                                           \
                                                                                           \
	t_## NAME ##_list*  alloc_## NAME ##_list() {                                          \
		t_## NAME ##_list* list = (t_## NAME ##_list*) malloc(sizeof(t_## NAME ##_list));  \
                                                                                           \
		if (list == NULL)                                                                  \
			return list;                                                                   \
                                                                                           \
		list->head = NULL;                                                                 \
		list->tail = NULL;                                                                 \
		list->size = 0;                                                                    \
		return list;                                                                       \
	}                                                                                      \
                                                                                           \
	void dealloc_## NAME ##_list(t_## NAME ##_list* list) {                                \
		t_## NAME ##_list_node* curr_node = list->head;                                    \
		t_## NAME ##_list_node* next_node = NULL;                                          \
                                                                                           \
		/* free all elements (no need to unlink, etc here) */                              \
		while (curr_node != NULL) {                                                        \
			next_node = curr_node->next;                                                   \
                                                                                           \
			safe_free(curr_node);                                                          \
                                                                                           \
			curr_node = next_node;                                                         \
		}                                                                                  \
                                                                                           \
                                                                                           \
		list->head = NULL;                                                                 \
		list->tail = NULL;                                                                 \
		list->size = 0;                                                                    \
                                                                                           \
		safe_free(list);                                                                   \
	}                                                                                      \
                                                                                                           \
                                                                                                           \
                                                                                                           \
	t_## NAME ##_list_node*  alloc_## NAME ##_list_node(TYPE data) {                                       \
		t_## NAME ##_list_node* node = (t_## NAME ##_list_node*) malloc(sizeof(t_## NAME ##_list_node));   \
                                                                                                           \
		if (node == NULL)                                                                                  \
			return node;                                                                                   \
                                                                                                           \
		node->data = data;                                                                                 \
		node->prev = NULL;                                                                                 \
		node->next = NULL;                                                                                 \
		return node;                                                                                       \
	}                                                                                                      \
                                                                                                           \
	/* note: caller must unlink and free payload itself first (if a pointer-type) */                       \
	void dealloc_## NAME ##_list_node(t_## NAME ##_list_node* node) {                                      \
		node->prev = NULL;                                                                                 \
		node->next = NULL;                                                                                 \
                                                                                                           \
		safe_free(node);                                                                                   \
	}                                                                                                      \
                                                                                                           \
                                                                                                           \
                                                                                                           \
	/* insert <node> directly after <prev> in <list> (O(1)) */                                             \
	void insert_## NAME ##_list_node(                                                                      \
		t_## NAME ##_list* list,                                                                           \
		t_## NAME ##_list_node* prev,                                                                      \
		t_## NAME ##_list_node* node                                                                       \
	) {                                                                                                    \
		assert(prev != NULL);                                                                              \
                                                                                                           \
		t_## NAME ##_list_node* next = prev->next;                                                         \
                                                                                                           \
		if (next != NULL)                                                                                  \
			next->prev = node;                                                                             \
		/* make sure tail stays in sync */                                                                 \
		if (prev == list->tail)                                                                            \
			list->tail = node;                                                                             \
                                                                                                           \
		prev->next = node;                                                                                 \
		node->prev = prev;                                                                                 \
		node->next = next;                                                                                 \
                                                                                                           \
		list->size += 1;                                                                                   \
	}                                                                                                      \
                                                                                                           \
	/* remove <node> from <list> (O(1)) */                                                                 \
	t_## NAME ##_list_node* remove_## NAME ##_list_node(                                                   \
		t_## NAME ##_list* list,                                                                           \
		t_## NAME ##_list_node* node                                                                       \
	) {                                                                                                    \
		assert(node != NULL);                                                                              \
                                                                                                           \
		t_## NAME ##_list_node* prev = node->prev;                                                         \
		t_## NAME ##_list_node* next = node->next;                                                         \
                                                                                                           \
		if (prev != NULL)                                                                                  \
			prev->next = next;                                                                             \
		if (next != NULL)                                                                                  \
			next->prev = prev;                                                                             \
                                                                                                           \
		/* make sure head and tail stay in sync */                                                         \
		if (node == list->head)                                                                            \
			list->head = (list->head)->next;                                                               \
		if (node == list->tail)                                                                            \
			list->tail = (list->tail)->prev;                                                               \
                                                                                                           \
		node->prev = NULL;                                                                                 \
		node->next = NULL;                                                                                 \
                                                                                                           \
		list->size -= 1;                                                                                   \
		return next;                                                                                       \
	}                                                                                                      \
                                                                                                           \
                                                                                                           \
                                                                                                           \
	/* append <node> to tail of <list> (O(1)) */                                                           \
	void push_back_## NAME ##_list_node(t_## NAME ##_list* list, t_## NAME ##_list_node* node) {           \
		assert(node != NULL);                                                                              \
                                                                                                           \
		if (list->tail == NULL) {                                                                          \
			/* list is still empty */                                                                      \
			assert(list->head == NULL);                                                                    \
			assert(list->size == 0);                                                                       \
                                                                                                           \
			node->prev = NULL;                                                                             \
			node->next = NULL;                                                                             \
			list->head = node;                                                                             \
			list->tail = node;                                                                             \
		} else {                                                                                           \
			assert(list->size != 0);                                                                       \
			assert(list->head != NULL);                                                                    \
                                                                                                           \
			node->prev = list->tail;                                                                       \
			node->next = NULL;                                                                             \
			(list->tail)->next = node;                                                                     \
			list->tail = node;                                                                             \
		}                                                                                                  \
                                                                                                           \
		list->size += 1;                                                                                   \
	}                                                                                                      \
                                                                                                           \
	/* prepend <node> to head of <list> (O(1)) */                                                          \
	void push_front_## NAME ##_list_node(t_## NAME ##_list* list, t_## NAME ##_list_node* node) {          \
		assert(node != NULL);                                                                              \
                                                                                                           \
		if (list->head == NULL) {                                                                          \
			/* list is still empty */                                                                      \
			assert(list->tail == NULL);                                                                    \
			assert(list->size == 0);                                                                       \
                                                                                                           \
			node->prev = NULL;                                                                             \
			node->next = NULL;                                                                             \
			list->head = node;                                                                             \
			list->tail = node;                                                                             \
		} else {                                                                                           \
			assert(list->size != 0);                                                                       \
			assert(list->tail != NULL);                                                                    \
                                                                                                           \
			node->prev = NULL;                                                                             \
			node->next = list->head;                                                                       \
			(list->head)->prev = node;                                                                     \
			list->head = node;                                                                             \
		}                                                                                                  \
                                                                                                           \
		list->size += 1;                                                                                   \
	}                                                                                                      \
                                                                                                           \
                                                                                                           \
                                                                                                           \
	/* remove element from front of <list> (O(1)) */                                                       \
	t_## NAME ##_list_node*  pop_front_## NAME ##_list_node(t_## NAME ##_list* list) {                     \
		if (list->size == 0)                                                                               \
			return NULL;                                                                                   \
                                                                                                           \
		t_## NAME ##_list_node* node = list->head;                                                         \
                                                                                                           \
		list->size -= 1;                                                                                   \
		list->head = node->next;                                                                           \
                                                                                                           \
		/* make sure head stays in sync if list still contains elements */                                 \
		if (list->head != NULL) {                                                                          \
			assert(list->size != 0);                                                                       \
			(list->head)->prev = NULL;                                                                     \
		} else {                                                                                           \
			assert(list->size == 0);                                                                       \
			assert(node->next == 0);                                                                       \
			list->tail = NULL;                                                                             \
		}                                                                                                  \
                                                                                                           \
		node->prev = NULL;                                                                                 \
		node->next = NULL;                                                                                 \
		return node;                                                                                       \
	}                                                                                                      \
                                                                                                           \
	/* remove element from back of <list> (O(1)) */                                                        \
	t_## NAME ##_list_node*  pop_back_## NAME ##_list_node(t_## NAME ##_list* list) {                      \
		if (list->size == 0)                                                                               \
			return NULL;                                                                                   \
                                                                                                           \
		t_## NAME ##_list_node* node = list->tail;                                                         \
                                                                                                           \
		list->size -= 1;                                                                                   \
		list->tail = node->prev;                                                                           \
                                                                                                           \
		/* make sure tail stays in sync if list still contains elements */                                 \
		if (list->tail != NULL) {                                                                          \
			assert(list->size != 0);                                                                       \
			(list->tail)->next = NULL;                                                                     \
		} else {                                                                                           \
			assert(list->size == 0);                                                                       \
			assert(node->prev == 0);                                                                       \
			list->head = NULL;                                                                             \
		}                                                                                                  \
                                                                                                           \
		node->prev = NULL;                                                                                 \
		node->next = NULL;                                                                                 \
		return node;                                                                                       \
	}

#endif

