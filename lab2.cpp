
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>

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
		int timeInPrevState = 0;  
		int timeRemaining; //  TC -  timeInPrevState
		int state_ts; // might not need
		bool completed = false;
	//constructor 
	PROCESS(int id, int tempAT, int tempTC, int tempCB,int tempIO) {
		pid = id;
		AT = tempAT;
		TC = tempTC;
		CB = tempCB;
		IO = tempIO;
		timeRemaining = tempTC;

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

	EVENT(int id, int ts, int newstate, int prevstate, PROCESS* ep) {
		pid = id;
		time_stamp = ts;
		transition = newstate;
		prev_state = prevstate;
		evtPrc = ep;
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
	int prc_indicator = 0; // Indictor for keeping track of next process in FCFS algo

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

		newEvt = new EVENT(prcList[i]->pid, prcList[i]->AT,0,0, prcList[i]); 
		eventList.push_back(newEvt);

		//cout << "TROUBLESHOOT: Confirm that event #" <<i << " has been created." << endl;
	}

	//cout << "EvtList has " << eventList.size() << " elements. " << endl;
}


/****************** PARSE RANDOM AND CREATE RANDOM VALUES ******************/

int randLimit;
int* randvals;

void createRandVals(FILE* rand_file) {

	char* str = (char*)malloc(sizeof(char) * 200); 
	char *ret; 

	ret = fgets(str, 200, rand_file);
	randLimit = stoi(str) -2;

	//set first number to randLimit 
	
	randvals = new int[randLimit];
	

	for(int i = 0; i < randLimit; i++){

		ret = fgets(str, 200, rand_file);
		if (i != 0 && i != 1 ) {
			//ret = fgets(str, 200, rand_file);
			randvals[i-2] = stoi(ret); 
		}
		//cout << "i: " << i << " " <<randvals[i] << 
		//cout << i << ": " << randvals[i]  << endl;
	}

	

}

//create function for randomNumber
int myrandom(int burst) { 

	if (ofs > randLimit) {
		//prevent out of bounds exception
		ofs = 0; 

	}

	//cout << "myrandom has been called. ofs is " << ofs << " burst is " << burst << " randval is " << randvals[ofs] ;
	int output = (randvals[ofs] % burst)+1;  
	//cout << " output: " << output << endl;
	ofs++;
	
	return output;	
}

EVENT* get_event() {
	EVENT* rtnPointer; 
	int tempTime = 10000000;
	int idx; 

	//cout << "Get_event has been called.  Total "<<eventList.size() << " elements." << endl;
	for (int i = 0; i < eventList.size(); i++) {
		//cout << "iterating thru event list (total "<<eventList.size() << " elements) and now at position... " << i << endl;
		//cout << "This event timestamp is " << eventList[i]->time_stamp<< endl;
	//	cout << "@ event #" << i << ": Timestamp: "<< eventList[i]->time_stamp << " Transition: "<< eventList[i]->time_stamp <<endl;
		rtnPointer = eventList[i]; 


		if (rtnPointer->time_stamp < tempTime ) {

			tempTime = rtnPointer->time_stamp ;
			idx = i; 


			//cout << "CURRENT_TIME is " << CURRENT_TIME << " and current event is being selected and deleted from list" << endl;
		//	eventList.erase(eventList.begin()+i);
		//	break; 
		}
		
	}

	rtnPointer = eventList[idx];
	//cout << "Get_event has selected event with position " << idx << " with timestamp " << tempTime << endl;
	eventList.erase(eventList.begin()+idx);

	//cout << "get_event is now complete" << endl;
	return rtnPointer;
}


int get_next_event_time() {

	//cout << "get next event time called" << endl;
	int nextTime ;

	nextTime = eventList.front()->time_stamp;



	return nextTime;

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


//**************** SCHEDULING ALGORITHMS HERE ************************/


class SCHEDULER { 
	public: 
	virtual PROCESS* get_next_process() =0; ;
}; 


class FCFS: public SCHEDULER { 

	//EDGE CASE: Potential fix for below algo.  The processes in input may have different arrival times.
	//May have to arrange by arrival time.  Completed bit might not be okay to have
	/*PROCESS* get_next_process() {
		cout << "get next process is called" << endl;
		PROCESS* newProc; 
		for (int i = 0; i < prcList.size(); i++){
			if (prcList[i]->completed == false) {
				newProc = prcList[i];
				break;
			}
		}	
		return newProc;
	}
 	*/
	PROCESS* get_next_process() {
		
		PROCESS* newProc; 

		newProc = runQueue.front()->evtPrc;

		runQueue.erase(runQueue.begin());
		
		return newProc;
	}


};


SCHEDULER *sched = NULL; 

/********************************  SIMULATOR ***************************************/ 

int cpuBurst, ioBurst ;

void Simulation() {
 	EVENT* evt;
 	EVENT* tempEvt;
 	int simTimer = 0;  
 	int tempTrans ; 
 	string trans_output ; 


 	//cout << "Starting simulator" <<endl;
 //	cout << "EvtList has " << eventList.size() << " elements. " << endl;

 	while( (evt = get_event()) ) {
 			//cout << "SIMULATION TIME: " << simTimer<< endl;
 			if (simTimer > 100 || CURRENT_TIME > 1000) {
 				break;
 			}
 			//cout << "EvtList has " << eventList.size() << " elements. " << endl;

  		
			PROCESS *proc = evt->evtPrc; // this is the process the event works on
			//cout << "Testing.  current process is :" << evt->evtPrc->pid << endl;
			CURRENT_TIME = evt->time_stamp;

			//cout << "CURRENT_TIME is: " << CURRENT_TIME << endl;
			evt->evtPrc->timeInPrevState = CURRENT_TIME - (proc->state_ts);

			switch(evt->transition) { // which state to transition to?
				case TRANS_TO_READY:
				 	// must come from BLOCKED or from PREEMPTION
					// must add to run queue

					//if event CREATED -> READY, generate new event for run and add to run queue 
					if (evt->prev_state == 0 ){
						//cout << "trans to ready scenario 1" << endl ;
						tempEvt = new EVENT(proc->pid, CURRENT_TIME, 1,0, proc);
						eventList.push_back(tempEvt);
						runQueue.push_back(tempEvt);
						proc->state_ts = 0;
						trans_output = "CREATED -> READY";
					}	
					else if (evt->prev_state == 3) {
						//cout << "trans to ready scenario 3" << endl ;

						tempEvt = new EVENT(proc->pid, CURRENT_TIME, 1,3, proc);
						eventList.push_back(tempEvt);
						runQueue.push_back(tempEvt);
						proc->timeInPrevState = proc->state_ts;
						proc->state_ts = 0;
						trans_output = "BLOCK -> READY";
					}	
					else if (evt->prev_state == 4) {
						//cpuBurst = myrandom(proc->CB);
						//cout << "trans to ready scenario 4" << endl ;
						tempEvt = new EVENT(proc->pid, CURRENT_TIME, 1,4, proc);
						eventList.push_back(tempEvt);
						runQueue.push_back(tempEvt);
						proc->state_ts = 0;
					}

					CALL_SCHEDULER = true; // conditional on whether something is run
					tempTrans = 0;
					//cout << "TRANS TO READY COMPLETE" << endl;
					break;


				case TRANS_TO_RUN:
				 	// create event for either preemption or blocking
					//event for block
					cpuBurst = myrandom(proc->CB);
					if (cpuBurst > proc->timeRemaining) {
						cpuBurst = proc->timeRemaining;

					}
					
					tempEvt = new EVENT(proc->pid, CURRENT_TIME+cpuBurst,2,2, proc);
					//cout << "current time + cpu burst is " << CURRENT_TIME+cpuBurst << endl;
					eventList.push_back(tempEvt);
					proc->timeInPrevState = proc->state_ts;  // last burst time is now time in previous state
					proc->timeRemaining = (proc->timeRemaining) - (proc->timeInPrevState);
					proc->state_ts = cpuBurst; // new burst time


					trans_output = "READY -> RUNNG"; 
					tempTrans = 1; 
					//cout << "TRANS TO RUN COMPLETE" << endl;
				 	break;

				case TRANS_TO_BLOCK:
					//create an event for when process becomes READY again
					ioBurst = myrandom(proc->IO);
					if (ioBurst > proc->timeRemaining) {
						ioBurst = proc->timeRemaining;
					}

					tempEvt = new EVENT(proc->pid, CURRENT_TIME+ioBurst,0,3, proc);
					eventList.push_back(tempEvt);
					CALL_SCHEDULER = true;

					proc->timeInPrevState = proc->state_ts;
					proc->timeRemaining = (proc->timeRemaining) - (proc->timeInPrevState);
					proc->state_ts = ioBurst;
					trans_output =  "RUNNG -> BLOCK";
					tempTrans = 2; 
					//cout << "TRANS TO BLOCK COMPLETE" << endl;
					//proc->nullptr;
					break;

				case TRANS_TO_PREEMPT:
					// add to runqueue (no event is generated)
					runQueue.push_back(evt);
					CALL_SCHEDULER = true;
					cout << "TRANS TO PREEMPT COMPLETE" << endl;
					tempTrans = 0; 
					break;
			}

			//cout << "Switching statement complete" << endl;


			//remove current event object from Memory
			delete evt;
			evt = nullptr; //maybe delete tempEvt as well?

			
			if(CALL_SCHEDULER) {

				//cout << "SCHEDULE HAS BEEN CALLED" << endl; 
				if (get_next_event_time() == CURRENT_TIME) {
			 		continue;//process next event from Event queue
				}	

				CALL_SCHEDULER = false; // reset global flag
				if (proc == nullptr) {
			 		proc = sched->get_next_process();
			 		if (proc == nullptr)
			 			continue;
			 		// create event to make this process runnable for same time.
			 	}
			}

			
			simTimer++; //for debugging purposes ONLY
			
			//Consider having bottom before Scheduler is called, in case process is switched
			cout << CURRENT_TIME << " " << proc->pid << " " <<proc->timeInPrevState<< ": "<< trans_output   ; //write code for time spent
			if (tempTrans == 1) {
				cout << " cb=" << cpuBurst << " rem="<< proc->timeRemaining << " prio=1" << endl;
			}
			else if (tempTrans == 2) {
				cout << " ib=" << ioBurst << " rem="<< proc->timeRemaining << endl;
			}
			else {
				cout << endl;
			} 
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

/*
void printEventList() {
	//cout << "PRINT EVENT LIST:  EvtList has " << eventList.size() << " elements. " << endl;
	vector <EVENT*> printQueue = eventList; 
	//cout << "PRINTQUEUE:  printQueue has " << printQueue.size() << " elements. " << endl;
	int max = printQueue.size();
	for (int i = 0; i < max; i++) { 
		cout << "Event #: " << i << endl;
		cout << "Event pid: " << printQueue.front()->pid << endl;
		cout << "Timestamp:  " << printQueue.front()->time_stamp << endl;
		cout << "Transition:  " << printQueue.front()->transition << endl; 
		cout << endl;

		//printQueue.pop();
	} 

	//cout << "Checking if evtList isn't affected: " << eventList.size() << " and " << printQueue.size() << endl;

	//delete printQueue;
	//printQueue = nullptr; 
} */

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
	sched = new FCFS();
	//cout << "This is the original text file: " << endl;
	//read_line(input_file);
	//cout << "This is the re-arranged text file: " << endl;
	//test_store_data(input_file);
	parse_data(input_file);
	createRandVals(rand_file);
	//printProcess();
	//printEventList();
	Simulation();
	
	//printRunQueue();
	return 0;
}


