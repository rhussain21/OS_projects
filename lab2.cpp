
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
	PREEMPTION = 5
};



/********  PROCESS STRUCT **********/

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

	//constructor 
	PROCESS(int id) {
		pid = id;
	}

};

vector <PROCESS*> prcList; //consider making this a queue 

/******** EVENT STRUCT **********/

struct EVENT{
		int pid; 
		int time_stamp;
		int transition;  
		int prev_state;
		PROCESS* evtPrc; 

};	

vector <EVENT*> eventList; //consider making this a queue 

/********  FUNCTION FOR CREATING SUMMARY TABLE			 **********/



/********  CREATE DATA STRUCTURE FOR PROCESSES		 **********/

//declare variables

// need event queue 
// need process queue 
// need ready queue this is part of the scheduler  

//need random number generator -- use what was given in class

class DES_layer() {
	get_event()
} 

void Simulation() {
	EVENT* evt;
	while( (evt = get_event()) ) {
	 	PROCESS *proc = evt->evtProcess; // this is the process the event works on
		CURRENT_TIME = evt->evtTimeStamp;
		evt->evtProcess->timeInPrevState = CURRENT_TIME – proc->state_ts;

		switch(evt->transition) { // which state to transition to?
		case TRANS_TO_READY:
			// must come from BLOCKED or from PREEMPTION
			// must add to run queue
			CALL_SCHEDULER = true; // conditional on whether something is run
			break;
	 	case TRANS_TO_RUN:
	 		// create event for either preemption or blocking
			 break;
	 	case TRANS_TO_BLOCK:
		 	//create an event for when process becomes READY again
			CALL_SCHEDULER = true;
			break;
	 	case TRANS_TO_PREEMPT:
			 // add to runqueue (no event is generated)
			CALL_SCHEDULER = true;
			break;
	 	}
	 	//remove current event object from Memory
	 	delete evt;
		evt = nullptr;

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
	}
}




/*******************************  READ AND PARSE FILE ********************************************/
//UPDATE 100619

// to make sure the file was read 
void read_line (FILE* file) {
	char* str2 = (char*)malloc(sizeof(char) * 200);
	char* ret2;
	while(1) {
		ret2 = fgets(str2, 200, file);
		if (ret2 == NULL){
			cout << "REACHED END OF FILE" << endl;
			return;
		}
		cout << str2 << endl;
	}
}




bool checkAlphaNumeric(string S) {
	for (int i = 0; i < S.length(); i++) {
		if(!isalnum(S[i])) {
			return 0;
		}
	}
	return 1; 
} 


/** USE THIS TO TEST STORING DATA STREAM, PARSING, CONVERTING TO NUMBERS, AND USING TO CREATE PROCESSES **/
void test_store_data (FILE* file) {

	vector <string> inputs;
	//string line = file;

	char* str = (char*)malloc(sizeof(char) * 200) ;
	string example; 
	char *ret; 
	while(1) {
		ret = fgets(str, 200, file);
		if (ret == NULL){
			//cout << "REACHED END OF FILE" << endl;
			break;
		}
		example.append(ret);
	}

	stringstream parser(example); 

	string token; 
	/**** MY OLD WAY OF PARSING.  DELETE THIS 
	while (getline(parser, token, ' ')) {
		stringstream parser1(token); //delete this
		while (getline(parser1, token, '\n')) {  //this is also unnecessary 
				//check alphanumeric 
				if (checkAlphaNumeric(token)) {
					inputs.push_back(token);
				}
		}
	} */

	while (getline(parser, token, ' ')) {
		stringstream parser1(token); //delete this
				for(string s; parser1 >> s; ) {
        		inputs.push_back(s);
    			}
		
	}

	//print to see if it works
	cout << "Size of stringstream: " << inputs.size() << endl;
	for (int i = 0; i < inputs.size(); i++) {
		
		
		//cout << inputs[i] << endl; 
		
		//inputs[i] = stoi(inputs[i]);
	} 
	/*
	int pid = 0; 
	for (int i = 0; i < inputs.size(); i++) {
		if (inputs[i] != '\n') {

		}
	}  */


	//cout << "The # of entries in this vector is: " << inputs.size() << endl; // delete this
	//create vector of processes 

	/* TEST IF PROCESS WORKS 
	PROCESS p1;
	PROCESS p2;
	p1.pid = 1;
	p1.AT = stoi(inputs[0]);
	p1.TC = stoi(inputs[1]);
	p1.CB = stoi(inputs[2]);
	p1.IO = 11105;
	p1.PRIO = 0;

		
	cout << "Pid:  000"<<p1.pid<<":  "<<p1.AT<<"  "<<p1.TC<<"  "<<p1.CB<<"  "<<p1.IO<<"  "<<p1.PRIO<<endl; 

	p1.IO = stoi(inputs[3]);
	cout << "Pid:  000"<<p1.pid<<":  "<<p1.AT<<"  "<<p1.TC<<"  "<<p1.CB<<"  "<<p1.IO<<"  "<<p1.PRIO<<endl; 
	*/
 
 	/************** CREATE PROCESS QUEUE AND POPULATE ******************************/

	//Create vector for each process in the input  i.e., process queue 
	int maxSlots = inputs.size()/4;  
	int numSlots = 0;
	//vector <PROCESS> processQueue; 
	PROCESS processQueue[maxSlots];
	int pidCounter = 0;
	
	double tempPID, tempAT, tempTC, tempCB, tempIO; //might have to initialize these. run code and check later
	for (int i = 1; i <= inputs.size(); i++) {
		//cout << "initiate for loop" << endl; 
		//cout << "TESTING:  i value is now" << i << " and i mod inputs.size() is" << (i % inputs.size())<< endl;
		//assign PID and AT
		if (i % 4 == 1) {
			tempPID = pidCounter;
			tempAT = stoi(inputs[i-1]);
		}
		//Assign TC
		else if (i % 4 == 2) {
			tempTC = stoi(inputs[i-1]);
		} 
		//Assign CB
		else if (i % 4 == 3) {
			tempCB = stoi(inputs[i-1]);
		} 
		//Assign IO
		else if (i % 4 == 0) {
			tempIO = stoi(inputs[i-1]);
			processQueue[numSlots].pid = tempPID;
			processQueue[numSlots].AT = tempAT;
			processQueue[numSlots].TC = tempTC;
			processQueue[numSlots].CB = tempCB; 
			processQueue[numSlots].IO = tempIO; 

			tempAT = tempTC = tempCB = tempIO = 0; 

			//cout << "pid Counter is: " << pidCounter; 
			//cout << "numSlots is: " << pidCounter; 
			pidCounter++;
			numSlots++; 
		}
		
		//cout << "END OF LOOP # " << i << endl << endl;
		//Use inputs to create process
	}

	//cout << "The # of entries in this vector is: " << inputs.size() << endl; 
	//cout << "The # of inputs in this vector are: " << maxSlots << endl;

	for (int i = 0; i < maxSlots; i++) {
		cout << "Pid:  000"<<processQueue[i].pid<<":  "<<processQueue[i].AT<<"  "<<processQueue[i].TC<<"  "<<processQueue[i].CB<<"  "<<processQueue[i].IO<<endl;
	}

	//return inputs; // this doesnt work.  find a way to return vector or maybe cleanup here and return an array.  OR maybe just create processes from here. 
	
	/***************************** END PROCESS QUEUE HERE *******/
}
 
 

/******************************* START EVENT QUEUE HERE ******************************************/

vector <EVENT> eventQueue; 



/*******************************   END EVENT QUEUE HERE ******************************************/

/********************************  START MAIN PROGRAM HERE ***************************************/ 

int main(int argc, char** argv) {

	cout << "argv[1]: " << argv[1] << endl;
	
	FILE *input_file = fopen(argv[1], "r");

	if (input_file == NULL) {
		cout << "Cannot read the file" << endl;
	}

	//cout << "This is the original text file: " << endl;
	//read_line(input_file);
	cout << "This is the re-arranged text file: " << endl;
	test_store_data(input_file);
	return 0;
}


