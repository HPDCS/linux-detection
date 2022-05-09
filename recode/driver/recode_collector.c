#include <linux/vmalloc.h>

#include "recode_collector.h"

DEFINE_PER_CPU(struct data_collector *, pcpu_data_collector);

struct data_collector *init_collector(unsigned cpu)
{
	struct data_collector *dc = NULL;

	// We use a flexbile array to generate p_s_rings
	dc = vzalloc(sizeof(struct data_collector) + BUFFER_MEMORY_SIZE);
	if (!dc)
		goto end;

	dc->cpu = cpu;
	dc->size = BUFFER_MEMORY_SIZE;

end:
	return dc;
}

void fini_collector(unsigned cpu)
{
	vfree(per_cpu(pcpu_data_collector, cpu));
}

struct data_collector_sample *get_write_dc_sample(struct data_collector *dc,
						  unsigned hw_events_cnt)
{
	unsigned wr_i, rd_i, req_size;
	struct data_collector_sample *dc_sample = NULL;

	if (hw_events_cnt <= 0 || !dc)
		goto end;

	wr_i = dc->wr_i;
	rd_i = dc->rd_i;

	req_size = sizeof(struct data_collector_sample) +
		   (sizeof(pmc_ctr) * hw_events_cnt);

	pr_debug("WR SAMPLE - size %lu - req_size %u - wr_i %u, rd_i %u\n", dc->size, req_size, wr_i, rd_i);

	/* There is space in the following buffer elements */
	if (wr_i + req_size <= dc->size) {
		if (rd_i <= wr_i || wr_i + req_size < rd_i) {
			dc_sample =
				(struct data_collector_sample *)(dc->raw_memory +
								 wr_i);
			dc->wr_p = wr_i + req_size;
		} else {
		}
	} else if (req_size <= rd_i) {
		dc_sample = (struct data_collector_sample *)dc->raw_memory;
		dc->ov_i = wr_i;
		dc->wr_p = 0;
	} else {
		pr_debug(
			"WR SAMPLE Unexpected - wr_i %u, rd_i %u, req_size %u\n",
			wr_i, rd_i, req_size);
	}

	if (dc_sample)
		dc_sample->pmcs.cnt = hw_events_cnt;

end:
	return dc_sample;
}

void put_write_dc_sample(struct data_collector *dc)
{
	if (!dc)
		return;

	dc->wr_i = dc->wr_p;
}

struct data_collector_sample *get_read_dc_sample(struct data_collector *dc)
{
	unsigned wr_i, rd_i, req_size;
	struct data_collector_sample *dc_sample;

	if (!dc)
		return NULL;

	wr_i = dc->wr_i;
	rd_i = dc->rd_i;

	/* Empty buffer */
	if (rd_i == wr_i)
		return NULL;

	/* Reset index when there is an overflow */
	if (rd_i == dc->ov_i)
		rd_i = 0;

	dc_sample = (struct data_collector_sample *)(dc->raw_memory + rd_i);

	req_size = sizeof(struct data_collector_sample) +
		   (sizeof(pmc_ctr) * dc_sample->pmcs.cnt);

	dc->rd_p = rd_i + req_size;


	return dc_sample;
}

void put_read_dc_sample(struct data_collector *dc)
{
	if (!dc)
		return;

	dc->rd_i = dc->rd_p;
}

bool check_read_dc_sample(struct data_collector *dc)
{
	pr_debug("RD CHECK wr_i %u, rd_i %u, RES: %u\n", dc->wr_i, dc->rd_i,
		 dc && (dc->rd_i != dc->wr_i));
	return dc &&
	       (dc->rd_i != dc->wr_i) && /* If rd == wr, then no sample */
	       (dc->rd_i != dc->ov_i || dc->wr_i != 0);
}