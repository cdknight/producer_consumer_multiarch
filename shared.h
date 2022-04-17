#pragma once

#include "drepper_mutex.h"
#include "fxsem.h"
#include <errno.h>
#include <linux/futex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/syscall.h>
#include <unistd.h>

#define IPC_KEY 35576
#define OP_BUFFER_SIZE 8

typedef enum { SERVER, CLIENT, FINISHED } offload_turn_t;

typedef struct {
  uint32_t op_buffer[OP_BUFFER_SIZE];
  uint32_t futex_turn;
  drepper_mutex_t data_mtx;
  fxsem_t empty; // to block the client from adding more data until the server
                 // has emptied the buffer
  fxsem_t full;  // to block the server from printing until the client has
                 // populated the buffer
} offload_process_t;

// TODO move to shm_open
offload_process_t *get_shm_ptr(bool create) {
  int shm_id = shmget(IPC_KEY, sizeof(offload_process_t), IPC_CREAT | 0777);
  offload_process_t *shm_dat = (offload_process_t *)shmat(shm_id, NULL, 0);
  if (create) {
    shm_dat->futex_turn = CLIENT; // start off by offloading to the client
    // reset buffer
    for (int i = 0; i < OP_BUFFER_SIZE; ++i) {
      shm_dat->op_buffer[i] = 0;
    }
    drepper_init(&shm_dat->data_mtx);
    fxsem_init(&shm_dat->empty);
    fxsem_init(&shm_dat->full);
  }
  return shm_dat;
}
