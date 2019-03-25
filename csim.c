// Ryan Resma rmr3429

// libraries included for implementation of this cache
#include "cachelab.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <stdint.h>
#include <ctype.h>

// error identifiers
#define MISSING_FLAG 1
#define MISSING_ARGUMENT 2
#define MISSING_FILENAME 3
#define FILE_NAME_NOT_FOUND 4

// flags
#define FLAGS_h 0
#define FLAGS_v 1
#define FLAGS_s 2
#define FLAGS_E 3
#define FLAGS_b 4
#define FLAGS_t 5
#define FLAGS_COUNT 6

// size identifiers
#define VALID_BIT_SIZE 1
#define POINTER_SIZE 8
#define MEMORY_ADDRESS_SIZE 64

// bases used
#define HEXADECIMAL 16
#define DECIMAL 10

// types of cache accesses
#define MISS_EMPTY 0
#define MISS_FULL 1
#define HIT 2

// INTEGER.MAX_VALUE
#define MAX_INT 2147483647

// Cache struct, contains sets
typedef struct Cache {
	struct Set **sets; // arr of sets in cache
	int numSets;
	int clock;
} Cache;

typedef struct Set {
	struct Line **lines; // arr of lines in set
	int numLines;
} Set;

typedef struct Line {
	char *full; // full line
	bool valid;
	int tag;
	int totSize;
	int tagSize;
	int blockSize;
	int lastTimeUsed; // used for replacement policy
} Line;

void formatHelp(char *op);

void initiateFlags(bool flags[], int argc, char *argv[]);
bool collectInput(int *s, int *E, int *b, int argc, char *argv[]);
bool collectFileName(char **fileName, int argc, char *argv[]);

void makeCache(Cache *cache, int s, int E, int b);
void flushCache(Cache *cache);
void parseFile(Cache *cache, char *fileName, bool vbool, int *hits, int *misses, int *evictions);
int useCache(Cache *cache, long address);

void cutLeadingAddressZeros(char str[]);
void trimEnd(char str[]);
void getBin(int64_t num, char *str);
void reverseStr(char *p);

int main(int argc, char *argv[])
{
    int STATUS = 0;
  	int hits = 0, misses = 0, evictions = 0;
  	int s = -1, E = -1, b = -1; // error checking to make sure set
  	char *fileName;
  	// Makes sure arguments exist in order: h, v, s, E, b, t
  	bool flags[FLAGS_COUNT] = {false, false, false, false, false, false};

  	initiateFlags(flags, argc, argv);
  	if(flags[FLAGS_h]) { // help screen is called
    	formatHelp(argv[0]);
    	return 0;
  	} else if(!flags[FLAGS_s] || !flags[FLAGS_E] || !flags[FLAGS_b] || !flags[FLAGS_t]
          || !collectInput(&s, &E, &b, argc, argv)) { // missing flags or missing vals
    	fprintf(stderr, "%s: Error: missing command-line arg\n", argv[0]);
   	 	formatHelp(argv[0]);
    	STATUS = MISSING_FLAG;
  	} else if(!collectFileName(&fileName, argc, argv)) { // if no file is specified
    	fprintf(stderr, "%s: Error: flag needs argument -- 't'\n", argv[0]);
    	formatHelp(argv[0]);
    	STATUS = MISSING_FILENAME;
  	} else if(access(fileName, R_OK) == -1) {
    	fprintf(stderr, "%s: Error: No such file or directory", fileName);
    	STATUS = FILE_NAME_NOT_FOUND;
  	} else { // everything is ok, simulate
    	// create cache
    	Cache *cache = malloc(sizeof(Cache));
    	makeCache(cache, s, E, b);
    	// simulate cache
    	parseFile(cache, fileName, flags[FLAGS_v], &hits, &misses, &evictions);
    	// print results
    	printSummary(hits, misses, evictions);
    	// 
    	flushCache(cache);
  	}
  	return STATUS;
}

// helper function to set boolean flags
void initiateFlags(bool flags[], int argc, char *argv[]) {
	int idx; // counter
	char temp; // temporary flag
	// parse through the arguments
	for (idx = 0; idx < argc; idx++) {
		// check if a given character flag is found
		if (argv[idx][0] == '-' && argv[idx][2] == '\0') {
			temp = argv[idx][1]; // assign flag
			// assign boolean values to flags arr
			if (temp == 'h') {
				flags[FLAGS_h] = true;
			} else if (temp == 'v') {
				flags[FLAGS_v] = true;
			} else if (temp == 's') {
				flags[FLAGS_s] = true;
			} else if (temp == 'E') {
				flags[FLAGS_E] = true;
			} else if (temp == 'b') {
				flags[FLAGS_b] = true;
			} else if (temp == 't') {
				flags[FLAGS_t] = true;
			}
		}
	}
}

// helper function to return true or false based on successful v unsuccessful
// input data collection from the command line
bool collectInput(int *s, int *E, int *b, int argc, char *argv[]) {
	int idx; // counter
	char temp; // temporary flag
	char **eof = '\0'; // strtol arg, used to be next char after numerical value
	// parse the command line but not over last arg (cant be flag)
	for (idx = 1; idx < argc -1; idx ++) {
		// check if a given character flag is found
		if (argv[idx][0] == '-' && argv[idx][2] == '\0') {
			temp = argv[idx][1]; // assign flag
			if (temp == 's') {
				*s = strtol(argv[idx + 1], eof, DECIMAL); // convert string to long
				if (*s <= 0) { // check if 0 or negative long, if so, NaN or invalid
					return false; // break out of loop
				}
			} else if (temp == 'E') {
				*E = strtol(argv[idx + 1], eof, DECIMAL); // convert string to long
				if (*E <= 0) { // check if 0 or negative long, if so, NaN or invalid
					return false; // break out of loop
				}
			} else if (temp == 'b') {
				*b = strtol(argv[idx + 1], eof, DECIMAL); // convert string to long
				if (*b <= 0) { // check if 0 or negative long, if so, NaN or invalid
					return false; // break out of loop
				}
			}
		}
	}
	return true; // successful input collectiokn
}

bool collectFileName(char **fileName, int argc, char *argv[]) {
	int idx; // arg counter
	char temp; // temporary flag
	// parse the command line, searching for the -t flag to signal for trace file name
	// skip over last arg for it cannot be a flag
	for (idx = 1; idx < argc - 1; idx++) {
		// check if a given character flag is found
		if (argv[idx][0] == '-' && argv[idx][2] == '\0') {
			temp = argv[idx][1]; // assign flag
			// -t flag signals trace file name
			if (temp == 't') {
				*fileName = argv[idx + 1]; // assign fileName char array to the next argument
				return true; // break out, successful in finding loop
			}
		}
	}
	return false; // unsuccessful in finding file name
}

// simulation of cache
void parseFile(Cache *cache, char *fileName, bool vbool, int *hits, int *misses, int *evictions) {
	int state; // state of cache
	char **endptr = '\0'; // endptr arg needed for strtol
	int64_t address; // address being read
	FILE *file; // file being read
	char buff[100]; // number of characters being read in at a time is 100
	char verbose[20]; // verbose mode
	char temp; // temporary var
	bool toPrint; // boolean to print

	file = fopen(fileName, "r"); // open the file passed in read mode
	// parse entire file until eof char is reached
	while (1) { 
		fgets(buff, 100, file); // read in 100 characters
		if (feof(file)) {
			break;
		}
		toPrint = false;
		// check if first character is space, for all our mem accesses starts w/ [space] *type of access*
		if (buff[0] == ' ') {
			toPrint = true; // valid print
			address = strtol(&buff[3], endptr, 16); // address starts at the 4th space
			temp = buff[1]; // type of memory access at 2nd space
			// same operations for both store and load
			if (temp == 'S' || temp == 'L') {
				state = useCache(cache, address); // use the cache
				if(state == MISS_EMPTY) {
          			(*misses)++;
          			strcpy(verbose, "miss");
       	 		} else if(state == MISS_FULL) {
          			(*misses)++;
          			(*evictions)++;
          			strcpy(verbose, "miss eviction");
        		} else {
          			(*hits)++;
          			strcpy(verbose, "hit");
        		}
			} else if (temp == 'M') { // data modify
				state = useCache(cache, address);
				if(state == MISS_EMPTY) {
          			(*misses)++;
          			(*hits)++;
          			strcpy(verbose, "miss hit");
        		} else if(state == MISS_FULL) {
          			(*misses)++;
          			(*evictions)++;
          			(*hits)++;
          			strcpy(verbose, "miss eviction hit");
        		} else {
          			(*hits) += 2;
          			strcpy(verbose, "hit hit");
        		}
			}
		}
		if(vbool && toPrint) {
      		cutLeadingAddressZeros(buff);
      		trimEnd(buff);
      		trimEnd(verbose);
      		printf("%s %s \n", &buff[1], verbose); // 1 is to get rid of the starting space that's always before a L, S, or M instruction in trace files
    	}
	}
}

// function to access the cache
int useCache(Cache *cache, int64_t address) {
  	int tagBits, indexbits, i, numOfLines, LRUIndex, LRUTime; // local vars
  	long tag, index; // vars to store the tag and the index
  	char binary[66]; // addresses are 64 bits long + 1 EOL char
  	char tagS[64];
  	char indexS[64];
  	char **endptr = '\0'; // eof character needed for strtol arg

  	Set *set; 
  	Line **lines, *line;

  	bool empty = false; // cache is not empty right now
  	int status = -1; // status to be returned at the end of the method

  	LRUIndex = 0; // vars for Least Recently Used replacement policy
  	LRUTime = MAX_INT;
  
  	tagBits = cache->sets[0]->lines[0]->tagSize; // init tag bits
  	indexbits = cache->numSets; // init index bits
  
  	indexbits = (int) log2(indexbits);

  	getBin(address, binary);
  	binary[65] = '\0';
  	reverseStr(binary);
  	binary[64] = '\0';
  	strncpy(tagS, &binary[0], tagBits);
  	tagS[tagBits] = '\0';
  	strncpy(indexS, &binary[tagBits], indexbits); // starts after tagBits
  	indexS[indexbits] = '\0';
  
  	tag = strtol(tagS, endptr, 2);
  	index = strtol(indexS, endptr, 2);

  	set = cache->sets[index];
  	lines = set->lines;
  	numOfLines = set->numLines;

  	cache->clock++;
  	
  	// parse through the lines of the given set
  	for(i = 0; i < numOfLines; i++) {
    	line = lines[i];
    	if(!(line->valid)) { // if line is not valid
      		empty = true; // full line
      		LRUIndex = i;
    	} else if(line->tag == tag) { // HIT
      		status = HIT;
      		line->lastTimeUsed = cache->clock;
      		break;
    	}

  		if(!empty && line->lastTimeUsed < LRUTime) { // look oldest index if full
      		LRUIndex = i;
      		LRUTime = line->lastTimeUsed;
    	}
  	}
  
  	if(status != HIT && !empty) { // MISS_FULL
    	lines[LRUIndex]->tag = tag;
    	lines[LRUIndex]->lastTimeUsed = cache->clock;
    	status = MISS_FULL;
  	} else if(status != HIT && empty) { // MISS_EMPTY
    	lines[LRUIndex]->tag = tag;
    	lines[LRUIndex]->lastTimeUsed = cache->clock;
    	lines[LRUIndex]->valid = true;
    	status = MISS_EMPTY;
  	}

  	if(status < 0) {
    	fprintf(stderr, "ERROR! STATUS = %d\n", status);
  	}
  	return status;
}

// helper function to create cache
void makeCache(Cache *cache, int s, int E, int b) {
  	int setIndex, lineIndex, numOfSets;
  	Set **sets, *set;
  	Line *line;
  
  	cache->numSets = (int) pow(2, s); // number of sets = 2^s
  	numOfSets = cache->numSets; // removes dereference penalty in loop
  	cache->sets = malloc(numOfSets * POINTER_SIZE);
  	sets = cache->sets;
  	cache->clock = 0;
  
  	for(setIndex = 0; setIndex < numOfSets; setIndex++) { // init each set
    	sets[setIndex] = malloc(sizeof(Set));
    	set = sets[setIndex];
    	set->lines = malloc(E * POINTER_SIZE);
    	set->numLines = E;

    	for(lineIndex = 0; lineIndex < E; lineIndex++) { // init each line
      		set->lines[lineIndex] = malloc(sizeof(Line));
      		line = set->lines[lineIndex];
      		line->valid = false;
      		line->lastTimeUsed = -1;
      		line->tagSize = MEMORY_ADDRESS_SIZE - (s + b); // formula from book
      		line->blockSize = b;
      		line->totSize = VALID_BIT_SIZE + line->tagSize + line->blockSize;

      		line->full = malloc(line->totSize * sizeof(char));
      		line->tag = -1;
    	}
  	}
}

// helper function to reset a cache
void flushCache(Cache *cache) {
  	int setIndex, lineIndex; // temps
  	Set **sets, *set; // temp set ptr
  	Line *line; // temp line ptr
  	sets = cache->sets; // assign sets from given cache

  	// parse through all sets
  	for(setIndex = 0; setIndex < cache->numSets; setIndex++) { // free each set
    	set = sets[setIndex]; // temp set
    	// parse through all lines in given set
    	for(lineIndex = 0; lineIndex < set->numLines; lineIndex++) { // free each line
      		line = set->lines[lineIndex];
      		free(line->full);
      		free(line);
    	}
    	free(set->lines);
    	free(set);
  	}
  	free(sets);
  	free(cache); // freed all data storages
}

void getBin(int64_t num, char *str) {
  	int64_t mask = 0x1;
  	*str++ = 'e';
  	while(mask != 0) {
    	*str++ = !!(mask & num) + '0';
    	mask <<= 1;
  	}
}

// helper function to reverse a string
void reverseStr(char s[]) {
	// local var for length
    int length = strlen(s) ;
    int temp, i, j; // temps

    // sandwich the middle of the array of chars, swapping ith and jth elements
    for (i = 0, j = length - 1; i < j; i++, j--) {
        temp = s[i];
        s[i] = s[j];
        s[j] = temp;
    }
}

void cutLeadingAddressZeros(char str[]) {
  	int i = 0, firstDigitIndex;
  	char buff[30];

  	// as long as input is valid, first digit will never be num
  	while(!isdigit(str[++i])) { // find the first digit
  		i++;
  	}
  	firstDigitIndex = i;

  	if(str[firstDigitIndex] == '0') {
    	while(str[++i] == '0') { // find first nonzero digit
    		i++;
   	 	}
		strcpy(buff, &str[i]); // move to buffer to prevent mem errors
    	strcpy(&str[firstDigitIndex], buff);
  	}
}

void trimEnd(char str[]) {
  	int i = strlen(str); // get end of str
 	while(str[--i] <= 32) { // while str is not a printable char
 	 	i++;
  	}
  	// after loop i is the last non-space character in str
	str[++i] = '\0';
}

// helper function to help print the instructions for the given flag modes
void formatHelp(char* op) {
	printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", op);
  	printf("Options:\n");
 	printf("  -h         Print this help message.\n");
 	printf("  -v         Optional verbose flag.\n");
 	printf("  -s <num>   Number of set index bits.\n");
 	printf("  -E <num>   Number of lines per set.\n");
  	printf("  -b <num>   Number of block offset bits.\n");
  	printf("  -t <file>  Trace file.\n\n");
  	printf("Examples:\n");
  	printf("  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", op);
  	printf("  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", op);
}
