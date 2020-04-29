
Logic in Memory Emulator (LiME) is a hardware/software tool specially designed for memory system evaluation and experiment. Emerging memories display a wide range of bandwidths, latencies, and capacities, making it challenging for the computer architect to navigate the design space of potential memory configurations, and for the application developer to assess performance implications of using such memories. With the LiME framework, architectural ideas can be prototyped in great detail yet with sufficient performance to support realistic evaluation on long running applications. LiME consists of two fundamental components: 1) the hardware and OS infrastructure for the emulator, and 2) a suite of benchmark applications to assist in characterizing the performance of current and future computer architectures. Some of the applications have been collected from other open source projects.

Uses:
Logging, replay and analysis of an application's memory behavior
Evaluate impact of emerging memory technology on application performance
Emulate complex memory interactions in whole applications orders of magnitude faster than software simulation
Emulate acceleration hardware co-located with the memory subsystem

Features:
Capture and log external memory accesses to a separate off-chip memory device without affecting application execution
Memory traces include the address, length, timestamp, and optionally the data for each transaction
Captured trace data can be saved to an SD card for off-line analysis
Configure a wide range of memory latencies in sub-nanosecond increments that encompass high-bandwidth and storage class memories
Specify regions of interest (ROI) in applications to reduce the amount of trace data captured for analysis
Currently supports execution on Xilinx Zynq SoC which integrates an ARM processor with FPGA logic on a single device
Applications can be run under Linux or in bare metal mode on the ARM cores

Project:
The project consists of hardware and software source code contained within two repositories. One (lime) is more specific to the hardware and operating system infrastructure for the emulator, and the other (lime-apps) contains a suite of benchmark applications. The lime repository also contains a few test programs useful in validating basic features of the emulator.

References:
[1] A. Jain, S. Lloyd, M. Gokhale, "Microscope on Memory: MPSoC-enabled Computer Memory System Assessments," Proceedings of FCCM, IEEE, 173-180, May 2018. doi:10.1109/FCCM.2018.00035

Dependencies:
Building the emulator requires an FPGA development license from Xilinx.
http://www.xilinx.com

Access to the Internet is needed to clone public repositories when building the Linux kernel and U-Boot.

Compilation of applications from the lime-apps repository require access to common files from the lime repository. The directory location where lime is installed can be specified through the LIME environment variable.

Some of the applications are dependent on the boost library (bfs, pager).
http://www.boost.org/

Supported FPGA Boards:
  Xilinx ZCU102
  Fidus Sidewinder
  Xilinx ZC706 (partial)
