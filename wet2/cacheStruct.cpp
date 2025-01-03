#include <vector>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <cstdlib>


using std::string;

class Address {
    unsigned int full_address;
    unsigned int offset;
    unsigned int setl1;
    unsigned int setl2;
    unsigned int tagl1;
    unsigned int tagl2;
    unsigned int last_seen_l1;
    unsigned int last_seen_l2;
    bool in_l1;
    bool in_l2;

    public:
    //Constructor -- change this
    Address(string address, string offset, string set, string tag,
            int l1_counter, int l2_counter, bool in_l1, bool in_l2) : 
            full_address(address), offset(offset), set(set), tag(tag),
            last_seen_l1(l1_counter), last_seen_l2(l2_counter), in_l1(in_l1), in_l2(in_l2) {}


    // new constructor 
    //Address(num, offset_size, set_size_l1, set_size_l2, tag_size_l1, tag_size_l2);
    Address(int num, int offset_size, int set_size_l1, int set_size_l2, int tag_size_l1,  int tag_size_l2) {

        // createing masks for the address components
        unsigned int mask_set1 = ((1<<offset_size+set_size_l1) - 1) ^ ((1<<offset_size) - 1);
        unsigned int mask_set2 = ((1<<offset_size+set_size_l2) - 1) ^ ((1<<offset_size) - 1);
        unsigned int mask_tag1 = ((1U<<32)-1) ^ ((1<<offset_size+set_size_l1) - 1);
        unsigned int mask_tag2 = ((1U<<32)-1) ^ ((1<<offset_size+set_size_l2) - 1);

        // extracting the matching componenets and assigning them to the class 

        

    }
    //Copy Constructor
   

    int address_parser(string adress) {
        // fill 
    }

} ;

class Way {
    int Way_Size; /*Number of lines in the way*/
    //Address** Adress_Arr;
    std::vector<Address> Address_Arr;

    public:
    //constructor
    Way(int way_size): Way_Size(way_size), Address_Arr() { }

    //destructor 
    ~Way() { }
} ;

class Cache {
    int BlockSize; /*maybe won't need it inside class*/
    int Wr_Alloc; /*maybe won't need it inside class*/
    int CasheSize;
    int Nways;
    int CounterTime;
    int Way_Size; /*Number of lines in each way = Number of addresses can be saved*/

    Way** Ways_Arr;

    public: 
    //constructor
    Cache(int blocksize, int wr_alloc, int cashesize, int nways, int way_size) : BlockSize(blocksize), Wr_Alloc(wr_alloc), CasheSize(cashesize), Nways(nways), Way_Size(way_size){
        CounterTime = 0;
        
        Ways_Arr = new Way*[Nways];
        
        // Initialize each pointer in the array with a new Way object
        for (int i = 0; i < Nways; i++) {
            Ways_Arr[i] = new Way(way_size); // Dynamically create a new Way object
        }
    }
    //copyconstructor

    //destructor
    ~Cache() {
        for (int i = 0; i < Nways; ++i) {
            delete Ways_Arr[i]; // Delete each Way object
        }
        delete[] Ways_Arr; // Delete the array of pointers
    }
} ;



