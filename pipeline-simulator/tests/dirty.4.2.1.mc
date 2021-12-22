 lw 1 0 1 #first this should  grab the  block of instructions of 0-4 then grab 0-3 execute those loads
 lw 1 0 2 #then grab the next set of instructions do the sw which changes the dirty bit 
 lw 1 0 3 #then does the write back 
 lw 1 0 4
 sw 1 0 4
 lw 1 0 45
 lw 1 0 17
 lw 1 0 18
 halt
