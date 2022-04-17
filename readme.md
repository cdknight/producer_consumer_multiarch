# Multiarch Producer-Consumer Problem


### What?

I had the bright idea of trying to call functions from a random ARMv7 (32-bit) `.so` file pulled off of an embedded system. Easy, right? Just like,
write a header file (someone already did this for my library), make some binary that calls those functions, compile with the right parameters, and things work?

Yeah, easy. But... did I specify I wanted to call those random functions from a **native amd64** binary? 
Not so easy anymore, is it...

Basically my solution to this problem boiled down to this:

* I decided to run a process in a QEMU emulated environment, which calls my ARMv7 library
* In the producer-consumer problem, we label the emulated environment binary the producer, and the x86 library/binary the consumer.
* I use a piece of shared memory between the host environment and the emulated environment to send data and recieve to/from the producer.
* In order to synchronise the execution (eg. wait for the producer to finish its function, wait for the consumer to hand-off execution to the producer, etc.), I **spun the CPU**, wasting CPU cycles and making my program horribly inefficient. This was because I had no idea that **condition variables** were a thing.
* I then tried using `pthread_cond_t` and other things from the standard library. However, nothing worked. But condition variables worked when I tried an x86 to x86 approach (eg. nothing in an emulator). The **reason you can't just use `pthread_cond_t` for this is since the struct sizes are different depending on CPU arch, so when you try instantiating a conditional variable in the host environment the ARM libc is going to be addressing completely different parts of the C struct due to the size differences.** You thus have to implement your own synchronisation types that are **the same size on both platforms.** 
* The best way to do this, at least on Linux, is using the `futex(2)` system call. I am not smart enough to figure out how to implement a mutex myself (because of all the crazy race conditions), but Ulrich Drepper is. So I read his whitepaper and implemented a mutex using futexes. Maybe I am smart enough to implement my own semaphore, which can also be used to solve this problem. In fact, I believe I am using my mutex more like a semaphore than a mutex. So... maybe I should just use a sempahore.
* So I used Ulrich Drepper's mutex implementation that uses futexes, which will only require a 32-bit integer regardless of platform, and now I am able to solve this variant of the producer-consumer problem **without wasting CPU cycles and spinning my CPU.** Yay!

**This repo is my testing/experimentation code that proves that I can indeed synchronise two processes in an ARMv7 and an x86 environment using SHM.** Now that this works, it should be very easy to just scale this to my library-calling project. 

### Try It

#### ArchLinux ARM Side
* Set up the qemu-binfmt. 
* Get an ArchLinux ARM image (for whatever arch you want but I do ARMv7l)
* Go `arch-chroot` into the arm image after you extract it, and set up the keys so you can install software
* Install `base-devel`
* Bindmount this repo into your chroot and then `cc -lpthread -o client client.c`

##### Host side
* `cc -lpthread -o server server.c`

#### Start it
* Host side: `./server` THEN
* ARM side: `./client`

I plan to make the server automatically spawn the QEMU process as a child in the future (most of this will go into a different repo)

Thanks to Ulrich Drepper's fun whitepaper, and using mutexes in a really wrong way (unlocking mutexes from a process that doesn't even hold a lock on it), behold.
Synchronisation. 

