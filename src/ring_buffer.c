// SPDX-License-Identifier: BSD-3-Clause

#include "ring_buffer.h"

int ring_buffer_init(so_ring_buffer_t *ring, size_t cap)
{
	/* TODO: implement ring_buffer_init */
	if (!ring || cap == 0)
		return -1;

	ring->data = (char *)calloc(cap, sizeof(char));
	DIE(!(ring->data), "calloc failed");

	ring->timestamps = (size_t *)calloc(50000, sizeof(so_packet_t));
	DIE(!(ring->timestamps), "calloc failed");

	ring->read_pos = 0;
	ring->write_pos = 0;
	ring->cap = cap;
	ring->len = 0;
	ring->prod_done = 0;
	ring->pos_time = 0;
	ring->len_time = 0;

	pthread_mutex_init(&(ring->lock), NULL);
	pthread_cond_init(&(ring->cond_empty), NULL);
	pthread_cond_init(&(ring->cond_full), NULL);
	pthread_cond_init(&(ring->cond_time), NULL);

	return 1;
}

ssize_t ring_buffer_enqueue(so_ring_buffer_t *ring, void *data, size_t size)
{
	/* TODO: implement ring_buffer_enqueue */
	if (!ring || !data || size <= 0) // argumente invalide
		return -1;

	pthread_mutex_lock(&(ring->lock)); // lock la mutex ca alte thread-uri sa nu poata face modif
	while (size > (ring->cap - ring->len)) // nu este suficient spatiu in buffer
		pthread_cond_wait(&(ring->cond_full), &(ring->lock));

	size_t end_size = size > (ring->cap - ring->write_pos) ? ring->cap - ring->write_pos : size;

	memcpy(ring->data + ring->write_pos, data, end_size);
	if (end_size < size) { // pt circularitate (pun la final de buffer dar nu incape si adaug si la inceput)
		size_t begin_size = size - end_size;

		memcpy(ring->data, (char *) data + end_size, begin_size);
	}

	ring->len += size;
	ring->write_pos = (ring->write_pos + size) % ring->cap;
	*(ring->timestamps + ring->len_time) = ((so_packet_t *)data)->hdr.timestamp;
	ring->len_time++;

	pthread_mutex_unlock(&(ring->lock)); // unlock pt ca am terminat cu modif de la thread-ul curent
	pthread_cond_broadcast(&(ring->cond_empty)); // da signal la consumer ca exista data avalable

	return size;
}

ssize_t ring_buffer_dequeue(so_ring_buffer_t *ring, void *data, size_t size)
{
	/* TODO: Implement ring_buffer_dequeue */
	if (!ring || !data || size <= 0) // argumente invalide
		return -1;

	pthread_mutex_lock(&(ring->lock)); // lock la mutex ca alte thread-uri sa nu poata face modif
	while (!(ring->len) && !(ring->prod_done)) // buffer-ul e gol si astept sa adauge producer-ul in buffer
		pthread_cond_wait(&(ring->cond_empty), &(ring->lock));

	if (!(ring->len) && ring->prod_done) { // daca buffer-ul e gol si producer e gata am terminat toate datele
		pthread_mutex_unlock(&(ring->lock)); // unlock pt ca am terminat cu modif de la thread-ul curent
		return -2;
	}

	size_t end_size = size > (ring->cap - ring->read_pos) ? ring->cap - ring->read_pos : size;

	memcpy(data, ring->data + ring->read_pos, end_size);
	if (end_size < size) { // pt circularitate (scot de la final de buffer dar mai am si la inceput)
		size_t begin_size = size - end_size;

		memcpy((char *) data + end_size, ring->data, begin_size);
	}

	ring->len -= size;
	ring->read_pos = (ring->read_pos + size) % ring->cap;

	pthread_mutex_unlock(&(ring->lock)); // unlock pt ca am terminat cu modif de la thread-ul curent
	pthread_cond_broadcast(&(ring->cond_full)); // da signal la producer ca poate adauga

	return size;
}

void ring_buffer_destroy(so_ring_buffer_t *ring)
{
	/* TODO: Implement ring_buffer_destroy */
	if (!ring)
		return;

	free(ring->data);
	free(ring->timestamps);
	ring->data = NULL;
	ring->read_pos = 0;
	ring->write_pos = 0;
	ring->len = 0;
	ring->cap = 0;
	ring->prod_done = 1;

	pthread_mutex_destroy(&(ring->lock));
	pthread_cond_destroy(&(ring->cond_empty));
	pthread_cond_destroy(&(ring->cond_full));
}

void ring_buffer_stop(so_ring_buffer_t *ring)
{
	/* TODO: Implement ring_buffer_stop */
	if (!ring)
		return;

	pthread_mutex_lock(&(ring->lock)); // lock la mutex ca alte thread-uri sa nu poata face modif
	ring->prod_done = 1; // producer e gata
	pthread_mutex_unlock(&(ring->lock)); // unlock pt ca am terminat cu modif de la thread-ul curent
	pthread_cond_broadcast(&(ring->cond_empty)); // dau unlock la toate thread-urile blocate de cond
}
