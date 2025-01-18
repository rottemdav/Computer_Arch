/* 046267 Computer Architecture - HW #3 */
/* API for the dataflow statistics calculator          */

#ifndef _DFLOW_CALC_H_
#define _DFLOW_CALC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>

/// Maximum number of opcodes in the processor
#define MAX_OPS 32

/// Program context
/// This is a reference to the (internal) data maintained for a given program
typedef void *ProgCtx;
#define PROG_CTX_NULL NULL

/// Instruction info required for dataflow calculations
/// This structure provides the opcode and the register file index of each source operand and the destination operand
typedef struct {
    unsigned int opcode;
             int dstIdx;
    unsigned int src1Idx;
    unsigned int src2Idx;
} InstInfo;

/** analyzeProg: Analyze given program and save results
    \param[in] opsLatency An array of MAX_OPS values of functional unit latency for each opcode
               (some entries may be unused - in that case their value would be 0)
    \param[in] progTrace An array of instructions information from execution trace of a program
    \param[in] numOfInsts The number of instructions in progTrace[]
    \returns Analysis context that may be queried using the following query functions or PROG_CTX_NULL on failure */
ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts);

/** freeProgCtx: Free the resources associated with given program context
    \param[in] ctx The program context to free
*/
void freeProgCtx(ProgCtx ctx);

/** getInstDepth: Get the dataflow dependency depth in clock cycles
    Instruction that are direct decendents to the entry node (depend only on Entry) should return 0
    \param[in] ctx The program context as returned from analyzeProg()
    \param[in] theInst The index of the instruction of the program trace to query (the index in given progTrace[])
    \returns >= 0 The dependency depth, <0 for invalid instruction index for this program context
*/
int getInstDepth(ProgCtx ctx, unsigned int theInst);

/** getInstDeps: Get the instructions that a given instruction depends upon
    \param[in] ctx The program context as returned from analyzeProg()
    \param[in] theInst The index of the instruction of the program trace to query (the index in given progTrace[])
    \param[out] src1DepInst Returned index of the instruction that src1 depends upon (-1 if depends on "entry")
    \param[out] src2DepInst Returned index of the instruction that src2 depends upon (-1 if depends on "entry")
    \returns 0 for success, <0 for error (e.g., invalid instruction index)
*/
int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst);

/** getProgDepth: Get the longest execution path of this program (from Entry to Exit)
    \param[in] ctx The program context as returned from analyzeProg()
    \returns The longest execution path duration in clock cycles 
*/
int getProgDepth(ProgCtx ctx);

#ifdef __cplusplus
}
#endif

#endif /*_DFLOW_CALC_H_*/
