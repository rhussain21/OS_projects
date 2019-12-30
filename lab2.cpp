
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
int quantum = 10000; //this is provided via getOpt
bool CALL_SCHEDULER = true;
bool prio_flag = false; 
string schedName = "FCFS" ;


/********  DEFINE PROCESS STRUCT **********/

struct PROCESS {

		int pid; //GIVEN
		int AT; //arrival time (AT)  GIVEN
		int TC; //Total CPU time (TC) GIVEN
		int CB; //CPU burst (CB) GIVEN
		int IO; //IO burst (IO) GIVEN
		int FT; //finishing time (FT)
		int TT ; //Turnaround time (TT)  FT-AT
		int IT = 0; //IT AVG I/O time
		int CW = 0; //CW AVT CPU wait time
		int staticPRIO; //ASSIGNED LATER
		int dynamicPRIO;
		int timeInPrevState = 0;  
		int timeRemaining; //  TC -  timeInPrevState
		int state_ts; // might not need
		int currentBurst;

		bool preempted = false;
		bool running = false;
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
	//122119 added constructor with PRIO
	PROCESS(int id, int tempAT, int tempTC, int tempCB,int tempIO, int tempPRIO) {
		pid = id;
		AT = tempAT;
		TC = tempTC;
		CB = tempCB;
		IO = tempIO;
		timeRemaining = tempTC;
		staticPRIO = tempPRIO;
		dynamicPRIO = tempPRIO; 

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

		//constructor w/ timestamp

	EVENT(int id, int ts, int newstate, int prevstate, PROCESS* ep) {
		pid = id;
		time_stamp = ts;
		transition = newstate;
		prev_state = prevstate;
		evtPrc = ep;
	}

	//constructor w/o timestamp

	EVENT(int id, int newstate, int prevstate, PROCESS* ep) {
		pid = id;
		transition = newstate;
		prev_state = prevstate;
		evtPrc = ep;
	}


};	

vector <EVENT*> eventList; //consider making this a queue 
vector <PROCESS*> runQueue; // aka readyQueue or active queue 
vector <PROCESS*> expiredQueue; 
vector <PROCESS*> tempQueue; //to help switch runQueue and expiredQueue

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
	int tempAT, tempCT, tempCB, tempIB, tempPRIO;
	int prc_indicator = 0; // Indictor for keeping track of next process in FCFS algo


	PROCESS* prc ; //= new PROCESS(i, tempAT, tempCT, tempCB, tempIB);
	while (1) {

		getline(buf,temp,'\n');

		stringstream buf2(temp);
		buf2 >> tempAT >> tempCT >> tempCB >> tempIB;

		//tempPRIO = myrandom(4);
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
	randLimit = stoi(str);

	//set first number to randLimit 
	
	randvals = new int[randLimit];
	

	for(int i = 0; i < randLimit; i++){

		ret = fgets(str, 200, rand_file);
	
			//ret = fgets(str, 200, rand_file);
		randvals[i] = stoi(ret); 
		
		//cout << "i: " << i << " " <<randvals[i] << 
		//cout << i << ": " << randvals[i]  << endl;
	}

}

//create function for randomNumber
int myrandom(int burst) { 
	//cout << "ofs: " << ofs << " burst: "<< burst;
	if (ofs > randLimit) {
		//prevent out of bounds exception
		ofs = 0; 

	}

	//cout << "myrandom has been called. ofs is " << ofs << " burst is " << burst << " randval is " << randvals[ofs] ;
	int output = (randvals[ofs] % burst)+1;  
	//cout << " randval: "<< randvals[ofs] << " burst: "<< burst <<" output: " << output << endl;
	ofs++;
	
	return output;	
}

int myPRIO(int burst) {
	if (ofs > randLimit) {
		//prevent out of bounds exception
		ofs = 0; 

	}

	//cout << "myrandom has been called. ofs is " << ofs << " burst is " << burst << " randval is " << randvals[ofs] ;
	int output = (randvals[ofs] % burst);  
	//cout << " randval: "<< randvals[ofs] << " output: " << output << endl;
	ofs++;
	
	return output;	
}


void addPRIO(){
		//TEMPORARY: 122119  populate prio for each of the processes
	for (int i = 0; i < prcList.size(); i++) {
		prcList[i]->staticPRIO = myPRIO(4);
		prcList[i]->dynamicPRIO = prcList[i]->staticPRIO ;
	}
}





EVENT* get_event() {
	EVENT* rtnPointer = NULL; 
	int tempTime = 10000000;
	int idx; 
	/*
	if (CURRENT_TIME >200) {
		cout << "Get_event has been called.  Total "<<eventList.size() << " elements." << endl;
	} */
	//cout << "Get_event has been called.  Total "<<eventList.size() << " elements." << endl;

	if (!(eventList.empty())) {
		for (int i = 0; i < eventList.size(); i++) { /*
			if (CURRENT_TIME > 200) {
				cout << "iterating thru event list (total "<<eventList.size() << " elements) and now at position... " << i << endl;
				cout << "This event timestamp is " << eventList[i]->time_stamp<< endl;
				cout << "@ event #" << i << ": Timestamp: "<< eventList[i]->time_stamp << " Transition: "<< eventList[i]->transition <<endl;
			}	*/
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

	}



	//cout << "get_event is now complete" << endl;
	return rtnPointer;
}


int get_next_event_time() {

	//cout << "get next event time called" << endl;
	int nextTime = 2000000000;

	if(eventList.empty()) { 
		return -1;
	}
	else {
		//nextTime = eventList.front()->time_stamp;
		for (int i = 0; i < eventList.size(); i++) {
			if (nextTime > eventList[i]->time_stamp){
				nextTime = eventList[i]->time_stamp;
			}
		}
		return nextTime;
		//nextTime = eventList.front()->time_stamp;
	}
	

}

bool checkRunningProcess(PROCESS* proc) {
	for (int i = 0; i < prcList.size(); i++) {
		if (prcList[i]->running == true) {
			return true;
		}

	}
	return false;
}


//********************* FOR DEBUGGING PURPOSES ***********************//
bool checkAlphaNumeric(string S) {
	for (int i = 0; i < S.length(); i++) {
		if(!isalnum(S[i])) {
			return 0;
		}
	}
	return 1; 
} 


void checkRunQueue() {

	if (!runQueue.empty()) {
		for (int i = 0; i < runQueue.size(); i++) {
			cout << "process #"<<i<<": " << runQueue[i]->pid << endl;
		} 
	}
	else {
		cout << "Runqueue is empty" << endl;
	}
}


//**************** SCHEDULING ALGORITHMS HERE ************************/


class SCHEDULER { 
	public: 
	virtual PROCESS* get_next_process() =0 ;
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

		newProc = runQueue.front();

		runQueue.erase(runQueue.begin());
		
		return newProc;
	}


};

class LCFS: public SCHEDULER { 

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

		newProc = runQueue.back();

		runQueue.pop_back();
		
		return newProc;
	}


};


class SRTF: public SCHEDULER { 

 
	PROCESS* get_next_process() {
		
		PROCESS* newProc; 

		int shortestTime = 1000000000;
		int idx = 0;

		for (int i = 0; i < runQueue.size(); i++) {

			if (shortestTime > runQueue[i]->timeRemaining){
				shortestTime = runQueue[i]->timeRemaining;
				idx = i;
			} 

		}

		newProc = runQueue[idx];
		runQueue.erase(runQueue.begin()+idx);

		
		return newProc;
	}


};


class RR: public SCHEDULER { 

 
	PROCESS* get_next_process() {

		PROCESS* newProc; 
		cout <<"Initiating RR" << endl;
 
		newProc = runQueue.front();

		runQueue.erase(runQueue.begin());
		
		return newProc;
	}


};


class PRIO: public SCHEDULER { 

 	
	PROCESS* get_next_process() {

		PROCESS* newProc; 
		int tempPRIO = -1; 
		int idx; 


		if (runQueue.empty()) {
			tempQueue = expiredQueue ;
			expiredQueue = runQueue;
			runQueue = tempQueue;

		}
		
		for (int i = 0; i < runQueue.size(); i++) {
			//cout << "At idx " << i << " the dynamicPRIO is " << (runQueue[i]->dynamicPRIO) << " and staticPRIO is " << (runQueue[i]->staticPRIO) << endl;
			if (runQueue[i]->dynamicPRIO > tempPRIO) {
				idx = i; 
				tempPRIO = runQueue[i]->dynamicPRIO;
				
			}
			
			//cout << "runQueue.begin()+idx: " << runQueue[runQueue.begin()+idx]->pid << endl;
			//cout << "idx chosen: " << idx << endl;
			

		}

		newProc = runQueue[idx];
		
		runQueue.erase(runQueue.begin()+idx);

		return newProc;
	}


};

SCHEDULER *sched = NULL; 


void printSummary(){


	cout << schedName << endl;


	for (int i = 0; i < prcList.size(); i++) {
		// pid: AT TC CB IO PRIO | FT TT IT CW

	

		cout << prcList[i]->pid <<":    " << prcList[i]->AT << "   "<< prcList[i]->TC << "   "<< prcList[i]->CB << "   "<< prcList[i]->IO << "   "<< (prcList[i]->staticPRIO)+1;
		cout << "   |   " << prcList[i]->FT << "   "<< prcList[i]->TT << "   "<< prcList[i]->IT << "   "<< prcList[i]->CW << endl;
	}


}

void printEventList() {
	
	for (int j = 0; j < eventList.size(); j++) {
		cout << "Event for PID #" <<eventList[j]->pid << " at timestamp" << eventList[j]->time_stamp << endl;
	} 
} 

void printCPUbursts(){
	for (int j = 0; j < prcList.size(); j++){
		cout << "At PID: "<< prcList[j]->pid << " the CPU burst is curently: " << prcList[j]->currentBurst << endl;
	}
}

/********************************  SIMULATOR ***************************************/ 



void Simulation() {
	int cpuBurst, ioBurst ;
 	EVENT* evt;
 	EVENT* tempEvt; 
 	int tempTrans ; 
 	int tempTime; 
 	int tempPRIO;
 	string trans_output ; 


 	//cout << "Starting simulator" <<endl;
 //	cout << "EvtList has " << eventList.size() << " elements. " << endl;

 	while( (evt = get_event()) ) {
 		//	cout << "Event called" << endl;
 			//cout << "SIMULATION TIME: " << simTimer<< endl;
 		
 			//cout << "EvtList has " << eventList.size() << " elements. " << endl;

  		
			PROCESS *proc = evt->evtPrc; // this is the process the event works on
			//cout << "Testing.  current process is :" << evt->evtPrc->pid << endl;
			CURRENT_TIME = evt->time_stamp;
		//	cout << "CURRENT_TIME: " << CURRENT_TIME << endl;
			//cout << "CURRENT_TIME is: " << CURRENT_TIME << endl;
			evt->evtPrc->timeInPrevState = CURRENT_TIME - (proc->state_ts);

			switch(evt->transition) { // which state to transition to?
				case TRANS_TO_READY:
				 	// must come from BLOCKED or from PREEMPTION
					// must add to run queue

					if (evt->prev_state == 0 ){

						runQueue.push_back(evt->evtPrc);

						proc->state_ts = CURRENT_TIME;
						proc->timeInPrevState = CURRENT_TIME - (proc->state_ts);
						trans_output = "CREATED -> READY";
						tempTrans = 0;
					}	
					else if (evt->prev_state == 3) {
						runQueue.push_back(evt->evtPrc);

						proc->timeInPrevState = CURRENT_TIME - proc->state_ts;
						proc->state_ts = CURRENT_TIME;
						trans_output = "BLOCK -> READY";
						tempTrans = 0;
						
					}	

					CALL_SCHEDULER = true; // conditional on whether something is run
				
					break;


				case TRANS_TO_RUN:
				 	// create event for either preemption or blocking
					//event for block

					proc->running = true; 

					proc->CW = (proc->CW) + proc->timeInPrevState;

					tempPRIO = proc->dynamicPRIO;

					if (!proc->preempted) {   //calculate new cpu burst for processes that were previously blocked, not preempted
						cpuBurst = myrandom(proc->CB);
						proc->currentBurst = cpuBurst;

						if (cpuBurst > proc->timeRemaining) {
							cpuBurst = proc->timeRemaining;
							proc->currentBurst = cpuBurst;
						}	

					}
					else {
						//proc->currentBurst = (proc->currentBurst)-quantum;
						if (proc->currentBurst > proc->timeRemaining) {
							proc->currentBurst = proc->timeRemaining;
							proc->preempted = false;
						}

					}
					

					if ( (proc->currentBurst <= quantum) ){  //proc->timeRemaining <= quantum

						tempTime = proc->timeRemaining;
						proc->timeRemaining = (proc->timeRemaining) - (proc->currentBurst); //change this calculation
						//cout << "Temp time is " << tempTime << " & process time remaining is now " << proc->timeRemaining << endl;
						proc->timeInPrevState = CURRENT_TIME - proc->state_ts;
						proc->state_ts = CURRENT_TIME; // new burst time
						
						tempEvt = new EVENT(proc->pid, CURRENT_TIME+(proc->currentBurst),2,2, proc);
						eventList.push_back(tempEvt);
						proc->preempted = false;

					}
					else {
						tempTime = proc->timeRemaining;
						proc->timeRemaining = (proc->timeRemaining) - quantum;
						//cout << "TEMP time is " << tempTime << " & process time remaining is now " << proc->timeRemaining << endl;
						proc->timeInPrevState = CURRENT_TIME - proc->state_ts;
						proc->state_ts = CURRENT_TIME;

						tempEvt = new EVENT(proc->pid, CURRENT_TIME+quantum,3,2, proc);
						eventList.push_back(tempEvt);
						proc->preempted = true; 
					}
			

					trans_output = "READY -> RUNNG"; 
					tempTrans = 1;
					//cout << "TRANS TO RUN COMPLETE" << endl;
				 	break;

				case TRANS_TO_BLOCK:
					//create an event for when process becomes READY again
					//cout << "Proc: " << proc->pid << " Time Remaining: " << proc->timeRemaining << " Last state ts: " << proc->state_ts << endl;
					proc->running = false;
					proc->timeInPrevState = CURRENT_TIME - proc->state_ts;
					

					if (proc->timeRemaining == 0) {
						proc->completed = true; 
						trans_output =  "Done";
						proc->FT = CURRENT_TIME;
						proc->TT = (proc->FT) - (proc->AT);
						tempTrans = 0; 
						CALL_SCHEDULER = true;
					}
					else {

						ioBurst = myrandom(proc->IO);
						proc->IT = (proc->IT) + ioBurst;
						proc->state_ts = CURRENT_TIME;
						tempEvt = new EVENT(proc->pid, CURRENT_TIME+ioBurst,0,3, proc);
						eventList.push_back(tempEvt);
						proc->dynamicPRIO = proc->staticPRIO; //reset priority
						proc->running = false;
						CALL_SCHEDULER = true;

						trans_output =  "RUNNG -> BLOCK";
						tempTrans = 2; 
					}


					break;

				case TRANS_TO_PREEMPT:
					// add to runqueue (no event is generated)
					//cout << "preemption is occurring " << endl;
					proc->timeInPrevState = CURRENT_TIME - proc->state_ts;
					proc->state_ts = CURRENT_TIME;
					proc->currentBurst = (proc->currentBurst)-quantum;
					//printCPUbursts();
					tempPRIO = evt->evtPrc->dynamicPRIO;
					evt->evtPrc->dynamicPRIO --;  
					//cout << "dynamicPRIO has ben decremented " << endl;
					if (evt->evtPrc->dynamicPRIO == -1 && prio_flag) {
						//cout << "prio below -1" << endl;
						evt->evtPrc->dynamicPRIO = evt->evtPrc->staticPRIO; 
						
						expiredQueue.push_back(evt->evtPrc);
						
					}
					else {
					//	cout <<"pushed to runqeue" << endl;
						runQueue.push_back(evt->evtPrc);
					}

					/*
					if(proc->currentBurst <= quantum) {
						proc->preempted = false;
					}*/
					
					proc->running = false;
					CALL_SCHEDULER = true;
					tempTime = proc->timeRemaining ;
					trans_output = "RUNNG -> READY";
					tempTrans = 1; 
					break;
			}


			//INSERT HERE: check if process is running

			//remove current event object from Memory
			delete evt;
			evt = nullptr; //maybe delete tempEvt as well?
			//Consider having bottom before Scheduler is called, in case process is switched



			cout << CURRENT_TIME << " " << proc->pid << " " <<proc->timeInPrevState<< ": "<< trans_output   ; //write code for time spent
			if (tempTrans == 1 && prio_flag) {
				cout << " cb=" << proc->currentBurst << " rem="<< tempTime << " prio=" << tempPRIO << endl; //" and static: " << proc->staticPRIO<< endl;
			}  
			else if (tempTrans ==1 ) {
				cout << " cb=" << proc->currentBurst << " rem="<< tempTime << " prio="<<proc->staticPRIO << endl;
			} 
			else if (tempTrans == 2) {
				cout << " ib=" << ioBurst << " rem="<< proc->timeRemaining << endl;
			}
			else {
				cout << endl;
			} 
			



			if(CALL_SCHEDULER) {
				cout << "Scheduler called" << endl;

				if (get_next_event_time() == CURRENT_TIME) { 
			 		continue;//process next event from Event queue
				}	

				CALL_SCHEDULER = false; // reset global flag
			
				if (!(checkRunningProcess(proc)) ) {

					if (!runQueue.empty() || !expiredQueue.empty()) {
						cout <<"IF loop condition met" << endl;
				 		proc = sched->get_next_process();

				 		tempEvt = new EVENT(proc->pid, CURRENT_TIME, 1,0, proc);
						eventList.push_back(tempEvt);
						proc->running = true;
					}

			 		if (proc == nullptr)
			 			continue;
			 		// create event to make this process runnable for same time.
			 	}
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
void printRunQueue() {
	for (int i = 0; i < runQueue.size(); i++) { 
		cout << "Event #: " << i << endl;
		cout << "Event pid: " << runQueue[i]->pid << endl;
		cout << "Timestamp:  " << runQueue[i]->time_stamp << endl;
		cout << "Transition:  " << runQueue[i]->transition << endl;
		cout << endl;
	}
} */

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
	sched = new RR();
	quantum = 5;
	//cout << "This is the original text file: " << endl;
	//read_line(input_file);
	//cout << "This is the re-arranged text file: " << endl;
	//test_store_data(input_file);
	parse_data(input_file);
	createRandVals(rand_file);
	addPRIO();
	prio_flag = false;
	//printProcess();
	//printEventList();
	Simulation();
	printSummary();
	
	//printRunQueue();
	return 0;
}


