#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/utsname.h>

/*
objdump -D -b binary -mi386 <stage1_code>
  start:
    0x00:   eb 04                   jmp    0x6

  unknown:
    0x02:   af                      scas   %es:(%edi),%eax
    0x03:   c2 bf a3                ret    $0xa3bf                    // garbage instrs --> constants

  init:
    0x06:   81 ec 00 01 00 00       sub    $0x100,%esp                // code is exactly 160 bytes long
    0x0c:   31 c9                   xor    %ecx,%ecx
  search_for_zero_byte:
    0x0e:   88 0c 0c                mov    %cl,(%esp,%ecx,1)
    0x11:   fe c1                   inc    %cl
    0x13:   75 f9                   jne    0x0e                       // back to start of loop if not 0

    0x15:   31 c0                   xor    %eax,%eax
    0x17:   ba ef be ad de          mov    $0xdeadbeef,%edx

  checksum_loop:
    0x1c:   02 04 0c                add    (%esp,%ecx,1),%al
    0x1f:   00 d0                   add    %dl,%al
    0x21:   c1 ca 08                ror    $0x8,%edx                  // right-rotate %edx = $0xdeadbeef
    0x24:   8a 1c 0c                mov    (%esp,%ecx,1),%bl
    0x27:   8a 3c 04                mov    (%esp,%eax,1),%bh
    0x2a:   88 1c 04                mov    %bl,(%esp,%eax,1)          // swap byte values
    0x2d:   88 3c 0c                mov    %bh,(%esp,%ecx,1)          // swap byte values
    0x30:   fe c1                   inc    %cl                        // run loop until %cl wraps to 0
    0x32:   75 e8                   jne    0x1c

    0x34:   e9 5c 00 00 00          jmp    0x95                       // nop/nop/recurse

  sub_39:
    0x39:   89 e3                   mov    %esp,%ebx
    0x3b:   81 c3 04 00 00 00       add    $0x4,%ebx
    0x41:   5c                      pop    %esp                       // shift esp to end-of-program
    0x42:   58                      pop    %eax                       // copy end-of-program bytes into %eax
    0x43:   3d 41 41 41 41          cmp    $0x41414141,%eax           // test if end of program looks sane ("AAAA")
    0x48:   75 43                   jne    0x8d
    0x4a:   58                      pop    %eax
    0x4b:   3d 42 42 42 42          cmp    $0x42424242,%eax           // test if stage1 b64-decoded data is present ("BBBB...")
    0x50:   75 3b                   jne    0x8d
    0x52:   5a                      pop    %edx
    0x53:   89 d1                   mov    %edx,%ecx
    0x55:   89 e6                   mov    %esp,%esi
    0x57:   89 df                   mov    %ebx,%edi
    0x59:   29 cf                   sub    %ecx,%edi
    0x5b:   f3 a4                   rep movsb %ds:(%esi),%es:(%edi)   // memcpy stage1 data (%ecx bytes) from %esi to %edi
    0x5d:   89 de                   mov    %ebx,%esi
    0x5f:   89 d1                   mov    %edx,%ecx
    0x61:   89 df                   mov    %ebx,%edi
    0x63:   29 cf                   sub    %ecx,%edi
    0x65:   31 c0                   xor    %eax,%eax
    0x67:   31 db                   xor    %ebx,%ebx
    0x69:   31 d2                   xor    %edx,%edx
    0x6b:   fe c0                   inc    %al
    0x6d:   02 1c 06                add    (%esi,%eax,1),%bl
    0x70:   8a 14 06                mov    (%esi,%eax,1),%dl
    0x73:   8a 34 1e                mov    (%esi,%ebx,1),%dh
    0x76:   88 34 06                mov    %dh,(%esi,%eax,1)
    0x79:   88 14 1e                mov    %dl,(%esi,%ebx,1)
    0x7c:   00 f2                   add    %dh,%dl
    0x7e:   30 f6                   xor    %dh,%dh
    0x80:   8a 1c 16                mov    (%esi,%edx,1),%bl
    0x83:   8a 17                   mov    (%edi),%dl
    0x85:   30 da                   xor    %bl,%dl
    0x87:   88 17                   mov    %dl,(%edi)
    0x89:   47                      inc    %edi
    0x8a:   49                      dec    %ecx
    0x8b:   75 de                   jne    0x6b
    0x8d:   31 db                   xor    %ebx,%ebx
    0x8f:   89 d8                   mov    %ebx,%eax
    0x91:   fe c0                   inc    %al
    0x93:   cd 80                   int    $0x80                      // int syscall (eax=1 --> exit)
    0x95:   90                      nop
    0x96:   90                      nop
    0x97:   e8 9d ff ff ff          call   0x39                       // recurse
    0x9c:   41                      inc    %ecx                       // end-of-program bytes (never read as instrs)
    0x9d:   41                      inc    %ecx
    0x9e:   41                      inc    %ecx
    0x9f:   41                      inc    %ecx
*/
static char stage1_code[16*10] = {
	0xeb, 0x04, 0xaf, 0xc2, 0xbf, 0xa3, 0x81, 0xec, 0x00, 0x01, 0x00, 0x00, 0x31, 0xc9, 0x88, 0x0c,
	0x0c, 0xfe, 0xc1, 0x75, 0xf9, 0x31, 0xc0, 0xba, 0xef, 0xbe, 0xad, 0xde, 0x02, 0x04, 0x0c, 0x00,
	0xd0, 0xc1, 0xca, 0x08, 0x8a, 0x1c, 0x0c, 0x8a, 0x3c, 0x04, 0x88, 0x1c, 0x04, 0x88, 0x3c, 0x0c,
	0xfe, 0xc1, 0x75, 0xe8, 0xe9, 0x5c, 0x00, 0x00, 0x00, 0x89, 0xe3, 0x81, 0xc3, 0x04, 0x00, 0x00,
	0x00, 0x5c, 0x58, 0x3d, 0x41, 0x41, 0x41, 0x41, 0x75, 0x43, 0x58, 0x3d, 0x42, 0x42, 0x42, 0x42,
	0x75, 0x3b, 0x5a, 0x89, 0xd1, 0x89, 0xe6, 0x89, 0xdf, 0x29, 0xcf, 0xf3, 0xa4, 0x89, 0xde, 0x89,
	0xd1, 0x89, 0xdf, 0x29, 0xcf, 0x31, 0xc0, 0x31, 0xdb, 0x31, 0xd2, 0xfe, 0xc0, 0x02, 0x1c, 0x06,
	0x8a, 0x14, 0x06, 0x8a, 0x34, 0x1e, 0x88, 0x34, 0x06, 0x88, 0x14, 0x1e, 0x00, 0xf2, 0x30, 0xf6,
	0x8a, 0x1c, 0x16, 0x8a, 0x17, 0x30, 0xda, 0x88, 0x17, 0x47, 0x49, 0x75, 0xde, 0x31, 0xdb, 0x89,
	0xd8, 0xfe, 0xc0, 0xcd, 0x80, 0x90, 0x90, 0xe8, 0x9d, 0xff, 0xff, 0xff, 0x41, 0x41, 0x41, 0x41,
};

static char stage1_data[16*3 + 10] = {
	0x42, 0x42, 0x42, 0x42, 0x32, 0x00, 0x00, 0x00,  0x91, 0xd8, 0xf1, 0x6d, 0x70, 0x20, 0x3a, 0xab,
	0x67, 0x9a, 0x0b, 0xc4, 0x91, 0xfb, 0xc7, 0x66,  0x0f, 0xfc, 0xcd, 0xcc, 0xb4, 0x02, 0xfa, 0xd7,
	0x77, 0xb4, 0x54, 0x38, 0xab, 0x1f, 0x0e, 0xe3,  0x8e, 0xd3, 0x0d, 0xeb, 0x99, 0xc3, 0x93, 0xfe,
	0xd1, 0x2b, 0x1b, 0x11, 0xc6, 0x11, 0xef, 0xc8,  0xca, 0x2f,
};

static const char rc4_decrypt[] = {
	0xba, 0x31, 0x00, 0x00, 0x00, // mov edx, 0x40
	0x8d, 0x4f, 0xce,             // lea ecx, [edi-0x32]
	0x31, 0xdb,                   // xor ebx, ebx
	0x43,                         // inc ebx (stdout)
	0x31, 0xc0,                   // xor eax, eax
	0xb0, 0x04,                   // add al, 0x4
	0xcd, 0x80,                   // int syscall (eax=4 --> write)
	0x31, 0xdb,                   // xor ebx,ebx
	0x43,                         // inc ebx
	0x31, 0xd2,                   // xor edx,edx
	0x42,                         // inc edx
	0x68, 0x0a, 0x00,0x00, 0x00,  // push 0xa
	0x8d, 0x0c, 0x24,             // lea ecx,[esp]
	0xb8, 0x04, 0x00,0x00, 0x00,  // mov eax, 0x4
	0xcd, 0x80,                   // int syscall (eax=4 --> write)
	0x31, 0xdb,                   // xor ebx,ebx
	0x31, 0xc0,                   // xor eax,eax
	0x40,                         // inc eax
	0xcd, 0x80,                   // int syscall (eax=1 --> exit)
};

uint32_t patch_mem(char* ptr, size_t size) {
	uint32_t i;

	// look for the first interrupt syscall (0x80)
	for (i = 0; i < size; i++) {
		if (*(uint16_t*) &ptr[i] == 0x80cd) {
			*(uint16_t*) &ptr[i] = 0x45eb;
			return 0;
		}
	}

	return 1;
}

uint32_t bad_cpu_arch(void) {
	if (sizeof(void*) != 4)
		return 1;

	struct utsname kernel_info;

	uname(&kernel_info);

	// x86-32 assembly, so can not run in 64-bit mode
	return (strcmp(kernel_info.machine, "i686") != 0);
}

int main(int argc, char** argv) {
	(void) argc;
	(void) argv;

	if (bad_cpu_arch()) {
		printf("[%s] can only execute on x86-32 machine\n", __FUNCTION__);
		return EXIT_FAILURE;
	}

	void* mem = memalign(4096, 4096);

	if (mem == NULL) {
		printf("[%s] failed allocating page-aligned memory: %s\n", strerror(errno), __FUNCTION__);
		return EXIT_FAILURE;
	}

	memset(mem, 0, 4096);

	// memory-page in which code is embedded must be executable
	if (mprotect(mem, 4096, PROT_READ | PROT_WRITE | PROT_EXEC)) {
		printf("[%s] failed setting page-permissions: %s\n", strerror(errno), __FUNCTION__);
		return EXIT_FAILURE;
	}

	// combine all of stage1
	memcpy(mem,                                             stage1_code, sizeof(stage1_code));
	memcpy(mem + sizeof(stage1_code),                       stage1_data, sizeof(stage1_data));
	memcpy(mem + sizeof(stage1_code) + sizeof(stage1_data), rc4_decrypt, sizeof(rc4_decrypt));

	if (patch_mem((char*) mem, sizeof(stage1_code))) {
		printf("[%s] failed to patch memory\n", __FUNCTION__);
		return EXIT_FAILURE;
	}

	// execute the page
	((int(*)(void)) mem)();
	return EXIT_SUCCESS;
}

