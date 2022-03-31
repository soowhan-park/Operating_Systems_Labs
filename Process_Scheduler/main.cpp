#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <list>
#include <iterator>
#include <deque>
#include <cstddef>
#include <stack> 
#include <queue> 
#include <iomanip>
#include <unistd.h>

using namespace std;

// Reading input variables
string line; 
char *each_line = NULL; 
const char delim[] = " \t\r\n\v\f";
int p_id = 0;
char schedspec;
bool verbose, verbose_T, verbose_E = false;
string sched_type;

// Static variables 
const int QUANTUM_10k = 10000;
const int MAX_PRIO = 4;

// Preemtive variables
int quantum = QUANTUM_10k;
int maxprio = MAX_PRIO;

// Simulation variables
int CURRENT_TIME; 
int timeInPrevState; 

//IO Utilization
int last_io;
int io_busy;

// Random values
int randvals_size;
vector<int> randvals; 
int ofs = 0;

// Process states
typedef enum {
    STATE_CREATED,
    STATE_RUNNING,
    STATE_BLOCKED,
    STATE_READY,
    STATE_DONE
} process_state_t;

// Transition states
typedef enum {
    TRANS_TO_READY,
    TRANS_TO_RUN,
    TRANS_TO_BLOCK,
    TRANS_TO_PREEMPT,
    TRANS_TO_DONE
} transitions;

class Process {       
  public:
    int pid;
    int arrival_time;
    int total_CPU_time;
    int CPU_burst;
    int IO_burst;
    int state_ts;
    int PRIO; // static priority
    int dynamic_PRIO; 
    process_state_t curr_state;

    int curr_CPU_burst;
    int curr_IO_burst;
    int remaining_time; 
    int FT = 0 ; // Finishing TIme
    int TT = 0; // Turnaround time
    int IT = 0; // I/O Time
    int CW = 0; // CPU Waiting time

    Process (int p_id, int at, int tc, int cb, int io, int prio){
        pid = p_id;
        arrival_time = at;
        total_CPU_time = tc;
        CPU_burst = cb; 
        IO_burst = io;
        state_ts = at;
        PRIO = prio;
        dynamic_PRIO = PRIO - 1;
    }
};

class EVENT {
public:
    int evtTimeStamp;
    Process* evtProcess;
    process_state_t old_state;
    process_state_t new_state;
    transitions transition;

    EVENT(int time, Process* p, process_state_t prev, process_state_t curr, transitions t){
        evtTimeStamp = time;
        evtProcess = p;
        old_state = prev;
        new_state = curr;
        transition = t;
    }
};

// DES variables
list <EVENT*> eventQ;
list <Process*> process_list;

void put_event(EVENT* evt) {
    int prev_size = eventQ.size();
    list <EVENT*> ::iterator it;
    for (it = eventQ.begin(); it != eventQ.end(); it++) {
        if (evt->evtTimeStamp < (*it)->evtTimeStamp){
            eventQ.insert(it, evt);
            break;
        }
    }
    if (eventQ.size() != prev_size + 1) {
        eventQ.push_back(evt);
    }
}

void rm_event(Process* p) {
    list <EVENT*> ::iterator it;
    for (it = eventQ.begin(); it != eventQ.end(); it++) {
        if ((*it)->evtProcess->pid == p->pid){
            
        }
    }
}

EVENT* get_event() {
    if (eventQ.size() == 0)
        return nullptr;
    EVENT* first_evt = eventQ.front();
    eventQ.pop_front();
    return first_evt;
}

int get_next_event_time() {
    if (eventQ.size() > 0)
        return eventQ.front()->evtTimeStamp;
    return -1;
}

int myrandom(int burst){
    int rand_num = 1 + (randvals[ofs] % burst);
    ofs += 1; 
    if (ofs == randvals_size){
        ofs = 0;
    }
    return rand_num;
}

void read_rfile(char* file){
    ifstream input;
    input.open(file);
    string line;
    if (!input) {
        cerr << "Invalid Input" << endl;
        exit(1);
    }else{
        getline(input,line);
        randvals_size = stoi(line);
        while(getline(input,line)){
            randvals.push_back(stoi(line));
        }
    }
    input.close();
}

void readInput_Process(char* file /*int maxprio*/) {
    string temp_line;
    ifstream input;
    input.open(file);
    int proc_specs[4];
    if (!input) {
        cerr << "Invalid Input/File" << endl;
        exit(1);
    }
    while (!getline(input, temp_line).eof()) {
        each_line = &temp_line[0];
        char *token = strtok(each_line, delim);
        int i = 0;
        while (token != NULL) {
            proc_specs[i] = stoi(token);
            i += 1;
            token = strtok(NULL, delim);
        }

        int prio = myrandom(maxprio);  
        Process *p = new Process(p_id, proc_specs[0],proc_specs[1],proc_specs[2],proc_specs[3],prio); 
        process_list.push_back(p); // create a process object while reading the input
        p_id += 1;
        p->remaining_time = p->total_CPU_time;
        EVENT *e = new EVENT(proc_specs[0], p, STATE_CREATED, STATE_READY, TRANS_TO_READY);
        put_event(e);
    }
    input.close();
}

class Scheduler {
public:
    virtual void add_process(Process* p){};
    virtual Process* get_next_process(){};
};

class FCFS: public Scheduler {
public:
    queue<Process*> run_Queue; //queue for FIFO
    void add_process(Process* p){
        run_Queue.push(p);
    }

    Process* get_next_process() {
        Process* temp;
        if (run_Queue.size() > 0){
            temp = run_Queue.front();
            run_Queue.pop();
            return temp; 
        }
        return nullptr;
    }
};

class LCFS: public Scheduler {
public:
    stack<Process*> run_Queue; //stack for LIFO
    void add_process(Process* p){
        run_Queue.push(p);
    }

    Process* get_next_process() {
        Process* temp;
        if (run_Queue.size() > 0){
            temp = run_Queue.top();
            run_Queue.pop();
            return temp;
        }
        return nullptr;
    }
};

class SRTF: public Scheduler {
public:
    deque<Process*> run_Queue;
    void add_process(Process* p){
        deque<Process *> :: iterator it;
        for(it = run_Queue.begin(); it != run_Queue.end(); it++){
            if (p->remaining_time < (*it)->remaining_time){
                break;
            }
        }
        run_Queue.insert(it, p);
    }

    Process* get_next_process() {
        Process* temp;
        if (run_Queue.size() > 0){
            temp = run_Queue.front();
            run_Queue.pop_front();
            return temp;
        }
        return nullptr;
    }
};

class RR: public Scheduler {
public:
    queue<Process*> run_Queue;
    void add_process(Process* p){
        run_Queue.push(p);
    }

    Process* get_next_process() {
        Process* temp;
        if (run_Queue.size() > 0){
            temp = run_Queue.front();
            run_Queue.pop();
            return temp;
        }
        return nullptr;
    }
};

class PRIO: public Scheduler {
public:
    queue<Process*> *activeQ;
    queue<Process*> *expiredQ;

    PRIO(){
        activeQ = new queue<Process*>[maxprio]();
        expiredQ = new queue<Process*>[maxprio]();
    }

    bool is_activeQ_empty(){
        bool res = false;
        int count = 0;
        for(int i = 0; i < maxprio; i++){
           if(activeQ[i].empty()){
               count += 1;
           }
       }
       if ( count == maxprio){
           res = true;
       }
       return res;
    }

    bool is_expiredQ_empty(){
        bool res = false;
        int count = 0;
        for(int i = 0; i < maxprio; i++){
           if(expiredQ[i].empty()){
               count += 1;
           }
       }
       if ( count == maxprio){
           res = true;
       }
       return res;
    }

    void add_process(Process* p){
        /*
        if(dynamic_prio < 0){
            reset and enter into expireQ
        }else{
            add to activeQ
        }
        */
        if(p->dynamic_PRIO < 0){
            p->dynamic_PRIO = p->PRIO - 1;
            expiredQ[p->dynamic_PRIO].push(p);
        }else{
            activeQ[p->dynamic_PRIO].push(p);
        }
    }

    Process* get_next_process() {
        /*
        if activeQ not empty
            pick activeQ[highest_prio].front()
        else
            swap(activeQ, expireQ) and try again
            [swapping means swapping the pointers to array not swapping the entire arrays]
        */
       int count = 0;
       int highest_prio = maxprio -1;
       if (is_activeQ_empty()){
           if(is_expiredQ_empty()){
               return nullptr;
           }
           swap(activeQ,expiredQ);
        //    queue<Process*> *temp_p = expiredQ;
        //    *expiredQ = *activeQ;
        //    *activeQ = *temp_p;
       }
       while(activeQ[highest_prio].empty()){
           highest_prio -= 1;
       }
       Process *p = activeQ[highest_prio].front();
       activeQ[highest_prio].pop();
       return p;
       
    }
};

class PREPRIO: public Scheduler {
public:
    queue<Process*> *activeQ;
    queue<Process*> *expiredQ;

    PREPRIO(){
        activeQ = new queue<Process*>[maxprio]();
        expiredQ = new queue<Process*>[maxprio]();
    }

    bool is_activeQ_empty(){
        bool res = false;
        int count = 0;
        for(int i = 0; i < maxprio; i++){
           if(activeQ[i].empty()){
               count += 1;
           }
       }
       if ( count == maxprio){
           res = true;
       }
       return res;
    }

    bool is_expiredQ_empty(){
        bool res = false;
        int count = 0;
        for(int i = 0; i < maxprio; i++){
           if(expiredQ[i].empty()){
               count += 1;
           }
       }
       if ( count == maxprio){
           res = true;
       }
       return res;
    }

    void add_process(Process* p){
        /*
        if(dynamic_prio < 0){
            reset and enter into expireQ
        }else{
            add to activeQ
        }
        */
        if(p->dynamic_PRIO < 0){
            p->dynamic_PRIO = p->PRIO - 1;
            expiredQ[p->dynamic_PRIO].push(p);
        }else{
            activeQ[p->dynamic_PRIO].push(p);
        }
    }

    Process* get_next_process() {
        /*
        if activeQ not empty
            pick activeQ[highest_prio].front()
        else
            swap(activeQ, expireQ) and try again
            [swapping means swapping the pointers to array not swapping the entire arrays]
        */
       int count = 0;
       int highest_prio = maxprio -1;
       if (is_activeQ_empty()){
           if(is_expiredQ_empty()){
               return nullptr;
           }
           swap(activeQ, expiredQ);
        //    queue<Process*> *temp_p = expiredQ;
        //    *expiredQ = *activeQ;
        //    *activeQ = *temp_p;
       }
       while(activeQ[highest_prio].empty()){
           highest_prio -= 1;
       }
       Process *p = activeQ[highest_prio].front();
       activeQ[highest_prio].pop();
       return p;
    }
};

void print_V(int current_time, EVENT* evt, Process* proc, int timeInPrevState) {
    switch(evt->new_state){
        case STATE_READY:
            if (evt->old_state == STATE_CREATED) {
                cout << current_time << " " << proc->pid << " " << timeInPrevState << ": CREATED -> READY" << endl;
            }else if (evt->old_state == STATE_BLOCKED) {
                cout << current_time << " " << proc->pid << " " << timeInPrevState << ": BLOCK -> READY" << endl;
            }
            else if (evt->old_state == STATE_RUNNING) {
                cout << current_time << " " << proc->pid << " " << timeInPrevState << ": RUNNG -> READY"<< "  cb=" << proc->curr_CPU_burst << " rem=" << proc->remaining_time << " prio=" << proc->dynamic_PRIO << endl;
            }
            break;
        case STATE_RUNNING:
            cout << current_time << " " << proc->pid << " " << timeInPrevState << ": READY -> RUNNG" << " cb=" << proc->curr_CPU_burst << " rem=" << proc->remaining_time << " prio=" << proc->dynamic_PRIO << endl;
            break;
        case STATE_BLOCKED:
            cout << current_time << " " << proc->pid << " " << timeInPrevState << ": RUNNG -> BLOCK" << "  ib=" << proc->curr_IO_burst << " rem=" << proc->remaining_time << endl;
            break;
        case STATE_DONE:
            cout << current_time << " " << proc->pid << " " << timeInPrevState << ": Done" << endl;
            break;
    }
}

// Scheduler variables
Process* CURRENT_RUNNING_PROCESS = nullptr;
Scheduler* SCHEDULER; 

void Simulation() {
    switch (schedspec){
    case 'F':
        sched_type = "FCFS";
        SCHEDULER = new FCFS();
        break;
    case 'L':
        sched_type = "LCFS";
        SCHEDULER = new LCFS();
        break;
    case 'S':
        sched_type =  "SRTF";
        SCHEDULER = new SRTF();
        break;
    case 'R':
        sched_type = "RR";
        SCHEDULER = new RR();
        break;
    case 'P':
        sched_type = "PRIO";
        SCHEDULER = new PRIO();
        break;
    case 'E':
        sched_type = "PREPRIO";
        SCHEDULER = new PREPRIO();
        break;
    }

    EVENT* evt;
    bool CALL_SCHEDULER = false;
    while( (evt = get_event() )) {
        Process *proc = evt->evtProcess; // this is the process the event works on
        CURRENT_TIME = evt->evtTimeStamp;
        timeInPrevState = CURRENT_TIME - proc->state_ts;
        EVENT *e;
        switch(evt->transition) { 
        case TRANS_TO_READY:
            // must come from BLOCKED or from PREEMPTION, must add to run queue
            // created, blocked, running -> ready
            if(proc->curr_state == STATE_BLOCKED){
                proc->dynamic_PRIO = proc->PRIO - 1;
            }
            proc->curr_state = STATE_READY;
            proc->curr_CPU_burst = 0;
            proc->curr_IO_burst = 0;
            proc->state_ts = CURRENT_TIME;
            SCHEDULER->add_process(proc);

            if(sched_type == "PREPRIO"){
                if(CURRENT_RUNNING_PROCESS != nullptr && CURRENT_RUNNING_PROCESS->dynamic_PRIO < proc->dynamic_PRIO){
                    list <EVENT*> ::iterator it;
                    int time;
                    for (it = eventQ.begin(); it != eventQ.end(); it++) {
                            if ((*it)->evtProcess->pid == CURRENT_RUNNING_PROCESS->pid){
                                time = (*it)->evtTimeStamp;
                                break;
                            }
                    }
                    if(CURRENT_TIME != time){
                        for (it = eventQ.begin(); it != eventQ.end(); it++) {
                            if ((*it)->evtProcess->pid == CURRENT_RUNNING_PROCESS->pid){
                                eventQ.erase(it);
                                break;
                            }
                        }
                        CURRENT_RUNNING_PROCESS->remaining_time += quantum - (CURRENT_TIME - CURRENT_RUNNING_PROCESS->state_ts);
                        CURRENT_RUNNING_PROCESS->curr_CPU_burst += quantum - (CURRENT_TIME - CURRENT_RUNNING_PROCESS->state_ts);
                        e = new EVENT(CURRENT_TIME, CURRENT_RUNNING_PROCESS, CURRENT_RUNNING_PROCESS->curr_state, STATE_READY, TRANS_TO_PREEMPT);
                        put_event(e);
                    }
                }
            }
            CALL_SCHEDULER = true; // conditional on whether something is run
            if(verbose){
                print_V(CURRENT_TIME, evt, proc, timeInPrevState);
            }
            break;
        case TRANS_TO_RUN:
            // create event for either preemption or blocking
            // ready -> running 
            proc->curr_state = STATE_RUNNING;
            if (proc->curr_CPU_burst == 0){
                proc->curr_CPU_burst = myrandom(proc->CPU_burst);
                if (proc->curr_CPU_burst > proc->remaining_time){
                    proc->curr_CPU_burst = proc->remaining_time;
                }
            }
            proc->state_ts = CURRENT_TIME;
            proc->curr_IO_burst = 0;
            proc->CW += timeInPrevState;
            if (quantum < proc->curr_CPU_burst){
                e = new EVENT(CURRENT_TIME + quantum, proc, proc->curr_state, STATE_READY, TRANS_TO_PREEMPT);
                put_event(e);
            }else{
                if (proc->curr_CPU_burst >= proc->remaining_time){ //when the remaining TC is about to ran out running -> done
                    e = new EVENT(CURRENT_TIME + proc->remaining_time, proc, proc->curr_state, STATE_DONE,TRANS_TO_DONE);
                    put_event(e);
                }else{ // when there is sufficient remaining TC
                    e = new EVENT(CURRENT_TIME + proc->curr_CPU_burst, proc, proc->curr_state, STATE_BLOCKED, TRANS_TO_BLOCK);
                    put_event(e);
                }
            }
            if(verbose){
                print_V(CURRENT_TIME, evt, proc, timeInPrevState);
            }
            break;
        case TRANS_TO_BLOCK:
            //create an event for when process becomes READY again
            proc->curr_state = STATE_BLOCKED;
            proc->remaining_time -= proc->curr_CPU_burst;
            proc->curr_IO_burst = myrandom(proc->IO_burst);
            proc->IT += proc->curr_IO_burst;
            proc->curr_CPU_burst = 0;
            proc->state_ts = CURRENT_TIME;
            if (CURRENT_TIME + proc->curr_IO_burst > last_io){
                if(last_io > 0){
                    io_busy += ((CURRENT_TIME + proc->curr_IO_burst) - max(CURRENT_TIME, last_io));
                }else{
                    io_busy += proc->curr_IO_burst;
                }
                last_io = CURRENT_TIME + proc->curr_IO_burst;
            }
            
            e = new EVENT(CURRENT_TIME + proc->curr_IO_burst, proc, proc->curr_state, STATE_READY,TRANS_TO_READY);
            put_event(e);
            CALL_SCHEDULER = true;
            CURRENT_RUNNING_PROCESS = nullptr;
            if(verbose){
                print_V(CURRENT_TIME, evt, proc, timeInPrevState);
            }
            break;
        case TRANS_TO_PREEMPT:
            // add to runqueue (no event is generated)
            proc->curr_state = STATE_READY;
            proc->curr_CPU_burst -= quantum;
            proc->remaining_time -= quantum;
            proc->state_ts = CURRENT_TIME;
            proc->dynamic_PRIO -= 1;
            SCHEDULER->add_process(proc);
            CALL_SCHEDULER = true;
            CURRENT_RUNNING_PROCESS = nullptr;
            if(verbose){
                print_V(CURRENT_TIME, evt, proc, timeInPrevState);
            }
            break;
        case TRANS_TO_DONE:
            proc->curr_state = STATE_DONE;
            proc->remaining_time = 0;
            proc->FT = CURRENT_TIME;
            proc->TT = proc->FT - proc->arrival_time;
            CALL_SCHEDULER = true;
            CURRENT_RUNNING_PROCESS = nullptr;
            if(verbose){
                print_V(CURRENT_TIME, evt, proc, timeInPrevState);
            }
            break;
        }

        delete evt; evt = nullptr;

        if(CALL_SCHEDULER) {
            if (get_next_event_time() == CURRENT_TIME)
                continue; //process next event from Event queue
            CALL_SCHEDULER = false; // reset global flag
            if (CURRENT_RUNNING_PROCESS == nullptr) {
                CURRENT_RUNNING_PROCESS = SCHEDULER->get_next_process();
                if (CURRENT_RUNNING_PROCESS == nullptr)
                    continue;
                 // create event to make this process runnable for same time.
                EVENT* new_evt = new EVENT(CURRENT_TIME, CURRENT_RUNNING_PROCESS, CURRENT_RUNNING_PROCESS->curr_state, STATE_RUNNING, TRANS_TO_RUN);
                put_event(new_evt);
            }
        }
    }
}
int main(int argc, char *argv[])
{   
    int c; 
    while ((c = getopt(argc, argv, "vtes:")) != -1) {
        switch (c) {
        case 'v':
            verbose = true;
            break;
        case 't':
            verbose_T = true;
            break;
        case 'e':
            verbose_E = true;
            break;
        case 's':
            sscanf(optarg, "%c%d:%d", &schedspec, &quantum, &maxprio);
            break;
        }
    }
    read_rfile(argv[optind + 1]);
    readInput_Process(argv[optind]);
    Simulation();
    Process * p;
    double total_TT;
    double total_CW;
    double total_CPU;
    double proc_list_size = process_list.size();

    if (sched_type == "RR" || sched_type == "PRIO" || sched_type == "PREPRIO"){
        cout << sched_type << " " << quantum << endl;
    }else{
        cout << sched_type << endl;
    }
    while(!process_list.empty()){
        p = process_list.front();
        process_list.pop_front();
        printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n",
               p ->pid, p->arrival_time, p->total_CPU_time, p->CPU_burst, p->IO_burst, p->PRIO, p->FT, p->TT, p->IT, p->CW);
        total_CPU += p->total_CPU_time;
        total_TT += p->TT;
        total_CW += p->CW;
    }

    double CPU_utilization = (total_CPU / (double) CURRENT_TIME) * 100;
    double avg_TT = total_TT / proc_list_size;
    double throughput = (proc_list_size / (double) CURRENT_TIME) * 100;
    double avg_CW = total_CW / proc_list_size;
    double IO_utilization = (io_busy / (double)CURRENT_TIME) * 100;
    printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n",
           CURRENT_TIME, CPU_utilization, IO_utilization, avg_TT, avg_CW, throughput);
    
    return 0;
}