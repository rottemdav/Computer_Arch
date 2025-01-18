/* 046267 Computer Architecture - HW #3 */
/* Main program for testing invocations to dflow_calc  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "dflow_calc.h"

/// Quota of program buffer size increase
#define PROG_SIZE_QUOTA 64

/// Resize program buffer
/// \param[in] maxSize New program buffer size limit
/// \param[in] oldBuf The old buffer used for the program. May be NULL for new allocation.
/// \returns The allocated program buffer of given maxSize entries
InstInfo *allocProgBuffer(size_t maxSize, InstInfo *oldBuf)
{
    InstInfo *newBuf = realloc(oldBuf, maxSize * sizeof(InstInfo));
    if (newBuf == NULL) {
        printf("ERROR: Failed allocating program buffer for %zu instructions!\n", maxSize);
        exit(1);
    }
    return newBuf;
}

/// readTrace: Read a program file formatted with {dst src1 src2} triplets
/// \param[in] filename The trace file name
/// \param[out] prog The program trace (array) that was read from the given file. Allocated in this function. Should be freed by the caller.
/// \returns >0 The number of elements (instructions) in the returned prog[] , <0 error reading the trace file or creating the array
int readProgram(const char *filename, InstInfo **prog) {
    int i;
    unsigned lineNum = 0;
	char curLine[81];
    char *curField, *endOfVal;
    long int fieldVal[4];
    char *tokenizerEntry; // Entry "tag" for strtok()
    InstInfo *progBuf = NULL; // Current program buffer (may be resized as needed)
    size_t maxInsts = 0; // Current limit on the program buffer

    *prog = NULL; // Initialize in case of exit with error

    FILE *progFile = fopen(filename, "r");
	if (progFile == NULL) {
		printf("ERROR: Failed openning the program file: %s\n", filename);
		return -1;
	}

    // Read program file lines
	while (fgets(curLine, sizeof(curLine), progFile) != NULL) {
        //printf("*** curLine=%s\n", curLine);
        if (lineNum == maxInsts) { // Need to resize buffer
            maxInsts += PROG_SIZE_QUOTA;
            progBuf = allocProgBuffer(maxInsts, progBuf);
        }
        tokenizerEntry = curLine;
        while (isspace(*tokenizerEntry)) ++tokenizerEntry; // Strip leading whitespace
        if ((tokenizerEntry[0] == 0) || (tokenizerEntry[0] == '#'))
            continue; // Ignore empty lines and comments (lines that start with '#')
        // Parse line of 4 decimal numbers (opcode + register indices: op dst src1 src2)
        for (i = 0; i < 4; ++i) {
            curField = strtok(tokenizerEntry, " \t\n\r");
            if (curField == NULL) {
                printf("ERROR: Error parsing instruction #%u of %s\n", lineNum, filename);
                return -2;
            }
            fieldVal[i] = strtol(curField, &endOfVal, 10);
            if (endOfVal[0] != 0) {
                printf("ERROR: Failed parsing field %d of line #%u of %s: %s\n", i, lineNum, filename, curLine);
                return -2;
            }
            tokenizerEntry = NULL; // for next tokens should provide strtok NULL
        }
        progBuf[lineNum].opcode = fieldVal[0];
        progBuf[lineNum].dstIdx = fieldVal[1];
        progBuf[lineNum].src1Idx = fieldVal[2];
        progBuf[lineNum].src2Idx = fieldVal[3];
        ++lineNum;
	}

    fclose(progFile);
    *prog = progBuf; // To return to caller
    return lineNum;
}

/// readOpsLatency: Read data file for opcodes execution latency
/// \param[in] opFname The filename of the file with the respective data (one decimal number per line)
/// \param[out] opsLatency Pointer to an array of MAX_OPS entries that would be filled with the respective opcode latency
/// \returns int The number of opcodes info filled in opsLatency[]. The other entries are set to 0.
int readOpsLatency(const char* opFname, unsigned int opsLatency[]) {
    unsigned int numOps = 0;
	char curLine[81];
    char *endOfVal;
    FILE *opcodeFile;

    for (int i = 0; i < MAX_OPS; ++i)
        opsLatency[i] = 0; // Initialize for opcodes that would not be included in the info file
    opcodeFile = fopen(opFname, "r");
	if (opcodeFile == NULL) {
		printf("ERROR: Failed openning %s\n", opFname);
		return -1;
	}
	while (fgets(curLine, sizeof(curLine), opcodeFile) != NULL) { // Read available opcode latency data
        if (numOps >= MAX_OPS) {
            printf("ERROR: Opcodes latency file has more opcodes than maximum supported\n");
            return -3;
        }
        opsLatency[numOps++] = strtol(curLine, &endOfVal, 10);
        while (isspace(*endOfVal)) ++endOfVal; // Strip trailing spaces
        if (endOfVal[0] != 0) { // Verify that parsing ended at end of the line
            printf("ERROR: Failed parsing opcode latency at line %d of %s\n", numOps, opFname);
            return -2;
        }
    }
    fclose(opcodeFile);
    return numOps;
}

void usage(void) {
    printf("Usage: dflow_calc <opcodes info. filename> <program filename> [<Query> <Query>...]\n");
    printf("\tQuery: [p|d]<program line#> - Report [dependency depth| dependencies of this inst.]\n");
    printf("Example: dflow_calc opcode.dat example1.in d4 d7 p12\n");
    exit(1);
}

int main(int argc, const char *argv[]) {
    const char *opFname = argv[1];
    const char *progName = argv[2];
    unsigned int opsLatency[MAX_OPS];
    InstInfo *theProg;
    int progLen, numOps, i, rc;
    int src1Dep, src2Dep;
    ProgCtx ctx;
    char *endPtr;

    if (argc < 3) {
        usage();
    }

    printf("Reading the opcodes latency info from %s ... ", opFname);
    numOps = readOpsLatency(opFname, opsLatency);
    if (numOps < 0)
        exit(1);
    printf("Got latency for %d opcodes\n", numOps);
    printf("Reading the program file %s ... ", progName);
    progLen = readProgram(progName, &theProg);
    if (progLen <= 0) {
        printf("Error reading program file %s!\n", progName);
        exit(1);
    }
    printf("Found %d instructions\n", progLen);
    // Analyze the program
    ctx = analyzeProg(opsLatency, theProg, progLen);
    if (ctx == PROG_CTX_NULL) {
        printf("Error on invocation to analyzeCtx()\n");
        exit(2);
    }
    // Report longest execution path
    printf("getProgDepth()==%d\n", getProgDepth(ctx));
    // Read instruction specific queries (if any)
    for (i = 3; i < argc; ++i) {
        const char qType = argv[i][0];
        const unsigned int instNum = strtol(argv[i]+1, &endPtr, 10);
        if (*endPtr != 0) {
            printf("Error: Invalid instruction number in the query: %s\n", argv[i]);
            exit(3);
        }
        switch (qType) {
        case 'p': // Dependency depth
            rc = getInstDepth(ctx, instNum);
            if (rc < 0) {
                printf("Error %d for getDepDepth(%u)\n", rc, instNum);
            } else {
                printf("getDepDepth(%u)==%d\n", instNum, rc);
            }
            break;
        case 'd': // Instruction dependencies
            rc = getInstDeps(ctx, instNum, &src1Dep, &src2Dep);
            if (rc != 0) {
                printf("Error %d for getInstDeps(%u)\n", rc, instNum);
            } else {
                printf("getInstDeps(%u)=={%d,%d}\n", instNum, src1Dep, src2Dep);
            }
            break;
        default:
            printf("Invalid query type '%c' in argument '%s'\n", qType, argv[i]);
            exit(3);
        }
    }
    freeProgCtx(ctx);
    free(theProg); // We keep theProg up to here to allow your analyzer to use it - if it wants
    return 0;
}
