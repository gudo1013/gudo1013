
README:
Hello, you can use "make" to create the executable simulator
loadstall.asm and .mc test if we do a load stall for a lw correctly
lotsoadd.asm and .mc test if we data forward multiple times correctly
mixed.asm and .mc test if all the instructions (other than jarl) work correctly
once you’re all done with these you can use "make clean" to eliminate the executable

Description:

Simulate a pipelining CPU that accounts for various hazards implementing the UST-3400 Assembly Language. Instructions are loaded in sequentially with port-forwarding and noop injections when needed based on memory accesses and instructions. A total of 5 stages for each instruction.
