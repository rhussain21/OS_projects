
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

using namespace std; 


enum TRANSITION {
	TRANS_TO_READY = 0,
	TRANS_TO_RUN = 1,
	TRANS_TO_BLOCK = 2,
	TRANS_TO_PREEMPT = 3
};

//create enum for prev state 
enum PREV_STATE {
	CREATED = 0,
	READY = 1,
	RUNNING = 2,
	BLOCKED = 3,
	PREEMPTION = 4
};

int CURRENT_TIME = 0;
int ofs;
bool CALL_SCHEDULER = true;


/********  DEFINE PROCESS STRUCT **********/

struct PROCESS {

		int pid; //GIVEN
		int AT; //arrival time (AT)  GIVEN
		int TC; //Total CPU time (TC) GIVEN
		int CB; //CPU burst (CB) GIVEN
		int IO; //IO burst (IO) GIVEN
		int FT; //finishing time (FT)
		int TT ; //Turnaround time (TT)  FT-AT
		int IT; //IT AVG I/O time
		int CW; //CW AVT CPU wait time
		int PRIO; //ASSIGNED LATER
		int timeInPrevState;  
		int timeRemaining; //might not need
		int state_ts; // might not need
	//constructor 
	PROCESS(int id, int tempAT, int tempTC, int tempCB,int tempIO) {
		pid = id;
		AT = tempAT;
		TC = tempTC;
		CB = tempCB;
		IO = tempIO;
	}

};

vector <PROCESS*> prcList; //consider making this a queue 

/******** DEFINE EVENT STRUCT **********/

struct EVENT{
		int pid; 
		int time_stamp;
		int transition;  // new state
		int prev_state; //time in previous state?
		PROCESS* evtPrc;  // Is this needed?

		//constructor

	EVENT(int id, int ts, int newstate, int prevstate) {
		pid = id;
		time_stamp = ts;
		transition = newstate;
		prev_state = prevstate;
		PROCESS* evtPrc;
	}

};	

vector <EVENT*> eventList; //consider making this a queue 
vector <EVENT*> runQueue; 


/****************** PARSE INPUT DATA  ******************/

string output;
void parse_data (FILE* file) {

	char* str2 = (char*)malloc(sizeof(char) * 200);
	char* ret2;
	//string output; 
	
	while(1) {
		ret2 = fgets(str2, 200, file);
		if (ret2 == NULL){
			//cout << "REACHED END OF FILE" << endl;
			break; 
		}
		else if (ret2[0] != '#') {
			//cout << "TEST: THIS STRING DOES NOT HAVE A #" << endl;
			output.append(ret2);
		}
	}

/******************** CREATE PROCESSES ***********************/

	stringstream buf(output);
	string temp;
	int i = 0;
	int tempAT, tempCT, tempCB, tempIB;


	PROCESS* prc ; //= new PROCESS(i, tempAT, tempCT, tempCB, tempIB);
	while (1) {

		getline(buf,temp,'\n');

		stringstream buf2(temp);
		buf2 >> tempAT >> tempCT >> tempCB >> tempIB;

		
		prc = new PROCESS(i, tempAT, tempCT, tempCB, tempIB);
		prcList.push_back(prc);

		i++;

		//sloppy! Re-do this later
		if(buf.rdbuf()->in_avail() == 0) {
			break;
		} 
	}

/******************** GENERATE "CREATED" EVENTS ***********************/
	EVENT* newEvt; // =EVENT(int id, int ts, int newstate, int prevstate)
	for (int i = 0; i < prcList.size(); i++) {

		newEvt = new EVENT(prcList[i]->pid, prcList[i]->AT,0,0); 
		eventList.push_back(newEvt);
	}

}


/****************** PARSE RANDOM AND CREATE RANDOM VALUES ******************/

int randLimit;
int* randvals;

void createRandVals(FILE* rand_file) {

	char* str = (char*)malloc(sizeof(char) * 200); 
	char *ret; 

	ret = fgets(str, 200, rand_file);
	randLimit = stoi(str);
	//set first number to randLimit 
	
	randvals = new int[randLimit];

	for(int i = 0; i < randLimit; i++){
		ret = fgets(str, 200, rand_file);
		randvals[i] = stoi(ret);
	}
}

//create function for randomNumber
int myrandom(int burst) { 

	if (ofs > randLimit) {
		//prevent out of bounds exception
		ofs = 0; 

	}

	int output = (randvals[ofs] % burst)+1;  
	ofs++;
	
	return output;	
}

EVENT* get_event() {

	return eventList.front();
}


//FOR DEBUGGING PURPOSES -- DELETE LATER
bool checkAlphaNumeric(string S) {
	for (int i = 0; i < S.length(); i++) {
		if(!isalnum(S[i])) {
			return 0;
		}
	}
	return 1; 
} 


/********************************  SIMULATOR ***************************************/ 

int cpuBurst, ioBurst ;

void Simulation() {
 	EVENT* evt;
 	EVENT* tempEvt;
 	while( (evt = get_event()) ) {
			PROCESS *proc = evt->evtPrc; // this is the process the event works on
			CURRENT_TIME = evt->time_stamp;
			evt->evtPrc->timeInPrevState = CURRENT_TIME - (proc->state_ts);

			switch(evt->transition) { // which state to transition to?
				case TRANS_TO_READY:
				 	// must come from BLOCKED or from PREEMPTION
					// must add to run queue

					//if event CREATED -> READY, generate new event for run and add to run queue 
					if (evt->prev_state == 0 ){
						tempEvt = new EVENT(proc->pid, CURRENT_TIME, 1,0);
						runQueue.push_back(tempEvt);
					}	
					else if (evt->prev_state == 3) {
						tempEvt = new EVENT(proc->pid, CURRENT_TIME, 1,3);
						runQueue.push_back(tempEvt);
					}
					else if (evt->prev_state == 4) {
						//cpuBurst = myrandom(proc->CB);
						tempEvt = new EVENT(proc->pid, CURRENT_TIME, 1,4);
						runQueue.push_back(tempEvt);
					}

					CALL_SCHEDULER = true; // conditional on whether something is run
					break;


				case TRANS_TO_RUN:
				 	// create event for either preemption or blocking
					//event for block
					cpuBurst = myrandom(proc->CB);
					tempEvt = new EVENT(proc->pid, CURRENT_TIME+cpuBurst,2,2);
					eventList.push_back(tempEvt);
				 	break;

				case TRANS_TO_BLOCK:
					//create an event for when process becomes READY again
					ioBurst = myrandom(proc->IO);
					tempEvt = new EVENT(proc->pid, CURRENT_TIME+ioBurst,0,3);
					eventList.push_back(tempEvt);
					CALL_SCHEDULER = true;
					break;

				case TRANS_TO_PREEMPT:
					// add to runqueue (no event is generated)
					runQueue.push_back(evt);
					CALL_SCHEDULER = true;
					break;
			}


			//remove current event object from Memory
			delete evt;
			evt = nullptr; //maybe delete tempEvt as well?

			/***********  120419 commented for debugging 
			if(CALL_SCHEDULER) {
				if (get_next_event_time() == CURRENT_TIME) {
			 		continue;//process next event from Event queue
				}	

				CALL_SCHEDULER = false; // reset global flag
				if (CURRENT_RUNNING_PROCESS == nullptr) {
			 		CURRENT_RUNNING_PROCESS = THE_SCHEDULER->get_next_process();
			 		if (CURRENT_RUNNING_PROCESS == nullptr)
			 			continue;
			 		// create event to make this process runnable for same time.
			 	}
			}
			**********/
	 }
}


/***************** DEBUGGING PURPOSES ONLY **************************/

// check contents of process list

void printProcess(){
	for (int i = 0; i < prcList.size(); i++) {

		cout << "Pid: " << prcList[i]->pid << endl;
		cout << "AT: " << prcList[i]->AT << endl;
		cout << "TC: " << prcList[i]->TC << endl;
		cout << "CB: " << prcList[i]->CB << endl;
		cout << "IO: " << prcList[i]->IO << endl;

		cout << endl;

	}
}

void printEventList() {
	for (int i = 0; i < eventList.size(); i++) { 
		cout << "Event #: " << i << endl;
		cout << "Event pid: " << eventList[i]->pid << endl;
		cout << "Timestamp:  " << eventList[i]->time_stamp << endl;
		cout << "Transition:  " << eventList[i]->transition << endl;
		cout << endl;
	}
}

void printRunQueue() {
	for (int i = 0; i < runQueue.size(); i++) { 
		cout << "Event #: " << i << endl;
		cout << "Event pid: " << runQueue[i]->pid << endl;
		cout << "Timestamp:  " << runQueue[i]->time_stamp << endl;
		cout << "Transition:  " << runQueue[i]->transition << endl;
		cout << endl;
	}
}

/********************************  START MAIN PROGRAM HERE ***************************************/ 

int main(int argc, char** argv) {

	//cout << "argv[1]: " << argv[1] << endl;
	
	FILE *input_file = fopen(argv[1], "r");

	if (input_file == NULL) {
		cout << "Cannot read the file" << endl;
	}

	FILE *rand_file = fopen(argv[2], "r");
	if (rand_file == NULL) {
		cout << "Cannot read the random file" << endl; 
	}

	//cout << "This is the original text file: " << endl;
	//read_line(input_file);
	//cout << "This is the re-arranged text file: " << endl;
	//test_store_data(input_file);
	parse_data(input_file);
	createRandVals(rand_file);
	printProcess();
	printEventList();
	printRunQueue();
	return 0;
}


