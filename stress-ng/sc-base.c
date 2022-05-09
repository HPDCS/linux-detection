#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "sc-attacks.h"

#define USEC (1)
#define MSEC (1000 * USEC)
#define SEC (1000 * MSEC)

static unsigned init = 0;
static struct timespec start, end;
static unsigned delay = 100 * MSEC; // 100 ms

unsigned init_sc(void)
{
	long delta_us;
	switch (init)
	{
	case 0:
		srand(time(NULL));
		clock_gettime(CLOCK_MONOTONIC_RAW, &start);
		delay = delay + (rand() % (3 * SEC)); // At least 100 Ms + up to 1 Sec
		init = 1;
		break;
	case 1:
		clock_gettime(CLOCK_MONOTONIC_RAW, &end);
		delta_us = (end.tv_sec - start.tv_sec) * 1000000 +
			   (end.tv_nsec - start.tv_nsec) / 1000;
		if (delta_us < delay)
			break;
		init = 2;
		printf("Side Channel enabled after %lu us\n", delta_us);
		[[fallthrough]];
	default:
		return (rand() % 100) > 50;
	}

	return 0;
}