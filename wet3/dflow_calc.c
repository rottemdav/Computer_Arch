/* 046267 Computer Architecture - HW #3 */
/* Implementation (skeleton)  for the dataflow statistics calculator */
#include <stdio.h>
#include "dflow_calc.h"

struct Inst{
int depend1;
int depend2;
int id;
int depth;
int type; //latency of the inst
int is_father; //does the instruction has children in the tree?
} typedef Inst;

struct DependGraph{
Inst* depend_arr;
int* written_last;
int longest_path;
int num_of_insts;
} typedef DependGraph;



ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts) {
    int curr_longest_path = -1;
    DependGraph* graph = (DependGraph*)malloc(sizeof(DependGraph));
    // initialize depend_arr and written_last
    graph->depend_arr = (Inst*)(malloc(sizeof(Inst)*numOfInsts));
    graph->written_last = (int*)(malloc(sizeof(int)*numOfInsts));
    
    for (int i=0; i<numOfInsts; i++){
        graph->written_last[i] = -1;
    }
    graph->num_of_insts = numOfInsts;
    for(int i=0; i<numOfInsts; i++){
        InstInfo curr_inst = progTrace[i];
        unsigned int opcode = curr_inst.opcode;
        unsigned int dst = curr_inst.dstIdx;
        unsigned int src1 = curr_inst.src1Idx;
        unsigned int src2 = curr_inst.src2Idx;
        
        int depend1 = -1;
        int depend2 = -1;
        
        int path1_len=0;
        int path2_len=0;
        // checks if there is a prev inst that written to src1 or src2 -> dependent
        for (int j=0; j<numOfInsts; j++){
            if(graph->written_last[j] == src1){
                depend1 = j;
                graph->depend_arr[j].is_father=1;
            }
            if(graph->written_last[j] == src2){
                depend2 = j;
                graph->depend_arr[j].is_father=1;
            }
        }
        // enter dst to written_last as the most update access to this reg
        for (int j=0;j<numOfInsts;j++){
            if(graph->written_last[j] == dst){
                graph->written_last[j] = -1;
            }
        }
        graph->written_last[i] = dst;

        //calculate pathes length
        if(depend1 != -1){//there is a dependent ints
            int depend1_type = graph->depend_arr[depend1].type;
            path1_len = graph->depend_arr[depend1].depth + opsLatency[depend1_type];
        }
        if(depend2 != -1){
            int depend2_type = graph->depend_arr[depend2].type;
            path2_len = graph->depend_arr[depend2].depth + opsLatency[depend2_type];
        }

        //updates data in the depend_arr[i]
        graph->depend_arr[i].is_father = 0;
        graph->depend_arr[i].depend1 = depend1;
        graph->depend_arr[i].depend2 = depend2;
        graph->depend_arr[i].id = i;
        //choose the maximum len and set to depend_arr, 
        //if the inst has no dependencies, this lines will initilize graph->depend_arr[i].depth=0 as we want
        if(path1_len>path2_len){ 
            graph->depend_arr[i].depth = path1_len;
        } else {
            graph->depend_arr[i].depth = path2_len;
        }
      //  printf("first: depth %d\n", graph->depend_arr[i].depth);
        graph->depend_arr[i].type = opcode;

        /*if (graph->depend_arr[i].depth > curr_longest_path) {
            curr_longest_path = graph->depend_arr[i].depth;
            curr_longest_type = graph->depend_arr[i].type;
        }*/
    }
    
    // find the longest path in the tre
    for(int i=0; i<numOfInsts; i++){
       // printf("is father: %d\n",graph->depend_arr[i].is_father);
        if(graph->depend_arr[i].is_father == 1){
           // printf("found parent\n");
            continue;
        }
        //printf("curr_longest before if: %d\n", curr_longest_path);
       // printf("second depth: %d\n", graph->depend_arr[i].depth+opsLatency[graph->depend_arr[i].type]);
        int temp_depth = ((graph->depend_arr[i].depth)+(opsLatency[graph->depend_arr[i].type]));
        if(temp_depth > curr_longest_path){
            curr_longest_path = temp_depth;
            //printf("curr_longest_path: %d\n", curr_longest_path);
        }
       // printf("\n");


    }

    graph->longest_path = curr_longest_path;
    return (ProgCtx)graph;
}

void freeProgCtx(ProgCtx ctx) {
    DependGraph* graph = (DependGraph*)ctx;
    // free every pointer in arrays
    free(graph->written_last);
    free(graph->depend_arr);
    free(graph);
}

int getInstDepth(ProgCtx ctx, unsigned int theInst) {
    DependGraph* graph = (DependGraph*)ctx;
    if((theInst <= graph->num_of_insts) && (theInst >= 0)){
        return graph->depend_arr[theInst].depth;
    }
    return -1; //invalid instruction number
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst) {
    DependGraph* graph = (DependGraph*)ctx;
    if((theInst <= graph->num_of_insts) && (theInst >= 0)){
        *src1DepInst = graph->depend_arr[theInst].depend1;
        *src2DepInst = graph->depend_arr[theInst].depend2;
        return 0;
    }
    return -1;
}

int getProgDepth(ProgCtx ctx) {
    DependGraph* graph = (DependGraph*)ctx;
    return graph->longest_path;
}


