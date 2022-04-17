# multiarch producer-consumer problem


### what?



### make it work

#### archlinux arm side
* set up the qemu binfmt thingy
* go get an archlinux arm image (for whatever arch you want but i do armv7l)
* go `arch-chroot` into the arm image after you extract it and set up the keys like it tells you to
* install `base-devel`
* bindmount this repo into your chroot and then `cc -lpthread -o encoder_arm encoder.c`

##### host side
* `cc -lpthread -o client client.c`

#### start it
* host side: `./client` THEN
* arm side: `./encoder_arm` 


thanks to Ulrich Drepper's fun whitepaper, and using mutexes in a really wrong way (unlocking mutexes from a process that doesn't even hold a lock on it), behold.
synchronisation. (happy face)

