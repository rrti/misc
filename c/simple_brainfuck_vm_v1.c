#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CODE_TEST "++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++."
#define DATA_SIZE 32768

#define OUT_STREAM stdout
#define INP_STREAM stdin

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


static const char* jump_fwd(const char* code) {
	size_t num_pars = 0;

	while (1) {
		num_pars += ((*code) == '[');
		num_pars -= ((*code) == ']');

		// final step if matching ]-bracket found
		if (num_pars == 0) {
			code++; break;
		}

		code++;
	}

	return code;
}

static const char* jump_rev(const char* code) {
	size_t num_pars = 0;

	while (1) {
		num_pars -= ((*code) == '[');
		num_pars += ((*code) == ']');

		// final step if matching [-bracket found
		if (num_pars == 0) {
			code++; break;
		}

		code--;
	}

	return code;
}


static void interpret_code(const char* code, char* data) {
	const char* min_code_addr = &code[               0];
	const char* max_code_addr = &code[strlen(code) - 1];

	const char* min_data_addr = &data[            0];
	const char* max_data_addr = &data[DATA_SIZE - 1];

	while ((*code) != 0) {
		assert(code >= min_code_addr);
		assert(code <= max_code_addr);
		assert(data >= min_data_addr);
		assert(data <= max_data_addr);

		switch (*code) {
			case '>': { data++; code++; } break;
			case '<': { data--; code++; } break;

			case '+': { (*data)++; code++; } break;
			case '-': { (*data)--; code++; } break;

			case '.': { putc(*data, OUT_STREAM); code++; } break;
			case ',': { *data = getc(INP_STREAM); code++; } break;

			case '[': {
				if ((*data) == 0) {
					code = jump_fwd(code);
				} else {
					code++;
				}
			} break;
			case ']': {
				if ((*data) != 0) {
					code = jump_rev(code);
				} else {
					code++;
				}
			} break;

			default: {
				code++; // ignore all other characters
			} break;
		}
	}
}

int main(int argc, char** argv) {
	char* code = (argc <= 1)? heap_copy(CODE_TEST): read_file(argv[1]);
	char* data = (char*) malloc(DATA_SIZE);

	if (code != NULL && data != NULL) {
		interpret_code(code, data);
	}

	free(code);
	free(data);
	return 0;
}

