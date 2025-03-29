// SPDX-License-Identifier: BSD-3-Clause

#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>

#include "consumer.h"
#include "ring_buffer.h"
#include "packet.h"
#include "utils.h"

void *consumer_thread(void *ct)
{
	/* TODO: implement consumer thread */
	so_packet_t *pkt = calloc(1, sizeof(so_packet_t));
	so_consumer_ctx_t *ctx = (so_consumer_ctx_t *)ct;
	ssize_t size = 0;

	while (size != -2) {
		so_ring_buffer_t *rb = ctx->producer_rb;

		size = ring_buffer_dequeue(ctx->producer_rb, pkt, sizeof(so_packet_t));
		ctx->producer_rb = rb;
		if (size > 0) {
			char out_buf[256] = "";

			int len = snprintf(out_buf, 256, "%s %016lx %lu\n", RES_TO_STR(process_packet(pkt)),
								packet_hash(pkt), pkt->hdr.timestamp);

			pthread_mutex_lock(&(ctx->producer_rb->lock));
			while (*(ctx->producer_rb->timestamps + ctx->producer_rb->pos_time) != pkt->hdr.timestamp)
				pthread_cond_wait(&(ctx->producer_rb->cond_time), &(ctx->producer_rb->lock));
			ctx->producer_rb->pos_time++;

			write(ctx->fd, out_buf, len);
			pthread_mutex_unlock(&(ctx->producer_rb->lock));
			pthread_cond_broadcast(&(ctx->producer_rb->cond_time));
		}
	}
	free(pkt);
	return NULL;
}

int create_consumers(pthread_t *tids,
					 int num_consumers,
					 struct so_ring_buffer_t *rb,
					 const char *out_filename)
{
	if (!tids || num_consumers <= 0 || !rb || !out_filename)
		return -1;

	int fd = open(out_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);

	DIE(fd < 0, "open failed");

	so_consumer_ctx_t *ctxs = (so_consumer_ctx_t *)calloc(num_consumers, sizeof(so_consumer_ctx_t));

	DIE(!ctxs, "calloc failed");

	for (int i = 0; i < num_consumers; i++) {
		/*
		 * TODO: Launch consumer threads
		 **/
		(ctxs + i)->producer_rb = rb;
		(ctxs + i)->fd = fd;

		if (pthread_create(tids + i, NULL, consumer_thread, (void *)(ctxs + i))) {
			// daca ia fail la crearea unui thread nou
			free(ctxs);
			return -1;
		}
	}

	return num_consumers;
}
