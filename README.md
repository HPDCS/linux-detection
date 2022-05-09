# Side Channel Attack Detection for Linux

This repository keeps the code associated with the paper:

_S. Carnà, S. Ferracci, F. Quaglia, and A. Pellegrini_    
_“Fight Hardware with Hardware: System-wide Detection and Mitigation of Side-Channel Attacks using Performance Counters”_    
Digital Threats: Research and Practice, 2022.

If you find the code in this repository useful, or if you base
your work also partially on the content of this repository, we
kindly as you to cite the above article in your work.

The code is organized into three folders, that represent the
different components of the system used to run the experiments
provided in the paper. In particular:

* **linux-5.4.x**: This folder contains the modified kernel
  sources. They are aligned to Linux 5.4.72 LTS.
* **recode**: This is the userspace utility to drive the PMU
  management and detection activities of the system.
* **stress-ng**: This is a modified version of the stress-ng
  benchmark suite, where side channel attacks are embedded in
  some applications.

The content of this repository is freezed to the version used
in the above article, also for reproducibility purposes.
If you are interested with more updated version of the code,
the working repositories can be found at the following links:

* [linux](https://github.com/stefanocarna/linux-5.4.x)
* [recode](https://github.com/stefanocarna/recode)
* [stress-ng](https://github.com/stefanocarna/stress-ng/)

The folders above have additional README files detailing the
content of each project.




