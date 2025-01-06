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
    unsigned int dirty_bit_L1;
    unsigned int dirty_bit_L2;

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
        tag_L2 = address_in_binary & tag_mask_L2;

        //initializing the rest of the class memebers
        last_seen_L1 = 0;
        last_seen_L2 = 0;

        in_L1 = false;
        in_L2 = false;  

        dirty_bit_L1 = 0;
        dirty_bit_L2 = 0;      

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
    int Way_Size; /*Number of lines in each way = Number of addresses can be saved in a single way*/

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

/* returns the address if found, else returns NULL */
Address* address_exists(Cache cache, Address address, int cache_level) {
    Address* curr_address = address.get_address();
    Cache* curr_cache = cache.get_cache();
    unsigned int curr_tag;
    unsigned int curr_set;

    if (cache_level == 1) {// cache_level==1 means we want to find address in L1
        curr_tag = curr_address->tag_L1;
        curr_set = curr_address->set_L1;
    } else if (cache_level ==2) { //
        curr_tag = curr_address->tag_L2;
        curr_set = curr_address->set_L2; // cache_level==2 means we want to find address in L2
    }

    /* iterates through all ways in cache - if there is an address with the same tag AND the same set in one of them - We have a match! return 1. 
        else - return 0,  cause it means the address wasn't found in the cache */ 
    
    for (int i=0; i< curr_cache->Nways; i++) {
        Way* way_it = curr_cache->Ways_Arr[i];

        for (size_t j=0; j < way_it->Address_Arr.size(); j++){
            //if the tag of the address in the way found, return 1
            if (cache_level == 1) {
                if ((way_it->Address_Arr[j].tag_L1 == curr_tag) && (way_it->Address_Arr[j].set_L1 == curr_set) ){
                    return &(way_it->Address_Arr[j]);
                }
            }

            if (cache_level == 2) {
                if ((way_it->Address_Arr[j].tag_L2 == curr_tag) && (way_it->Address_Arr[j].set_L2 == curr_set) ){
                    return &(way_it->Address_Arr[j]);
                }
            }
        }
    }
    return NULL; //if we got out of outer loop it means address is not in any way of cashe
}

int find_free_way(Cache cache, Address address, int cache_level){
    Address* curr_address = address.get_address();
    Cache* curr_cache = cache.get_cache();
    unsigned int curr_set;

    if (cache_level == 1) {// cache_level==1 means we want to find address in L1
        curr_set = curr_address->set_L1;
    } else if (cache_level ==2) { //
        curr_set = curr_address->set_L2; // cache_level==2 means we want to find address in L2
    }

    /* iterates through all ways in cache - if there is a free way in the cuur_set index - return the way index in cache. 
        else - return -1,  which means we need to replace an address in a way the cache */ 
    
    for (int i=0; i< curr_cache->Nways; i++) {
        Way* way_it = curr_cache->Ways_Arr[i];
        bool found_free_way = true;

        for (size_t j=0; j<way_it->Address_Arr.size(); j++){
            //if the tag of the address in the way found, return 1
            if (cache_level == 1) {
                if(way_it->Address_Arr[j].set_L1 == curr_set){
                    found_free_way = false;
                }
            }

            if (cache_level == 2) {
               if(way_it->Address_Arr[j].set_L2 == curr_set){
                    found_free_way = false;
                }
            }
        }
        if(found_free_way){
            return i;
        }
    }
    return -1;
}

void insert_address(Cache cache, Address address, int cache_level, int way_idx){
    Address* curr_address = address.get_address();
    Cache* curr_cache = cache.get_cache();

    if (cache_level == 1) {// insert & update times
        curr_cache->Ways_Arr[way_idx]->Address_Arr.push_back(*curr_address);
        curr_address->in_L1 = true;
    
    } else if (cache_level ==2) { //
        curr_cache->Ways_Arr[way_idx]->Address_Arr.push_back(*curr_address);
        curr_address->in_L2 = true;
    }
}

int remove_address(Cache cache, Address address, int cache_level, Address** addr_to_remove){
    // function uses LRU algorithm to remove the address which it's last seen is the smallest
    // returns the way of the removed address

    Address* curr_address = address.get_address();
    Cache* curr_cache = cache.get_cache();
    unsigned int curr_set;

    if (cache_level == 1) {// cache_level==1 means we want to find address in L1
        curr_set = curr_address->set_L1;
    } else if (cache_level ==2) { //
        curr_set = curr_address->set_L2; // cache_level==2 means we want to find address in L2
    }

    

    /* smallest_last_seen is the current counter and we search for the biggest gap between the current time and LRU. !!! need to update the function !!! */
    int smallest_last_seen = curr_cache->CounterTime;
    int smallet_way = -1;

    for (int i=0; i< curr_cache->Nways; i++) {
        Way* way_it = curr_cache->Ways_Arr[i];

        for (size_t j=0; j<way_it->Address_Arr.size(); j++){
            //if the tag of the address in the way found, return 1
            if (cache_level == 1) {
                if (way_it->Address_Arr[j].set_L1 == curr_set){
                    if (way_it->Address_Arr[j].last_seen_L1 <= smallest_last_seen){
                            smallest_last_seen = way_it->Address_Arr[j].last_seen_L1;
                            smallet_way = i;
                            break; // cause there is no other address in this way that will have the same set
                        } 
                }
            }
            if (cache_level == 2) {
                if (way_it->Address_Arr[j].set_L2 == curr_set){
                    if (way_it->Address_Arr[j].last_seen_L2 <= smallest_last_seen){
                            smallest_last_seen = way_it->Address_Arr[j].last_seen_L2;
                            smallet_way = i;
                            break; // cause there is no other address in this way that will have the same set
                        } 
                }
            }
        }
    }

    // find the right way, now lets remove the address
    if (cache_level == 1) {
        for (auto it = curr_cache->Ways_Arr[smallet_way]->Address_Arr.begin(); it != curr_cache->Ways_Arr[smallet_way]->Address_Arr.end(); ++it) {
            if (it->set_L1 == curr_set) {
                *addr_to_remove = &(*it);
                curr_cache->Ways_Arr[smallet_way]->Address_Arr.erase(it);
                break; // Remove only the first matching element
            }
        }
    } else if (cache_level == 2){
        for (auto it = curr_cache->Ways_Arr[smallet_way]->Address_Arr.begin(); it != curr_cache->Ways_Arr[smallet_way]->Address_Arr.end(); ++it) {
            if (it->set_L2 == curr_set) {
                *addr_to_remove = &(*it);
                curr_cache->Ways_Arr[smallet_way]->Address_Arr.erase(it);
                break; // Remove only the first matching element
            }
        }
    } 

return smallet_way;
}

int remove_specific_address(Cache cache, Address address, int cache_level) {
       Address* curr_address = address.get_address();
    Cache* curr_cache = cache.get_cache();
        unsigned int curr_tag;
        unsigned int curr_set;

        if (cache_level == 1) {// cache_level==1 means we want to find address in L1
            curr_tag = curr_address->tag_L1;
            curr_set = curr_address->set_L1;
        } else if (cache_level == 2) { //
            curr_tag = curr_address->tag_L2;
            curr_set = curr_address->set_L2; // cache_level==2 means we want to find address in L2
        }

        /* iterates through all ways in cache - if there is an address with the same tag AND the same set in one of them - We have a match! return 1. 
            else - return 0,  cause it means the address wasn't found in the cache */ 
        
        for (int i=0; i< curr_cache->Nways; i++) {
            Way* way_it = curr_cache->Ways_Arr[i];

            for (auto it = curr_cache->Ways_Arr[i]->Address_Arr.begin(); it != curr_cache->Ways_Arr[i]->Address_Arr.end(); ++it) {
                if (cache_level == 1) {
                    if (it->set_L1 == curr_set && it->tag_L1 == curr_tag) {
                        curr_cache->Ways_Arr[i]->Address_Arr.erase(it);
                        break; // Remove only the first matching element
                     }
                 }
                if (cache_level == 2) {
                    if (it->set_L2 == curr_set && it->tag_L2 == curr_tag) {
                        curr_cache->Ways_Arr[i]->Address_Arr.erase(it);
                        break; // Remove only the first matching element
                    }
                }
        }
    }
}

int update_timestamp(Cache cache, Address address, int cache_level) {
    Address* curr_address = address.get_address();
    Cache* curr_cache = cache.get_cache();
    if (cache_level == 1) {
        curr_cache->CounterTime++;
        curr_address->last_seen_L1 = curr_cache->CounterTime;
    }

    if (cache_level == 2) {
        curr_cache->CounterTime++;
        curr_address->last_seen_L2 = curr_cache->CounterTime;
    }

}

