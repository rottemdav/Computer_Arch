#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <cstdint>

//#include "cacheStruct.cpp"

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;

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
		full_address = address_in_binary;

        // createing masks for the address components
        uint64_t  set_mask_L1 = ((1ULL << offset_size+set_size_l1) - 1) ^ ((1ULL << offset_size) - 1);
        uint64_t  set_mask_L2 = ((1ULL << offset_size+set_size_l2) - 1) ^ ((1ULL << offset_size) - 1);
        uint64_t  tag_mask_L1 = ((1ULL<<32)-1) ^ ((1ULL << offset_size+set_size_l1) - 1);
        uint64_t  tag_mask_L2 = ((1ULL<<32)-1) ^ ((1ULL << offset_size+set_size_l2) - 1);

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
		//std::cout << "created new address"  << std::endl;
		//std::cout << "full address:" << address_in_binary << std::endl;

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
        BlockSize(blocksize), Wr_Alloc(wr_alloc), CasheSize(cashesize), Nways(nways), CounterTime(0), Way_Size(way_size), Ways_Arr() {

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
Address* address_exists(Cache& cache, Address& address, int cache_level) {
	cout << "checking if the address exist..." << endl;
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
				printf ("L1 tag: %d, L1 set: %d, curr_tag: %d, curr_set: %d\n", way_it->Address_Arr[j].tag_L1, way_it->Address_Arr[j].set_L1, curr_tag, curr_set);
                if ((way_it->Address_Arr[j].tag_L1 == curr_tag) && (way_it->Address_Arr[j].set_L1 == curr_set) ){
                    return &(way_it->Address_Arr[j]);
                }
            }

            if (cache_level == 2) {
				printf ("L2 tag: %d, L2 set: %d, curr_tag: %d, curr_set: %d\n", way_it->Address_Arr[j].tag_L2, way_it->Address_Arr[j].set_L2, curr_tag, curr_set);
                if ((way_it->Address_Arr[j].tag_L2 == curr_tag) && (way_it->Address_Arr[j].set_L2 == curr_set) ){
                    return &(way_it->Address_Arr[j]);
                }
            }
        }
    }
    return NULL; //if we got out of outer loop it means address is not in any way of cashe
}

int find_free_way(Cache& cache, Address& address, int cache_level){
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

void insert_address(Cache& cache, Address& address, int cache_level, int way_idx){
    Address* curr_address = address.get_address();
    Cache* curr_cache = cache.get_cache();

    if (cache_level == 1) {// insert & update times
        curr_cache->Ways_Arr[way_idx]->Address_Arr.push_back(*curr_address);
        curr_address->in_L1 = true;
    
    } else if (cache_level ==2) { //
        curr_cache->Ways_Arr[way_idx]->Address_Arr.push_back(*curr_address);
        curr_address->in_L2 = true;
		printf("added addres in way: %d, cache level: %d\n", way_idx, cache_level);
    }
}

int remove_address(Cache& cache, Address& address, int cache_level, Address** addr_to_remove){
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
    int smallest_way = -1;

    for (int i=0; i< curr_cache->Nways; i++) {
        Way* way_it = curr_cache->Ways_Arr[i];

        for (size_t j=0; j<way_it->Address_Arr.size(); j++){
            //if the tag of the address in the way found, return 1
            if (cache_level == 1) {
                if (way_it->Address_Arr[j].set_L1 == curr_set){
                    if (way_it->Address_Arr[j].last_seen_L1 <= smallest_last_seen){
                            smallest_last_seen = way_it->Address_Arr[j].last_seen_L1;
                            smallest_way = i;
                            continue; // cause there is no other address in this way that will have the same set
                        } 
                }
            }
            if (cache_level == 2) {
                if (way_it->Address_Arr[j].set_L2 == curr_set){
                    if (way_it->Address_Arr[j].last_seen_L2 <= smallest_last_seen){
                            smallest_last_seen = way_it->Address_Arr[j].last_seen_L2;
                            smallest_way = i;
                            continue; // cause there is no other address in this way that will have the same set
                        } 
                }
            }
        }
    }

	if (smallest_way == -1) {
		return smallest_way;
	}

    // find the right way, now lets remove the address
    if (cache_level == 1) {
        for (auto it = curr_cache->Ways_Arr[smallest_way]->Address_Arr.begin(); 
                  it != curr_cache->Ways_Arr[smallest_way]->Address_Arr.end(); 
                  ++it) {
            if (it->set_L1 == curr_set) {
                *addr_to_remove = &(*it);
                curr_cache->Ways_Arr[smallest_way]->Address_Arr.erase(it);
                break; // Remove only the first matching element
            }
        }
    } else if (cache_level == 2){
        for (auto it = curr_cache->Ways_Arr[smallest_way]->Address_Arr.begin(); it != curr_cache->Ways_Arr[smallest_way]->Address_Arr.end(); ++it) {
            if (it->set_L2 == curr_set) {
                *addr_to_remove = &(*it);
                curr_cache->Ways_Arr[smallest_way]->Address_Arr.erase(it);
                break; // Remove only the first matching element
            }
        }
    } 

return smallest_way;
}

int remove_specific_address(Cache& cache, Address& address, int cache_level) {
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

int update_timestamp(Cache& cache, Address& address, int cache_level) {
    Address* curr_address = address.get_address();
    Cache* curr_cache = cache.get_cache();
    if (cache_level == 1) {
        curr_cache->CounterTime++;
		printf("curr_cache->CounterTime: %d \n", curr_cache->CounterTime);
        curr_address->last_seen_L1 = curr_cache->CounterTime;
    }

    if (cache_level == 2) {
        curr_cache->CounterTime++;
		printf("curr_cache->CounterTime: %d \n", curr_cache->CounterTime);
        curr_address->last_seen_L2 = curr_cache->CounterTime;
    }

}



int main(int argc, char **argv) {

	if (argc < 19) {
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	// Get input arguments

	// File
	// Assuming it is the first argument
	char* fileString = argv[1];
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good()) {
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}

	unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
			L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

	for (int i = 2; i < 19; i += 2) {
		string s(argv[i]);
		if (s == "--mem-cyc") {
			MemCyc = atoi(argv[i + 1]);
		} else if (s == "--bsize") {
			BSize = atoi(argv[i + 1]);
		} else if (s == "--l1-size") {
			L1Size = atoi(argv[i + 1]);
		} else if (s == "--l2-size") {
			L2Size = atoi(argv[i + 1]);
		} else if (s == "--l1-cyc") {
			L1Cyc = atoi(argv[i + 1]);
		} else if (s == "--l2-cyc") {
			L2Cyc = atoi(argv[i + 1]);
		} else if (s == "--l1-assoc") {
			L1Assoc = atoi(argv[i + 1]);
		} else if (s == "--l2-assoc") {
			L2Assoc = atoi(argv[i + 1]);
		} else if (s == "--wr-alloc") {
			WrAlloc = atoi(argv[i + 1]);
		} else {
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}

	/*Calculating sizes of set, tag, offset*/
	int lines_in_l1_log = L1Size - BSize; //number of addresses can be saved in L1 - LOG
	int lines_in_l2_log = L2Size - BSize; //number of addresses can be saved in L2 - LOG

	int set_size_l1 = lines_in_l1_log - L1Assoc; 
	int set_size_l2 = lines_in_l2_log - L2Assoc; 

	int offset_size = BSize;

	int tag_size_l1 = 32-(offset_size + set_size_l1);
	int tag_size_l2 = 32-(offset_size + set_size_l2);

	int way_num_l1 = pow(2,L1Assoc); //number of ways in L1
	int way_num_l2 = pow(2,L2Assoc); //number of ways in L2

	int way_lines_l1 = pow(2,set_size_l1); //number of addresses can be saved in each way in L1
	int way_lines_l2 = pow(2,set_size_l2); //number of addresses can be saved in each way in L2


	/* -------------- create cache and ways -----------------*/
	//   Cache(int blocksize, int wr_alloc, int cashesize, int nways, int way_size) 
	Cache L1(BSize, WrAlloc, L1Size, way_num_l1 , way_lines_l1);
	Cache L2(BSize, WrAlloc, L2Size, way_num_l2 , way_lines_l1);

	int num_access_L1 = 0;
	int num_misses_L1 = 0;
	int num_access_L2 = 0;
	int num_misses_L2 = 0;


	while (getline(file, line)) {

		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)

		// extracting operation from ss to operation and address variables
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}
		cout << " <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< new address >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl;
		// DEBUG - remove this line
		cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		cout << ", address (hex)" << cutAddress;

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		// DEBUG - remove this line
		cout << " (dec) " << num << endl;

		/* creates address class with parsed data */
		Address new_addr(num, offset_size, set_size_l1, set_size_l2, tag_size_l1, tag_size_l2);

		/* checks if the address exists in the cache */
		/* 
		if exist in L1 -> we have a hit. num_access_L1++, continue loop (move to other command)
		if not exist in L1 -> check in L2. num_access_L1++, num_misses_L1++.
		if exist in L2 -> hit L2. num_eccess_l2++, continue loop (move to other command)
		if not exist in L2 -> miss. num_access_L2++, num_misses_L2++. insert address to the cashe:
				if operation==R: bring address to L1 and L2.
				if operation==W: bring address to L1 and L2 only if policy is write allocate
		*/

		if(operation == 'r'){ //new start for chache logic, follow the dependencies chart
			if (address_exists(L1, new_addr, 1)) { // L1 hit
				num_access_L1++;
				printf("L1 hit: num_access_L1: %d\n", num_access_L1 );
				update_timestamp(L1, new_addr, 1);
				continue;
			} 	else if (address_exists(L2, new_addr, 2)) { // L1 miss, L2 hit:
				// if found here - that means that the address WASN'T found in L1, so it counts as a MISS.
				num_access_L1++;
				num_misses_L1++;
				num_access_L2++;
				printf("L1 miss, L2 hit: num_access_L1: %d, num_misses_L1: %d, num_access_L2: %d\n", num_access_L1 , num_misses_L1, num_access_L2);
				update_timestamp(L2, new_addr, 2);
				
				// adding to L1 because every address that was found in L2 must be in L1 also
				int insert_way_l1 = find_free_way(L1, new_addr, 1);
				if (insert_way_l1 != -1) {
					update_timestamp(L1, new_addr, 1);
					insert_address(L1, new_addr, 1, insert_way_l1);
				} else {
					// didn't find any empty way to add the address - need to evict the oldest address in cache
					// dirty bit logic 
					Address* addr_to_remove;
					insert_way_l1 = remove_address(L1, new_addr, 1, &addr_to_remove); // remove oldest address and save the way number it was in.
					Address* addr_to_remove_in_cache = address_exists(L1, *addr_to_remove, 1);
					if (addr_to_remove_in_cache) {
						if (addr_to_remove_in_cache->dirty_bit_L1 == 1) {
							//num_access_L2++;
							Address* addr_to_change_L2 = address_exists(L2, *addr_to_remove, 2);
							addr_to_change_L2->dirty_bit_L2 = 1;
							update_timestamp(L2, *addr_to_change_L2, 2);				
							} 
						}
					update_timestamp(L1, new_addr, 1);
					insert_address(L1, new_addr, 1, insert_way_l1); // add the new address to the way
				}
				// need to update here LRU statistics and add the address to L1 (the whole logic of adding an address and removing it)
				continue;

			} else { // miss in L1 and miss in L2
				num_access_L1++;
				num_misses_L1++;
				num_access_L2++;
				num_misses_L2++;
				printf(" L1 miss, L2 miss: num_access_L1: %d, num_misses_L1: %d, num_access_L2: %d, num_misses_L2: %d\n", num_access_L1 , num_misses_L1, num_access_L2, num_misses_L2);

				// compulsory miss - now need to add the address BOTH to L1 and L2 
				// update in L2
				int insert_way_l2 = find_free_way(L2, new_addr, 2); // is there an empty space in any way in L2
				
				if (insert_way_l2 != -1) { //empty space found
					update_timestamp(L2, new_addr,2);
					insert_address(L2, new_addr, 2, insert_way_l2);

				} else { // need to evict from L2
					Address* addr_to_remove;
					insert_way_l2 = remove_address(L2, new_addr, 2, &addr_to_remove);
					// return the location in L1 of the evicted address and then snoop L1
					Address* addr_to_remove_in_L1 = address_exists(L1, *addr_to_remove, 1);
					if (addr_to_remove_in_L1) { // don't care for dirty bit in this logic - either way - remove the address from L1
						remove_specific_address(L1, *addr_to_remove_in_L1, 1);
					}
					// add new address to L2
					update_timestamp(L2, new_addr,2);
					insert_address(L2, new_addr, 2, insert_way_l2);
				}
				//update in L1
				int insert_way_l1 = find_free_way(L1, new_addr, 1);
				if (insert_way_l1 != -1) { // empty space found
					update_timestamp(L1, new_addr,1);
					insert_address(L1, new_addr, 1, insert_way_l1);
				} else {
					Address* addr_to_remove;
					insert_way_l1 = remove_address(L1, new_addr, 1, &addr_to_remove);
					if (addr_to_remove->dirty_bit_L1 == 1) {
						Address* addr_to_update_in_L2 = address_exists(L2, *addr_to_remove, 2);
						addr_to_update_in_L2->dirty_bit_L2 == 1;
						update_timestamp(L2, *addr_to_update_in_L2,2);
						//num_access_L2++;
					}
					update_timestamp(L1, new_addr,1);
					insert_address(L1, new_addr, 1, insert_way_l1);
				}
			continue;
			}
		} // finish operation == 'R' 

		else if (operation == 'w') {

			Address* addr_to_update_L1  = address_exists(L1, new_addr, 1);
			Address* addr_to_update_L2 = address_exists(L2, new_addr, 2);

			// hit in L1
			if (addr_to_update_L1) {
				num_access_L1++;
				printf("L1 hit: num_access_L1: %d\n", num_access_L1 );
				// mark block as dirty and update LRU
				addr_to_update_L1->dirty_bit_L1 = 1;
				update_timestamp(L1, *addr_to_update_L1, 1);
				// for later: update dirty bit
				continue;

			// miss in L1 and hit in L2
			} 	else if (addr_to_update_L2) {
				// if found here - that means that the address WASN'T found in L1, so it counts as a MISS.
				num_access_L1++;
				num_misses_L1++;
				num_access_L2++;
				printf("L1 miss, L2 hit: num_access_L1: %d, num_misses_L1: %d, num_access_L2: %d\n", num_access_L1 , num_misses_L1, num_access_L2);
				//update_timestamp(L1, new_addr, 1);
				update_timestamp(L2, *addr_to_update_L2, 2);

				if (WrAlloc == 1) {

					int insert_way_l1 = find_free_way(L1, new_addr, 1);
					if (insert_way_l1 != -1) { // found empty space in L1
						new_addr.dirty_bit_L1 = 1;
						update_timestamp(L1, new_addr, 1); 
						insert_address(L1, new_addr, 1, insert_way_l1);
					
					} else { // need to evict in L1
					// didn't find any empty way to add the address - need to evict the oldest address in cache
						Address* addr_to_remove;
						insert_way_l1 = remove_address(L1, new_addr, 1, &addr_to_remove); // remove oldest address and save the way number it was in.
						Address* addr_to_update_L2 = address_exists(L2, *addr_to_remove, 2);
						if (addr_to_remove->dirty_bit_L1 == 1) {
							addr_to_update_L2->dirty_bit_L2 = 1;
							update_timestamp(L2, *addr_to_update_L2, 2);
						}

						new_addr.dirty_bit_L1 = 1;
						update_timestamp(L1, new_addr, 1);
						insert_address(L1, new_addr, 1, insert_way_l1); // add the new address to the way
					}

					// need to update here LRU statistics and add the address to L1 (the whole logic of adding an address and removing it)
					continue;

				} else { // no write-allocate
						addr_to_update_L2->dirty_bit_L2 = 1;
						continue;
					}

			} else { // miss in L1 and miss in L2
				num_access_L1++;
				num_misses_L1++;
				num_access_L2++;
				num_misses_L2++;
				printf("L1 miss, L2 miss: num_access_L1: %d, num_misses_L1: %d, num_access_L2: %d, num_misses_L2: %d\n", num_access_L1 , num_misses_L1, num_access_L2, num_misses_L2);
				if (WrAlloc == 1) {
					// compulsory miss - now need to add the address BOTH to L1 and L2 
					int insert_way_l2 = find_free_way(L2, new_addr, 2); // is there an empty space in any way in L2
					printf("way in L2 that's have empty space: %d\n", insert_way_l2);
					if (insert_way_l2 != -1) { // empty space found
						update_timestamp(L2, new_addr,2);
						insert_address(L2, new_addr, 2, insert_way_l2);
						cout << "added succesuflly to L2" << endl;
					} else {
						Address* addr_to_remove;
						insert_way_l2 = remove_address(L2, new_addr, 2, &addr_to_remove);
						Address* addr_to_update_L1 = address_exists(L1, *addr_to_remove, 1);
						if (addr_to_update_L1) {
							remove_specific_address(L1, *addr_to_update_L1, 1);
						}
						update_timestamp(L2, new_addr,2);
						insert_address(L2, new_addr, 2, insert_way_l2);
						// for later: change address according to dirty bit
					}

					int insert_way_l1 = find_free_way(L1, new_addr, 1);
					printf("way in L1 that's have empty space: %d\n", insert_way_l1);
					if (insert_way_l1 != -1) { // empty space found
						update_timestamp(L1, new_addr, 1);
						insert_address(L1, new_addr, 1, insert_way_l1);
						cout << "added succesuflly to L1" << endl;
					} else {
					// didn't find any empty way to add the address - need to evict the oldest address in cache
						Address* addr_to_remove;	
						insert_way_l1 = remove_address(L1, new_addr, 1, &addr_to_remove); // remove oldest address and save the way number it was in.
						Address* addr_to_update_L2 = address_exists(L2, *addr_to_remove, 2);
						if(addr_to_remove->dirty_bit_L1 == 1) {
							addr_to_update_L2->dirty_bit_L2 = 1;
							update_timestamp(L2, *addr_to_update_L2, 2);
						}
						update_timestamp(L1, new_addr, 1);
						insert_address(L1, new_addr, 1, insert_way_l1); // add the new address to the way
					}
					// need to update here LRU statistics and add the address to L1 (the whole logic of adding an address and removing it)
					continue;
				} else { // no-write-allocate
					continue;
				}
			}
		} // finish opeartion == 'W'
	}
	printf("num_access_L1: %d, num_misses_L1: %d, num_access_L2: %d, num_misses_L2: %d\n", num_access_L1 , num_misses_L1, num_access_L2, num_misses_L2);
	
	double L1MissRate = (num_access_L1 < 1) ? 0 : static_cast<double>(num_misses_L1)/num_access_L1;
	double L2MissRate = (num_access_L2 < 1) ? 0 : static_cast<double>(num_misses_L2)/num_access_L2;
	double avgAccTime = L1Cyc + L1MissRate*L2Cyc + L2MissRate*MemCyc;
	double avgAccTime2 = (1 - L1MissRate) * L1Cyc +  (L1MissRate * (1 - L2MissRate)) * (L1Cyc + L2Cyc) + (L1MissRate * L2MissRate) * (L1Cyc + L2Cyc + MemCyc);

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f", avgAccTime);
	printf("AccTimeAvg2=%.03f\n", avgAccTime2);

	return 0;
}
