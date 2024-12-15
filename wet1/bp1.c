/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_SIZE 32 //max number of lines in the table
#define MAX_MACHINE 256 //max number of machines (if history size is 8 bits)

// 00-0 - SNT; 01-1 - WNT; 10-2 - WT; 11-3 - ST
enum States {
    SNT = 0,
    WNT = 1,
    WT = 2,
    ST = 3
};
//0 - LHLT; 1 - LHGT; 2 - GHLT; 3 - GHGT
enum Types {
    LHLT = 0,
    LHGT = 1,
    GHLT = 2,
    GHGT = 3
};

int check_flush(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);
int BP_pref(bool global_hist, bool global_table);
void PC_exist_update_machine_history(int extc_index, bool taken, uint32_t pc);
void PC_new_update_machine_history(int extc_index, bool taken, uint32_t pc);
int shared_index(uint32_t pc, int hist_register);

//struct that holds btb table
typedef struct {
    int btb_array[MAX_SIZE]; // array of the instruction tags
    int is_loc[MAX_SIZE]; // indicator if the row of the table is occupied (made for cases where tag=0)
    unsigned dest_target[MAX_SIZE]; //array of dest targets of PC
    unsigned hist_array[MAX_SIZE]; // 32 bytes array, occupies local history or global (in index 0)
    //int* states_machine[MAX_SIZE]; //array of pointers to machines, each machine is a counter counts from 0 to 3
    int states_machine[MAX_SIZE][MAX_MACHINE];
    unsigned fsmstate; //initial state of the machine
    unsigned historysize; //size of history in bytes. affect number of machines.
    unsigned history_mask; // the maximum number in binary mask
    unsigned tagsize; // size of tag in bytes
    bool global_hist; //true if global history
    bool global_table; //true if global table
    int shared; //0-no shared. 1-lsb. 2-mid.
    int index_size; // index size in bytes in the table. affected by size of table.
    int type; // one of 4 types (enum)
    int branch_num;
    int flush_num;
    unsigned btbSize;
    } BTB_table;


//declare the global variable btb_table
BTB_table* btb_table;



int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){

    btb_table = (BTB_table*)malloc(sizeof(BTB_table));
    if (!btb_table) {
        return -1;
    }

    // Validate btb table size
    if (btbSize > MAX_SIZE || btbSize == 0) {
        return -1; // Invalid size
    }

    if (historySize > 8 || historySize < 1) {
        return -1; //invalid history size
    }

    // assign the size
    if (1==btbSize) {
        btb_table->index_size = 0;
    } else if (2==btbSize){
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

    if (tagSize < 0 || tagSize > (30 - btb_table->index_size) ) {
        return -1 ; // invalid tag size
    }

    if(fsmState < 0 || fsmState > 3) { // 0,1,2,3
        return -1; //invalid fsm size
    }

    if ((Shared != 0) && !isGlobalTable) {
        return -1; // share with local table
    }

    for (int i = 0 ; i < MAX_SIZE; i++) {
        btb_table->btb_array[i] = 0;
        btb_table->is_loc[i] = 0;
        btb_table->dest_target[i] = 0;
        btb_table->hist_array[i] = 0;
        for (int j = 0; j < MAX_MACHINE; j++ ) {
            btb_table->states_machine[i][j] = fsmState;
        }
    }

    btb_table->fsmstate = fsmState;
    btb_table->historysize = historySize;
    btb_table->history_mask = (1 << historySize) - 1; // 1 --> 100 --> 011
    btb_table->tagsize = tagSize;
    btb_table->global_hist = isGlobalHist;
    btb_table->global_table = isGlobalTable;
    btb_table->shared = Shared;
    btb_table->type = BP_pref(isGlobalHist, isGlobalTable);
    btb_table->branch_num = 0;
    btb_table->flush_num = 0;
    btb_table->btbSize = btbSize;

    return 0; // Indicate success
}

bool BP_predict(uint32_t pc, uint32_t *dst){
    // extracting the pc index to table, and tag

    int index_mask = ( (1 << btb_table->index_size) - 1) << 2; // 0x00000001 -> 0x00001100
    int extc_index = (index_mask & pc) >> 2 ;

    int tag_mask = ( ( 1 << btb_table->tagsize) - 1) << (2+btb_table->index_size); //0x00000001 -> 0x01110000 
    int extc_tag = (tag_mask & pc) >> (btb_table->index_size + 2); 

    // H:L , T:G
    int hist_register = btb_table->hist_array[extc_index];
    int* pc_state_machine = btb_table->states_machine[extc_index]; 

    /*
    *   Example for Local History - Global Table   btb_table->states_machine[0][hist_register]-- ;
    *
    * 
    *
    */

    //choosing right indexes for the history and tables
    // H:L , T:G
    if (btb_table->type == LHGT ) {
        pc_state_machine = btb_table->states_machine[0];    
    // H:G , T:L
    } else if (btb_table->type == GHLT ) {
        hist_register = btb_table->hist_array[0];
    // H:G  , T:G
    } else if (btb_table->type == GHGT) {
        pc_state_machine = btb_table->states_machine[0];    
        hist_register = btb_table->hist_array[0];
    }

    // here is the shared logic (lsb / mid, pc xor history) (only if needed)
    // if we don't use share, return hist_register passed to function
    //printf("\nhist register before share: %d\n",hist_register);

    int shared_hist_register = shared_index(pc, hist_register); //finding the index after hashing
    //hist_register = new_hist_register; //changing the hist_register to contain the xor result
    
   // printf("\nhist register: %d, shared_register: %d\n", hist_register, shared_hist_register);
    /*  int curr_fsm;
    if (btb_table->type == LHGT) {
        curr_fsm = btb_table->states_machine[0][hist_register];
        hist_register = btb_table->hist_array[extc_index];
    }*/

    //  //   printf("\n***** curr fsm: %d, curr_hist_reg: %d\n ", curr_fsm, hist_register);
    //    printf("\n fsm: 0: %d, 1:%d, 2:%d, 3:%d, 4:%d, 5:%d, 6:%d, 7:%d\n", btb_table->states_machine[0][0], btb_table->states_machine[0][1], btb_table->states_machine[0][2],
    //                                                                     btb_table->states_machine[0][3], btb_table->states_machine[0][4], btb_table->states_machine[0][5], 
    //                                                                     btb_table->states_machine[0][6], btb_table->states_machine[0][7]);

    //here is where we're predicting

//    printf("\nin predict function: extc tag: 0x%08x, extrc index : %d\n",extc_tag, extc_index);
//    printf("fsm state: %d\n", pc_state_machine[hist_register]);
//    printf("is loc: %d, nbtb_table tag:0x%08x, extracted tag: 0x%08x, \n",btb_table->is_loc[extc_index],  btb_table->btb_array[extc_index], extc_tag);
//    printf("hist reg: %d, fsm machime: %d \n", hist_register,  pc_state_machine[shared_hist_register]);

    if( (btb_table->is_loc[extc_index]) && (btb_table->btb_array[extc_index] == extc_tag) ) { //tag of PC is in the BTB. predict.
            if(((pc_state_machine[shared_hist_register]) == SNT) || ((pc_state_machine[shared_hist_register]) == WNT)){
               // printf("fsm state: %d\n", pc_state_machine[hist_register]);
                *dst = pc+4;
              //   printf("\n if SNT OR WNT +4 dest: 0x%08x\n", *dst);
                return false; //predict branch not taken        
            }
            else{
                *dst = btb_table->dest_target[extc_index];
               // printf("\n not SNT OR WNT jump dest: %04x\n", *dst);
                return true; //predict branch taken
            }
    } else {
        //PC is not in the BTB, predict branch not taken
        *dst = pc+4;
      //  printf("\nnew +4 dest: %04x\n", *dst);
        return false;
        
    }	
}

// if prediction exists- update the state machine and history register
// if prediction doesn't exist - add new line to btb_table and initialize state machine and history register and assign values
// in Shared situation, we need to index the machine based on this hashed index
// need to call here the BP_predict func (before update), so we can have statistics
void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
    int index_mask = ( (1 << btb_table->index_size) - 1) << 2; // 0x00000001 -> 0x00001100
    int extc_index = (index_mask & pc) >> 2 ;
 //   printf("mast: %d, index: %d\n", index_mask, extc_index);

    int tag_mask = ( ( 1 << btb_table->tagsize) - 1) << (2+btb_table->index_size); //0x00000001 -> 0x01110000 
    int extc_tag = (tag_mask & pc) >> (btb_table->index_size + 2); 


    if ( (btb_table->is_loc[extc_index]) && (btb_table->btb_array[extc_index] == extc_tag) ) { // pc is in the BTB table
            // updates the machine and the history, depends if its local / global
            btb_table->dest_target[extc_index] = targetPc;
     //       int hist_reg = btb_table->hist_array[extc_index];
            PC_exist_update_machine_history(extc_index, taken, pc);
    //        printf("\n***existed updated: hist reg: %d, fsm machime: %d \n",hist_reg,  btb_table->states_machine[0][hist_reg]);
    } else { 
        // pc not in BTB, need to add it to table & update machine and history
        btb_table->btb_array[extc_index] = extc_tag; //insert the pc to table in correct index
        btb_table->is_loc[extc_index] = 1;
        btb_table->dest_target[extc_index] = targetPc;
    //  int hist_reg = btb_table->hist_array[extc_index];

        PC_new_update_machine_history(extc_index, taken, pc);
        
   //    printf("\n***new updated: hist reg: %d, fsm machime: %d \n",hist_reg,  btb_table->states_machine[0][hist_reg]);
    }

    btb_table->branch_num += 1; // sum the new branch to branch counter
    btb_table->flush_num += check_flush(pc, targetPc, taken, pred_dst);
	return;
}


void BP_GetStats(SIM_stats *curStats){
	curStats->flush_num = btb_table->flush_num;
    curStats->br_num = btb_table->branch_num;

    //Amit: not sure about the sizes here, and where should we add the valid bit
    

    if (btb_table->type == LHLT){
                //(number of lines in table) * ((valid + tag size + target size + hist_reg size) + (FSM bits num * local FSM num))
        curStats->size = (btb_table->btbSize)*((31+btb_table->tagsize+btb_table->historysize) + 2*(1 << (btb_table->historysize)));
    }
    else if (btb_table->type == LHGT) {
        curStats->size = (btb_table->btbSize) * (31+btb_table->tagsize+btb_table->historysize) + 2*(1 << btb_table->historysize);
    } else if (btb_table->type == GHLT ) {
        curStats->size = (btb_table->btbSize) * ( (31+btb_table->tagsize)  + (2*(1 << btb_table->historysize)) ) + btb_table->historysize;
    } else if (btb_table->type == GHGT) {
        curStats->size = (btb_table->btbSize) * ((31+btb_table->tagsize)) + btb_table->historysize + (2*(1 << btb_table->historysize));
    }
    
    free(btb_table); // Free the BTB table itself
    
    //btb_table = NULL; // Avoid dangling pointer

    return;
}

int check_flush(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst) {
    // Possible situations:
    // 1. Branch taken: targetPc == pred_dst -> no flush
    // 2. Branch taken: targetPc != pred_dst -> flush
    // 3. Branch not taken: targetPc == pred_dst -> flush 
    //    (Remember! targetPc = branch pc, even if NT)
    // 4. Branch not taken: targetPc != pred_dst -> no flush

    uint32_t pc4 = pc + 4; // Address of the next instruction (pc + 4)

    // Case 1: Pc is not in table ("liar PC")
    if ((targetPc != pred_dst) && (pc4 != pred_dst)) {
        // Flush if branch is taken or if branch not taken and pc4 != pred_dst
        return taken || (!taken && (pc4 != pred_dst));
    }

    // Case 2: Pc is in table ("good PC")
    if (taken) {
        // Flush if targetPc != pred_dst
        return (targetPc != pred_dst);
    } else {
        // Flush if targetPc == pred_dst (branch not taken)
        return (targetPc == pred_dst);
    }
}


int BP_pref(bool global_hist, bool global_table) {
    int ret_value= GHGT; //GHGT
    if (!global_hist) {
        if(global_table) {
            ret_value = LHGT;//LHGT
        } else {
            ret_value = LHLT;//LHLT
        }   
    } else {
        if (!global_table) {
            ret_value = GHLT;//GHLT
        }
    }
    return ret_value;
}

void PC_exist_update_machine_history(int extc_index, bool taken, uint32_t pc){
    // This function is for updating history and machines, if the pc is in the table, based on Type
  //  int hist_reg = btb_table->hist_array[extc_index];
  //  printf("\n***inside update function: hist reg: %d, fsm machime: %d \n",hist_reg,  btb_table->states_machine[extc_index][hist_reg]);
    // H:L , T:L
    if(btb_table->type == LHLT){
        int hist_register = (int)btb_table->hist_array[extc_index];
        int* pc_state_machine = btb_table->states_machine[extc_index];

        if (taken && (pc_state_machine[hist_register]) != ST) {//update machine
            btb_table->states_machine[extc_index][hist_register]++ ;
        }
        else if(!taken && (pc_state_machine[hist_register]) != SNT){
            btb_table->states_machine[extc_index][hist_register]-- ;
        }

        
        btb_table->hist_array[extc_index] = ((hist_register << 1) + (int)taken) & btb_table->history_mask; //update history
        // 11 --> 110 --> 110 & 000..0011 --> 010 --> 00...00010
    }
    // H:L , T:G
     else if(btb_table->type == LHGT){

        int hist_register = btb_table->hist_array[extc_index];
        int* pc_state_machine = btb_table->states_machine[0]; //reffering to global machine

   //     printf("first hist register: %d\n", hist_register);

        // shared logic - to choose right index for the global machine
        int shared_hist_register = shared_index(pc, hist_register); //finding the index after hashing
      //  hist_register = new_hist_register; //changing the hist_register to contain the xor result

    //   printf("\nold fsm: %d   , register before change and after share: %d\n", btb_table->states_machine[0][hist_register],  hist_register);

        if (taken && (pc_state_machine[shared_hist_register]) != ST) {//update global machine
             btb_table->states_machine[0][shared_hist_register]++ ;
        }
        else if(!taken && (pc_state_machine[shared_hist_register]) != SNT){
            btb_table->states_machine[0][shared_hist_register]-- ;
        }

        //history register update
        btb_table->hist_array[extc_index] =  ( ((hist_register << 1) + (int)taken) ) & (btb_table->history_mask); //update local history of pc
     //   printf("\nhistory mask: 0x%08x, moved register: 0x%08x\n ",btb_table->history_mask , ( ((hist_register << 1) + (int)taken) ) & (btb_table->history_mask) );
    //   printf("\nnew fm: %d   ,new history register:%d taken: %d  \n", btb_table->states_machine[0][hist_register] , btb_table->hist_array[extc_index], taken);




    }
 // H:G , T:L
    else if(btb_table->type == GHLT){

        int hist_register = (int)btb_table->hist_array[0];
        int* pc_state_machine = btb_table->states_machine[extc_index];

        //printf("old hist register: %d,btb_index: %d\n", hist_register, extc_index);

        if (taken && (pc_state_machine[hist_register]) != ST) {//update local machine
            btb_table->states_machine[extc_index][hist_register]++ ;
        }
        else if(!taken && (pc_state_machine[hist_register]) != SNT){
            btb_table->states_machine[extc_index][hist_register]-- ;
        }

     //   printf("updated fsm: %d\n", btb_table->states_machine[extc_index][hist_register]);

        btb_table->hist_array[0] = ((hist_register << 1) + (int)taken) & (btb_table->history_mask); //update global history
       
    //    printf("hist1: %d, mask: %d\n", (hist_register << 1) + (int)taken, btb_table->history_mask);
    //     printf("hist1: %d\n", (((hist_register << 1) + (int)taken) & (btb_table->history_mask)));
    //     printf("new history register:%d\n", btb_table->hist_array[0]);

        
}
    else { // H:G, T:G
        int hist_register = (int)btb_table->hist_array[0];
        int* pc_state_machine = btb_table->states_machine[0];

        // shared logic - to choose right index for the global machine
        int shared_hist_register = shared_index(pc, hist_register); //finding the index after hashing
        //hist_register = new_hist_register; //changing the hist_register to contain the xor result

        if (taken && (pc_state_machine[shared_hist_register]) != ST) {//update global machine
            btb_table->states_machine[0][shared_hist_register]++ ;
        }
        else if(!taken && (pc_state_machine[shared_hist_register]) != SNT){
            btb_table->states_machine[0][shared_hist_register]-- ;
        }

        btb_table->hist_array[0] =  ((hist_register << 1) + (int)taken) & (btb_table->history_mask); //update global history
        }
}

void PC_new_update_machine_history(int extc_index, bool taken, uint32_t pc){
    // This function is for updating history and machines, if the pc is NOT in the table, based on Type
    // Amit: remmember, if we are in share, we use it to access in a different index to state table.
     // H:L , T:L
    if(btb_table->type == LHLT){
        int fsmstate = btb_table->fsmstate; //restart state of the machine
        int new_state = fsmstate;
        if(taken && fsmstate != ST){
            new_state++;
        }
        if(!taken && fsmstate != SNT){
            new_state--;
        }
        
        //Amit: needed to change here! to old hist register...
        //int hist_register = 0 + (int)taken; //index to machine 
        int hist_register = 0;
        btb_table->hist_array[extc_index] = (0 + (int)taken) & btb_table->history_mask; //update local hist
        //restart machine to start state

        //memset(btb_table->states_machine[extc_index], fsmstate, MAX_MACHINE); 
        for(int j=0; j<MAX_MACHINE; j++){
            btb_table->states_machine[extc_index][j] = fsmstate;
        }
        // update machine in index history to new_state
        btb_table->states_machine[extc_index][hist_register] = new_state;
        }
    // H:L , T:G
    else if(btb_table->type == LHGT){
      // no need to restart machine to fsm cause it's global, just need to update it & restart history
      // add Shared logic cause of global machine
        int hist_register = btb_table->hist_array[extc_index];
       //Amit: needed to change here! to old hist register...
       //int hist_register = 0 + (int)taken;
        //printf("hisg register after zero: %d\n", hist_register);

        hist_register = 0;
        int shared_hist_register = shared_index(pc, hist_register); //finding the index after hashing

        //hist_register = new_hist_register; //changing the hist_register to contain the xor result

        //printf("hist register after shared: %d\n", hist_register);

        //printf("old fsm: %d   , old hist regier: %d\n", btb_table->states_machine[0][hist_register],  hist_register);
        
        // update global machine
        if(taken && btb_table->states_machine[0][shared_hist_register] != ST){
           btb_table->states_machine[0][shared_hist_register]++;
        } else if(!taken && btb_table->states_machine[0][shared_hist_register] != SNT){
           btb_table->states_machine[0][shared_hist_register]--;
        }

        btb_table->hist_array[extc_index] = (0 + (int)taken) & (btb_table->history_mask); //update local hist

     //   printf("after new update machine: new fm: %d   ,new history register:%d, taken %d \n", btb_table->states_machine[0][hist_register] , btb_table->hist_array[extc_index],taken);

    }
    // H:G , T:L
    else if(btb_table->type == GHLT){
    // no need to restart global history, just update it. need to restart and update machine.

        //Amit: needed to change here! to old hist register...
        //btb_table->hist_array[0] = (btb_table->hist_array[0] << 1) + (int)taken; //update global hist

        int fsmstate = btb_table->fsmstate; //restart state of the machine
        int new_state = fsmstate;
        if(taken && fsmstate != ST){
            new_state++;
        }
        if(!taken && fsmstate != SNT){
            new_state--;
        }

        int hist_register = (int)btb_table->hist_array[0]; //index to machine table, before updating history

      //  printf("old hist register: %d,btb_index: %d\n", hist_register, extc_index);

        //restart machine to start state
        
        //memset(btb_table->states_machine[extc_index], fsmstate, MAX_MACHINE); 

        for(int j=0; j<MAX_MACHINE; j++){
            btb_table->states_machine[extc_index][j] = fsmstate;
        }

        // update machine in enter history to new_state
        btb_table->states_machine[extc_index][hist_register] = new_state;

     //   printf("updated fsm: %d\n", btb_table->states_machine[extc_index][hist_register]);

        btb_table->hist_array[0] = ((btb_table->hist_array[0] << 1) + (int)taken) & (btb_table->history_mask); //update history
      //  printf("after new update machine: new fm: %d   ,new history register:%d, taken %d \n", btb_table->states_machine[0][hist_register] , btb_table->hist_array[extc_index],taken);

     //   printf("new history register:%d\n", btb_table->hist_array[0]);


    }
     // H:G, T:G
    else {
        // no need to restart global history, just update it
        // no need to restart global machine, just update it.
        // using shared logic
        
        //Amit: needed to change here! to old hist register...

        //btb_table->hist_array[0] = (btb_table->hist_array[0] << 1) + (int)taken; // updated global hist
        int hist_register = btb_table->hist_array[0]; 

        int shared_hist_register = shared_index(pc, hist_register); //finding the index after hashing
        //hist_register = new_hist_register; //changing the hist_register to contain the xor result

        // update global machine
        if(taken && btb_table->states_machine[0][shared_hist_register] != ST){
           btb_table->states_machine[0][shared_hist_register]++;
        } else if(!taken && btb_table->states_machine[0][shared_hist_register] != SNT){
           btb_table->states_machine[0][shared_hist_register]--;
        }
        

        btb_table->hist_array[0] = (((btb_table->hist_array[0] << 1) + (int)taken)) & (btb_table->history_mask); // updated global hist   

       // printf("after new update machine: new fm: %d   ,new history register:%d, taken %d \n", btb_table->states_machine[0][hist_register] , btb_table->hist_array[extc_index],taken);

    }
}

int shared_index(uint32_t pc, int hist_register){
    // here is the shared logic (lsb / mid, pc xor history)
    if(btb_table->shared == 1){ //using share lsb
        
        int pc_bits = (pc >> 2) ; //bitwize and between pc and lsb mask
     //   printf("\nhist_register: 0x%08x, pc: 0x%08x,  moved pc bits: 0x%08x, pc_bits\n",hist_register, pc, pc_bits);
        int new_hist_register = (pc_bits ^ hist_register) & (btb_table->history_mask); //this is a new index to the machine table
     //   printf("pic_bit XORED with hist_register: 0x%08x, histroy mask: 0x%08x, pc bits xored and masked: 0x%08x\n", pc_bits ^ hist_register,btb_table->history_mask,  new_hist_register);
      //  printf("pc_bits %d,    ne_hist_register: %d\n",pc_bits,new_hist_register  );

        return new_hist_register; 

    }else if(btb_table->shared == 2){ //using share mid

        int pc_bits = (pc >> 16) ; //bitwize and between pc and mid mask
   //     printf("pc: 0x%08x,  moved pc bits: 0x%08x, pc_bits\n", pc, pc_bits);
        int new_hist_register = (pc_bits ^ hist_register) & (btb_table->history_mask); //this is a new index to the machine table
        // printf("pic_bit XORED with hist_register: 0x%08x, histroy mask: 0x%08x, pc bits xored and masked: 0x%08x\n", pc_bits ^ hist_register,btb_table->history_mask,  new_hist_register);
        // printf("history_mask: %d\n", btb_table->history_mask);
        // printf("pc_bits %d,    new_hist_register: %d,     old hist register: %d\n",pc_bits,new_hist_register, hist_register  );

        return new_hist_register; 
    }
    
    return hist_register; //if we're not in shared, we need to return the hist_register we've calculated
}
    

            
           
         
