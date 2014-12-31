Simple Rootkit
==============

Simple rootkit that opens TCP port for listening and hides itself from the process list

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
