#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <iterator>
#include <deque>
#include <vector>
#include <unistd.h>
#include <limits.h>

using namespace std;

//Simulation
int idx = 0;
int head = 0;
bool curr_dir = true;
int curr_time = 0;

//For total sum
int total_turnaround = 0;
int tot_movement = 0;
int total_waittime = 0;
int max_waittime = 0;

//Option switches from command
bool v_option = false;
bool q_option = false;
bool f_option = false;

class IO_OPS {
public:
    int time_step;
    int track;
    int id;
    int start_time = 0;
    int end_time = 0;
    bool used = false;

    IO_OPS(int time, int track_, int io_id){
        time_step = time;
        track = track_;
        id = io_id;
    }
};

vector<IO_OPS*> IO_OPERATION_LIST;

class IO_Scheduler {
public:
    virtual void add_request(IO_OPS* request) = 0;
    virtual IO_OPS* get_request() = 0;
    virtual bool is_empty() = 0;
};

class FIFO : public IO_Scheduler {
public:
    deque<IO_OPS*> IO_queue;  
    void add_request(IO_OPS* request){
        IO_queue.push_back(request);
    }
    IO_OPS* get_request(){
        IO_OPS* next_io = nullptr;
        if (!IO_queue.empty()) {
            next_io = IO_queue.front();
            IO_queue.pop_front();
        }
        return next_io;
    }
    bool is_empty(){
        return IO_queue.empty();
    }
};

class SSTF : public IO_Scheduler {
public:
    deque<IO_OPS*> IO_queue;
    void add_request(IO_OPS* request){
        IO_queue.push_back(request);
    }

    IO_OPS* get_request(){
        int min_index = 0;
        int min_dist = abs(head - IO_queue.at(0)->track);
        IO_OPS* next_io = nullptr;
        if(IO_queue.empty()){
            return next_io;
        }else{
            for (int i = 0; i < IO_queue.size(); i++){
                int dist = abs(head - IO_queue.at(i)->track);
                if (min_dist > dist){
                    min_dist = dist;
                    min_index = i;
                }
            }
            next_io = IO_queue.at(min_index);
            IO_queue.erase(IO_queue.begin() + min_index);
        }
        return next_io;
    }
    bool is_empty(){
        return IO_queue.empty();
    }
};

class LOOK : public IO_Scheduler {
public:
    deque<IO_OPS*> IO_queue;

    //Similar to the technique used in lab2, use insert sort to easily deal with directions
    void add_request(IO_OPS* request){
        int prev_size = IO_queue.size();
        deque<IO_OPS*>::iterator it;
        for (it = IO_queue.begin(); it != IO_queue.end(); it++) {
            if (request->track < (*it)->track) {
                IO_queue.insert(it, request);
                break;
            }
        }
        if (IO_queue.size() != prev_size + 1) {
            IO_queue.push_back(request);
        }
    }
    
    IO_OPS* get_request(){
        if (IO_queue.empty()){
            return nullptr;
        }
        int min_index = 0;
        int min_dist = INT_MAX;
        IO_OPS* next_io = nullptr;
        if (!curr_dir) { 
            if (IO_queue.front()->track < head) {
                for (int i = 0; i < IO_queue.size(); i++) {
                    int dist = abs(head - IO_queue.at(i)->track);
                    if (head >= IO_queue.at(i)->track && dist < min_dist) {
                        min_dist = dist;
                        min_index = i;
                    }
                }
                next_io = IO_queue.at(min_index);
                IO_queue.erase(IO_queue.begin() + min_index);
            } else {
                next_io = IO_queue.front();
                IO_queue.pop_front();
            }

        }else{ 
            if (IO_queue.back()->track > head) {
                for (int i = 0; i < IO_queue.size(); i++){
                    int dist = abs(IO_queue.at(i)->track - head);
                    if (head <= IO_queue.at(i)->track && dist < min_dist){
                        min_dist = dist;
                        min_index = i;
                    }
                }
                next_io = IO_queue.at(min_index);
                IO_queue.erase(IO_queue.begin() + min_index);
            } else {
                next_io = IO_queue.back();
                IO_queue.pop_back();
            }
        }
        return next_io;
    }
                 
    bool is_empty(){
        return IO_queue.empty();
    }
};

class CLOOK : public IO_Scheduler {
public:
    deque<IO_OPS*> IO_queue;

    //Similar to the technique used in lab2, use insert sort to easily deal with directions
    void add_request(IO_OPS* request){
        int prev_size = IO_queue.size();
        deque<IO_OPS*>::iterator it;
        for (it = IO_queue.begin(); it != IO_queue.end(); it++) {
            if (request->track < (*it)->track) {
                IO_queue.insert(it, request);
                break;
            }
        }
        if (IO_queue.size() != prev_size + 1) {
            IO_queue.push_back(request);
        }
    }
    
    IO_OPS* get_request(){
        if (IO_queue.empty()){
            return nullptr;
        }
        IO_OPS* next_io = nullptr;
        int min_index = 0;
        int min_dist = INT_MAX;
        if (IO_queue.back()->track > head) {
            for (int i = 0; i < IO_queue.size(); i++) {
                int dist = abs(IO_queue.at(i)->track - head);
                if (head <= IO_queue.at(i)->track && dist < min_dist) {
                    min_dist = dist;
                    min_index = i;
                }
            }
            next_io = IO_queue.at(min_index);
            IO_queue.erase(IO_queue.begin() + min_index);
        }else{
            next_io = IO_queue.front();
            IO_queue.pop_front();
        }
        return next_io;
    }
                 
    bool is_empty(){
        return IO_queue.empty();
    }
};

class FLOOK : public IO_Scheduler {
public:
    deque<IO_OPS*> active;
    deque<IO_OPS*> add;

    //Similar to the technique used in lab2, use insert sort to easily deal with directions
    void add_request(IO_OPS* request){
        int prev_size = add.size();
        deque<IO_OPS*>::iterator it;
        for (it = add.begin(); it != add.end(); it++) {
            if (request->track < (*it)->track) {
                add.insert(it, request);
                break;
            }
        }
        if (add.size() != prev_size + 1) {
            add.push_back(request);
        }
    }
    
    IO_OPS* get_request(){
        if (active.empty()) {
                active.swap(add);
            }
        int min_index = 0;
        int min_dist = INT_MAX;
        IO_OPS* next_io = nullptr;
        if (!curr_dir) { 
            if (active.front()->track < head){
                for (int i = 0; i < active.size(); ++i){
                    int dist = abs(head - active.at(i)->track );
                    if (head >= active.at(i)->track && dist < min_dist){
                        min_dist = dist;
                        min_index = i;
                    }
                }
                next_io = active.at(min_index);
                active.erase(active.begin() + min_index);
            } else {
                next_io = active.front();
                active.pop_front();
                return next_io;
             }

        }else{
            if (active.back()->track > head){
                for (int i = 0; i < active.size(); i++){
                    int dist = abs(active.at(i)->track - head);
                    if (head <= active.at(i)->track && dist < min_dist){
                        min_dist = dist;
                        min_index = i;
                    }
                }
                next_io = active.at(min_index);
                active.erase(active.begin() + min_index);
            }else {
                next_io = active.back();
                active.pop_back();
            }
        }
        return next_io;
    }

    bool is_empty(){
        return active.empty() && add.empty();
    }
};

IO_Scheduler* THE_SCHEDULER;

// Check if all the requests are done
bool req_done(){
    int count = 0;
    for (int i = 0; i < IO_OPERATION_LIST.size(); i++){
        if (IO_OPERATION_LIST.at(i)->used == true){
            count ++;
        }
    }
    return count == IO_OPERATION_LIST.size();
}

//Simulate the disk scheduling
void simulation(){
    IO_OPS* curr_io = nullptr;
    IO_OPS* new_io;
    while (true) {
        //New I/O arrived
        if (idx != IO_OPERATION_LIST.size() && curr_time == IO_OPERATION_LIST.at(idx)->time_step) {
            new_io = IO_OPERATION_LIST.at(idx);
            THE_SCHEDULER->add_request(new_io);
            idx++;
        }
        // I/O is active and completed
        if (curr_io != nullptr && head == curr_io->track) {
            curr_io->end_time = curr_time;
            total_turnaround += (curr_time - curr_io->time_step);
            curr_io->used = true;
            curr_io = nullptr;
        }
        // I/O is not active
        if (curr_io == nullptr) {
            // Requests are pending
            if (!THE_SCHEDULER->is_empty()) {
                curr_io = THE_SCHEDULER->get_request();
                curr_io->start_time = curr_time;
                total_waittime += curr_time - curr_io->time_step;
                if (curr_time - curr_io->time_step > max_waittime){
                    max_waittime = curr_time - curr_io->time_step;
                }
                // change direction  
                if (curr_io->track > head) {
                    curr_dir = true;
                } else if (curr_io->track < head){
                    curr_dir = false;  
                } else {
                    continue;
                }
            //Exit simulation after all io from input file is processed
            }else if (THE_SCHEDULER->is_empty() && req_done()) {
                break;
            }
        }
        // I/O is active
        if (curr_io != nullptr && curr_io->track != head) {
            // move the head by one unit in the direction its going
            if (curr_dir) {
                head++;
            } else {
                head--;
            }
            tot_movement ++;
        }
        curr_time++;
    }
}

void read_input(char* file) {
    string line; 
    char *each_line = NULL;
    ifstream input;
    input.open(file);
    int IO_counter = 0;
    if (!input) {
        cerr << "Invalid Input/File" << endl;
        exit(1);
    }
    while (!getline(input, line).eof()) {
        if (line.find("#") == string::npos){
            each_line = &line[0];
            istringstream iss(each_line);
            int time_step;
            int track;
            iss >> time_step >> track;
            IO_OPS* ioreq = new IO_OPS(time_step, track, IO_counter);
            IO_OPERATION_LIST.push_back(ioreq);
            IO_counter ++;
        }
    }
    input.close();
}

int main(int argc, char *argv[]) {
    int c;
    while((c = getopt (argc, argv, "s:v:q:f")) != -1){
        switch(c){
            case 's':
                if (optarg[0] == 'i'){
                    THE_SCHEDULER = new FIFO();
                }else if(optarg[0] == 'j'){
                    THE_SCHEDULER = new SSTF();
                }else if(optarg[0] == 's'){
                    THE_SCHEDULER = new LOOK();
                }else if(optarg[0] == 'c'){
                    THE_SCHEDULER = new CLOOK();
                }else if(optarg[0] == 'f'){
                    THE_SCHEDULER = new FLOOK();
                }
                break;
            case 'v':
                v_option = true;
                break;
            case 'q':
                q_option = true;
                break;
            case 'f':   
                f_option = true;
                break;        
            default:    
                cout << "invalid" << endl;
                break;
        }
    }

    //read_input((char *)"input9");
    char* fileName = argv[optind];
    read_input(fileName);
    //THE_SCHEDULER = new LOOK();
    simulation();
    for (int i = 0; i < IO_OPERATION_LIST.size(); i++) {
        printf("%5d: %5d %5d %5d\n", IO_OPERATION_LIST.at(i)->id, IO_OPERATION_LIST.at(i)->time_step, IO_OPERATION_LIST.at(i)->start_time, IO_OPERATION_LIST.at(i)->end_time);
    }
    printf("SUM: %d %d %.2lf %.2lf %d\n", curr_time, tot_movement, double(total_turnaround) / idx, double(total_waittime)/idx, max_waittime);
}