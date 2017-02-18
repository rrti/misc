#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <vector>

// for unbuffered input control
#include <unistd.h>
#include <termios.h>

typedef unsigned char t_uint8;
typedef           int t_int32;
typedef unsigned  int t_uint32;



struct t_scoped_unbuffered_stdin_ctl {
public:
	t_scoped_unbuffered_stdin_ctl() {
		// get the terminal settings for stdin
		tcgetattr(STDIN_FILENO, &old_tio);

		// copy old settings to restore them later
		new_tio = old_tio;

		// disable canonical mode (buffered IO) and local echo
		new_tio.c_lflag &= (~ICANON & ~ECHO);

		// set the new settings immediately
		tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
	}

	~t_scoped_unbuffered_stdin_ctl() {
		// restore the former settings
		tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
	}

private:
	struct termios old_tio;
	struct termios new_tio;
};



class t_simple_virtual_machine {
public:
	// NOTE:
	//   do we want a separate set of instrs for every data-type
	//   (easy but ugly) or do we want to parameterize them (hard
	//   but elegant)?
	//   do we want stack and data to have byte-level granularity
	//   (more stack pressure) or granularity of largest data-type
	//   (more wasted space)?
	//
	enum t_vm_instr {
		// literal ops
		INSTR_UINT8_PUSH = 0x00,
		INSTR_UINT8_PEEK = 0x01,

		// data ops
		INSTR_UINT8_SET_MEM,
		INSTR_UINT8_GET_MEM,
		INSTR_UINT8_SWP_MEM,

		INSTR_UINT8_SET_CODE,
		INSTR_UINT8_GET_CODE,

		INSTR_UINT8_SET_PTR,
		INSTR_UINT8_GET_PTR,

		// arithmetic ops
		INSTR_UINT8_ADD,
		INSTR_UINT8_SUB,
		INSTR_UINT8_MUL,
		INSTR_UINT8_DIV,
		INSTR_UINT8_MOD,

		// bitwise ops
		INSTR_UINT8_BIT_AND,
		INSTR_UINT8_BIT_OR,
		INSTR_UINT8_BIT_NOT,
		INSTR_UINT8_BIT_SHL,
		INSTR_UINT8_BIT_SHR,

		// logical ops
		INSTR_UINT8_LOG_AND,
		INSTR_UINT8_LOG_OR,
		INSTR_UINT8_LOG_NOT,

		// relational ops
		INSTR_UINT8_CMP_LT,
		INSTR_UINT8_CMP_LE,
		INSTR_UINT8_CMP_EQ,
		INSTR_UINT8_CMP_GE,
		INSTR_UINT8_CMP_GT,

		// control-flow ops; unconditional absolute/relative jumps
		INSTR_UINT8_JMP_ABS,
		INSTR_UINT8_JMP_REL,

		// conditional jumps based on result of previous logic-op
		INSTR_UINT8_JMP_ABS_IF,
		INSTR_UINT8_JMP_REL_IF,

		INSTR_UINT8_GOTO_INS,
		INSTR_UINT8_GOTO_LBL,

		// function-call ops (these all act on literals)
		INSTR_UINT8_LBL_ADDR,
		INSTR_UINT8_JMP_FUNC,
		INSTR_UINT8_RET_FUNC,

		// aka. SUSPEND/PAUSE/BREAK/etc
		INSTR_YIELD_CPU,
		INSTR_ERASE_MEM,
		INSTR_ALLOC_MEM,
	};

	enum t_vm_func_labels {
		LABEL_MAIN_FUNC = 0,
		LABEL_EXIT_FUNC = 1,
	};


	#if 0
	enum t_vm_value_type {
		TYPE_INT    = 0,
		TYPE_DOUBLE = 1,
		TYPE_STRING = 2,
	};

	// tagged-union
	struct t_vm_value {
		t_vm_value_type type;

		union {
			int       int_val;
			double double_val;
			char*  string_val;
		};
	};
	#endif


	t_simple_virtual_machine(size_t max_exec_stack_size = 128, size_t max_call_stack_size = 64, size_t max_data_array_size = 256) {
		m_exec_stack.resize(max_exec_stack_size, 0);
		m_call_stack.resize(max_call_stack_size, 0);
		m_data_array.resize(max_data_array_size, 0);

		clear_execution_context();
	}
	~t_simple_virtual_machine() {
		m_exec_stack.clear();
		m_data_array.clear();
		m_code_bytes.clear();
	}

	t_simple_virtual_machine& operator = (const t_simple_virtual_machine& vm) {
		set_cur_exec_stack_size(vm.get_cur_exec_stack_size());
		set_cur_prog_counter(vm.get_cur_prog_counter());
		set_num_instructions(vm.get_num_instructions());

		set_exec_stack(vm.get_exec_stack());
		set_call_stack(vm.get_call_stack());
		set_data_array(vm.get_data_array());

		set_code_bytes(vm.get_code_bytes());
		set_code_addrs(vm.get_code_addrs());
		return *this;
	}


	// VM state accessors
	size_t get_cur_exec_stack_size() const { return m_cur_exec_stack_size; }
	size_t get_cur_call_stack_size() const { return m_cur_call_stack_size; }
	size_t get_max_exec_stack_size() const { return m_exec_stack.size(); }
	size_t get_max_call_stack_size() const { return m_call_stack.size(); }
	size_t get_max_data_array_size() const { return m_data_array.size(); }


	size_t get_cur_prog_counter() const { return m_cur_prog_counter; }
	size_t get_num_instructions() const { return m_num_instructions; }

	const std::vector<t_uint8>& get_exec_stack() const { return m_exec_stack; }
	const std::vector< size_t>& get_call_stack() const { return m_call_stack; }
	const std::vector<t_uint8>& get_data_array() const { return m_data_array; }

	const std::vector<t_uint8>& get_code_bytes() const { return m_code_bytes; }
	const std::vector< size_t>& get_code_addrs() const { return m_code_addrs; }

	// providing this allows host to act on current program
	// instruction (e.g. a yield) whenever run_code returns
	// prematurely
	t_uint8 get_cur_instruction() const { return m_code_bytes[m_cur_prog_counter]; }

	void set_cur_exec_stack_size(size_t n) { m_cur_exec_stack_size = std::min(n, get_max_exec_stack_size()); }
	void set_cur_call_stack_size(size_t n) { m_cur_call_stack_size = std::min(n, get_max_call_stack_size()); }
	void set_max_exec_stack_size(size_t n) { m_exec_stack.resize(n, 0); }
	void set_max_call_stack_size(size_t n) { m_call_stack.resize(n, 0); }
	void set_max_data_array_size(size_t n) { m_data_array.resize(n, 0); }

	void set_cur_prog_counter(size_t n) { m_cur_prog_counter = n; }
	void set_num_instructions(size_t n) { m_num_instructions = n; }
	void set_last_yield_instr(size_t n) { m_last_yield_instr = n; }

	void set_exec_stack(const std::vector<t_uint8>& exec_stack) { m_exec_stack = exec_stack; }
	void set_call_stack(const std::vector< size_t>& call_stack) { m_call_stack = call_stack; }
	void set_data_array(const std::vector<t_uint8>& data_array) { m_data_array = data_array; }

	void set_code_bytes(const std::vector<t_uint8>& code_bytes) { m_code_bytes = code_bytes; }
	void set_code_addrs(const std::vector< size_t>& code_addrs) { m_code_addrs = code_addrs; }



	void clear_execution_context() {
		// prepare new execution context
		//
		// maximum stack- and data-size has either *already*
		// been set normally or will be by read_state_file()
		set_cur_exec_stack_size(0);
		set_cur_call_stack_size(0);

		set_cur_prog_counter(0);
		set_num_instructions(0);
		set_last_yield_instr(0);
	}


	// call this after read_code{_file}() and (possibly) read_state_file()
	// code must have been loaded, state is optional and can be left alone
	//
	bool run_code(const size_t init_prog_counter, bool single_step_mode) {
		// technically no harm in this (VM will exit)
		assert(!m_code_bytes.empty());
		// these are not so innocent (VM might crash)
		assert(!m_exec_stack.empty());
		assert(!m_call_stack.empty());
		assert(!m_data_array.empty());
		assert(!m_code_addrs.empty());

		// we have either already jumped to the address of a function label instr.
		// and are on the literal byte, or are at a yield and need to skip past it
		set_cur_prog_counter(init_prog_counter + 1);

		// start interpreting from the given program-counter
		while (m_cur_prog_counter < m_code_bytes.size()) {
			m_num_instructions += 1;

			// if false, stepping remains disabled until next yield
			if (single_step_mode)
				single_step_mode &= dump_single_step_state(stdout);

			if (!handle_opcode(m_code_bytes[m_cur_prog_counter]))
				break;

			// empty stack (should) mean(s) we returned from main
			if (m_cur_call_stack_size == 0)
				break;

			// pre-empt the program after any long non-yielding period
			if ((m_num_instructions - m_last_yield_instr) >= 10000) {
				m_last_yield_instr = m_num_instructions;
				break;
			}
			// also do periodic pre-empting whether it yielded or not
			if ((m_num_instructions % 10000) == 0) {
				break;
			}

			m_cur_prog_counter += 1;
		}

		// code should ideally not leave anything on the stack
		// but this can not be enforced due to potential yield
		// instructions
		//
		// assert(m_cur_exec_stack_size == 0);
		// assert(m_cur_call_stack_size == 0);

		// check for successful (complete) program termination
		// note: code can also jump past its final instruction
		return (m_cur_prog_counter >= m_code_bytes.size() || m_cur_call_stack_size == 0);
	}



	// read a raw code byte-stream, useful for inline testing
	void read_code(const t_uint8* raw_code, const size_t raw_size) {
		m_code_bytes.clear();
		m_code_bytes.resize(raw_size, 0);
		m_code_addrs.clear();

		memcpy(&m_code_bytes[0], &raw_code[0], raw_size);

		// prepare a stackframe for "main" (the callee ensures
		// this function label always exists), begin execution
		// from it
		set_address_labels();
		jmp_function(LABEL_MAIN_FUNC);
	}

	bool read_code_file(const char* vm_code_file) {
		FILE* f = fopen(vm_code_file, "rb");
		bool r = true;

		if (f == NULL)
			return (!r);

		fseek(f, 0, SEEK_END);

		m_code_bytes.clear();
		m_code_bytes.resize(ftell(f), 0);
		m_code_addrs.clear();

		fseek(f, 0, SEEK_SET);

		// check the read-operation for success
		r &= (fread(&m_code_bytes[0], m_code_bytes.size(), 1, f) == 1);

		fclose(f);

		set_address_labels();
		jmp_function(LABEL_MAIN_FUNC);
		return r;
	}

	// stores loaded code in a binary file
	// so it can be loaded from disk later
	bool write_code_file(const char* vm_code_file) {
		FILE* f = fopen(vm_code_file, "wb");
		bool r = true;

		if (f == NULL)
			return (!r);

		// check the write-operation for success
		r &= (fwrite(&m_code_bytes[0], m_code_bytes.size(), 1, f) == 1);

		fclose(f);
		return r;
	}


	// (de)serialize the VM state, enables doing e.g.
	//   1. read_code_file("vm_code_file.dat");
	//   2. read_state_file("vm_state_file.dat");
	//   3. run_code(vm_get_cur_prog_counter());
	bool read_state_file(const char* vm_state_file) {
		FILE* f = fopen(vm_state_file, "rb");
		bool r = true;

		if (f == NULL)
			return (!r);

		size_t max_vm_exec_stack_size = 0;
		size_t max_vm_call_stack_size = 0;
		size_t max_vm_data_array_size = 0;

		r &= (fread(&m_cur_exec_stack_size, sizeof(size_t), 1, f) == 1);
		r &= (fread(&m_cur_call_stack_size, sizeof(size_t), 1, f) == 1);

		r &= (fread(&max_vm_exec_stack_size, sizeof(size_t), 1, f) == 1);
		r &= (fread(&max_vm_call_stack_size, sizeof(size_t), 1, f) == 1);
		r &= (fread(&max_vm_data_array_size, sizeof(size_t), 1, f) == 1);

		set_max_exec_stack_size(max_vm_exec_stack_size);
		set_max_call_stack_size(max_vm_call_stack_size);
		set_max_data_array_size(max_vm_data_array_size);

		// restore data-stack state
		r &= (fread(&m_exec_stack[0], m_exec_stack.size(), 1, f) == 1);
		// restore call-stack state
		r &= (fread(&m_call_stack[0], m_call_stack.size(), 1, f) == 1);
		// restore main-memory state
		r &= (fread(&m_data_array[0], m_data_array.size(), 1, f) == 1);

		// restore processor-state
		r &= (fread(&m_cur_prog_counter, sizeof(size_t), 1, f) == 1);
		r &= (fread(&m_num_instructions, sizeof(size_t), 1, f) == 1);

		fclose(f);
		return r;
	}

	bool write_state_file(const char* vm_state_file) {
		FILE* f = fopen(vm_state_file, "wb");
		bool r = true;

		if (f == NULL)
			return (!r);

		const size_t max_vm_exec_stack_size = m_exec_stack.size();
		const size_t max_vm_call_stack_size = m_call_stack.size();
		const size_t max_vm_data_array_size = m_data_array.size();

		// need to use fwrite because fprintf ignores "b" mode
		r &= (fwrite(&m_cur_exec_stack_size, sizeof(size_t), 1, f) == 1);
		r &= (fwrite(&m_cur_call_stack_size, sizeof(size_t), 1, f) == 1);

		r &= (fwrite(&max_vm_exec_stack_size, sizeof(size_t), 1, f) == 1);
		r &= (fwrite(&max_vm_call_stack_size, sizeof(size_t), 1, f) == 1);
		r &= (fwrite(&max_vm_data_array_size, sizeof(size_t), 1, f) == 1);

		// write data-stack state
		r &= (fwrite(&m_exec_stack[0], m_exec_stack.size(), 1, f) == 1);
		// write call-stack state
		r &= (fwrite(&m_call_stack[0], m_call_stack.size(), 1, f) == 1);
		// write main-memory state
		r &= (fwrite(&m_data_array[0], m_data_array.size(), 1, f) == 1);

		// write processor-state
		r &= (fwrite(&m_cur_prog_counter, sizeof(size_t), 1, f) == 1);
		r &= (fwrite(&m_num_instructions, sizeof(size_t), 1, f) == 1);

		fclose(f);
		return r;
	}



	void dump_state(const char* prefix = "", FILE* stream = stdout) const {
		fprintf(stream, "%s", prefix);

		dump_exec_stack(stream);
		dump_call_stack(stream);
		dump_code(stream);
		dump_data(stream);
		dump_proc(stream);
	}

private:
	t_uint8 read_uint8_data(size_t idx) const { assert(idx < m_data_array.size()); return m_data_array[idx]; }
	t_uint8 read_uint8_code(size_t idx) const { assert(idx < m_code_bytes.size()); return m_code_bytes[idx]; }

	void write_uint8_data(size_t idx, t_uint8 val) { assert(idx < m_data_array.size()); m_data_array[idx] = val; }
	void write_uint8_code(size_t idx, t_uint8 val) { assert(idx < m_code_bytes.size()); m_code_bytes[idx] = val; }


	// data-stack operations (push, pop, read)
	void push_uint8_val(t_uint8 value) {
		assert(m_cur_exec_stack_size < m_exec_stack.size());
		m_exec_stack[m_cur_exec_stack_size++] = value;
	}

	t_uint8 pop_uint8_val() {
		assert(m_cur_exec_stack_size > 0);
		return (m_exec_stack[--m_cur_exec_stack_size]);
	}

	t_uint8 read_uint8_val(size_t stack_index_offset) const {
		assert(m_cur_exec_stack_size >= stack_index_offset);
		return (m_exec_stack[m_cur_exec_stack_size - stack_index_offset]);
	}


	// call-stack operations
	void jmp_abs(size_t code_address) { m_cur_prog_counter = code_address; }
	void jmp_rel(size_t code_offset) { m_cur_prog_counter += code_offset; }

	void jmp_function(t_uint8 func_address_index) {
		assert(func_address_index < m_code_addrs.size());
		assert(m_cur_call_stack_size < m_call_stack.size());

		// set return-address to one instruction beyond the call
		// (implicitly, the loop-incr on return will handle this)
		m_call_stack[m_cur_call_stack_size++] = m_cur_prog_counter;
		// label-instr will always be skipped by loop-increment
		m_cur_prog_counter = m_code_addrs[func_address_index];
	}

	void ret_function(t_uint8 num_args) {
		assert(m_cur_call_stack_size >                    0);
		assert(m_cur_call_stack_size <= m_call_stack.size());

		// set prog-counter to return-address (instr. following call)
		m_cur_prog_counter = m_call_stack[--m_cur_call_stack_size];

		// pop the specified number of arguments
		for (t_uint8 n = 0; n < num_args; n++) {
			pop_uint8_val();
		}
	}



	bool handle_opcode(t_uint8 opcode) {
		bool ret = true;

		switch (opcode) {
			case INSTR_UINT8_PUSH: {
				// read next code byte and skip over it
				// (since we cannot execute that byte!)
				push_uint8_val(read_uint8_code(m_cur_prog_counter += 1));
			} break;
			case INSTR_UINT8_PEEK: {
				// allow retrieving elements lower on the stack
				// note: literal is offset relative to stack top
				push_uint8_val(read_uint8_val(read_uint8_code(m_cur_prog_counter += 1)));
			} break;


			case INSTR_UINT8_SET_MEM: {
				const t_uint8 val = pop_uint8_val();
				const t_uint8 idx = pop_uint8_val();

				// copy data from stack into memory
				write_uint8_data(idx, val);
			} break;
			case INSTR_UINT8_GET_MEM: {
				// copy data from memory onto stack
				push_uint8_val(read_uint8_data(pop_uint8_val()));
			} break;
			case INSTR_UINT8_SWP_MEM: {
				// swap contents of two memory cells
				const t_uint8 dst_idx = pop_uint8_val();
				const t_uint8 src_idx = pop_uint8_val();

				const t_uint8 dst_val = read_uint8_data(dst_idx);
				const t_uint8 src_val = read_uint8_data(src_idx);

				write_uint8_data(src_idx, dst_val);
				write_uint8_data(dst_idx, src_val);
			} break;

			case INSTR_UINT8_SET_CODE: {
				#if 0
				const t_uint8 val = pop_uint8_val();
				const t_uint8 idx = pop_uint8_val();

				// DANGEROUS, ALLOWS SELF-MODIFYING CODE
				write_uint8_code(idx, val);
				#endif
			} break;
			case INSTR_UINT8_GET_CODE: {
				// allow program self-inspection
				push_uint8_val(read_uint8_code(pop_uint8_val()));
			} break;

			case INSTR_UINT8_SET_PTR: {
				const t_uint8 ptr_val =                (pop_uint8_val());
				const t_uint8 ptr_idx = read_uint8_data(pop_uint8_val());

				// *ptr = val (write a memory-cell indirectly)
				write_uint8_data(ptr_idx, ptr_val);
			} break;
			case INSTR_UINT8_GET_PTR: {
				const t_uint8 ptr_idx = read_uint8_data(pop_uint8_val());
				const t_uint8 ptr_val = read_uint8_data(ptr_idx);

				// val = *ptr (read a memory-cell indirectly)
				push_uint8_val(ptr_val);
			} break;


			// NOTE:
			//   since operand order is reversed by stack pushes, need to
			//   pop them in reverse order for all instr's where the order
			//   matters
			case INSTR_UINT8_ADD: { push_uint8_val(pop_uint8_val() + pop_uint8_val()); } break;
			case INSTR_UINT8_MUL: { push_uint8_val(pop_uint8_val() * pop_uint8_val()); } break;

			case INSTR_UINT8_SUB: {
				const t_uint8 rhs = pop_uint8_val();
				const t_uint8 lhs = pop_uint8_val();

				push_uint8_val(lhs - rhs);
			} break;
			case INSTR_UINT8_DIV: {
				const t_uint8 rhs = pop_uint8_val();
				const t_uint8 lhs = pop_uint8_val();

				push_uint8_val(lhs / rhs);
			} break;
			case INSTR_UINT8_MOD: {
				const t_uint8 rhs = pop_uint8_val();
				const t_uint8 lhs = pop_uint8_val();

				push_uint8_val(lhs % rhs);
			} break;


			case INSTR_UINT8_BIT_AND: {
				push_uint8_val(pop_uint8_val() & pop_uint8_val());
			} break;
			case INSTR_UINT8_BIT_OR: {
				push_uint8_val(pop_uint8_val() | pop_uint8_val());
			} break;
			case INSTR_UINT8_BIT_NOT: {
				push_uint8_val(~pop_uint8_val());
			} break;
			case INSTR_UINT8_BIT_SHL: {
				const t_uint8 rhs = pop_uint8_val();
				const t_uint8 lhs = pop_uint8_val();

				push_uint8_val(lhs << rhs);
			} break;
			case INSTR_UINT8_BIT_SHR: {
				const t_uint8 rhs = pop_uint8_val();
				const t_uint8 lhs = pop_uint8_val();

				push_uint8_val(lhs >> rhs);
			} break;


			case INSTR_UINT8_LOG_AND: {
				push_uint8_val(pop_uint8_val() && pop_uint8_val());
			} break;
			case INSTR_UINT8_LOG_OR: {
				push_uint8_val(pop_uint8_val() || pop_uint8_val());
			} break;
			case INSTR_UINT8_LOG_NOT: {
				push_uint8_val(!pop_uint8_val());
			} break;


			case INSTR_UINT8_CMP_LT: {
				const t_uint8 rhs = pop_uint8_val();
				const t_uint8 lhs = pop_uint8_val();

				push_uint8_val(lhs < rhs);
			} break;
			case INSTR_UINT8_CMP_LE: {
				const t_uint8 rhs = pop_uint8_val();
				const t_uint8 lhs = pop_uint8_val();

				push_uint8_val(lhs <= rhs);
			} break;
			case INSTR_UINT8_CMP_EQ: {
				const t_uint8 rhs = pop_uint8_val();
				const t_uint8 lhs = pop_uint8_val();

				push_uint8_val(lhs == rhs);
			} break;
			case INSTR_UINT8_CMP_GE: {
				const t_uint8 rhs = pop_uint8_val();
				const t_uint8 lhs = pop_uint8_val();

				push_uint8_val(lhs >= rhs);
			} break;
			case INSTR_UINT8_CMP_GT: {
				const t_uint8 rhs = pop_uint8_val();
				const t_uint8 lhs = pop_uint8_val();

				push_uint8_val(lhs > rhs);
			} break;


			// note the -1's: next loop-increment will cancel them out
			case INSTR_UINT8_JMP_ABS: {
				jmp_abs(pop_uint8_val() - 1);
			} break;
			case INSTR_UINT8_JMP_REL: {
				jmp_rel(pop_uint8_val() - 1);
			} break;

			// note: comparison result must be pushed first, address second
			case INSTR_UINT8_JMP_ABS_IF: {
				const t_uint8 npc = pop_uint8_val();
				const t_uint8 cmp = pop_uint8_val();

				if (cmp != 0) {
					jmp_abs(npc - 1);
				}
			} break;
			case INSTR_UINT8_JMP_REL_IF: {
				const t_uint8 npc = pop_uint8_val();
				const t_uint8 cmp = pop_uint8_val();

				if (cmp != 0) {
					jmp_rel(npc - 1);
				}
			} break;

			case INSTR_UINT8_GOTO_INS: {
				// jump to instruction (same as PUSH followed by JMP_ABS)
				jmp_abs(read_uint8_code(m_cur_prog_counter + 1) - 1);
			} break;
			case INSTR_UINT8_GOTO_LBL: {
				// jump to label
				jmp_abs(m_code_addrs[read_uint8_code(m_cur_prog_counter + 1)]);
			} break;

			case INSTR_UINT8_LBL_ADDR: {
				// silently ignore, these are pre-processed
				m_cur_prog_counter += 1;
			} break;
			case INSTR_UINT8_JMP_FUNC: {
				jmp_function(read_uint8_code(m_cur_prog_counter += 1));
			} break;
			case INSTR_UINT8_RET_FUNC: {
				ret_function(read_uint8_code(m_cur_prog_counter += 1));
			} break;

			case INSTR_YIELD_CPU: {
				m_last_yield_instr = m_num_instructions;

				// let the interpreter know we are not done yet
				ret = false;
			} break;
			case INSTR_ERASE_MEM: {
				memset(&m_data_array[0], 0, get_max_data_array_size());
			} break;
			case INSTR_ALLOC_MEM: {
				m_data_array.resize(get_max_data_array_size() * 2, 0);
			} break;

			default: {
				// illegal instruction
				assert(false);
			} break;
		}

		return ret;
	}


	void set_address_labels() {
		assert(!m_code_bytes.empty());
		assert(m_code_addrs.empty());

		// consider "main" to be either the first label
		// instruction encountered or address 0 if none
		m_code_addrs.resize(16, static_cast<size_t>(-1));

		for (size_t n = 0; n < m_code_bytes.size(); n++) {
			switch (m_code_bytes[n]) {
				// skip over literals that might potentially
				// have the same byte-value as INSTR_LBL_FUNC
				case INSTR_UINT8_PUSH:     { n += 1; continue; } break;
				case INSTR_UINT8_GOTO_INS: { n += 1; continue; } break;
				case INSTR_UINT8_GOTO_LBL: { n += 1; continue; } break;
				case INSTR_UINT8_PEEK:     { n += 1; continue; } break;

				case INSTR_UINT8_LBL_ADDR: {
					// advance to literal (label value)
					n += 1;

					while (m_code_addrs.size() <= m_code_bytes[n]) {
						m_code_addrs.resize(m_code_addrs.size() * 2, static_cast<size_t>(-1));
					}

					// disallow relabeling
					assert(m_code_addrs[m_code_bytes[n]] == static_cast<size_t>(-1));

					// map label value to function address (instr-index)
					m_code_addrs[m_code_bytes[n]] = n;
				} break;

				case INSTR_UINT8_JMP_FUNC: { n += 1; continue; } break;
				case INSTR_UINT8_RET_FUNC: { n += 1; continue; } break;
			}
		}
	}



	void dump_exec_stack(FILE* stream) const {
		fprintf(stream, "\n\t[exec_stack]\n");

		// note: older elements get printed first, top is lower-most line
		for (size_t n = 0; n < m_cur_exec_stack_size; n++) {
			fprintf(stream, "\t\t[n=%lu] %u\n", n, m_exec_stack[n]);
		}
	}

	void dump_call_stack(FILE* stream) const {
		fprintf(stream, "\n\t[call_stack]\n");

		for (size_t n = 0; n < m_cur_call_stack_size; n++) {
			fprintf(stream, "\t\t[n=%lu] %lu\n", n, m_call_stack[n]);
		}
	}

	void dump_code(FILE* stream) const { fprintf(stream, "\n\t[code_bytes]\n\t\t     "); dump_array(stream, m_code_bytes); }
	void dump_data(FILE* stream) const { fprintf(stream, "\n\t[data_bytes]\n\t\t     "); dump_array(stream, m_data_array); }
	void dump_proc(FILE* stream) const {
		#define pc_fs "\t\tprog_count=%lu\n"
		#define ni_fs "\t\tnum_instrs=%lu\n"
		fprintf(stream, "\t[proc]\n" pc_fs ni_fs, m_cur_prog_counter, m_num_instructions);
		#undef ni_fs
		#undef pc_fs
	}

	void dump_array(FILE* stream, const std::vector<t_uint8>& array) const {
		// column labels
		for (t_uint8 n = 0; n <= 0xF; n++)
			fprintf(stream, " %02X ", n);

		// table divider
		fprintf(stream, "\n\t\t   +================================================================\n");

		for (size_t n = 0; n < array.size(); n++) {
			// row labels
			if ((n % 16) == 0)
				fprintf(stream, "\t\t%02X | ", static_cast<t_uint32>(n / 16));

			fprintf(stream, " %02X ", array[n]);

			// row splits
			if ((n > 0) && ((n + 1) % 16) == 0) {
				fprintf(stream, "\n");
			}
		}

		fprintf(stream, "\n");
	}

	bool dump_single_step_state(FILE* stream) {
		const char* fmt = "\n[vm::%s] cur_pc=%lu cur_ins=%lu :: #exec_stack=%lu #call_stack=%lu\n";

		const size_t pc = m_cur_prog_counter;
		const size_t cb = m_code_bytes[m_cur_prog_counter];
		const size_t ess = m_cur_exec_stack_size;
		const size_t css = m_cur_call_stack_size;

		fprintf(stream, fmt, __FUNCTION__, pc, cb, ess, css);
		dump_proc(stream);
		dump_exec_stack(stream);
		dump_call_stack(stream);

		// blocks until any key is pressed
		return (getchar() != 'q');
	}

private:
	size_t m_cur_exec_stack_size;
	size_t m_cur_call_stack_size;

	size_t m_cur_prog_counter;
	size_t m_num_instructions;
	size_t m_last_yield_instr;

	std::vector<t_uint8> m_exec_stack; // runtime data
	std::vector<t_uint8> m_data_array; // "main memory"
	std::vector<t_uint8> m_code_bytes; // instructions

	// call-stack and code address-labels
	std::vector<size_t> m_call_stack;
	std::vector<size_t> m_code_addrs;
};




// simple infinite loop, test unconditional jumps
static const t_uint8 sample_vm_code_0[] = {
	t_simple_virtual_machine::INSTR_UINT8_PUSH, 0,
	t_simple_virtual_machine::INSTR_UINT8_JMP_ABS,
};

// finite loop, test conditional jumps
static const t_uint8 sample_vm_code_1[] = {
	t_simple_virtual_machine::INSTR_UINT8_LBL_ADDR, 123,

	t_simple_virtual_machine::INSTR_UINT8_PUSH, 0,        //  0: push index to GET_MEM
	t_simple_virtual_machine::INSTR_UINT8_GET_MEM,        //  2: push mem[0]
	t_simple_virtual_machine::INSTR_UINT8_PUSH, 100,      //  3: push 10 (max. loop iter.)
	t_simple_virtual_machine::INSTR_UINT8_CMP_GE,         //  5: push (mem[0] >= 10)
	t_simple_virtual_machine::INSTR_UINT8_PUSH, 255,      //  6: push code address to jump to
	t_simple_virtual_machine::INSTR_UINT8_JMP_ABS_IF,     //  8: jump to 255 if CMP_GE

	t_simple_virtual_machine::INSTR_YIELD_CPU,            //  9: sleep each iteration
	// t_simple_virtual_machine::INSTR_ERASE_MEM,         // 10: clear counter to loop forever

	t_simple_virtual_machine::INSTR_UINT8_PUSH, 0,        // 11: push index to SET_MEM
	t_simple_virtual_machine::INSTR_UINT8_PUSH, 0,        // 13: push index to GET_MEM
	t_simple_virtual_machine::INSTR_UINT8_GET_MEM,        // 15: push mem[0]
	t_simple_virtual_machine::INSTR_UINT8_PUSH, 1,        // 16: push 1
	t_simple_virtual_machine::INSTR_UINT8_ADD,            // 18: push (mem[0] + 1)
	t_simple_virtual_machine::INSTR_UINT8_SET_MEM,        // 19: write mem[0] = mem[0] + 1

	// t_simple_virtual_machine::INSTR_UINT8_GOTO_INS, 0,    // 20: jump back to program start
	t_simple_virtual_machine::INSTR_UINT8_GOTO_LBL, 123,    // 20: jump back to program start
};

// recursive Fibonacci
static const t_uint8 sample_vm_code_2[] = {
	t_simple_virtual_machine::INSTR_UINT8_LBL_ADDR,       //  0: label 0 as address of main
	t_simple_virtual_machine::LABEL_MAIN_FUNC,

	t_simple_virtual_machine::INSTR_UINT8_PUSH, 8,        //  2: push argument to fibo(n)
	t_simple_virtual_machine::INSTR_UINT8_JMP_FUNC, 4,    //  4: call fibo
	t_simple_virtual_machine::INSTR_UINT8_RET_FUNC, 0,    //  6: return from main, 0 args to pop (empty call-stack triggers exit)


	t_simple_virtual_machine::INSTR_UINT8_LBL_ADDR, 100,  //  8: dummy label
	t_simple_virtual_machine::INSTR_UINT8_LBL_ADDR, 101,  // 10: dummy label
	t_simple_virtual_machine::INSTR_UINT8_LBL_ADDR, 102,  // 12: dummy label


	t_simple_virtual_machine::INSTR_UINT8_LBL_ADDR, 4,    // 14: label 14 as address of fibo

	t_simple_virtual_machine::INSTR_UINT8_PEEK, 1,        // 16: push argument
	t_simple_virtual_machine::INSTR_UINT8_PUSH, 0,        // 18: push comparison literal
	t_simple_virtual_machine::INSTR_UINT8_CMP_EQ,         // 20: push result of (arg == 0)
	t_simple_virtual_machine::INSTR_UINT8_PUSH, 49,       // 21: push address of return instr
	t_simple_virtual_machine::INSTR_UINT8_JMP_ABS_IF,     // 23: goto "return 1" if 0==n

	t_simple_virtual_machine::INSTR_UINT8_PEEK, 1,        // 24: push argument
	t_simple_virtual_machine::INSTR_UINT8_PUSH, 1,        // 26: push comparison literal
	t_simple_virtual_machine::INSTR_UINT8_CMP_EQ,         // 28: push result of (arg == 1)
	t_simple_virtual_machine::INSTR_UINT8_PUSH, 49,       // 29: push address of return instr
	t_simple_virtual_machine::INSTR_UINT8_JMP_ABS_IF,     // 31: goto "return 1" if 1==n

	t_simple_virtual_machine::INSTR_YIELD_CPU,            // 32: sleep

	// note: can not push any return values on data stack
	// because that messes up the peek references for the
	// right-hand recursive branches --> need to pop them
	// after returning
	t_simple_virtual_machine::INSTR_UINT8_PEEK, 1,        // 33: push LHS operand to SUB
	t_simple_virtual_machine::INSTR_UINT8_PUSH, 1,        // 35: push RHS operand to SUB
	t_simple_virtual_machine::INSTR_UINT8_SUB,            // 37: push n-1
	t_simple_virtual_machine::INSTR_UINT8_JMP_FUNC, 4,    // 38: call fibo(n-1)

	t_simple_virtual_machine::INSTR_UINT8_PEEK, 1,        // 40: push LHS operand to SUB
	t_simple_virtual_machine::INSTR_UINT8_PUSH, 2,        // 42: push RHS operand to SUB
	t_simple_virtual_machine::INSTR_UINT8_SUB,            // 44: push n-2
	t_simple_virtual_machine::INSTR_UINT8_JMP_FUNC, 4,    // 45: call fibo(n-2)

	t_simple_virtual_machine::INSTR_UINT8_RET_FUNC, 1,    // 47: return from recursive call, pop 1 arg


	t_simple_virtual_machine::INSTR_UINT8_PUSH, 0,        // 49: push idx for SET_MEM
	t_simple_virtual_machine::INSTR_UINT8_PUSH, 0,        // 51: push idx for GET_MEM
	t_simple_virtual_machine::INSTR_UINT8_GET_MEM,        // 53: push mem[0]
	t_simple_virtual_machine::INSTR_UINT8_PUSH, 1,        // 54: push 1
	t_simple_virtual_machine::INSTR_UINT8_ADD,            // 56: push mem[0] + 1
	t_simple_virtual_machine::INSTR_UINT8_SET_MEM,        // 57: write mem[0] = mem[0] + 1


	t_simple_virtual_machine::INSTR_UINT8_RET_FUNC, 1,    // 58: return from recursive call, pop 1 arg
};



int main(int argc, char** argv) {
	// disable stdin buffering in the tty driver
	t_scoped_unbuffered_stdin_ctl ctl;

	{
		t_simple_virtual_machine vm_a;
		t_simple_virtual_machine vm_b;
		t_simple_virtual_machine vm_c;

		const bool vm_a_single_step = (argc >= 2 && (*argv[1] == 's'));
		const bool vm_b_single_step = (argc >= 3 && (*argv[2] == 's'));
		const bool vm_c_single_step = (argc >= 4 && (*argv[3] == 's'));

		if (false) {
			// first VM executes raw program code and dumps it to file when done
			vm_a.read_code(sample_vm_code_1, sizeof(sample_vm_code_1));

			while (!vm_a.run_code(vm_a.get_cur_prog_counter(), vm_a_single_step)) {
				vm_a.dump_state("[vm_a]");
			}

			vm_a.dump_state("[vm_a]");
			vm_a.write_code_file("vm_a_test_cond_jump_code.dat");
			vm_a.write_state_file("vm_a_test_cond_jump_state.dat");
		}

		if (false) {
			// second VM executes the dump, ends in identical execution state
			vm_b.read_code_file("vm_a_test_cond_jump_code.dat");
			// if this is uncommented, second VM will actually do nothing
			// vm_b.read_state_file("vm_a_test_cond_jump_state.dat");

			while (!vm_b.run_code(vm_b.get_cur_prog_counter(), vm_b_single_step)) {
				vm_b.dump_state("[vm_b]");
			}

			vm_b.dump_state("[vm_b]");
			vm_b.write_code_file("vm_b_test_cond_jump_code.dat");
			vm_b.write_state_file("vm_b_test_cond_jump_state.dat");
		}

		if (true) {
			vm_c.read_code(sample_vm_code_2, sizeof(sample_vm_code_2));

			while (!vm_c.run_code(vm_c.get_cur_prog_counter(), vm_c_single_step)) {
				vm_c.dump_state("[vm_c]");
			}

			vm_c.dump_state("[vm_c]");
			vm_c.write_code_file("vm_c_test_func_call_code.dat");
			vm_c.write_state_file("vm_c_test_func_call_state.dat");
		}
	}

	return 0;
}

