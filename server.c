#include "drepper_mutex.h"
#include "shared.h"

int main() {
  offload_process_t *shm_seg = get_shm_ptr(true);

  // keep getting data from the client and displaying it
  printf("shm_seg->futex_turn address: %p\n", &shm_seg->futex_turn);
  int n = 0;
  while (true) {
    // wait for data
    // this will only ever happen once and once we start forking it will never
    // happen

    fxsem_up(&shm_seg->empty);  // tells the client that the buffer is now
                                // empty/ready for writing
    fxsem_down(&shm_seg->full); // wait until the client has populated the
                                // buffer with fresh data
    drepper_lock(&shm_seg->data_mtx);
    printf("[ ");
    for (int i = 0; i < OP_BUFFER_SIZE; ++i) {
      printf("%d ", shm_seg->op_buffer[i]);
    }
    printf("]\n");
    drepper_unlock(&shm_seg->data_mtx);
    // the thing needs to explicitly wait for the other side to finish its turn
    // before heading into a drepper_lock

    /*
    // the futex_turn gets set before the client starts the FUTEX_WAIT syscall
    // and as such, the client immediiately increments the buffer
    shm_seg->futex_turn = CLIENT; // TODO cmpxchg this atomically between server
                                  // and client, not that it should matter...

    // by the time we get here, the client has already incremented the buffer
    // one more time, and is trying to wake up this thread, even though we
    // haven't slept yet!
    if (shm_seg->futex_turn == CLIENT) {
      futex_wake(&shm_seg->futex_turn);
    }*/
    ++n;
  }
}
