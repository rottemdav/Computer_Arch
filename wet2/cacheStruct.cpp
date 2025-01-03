#include <vector>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <cstdlib>


using std::string;

class Address {
public:
    unsigned int full_address;
    unsigned int offset;
    unsigned int set_L1;
    unsigned int set_L2;
    unsigned int tag_L1;
    unsigned int tag_L2;
    unsigned int last_seen_L1;
    unsigned int last_seen_L2;
    bool in_L1;
    bool in_L2;

    //constructor 
    Address(int address_in_binary, int offset_size, int set_size_l1, int set_size_l2, int tag_size_l1,  int tag_size_l2) {

        // createing masks for the address components
        unsigned int set_mask_L1 = ((1<<offset_size+set_size_l1) - 1) ^ ((1<<offset_size) - 1);
        unsigned int set_mask_L2 = ((1<<offset_size+set_size_l2) - 1) ^ ((1<<offset_size) - 1);
        unsigned int tag_mask_L1 = ((1U<<32)-1) ^ ((1<<offset_size+set_size_l1) - 1);
        unsigned int tag_mask_L2 = ((1U<<32)-1) ^ ((1<<offset_size+set_size_l2) - 1);

        // extracting the matching componenets and assigning them to the class 
        offset = address_in_binary & ((1<<offset_size)-1);
        set_L1 = address_in_binary & set_mask_L1;
        set_L2 = address_in_binary & set_mask_L2;
        tag_L1 = address_in_binary & tag_mask_L1;
        tag_L1 = address_in_binary & tag_mask_L2;

        //initializing the rest of the class memebers
        last_seen_L1 = 0;
        last_seen_L2 = 0;

        in_L1 = false;
        in_L2 = false;        

}
    //Copy Constructor

    // class functions
    Address* get_address()  {
        return this;
    }
    
} ;

class Way {
public:
    int Way_Size; /*Number of lines in the way*/
    std::vector<Address> Address_Arr;

    //constructor
    Way(int way_size): Way_Size(way_size), Address_Arr() { }

    //destructor 
    ~Way() { }
} ;

class Cache {
public: 
    int BlockSize; /*maybe won't need it inside class*/
    int Wr_Alloc; /*maybe won't need it inside class*/
    int CasheSize;
    int Nways;
    int CounterTime;
    int Way_Size; /*Number of lines in each way = Number of addresses can be saved*/

    Way** Ways_Arr;

    //constructor
    Cache(int blocksize, int wr_alloc, int cashesize, int nways, int way_size) : 
        BlockSize(blocksize), Wr_Alloc(wr_alloc), CasheSize(cashesize), Nways(nways), Way_Size(way_size) {

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

    // class function
    Cache* get_cache() {
        return this;
    }

} ;

int address_exists(Cache cache, Address address, int cache_level) {
    Address* curr_address = address.get_address();
    Cache* curr_cache = cache.get_cache();
    unsigned int curr_tag;
    unsigned int curr_set;

      if (cache_level == 1) {
        curr_tag = curr_address->tag_L1;
        curr_set = curr_address->set_L1;
    } else if (cache_level ==2) {
        curr_tag = curr_address->tag_L2;
        curr_set = curr_address->set_L2; 
    }

    //gets the matching way based on the set of the address
    Way* way_it = curr_cache->Ways_Arr[curr_set];

    // loop through the way to find matching address tag
    for (int i = 0 ; i < curr_cache->Way_Size; i ++) {
        //if the tag of the address in the way found, return 1
        if (cache_level == 1) {
            if (way_it->Address_Arr[i].tag_L1 == curr_tag) {
                return 1;
            }
        }

        if (cache_level == 2) {
              if (way_it->Address_Arr[i].tag_L2 == curr_tag) {
                return 1;
            }
        }
       
    }
    
    return 0;
}

