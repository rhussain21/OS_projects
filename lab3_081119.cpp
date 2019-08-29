
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <bitset>
#include <cmath>
#include <unistd.h> 

using namespace std; 


/*  EXPLANATION OF CODE: 

******************  DEFINTIONS FOR CONSTANTS, GLOBAL VARIABLES, AND STRUCTS ************************

*/

int read_count = 0;//delete this
int write_count = 0;//delete this

int PTE_SIZE = 64;
int ofs = 0; 
int randLimit; //num of entries in provided rand file.  RANDLIMIT - 1.  POSSIBLY CHANGE THIS LATER
int* randvals; 
int next_frame=0; // to be used by FCFS algo
int hand = 0; // to be used by ClockPage and NRU algos
string instructions;
int MAX_FRAME_SIZE = 16;  // this can change to 16
int counter = 0;
int resetCtr = 0; 
vector <int> time_last_used(MAX_FRAME_SIZE); //change this 
int cost = 0; 
int ctx_switches = 0; 
int process_exits = 0;
bool O_flag = false;
bool P_flag = false;
bool F_flag = false;
bool S_flag = false; 

struct VMA_ENTRY {
	int start_vpage;
	int end_vpage;
	int write_protected; 
	int file_mapped;

	//constructor
	VMA_ENTRY(int insert_start_vpage, int insert_end_vpage, int insert_write_protected, int insert_file_mapped ) {
		start_vpage = insert_start_vpage;
		end_vpage = insert_end_vpage;
		write_protected = insert_write_protected; 
		file_mapped = insert_file_mapped;

	}

};

struct PAGE_TABLE_ENTRY {
 	
 	unsigned int PRESENT_VALID: 1;
 	unsigned int WRITE_PROTECT: 1;
 	unsigned int MODIFIED: 1;
 	unsigned int REFERENCED: 1; 
 	unsigned int PAGEDOUT: 1;
 	unsigned int frameIndex: 7;

 	//constructor
 	PAGE_TABLE_ENTRY () {
 		PRESENT_VALID = 0;
	 	WRITE_PROTECT = 0;
	 	MODIFIED = 0;
	 	REFERENCED = 0; 
	 	PAGEDOUT = 0;
	 	
 	}
};

/*
PAGE_TABLE_ENTRY test;
test.PRESENT_VALID = 0;
*/

struct PROCESS {
	int pid;
	int numVMA; 
	vector <VMA_ENTRY> VMA_List;
	PAGE_TABLE_ENTRY PTE_Table [64]; // change this to global var later

	//constructor 
	PROCESS(int id) {
		pid = id;
	}
};

struct PSTATS {
	unsigned long unmaps =0;
	unsigned long maps=0;
	unsigned long ins=0;
	unsigned long outs=0; 
	unsigned long fins=0; 
	unsigned long fouts=0;
	unsigned long zeros=0;
	unsigned long segv=0;
	unsigned long segprot=0; 


	PSTATS() {
		unmaps =0;
		maps=0;
		ins=0;
		outs=0; 
		fins=0; 
		fouts=0;
		zeros=0;
		segv=0;
		segprot=0; 
	} 

};

vector <PROCESS*> prcList; 
vector <PSTATS*> pstatsList; 

struct FRAME {
	int pid = 0;
	int pageID = 0;  
	int frameNum = 0;
	bitset <32> age;  
	bool available = 0 ; 
	int tau = 0; 
	//indicator for aging?

	//constructor
	FRAME (int num) {
		frameNum = num;
	}

};



vector <FRAME> frame_t;
queue <FRAME*> free_list;
	

void createFrameTable() {
	FRAME* curr_frame;
	for (int i = 0; i < MAX_FRAME_SIZE ; i++) {
		FRAME temp(i);
		frame_t.push_back(temp);
	}

	for (int i = 0; i < MAX_FRAME_SIZE; i++) {
		curr_frame = &frame_t[i];
		free_list.push(curr_frame);
	}

	//optimize this later
}
//vector <FRAME> frame_t(32); 


/***************************** PARSE AND GET RANDOM VALUE FROM RFILE ********************************/


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

	int output = (randvals[ofs] % burst);  
	ofs++;
	
	return output;	
}



	/**************************************************************/
	/*                    READ INITIAL DATA 					  */
	/**************************************************************/

void parse_data (FILE* file) {

	char* str2 = (char*)malloc(sizeof(char) * 200);
	char* ret2;
	string output; 
	
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

	VMA_ENTRY* vma;
	int numOfVMA;
	int temp_start_vpage, temp_end_vpage, temp_write_protect, temp_file_map;
	stringstream buf(output);


	string temp, tempVMA; 
	(getline(buf,temp,'\n'));
	int numProcesses = stoi(temp);

	



	for (int i = 0; i < numProcesses; i++) {

		PROCESS* prc = new PROCESS(i);
		

		//cout <<"TESTING POINT 1" << endl;
		getline(buf,temp,'\n');
		numOfVMA = stoi(temp);

		prc->numVMA = numOfVMA;

		for (int j = 0; j < numOfVMA; j++) {
			
			getline(buf, tempVMA, '\n');

			stringstream buf2(tempVMA);
			buf2 >> temp_start_vpage >> temp_end_vpage >> temp_write_protect >> temp_file_map;

			prc->VMA_List.push_back(VMA_ENTRY(temp_start_vpage, temp_end_vpage, temp_write_protect,temp_file_map));

		}
		
		prcList.push_back(prc);
		pstatsList.push_back(new PSTATS());
	}



	/******************** STORE REMAINING INSTRUCTIONS ***********************/

	//GET INSTRUCTIONS 
	//string instructions declared above; 
	 while (1) { 
	 	//Originally written as buf == NULL
	 	if (buf.rdbuf()->in_avail() == 0) {
	 		//cout << "REACHED END OF INSTRUCTIONS" << endl;
	 		break;
	 	}

	 	getline(buf,temp,'\n'); 
	 	instructions.append(temp+"\n");
	 }

	// cout << "INSTRUCTIONS\n" << instructions << endl; //Testing if the instructions were captured

}



class PAGER { 
	public: 
	virtual FRAME* select_victim_frame() = 0;
}; // base pager class.  other paging algos will inherit from this 



class FCFS: public PAGER {
	//use pointer or queue 
	FRAME* select_victim_frame() {

		FRAME* victim = &frame_t[next_frame];
 		next_frame++;
 		if (next_frame > (MAX_FRAME_SIZE-1)) {
			next_frame = 0;
		}
		return victim;
	}

};

class RandomFrames: public PAGER {
//calling this RandomFrames so there is no conflict with any other pre-set "Random" function in C++

	FRAME* select_victim_frame() {
		int idx = myrandom(MAX_FRAME_SIZE);
		FRAME* victim = &frame_t[idx];

		return victim;
	}
	

};

void showStatus () { //for debugging purposes only 
	cout << "Hand is currently at position " << hand << endl; 
	int tempvpage ; 
	for (int i = 0 ; i < frame_t.size(); i++) {
		cout << "Frame #"<<i << endl; 
		cout << "Frame Age :" << frame_t[i].age << endl;
		cout << "Frame age (in Int) :" << frame_t[i].age.to_ulong() << endl;
		cout << "Frame available :" << frame_t[i].available << endl; 
		cout <<"PID: " <<frame_t[i].pid << endl;
		cout << "Page ID: "<< frame_t[i].pageID << endl;
		tempvpage = frame_t[i].pageID;
		cout << "PTE REFERENCE: " << prcList[frame_t[i].pid]->PTE_Table[tempvpage].REFERENCED << endl;
		cout << "PTE MODIFIED: " << prcList[frame_t[i].pid]->PTE_Table[tempvpage].MODIFIED << endl;
		//cout << "AGE"
		cout << "PTE PAGEDOUT BIT:  " << prcList[frame_t[i].pid]->PTE_Table[tempvpage].PAGEDOUT << endl; 
		cout << endl;
	}
}

//I created this to help debug issue with input11 -- process 1 

void showProcessStatus(int j){
	for (int i = 0; i < 64; i++) {
		cout << "Pid: "<< prcList[j]->pid << " Vpage: " << i <<" PAGEDOUT:  "<< prcList[j]->PTE_Table[i].PAGEDOUT <<endl; 

	}
} 

class ClockPage: public PAGER {
	FRAME* select_victim_frame() {
		//cout << "INITIATE CLOCK ALGO.  Hand is currently at position: "<<hand << endl;
		//showStatus();
		//before we begin
		int temp_pid, temp_vpage;
		while (1) {  

			if (hand > MAX_FRAME_SIZE-1) {
				//cout << "Hand is set to zero" << endl;
				hand = 0;
			}

			temp_pid = frame_t[hand].pid;	
			temp_vpage = frame_t[hand].pageID;

			//cout << "Checking pid "<<temp_pid << " at vpage " << temp_vpage << endl; 
			if (prcList[temp_pid]->PTE_Table[temp_vpage].REFERENCED == 1) {
				prcList[temp_pid]->PTE_Table[temp_vpage].REFERENCED = 0;
			//	cout << "The PTE corresponding to this vpage is referenced." << endl; 
				//cout << "Hand is now set at " << hand << " and it is referencing " << frame_t[hand].frameNum << endl;
				hand++;
			}
			else {
				//cout << "Hand is now set at " << hand << " and it is referencing " << frame_t[hand].frameNum << endl;
			//	cout << "Referenced is zero.  Confirm: " << prcList[temp_pid]->PTE_Table[temp_vpage].REFERENCED << endl;
				FRAME* victim = &frame_t[hand];
				hand++;
				return victim;
			}
		}

	}
};

class NRU: public PAGER {
	FRAME* select_victim_frame() { 
		FRAME* victim;
		

		queue <FRAME*> class0; 
		queue <FRAME*> class1; 
		queue <FRAME*> class2; 
		queue <FRAME*> class3; 

		//TRY THIS.  ADD TIMER.  If TIMER >= 50, then traverse through all frames and clear reference bit 
		int temp_pid, temp_vpage;
		
		for (int i = 0; i < frame_t.size(); i++) {
			if (hand > MAX_FRAME_SIZE-1) {
					//cout << "Hand is set to zero" << endl;
				hand = 0;
			}

			temp_pid = frame_t[hand].pid;	
			temp_vpage = frame_t[hand].pageID;

			//class 0
			if((prcList[temp_pid]->PTE_Table[temp_vpage].REFERENCED == 0)&&(prcList[temp_pid]->PTE_Table[temp_vpage].MODIFIED == 0)){
				class0.push(&frame_t[hand]);
			}
			//class 1
			else if((prcList[temp_pid]->PTE_Table[temp_vpage].REFERENCED == 0)&&(prcList[temp_pid]->PTE_Table[temp_vpage].MODIFIED == 1)){
				class1.push(&frame_t[hand]);
			}
			//class 2
			else if((prcList[temp_pid]->PTE_Table[temp_vpage].REFERENCED == 1)&&(prcList[temp_pid]->PTE_Table[temp_vpage].MODIFIED == 0)){
				class2.push(&frame_t[hand]);
			}
			//class 3
			else if((prcList[temp_pid]->PTE_Table[temp_vpage].REFERENCED == 1)&&(prcList[temp_pid]->PTE_Table[temp_vpage].MODIFIED == 1)){
				class3.push(&frame_t[hand]);
			}
			hand++;
		
		}

		if (!class0.empty()) {
			victim = class0.front();
			hand = (victim->frameNum)+1;
			class0.pop();
		}
		else if (!class1.empty()) {
			victim = class1.front();
			hand = (victim->frameNum)+1;
			class1.pop();
		}
		else if (!class2.empty()) {
			victim = class2.front();
			hand = (victim->frameNum)+1;
			class2.pop();
		}
		else if (!class3.empty()) {
			victim = class3.front();
			hand = (victim->frameNum)+1;
			class3.pop();
		}

		// check if REFERENCED bit needs to be reset 
		//DOUBLECHECK IF THIS BELONGS BEFORE SELECTING VICTIM FRAME
		if (resetCtr >= 50) {
			for (int i = 0 ; i < frame_t.size(); i++) {
				prcList[frame_t[i].pid]->PTE_Table[frame_t[i].pageID].REFERENCED = 0; 
			}
			resetCtr = 0;
		}
		return victim; 
	}
};

class aging: public PAGER {
	FRAME* select_victim_frame() {
		FRAME* victim; 
		int checkpid;
		int checkvpage; 
		double min_age = pow(2,32); // arbritrary large number 
		double checkage; 

		//is there a significance to having a hand here? 
		//cout << "Starting aging algo" << endl; 
		for (int i = 0; i < frame_t.size(); i++) {

			if (hand > MAX_FRAME_SIZE-1) {
				hand = 0;
			}

			checkpid = frame_t[hand].pid; 
			checkvpage = frame_t[hand].pageID; 

			//cout << "Testing 1" << endl; 
			if (prcList[checkpid]->PTE_Table[checkvpage].REFERENCED == 1) {
				//set to 0

				//cout << "Testing 2a" << endl;
				
				//bitset to 1
				//cout << "Testing 3a" << endl;
				frame_t[hand].age = (frame_t[hand].age >> 1);
				frame_t[hand].age = frame_t[hand].age.set(31,1);
				prcList[checkpid]->PTE_Table[checkvpage].REFERENCED = 0;
				//cout << "Testing 4a" << endl;
			}
			else {
				//bitset to 0 
				//cout << "Testing 2b" << endl;
				frame_t[hand].age = (frame_t[hand].age >> 1);
				//cout << "Testing 3b" << endl;
				frame_t[hand].age = frame_t[hand].age.set(31,0);
			//	cout << "Testing 4b" << endl;
			}

		//	cout << "Testing 5" << endl;
			checkage = frame_t[hand].age.to_ulong(); 
			//cout << "Testing 6" << endl;
			//calculate age
			//if (age) < min, victim = &frame_t[hand]
		//	cout << "Currently checking frame # " << frame_t[hand].frameNum << "  age: " << frame_t[hand].age << " check age: "<< checkage << " Min age: "<< min_age<<endl;

			if (checkage < min_age) {
				victim = &frame_t[hand];
				min_age = checkage;
			}
			hand++; //check if hand should be right after the selected frame

	}
		hand = (victim->frameNum)+1; 

		return victim; 
	}
};


class WorkingSet: public PAGER {


	FRAME* select_victim_frame() {

		//cout << "select_victim_frame is called " <<endl; 
		FRAME* victim; 
		int checkpid;
		int checkvpage; 
		//int time_last_used; 
		int current_age; 
		int maxAge = -1; 
		int oldestTau; 

		//cout << "Commencing WorkingSet algo" << endl; 
		for (int i = 0; i < frame_t.size(); i++) {
			//cout << "Counter i is at: " << i << endl << endl;
			if (hand > MAX_FRAME_SIZE-1) {
				hand = 0;
			}

			//cout << "hand is at frame " << hand << endl; 

			checkpid = frame_t[hand].pid; 
			checkvpage = frame_t[hand].pageID; 
			//cout << "hand is currently at " << hand << endl; 
			if ( prcList[checkpid]->PTE_Table[checkvpage].REFERENCED == 1 ) {
				//record current time
				time_last_used[hand] = counter; 
				prcList[checkpid]->PTE_Table[checkvpage].REFERENCED = 0;

			//	cout << "pid "<<checkpid << " at vpage "<<checkvpage<< " is now set to 0.  Age is 0. ";
				current_age = 0; 
				if (current_age > maxAge) {
					maxAge = current_age; 
					victim = &frame_t[hand];
				//	cout << "Selected as victim.  "; 
				}	
				//cout << endl; 			
			}

			else if (prcList[checkpid]->PTE_Table[checkvpage].REFERENCED == 0){
				current_age = counter - time_last_used[hand]; 
				//cout << "pid "<<checkpid << " at vpage "<<checkvpage<< " is already at 0. ";
				//cout << "Current age is " << current_age << ". " << endl; 
				if (current_age >= 50) {
					//cout << "Over 50, so this is selected as victim.  " << endl << endl;
					victim = &frame_t[hand]; 
					hand = (victim->frameNum)+1; 

					return victim;
				}
				else {
					//cout << "Max age is set to " << maxAge << ".  ";
					if (current_age > maxAge) {
						maxAge = current_age; 
						victim = &frame_t[hand];
						//cout << "Victim is set to current frame:  " << frame_t[hand].frameNum << endl << endl; 
						//oldestTau = frame_t[hand].tau; 
					}
					//cout << endl; 	
					/*
					else if (current_age == maxAge) {
						if (frame_t[hand].tau > oldestTau) {
							victim = &frame_t[hand];
							oldestTau = frame_t[hand].tau;
						}
					}		*/	

			}

		}
			hand++;
			//cout << "hand++ is evoked" << endl;
	
		}
		hand = (victim->frameNum)+1; 

		return victim; 
	}
};

PAGER *pager = NULL;

FRAME* get_frame() {

	//cout << "newFrame" << endl;
	FRAME* newFrame;

	//cout << "Checking if free_list is empty" << endl; 
	if(free_list.empty()) {
		//cout << "NO available frames, triggering Algo " << endl;
		//cout << "Algo being invoked" << endl; 
		//cout << "selecting victim frame" << endl;

		newFrame = pager->select_victim_frame();
	}
	else {
		//cout << "popping frame from front" << endl;
		//cout <<"there are "<< free_list.size() <<" available frames." << endl;
		newFrame = free_list.front();
		free_list.pop();
	}
//	cout <<"new frame created" << endl;
	return newFrame;
} 



int is_vpage_valid (int target_page, PROCESS* target_prc) {

	// check if target page is between the start and end pages within each VMA
	for (int i = 0; i < target_prc->numVMA; i++) {

		if ( (target_page >= (target_prc->VMA_List[i].start_vpage)) && (target_page <= (target_prc->VMA_List[i].end_vpage))) {
			return i;
		} 
			
	}

	return -1;
}

	
bool is_PTE_Valid(int vpage, PROCESS* curr_prc) {
	if (curr_prc->PTE_Table[vpage].PRESENT_VALID == 1){
		return true;
	}
	
	return false;	
}


bool is_file_mapped(int vpage, PROCESS* curr_prc) {

	for (int i = 0; i < curr_prc -> numVMA; i++) {
		if ( (vpage >= (curr_prc->VMA_List[i].start_vpage)) && (vpage <= curr_prc->VMA_List[i].end_vpage)) {
			if(curr_prc->VMA_List[i].file_mapped == 1) {
				return true;
			}
		}
	}

	return false; 	

}
 
bool is_write_protected(int vpage, PROCESS* curr_prc) {
	for (int i = 0; i < curr_prc -> numVMA; i++) {
		if ( (vpage >= (curr_prc->VMA_List[i].start_vpage)) && (vpage <= curr_prc->VMA_List[i].end_vpage)) {
			if(curr_prc->VMA_List[i].write_protected == 1) {
				return true;
			}
		}
	}

	return false; 
}


void Simulator () {

	//pstatsList.reserve(prcList.size());
	//cout << "INITIATING SIMULATOR " << endl;

	//cout << instructions << endl;
	createFrameTable();
	PROCESS* curr_prc;
	string temp;  
	int vpage; 
	PAGE_TABLE_ENTRY* oldPTE; 

	
	stringstream buf2(instructions);
	char instType; // temp variable used to parse the next instruction
	int instPage;  // temp variable used to parse the vpage that corresponds to next instruction
	int valid_vpage; // if instPage exists within a VMA in the curr prc, this is used to go into correct VMA Entry for modification
	FRAME* instFrame; 
	bool programRunning = true; 


	while (programRunning) {

		getline(buf2, temp, '\n');

		if (buf2.rdbuf()->in_avail() == 0) {
			//cout << "no more lines left!" << endl;
			programRunning = false;
		}

		instType = temp[0];
		instPage = atoi(temp.c_str() + 2);

		resetCtr++; 
		
		//revision 071319
		if (O_flag){
			cout << counter <<": ==> "<<instType << " " <<instPage << endl; 
		}	
		

		switch(instType) {
				case 'c':
				 	ctx_switches++;
					curr_prc = prcList[instPage];
					cost = cost + 121;
					break;
				case 'e':
					//showStatus();
					//cout << "INSTRUCTION E at "<< instPage  << endl; //Re-visit this soon  
					process_exits++;

					if (O_flag) {
						cout << "EXIT current process " <<curr_prc->pid << endl; 
					}

						//need to re-do based on instruction
						//FRAME SHOULD BE USED AGAIN IN THE ORDER THEY WERE RELEASED
						for (int i = 0; i < 64; i++) {
							if (curr_prc->PTE_Table[i].PRESENT_VALID == 1) {
								pstatsList[curr_prc->pid]->unmaps++;
								cost = cost + 400;

								//showProcessStatus(curr_prc->pid);
								//revision 071319
								if (O_flag){
									cout << " UNMAP " << curr_prc->pid << ":" << i << endl;
								}
								/****** ADDED 080119 START *******/
								if (curr_prc->PTE_Table[i].MODIFIED == 1) {
									if(is_file_mapped(i, prcList[curr_prc->pid])) {
										pstatsList[curr_prc->pid]->fouts++;
										cost = cost + 2500;
										//revision 071319
										if (O_flag){
											cout << " FOUT" << endl;
										}
									}
								}
	
								/******* ADDED 080119 END *******/
								curr_prc->PTE_Table[i].PRESENT_VALID = 0;
							 	curr_prc->PTE_Table[i].WRITE_PROTECT = 0;
							 	//curr_prc->PTE_Table[i].MODIFIED = 0;
							 	curr_prc->PTE_Table[i].REFERENCED = 0; 
							 	//if ( curr_prc->PTE_Table[i].MODIFIED == 1) {
								//curr_prc->PTE_Table[i].PAGEDOUT = 0;
								curr_prc->PTE_Table[i].MODIFIED = 0;

									/*
									if(is_file_mapped(i, curr_prc)) {
										cout << " FOUT" << endl;
									} 
									else {
										cout << " OUT" << endl;
									}*/
								
								//}
							//	cout << "Success. PAGEDOUT: "<< curr_prc->PTE_Table[i].PAGEDOUT << endl ;
							 	//search frame
							 	
							 	for (int j = 0; j <frame_t.size(); j++) {
							 		//cout << "counter i "<< i << " j = " << j << endl; 
							 		//cout << "At PTE #"<< i <<" and frame #"<<j << ": pageID"<< frame_t[j].pageID << endl;
							 		if(frame_t[j].pageID == i && frame_t[j].pid == curr_prc->pid) {
							 			frame_t[j].pid = 0;
										frame_t[j].pageID = 0;
										frame_t[j].available = 0; 
										frame_t[j].age = 0; 
										free_list.push(&frame_t[j]);
									//	cout << "Frame #"<<frame_t[j].frameNum<<" has been released.  # of free frames is " << free_list.size() << endl; 
							 		}
							 	} 
							 }

							 curr_prc->PTE_Table[i].PAGEDOUT = 0;
						}

						cost = cost + 175; 

						//showProcessStatus(curr_prc->pid);

					break;
				case 'r': //read instruction
					//check if vpage is valid
					//question for later.  if vpage is not valid, does read/write still count towards cost?
					
					//cout << "Instruction is read" << endl; 
					cost = cost + 1;
					valid_vpage = is_vpage_valid(instPage, curr_prc);

					if (valid_vpage < 0) {
						//segmentation fault
						pstatsList[curr_prc->pid]->segv++;
						cost = cost +240; 
						//revision 071319
						if (O_flag) {
							cout << " SEGV" << endl;
						}
					}
					
					else {
					
						//check if PTE is already in use
						if (!(is_PTE_Valid(instPage, curr_prc))) {
							//cout << "Commencing get_frame " << endl; 
							//get next available frame and proceed with mapping
							
								//pager = new WorkingSet(); // DELETE THIS 081119
							
							instFrame = get_frame();
							//cout << "GET_FRAME YIELD FRAME # " << instFrame->frameNum << endl;
							//csout << "PAGE ID " <<instFrame->pageID << endl;
							//cout << "get_frame() executed" << endl;
							//EDGE CASE: if previous PTE is at vpage 0 
							//check if instFrame is currently mapped

							oldPTE = &(prcList[instFrame->pid]->PTE_Table[instFrame->pageID]);
							//cout << "instFrame is " << instFrame-?
							//cout <<"OLD PTE PRESENT_VALID bit: " << oldPTE -> PRESENT_VALID << endl;
							if( (instFrame->available)  && oldPTE->PRESENT_VALID == 1) { // BEGIN UNMAP OPERATION
								//Unmap operation first and check if frame is linked to a PTE being modified
								pstatsList[instFrame->pid]->unmaps++;
								cost = cost + 400;
								//revision 071319
								if (O_flag) {
									cout << " UNMAP " << instFrame->pid << ":" << instFrame->pageID << endl;
								}	
								if ( oldPTE->MODIFIED == 1) {

									//oldPTE->PAGEDOUT = 1;
									if(is_file_mapped(instFrame -> pageID, prcList[instFrame -> pid])) {
										pstatsList[instFrame->pid]->fouts++;
										cost = cost + 2500;

										//revision 071319
										if (O_flag){
											cout << " FOUT" << endl;
										}
									} 
									else {
										oldPTE->PAGEDOUT = 1;
										pstatsList[instFrame->pid]->outs++;
										cost = cost + 3000;
										//revision 071319
										if (O_flag){
											cout << " OUT" << endl;
										}
									}
								}
								
								//ummapping former page table and recording pageout
								oldPTE->PRESENT_VALID = 0;
							 	oldPTE->WRITE_PROTECT = 0;
							 	oldPTE->MODIFIED = 0;
							 	oldPTE->REFERENCED = 0; 
							 	//oldPTE->PAGEDOUT = 1;
							 	oldPTE->frameIndex = 0;

							 	//unmapping victim frame
								instFrame->pid = 0;
								instFrame->pageID = 0;
								instFrame->available = 0;   //double-check this
							}// END UNMAP OPERATION
							 
							// BEGIN MAP OPERATION

							curr_prc->PTE_Table[instPage].PRESENT_VALID = 1; 
							//cout << "debug: THIS SHOULD BE 0: "<<curr_prc->PTE_Table[instPage].REFERENCED << endl;
							curr_prc->PTE_Table[instPage].REFERENCED = 1; 
							
							//cout << "debug: instPage is "<< instPage << endl;
							//cout << "debug: currenlty mapping frame "<< instFrame->frameNum << endl;
							//cout << "debug: checking if REFERENCE BIT WAS SET: "<<curr_prc->PTE_Table[instPage].REFERENCED << endl; 
							
							curr_prc->PTE_Table[instPage].frameIndex = instFrame->frameNum ;//set frame index

							//cout << "Showing status now: " << endl; 
							//showStatus();

							instFrame->pid = curr_prc -> pid;
							instFrame->pageID = instPage;
							instFrame->available = 1; 
							instFrame->age = 0;
							instFrame->tau = counter; 

							if(is_write_protected(instPage, curr_prc)) {
								curr_prc->PTE_Table[instPage].WRITE_PROTECT = 1;

							}

							if (is_file_mapped(instPage, curr_prc)) {
								pstatsList[curr_prc->pid]->fins++;
								cost = cost + 2500;
								//revision 071319
								if (O_flag){
									cout << " FIN" << endl;
								}

							}
							else { 

								if(curr_prc->PTE_Table[instPage].PAGEDOUT) {
									pstatsList[curr_prc->pid]->ins++;
									cost = cost + 3000;
									//revision 071319
									if (O_flag){
										cout << " IN" << endl;
									}
								}
								else {
								//	cout << "this passes.  already checked zeroed"<< endl;
									//cout << "pstats zeros is currently at" << pstatsList.size() << endl; 
									pstatsList[curr_prc->pid]->zeros++;
									cost = cost + 150; 
									//revision 071319
									if (O_flag){
										cout << " ZERO" << endl;
									}
								}
							}
							pstatsList[curr_prc->pid]->maps++;
							cost = cost + 400;
							//revision 071319
							if (O_flag){
								cout << " MAP " << instFrame->frameNum << endl;
							}
						}

						else {
							curr_prc->PTE_Table[instPage].REFERENCED = 1;
							
						}


					}
					break;

				case 'w': // write operation
					cost = cost +1;
					valid_vpage = is_vpage_valid(instPage, curr_prc);
					if (valid_vpage < 0) {
						//segmentation fault
						pstatsList[curr_prc->pid]->segv++;
						cost = cost +240; 
						//revision 071319
						if (O_flag){
							cout << " SEGV" << endl;
						}
					}
					else {

						
						if (!(is_PTE_Valid(instPage, curr_prc))) {
							
							instFrame = get_frame();

							oldPTE = &(prcList[instFrame->pid]->PTE_Table[instFrame->pageID]);

							if( (instFrame->available)  && oldPTE->PRESENT_VALID == 1) { // BEGIN UNMAP OPERATION
									//Unmap operation first and check if frame is linked to a PTE being modified
									pstatsList[instFrame->pid]->unmaps++;
									cost = cost + 400;
									//revision 071319
									if (O_flag){
										cout << " UNMAP " << instFrame->pid << ":" << instFrame->pageID << endl;
									}
									if ( oldPTE->MODIFIED == 1) {

										//oldPTE->PAGEDOUT = 1;
										if(is_file_mapped(instFrame -> pageID, prcList[instFrame -> pid])) {
											pstatsList[instFrame->pid]->fouts++;
											cost = cost + 2500;
											//revision 071319
											if (O_flag){
												cout << " FOUT" << endl;
											}
										} 
										else {
											oldPTE->PAGEDOUT = 1;
											pstatsList[instFrame->pid]->outs++;
											cost = cost + 3000;
											//revision 071319
											if (O_flag){
												cout << " OUT" << endl;
											}
										}
									}
									
									//ummapping former page table contents and recording pageout
									oldPTE->PRESENT_VALID = 0;
								 	oldPTE->WRITE_PROTECT = 0;
								 	oldPTE->MODIFIED = 0;
								 	oldPTE->REFERENCED = 0; 
								 	//oldPTE->PAGEDOUT = 1;
								 	oldPTE->frameIndex = 0;

								 	//unmapping victim frame
									instFrame->pid = 0;
									instFrame->pageID = 0;
									instFrame->available = 0; 
							}

							//BEGIN MAPPING OPERATION

								// BEGIN MAP OPERATION
						
								curr_prc->PTE_Table[instPage].PRESENT_VALID = 1; 
								curr_prc->PTE_Table[instPage].REFERENCED = 1; 
								curr_prc->PTE_Table[instPage].frameIndex = instFrame->frameNum ;//set frame index


								instFrame->pid = curr_prc -> pid;
								instFrame->pageID = instPage;
								instFrame->available = 1;
								instFrame->age = 0;
								instFrame->tau = counter; 


								if (is_file_mapped(instPage, curr_prc)) {
									pstatsList[curr_prc->pid]->fins++;
									cost = cost + 2500;
									//revision 071319
									if (O_flag){
										cout << " FIN" << endl;
									}
								}
								else { 
									if(curr_prc->PTE_Table[instPage].PAGEDOUT) {
										pstatsList[curr_prc->pid]->ins++;
										cost = cost + 3000;
										//revision 071319
										if (O_flag){
											cout << " IN" << endl;
										}
									}
									else {
										pstatsList[curr_prc->pid]->zeros++;
										cost = cost + 150; 
										//revision 071319
										if (O_flag){
											cout << " ZERO" << endl;
										}
									}
								}
								pstatsList[curr_prc->pid]->maps++;
								cost = cost + 400;
								// revision 071319
								if (O_flag){
									cout << " MAP " << instFrame->frameNum << endl;
								}


								if(is_write_protected(instPage, curr_prc)) {
									curr_prc->PTE_Table[instPage].WRITE_PROTECT = 1;
									curr_prc->PTE_Table[instPage].MODIFIED = 0; 
									pstatsList[curr_prc->pid]->segprot++;
									cost = cost + 300; 
									// revision 071319
									if (O_flag){
										cout << " SEGPROT" << endl;
									}
								}
								else {
									curr_prc->PTE_Table[instPage].MODIFIED = 1; 
							
								}
						} 

						else {
							curr_prc->PTE_Table[instPage].REFERENCED = 1;
						 
							//curr_prc->PTE_Table[instPage].MODIFIED = 1; 
							if(is_write_protected(instPage, curr_prc)) {
									curr_prc->PTE_Table[instPage].WRITE_PROTECT = 1;
									curr_prc->PTE_Table[instPage].MODIFIED = 0; 
									pstatsList[curr_prc->pid]->segprot++;
									cost = cost + 300; 
									// revision 071319
									if (O_flag){
										cout << " SEGPROT" << endl;
									}
							}
							else {
								curr_prc->PTE_Table[instPage].MODIFIED = 1; 
							
							}
							 
						}	
					}


					break;
			}
		counter++;
	}
	
}	

void printPagetableOption(){

	for (int i = 0; i < prcList.size(); i++) {
		cout << "PT[" << i << "]: "; 
		for (int j = 0; j < PTE_SIZE; j++) {
			if (prcList[i]->PTE_Table[j].PRESENT_VALID == 1) {

				cout << j <<":";

				if (prcList[i]->PTE_Table[j].REFERENCED == 1) {
					cout<< "R";
				}
				else {
					cout <<"-";
				}

				if (prcList[i]->PTE_Table[j].MODIFIED == 1) {
					cout<< "M";
				}
				else {
					cout << "-";
				}

				if (prcList[i]->PTE_Table[j].PAGEDOUT == 1) {
					cout << "S ";
				}
				else {
					cout << "- ";
				} 
			}
			else {
				if (prcList[i]->PTE_Table[j].PAGEDOUT == 1) {
					cout << "# ";
				}
				else {
					cout << "* ";
				}

			}

		}
		cout << endl;
	}
}


void printFrametableOption() {
	cout << "FT: "; 
	for (int k = 0; k < frame_t.size(); k++) {
		if (frame_t[k].available == 1) {
			cout <<frame_t[k].pid <<":"<< frame_t[k].pageID<<" "; 
		}
		else {
			cout << "* ";
		}
	}
	cout << endl;
}

void printCostOption() {
	for (int p = 0; p < prcList.size(); p++) {
		printf("PROC[%d]: U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu\n",
			prcList[p]->pid,
			pstatsList[p]->unmaps, pstatsList[p]->maps, pstatsList[p]->ins, pstatsList[p]->outs,
			pstatsList[p]->fins, pstatsList[p]->fouts, pstatsList[p]->zeros,
			pstatsList[p]->segv, pstatsList[p]->segprot);
	}

	printf("TOTALCOST %lu %lu %lu %llu\n", counter, ctx_switches, process_exits, cost);
}

int main(int argc, char** argv) { 



	extern char* optarg; 
	char *algoType = NULL;
	char *optionsType = NULL; 
	int c;
	char in; 
	 
	
	while ((c = getopt (argc, argv, "a:o:f:")) !=  -1){
		switch(c) {
			case 'a': {
				algoType = optarg;
				in = algoType[0];
				if (in == 'f') {
					pager = new FCFS();
				}
				else if (in == 'r') {
					pager = new RandomFrames();
				}
				else if (in == 'c') {
					pager = new ClockPage();
				}
				else if (in == 'e') {
					pager = new NRU();
				}
				else if (in == 'a') {
					pager = new aging();
				}
				else if (in == 'w') {
					pager = new WorkingSet();
					cout << "WorkingSet is set as the paging algorithm" << endl;
					cout << "Check if paging is NULL (true/false) ";
					if (pager == NULL) {
						cout << "True " << endl;
					}
					else  {
						cout << "False " << endl;
					}				
				}
				break;
			}
			 
			case 'o': {
				optionsType = optarg;

				for (int i = 0; i < sizeof(optionsType); i++)  {
				in = optionsType[i];

				if (in == 'O') {
					O_flag = true;
				}
				if (in == 'P') {
					P_flag = true;
				}
				if (in == 'F') {
					F_flag = true;
				}
				if (in == 'S') {
					S_flag = true; 
				}
			}
				break;
			}
			
			case 'f': {
				MAX_FRAME_SIZE = stoi(optarg);
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

	optind++ ; 

	FILE *rand_file = fopen(argv[optind], "r");
	if (rand_file == NULL) {
		cout << "Cannot read the random file" << endl; 
	}

	parse_data(input_file);

	createRandVals(rand_file);



	Simulator();

	if (P_flag) {
		printPagetableOption();
	}
	if (F_flag) {
		printFrametableOption();
	}
	if (S_flag) {
		printCostOption();
	}

	//showProcessStatus(1); 

	return 0;

}
