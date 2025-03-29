/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __SO_RINGBUFFER_H__
#define __SO_RINGBUFFER_H__

#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "../utils/utils.h"
#include "packet.h"

typedef struct so_ring_buffer_t {
	char *data;

	size_t read_pos;
	size_t write_pos;

	size_t len;
	size_t cap;

	/* TODO: Add syncronization primitives */
	pthread_mutex_t lock; // mutex-ul pt sincronizare producer-consumer
	pthread_cond_t cond_full; // conditie pt buffer plin
	pthread_cond_t cond_empty; // conditie pt buffer gol
	int prod_done; // 1 - daca producer-ul este gata, 0 - otherwise

	size_t *timestamps; // buffer de salvat timestamp-urile in ordine
	size_t pos_time; // pozitia timestamp-ului care trebuie afisat
	size_t len_time; // cate timestamp-uri am in buffer
	pthread_cond_t cond_time; // conditie pt sincronizare/ordonare timestamps
} so_ring_buffer_t;

int     ring_buffer_init(so_ring_buffer_t *rb, size_t cap);
ssize_t ring_buffer_enqueue(so_ring_buffer_t *rb, void *data, size_t size);
ssize_t ring_buffer_dequeue(so_ring_buffer_t *rb, void *data, size_t size);
void    ring_buffer_destroy(so_ring_buffer_t *rb);
void    ring_buffer_stop(so_ring_buffer_t *rb);

#endif /* __SO_RINGBUFFER_H__ */
