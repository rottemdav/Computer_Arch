/* 046267 Computer Architecture - HW #1 */
/* API for the predictor simulator      */

#ifndef BP_API_H_
#define BP_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/* A structure to return information about the currect simulator state */
typedef struct {
	unsigned flush_num;           // Machine flushes
	unsigned br_num;      	      // Number of branch instructions
	unsigned size;		      // Theoretical allocated BTB and branch predictor size
} SIM_stats;

/*************************************************************************/
/* The following functions should be implemented in your bp.c (or .cpp) */
/*************************************************************************/

/*
 * BP_init - initialize the predictor
 * all input parameters are set (by the main) as declared in the trace file
 * return 0 on success, otherwise (init failure) return <0
 */
int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
bool isGlobalHist, bool isGlobalTable, int Shared);

/*
 * BP_predict - returns the predictor's prediction (taken / not taken) and predicted target address
 * param[in] pc - the branch instruction address
 * param[out] dst - the target address (when prediction is not taken, dst = pc + 4)
 * return true when prediction is taken, otherwise (prediction is not taken) return false
 */
bool BP_predict(uint32_t pc, uint32_t *dst);

/*
 * BP_update - updates the predictor with actual decision (taken / not taken)
 * param[in] pc - the branch instruction address
 * param[in] targetPc - the branch instruction target address
 * param[in] taken - the actual decision, true if taken and false if not taken
 * param[in] pred_dst - the predicted target address
 */
void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);

/*
 * BP_GetStats: Return the simulator stats using a pointer
 * curStats: The returned current simulator state (only after BP_update)
 */
void BP_GetStats(SIM_stats *curStats);


#ifdef __cplusplus
}
#endif

#endif /* BP_API_H_ */
