#include <string.h>
#include <stdio.h>

#define MAX_STANDARD_LEVEL_RAX 0x00000000u
#define MAX_EXTENDED_LEVEL_RAX 0x80000000u
#define MIN_STANDARD_LEVEL_SSE 0x00000001u
#define MIN_EXTENDED_LEVEL_SSE 0x80000001u

#define R_EAX 0
#define R_EBX 1
#define R_ECX 2
#define R_EDX 3

#if defined(__GNUC__)
	#define _noinline __attribute__((__noinline__))
#else
	#define _noinline
#endif



#if defined(__GNUC__)
	// can't allow this function to be inlined because of the asm
	_noinline
	void exec_cpuid(unsigned int* regs) {
	#ifndef __APPLE__
		__asm__ __volatile__(
			"cpuid"
			: "=a" (regs[R_EAX]), "=b" (regs[R_EBX]), "=c" (regs[R_ECX]), "=d" (regs[R_EDX])
			: "0" (regs[R_EAX])
		);
	#else
		#ifdef __x86_64__
			__asm__ __volatile__(
				"pushq %%rbx\n\t"
				"cpuid\n\t"
				"movl %%ebx, %1\n\t"
				"popq %%rbx"
				: "=a" (regs[R_EAX]), "=r" (regs[R_EBX]), "=c" (regs[R_ECX]), "=d" (regs[R_EDX])
				: "0" (regs[R_EAX])
			);
		#else
			__asm__ __volatile__(
				"pushl %%ebx\n\t"
				"cpuid\n\t"
				"movl %%ebx, %1\n\t"
				"popl %%ebx"
				: "=a" (regs[R_EAX]), "=r" (regs[R_EBX]), "=c" (regs[R_ECX]), "=d" (regs[R_EDX])
				: "0" (regs[R_EAX])
			);
		#endif
	#endif
	}

#elif defined(_MSC_VER)
	void exec_cpuid(unsigned int* regs) {
		unsigned int features[4];

		__cpuid(features, regs[R_EAX]);
		memcpy(&regs[R_EAX], &features[0], sizeof(unsigned int) * 4);
	}

#else
	// no-op on other compilers
	void exec_cpuid(unsigned int* regs) {
	}
#endif



int have_standard_level_sse_bits(unsigned int* regs) {
	// get the maximum standard level
	regs[R_EAX] = MAX_STANDARD_LEVEL_RAX;
	exec_cpuid(&regs[R_EAX]);

	if (regs[R_EAX] < MIN_STANDARD_LEVEL_SSE)
		return 0;

	regs[R_EAX] = MIN_STANDARD_LEVEL_SSE;
	exec_cpuid(&regs[R_EAX]);
	return 1;
}

int have_extended_level_sse_bits(unsigned int* regs) {
	// get the maximum extended level
	regs[R_EAX] = MAX_EXTENDED_LEVEL_RAX;
	exec_cpuid(&regs[R_EAX]);

	if (regs[R_EAX] < MIN_EXTENDED_LEVEL_SSE)
		return 0;

	regs[R_EAX] = MIN_EXTENDED_LEVEL_SSE;
	exec_cpuid(&regs[R_EAX]);
	return 1;
}



unsigned int cpu_standard_level_sse_bits(unsigned int* regs) {
	unsigned int bits = 0;

	if (have_standard_level_sse_bits(regs)) {
		const int  sse_42_bit = (regs[R_ECX] >> 20) & 1; bits |= ( sse_42_bit << 0); // SSE 4.2
		const int  sse_41_bit = (regs[R_ECX] >> 19) & 1; bits |= ( sse_41_bit << 1); // SSE 4.1
		const int ssse_30_bit = (regs[R_ECX] >>  9) & 1; bits |= (ssse_30_bit << 2); // Supplemental SSE 3.0
		const int  sse_30_bit = (regs[R_ECX] >>  0) & 1; bits |= ( sse_30_bit << 3); // SSE 3.0

		const int  sse_20_bit = (regs[R_EDX] >> 26) & 1; bits |= ( sse_20_bit << 4); // SSE 2.0
		const int  sse_10_bit = (regs[R_EDX] >> 25) & 1; bits |= ( sse_10_bit << 5); // SSE 1.0
		const int     mmx_bit = (regs[R_EDX] >> 23) & 1; bits |= (    mmx_bit << 6); // MMX
	}

	return bits;
}

unsigned int cpu_extended_level_sse_bits(unsigned int* regs) {
	unsigned int bits = 0;

	if (have_extended_level_sse_bits(regs)) {
		const int  sse_50a_bit = (regs[R_ECX] >> 11) & 1; bits |= (sse_50a_bit << 7); // SSE 5.0A
		const int  sse_40a_bit = (regs[R_ECX] >>  6) & 1; bits |= (sse_40a_bit << 8); // SSE 4.0A
		const int     msse_bit = (regs[R_ECX] >>  7) & 1; bits |= (   msse_bit << 9); // Misaligned SSE
	}

	return bits;
}

unsigned int get_cpu_sse_bits() {
	unsigned int regs[4] = {0, 0, 0, 0};
	unsigned int bits    = 0;

	bits |= cpu_standard_level_sse_bits(regs);
	bits |= cpu_extended_level_sse_bits(regs);

	return bits;
}



int main() {
	printf("[%s] sse_bits=%u\n", __FUNCTION__, get_cpu_sse_bits());
	return 0;
}

