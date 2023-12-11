// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#include "os_graph.h"
#include "os_threadpool.h"
#include "log/log.h"
#include "utils.h"

#define NUM_THREADS		4

static int sum;
static os_graph_t *graph;
static os_threadpool_t *tp;
/* TODO: Define graph synchronization mechanisms. */
pthread_mutex_t mutex;
/* TODO: Define graph task argument. */
static void process_node_task(void *idx)
{
	os_node_t *node;

	pthread_mutex_lock(&mutex);
	node = graph->nodes[(unsigned int)idx];
	sum += node->info;
	graph->visited[(unsigned int)idx] = DONE;

	pthread_mutex_unlock(&mutex);

	for (unsigned int i = 0; i < node->num_neighbours; i++) {
		pthread_mutex_lock(&mutex);
		if (graph->visited[node->neighbours[i]] == NOT_VISITED) {
			graph->visited[node->neighbours[i]] = DONE;
			os_task_t *task = create_task(process_node_task, node->neighbours[i], NULL);
			pthread_mutex_unlock(&mutex);
			enqueue_task(tp, task);
			sem_post(&tp->semaphore);
		} else {
			pthread_mutex_unlock(&mutex);
		}
	}

}

static void process_node(unsigned int idx)
{
	/* TODO: Implement thread-pool based processing of graph. */
	os_task_t *task = create_task(process_node_task, idx, NULL);
	enqueue_task(tp, task);
	sem_post(&tp->semaphore);
}

int main(int argc, char *argv[])
{
	FILE *input_file;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s input_file\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	input_file = fopen(argv[1], "r");
	DIE(input_file == NULL, "fopen");

	graph = create_graph_from_file(input_file);

	/* TODO: Initialize graph synchronization mechanisms. */
	int rc = pthread_mutex_init(&mutex, NULL);
	DIE(rc != 0, "pthread_mutex_init");
	
	tp = create_threadpool(NUM_THREADS);
	process_node(0);
	wait_for_completion(tp);
	destroy_threadpool(tp);

	printf("%d", sum);

	return 0;
}
