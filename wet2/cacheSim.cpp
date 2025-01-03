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
	int lines_in_l1_log = L1Size - BSize;
	int lines_in_l2_log = L2Size - BSize;

	int set_size_l1 = lines_in_l1_log - L1Assoc;
	int set_size_l2 = lines_in_l2_log - L2Assoc;

	int offset_size = BSize;

	int tag_size_l1 = 32-(offset_size + set_size_l1);
	int tag_size_l2 = 32-(offset_size + set_size_l2);

	int way_num_l1 = pow(2,L1Assoc);
	int way_num_l2 = pow(2,L2Assoc);

	int way_lines_l1 = pow(2,set_size_l1);
	int way_lines_l2 = pow(2,set_size_l2);


	/* -------------- create cache and ways -----------------*/
	//   Cache(int blocksize, int wr_alloc, int cashesize, int nways, int way_size) 
	Cache L1(BSize, WrAlloc, L1Size, way_num_l1 , way_lines_l1);
	Cache L2(BSize, WrAlloc, L2Size, way_num_l2 , way_lines_l1);


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
		cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		/*// DEBUG - remove this line
		cout << ", address (hex)" << cutAddress;*/

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		/*// DEBUG - remove this line
		cout << " (dec) " << num << endl;*/

		/* creates address class with parsed data */
		Address new_addr(num, offset_size, set_size_l1, set_size_l2, tag_size_l1, tag_size_l2);

		/* checks if the address exists in any way in L1 */
		if (address_exists(L1, new_addr, 1) == 1) {
			/* if exists in L1 - operate by R or W and calculate */
			if (operation == 'R') { // the access is with read, return hit
				
			} else if (operation == 'W') { // the access is with write, need to consider if write-allocate or NO-write-allocate

		}
		// the address was not found - assign miss and write the address to the matching place in the way array
		} else {

		}

		/* if not exists in L1 -> check if exists in L2 */
		if (address_exists(L2, new_addr, 2)) {
			/* if exists in L2 - operate by R or W and calculate */
			if (operation == 'R') {

			} else if (operation == 'W') {

			}

		} else {
			
		}

	}

	

	

	double L1MissRate;
	double L2MissRate;
	double avgAccTime;

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}
