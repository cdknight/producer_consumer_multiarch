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

// wait till the wait_val is satisifed
void futex_wait1(uint32_t *futex_word, offload_turn_t wait_val) {
  // Something something an Eli Bendersky blog post
  printf("beginning futex_wait at addr %p\n", futex_word);
  if (*futex_word == wait_val) {
    printf("futex word already is val, returning\n");
    return;
  }

  while (true) {
    // is using a val already equal to *futex_word really the best idea?
    int futex_retval =
        syscall(SYS_futex, futex_word, FUTEX_WAIT, *futex_word, NULL, NULL, 0);
    // basically, the futex call failed
    if (futex_retval == -1) {
      perror("futex wait fail");
      if (errno == EAGAIN) {
        exit(1); // sad face
      }
    }
    // the futex call was fine (and it must have finished waiting if it gets
    // here)
    else if (futex_retval == 0) {
      printf("futex_wait awoken; *futex_word: %d, want: %d\n", *futex_word,
             wait_val);
      if (*futex_word == wait_val) {
        return;
      } // otherwise it was a suprious wakeup or something and so the loop
        // should go again
    } else {
      abort(); // kills the program in a sad way, I think
    }
  }
}

void futex_wake1(uint32_t *futex_word) {
  printf("beginning futex_wake at addr %p\n", futex_word);
  while (true) {
    int futex_retval =
        syscall(SYS_futex, futex_word, FUTEX_WAKE, 1, NULL, NULL, 0);
    if (futex_retval == -1) {
      perror("futex wake fail");
      exit(1);
    } else if (futex_retval > 0) {
      printf("futex_wake returned %d\n", futex_retval);
      return;
    } else if (futex_retval == 0) {
      // printf("no waiters on the address, reloop\n");
    }
    // if it returns 0 then there were literally no waiters on the futex word,
    // which is what's happening here...
  }
}
