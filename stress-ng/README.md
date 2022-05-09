# stress-ng Side-Channel HACK (SC-HACK)


This modified version of the stress-ng suite allows users to specify, at compile-time, whether or not some selected benchmarks have to include side-channel code.

Every patched benchmark behaves regularly. After some random delay (less than 3 seconds), the side-channel function is enabled, and every time the benchmark test function is invoked, the injected routine is also called. However, the probability of executing the side-channel procedure is 10%. In the other case, the function directly returns to the benchmark activity.


## Building stress-ng SC-HACK


We try our best to automate the compilation phase, but keep in mind that this hack is for research purposes.

We took side-channel code from two different projects (Github):
  * https://github.com/Secure-AI-Systems-Group/Mastik
  * https://github.com/vusec/xlate
  
As preliminary step, extract the archive inside the attack folder

To compile a custom version of the stress-ng suite, execute:
```
	make ATTACK=FR ATTACK_TARGET="<path-to-library>"
```
e.g.
```
	make ATTACK=FR ATTACK_TARGET="/usr/lib/libfftw_threads.so.2.0.7"
```
and
```
	make ATTACK=<type> ATTACK_TARGET="attack/openssl-1.0.1e/libcrypto.so"
```
where type can be XFR, XA or XP.
You can provide as ATTACK_TARGET your custom version of the openssl library, but to make the attack praticable, you need a special compiled version.
(See  https://github.com/vusec/xlate for details).


## Running stress-ng SC-HACK


Indeed, to change the attack type, you need to re-compie the entire suite issuing the new make parameters.

The completed list of injectable benchmarks:
  * cpu
  * access
  * alarm
  * env
  * fault
  * file-ioctl
  * get
  * icache
  * itimer
  * l1cache
  * malloc
  * memcpy
  * open
  * poll
  * pthread
  * remap
  * rename
  * rtc
  * seek
  * sendfile
  * set
  * sigabrt
  * signal
  * stream
  * timer
  * utime
  * wait

To execute any of the benchmarks, execute:
```
	./stress-ng --<bench-name> X
```
where bench-name is the bench name you want to run and X is the number of threads the benchmark will spawn.


## Issues


It may happen during the side-channel phase execution that the terminal doesn't stop the benchmark execution on CTRL+C press.
To kill the process execute:
```
	pkill -9 stress-ng
```


## Extra


If you are interested in stress-ng, I advise you to visit the original Github repo
  * https://github.com/ColinIanKing/stress-ng
and to extend your read to README.stress-ng.md.

