

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <cmath>
#include <getopt.h> 
#include <unistd.h> 
#include <stdio.h>
#include <string.h>

using namespace std; 

int main (int argc, char** argv) {

	

	extern char* optarg; 
	char *algoType = NULL;
	int c;
	char in; 
	bool v_flag = false;
	bool f_flag = false;
	bool q_flag = false;

	while ((c = getopt (argc, argv, "vs:")) != -1) {
		switch(c) {
			case 's': {
				algoType = optarg;
				in = algoType[0];
				if (in == 'i') {
					cout << "You've selected i" << endl;
				}
				else if (in == 'j') {
					cout << "You've selected j" << endl; 
				}
				break;
			}
			case 'v': {
				v_flag = true;
				break;
			}
			case '?': {
				if (optopt == 's'){
					cerr<< "Option requires an argument" << endl;
				}
				else if (isprint(optopt)) {
					cerr << "Unknown option! " << endl ; 
				}
				else {
					cerr << "Unknown option character! " << endl;
				}
				return 1; 
			}
			default:
				abort();
		}
	}
	

}