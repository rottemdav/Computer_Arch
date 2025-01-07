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
    unsigned int Address_bin;
	unsigned int Offset_size;
	unsigned int Set_size;
	unsigned int Tag_size;
    unsigned int Set;
    unsigned int Tag;
    unsigned int Last_seen;
    unsigned int Dirty_bit;

    //constructor 
    Address(unsigned int address_in_binary, unsigned int offset_size, unsigned int set_size,
	 unsigned int tag_size, unsigned int set, unsigned int tag): Address_bin(address_in_binary), 
	 Offset_size(offset_size), Set_size(set_size), Tag_size(tag_size), Set(set), Tag(tag) {
        Last_seen = 0; 
        Dirty_bit = 0;
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

    Way** Ways_Arr; // array of pointers to struct of class Way

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

// some declaration

unsigned int find_set(int address_in_binary, int offset_size, int set_size);
unsigned int find_tag(int address_in_binary, int offset_size, int set_size, int tag_size);
int update_timestamp(Cache& cache, unsigned int set, unsigned int tag, bool D);
int remove_address(Cache& cache, unsigned int set, bool D, int set_size2, int tag_size2, Cache& L2,
bool S, int set_size1, int tag_size1, Cache& L1);


/* returns a pointer to address if found, else returns nullptr */
Address* address_exists(Cache& cache, unsigned int set, unsigned int tag) {
    Cache* curr_cache = cache.get_cache();

    /* iterates through all ways in cache - if there is an address with the same tag AND the same set in one of them - We have a match! return 1. 
        else - return 0,  cause it means the address wasn't found in the cache */ 
    for (int i=0; i< curr_cache->Nways; i++) {
		Way* way_it = curr_cache->Ways_Arr[i];

		for (size_t j=0; j < way_it->Address_Arr.size(); j++) {
			if ((way_it->Address_Arr[j].Set == set) && (way_it->Address_Arr[j].Tag == tag)) {
                    return &(way_it->Address_Arr[j]);
                } 
            }
        }
    return nullptr; //if we got out of outer loop it means address is not in any way of cashe
}

/*gets a cache, and a set, and returns a way with this set free. if there is'nt - return -1*/
int find_free_way(Cache& cache, unsigned int set){
    Cache* curr_cache = cache.get_cache();

    /* iterates through all ways in cache - if there is a free way in the cuur_set index - return the way index in cache. 
        else - return -1,  which means we need to replace an address in a way the cache */ 
    for (int i=0; i< curr_cache->Nways; i++) {
        Way* way_it = curr_cache->Ways_Arr[i];
        bool found_free_way = true;

        for (size_t j=0; j<way_it->Address_Arr.size(); j++){
            //if the tag of the address in the way found, return 1
			if(way_it->Address_Arr[j].Set == set){
				found_free_way = false;
			}
        }
        if(found_free_way){
            return i;
        }
    }
    return -1;
}

/*gets a cache, a set, a tag, a way, and other consts to remmemeber, and insert a new address to cache*/
void insert_address(Cache& cache, unsigned int address_bin, unsigned int offset, unsigned int set_size,
  unsigned int tag_size, unsigned int set, unsigned int tag, int way_idx, bool D){
    Cache* curr_cache = cache.get_cache();
	Address address_to_push = Address(address_bin, offset, set_size, tag_size, set, tag); //creates new Address element
	curr_cache->Ways_Arr[way_idx]->Address_Arr.push_back(address_to_push); //push it to vector
	update_timestamp(cache, set, tag, D);	//update times
}


/*gets a cache, a set, a flag: "D" with some sizes for dirty logic, and a flag "S" with some
sizes for snoop logic.
Finds a victim to remove. remmember the way of the removed victim.
if the victim was dirty - doing dirty logic. 
if the victim is in L2 - need to snoop L1 and removes it if there. removes the victim */
int remove_address(Cache& cache, unsigned int set, bool D, int set_size2, int tag_size2, Cache& L2,
bool S, int set_size1, int tag_size1, Cache& L1){
    // finding a victim using LRU
    Cache* curr_cache = cache.get_cache();
    int smallest_last_seen = curr_cache->CounterTime;
    int smallest_way = -1;
	Address* address_to_remove;

    for (int i=0; i< curr_cache->Nways; i++) {
        Way* way_it = curr_cache->Ways_Arr[i];

        for (size_t j=0; j<way_it->Address_Arr.size(); j++){
                if (way_it->Address_Arr[j].Set == set){
                    if (way_it->Address_Arr[j].Last_seen <= smallest_last_seen){
                            smallest_last_seen = way_it->Address_Arr[j].Last_seen;
                            smallest_way = i;
							address_to_remove = &(way_it->Address_Arr[j]);
                            break; // the inner loop, cause there is no other address in this way with the same set
                        } 
                }
        }
    }
    // after finding the victim, if we need to do "Dirty" logic then do it
	if(D){// Perform dirty logic if needed
		if(address_to_remove->Dirty_bit){
			// dirty logic only relevant when in L2, so find set and tag in l2
			unsigned int set_l2 = find_set(address_to_remove->Address_bin, address_to_remove->Offset_size, set_size2);
			unsigned int tag_l2 = find_tag(address_to_remove->Address_bin, address_to_remove->Offset_size, set_size2, tag_size2);
			// find address in l2, for updating LRU and dirty bit
			update_timestamp(L2, set_l2, tag_l2, D);			
		}
	}

	if(S){// Perform snoop logic if needed
		//find the address to remove in L1, if it's there - remove it
		unsigned int set_l1 = find_set(address_to_remove->Address_bin, address_to_remove->Offset_size, set_size1);
		unsigned int tag_l1 = find_tag(address_to_remove->Address_bin, address_to_remove->Offset_size, set_size1, tag_size1);
		Cache* curr_L1 = L1.get_cache();
		for (int i=0; i< curr_L1->Nways; i++) {
			for (auto it = curr_L1->Ways_Arr[i]->Address_Arr.begin(); it != curr_L1->Ways_Arr[i]->Address_Arr.end(); ++it){
				if ((it->Set == set_l1) && (it->Tag == tag_l1)) {
					curr_L1->Ways_Arr[i]->Address_Arr.erase(it);
					break;
					} 
				}
			}
	}

	 // Remove the victim from the cache
	for (auto it = curr_cache->Ways_Arr[smallest_way]->Address_Arr.begin(); it != curr_cache->Ways_Arr[smallest_way]->Address_Arr.end(); ++it) {
		if (it->Set == set) {
			curr_cache->Ways_Arr[smallest_way]->Address_Arr.erase(it);
			break; // Remove only the first matching element
		}
	}
return smallest_way; // Return the way where the victim was removed
}

/* gets a cache, set and tag, and a flag "D" for dirty, find the address in cache and updates it timestemp. Only for existing addresses*/
int update_timestamp(Cache& cache, unsigned int set, unsigned int tag, bool D) {
    Cache* curr_cache = cache.get_cache();
	Address* address_pointer= address_exists(cache, set, tag);
	// what if address_pointer is a nullptr - probably won't happen
	curr_cache->CounterTime++;
	//printf("curr_cache->CounterTime: %d \n", cache->CounterTime);
	address_pointer->Last_seen = curr_cache->CounterTime;
	if(D){
		address_pointer->Dirty_bit=1;
	}
}

unsigned int find_tag(int address_in_binary, int offset_size, int set_size, int tag_size) {

	// createing masks for the address components
	uint64_t  set_mask = ((1ULL << offset_size+set_size) - 1) ^ ((1ULL << offset_size) - 1);
	uint64_t  tag_mask = ((1ULL<<32)-1) ^ ((1ULL << offset_size+set_size) - 1);
	// extracting the matching componenets and assigning them to the class 
	unsigned int tag = address_in_binary & tag_mask;
	return tag;
}

unsigned int find_set(int address_in_binary, int offset_size, int set_size) {

	// createing masks for the address components
	uint64_t  set_mask = ((1ULL << offset_size+set_size) - 1) ^ ((1ULL << offset_size) - 1);
	// extracting the matching componenets and assigning them to the class 
	unsigned int set = address_in_binary & set_mask;
	return set;
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

	int set_size_l1 = lines_in_l1_log - L1Assoc; // number of addresses in each way - LOG
	int set_size_l2 = lines_in_l2_log - L2Assoc; // number of addresses in each way - LOG 

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
		//cout << " <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< new address >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl;
		// DEBUG - remove this line
		//cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		//cout << ", address (hex)" << cutAddress;

		unsigned long int address_bin = 0;
		address_bin = strtoul(cutAddress.c_str(), NULL, 16);

		// DEBUG - remove this line
		//cout << " (dec) " << address_bin << endl;

		/* Amit changes: instead of create a new address element, work only with address tag and set */
		unsigned int set_l1 = find_set(address_bin, offset_size, set_size_l1);
		unsigned int set_l2 = find_set(address_bin, offset_size, set_size_l2);

		unsigned int tag_l1 = find_tag(address_bin, offset_size, set_size_l1, tag_size_l1);
		unsigned int tag_l2 = find_tag(address_bin, offset_size, set_size_l2, tag_size_l2);
		

		/*Amit Chang: change all the internal functions to work with tag & set, and not with address object.
		also, change all the internal functions not to differ between cache L1 and L2*/

		if(operation == 'r'){ //new start for chache logic, follow the dependencies chart

			if (address_exists(L1, set_l1, tag_l1)) { // L1 hit
				num_access_L1++;
				//printf("L1 hit: num_access_L1: %d\n", num_access_L1 );
				update_timestamp(L1, set_l1, tag_l1, false);
				continue;
			} 	else if (address_exists(L2, set_l2, tag_l2)) { // L1 miss, L2 hit:
				// if found here - that means that the address WASN'T found in L1, so it counts as a MISS.
				num_access_L1++;
				num_misses_L1++;
				num_access_L2++;
				//printf("L1 miss, L2 hit: num_access_L1: %d, num_misses_L1: %d, num_access_L2: %d\n", num_access_L1 , num_misses_L1, num_access_L2);
				update_timestamp(L2, set_l2, tag_l2, false);
				
				// adding to L1 because every address that was found in L2 must be in L1 also
				int insert_way_l1 = find_free_way(L1, set_l1);
				if (insert_way_l1 != -1) {				
					/*Amit change: edited insert_address to add the timestamp inside*/	
					insert_address(L1, address_bin, offset_size, set_size_l1, tag_size_l1, set_l1, tag_l1, insert_way_l1, false);
				} else {
					// didn't find any empty way to add the address - need to evict the oldest address in cache
					// remove the victim and perform a dirty logic
					insert_way_l1 = remove_address(L1, set_l1, true, set_size_l2, tag_size_l2, L2, false, 0, 0, L1); // remove oldest address and save the way number it was in.
					insert_address(L1, address_bin, offset_size, set_size_l1, tag_size_l1, set_l1, tag_l1, insert_way_l1, false); // add the new address to the way
				}
				// need to update here LRU statistics and add the address to L1 (the whole logic of adding an address and removing it)
				continue;

			} else { // miss in L1 and miss in L2
				num_access_L1++;
				num_misses_L1++;
				num_access_L2++;
				num_misses_L2++;
				//printf(" L1 miss, L2 miss: num_access_L1: %d, num_misses_L1: %d, num_access_L2: %d, num_misses_L2: %d\n", num_access_L1 , num_misses_L1, num_access_L2, num_misses_L2);

				// compulsory miss - now need to add the address BOTH to L1 and L2 
				// update in L2
				int insert_way_l2 = find_free_way(L2, set_l2); // is there an empty space in any way in L2
				
				if (insert_way_l2 != -1) { //empty space found
					insert_address(L2, address_bin, offset_size, set_size_l2, tag_size_l2, set_l2, tag_l2, insert_way_l2, false);
				} else { // need to evict from L2, here we don't mind dirty logic, but need to snoop l1
					insert_way_l2 = remove_address(L2, set_l2, false, 0,0,L2, true, set_size_l1, tag_size_l1, L1);
					// after remove an address from L2 & snoop it in L1, we can add the new address to L2
					insert_address(L2, address_bin, offset_size, set_size_l2, tag_size_l2, set_l2, tag_l2, insert_way_l2, false);
				}
				//update in L1
				int insert_way_l1 = find_free_way(L1, set_l1);
				if (insert_way_l1 != -1) { // empty space found
					insert_address(L1, address_bin, offset_size, set_size_l1, tag_size_l1, set_l1, tag_l1, insert_way_l1, false);
				} else { //need to find a victim in L1, and perform dirty logic on it in L2. 
					insert_way_l1 = remove_address(L1, set_l1, true, set_size_l2, tag_size_l2, L2, false, 0,0,L1);
					insert_address(L1, address_bin, offset_size, set_size_l1, tag_size_l1, set_l1, tag_l1, insert_way_l1, false);
				}
			continue;
			}
		} // finish operation == 'R' 

		else if (operation == 'w') {

			//Address* addr_to_update_L1  = address_exists(L1, new_addr, 1);
			//Address* addr_to_update_L2 = address_exists(L2, new_addr, 2);

			// hit in L1
			if (address_exists(L1, set_l1, tag_l1)) {
				num_access_L1++;
				//printf("L1 hit: num_access_L1: %d\n", num_access_L1 );
				// mark block as dirty and update LRU
				update_timestamp(L1, set_l1, tag_l1, true);
				continue;

			// miss in L1 and hit in L2
			} 	else if (address_exists(L2, set_l2, tag_l2)) {
				// if found here - that means that the address WASN'T found in L1, so it counts as a MISS.
				num_access_L1++;
				num_misses_L1++;
				num_access_L2++;
				//printf("L1 miss, L2 hit: num_access_L1: %d, num_misses_L1: %d, num_access_L2: %d\n", num_access_L1 , num_misses_L1, num_access_L2);
				
				/* ---- if WrAlloc==1: dirty in L1. Else: dirty in L2. important for inserting to cache ----*/
				
				if(!WrAlloc){//no need to alloc in L1
					update_timestamp(L2, set_l2, tag_l2, true); // mark as dirty and update times
				}
				else {
					int insert_way_l1 = find_free_way(L1, set_l1);
					if (insert_way_l1 != -1) { // found empty space in L1
						// insert and mark dirty
						insert_address(L1, address_bin, offset_size, set_size_l1, tag_size_l1, set_l1, tag_l1, insert_way_l1, true);
					}
					else { // need to evict in L1 & dirty logic
						insert_way_l1 = remove_address(L1, set_l1, true, set_size_l2, tag_size_l2, L2, false, 0,0,L1);
						insert_address(L1, address_bin, offset_size, set_size_l1, tag_size_l1, set_l1, tag_l1, insert_way_l1, true);
					}
				}
			} else { // miss in L1 and miss in L2
				num_access_L1++;
				num_misses_L1++;
				num_access_L2++;
				num_misses_L2++;
				//printf("L1 miss, L2 miss: num_access_L1: %d, num_misses_L1: %d, num_access_L2: %d, num_misses_L2: %d\n", num_access_L1 , num_misses_L1, num_access_L2, num_misses_L2);
				if(!WrAlloc){ //no need to insert cache because it's writing
					continue;
				}
				else{ //write allocate logic, compulsory miss - now need to add the address BOTH to L1 and L2 
					int insert_way_l2 = find_free_way(L2, set_l2);
					if (insert_way_l2 != -1) { // empty space found
						insert_address(L2, address_bin, offset_size, set_size_l2, tag_size_l2, set_l2, tag_l2, insert_way_l2, false);
					} else{ //need to evict a victim, snoop in L1
						insert_way_l2 = remove_address(L2, set_l2, false, 0,0,L2, true, set_size_l1, tag_size_l1, L1);
						insert_address(L2, address_bin, offset_size, set_size_l2, tag_size_l2, set_l2, tag_l2, insert_way_l2, false);
					}
					int insert_way_l1 = find_free_way(L1, set_l1);
					if (insert_way_l1 != -1) { // empty space found
						insert_address(L1, address_bin, offset_size, set_size_l1, tag_size_l1, set_l1, tag_l1, insert_way_l1, false);
					} else{ //need to avict a victim from L1, dirty logic
						insert_way_l1 = remove_address(L1, set_l1, true, set_size_l2, tag_size_l2, L2, false, 0,0,L1);
						insert_address(L1, address_bin, offset_size, set_size_l1, tag_size_l1, set_l1, tag_l1, insert_way_l1, false);
					}
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
