/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#define MAX_SIZE 32 //max number of lines in the table
#define MAX_MACHINE 256

// 00-0 - SNT; 01-1 - WNT; 10-2 - WT; 11-3 - ST
enum States {
    SNT = 0,
    WNT = 1,
    WT = 2,
    ST = 3
};

//struct that holds btb table
typedef struct {
    int btb_array[MAX_SIZE]; // array of the instruction tags
    int is_loc[MAX_SIZE]; // indicator if the row of the table is occupied
    unsigned dest_target[MAX_SIZE]; //array of dest targets, if predicted taken
    char hist_array[MAX_SIZE]; // 32 bytes array
    char** states_machine[MAX_SIZE]; //[MAX_SIZE] // need to change - dynamic allocating
    char fsmstate;
    unsigned historysize;
    unsigned tagsize;
    bool global_hist;
    bool global_table;
    int shared;
    int index_size;
    int type;
} BTB_table;

//declare the global variable btb_table
BTB_table* btb_table;

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){

    btb_table = (BTB_table*)malloc(sizeof(btb_table));
    if (!btb_table) {
        return -1;
    }

    // Validate btb table size
    if (btbSize > MAX_SIZE || btbSize == 0) {
        return -1; // Invalid size
    }
    // assign the size
    if (1==btbSize || 2==btbSize){
        btb_table->index_size = 1;
    } else if(4==btbSize){
        btb_table->index_size = 2;
    } else if(8==btbSize){
        btb_table->index_size = 3;
    } else if(16==btbSize){
        btb_table->index_size = 4;
    } else if(32==btbSize){
        btb_table->index_size = 5;
    } else { // invalid btbSize
        return -1;
    }
    //maybe we need to add more checks
    memset(btb_table->btb_array, 0, sizeof(btb_table->btb_array));
    memset(btb_table->is_loc, 0, sizeof(btb_table->is_loc));
    memset(btb_table->dest_target, 0, sizeof(btb_table->dest_target));
    memset(btb_table->hist_array, 0, sizeof(btb_table->hist_array));

    for (int i = 0; i < MAX_SIZE; i++) {
    //initialize array of pointers to the state machines
    btb_table->states_machine[i] = (char*)malloc(MAX_MACHINE * sizeof(char));
    if (!btb_table->states_machine[i]) {
        return -1;
    }

    // Initialize the allocated memory with fsmState
    memset(btb_table->states_machine[i], fsmState, MAX_MACHINE);
}

    btb_table->fsmstate = fsmState;
    btb_table->historysize = historySize;
    btb_table->tagsize = tagSize;
    btb_table->global_hist = isGlobalHist;
    btb_table->global_table = isGlobalTable;
    btb_table->shared = Shared;
    btb_table->type = BP_pref(isGlobalHist, isGlobalTable);

    return 0; // Indicate success
}

bool BP_predict(uint32_t pc, uint32_t *dst){
    // extracting the pc index to table and tag
    //if pc is not in the table, dest will be pc+4, will update in the table in update function

    int index_mask = ( (1 << btb_table->index_size) - 1) << 2; // 0x00000001 -> 0x00001100
    int extc_index = index_mask & pc;

    int tag_mask = ( ( 1 << btb_table->tagsize) - 1) << 2+btb_table->index_size; //0x00000001 -> 0x01110000 
    int extc_tag = tag_mask & pc; 

    char hist_register = btb_table->hist_array[extc_index];
    char* pc_state_machine = btb_table->states_machine[extc_index]; 

    // H:L , T:G
    if (btb_table->type == 1 ) {
        pc_state_machine = btb_table->states_machine[1];    
    // H:G , T:L
    } else if (btb_table->type == 2 ) {
        hist_register = btb_table->hist_array[1];
    // H:G  , T:G
    } else if (btb_table->type == 3) {
        pc_state_machine = btb_table->states_machine[1];    
        hist_register = btb_table->hist_array[1];
    }
    
    if(btb_table->is_loc[extc_index]) { //the tag bits reprent existing prediction
        if (btb_table->btb_array[extc_index] == extc_tag) {
            //predict the existing address and return the matching state-machine answer
            //char hist_register = btb_table->hist_array[extc_index];
            //char* pc_state_machine = btb_table->states_machine[extc_index];
            if(((pc_state_machine[hist_register]) == SNT)||
             ((pc_state_machine[hist_register]) == WNT)){
                *dst = pc+4;
                return false;        
            }
            else{
                *dst = btb_table->dest_target[extc_index];
                return true;
            }
        }
    } else {
        //predict pc+4 and return false
        *dst = pc+4;
        return false;
        
    }	
}

// if prediction taken- update the matching state machine and history register
// if prediction not existing - add new line to btb_table and initialize state machine and history register and assign values
void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
    int index_mask = ( (1 << btb_table->index_size) - 1) << 2; // 0x00000001 -> 0x00001100
    int extc_index = index_mask & pc;

    int tag_mask = ( ( 1 << btb_table->tagsize) - 1) << 2+btb_table->index_size; //0x00000001 -> 0x01110000 
    int extc_tag = tag_mask & pc; 

   /* char hist_register = btb_table->hist_array[extc_index];
    char* pc_state_machine = btb_table->states_machine[extc_index]; 

    // H:L , T:G
    if (btb_table->type == 1 ) {
        pc_state_machine = btb_table->states_machine[1];    
    // H:G , T:L
    } else if (btb_table->type == 2 ) {
        hist_register = btb_table->hist_array[1];
    // H:G  , T:G
    } else if (btb_table->type == 3) {
        pc_state_machine = btb_table->states_machine[1];    
        hist_register = btb_table->hist_array[1];
    }*/

    if (btb_table->is_loc[extc_index]) { 
        if (btb_table->btb_array[extc_index] == extc_tag) { // prediction existing, update machine & update history
            //char hist_register = btb_table->hist_array[extc_index];
            //char* pc_state_machine = btb_table->states_machine[extc_index];
            if (taken && (pc_state_machine[hist_register]) == !ST) {
                btb_table->states_machine[extc_index][hist_register]++ ;
            }
            else if(!taken && (pc_state_machine[hist_register]) != SNT){
                btb_table->states_machine[extc_index][hist_register]-- ;
            }
            btb_table->hist_array[extc_index] = (hist_register << 1) + (int)taken; //update history
        }
    } else { // prediction not existing so not taken - pc+4
        btb_table->btb_array[extc_index] = extc_tag; //insert the pc to table in correct index
        btb_table->is_loc[extc_index] = 1;
        btb_table->dest_target[extc_index] = targetPc;

        memset(btb_table->states_machine[extc_index], btb_table->fsmstate, MAX_MACHINE);
        // checkif the history register needs to be upgraded here or not
        btb_table->hist_array[extc_index] = 0 + (int)taken; //update history
        char hist_register = btb_table->hist_array[extc_index];

        if(taken){

            btb_table->states_machine[extc_index][hist_register]++;
        }else{
            btb_table->states_machine[extc_index][hist_register]--;
        }
    }
	return;
}


void BP_GetStats(SIM_stats *curStats){
	return;
}

//hist-table 
// 11 = 3
// if hist == global && table == global --> 1 hist_reg and 1 FSM table
// 10 = 2
// if hist == global && table == local  --> 1 hist_reg and btb-size FSM table
// 01 = 1
// if hist == local && table == global  --> btb-size hist_reg and 1 FSM table
// 00 = 0
// if hist == local && table == local   --> btb size hist_reg and btb-size FSM table

// Ghare - global history register xored with the PC - output is a location in the FSM table
// LShare - local history register xored with the PC - output isa location in the FSM table


int BP_pref(bool global_hist, bool global_table) {
    int ret_value= 3;
    if (!global_hist) {
        if(global_table) {
            ret_value = 1;
        } else {
            ret_value = 0;
        }   
    } else {
        if (!global_table) {
            ret_value = 2;
        }
    }
    return ret_value;
}


