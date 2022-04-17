#pragma once

#include "drepper_mutex.h"
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
  drepper_mutex_t cnt_lck; // client lock
  drepper_mutex_t srv_lck; // server lock
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
    drepper_init(&shm_dat->srv_lck);
    drepper_init(&shm_dat->cnt_lck);
  }
  return shm_dat;
}
