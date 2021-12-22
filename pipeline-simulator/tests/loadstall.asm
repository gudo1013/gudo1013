 lw 1 0 hundi //load stall test
 lw 2 0 two
 add 3 1 2 //should do a load stall
 add 4 3 1 //should forward 	
 halt
two .fill 2
hundi .fill 100