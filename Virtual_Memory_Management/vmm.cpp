#include <iostream>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <list>
#include <deque>
#include <iomanip>
#include <unistd.h>

/* 
Author: Soowhan Park
Email: sp6682@nyu.edu
Title: Lab3
*/

using namespace std;

#define MAX_FRAMES 128
#define MAX_VPAGES 64

// Page table Entry
typedef struct { 
    unsigned int PRESENT_VALID  :1;
    unsigned int REFERENCED  :1;
    unsigned int MODIFIED  :1;
    unsigned int WRITE_PROTECT  :1;
    unsigned int PAGEDOUT  :1;
    unsigned int FILE_MAPPED :1;
    unsigned int FRAME_NUMBER :7; 
    unsigned int HOLE :1;
 } pte_t; 

// Frame table
typedef struct { 
    int pid; 
    int FRAME_NUMBER;
    int vpage;
    int last_time_use;
    unsigned int used : 1;
    uint32_t age;
 } frame_t;

 struct INSTRUCTION {
    char op;
    int id_page; 
};

INSTRUCTION createINST(char op, int id){
    INSTRUCTION inst;
    inst.op = op;
    inst.id_page = id;
    return inst; 
}

// Reading input variables
string line; 
char *each_line = NULL; 
const char delim[] = " \t\r\n\v\f";
bool P_option,F_option,O_option,S_option = false;

// For Random algorithm
vector <int> randvals;
int randvals_size; 
int ofs;
int myrandom(int burst);

// Global variables for frame table
frame_t frame_table[MAX_FRAMES]; 
int frame_num; // to be changed by input
deque<frame_t*> free_pool;
list <INSTRUCTION> INST_LIST; 

class VMA{
  public:
    int start_vpage;
    int end_vpage; // end vpage - start_vpage +1 
    int write_protected; // binary whether VMA is write protected or not
    int file_mapped;  // binary to indiate whether the VMA is mapped to a file or not

    VMA (int startVpage, int endVpage, int writeProtected, int fileMapped){
        start_vpage = startVpage;
        end_vpage = endVpage;
        write_protected = writeProtected;
        file_mapped = fileMapped; 
    }
};

class Process {       
  public:
    int pid;
    vector <VMA> VMA_list;
    pte_t page_table[MAX_VPAGES]; 

    unsigned long int unmaps = 0 ;
    unsigned long int maps = 0;
    unsigned long int ins = 0 ;
    unsigned long int outs = 0;
    unsigned long int fins = 0;
    unsigned long int fouts = 0;
    unsigned long int zeros = 0;
    unsigned long int segv = 0;
    unsigned long int segprot = 0;

    Process (int p_id, vector <VMA> vmas){
        pid = p_id;
        VMA_list = vmas;
        for (int i = 0; i < MAX_VPAGES; i ++){
            page_table[i].FRAME_NUMBER = i ;
        }
    }
};

// Container for multiple processes for inputs that are process # > 1 
vector <Process*> PROC_LIST;

// Page replacement algorithms
class Pager {
public:
    int hand;
    virtual frame_t* select_victim_frame() = 0; 
};

class FIFO: public Pager {
public:
    int hand = 0;
    frame_t* select_victim_frame(){
        frame_t* victim = &frame_table[hand];
        hand += 1;
        hand %= frame_num;
        victim->used = 1;
        return victim;
    };
};

class Random: public Pager {
public:
    frame_t* select_victim_frame(){
        int hand = myrandom(frame_num);
        frame_t* victim = &frame_table[hand];
        victim->used = 1;
        return victim;
    };
};

class Clock: public Pager {
public:
    int hand = 0;
    frame_t* victim;
    frame_t* select_victim_frame(){
        while (true){
            pte_t *old_page = &PROC_LIST.at(frame_table[hand].pid)->page_table[frame_table[hand].vpage];
            if (old_page->REFERENCED){
                old_page->REFERENCED = 0;
                hand ++;
                hand %= frame_num;
                continue;
            }else{
                victim = &frame_table[old_page->FRAME_NUMBER];
                victim->used = 1; 
                hand ++;
                hand %= frame_num;
                break;
            }
        }
        return victim;
    };
};

// time checker for the rest of algorithms
int time_check = 0;

class NRU: public Pager {
public:
    int hand = 0;
    frame_t* classes[4];
    int class_idx; 
    bool reference_reset = false;
    pte_t *old_page;
    frame_t* victim;
    frame_t* select_victim_frame(){
        for (int i = 0; i < 4; i++){
            classes[i] = nullptr;
        }
        for (int i = 0; i < frame_num; i++){
            old_page = &PROC_LIST.at(frame_table[hand].pid)->page_table[frame_table[hand].vpage];
            class_idx = old_page->REFERENCED * 2 + old_page->MODIFIED;
            
            if (classes[class_idx] == nullptr){
                classes[class_idx] = &frame_table[frame_table[hand].FRAME_NUMBER];
            }
            if (reference_reset){
                PROC_LIST.at(frame_table[i].pid)->page_table[frame_table[i].vpage].REFERENCED = 0;
            }
            hand ++;
            hand %= frame_num;
        }
        for (int i = 0; i < 4; i ++){
            if(classes[i] != nullptr){
                victim = classes[i];
                victim->used = 1;
                hand = victim->FRAME_NUMBER + 1 ;
                hand %= frame_num;
                break;
            }
        }
        if (time_check >= 50) {
            for (int i = 0; i < frame_num; i++) {
                PROC_LIST.at(frame_table[i].pid)->page_table[frame_table[i].vpage].REFERENCED = 0;
            }
            time_check = 0;
        }
        return victim;
    };
};

class Aging: public Pager {
public:
    int hand = 0;
    frame_t* select_victim_frame(){
        frame_t* victim = &frame_table[hand];
        for (int i =0; i < frame_num; i++){
            frame_table[hand].age >>= 1;
            if (PROC_LIST.at(frame_table[hand].pid)->page_table[frame_table[hand].vpage].REFERENCED == 1){
                frame_table[hand].age = (frame_table[hand].age | (uint32_t)(1<<31)) ;
                PROC_LIST.at(frame_table[hand].pid)->page_table[frame_table[hand].vpage].REFERENCED = 0;
            }
            if(frame_table[hand].age < victim->age){
                victim = &frame_table[hand];
            }
            hand ++;
            hand %= frame_num;
        }
        hand = victim->FRAME_NUMBER + 1;
        hand %= frame_num;
        victim->used = 1;
        return victim;
    };
};

class WorkingSet: public Pager {
public:
    int hand = 0;
    frame_t* victim;
    int smallest_time;
    frame_t* select_victim_frame(){
        for (int i = 0; i < frame_num; i++) {
            frame_t* temp_frame = &frame_table[hand];
            pte_t* old_page = &PROC_LIST.at(temp_frame->pid)->page_table[temp_frame->vpage];
            int age = time_check - temp_frame->last_time_use;
            if (old_page->REFERENCED == 1) {
                old_page->REFERENCED = 0;
                temp_frame->last_time_use = time_check;
            } else if (age > 49 && old_page->REFERENCED == 0 ) {
                    hand = temp_frame->FRAME_NUMBER + 1;
                    hand %= frame_num;
                    victim = temp_frame;
                    victim->used = 1;
                    return victim;
            }else{
                smallest_time = time_check - temp_frame->last_time_use;
                victim = &frame_table[smallest_time];
            }
            hand++;
            hand %= frame_num;
        }
        victim = &frame_table[hand];
        int temp = hand;
        for (int i = 0; i < frame_num; i++) {
            frame_t* temp_frame = &frame_table[temp];
            if (victim->last_time_use > temp_frame->last_time_use) {
                victim = temp_frame;
            }
            temp++;
            temp %= frame_num;
        }
        if (victim == nullptr){
            victim = &frame_table[hand];
        }
        hand = victim->FRAME_NUMBER + 1;
        hand %= frame_num;
        victim->used = 1;
        return victim;
    };
};

// Global Pager that is determined by the command by user
Pager *THE_PAGER;

// Helper functions for simulation 
frame_t *allocate_frame_from_free_pool(){
    frame_t *curr_frame = nullptr;
    if(!free_pool.empty()){
        curr_frame = free_pool.front();
        free_pool.pop_front();
    }
    return curr_frame;
}

frame_t *get_frame() {
    frame_t *frame = allocate_frame_from_free_pool();
    if (frame == nullptr) {
        frame = THE_PAGER->select_victim_frame();
    }
        return frame;
}

bool get_next_instruction(char *operation, int *vpage){
    if(INST_LIST.size() > 0){ 
        INSTRUCTION curr_ins = INST_LIST.front();
        INST_LIST.pop_front(); 
        *operation = curr_ins.op;
        *vpage = curr_ins.id_page;
        time_check ++;
        return true;
    }
    return false;
}

// Summary
int context_switches;
int cost; 
int process_exits;
int inst_count;

void Simulation(){
    char operation{};
    int vpage{};
    Process* current_process;
    while (get_next_instruction(&operation, &vpage)) {
        if(O_option)
            cout << inst_count << ": ==> " << operation << " " << vpage << endl;
        if (operation == 'c') {
            current_process = PROC_LIST.at(vpage);
            context_switches ++;
            cost += 130; 
            inst_count ++;
            continue; 
        } else if (operation == 'e') {
            if(O_option)
                cout << "EXIT current process " << vpage << endl;
            process_exits ++;
            cost += 1250;
            for (int i = 0; i< MAX_VPAGES; i++){
                pte_t* temp_page = &current_process->page_table[i];
                if (temp_page->PRESENT_VALID){
                    if(O_option)
                        cout << " UNMAP " << vpage << ":" << i << endl;
                    cost += 400;
                    current_process->unmaps += 1;
                    if (temp_page->MODIFIED && temp_page->FILE_MAPPED){
                        if(O_option)
                          cout << " FOUT" << endl;
                        current_process->fouts += 1;
                        cost += 2400;
                    }
                    frame_t* new_frame = &frame_table[temp_page->FRAME_NUMBER];
                    new_frame->used = 0;
                    free_pool.push_back(new_frame);
                }
                temp_page->PRESENT_VALID = 0;
                temp_page->PAGEDOUT = 0;
            }
            inst_count ++;
            continue;
        }
        cost ++;
        pte_t *pte = &current_process->page_table[vpage];
        if ( !pte->PRESENT_VALID) {
            for (int i = 0; i < current_process->VMA_list.size(); i++){
                if ( current_process->VMA_list.at(i).start_vpage <= vpage && vpage <= current_process->VMA_list.at(i).end_vpage){
                    pte->WRITE_PROTECT = current_process->VMA_list.at(i).write_protected;
                    pte->FILE_MAPPED = current_process->VMA_list.at(i).file_mapped;
                    pte->HOLE = 1;
                    break;
                }
            }
            if (pte->HOLE == 1) {
                frame_t *newframe = get_frame();
                if (newframe->used){
                    if(O_option)
                        cout << " UNMAP " << newframe->pid << ":" << newframe->vpage << endl;
                    PROC_LIST.at(newframe->pid)->unmaps++;
                    cost += 400;
                    pte_t *prev_proc = &PROC_LIST.at(newframe->pid)->page_table[newframe->vpage];
                    prev_proc->PRESENT_VALID = 0 ;
                    if (prev_proc->MODIFIED){
                        if (prev_proc->FILE_MAPPED){
                            prev_proc->MODIFIED = false;
                            if(O_option)
                                cout << " FOUT" << endl;
                            PROC_LIST.at(newframe->pid)->fouts ++;
                            cost += 2400;
                        }else{
                            prev_proc->MODIFIED = false;
                            prev_proc->PAGEDOUT = true;
                            if(O_option)
                                cout << " OUT" << endl;
                            PROC_LIST.at(newframe->pid)->outs ++;
                            cost += 2700;
                        }
                    }
                }
                if(pte->PAGEDOUT){
                    if(O_option)
                        cout << " IN" << endl;
                    current_process->ins++;
                    cost += 3100;
                }else if (pte->FILE_MAPPED){
                    if(O_option)
                        cout << " FIN" << endl;
                    current_process->fins++;
                    cost += 2800;
                }else{
                    if(O_option)
                        cout << " ZERO" << endl;
                    current_process->zeros++;
                    cost += 140;
                }
                newframe->pid = current_process->pid;
                newframe->vpage = vpage;
                newframe->age = 0;
                if(O_option)
                    cout << " MAP " << newframe->FRAME_NUMBER << endl;
                current_process->maps++;
                cost += 300; 
                pte->FRAME_NUMBER = newframe->FRAME_NUMBER;
                pte->PRESENT_VALID = 1;
            }else{
                if(O_option)
                    cout << " SEGV" << endl;
                current_process->segv ++;
                cost += 340;
                inst_count++;
                continue;
            }
        }
        if (operation == 'r' || operation == 'w'){
            pte->REFERENCED = 1;
        }
        if (operation == 'w'){
            if (pte->WRITE_PROTECT){ 
                if(O_option)
                    cout << " SEGPROT" << endl;
                current_process->segprot ++;
                cost += 420;
            } else{
                pte->MODIFIED = 1;
            }
        }
        inst_count ++;
    }
}

// Read random file 
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

int myrandom(int burst){
    int rand_num = (randvals[ofs] % burst);
    ofs += 1; 
    if (ofs == randvals_size){
        ofs = 0;
    }
    return rand_num;
}

// Parse the input and create list for vmas -> processes
void read_input(char* file) {
    string temp_line;
    ifstream input;
    input.open(file);
    if (!input) {
        cerr << "Invalid Input/File" << endl;
        exit(1);
    }
    int proc_cnt;
    int vma_cnt;
    while (!getline(input, temp_line).eof()) {
        if (temp_line.find("#") == string::npos){
            proc_cnt = stoi(temp_line);
            break;
        }
    }
    for (int i = 0; i < proc_cnt; i++){
        vector <VMA> VMAs;
        getline(input, temp_line);
        if (temp_line.find('#') != string::npos){
            while (temp_line.find('#') != string::npos){
                getline(input, temp_line);
            }
        }
        if (temp_line.find("#") == string::npos){
            vma_cnt = stoi(temp_line); 
            for (int i = 0; i < vma_cnt; i++){
                getline(input, temp_line);
                each_line = &temp_line[0];
                istringstream iss(each_line);
                int startVpage, endVpage, writeProtected, fileMapped; 
                iss >> startVpage >> endVpage >> writeProtected >> fileMapped;
                VMA vma = VMA(startVpage,endVpage,writeProtected,fileMapped);
                VMAs.push_back(vma);
            }
            Process *proc = new Process(i, VMAs);
            PROC_LIST.push_back(proc);
        }
    }
    while (!getline(input, temp_line).eof()) {
        if (temp_line.find("#") == string::npos){
            each_line = &temp_line[0];
            istringstream iss(each_line);
            char op;
            int page_id;
            iss >> op >> page_id;
            INSTRUCTION inst = createINST(op, page_id);
            INST_LIST.push_back(inst);
        }
    }
    input.close();
}

void print_pt_summary(){
    for (int i = 0; i < PROC_LIST.size(); i++){
        cout << "PT[" << i << "]:";
        for (int j = 0; j < MAX_VPAGES; j++) {
            if (PROC_LIST.at(i)->page_table[j].PRESENT_VALID){
                cout <<" "<< j << ":";
            
                if (PROC_LIST.at(i)->page_table[j].REFERENCED) {
                    cout << "R";
                } else {
                    cout << "-";
                }

                if (PROC_LIST.at(i)->page_table[j].MODIFIED) {
                    cout << "M";
                } else {
                    cout << "-";
                }

                if (PROC_LIST.at(i)->page_table[j].PAGEDOUT) {
                    cout << "S";
                } else {
                    cout << "-";
                }

            } else {
                if (PROC_LIST.at(i)->page_table[j].PAGEDOUT) {
                    cout << " #";
                } else {
                    cout << " *";
                } 
            }
        }
        cout << endl;   
    }
}
void print_ft_summary(){
    cout << "FT:";
    for(int i=0; i<frame_num; i++){
        frame_t *curr_frame = &frame_table[i];
        if(curr_frame->pid == -1){
            cout << " *";
        }else{
            cout << " "<< curr_frame->pid << ":" << curr_frame->vpage;
        }
    }
    cout << endl;
}

void print_proc_summary(){
    for (int i = 0; i < PROC_LIST.size(); i++) {
        printf("PROC[%d]: U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu\n",
               PROC_LIST.at(i)->pid, PROC_LIST.at(i)->unmaps, PROC_LIST.at(i)->maps, PROC_LIST.at(i)->ins, PROC_LIST.at(i)->outs,
               PROC_LIST.at(i)->fins, PROC_LIST.at(i)->fouts, PROC_LIST.at(i)->zeros, PROC_LIST.at(i)->segv, PROC_LIST.at(i)->segprot);
    }
}

void print_total_cost(){
    cout << "TOTALCOST " << inst_count << " " << context_switches << " " << process_exits << " " << cost << " " << sizeof(pte_t) <<  endl;
}

int main(int argc, char* argv[]){
    int c;    
    string output;
    while((c = getopt (argc, argv, "f:a:o:")) != -1){
        switch(c)
            {
            case 'f':
                frame_num = stoi(optarg);
                break;
            case 'a':
                if (optarg[0] == 'f'){
                    THE_PAGER = new FIFO();
                }else if(optarg[0] == 'r'){
                    THE_PAGER = new Random();
                }else if(optarg[0] == 'c'){
                    THE_PAGER = new Clock();
                }else if(optarg[0] == 'e'){
                    THE_PAGER = new NRU();
                }else if(optarg[0] == 'a'){
                    THE_PAGER = new Aging();
                }else if(optarg[0] == 'w'){
                    THE_PAGER = new WorkingSet();
                }
                break;
            case 'o':
                if (optarg != NULL){
                    output = optarg;
                    for (int i =0; i < output.size(); i++){
                        switch(output[i]){
                            case 'F':   F_option = true;
                                        break;
                            case 'P':   P_option = true;
                                        break;
                            case 'O':   O_option = true;
                                        break;
                            case 'S':   S_option = true;
                                        break;
                        }
                    }
                }
                break;
            default:
                cout << "Invalid option" << endl;
                break;
            }
    }
    read_input(argv[optind]); 
    read_rfile(argv[optind + 1]);

    // set up the free pool
    for (int i = 0; i < frame_num; i++) {
        frame_table[i].pid = -1;
        frame_table[i].FRAME_NUMBER = i;
        frame_table[i].vpage = -1; 
        frame_table[i].used = 0;
        frame_table[i].age = 0;
        frame_t* frame = &frame_table[i];
        free_pool.push_back(frame);
    }
    Simulation();
    if(P_option){
        print_pt_summary();
    }
    if(F_option){
        print_ft_summary();
    }
    if(S_option){
        print_proc_summary();
        print_total_cost();
    }
}
