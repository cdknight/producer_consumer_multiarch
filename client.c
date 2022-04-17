
#include "drepper_mutex.h"
#include "shared.h"
#include <time.h>

int main() {
  offload_process_t *shm_seg = get_shm_ptr(false);

  // keep getting data from the client and displaying it
  printf("shm_seg->futex_turn address: %p\n", &shm_seg->futex_turn);
  while (true) {
    // waits until it's NOT server.
    // futex_wait(&shm_seg->futex_turn, CLIENT);
    fxsem_down(
        &shm_seg->empty); // wait until the buffer is empty/cleared for writing
    drepper_lock(&shm_seg->data_mtx);
    if (shm_seg->futex_turn == FINISHED) {
      exit(0);
    }
    for (int i = 0; i < OP_BUFFER_SIZE; ++i) {
      shm_seg->op_buffer[i]++;
      // printf("shm_seg[%d] %d\n", i, shm_seg->op_buffer[i]);
    }
    // sleep for 0.2s since otherwise the program moves too fast (since the
    // client's operations are not expensive enough), causing the CPU to max out
    // (incrementing 8 integers is really not very real-life).
    // nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);

    drepper_unlock(&shm_seg->data_mtx);
    fxsem_up(&shm_seg->full); // tell the consumer that the buffer is full and
                              // ready for consumption
    /*shm_seg->futex_turn = SERVER; // TODO cmpxchg this atomically between
    server
                                  // and client, not that it should matter...
    if (shm_seg->futex_turn == SERVER) {
      futex_wake(&shm_seg->futex_turn);
    }*/
  }
}
