#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUMMEMORY 65536 /* maximum number of data words in memory */
#define NUMREGS 8 /* number of machine registers */

#define ADD 0
#define NAND 1
#define LW 2
#define SW 3
#define BEQ 4
#define JALR 5 /* JALR – not implemented in this project */
#define HALT 6
#define NOOP 7

#define NOOPINSTRUCTION 0x1c00000

typedef struct IFIDstruct{
	int instr;
	int pcplus1;
} IFIDType;

typedef struct IDEXstruct{
	int instr;
	int pcplus1;
	int readregA;
	int readregB;
	int offset;
} IDEXType;

typedef struct EXMEMstruct{
	int instr;
	int branchtarget;
	int aluresult;
	int readreg;
} EXMEMType;

typedef struct MEMWBstruct{
	int instr;
	int writedata;
} MEMWBType;

typedef struct WBENDstruct{
	int instr;
	int writedata;
} WBENDType;

typedef struct statestruct{
	int pc;
	int instrmem[NUMMEMORY];
	int datamem[NUMMEMORY];
	int reg[NUMREGS];
	int numMemory;
	IFIDType IFID;
	IDEXType IDEX;
	EXMEMType EXMEM;
	MEMWBType MEMWB;
	WBENDType WBEND;
	int cycles;       /* Number of cycles run so far */
	int fetched;     /* Total number of instructions fetched */
	int retired;      /* Total number of completed instructions */
	int branches;  /* Total number of branches executed */
	int mispreds;  /* Number of branch mispredictions*/
} statetype;

int field0(int instruction){
	return( (instruction>>19) & 0x7);
}

int field1(int instruction){
	return( (instruction>>16) & 0x7);
}

int field2(int instruction){
	return(instruction & 0xFFFF);
}

int opcode(int instruction){
	return(instruction>>22);
}

int signExtend(int num){
	// convert a 16-bit number into a 32-bit integer
	if (num & (1<<15) ) {
		num -= (1<<16);
	}
	return num;
}

void printInstruction(int instr){
	char opcodeString[10];
	if (opcode(instr) == ADD) {
		strcpy(opcodeString, "add");
	} else if (opcode(instr) == NAND) {
		strcpy(opcodeString, "nand");
	} else if (opcode(instr) == LW) {
		strcpy(opcodeString, "lw");
	} else if (opcode(instr) == SW) {
		strcpy(opcodeString, "sw");
	} else if (opcode(instr) == BEQ) {
		strcpy(opcodeString, "beq");
	} else if (opcode(instr) == JALR) {
		strcpy(opcodeString, "jalr");
	} else if (opcode(instr) == HALT) {
		strcpy(opcodeString, "halt");
	} else if (opcode(instr) == NOOP) {
		strcpy(opcodeString, "noop");
	} else {
		strcpy(opcodeString, "data");
	}
	if(opcode(instr) == ADD || opcode(instr) == NAND){
		printf("%s %d %d %d\n", opcodeString, field2(instr), field0(instr), field1(instr));
	}
	else if(0 == strcmp(opcodeString, "data")){
		printf("%s %d\n", opcodeString, signExtend(field2(instr)));
	}
	else{
	printf("%s %d %d %d\n", opcodeString, field0(instr), field1(instr),
		signExtend(field2(instr)));
	}

}

void printstate(statetype* stateptr){
    int i;
    printf("\n@@@\nstate before cycle %d starts\n", stateptr->cycles);
    printf("\tpc %d\n", stateptr->pc);

    printf("\tdata memory:\n");
	for (i=0; i<stateptr->numMemory; i++) {
	    printf("\t\tdatamem[ %d ] %d\n", i, stateptr->datamem[i]);
	}
    printf("\tregisters:\n");
	for (i=0; i<NUMREGS; i++) {
	    printf("\t\treg[ %d ] %d\n", i, stateptr->reg[i]);
	}
    printf("\tIFID:\n");
	printf("\t\tinstruction ");
	printInstruction(stateptr->IFID.instr);
	printf("\t\tpcplus1 %d\n", stateptr->IFID.pcplus1);
    printf("\tIDEX:\n");
	printf("\t\tinstruction ");
	printInstruction(stateptr->IDEX.instr);
	printf("\t\tpcplus1 %d\n", stateptr->IDEX.pcplus1);
	printf("\t\treadregA %d\n", stateptr->IDEX.readregA);
	printf("\t\treadregB %d\n", stateptr->IDEX.readregB);
	printf("\t\toffset %d\n", stateptr->IDEX.offset);
    printf("\tEXMEM:\n");
	printf("\t\tinstruction ");
	printInstruction(stateptr->EXMEM.instr);
	printf("\t\tbranchtarget %d\n", stateptr->EXMEM.branchtarget);
	printf("\t\taluresult %d\n", stateptr->EXMEM.aluresult);
	printf("\t\treadreg %d\n", stateptr->EXMEM.readreg);
    printf("\tMEMWB:\n");
	printf("\t\tinstruction ");
	printInstruction(stateptr->MEMWB.instr);
	printf("\t\twritedata %d\n", stateptr->MEMWB.writedata);
    printf("\tWBEND:\n");
	printf("\t\tinstruction ");
	printInstruction(stateptr->WBEND.instr);
	printf("\t\twritedata %d\n", stateptr->WBEND.writedata);
}

void print_stats(statetype* state){
	printf("total of %d cycles executed\n", state->cycles);
	printf("total of %d instructions fetched\n", state->fetched);
	printf("total of %d instructions retired\n", state->retired);
	printf("total of %d branches executed\n", state->branches);
	printf("total of %d branch mispredictions\n", state->mispreds);
}

void run(statetype* state, statetype* newstate){

	int changeregEX = -1;
	int changeregMEM = -1;
	int changeregWB = -1;
	int regA;
	int regB;
	int offset;
	int stallID = -1;
	int sum;

	state->cycles = 0;
	//account for the initial noops by offsetting the initialization by 3
	state->fetched = -3;
	state->retired = -3;
	state->branches = 0;
	state->mispreds = 0;
	state->IFID.instr = NOOPINSTRUCTION;
	state->IDEX.instr = NOOPINSTRUCTION;
	state->EXMEM.instr = NOOPINSTRUCTION;
	state->MEMWB.instr = NOOPINSTRUCTION;
	state->WBEND.instr = NOOPINSTRUCTION;
	// Primary loop
	int i = 0;
	while(i<40){
		i = i + 1;
		printstate(state);
		/* check for halt */
		if(HALT == opcode(state->MEMWB.instr)) {
			printf("machine halted\n");
			print_stats(state);
			break;
		}
		*newstate = *state;
		newstate->cycles++;

		/*------------------ IF stage ----------------- */
		//fetch instruction
		newstate->IFID.instr = state->instrmem[state->pc];

		//increment pc
		newstate->pc = state->pc+1;
		newstate->IFID.pcplus1 = state->pc + 1;

		//increment instructions fetched
		if(stallID == -1){
			newstate->fetched = state->fetched + 1;
		}//else
		/*------------------ ID stage ----------------- */
		//get the instruction from the previous stage
		stallID = -1;
		newstate->IDEX.instr = state->IFID.instr;

		// get the pcplus1 from previous buffer
		newstate->IDEX.pcplus1 = state->IFID.pcplus1;
		//HAZARD checking for a load stall
		if(opcode(state->EXMEM.instr) == LW){
			if(opcode(state->IDEX.instr) == ADD || opcode(state->IDEX.instr) == NAND || opcode(state->IDEX.instr) == BEQ ){
				if(changeregEX == field1(state->IDEX.instr) || changeregEX == field0(state->IDEX.instr)){
					newstate->IFID.instr = state->IFID.instr;
					newstate->IFID.pcplus1 = state->IFID.pcplus1;
					newstate->IDEX.instr = state->IDEX.instr;
					newstate->IDEX.pcplus1 = state->IDEX.pcplus1;
					newstate->IDEX.readregA = state->IDEX.readregA;
					newstate->IDEX.readregB = state->IDEX.readregB;
					newstate->IDEX.offset = state->IDEX.offset;
					stallID = state->IDEX.instr;
					newstate->pc = state->IFID.pcplus1;
				}//if
			}//if
			if(opcode(state->IDEX.instr) == SW || opcode(state->IDEX.instr) == LW){
                	        if(changeregEX == field1(state->IDEX.instr)){
	                                newstate->IFID.instr = state->IFID.instr;
        	                        newstate->IFID.pcplus1 = state->IFID.pcplus1;
                	                newstate->IDEX.instr = state->IDEX.instr;
                        	        newstate->IDEX.pcplus1 = state->IDEX.pcplus1;
                   		        newstate->IDEX.readregA = state->IDEX.readregA;
                        	        newstate->IDEX.readregB = state->IDEX.readregB;
                                	newstate->IDEX.offset = state->IDEX.offset;
                   	 	        stallID = state->IDEX.instr;
                                	newstate->pc = state->IFID.pcplus1;
                        	}//if
                        }//if

		}//if


		// get the fields from the instruction
		newstate->IDEX.readregA = state->reg[field0(newstate->IDEX.instr)];
		newstate->IDEX.readregB = state->reg[field1(newstate->IDEX.instr)];
		newstate->IDEX.offset = signExtend(field2(newstate->IDEX.instr));



		/*------------------ EX stage ----------------- */

		// get the instruction from the previous stage
		if(stallID > 0){
			//insert noop for a load stall
			newstate->EXMEM.instr = NOOPINSTRUCTION;
		}//if
		else{
			newstate->EXMEM.instr = state->IDEX.instr;
		}//else
		newstate->EXMEM.readreg = state->IDEX.readregA;
		newstate->EXMEM.branchtarget = state->IDEX.pcplus1 + state->IDEX.offset;
		newstate->EXMEM.aluresult = 0;

		regA = state->IDEX.readregA;
		regB = state->IDEX.readregB;

		//hazard checking for dataforwarding
		if(opcode(newstate->EXMEM.instr) == ADD || opcode(newstate->EXMEM.instr) == NAND || opcode(newstate->EXMEM.instr) == BEQ || opcode(newstate->EXMEM.instr) == SW){
			if(changeregWB == field0(newstate->EXMEM.instr)){
				regA = state->WBEND.writedata;
			}//if
			if(changeregWB == field1(newstate->EXMEM.instr)){
				regB = state->WBEND.writedata;
			}//if
			if(changeregMEM == field0(newstate->EXMEM.instr)){
                                regA = state->MEMWB.writedata;
                        }//if
                        if(changeregMEM == field1(newstate->EXMEM.instr)){
                                regB = state->MEMWB.writedata;
                        }//if
			if(changeregEX == field0(newstate->EXMEM.instr)){
                                regA = state->EXMEM.aluresult;
                        }//if
                        if(changeregEX == field1(newstate->EXMEM.instr)){
                                regB = state->EXMEM.aluresult;
                        }//if
		}//if
		if(opcode(newstate->EXMEM.instr) == LW){
			if(changeregWB == field1(newstate->EXMEM.instr)){
                                regB = state->WBEND.writedata;
                        }//if
                        if(changeregMEM == field1(newstate->EXMEM.instr)){
                                regB = state->MEMWB.writedata;
                        }//if
                        if(changeregEX == field1(newstate->EXMEM.instr)){
                                regB = state->EXMEM.aluresult;
                        }//if
                }//if

		changeregEX = -1;
		changeregMEM = -1;
		changeregWB = -1;

		//add
		if(opcode(newstate->EXMEM.instr) == ADD){
			//add the contents of regA and regB
			newstate->EXMEM.aluresult = regA + regB;
			changeregEX = field2(newstate->EXMEM.instr);
		}//if

		//nand
		if(opcode(newstate->EXMEM.instr) == NAND){
			//nand the contents of regA and regB
			newstate->EXMEM.aluresult = ~(regA & regB);
			changeregEX = field2(newstate->EXMEM.instr);
		}//if

		//lw and sw
		if(opcode(newstate->EXMEM.instr) == SW){
			//add the contents of regB and offset for the memory address
			newstate->EXMEM.aluresult = regB + state->IDEX.offset;
                }//if
		if(opcode(newstate->EXMEM.instr) == LW){
			//add the contents of regB and offset for the memory address
			newstate->EXMEM.aluresult = regB + state->IDEX.offset;
			changeregEX = field0(newstate->EXMEM.instr);
		}//if

		//beq
		if(opcode(newstate->EXMEM.instr) == BEQ){
			newstate->EXMEM.aluresult = regA - regB;
		 }//if

		//noop
		if(opcode(newstate->EXMEM.instr) == HALT){
			//nothing for now
          	}//if

		/*------------------ MEM stage ----------------- */
		//get instruction from previous buffer
                newstate->MEMWB.instr = state->EXMEM.instr;

		//store the aluresult from EXMEM for R-type instructions
		newstate->MEMWB.writedata = state->EXMEM.aluresult;

		//beq
		if(opcode(newstate->MEMWB.instr) == BEQ){
			newstate->branches = state->branches + 1;
			//branch if aluresult is 0
			if(state->EXMEM.aluresult == 0){
				newstate->pc = state->EXMEM.branchtarget;
				newstate->mispreds = state->mispreds + 1;
				newstate->IFID.instr = NOOPINSTRUCTION;
				newstate->IDEX.instr = NOOPINSTRUCTION;
				newstate->EXMEM.instr = NOOPINSTRUCTION;
			}//if
		}//if

                //load the data from memory into the writedata
                if(opcode(newstate->MEMWB.instr) == LW){
                        newstate->MEMWB.writedata = state->datamem[state->EXMEM.aluresult];
			changeregMEM = field0(newstate->MEMWB.instr);
                }//if
		if(opcode(newstate->MEMWB.instr) == SW){
			newstate->datamem[state->EXMEM.aluresult] = state->EXMEM.readreg;
		}//if
		if(opcode(newstate->MEMWB.instr) == ADD || opcode(newstate->MEMWB.instr) == NAND){
			changeregMEM = field2(newstate->MEMWB.instr);
		}//if

		/*------------------ WB stage ----------------- */
		//get instruction and writedata from previous buffer
		newstate->WBEND.instr = state->MEMWB.instr;
		newstate->WBEND.writedata = state->MEMWB.writedata;

		//write back to appropriate destRegs
		if(opcode(newstate->WBEND.instr) == ADD || opcode(newstate->WBEND.instr) == NAND){
			newstate->reg[field2(newstate->WBEND.instr)] = state->MEMWB.writedata;
			changeregWB = field2(newstate->WBEND.instr);
		}//if
		if(opcode(newstate->WBEND.instr) == LW){
			newstate->reg[field0(newstate->WBEND.instr)] = state->MEMWB.writedata;
			changeregWB = field0(newstate->WBEND.instr);
		}//if

		//calculate retired instructions
		if(stallID != -1){
			newstate->retired = state->fetched - state->mispreds*3;
		}//if
		else{
			newstate->retired = state->fetched - state->mispreds*3 + 1;
		}//else
		*state = *newstate; 	/* this is the last statement before the end of the loop.
					It marks the end of the cycle and updates the current
					state with the values calculated in this cycle
					– AKA “Clock Tick”. */
	}

}

int main(int argc, char** argv){

	/** Get command line arguments **/
	char* fname;

	opterr = 0;

	int cin = 0;

	while((cin = getopt(argc, argv, "i:")) != -1){
		switch(cin)
		{
			case 'i':
				fname=(char*)malloc(strlen(optarg));
				fname[0] = '\0';

				strncpy(fname, optarg, strlen(optarg)+1);
				break;
			case '?':
				if(optopt == 'i'){
					printf("Option -%c requires an argument.\n", optopt);
				}
				else if(isprint(optopt)){
					printf("Unknown option `-%c'.\n", optopt);
				}
				else{
					printf("Unknown option character `\\x%x'.\n", optopt);
					return 1;
				}
				break;
			default:
				abort();
		}
	}

	FILE *fp = fopen(fname, "r");
	if (fp == NULL) {
		printf("Cannot open file '%s' : %s\n", fname, strerror(errno));
		return -1;
	}

	/* count the number of lines by counting newline characters */
	int line_count = 0;
	int c;
	while (EOF != (c=getc(fp))) {
		if ( c == '\n' ){
			line_count++;
		}
	}
	// reset fp to the beginning of the file
	rewind(fp);

	statetype* state = (statetype*)malloc(sizeof(statetype));
	statetype* newstate = (statetype*)malloc(sizeof(statetype));

	//initialize the pc, memory, and registers to 0
	state->pc = 0;
	memset(state->instrmem, 0, NUMMEMORY*sizeof(int));
	memset(state->datamem, 0, NUMMEMORY*sizeof(int));
	memset(state->reg, 0, NUMREGS*sizeof(int));

	state->numMemory = line_count;

	char line[256];

	int i = 0;
	while (fgets(line, sizeof(line), fp)) {
		/* note that fgets doesn't strip the terminating \n, checking its
		   presence would allow to handle lines longer that sizeof(line) */
		state->instrmem[i] = atoi(line);
		state->datamem[i] = atoi(line);
		i++;
	}
	fclose(fp);

	/** Run the simulation **/
	run(state, newstate);

	free(state);
	free(fname);

}
