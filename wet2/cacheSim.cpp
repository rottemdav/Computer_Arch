#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include "cacheStruct.cpp"
#include <cmath>

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;

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

		// DEBUG - remove this line
		/*cout << "operation: " << operation;*/

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		/*// DEBUG - remove this line
		cout << ", address (hex)" << cutAddress;*/

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		/*// DEBUG - remove this line
		cout << " (dec) " << num << endl;*/

		/* creates address class with parsed data */
		Address new_addr(num, offset_size, set_size_l1, set_size_l2, tag_size_l1, tag_size_l2);

		/* checks if the address exists in the cache */
		/* 
		if exist in L1 -> we have a hit. num_access_L1++, break loop (move to other command)
		if not exist in L1 -> check in L2. num_access_L1++, num_misses_L1++.
		if exist in L2 -> hit L2. num_eccess_l2++, break loop (move to other command)
		if not exist in L2 -> miss. num_access_L2++, num_misses_L2++. insert address to the cashe:
				if operation==R: bring address to L1 and L2.
				if operation==W: bring address to L1 and L2 only if policy is write allocate
		*/

		if(operation == 'R'){ //new start for chache logic, follow the dependencies chart
			if (address_exists(L1, new_addr, 1) == 1) {
				num_access_L1++;
				update_timestamp(L1, new_addr, 1);
				break;
			} 	else if (address_exists(L2, new_addr, 2) == 1) {
				// if found here - that means that the address WASN'T found in L1, so it counts as a MISS.
					// ------------------------------?? --------------------- how many accesses to L1  --------------------------------- ??-------------------------------
				num_access_L1++;
				num_misses_L1++;
				num_access_L2++;
				//update_timestamp(L1, new_addr, 1);
				update_timestamp(L2, new_addr, 2);
				
				// adding to L1 because every address that was found in L2 must be in L1 also
				int insert_way_l1 = find_free_way(L1, new_addr, 1);
				if (insert_way_l1 != -1) {
					update_timestamp(L1, new_addr, 1);
					insert_address(L1, new_addr, 1, insert_way_l1);
				} else {
					// didn't find any empty way to add the address - need to evict the oldest address in cache
					// ------------------------------?? --------------------- do we need to consider the diry bit --------------------------------- ??-------------------------------
					insert_way_l1 = remove_address(L1, new_addr, 1); // remove oldest address and save the way number it was in.
					update_timestamp(L1, new_addr, 1);
					insert_address(L1, new_addr, 1, insert_way_l1); // add the new address to the way

				}
				// need to update here LRU statistics and add the address to L1 (the whole logic of adding an address and removing it)
				break;
			} else { // miss in L1 and miss in L2
				num_access_L1++;
				num_misses_L1++;
				num_access_L2++;
				num_misses_L2++;

				// compulsory miss - now need to add the address BOTH to L1 and L2 
				int insert_way_l2 = find_free_way(L2, new_addr, 2); // is there an empty space in any way in L2
				if (insert_way_l2 != -1) { //empty space found
					update_timestamp(L2, new_addr,2);
					insert_address(L2, new_addr, 2, insert_way_l2);
				} else {
					insert_way_l2 = remove_address(L2, new_addr, 2);
					update_timestamp(L2, new_addr,2);
					insert_address(L2, new_addr, 2, insert_way_l2);
				}
				int insert_way_l1 = find_free_way(L1, new_addr, 1);
				if (insert_way_l1 != -1) { //empty space found
					update_timestamp(L1, new_addr,1);
					insert_address(L1, new_addr, 1, insert_way_l1);
				} else {
					insert_way_l1 = remove_address(L1, new_addr, 1);
					update_timestamp(L1, new_addr,1);
					insert_address(L1, new_addr, 1, insert_way_l1);
				}
			break;
			}
		} // finish operation == 'R' 

		else if (operation == 'W') {
			if (address_exists(L1, new_addr, 1) == 1) {
				num_access_L1++;
				update_timestamp(L1, new_addr, 1);
				// for later: update dirty bit
				break;
			} 	else if (address_exists(L2, new_addr, 2) == 1) {
				// if found here - that means that the address WASN'T found in L1, so it counts as a MISS.
					// ------------------------------?? --------------------- how many accesses to L1  --------------------------------- ??-------------------------------
				num_access_L1++;
				num_misses_L1++;
				num_access_L2++;
				//update_timestamp(L1, new_addr, 1);
				update_timestamp(L2, new_addr, 2);

				if (WrAlloc == 1) {
					int insert_way_l1 = find_free_way(L1, new_addr, 1);
					if (insert_way_l1 != -1) {
						update_timestamp(L1, new_addr, 1);
						insert_address(L1, new_addr, 1, insert_way_l1);
					} else {
					// didn't find any empty way to add the address - need to evict the oldest address in cache
					// ------------------------------?? --------------------- do we need to consider the diry bit --------------------------------- ??-------------------------------
						insert_way_l1 = remove_address(L1, new_addr, 1); // remove oldest address and save the way number it was in.
						update_timestamp(L1, new_addr, 1);
						insert_address(L1, new_addr, 1, insert_way_l1); // add the new address to the way
					}
					// need to update here LRU statistics and add the address to L1 (the whole logic of adding an address and removing it)
					break;
			} else {
					// for later: update dirty bit
					break;
				}
			} else {
				num_access_L1++;
				num_misses_L1++;
				num_access_L2++;
				num_misses_L2++;
				// wasn't found in BOTH L1 and L2
				if (WrAlloc == 1) {
					// compulsory miss - now need to add the address BOTH to L1 and L2 
					int insert_way_l2 = find_free_way(L2, new_addr, 2); // is there an empty space in any way in L2
					if (insert_way_l2 != -1) { // empty space found
						update_timestamp(L2, new_addr,2);
						insert_address(L2, new_addr, 2, insert_way_l2);
					} else {
						insert_way_l2 = remove_address(L2, new_addr, 2);
						update_timestamp(L2, new_addr,2);
						insert_address(L2, new_addr, 2, insert_way_l2);
						// for later: change address according to dirty bit
					}

					int insert_way_l1 = find_free_way(L1, new_addr, 1);
					if (insert_way_l1 != -1) {
						update_timestamp(L1, new_addr, 1);
						insert_address(L1, new_addr, 1, insert_way_l1);
					} else {
					// didn't find any empty way to add the address - need to evict the oldest address in cache
					// ------------------------------?? --------------------- do we need to consider the diry bit --------------------------------- ??-------------------------------
						insert_way_l1 = remove_address(L1, new_addr, 1); // remove oldest address and save the way number it was in.
						update_timestamp(L1, new_addr, 1);
						insert_address(L1, new_addr, 1, insert_way_l1); // add the new address to the way
					}
					// need to update here LRU statistics and add the address to L1 (the whole logic of adding an address and removing it)
					break;
				}
			}
		} // finish opeartion == 'W'
	}
	
	double L1MissRate = num_misses_L1/num_access_L1;
	double L2MissRate = num_misses_L2/num_access_L2;
	double avgAccTime = L1Cyc + L1MissRate*L2Cyc + L2MissRate*MemCyc;

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}
