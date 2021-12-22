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
#define JALR 5
#define HALT 6
#define NOOP 7

#define NOOPINSTRUCTION 0x1c00000

typedef struct stateStruct {
	int pc;
	int mem[NUMMEMORY];
	int reg[NUMREGS];
	int numMemory;
	int hits;
	int misses;
} stateType;

typedef struct blockStruct {
	int valid;
	int dirty;
	int tag;
	int lru;
	int* blockData;
} blockType;

/*
* Log the specifics of each cache action.
*
* address is the starting word address of the range of data being transferred.
* size is the size of the range of data being transferred.
* type specifies the source and destination of the data being transferred.
*
* cache_to_processor: reading data from the cache to the processor
* processor_to_cache: writing data from the processor to the cache
* memory_to_cache: reading data from the memory to the cache
* cache_to_memory: evicting cache data by writing it to the memory
* cache_to_nowhere: evicting cache data by throwing it away
*/
enum action_type {cache_to_processor, processor_to_cache, memory_to_cache, cache_to_memory, cache_to_nowhere};

void print_action(int address, int size, enum action_type type)
{
	printf("transferring word [%i-%i] ", address, address + size - 1);
	if (type == cache_to_processor) {
		printf("from the cache to the processor\n");
	} else if (type == processor_to_cache) {
		printf("from the processor to the cache\n");
	} else if (type == memory_to_cache) {
		printf("from the memory to the cache\n");
	} else if (type == cache_to_memory) {
		printf("from the cache to the memory\n");
	} else if (type == cache_to_nowhere) {
		printf("from the cache to nowhere\n");
	}
}

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

	printf("%s %d %d %d\n", opcodeString, field0(instr), field1(instr),
			field2(instr));
}

void printState(stateType *statePtr){
	int i;
	printf("\n@@@\nstate:\n");
	printf("\tpc %d\n", statePtr->pc);
	printf("\tmemory:\n");
	for(i = 0; i < statePtr->numMemory; i++){
		printf("\t\tmem[%d]=%d\n", i, statePtr->mem[i]);
	}
	printf("\tregisters:\n");
	for(i = 0; i < NUMREGS; i++){
		printf("\t\treg[%d]=%d\n", i, statePtr->reg[i]);
	}
	printf("end state\n");
}

int signExtend(int num){
	// convert a 16-bit number into a 32-bit integer
	if (num & (1<<15) ) {
		num -= (1<<16);
	}
	return num;
}

void print_stats(stateType* state){
        printf("Hits: %d\n", state->hits);
        printf("Misses: %d\n", state->misses);
}

int logbasetwo(int num){
	int count = 0;
	while(num != 1){
		num = num >> 1;
		count++;
	}//while
	return count;
}//log

void run(stateType* state, int blocksize, int associativity, int sets){

	// Reused variables;
	int instr = 0;
	int regA = 0;
	int regB = 0;
	int offset = 0;
	int branchTarget = 0;
	int aluResult = 0;
	int total_instrs = 0;
	// Reused cache variables;
	int blockoffset;
	int lineoffset;
	int tag;
	int hit;
	int lrumax;
	enum action_type type;

	int size = blocksize * associativity * sets;
	// Creating the cache
	blockType** cache = (blockType**)malloc(sets*sizeof(blockType*));
	for( int i = 0; i< sets; i++){
		cache[i] = (blockType*)malloc(associativity * sizeof(blockType));
		for(int j = 0; j<associativity; j++){
			cache[i][j].valid = 0;
			cache[i][j].dirty = 0;
			cache[i][j].tag = 0;
			cache[i][j].lru = associativity-1;
			cache[i][j].blockData = malloc(blocksize*sizeof(int));
		}//for
	}//for

	// Primary loop
	while(1){
		total_instrs++;
		//printState(state);

		// Cache prep
		blockoffset = state->pc % blocksize;
		lineoffset = (state->pc >> logbasetwo(blocksize)) % sets;
		tag = state->pc >> (logbasetwo(blocksize) + logbasetwo(sets));

		// Instruction Fetch
		hit = 0;
		for( int i = 0; i<associativity; i++){
			//run through the associated blocks and see if they are valid and match the tag for a hit
			//if so, mark as a hit, update the instruction, update the lrumax, change the lru of the block to be most recently used, and print the action
			if(cache[lineoffset][i].valid == 1 && cache[lineoffset][i].tag == tag){
				hit = 1;
				instr = cache[lineoffset][i].blockData[blockoffset];
				lrumax = cache[lineoffset][i].lru;
				cache[lineoffset][i].lru = -1;
				type = cache_to_processor;
				print_action( (blockoffset+lineoffset+tag), 1, type);
				state->hits = state->hits + 1;
				break;
			}//if
		}//for
		//on a miss
		if( hit == 0 ){
			//first, figure out where to evict/read the memory needed
			int evictee = 0;
			//look for the first invalid entry or find the least recently used block
			for(int i = 0; i<associativity; i++){
				if(cache[lineoffset][i].valid == 0){
					evictee = i;
					break;
				}//if
				else if(cache[lineoffset][i].lru > cache[lineoffset][evictee].lru){
					evictee = i;
				}//if
			}//for
			//if dirty, write back
			if(cache[lineoffset][evictee].dirty == 1 && cache[lineoffset][evictee].valid == 1){
				for(int i = 0; i<blocksize; i++){
					state->mem[(((blockoffset+lineoffset+tag)/blocksize)*blocksize) + i] = cache[lineoffset][evictee].blockData[i];
				}//for
				cache[lineoffset][evictee].dirty = 0;
				type = cache_to_memory;
				print_action( (((blockoffset+lineoffset+tag)/blocksize)*blocksize), blocksize, type);
			}//if
			else if(cache[lineoffset][evictee].valid == 1){
				type = cache_to_nowhere;
				print_action( (((blockoffset+lineoffset+tag)/blocksize)*blocksize), blocksize, type);
			}//else

			//read into the cache
			for(int i = 0; i<blocksize; i++){
				cache[lineoffset][evictee].blockData[i] = state->mem[((state->pc/blocksize) * blocksize) + i];
			}//for
			type = memory_to_cache;
			print_action( (((blockoffset+lineoffset+tag)/blocksize)*blocksize), blocksize, type);

			//change valid bit and instr, as well as print action, set lrumax, update misses counter
			cache[lineoffset][evictee].valid = 1;
			instr = cache[lineoffset][evictee].blockData[blockoffset];
			type = cache_to_processor;
                        print_action( (blockoffset+lineoffset+tag), 1, type);
			lrumax = cache[lineoffset][evictee].lru;
			cache[lineoffset][evictee].lru = -1;
			state->misses = state->misses + 1;
		}//if
		// update lru
		for(int i = 0; i<associativity; i++){
			//if the lru is less than the block used, increment it by one
			if( cache[lineoffset][i].lru < lrumax ){
				cache[lineoffset][i].lru = cache[lineoffset][i].lru + 1;
			}//if
		}//for

		/* check for halt */
		if (opcode(instr) == HALT) {
			printf("machine halted\n");
			for(int i = 0; i<sets; i++){
				for(int j = 0; j<associativity; j++){
					//write back dirty entries
					 if(cache[i][j].dirty == 1 && cache[i][j].valid == 1){
						for(int k = 0; k<blocksize; k++){
							state->mem[(((blockoffset+lineoffset+tag)/blocksize)*blocksize) + k] = cache[i][j].blockData[k];
						}//for
					}//if
					//make entry invalid
					cache[i][j].valid = 0;
				}//for
			}//for
			break;
		}

		// Increment the PC
		state->pc = state->pc+1;

		// Set reg A and B
		regA = state->reg[field0(instr)];
		regB = state->reg[field1(instr)];

		// Set sign extended offset
		offset = signExtend(field2(instr));

		// Branch target gets set regardless of instruction
		branchTarget = state->pc + offset;

		/**
		 *
		 * Action depends on instruction
		 *
		 **/
		// ADD
		if(opcode(instr) == ADD){
			// Add
			aluResult = regA + regB;
			// Save result
			state->reg[field2(instr)] = aluResult;
		}
		// NAND
		else if(opcode(instr) == NAND){
			// NAND
			aluResult = ~(regA & regB);
			// Save result
			state->reg[field2(instr)] = aluResult;
		}
		// LW or SW
		else if(opcode(instr) == LW || opcode(instr) == SW){
			// Calculate memory address
			aluResult = regB + offset;
			if(opcode(instr) == LW){
				// Load

				// Cache prep
				blockoffset = aluResult % blocksize;
				lineoffset = (aluResult >> logbasetwo(blocksize)) % sets;
				tag = aluResult >> (logbasetwo(blocksize) + logbasetwo(sets));

				//run through the associated blocks to find a hit or miss
				hit = 0;
				for( int i = 0; i<associativity; i++){
					//if the block is valid and tags match, its a hit
					if(cache[lineoffset][i].valid == 1 && cache[lineoffset][i].tag == tag){
						hit = 1;
						state->reg[field0(instr)] = cache[lineoffset][i].blockData[blockoffset];
						type = cache_to_processor;
						print_action( (aluResult), 1, type);
						lrumax = cache[lineoffset][i].lru;
						cache[lineoffset][i].lru = -1;
						state->hits = state->hits + 1;
						break;
					}//if
				}//for
				//miss
				if( hit == 0 ){
					//find the LRU or invalid entry to be evicted/read into
					int evictee = 0;
					for(int i = 0; i<associativity; i++){
						if(cache[lineoffset][i].valid == 0){
							evictee = i;
							break;
						}//if
						else if(cache[lineoffset][i].lru > cache[lineoffset][evictee].lru){
							evictee = i;
						}//if
					}//for
					//if dirty, write back
					if(cache[lineoffset][evictee].dirty == 1 && cache[lineoffset][evictee].valid == 1){
						for(int i = 0; i<blocksize; i++){
							state->mem[((aluResult/blocksize)*blocksize) + i] = cache[lineoffset][evictee].blockData[i];
						}//for
						cache[lineoffset][evictee].dirty = 0;
						type = cache_to_memory;
                                		print_action( ((aluResult/blocksize)*blocksize), blocksize, type);
					}//if
					else if(cache[lineoffset][evictee].valid == 1){
						type = cache_to_nowhere;
                                		print_action( (((aluResult)/blocksize)*blocksize), blocksize, type);
					}//else

					//read into the cache
					for(int i = 0; i<blocksize; i++){
						cache[lineoffset][evictee].blockData[i] = state->mem[(aluResult/blocksize)*blocksize + i];
					}//for
					type = memory_to_cache;
                                	print_action( (((aluResult)/blocksize)*blocksize), blocksize, type);
					//change valid bit, process LW, print action, update lrumax, update block lru to me most recently used, misses counter
					cache[lineoffset][evictee].valid = 1;
					state->reg[field0(instr)] = cache[lineoffset][evictee].blockData[blockoffset];
					type = cache_to_processor;
                                	print_action( aluResult, 1, type);
					lrumax = cache[lineoffset][evictee].lru;
					cache[lineoffset][evictee].lru = -1;
					state->misses = state->misses + 1;
				}//if

				// update lru
				for(int i = 0; i<associativity; i++){
					//if the lru is less than the used, increment it by one
					if( cache[lineoffset][i].lru < lrumax ){
						cache[lineoffset][i].lru = cache[lineoffset][i].lru + 1;
					}//if
				}//for
			}else if(opcode(instr) == SW){
				// Store

				// Cache prep
				blockoffset = aluResult % blocksize;
				lineoffset = (aluResult >> logbasetwo(blocksize)) % sets;
				tag = aluResult >> (logbasetwo(blocksize) + logbasetwo(sets));

				// Check the cache for a hit or miss
				// On a hit, update the block in the cache with the stored value and change it to dirty
				hit = 0;
				for( int i = 0; i<associativity; i++){
					if(cache[lineoffset][i].valid == 1 && cache[lineoffset][i].tag == tag){
						hit = 1;
						cache[lineoffset][i].blockData[blockoffset] = state->reg[field0(instr)];
                                        	cache[lineoffset][i].dirty = 1;
						type = processor_to_cache;
						print_action( (blockoffset+lineoffset+tag), 1, type);
						lrumax = cache[lineoffset][i].lru;
						cache[lineoffset][i].lru = -1;
						state->hits = state->hits + 1;
						break;
					}//if
				}//for

				//On a miss, check to see if there is space to read in the associated block
				//If not, figure out which block to evict using the lru and write that back to memory if it is dirty
				//Read in the block and update the associated block and change the it to be dirty
				if( hit == 0 ){
					int evictee = 0;
					for(int i = 0; i<associativity; i++){
						if(cache[lineoffset][i].valid == 0){
							evictee = i;
							break;
						}//if
						else if(cache[lineoffset][i].lru > cache[lineoffset][evictee].lru){
							evictee = i;
						}//if
					}//for
					//if dirty, write back
					if(cache[lineoffset][evictee].dirty == 1 && cache[lineoffset][evictee].valid == 1){
						for(int i = 0; i<blocksize; i++){
							state->mem[((aluResult)*blocksize) + i] = cache[lineoffset][evictee].blockData[i];
						}//for
						type = cache_to_memory;
						print_action((((aluResult)/blocksize)*blocksize), blocksize, type);
						cache[lineoffset][evictee].dirty = 0;
					}//if
					else if(cache[lineoffset][evictee].valid == 1){
							type = cache_to_nowhere;
							print_action((((aluResult)/blocksize)*blocksize), blocksize, type);
						}//if
					//read into the cache
					for(int i = 0; i<blocksize; i++){
						cache[lineoffset][evictee].blockData[i] = state->mem[((aluResult/blocksize) * blocksize) + i];
					}//for
					type = memory_to_cache;
					print_action((((aluResult)/blocksize)*blocksize), blocksize, type);

					//change valid bit, change dirty, update blockData, process SW, update lrumax and block lru, print action
					cache[lineoffset][evictee].valid = 1;
					cache[lineoffset][evictee].blockData[blockoffset] = state->reg[field0(instr)];
					type = processor_to_cache;
					print_action(aluResult, 1, type);
					cache[lineoffset][evictee].dirty = 1;
					lrumax = cache[lineoffset][evictee].lru;
					cache[lineoffset][evictee].lru = -1;
					state->misses = state->misses + 1;
				}//if

				// update lru
				for(int i = 0; i<associativity; i++){
					//if the lru is less than the used, increment it by one
					if( cache[lineoffset][i].lru < lrumax ){
						cache[lineoffset][i].lru = cache[lineoffset][i].lru + 1;
					}//if
				}//for
			}
		}
		// JALR
		else if(opcode(instr) == JALR){
			// rA != rB for JALR to work
			// Save pc+1 in regA
			state->reg[field0(instr)] = state->pc;
			//Jump to the address in regB;
			state->pc = state->reg[field1(instr)];
		}
		// BEQ
		else if(opcode(instr) == BEQ){
			// Calculate condition
			aluResult = (regA - regB);

			// ZD
			if(aluResult==0){
				// branch
				state->pc = branchTarget;
			}
		}	
	} // While
	print_stats(state);
}


int main(int argc, char** argv){

	/** Get command line arguments **/
	char* fname;

	opterr = 0;

	int cin = 0;
	int blocksize;
	int sets;
	int associativity;


	while((cin = getopt(argc, argv, "f:b:s:a:")) != -1){
		switch(cin)
		{
			case 'f':
				fname=(char*)malloc(strlen(optarg));
				fname[0] = '\0';

				strncpy(fname, optarg, strlen(optarg)+1);
				printf("FILE: %s\n", fname);
				break;
			case 'b':
				blocksize = atoi(optarg);
				break;
			case 's':
				sets = atoi(optarg);
				break;
			case 'a':
				associativity = atoi(optarg);
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

	stateType* state = (stateType*)malloc(sizeof(stateType));

	state->pc = 0;
	memset(state->mem, 0, NUMMEMORY*sizeof(int));
	memset(state->reg, 0, NUMREGS*sizeof(int));

	state->numMemory = line_count;

	char line[256];

	int i = 0;
	while (fgets(line, sizeof(line), fp)) {
		/* note that fgets doesn't strip the terminating \n, checking its
		   presence would allow to handle lines longer that sizeof(line) */
		state->mem[i] = atoi(line);
		i++;
	}
	fclose(fp);
	/** Run the simulation **/
	run(state, blocksize, associativity, sets);

	free(state);
	free(fname);

}

