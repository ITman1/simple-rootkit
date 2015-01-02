Simple Rootkit
==============

Simple rootkit that opens TCP port for listening and hides itself from the process list (eg. from top, ps and others). Hiding is accomplished via two different ways. For both of them is used of the kernel module. First way modifies getdents system call. The second way cleans virtual file /proc/$PID/cmdline inside memory and that causes that process is not visible anymore.

# Usage
```
make               compile project - release version
make run           runs rootkit and inserts kernel module which hides this process
make stop          stops rootkit and tries to unload kernel module
make insert-module inserts kernel module which hides this process
make run-rootkit   runs rootkit only
make pack          packs all required files to compile this project    
make clean         clean temp compilers files    
make clean-all     clean all compilers files - includes project    
make clean-outp    clean output project files
```

## Contact and credits
                             
**Author:**    Radim Loskot  
**gmail.com:** radim.loskot (e-mail)
