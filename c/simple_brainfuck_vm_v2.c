#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CODE_TEST "++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++."
#define DATA_SIZE 32768

#define OUT_STREAM stdout
#define INP_STREAM stdin


struct s_simple_vm {
	char* code;
	char* data;
	size_t* jtbl;

	// use these addresses so code and data pointers do
	// not have to be manipulated directly (which would
	// impose free'ing them through unmodified copies)
	size_t insp;
	size_t addr;
};

typedef struct s_simple_vm t_simple_vm;




static size_t file_size(FILE* file) {
	fseek(file, 0, SEEK_END);
	const size_t size = ftell(file);
	fseek(file, 0, SEEK_SET);
	return size;
}

static char* read_file(const char* name) {
	FILE* file = fopen(name, "r");

	if (file == NULL)
		return NULL;

	const size_t size = file_size(file);
	char* data = (char*) malloc(size);

	if (data == NULL) {
		fclose(file);
		return NULL;
	}

	if (fread(data, sizeof(char), size, file) != size) {
		free(data);
		fclose(file);
		return NULL;
	}

	fclose(file);
	return data;
}

static char* heap_copy(const char* code) {
	char* copy = (char*) malloc(strlen(code));

	assert(copy != NULL);
	strcpy(copy, code);

	return copy;
}




static size_t* vm_calc_jtbl(const t_simple_vm* vm) {
	// stack can contain at most <strlen(code)> '['-elements
	//
	// the jump-table only needs to be as large as the number
	// of []-pairs, but we keep it simple with a single level
	// of indirection
	size_t stack_len = strlen(vm->code);
	size_t stack_idx = 0;

	size_t* table = (size_t*) calloc(stack_len, sizeof(size_t));
	size_t* stack = (size_t*) calloc(stack_len, sizeof(size_t));

	assert(table != NULL);
	assert(stack != NULL);

	for (size_t i = 0; vm->code[i] != 0; i++) {
		switch (vm->code[i]) {
			case '[': {
				// push code-address
				assert(stack_idx < stack_len);
				stack[stack_idx++] = i;
			} break;

			case ']': {
				assert(stack_idx > 0);

				// pop code-address
				const size_t j = stack[--stack_idx];

				table[i] = j + 1; // ']' jumps  back to '[' + 1
				table[j] = i + 1; // '[' jumps ahead to ']' + 1
			} break;

			default: {
			} break;
		}
	}

	free(stack);
	return table;
}

static void vm_init(t_simple_vm* vm, const char* arg) {
	char* code = ((*arg) == 0)? heap_copy(CODE_TEST): read_file(arg);
	char* data = malloc(DATA_SIZE);

	assert(code != NULL);
	assert(data != NULL);

	vm->code = code;
	vm->data = data;
	vm->jtbl = vm_calc_jtbl(vm);

	vm->insp = 0;
	vm->addr = 0;
}

static void vm_exec_inst(t_simple_vm* vm) {
	switch (vm->code[vm->insp]) {
		case '>': { (vm->addr)++; } break;
		case '<': { (vm->addr)--; } break;

		case '+': { vm->data[vm->addr]++; } break;
		case '-': { vm->data[vm->addr]--; } break;

		case '.': { putc(vm->data[vm->addr], OUT_STREAM); } break;
		case ',': { vm->data[vm->addr] = getc(INP_STREAM); } break;

		case '[': {
			// conditional forward jump
			if (vm->data[vm->addr] == 0) {
				vm->insp = vm->jtbl[vm->insp];
				return;
			}
		} break;
		case ']': {
			// conditional reverse jump
			if (vm->data[vm->addr] != 0) {
				vm->insp = vm->jtbl[vm->insp];
				return;
			}
		} break;

		default: {
			// ignore all other characters
		} break;
	}

	vm->insp += 1;
}

static void vm_exec(t_simple_vm* vm) {
	const size_t vm_max_code_addr = strlen(vm->code) - 1;
	const size_t vm_max_data_addr = DATA_SIZE - 1;

	while (vm->insp <= vm_max_code_addr && vm->addr <= vm_max_data_addr) {
		vm_exec_inst(vm);
	}
}

static void vm_kill(t_simple_vm* vm) {
	free(vm->code); vm->code = NULL;
	free(vm->data); vm->data = NULL;
	free(vm->jtbl); vm->jtbl = NULL;
}




int main(int argc, char** argv) {
	t_simple_vm vm;

	vm_init(&vm, (argc <= 1)? "": argv[1]);
	vm_exec(&vm);
	vm_kill(&vm);

	return 0;
}

