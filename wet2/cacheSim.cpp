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
		if (address_exists(L1, new_addr, 1) == 1) {
			num_access_L1++;
			break;
		}
		else if (address_exists(L2, new_addr, 2) == 1) {
			num_access_L1++;
			num_misses_L1++;
			num_access_L2++;
			break;
		}
		else { //miss in both cashes
			num_access_L1++;
			num_misses_L1++;
			num_access_L2++;
			num_misses_L2++;

			// insert address to cache (if needed)
			if (operation == 'R') {
				// insert to L1, L2 in a free way, in index set
				int insert_way_l1 = find_free_way(L1, new_addr, 1);
				int insert_way_l2 = find_free_way(L2, new_addr, 2);
				
				if (insert_way_l1 != -1){ //there is a free way in L1
					insert_address(L1, new_addr, 1, insert_way_l1);
				} else {
					// delete address from L1 by LRU policy, and remmember it's way for new inserting
					insert_way_l1 = remove_address(L1, new_addr, 1);
					insert_address(L1, new_addr, 1, insert_way_l1);
				}

				if (insert_way_l2 != -1){ //there is a free way in L2
					insert_address(L2, new_addr, 2, insert_way_l2);
				} else {
					// delete address from L2 by LRU policy, and remmember it's way for new inserting
					insert_way_l2 = remove_address(L2, new_addr, 2);
					insert_address(L2, new_addr, 2, insert_way_l2);
				}
			} else if (operation == 'W') {
				// insert to L1, L2 in a free way, in index set, ONLY if it's in WrAlloc.
				// IF NO WrAlloc - do nothing.
				if (WrAlloc == 1){
					
				int insert_way_l1 = find_free_way(L1, new_addr, 1);
				int insert_way_l2 = find_free_way(L2, new_addr, 2);
				
				if (insert_way_l1 != -1){ //there is a free way in L1
					insert_address(L1, new_addr, 1, insert_way_l1);
				} else {
					// delete address from L1 by LRU policy, and remmember it's way for new inserting
					insert_way_l1 = remove_address(L1, new_addr, 1);
					insert_address(L1, new_addr, 1, insert_way_l1);
				}

				if (insert_way_l2 != -1){ //there is a free way in L2
					insert_address(L2, new_addr, 2, insert_way_l2);
				} else {
					// delete address from L2 by LRU policy, and remmember it's way for new inserting
					insert_way_l2 = remove_address(L2, new_addr, 2);
					insert_address(L2, new_addr, 2, insert_way_l2);
				}
				}
			}
		}

	}
	
	double L1MissRate = num_misses_L1/num_access_L1;
	double L2MissRate = num_misses_L2/num_access_L2;
	double avgAccTime = L1Cyc + L1MissRate*L2Cyc + L2MissRate*MemCyc;

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}
