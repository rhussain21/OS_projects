
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


int head = 0;
int curr_time = 1;
string input;
int next_IO = 0;
int lookDirection = 0 ;
int totalMovement = 0;

//struct for IO

struct IO_REQUEST{
	int IO_op;
	int arrival;
	int disk_start_time;
	int disk_end_time;
	int track;

	//constructor

	IO_REQUEST(int i, int a, int t) {
		IO_op = i;
		arrival = a;
		track = t;
	}

	IO_REQUEST(int a, int t) {
		arrival = a;
		track = t;
	}

	IO_REQUEST() {
		//base constructor
	}

};



queue <IO_REQUEST> RequestList; // list of requests coming from input

vector <IO_REQUEST*> IO_QUEUE; // list of requests fulfilled by Scheduling algo 

vector<IO_REQUEST*> ACTIVE;
vector<IO_REQUEST*> EXPIRED;
vector<IO_REQUEST*> TEMP_Q;

class SCHEDULER {
	//base scheduler class
	public: 
	virtual IO_REQUEST* get_IO () = 0;
	virtual bool emptyQueue() = 0;
};

SCHEDULER* sched = NULL;

class FIFO: public SCHEDULER {
	//cout << "initiating fifo" << endl;
	IO_REQUEST* get_IO() { 
		IO_REQUEST* getNext = IO_QUEUE[next_IO];
		next_IO++;
		return getNext;
	}

	bool emptyQueue() {
		if(IO_QUEUE[next_IO]==NULL) {
			return true;
		}
		else {
			return false;
		}
	}
}; 

class SSTF: public SCHEDULER {

	IO_REQUEST* get_IO() {
		cout << "Initiating get IO (SSTF) "<< endl;
		IO_REQUEST* getNext;
		int distance = 10000; //arbritrary large number
		int x; 
		for (int i = 0; i < IO_QUEUE.size(); i++){
			//check if IO request has been completed 
			if (IO_QUEUE[i]->disk_end_time == 0 ) {
				// of the remaining IO requests, check for shortest track distance from current head
				if ( (abs(head - IO_QUEUE[i]->track)) < distance ) {
					//cout << "The distance value calculated is: " << distance
					distance = abs(head - IO_QUEUE[i]->track);
					x = i; 
				} 
			}
		}
		cout << "FOUND X VALUE:  "<< x << endl;
		getNext = IO_QUEUE[x];
		return getNext;
	}

	//an empty disk_end_time field will initialize to 0.  
	//in this program, it is not possible to end at time 0, so this indicates an incomplete IO request
	bool emptyQueue() {
		for (int i = 0; i < IO_QUEUE.size(); i++) {
			if (IO_QUEUE[i]->disk_end_time == 0) {
				return false; 
			}
		}
		return true;
	}

};


class LOOK: public SCHEDULER { 
	IO_REQUEST* get_IO() {
		int distance = 10000;
		int x = -1;


		do {
			cout << "initiating getIO function" << endl;
			cout << "THe time is now "<< curr_time << endl;
			//x = -1; 
			

				if (lookDirection == 0) {
					cout << "lookDirection is " << lookDirection << endl;
					for (int i = 0; i < IO_QUEUE.size(); i++) {
						if (IO_QUEUE[i]->disk_end_time == 0){
							if ( (abs(head - IO_QUEUE[i]->track)) < distance ) {
								//cout << "The distance value calculated is: " << distance
								distance = abs(head - IO_QUEUE[i]->track);
								x = i;  
							}
						}
					}
					lookDirection = 1; //initial direction of this algorithm is from 0 to higher tracks
				} 

				else if (lookDirection == 1) {
					cout << "lookDirection is " << lookDirection << endl;
					for (int i = 0; i < IO_QUEUE.size(); i++) { 
						//check if going in position of head++ but looking for the closest possible track 
						if (IO_QUEUE[i]->disk_end_time == 0) {
							if ( ((abs(head - IO_QUEUE[i]->track)) < distance) && (IO_QUEUE[i]->track >= head )) {
								distance = abs(head - IO_QUEUE[i]->track);
								x = i;
							}
						}
					}
					if (x == -1) {
						lookDirection = -1; 
					}
				}
				else if (lookDirection == -1) {
					cout << "lookDirection is " << lookDirection << endl;
					for (int i = 0; i < IO_QUEUE.size(); i++) { 
						//check if going in position of head-- but looking for the closest possible track 
						if (IO_QUEUE[i]->disk_end_time == 0){
							if ( ((abs(head - IO_QUEUE[i]->track)) < distance) && (IO_QUEUE[i]->track <= head )) {
								distance = abs(head - IO_QUEUE[i]->track);
								x = i;
							} 
						}
					}
					if (x == -1) {
						lookDirection = 1; 
					}
				}
			

		} while(x == -1);

		IO_REQUEST* getNext = IO_QUEUE[x];

		return getNext;
	}
	bool emptyQueue() {
		for (int i = 0; i < IO_QUEUE.size(); i++) {
			if (IO_QUEUE[i]->disk_end_time == 0) {
				return false; 
			}
		}
		return true;
	}
};

class CLOOK: public SCHEDULER { 
	IO_REQUEST* get_IO() {
		int distance = 10000;
		int x = -1;
		int newHead = 0;
		do {
			

				if (lookDirection == 0) {
					
					for (int i = 0; i < IO_QUEUE.size(); i++) {
						if (IO_QUEUE[i]->disk_end_time == 0){
							if ( (abs(head - IO_QUEUE[i]->track)) < distance ) {
								//cout << "The distance value calculated is: " << distance
								distance = abs(head - IO_QUEUE[i]->track);
								x = i;  
							}
						}
					}
					lookDirection = 1; //initial direction of this algorithm is from 0 to higher tracks
				} 

				else if (lookDirection == 1) {
					
					for (int i = 0; i < IO_QUEUE.size(); i++) { 
						//check if going in position of head++ but looking for the closest possible track 
						if (IO_QUEUE[i]->disk_end_time == 0) {
							if ( ((abs(head - IO_QUEUE[i]->track)) < distance) && (IO_QUEUE[i]->track >= head )) {
								distance = abs(head - IO_QUEUE[i]->track);
								x = i;
							}
						}
					}
					if (x == -1) {
						lookDirection = -1; 
						distance = 10000;

					}
				} 
				else if (lookDirection == -1) {
					
					for (int i = 0; i < IO_QUEUE.size(); i++) { 
						//check if going in position of head++ but looking for the closest possible track 
						if (IO_QUEUE[i]->disk_end_time == 0) {
							if ( ((abs(newHead - IO_QUEUE[i]->track)) < distance) && (IO_QUEUE[i]->track >= newHead )) {
								distance = abs(newHead - IO_QUEUE[i]->track);
								x = i;
							}
						}
					}
				

					lookDirection = 1; 
					newHead = 0;
				} 
				
			

		} while(x == -1);

		IO_REQUEST* getNext = IO_QUEUE[x];

		return getNext;

	}


	bool emptyQueue() {
		for (int i = 0; i < IO_QUEUE.size(); i++) {
			if (IO_QUEUE[i]->disk_end_time == 0) {
				return false; 
			}
		}
		return true;
	}	

};

class FLOOK: public SCHEDULER { 
	IO_REQUEST* get_IO() {
		int distance = 10000;
		int x = -1;
		
		int lastSwitch ; // delete this, only for debugging 

		//swamp queues
		if (ACTIVE.empty()) {
			
			TEMP_Q = EXPIRED;
			EXPIRED.clear();
			ACTIVE = TEMP_Q;
			lastSwitch = curr_time;
		
		}
		
		do {

			if (lookDirection == 0) {

			if (ACTIVE.size() == 1) {
				x = 0;
			}
			else {
				for (int i = 0; i < ACTIVE.size(); i++) {
					if ( (abs(head - ACTIVE[i]->track)) < distance ) {
						//cout << "The distance value calculated is: " << distance
						distance = abs(head - ACTIVE[i]->track);
						x = i;  
					}
				}
			}

			lookDirection = 1; 
		}

		else if (lookDirection == 1) {

			if (ACTIVE.size() == 1) {
				x = 0;
				if (ACTIVE[0]->track < head) {
					lookDirection = -1;
				}
			}
			else {
				for (int i = 0; i < ACTIVE.size(); i++) {
					if (ACTIVE[i]->track == head) {
						x = i;
						break; 
					}

					else if (abs(head - ACTIVE[i]->track) < distance && ACTIVE[i]->track >= head) {
						distance = abs(head - ACTIVE[i]->track);
						x = i;
					}
				}
				if(x == -1) {
					lookDirection = -1;
					
				}
			}

		}

		else if (lookDirection == -1) {

			if (ACTIVE.size() == 1) {
				x = 0;
				if (ACTIVE[0]->track > head) {
					lookDirection = 1;
				}
			}
			else {
				for (int i = 0; i < ACTIVE.size(); i++) {
					if (ACTIVE[i]->track == head) {
						x = i;
						break; 
					}
					else if (abs(head - ACTIVE[i]->track) < distance && ACTIVE[i]->track <= head) {
						distance = abs(head - ACTIVE[i]->track);
						x = i;
					}
				}
				if(x == -1) {
					lookDirection = 1;
					
				}
			}

		}

		} while(x == -1);
	
		//now, match against elements IO_QUEUE which maintains stats info is used by Simulator 

		IO_REQUEST* instRequest = ACTIVE[x];  
		int target; 

		for (int j = 0 ; j < IO_QUEUE.size(); j++ ) {
			if(instRequest->arrival == IO_QUEUE[j]->arrival) {
				target = j;
				break; 
			}
		}

	
		

		//cout << "Item will now be erased: " <<( ACTIVE.begin()+(x-1))->arrival << endl;
	
		ACTIVE.erase(ACTIVE.begin()+x);

		IO_REQUEST* getNext = IO_QUEUE[target];
	
		
		return getNext;
	}

	bool emptyQueue() {

		if (ACTIVE.empty() & EXPIRED.empty()) {
			return true; 
		}

		/*
		else if (ACTIVE.empty() & !EXPIRED.empty()) {
			//this means that active 
			return false; 
		}*/

		else {
	
		return false ; 
		}
	}
};
void read_input(FILE *input_file) {

	char* str = (char*)malloc(sizeof(char) * 200); 
	char *ret; 
	//string output; 
	
	while(1) {
		ret = fgets(str, 4096, input_file);
		if (ret == NULL){
			//cout << "REACHED END OF FILE" << endl;
			break;
		}
		else if (ret[0] != '#') {
			//cout << "TEST: THIS STRING DOES NOT HAVE A #" << endl;
			int InstArrival = atoi(strtok(str, " \t\n"));
			int InstTrack = atoi(strtok(NULL, " \t\n"));
			input.append(ret);
			RequestList.push(IO_REQUEST(InstArrival, InstTrack));
		}
		
	}

}
	
/*
//combine this with the read_input function
void create_IO_List() {
	int InstArrival, InstTrack; 

	stringstream buff(input);
	
	while (1) { 
		char* ret 
		buff  >> InstArrival >> InstTrack ;
		/*
		if (strcmp(buff.c_str(), "") == 0) {
			break;
		}
		
		RequestList.push(IO_REQUEST(InstArrival, InstTrack));	
	} 

} 
*/


void printSummary(vector <IO_REQUEST*> IO_Q) {
	double turnAround = 0;
	double waitTime = 0;
	int maxWaitTime = 0;

	for(int i = 0; i < IO_Q.size(); i++ ) {
		//cout << IO_QUEUE[i]->IO_op <<": " <<IO_QUEUE[i]-> arrival <<"  " << IO_QUEUE[i]->disk_start_time << "  " << IO_QUEUE[i]->disk_end_time << endl;
		turnAround += (IO_Q[i]->disk_end_time) - (IO_Q[i]->arrival);
		waitTime += (IO_Q[i]->disk_start_time) - (IO_Q[i]->arrival) ;

		if (((IO_Q[i]->disk_start_time) - (IO_Q[i]->arrival)) > maxWaitTime) {
			maxWaitTime = (IO_Q[i]->disk_start_time) - (IO_Q[i]->arrival);
		}
		
		printf("%5d: %5d %5d %5d\n", i, IO_Q[i]-> arrival, IO_Q[i]->disk_start_time, IO_Q[i]->disk_end_time);
		
	}

	//print total summary
	printf("SUM: %d %d %.2lf %.2lf %d\n", curr_time, totalMovement, turnAround/IO_Q.size(), waitTime/IO_Q.size(), maxWaitTime);

}

void Simulator(){
	//select scheduler and execute IO requests based on output from read_input 

	bool activeJob = false;  // this really affects the first IO Request.  Could simplify method and remove this
	IO_REQUEST* instIO; 
	IO_REQUEST* currIO = NULL; 
	int jobNum = 0;
	
	bool programRunning = true; 
	//vars for summary line

	while(programRunning) {
		//did a new IO arrive to the system at this time?

		if (RequestList.front().arrival == curr_time) { 
		
			instIO = &RequestList.front();
	
			IO_QUEUE.push_back(instIO);
			EXPIRED.push_back(instIO); //only for use with FLOOK

			RequestList.pop(); 
		}
	 
		if (activeJob) {
		
			//if active job is complete now, compute stats
			if (currIO->track == head) {
				
			 	//record end time in IO Request and set currIO to NULL 
				currIO -> disk_end_time = curr_time; 
				activeJob = false; 
				
				jobNum++; 

				}
			
			
				else { // fix this.

					//move head in direction of the desired track for current job
					if (currIO->track > head ) {
						head++; // check for edge cases here -- what is the range of values for edge 
					}
					else {
						head--;  // check for edge cases here -- what is the range of values for edge 
					}
					totalMovement++;
					curr_time++;
				}
		
			}

			else {
			
				if (!sched->emptyQueue()) {
					currIO = sched->get_IO();
					activeJob = true;
					currIO -> IO_op = jobNum;
					currIO -> disk_start_time = curr_time;
				}
				else {
					curr_time++ ;
				}
			}

		if (!activeJob && sched->emptyQueue() && RequestList.empty()) {
			programRunning = false; 
		}
		
	}

	//print IO Summary Lines 
	printSummary(IO_QUEUE);
	
}



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
					sched = new FIFO();
				}
				else if (in == 'j') {
					sched = new SSTF();
				}
				else if (in == 'c') {
					sched = new CLOOK();
				}
				else if (in == 's') {
					sched = new LOOK();
				}
				else {
					sched = new FLOOK();
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

	FILE *input_file = fopen(argv[optind], "r");
	if (input_file == NULL) {
		cout << "Cannot read the file" << endl;
	}
	//sched = new FLOOK();
	read_input(input_file);
	//create_IO_List();
	Simulator();

}
