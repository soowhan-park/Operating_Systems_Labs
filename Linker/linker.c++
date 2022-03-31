#include <iostream>
//#include <memory>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <iomanip>

/* 
Author: Soowhan Park
Email: sp6682@nyu.edu
Term: Fall/2021
Title: Lab1
*/

// Global Variables
using namespace std;
ifstream file;
char *token = NULL; //next token 
string temp_line; // temporaliy store each line into string
char *each_line = NULL; // transform the string line to char *
int line = 0; // line number (row)
int line_offset = 1; // line offset (column)
const char delim[] = " \t\r\n\v\f";
char* filename; 
int moduleNum; // Holds module count. 

//Rule 5 
int code_count;

//Rule 6
bool rule6 = false;

// Global variables for instructions
int instcount = 0;
int total_inst_count = 0;
const int MEM_SIZE = 512;


//functions to be used 
void __parseerror(int errcode);
int find_num (int addr);
int find_operand(int addr);
int find_opcode(int addr);
int find_offset(string sym);

//Symbol createSymbol(string sym, int val);

// holds symbol and the offset
struct Symbol {
    string sym;
    int val;
};

struct Memory {
    int module_number;
    char addr_mode;
    int addr;
    int moddule_count;
    int current_inst;
};

struct use_list_rule7{
    string sym;
    bool used;
    int module_num;
};

struct def_list{
    string sym;
    int module_num;
};

use_list_rule7 create_use_list(string sym, bool used, int module_num){
    use_list_rule7 u;
    u.sym = sym;
    u.used = used;
    u.module_num = module_num;
    return u;
}

Memory createMemory(int module_number, char addr_mode, int addr, int module_count, int current_inst){
    Memory m;
    m.module_number = module_number;
    m.addr_mode = addr_mode;
    m.addr = addr;
    m.moddule_count = module_count;
    m.current_inst = current_inst; 
    return m;
}

Symbol createSymbol(string sym, int val){
    Symbol s;
    s.sym = sym;
    s.val = total_inst_count + val;
    return s;
}

def_list createDefList(string sym, int module_num){
    def_list d;
    d.sym = sym; 
    d.module_num = module_num;
    return d;
}

// Symbol table, Memory Map, and E intruction holders..
vector<Symbol> symbol_table;

vector<Memory> memory_map;

vector<def_list> defintions;

vector<use_list_rule7> use_list_vector; 

vector< vector<string> > use_list;

vector<int> use_list_counts;

vector<def_list> rule4_pair;



// Get the next token
void getToken()
{   
    token = strtok(NULL, delim);
    if(token == NULL){
        getline(file, temp_line);
        line ++;
        each_line = &temp_line[0];
        // special case when there is a empty line in between two lines 
        if (strlen(each_line) == 0){
            line ++;
            //line_offset = 1;
            getline(file, temp_line);
            each_line = &temp_line[0];
            token = strtok(each_line, delim);
        }else{
            token = strtok(each_line, delim);
            line_offset = temp_line.find(token[0])+1;
        }
    }
    else{
        line_offset = (token - each_line) + 1;
    }
    //final_offset = line_offset + strlen(token);
}

/* Checks wheteher the token is a valid integer 
* 1. If token is a valid integer, convert its data type to int 
* 2. If the integer is larger than 2^30, print the error message
*/
int readInt()
{
    getToken(); 
    char *num = token;
    int parsed_int = 0;
    // converts string of num to integer
    for (int i = 0; i < strlen(num); i++){
        if (!isdigit(num[i])){
            __parseerror(0);
            exit(1);
        }
    }
    stringstream s_to_int(token);
    s_to_int >> parsed_int;
    if (parsed_int >= pow(2,30)){
        __parseerror(0);
        exit(1);
    }
    return parsed_int;
}

/* Check if the token is a valid symbol 
* 1. Symbols always begin with alpha characters followed by optional alphanumerical characters, i.e.[a-Z][a-Z0-9]
* 2. Valid symbols can be up to 16 characters. Integers are decimal based.
*/
char* readSymbol()
{
    int temp_line_num = line; //special case when there is only one input
    getToken();
    if (file.eof()){
        line = temp_line_num;
        line_offset++;
        __parseerror(1);
        exit(1);
    }

    if (token == NULL){
        __parseerror(1);
        exit(1);
    }

    if (strlen(token) >= 16){
        __parseerror(3);
        exit(1);
    }
    if (!isalpha(token[0])){ // Check if first symbol is alphabet and length of token is less than 16
        __parseerror(1);
        exit(1);
    }
    for (int i = 0; i < strlen(token); i++){
        if (!isalnum(token[i])){
            __parseerror(1);
            //error message and exit
        }
    }

    char * curr_token = token;
    return curr_token;
}

/* Check if the token has a valid addressing
* 1. Expected A/E/I/R
* 
*/

bool isValidAddress(char address){
    if (address == 'A' || address == 'E' || address == 'I' || address == 'R'){
        return true;
    }else{
        return false;
    }
}
char readIAER()
{
    char* temporary_token = token;
    int temporary_line = line;
    int temporary_line_offset = line_offset;
    getToken();

    if (token == NULL){
        line = temporary_line;
        line_offset = temporary_line_offset + strlen(temporary_token);
        __parseerror(2);
        exit(1);
    }

    if (strlen(token) != 1 || !isValidAddress(token[0])){
        __parseerror(2);
        exit(1);
    } 
    return token[0];
}

// prints corresponding parse error
void __parseerror (int errcode) {
    static char* errstr[] = {
        (char *)"NUM_EXPECTED",
        (char *)"SYM_EXPECTED",
        (char *)"ADDR_EXPECTED",
        (char *)"SYM_TOO_LONG",
        (char *)"TOO_MANY_DEF_IN_MODULE",
        (char *)"TOO_MANY_USE_IN_MODULE",
        (char *)"TOO_MANY_INSTR",
    };
    printf("Parse Error line %d offset %d: %s\n", line, line_offset, errstr[errcode]);
}

void defcountChcker(int defcount)
{
    if (defcount > 16){
        __parseerror(4);
        exit(1); 
    }
}

void usecountChcker(int usecount)
{
    if (usecount > 16){
        __parseerror(5);
        exit(1); 
    }
}

void instcountChcker(int instcount)
{
    total_inst_count += instcount;
    if (total_inst_count > MEM_SIZE){
        __parseerror(6);
        exit(1); 
    }
}

vector<Symbol> rule5 (vector<Symbol> sym_table)
{
    int max_module_size = instcount - 1;
    for (int i = 0; i < sym_table.size(); i++){
        if (sym_table.at(i).val + instcount - total_inst_count> max_module_size){
            printf("Warning: Module %d: %s too big %d (max=%d) assume zero relative\n", moduleNum, sym_table.at(i).sym.c_str(), sym_table.at(i).val - code_count,max_module_size);
            // cout << total_inst_count << endl;
            // cout << sym_table.at(i).val << endl;
            // cout << max_module_size << endl;
            sym_table.at(i).val = code_count;
        }
    }
    code_count += instcount;
    return sym_table;
}

bool contains(vector<Symbol> sym_table, string symbol)
{
    bool result = false;
    for (int i = 0; i < sym_table.size(); i++){
        if (sym_table.at(i).sym == symbol){
            result = true;
        }
    }
    return result;
}

void pass1()
{
    while (file.peek() != EOF) {
        moduleNum++;
        int defcount = readInt();
        Symbol symbols;
        vector <Symbol> temp_table;
        defcountChcker(defcount);
        //cout << line << ":" << line_offset << ":" << defcount << endl;
        for (int i = 0; i < defcount; i++){
            string sym = readSymbol(); //defined symbol
            //cout << line << ":" << line_offset << ":" << sym << endl;
            int val = readInt(); //relative address
            //cout << line << ":" << line_offset << ":" << val << endl;
            symbols = createSymbol(sym,val);
            if (contains(symbol_table, sym)){
                rule6 = true;
                break;
            }else{
                temp_table.push_back(symbols);
            }
        }
        int usecount = readInt();
        usecountChcker(usecount);
        //cout << line << ":" << line_offset << ":" << usecount << endl;
        for (int i =0; i < usecount; i++){
            string sym = readSymbol();
            //cout << line << ":" << line_offset << ":" << sym << endl;
        }
        instcount = readInt();
        instcountChcker(instcount);
        //cout << line << ":" << line_offset << ":" << instcount << endl;
        for (int i = 0; i < instcount; i++){
            char addressmode = readIAER();
            //cout << line << ":" << line_offset << ":" << addressmode << endl;
            int operand = readInt();
            //cout << line << ":" << line_offset << ":" <<  operand << endl;
        }
        temp_table = rule5(temp_table);
        for (int i =0; i < temp_table.size(); i++){
            symbol_table.push_back(temp_table[i]);
        }
    }
    line_offset += strlen(token); // get the final offset by adding the length of last token

}

void reset_global_variables(){
    moduleNum = 0;
    line = 0;
    line_offset = 0;
    total_inst_count = 0;
}

void rule7(int module_num, string sym){
    for(int i = 0; i < use_list_vector.size(); i++){
        if(use_list_vector.at(i).module_num == module_num && use_list_vector.at(i).sym == sym){
            use_list_vector.at(i).used = true;
        }
    }
}

void rule7_check(int module_num){
    for(int i = 0; i < use_list_vector.size(); i++){
        if(use_list_vector.at(i).module_num == module_num && !use_list_vector.at(i).used){
            cout << "Warning: Module " << module_num << ": " <<  use_list_vector.at(i).sym <<" appeared in the uselist but was not actually used" << endl;
        }
    }
}

void rule4(){
    for(int i = 0; i < defintions.size(); i++){
        bool b  = false;
        for(int j = 0; j < use_list_vector.size(); j++){
            if(defintions.at(i).sym == use_list_vector.at(j).sym){
                b = true;
                break;
            }
        }
        if (b == false){
            def_list d = createDefList(defintions.at(i).sym, defintions.at(i).module_num);
            rule4_pair.push_back(d);
        }
    }
    for (int z = 0; z < rule4_pair.size(); z++){
        cout << "Warning: Module " << rule4_pair.at(z).module_num << ": " << rule4_pair.at(z).sym << " was defined but never used" << endl;
    }
}



vector <Memory> addr_mode_handler (vector <Memory> mem_map)
{
    cout << "Memory Map" << endl;
    string temp;
    bool checker = false;
    int addr_count = 0;
    for(int i = 0; i < mem_map.size(); i++){
        int op_code = find_opcode(mem_map.at(i).addr);
        int op_erand = find_operand(mem_map.at(i).addr);
        int curr_module = mem_map.at(i).moddule_count;

        switch (mem_map.at(i).addr_mode){
            case 'R':
                if(op_code >= 10){
                    mem_map.at(i).addr = 9999;
                    cout << setw(3) << setfill('0') << i << ": " << setw(4) << setfill('0') << mem_map.at(i).addr << " Error: Illegal opcode; treated as 9999" << endl;

                }else if (op_erand > mem_map.at(i).current_inst){
                    mem_map.at(i).addr = mem_map.at(i).addr - op_erand + mem_map.at(i).module_number;
                    cout << setw(3) << setfill('0') << i << ": " << setw(4) << setfill('0') << mem_map.at(i).addr << " Error: Relative address exceeds module size; zero used" << endl;
                }
                else{
                    mem_map.at(i).addr += mem_map.at(i).module_number;
                    cout << setw(3) << setfill('0') << i << ": " << setw(4) << setfill('0') << mem_map.at(i).addr << endl;
                }
                break;
            case 'E':
                if(op_erand >= use_list_counts.at((mem_map.at(i).moddule_count) - 1) ) {
                    cout << setw(3) << setfill('0') << i << ": " << setw(4) << setfill('0') << mem_map.at(i).addr << " Error: External address exceeds length of uselist; treated as immediate" << endl;
                    break;
                }
                temp = use_list.at(mem_map.at(i).moddule_count - 1).at(op_erand);
                for(int j = 0; j < symbol_table.size(); j++){
                    if(symbol_table.at(j).sym == temp){
                        checker = true;
                    }
                }
                if (!checker){
                    cout << setw(3) << setfill('0') << i << ": " << setw(4) << setfill('0') << mem_map.at(i).addr <<" Error: " << temp << " is not defined; zero used" << endl;
                    rule7(curr_module, temp);
                }
                else{
                    mem_map.at(i).addr = find_num(mem_map.at(i).addr) + find_offset(use_list.at(mem_map.at(i).moddule_count - 1).at(op_erand));
                    cout << setw(3) << setfill('0') << i << ": " << setw(4) << setfill('0') << mem_map.at(i).addr << endl;
                    checker = false;
                    rule7(curr_module, temp);
                }
                break;
            case 'I':
                if (mem_map.at(i).addr >= 10000){
                    mem_map.at(i).addr = 9999;
                    cout << setw(3) << setfill('0') << i << ": " << setw(4) << setfill('0') << mem_map.at(i).addr << " Error: Illegal immediate value; treated as 9999" << endl;
                }else{
                    cout << setw(3) << setfill('0') << i << ": " << setw(4) << setfill('0') << mem_map.at(i).addr << endl;
                }
                break;
            case 'A':
                if(op_erand > MEM_SIZE){
                    mem_map.at(i).addr -= op_erand;
                    cout << setw(3) << setfill('0') << i << ": " << setw(4) << setfill('0') << mem_map.at(i).addr << " Error: Absolute address exceeds machine size; zero used" << endl;
                }else{
                    cout << setw(3) << setfill('0') << i << ": " << setw(4) << setfill('0') << mem_map.at(i).addr << endl;
                }
                break;
        }
        addr_count += 1;
        if(addr_count == mem_map.at(i).current_inst){
            rule7_check(curr_module);
            addr_count = 0;
        }
    }
    return mem_map;
}

int find_offset(string sym){
    for(int i = 0; symbol_table.size(); i++){
        if(sym == symbol_table.at(i).sym){
            return symbol_table.at(i).val;
        }
    }
    return 9999999;
}
int find_opcode(int addr)
{
    return addr / 1000;
}

int find_operand(int addr)
{
    return addr % 1000;
}

int find_num(int addr)
{
    int addr_copy = addr;
    int num = 1;
    int leading_num = 0;
    //string n = to_string(addr);
    while (addr_copy >= 10){
        addr_copy = addr_copy / 10;
        num *= 10;
    }
    leading_num = addr_copy;
    return num * leading_num;
}

void pass2()
{
    while (file.peek() != EOF) {
        moduleNum++;
        int defcount = readInt();
        defcountChcker(defcount);
        for (int i = 0; i < defcount; i++){
            string sym = readSymbol(); //defined symbol
            int val = readInt(); //relative address 
            def_list d = createDefList(sym, moduleNum);
            defintions.push_back(d);           
        }
        int usecount = readInt();
        usecountChcker(usecount);
        use_list_counts.push_back(usecount);
        use_list_rule7 use;
        vector<string> use_list_per_module;
        for (int i =0; i < usecount; i++){
            string sym = readSymbol();
            use_list_per_module.push_back(sym);
            use = create_use_list(sym,false,moduleNum);
            use_list_vector.push_back(use);
        }
        use_list.push_back(use_list_per_module);
        instcount = readInt();
        instcountChcker(instcount);
        //int count_E = 0;
        for (int i = 0; i < instcount; i++){
            char addressmode = readIAER();
            int operand = readInt();
            // if(addressmode == 'E'){
            //     count_E += 1; 
            // }
            Memory meminfo = createMemory(total_inst_count - instcount, addressmode, operand, moduleNum, instcount);
            memory_map.push_back(meminfo);
        }
    }
    memory_map = addr_mode_handler(memory_map);
    cout << endl;
    rule4();
    line_offset += strlen(token); // get the final offset by adding the length of last token
    //cout << total_inst_count << endl;

}

void printSymTable(){
    cout << "Symbol Table" << endl;
    for (int i = 0; i < symbol_table.size(); i++){
        if (rule6){
            cout << symbol_table.at(i).sym << "=" << symbol_table.at(i).val << " Error: This variable is multiple times defined; first value used" << endl;
        }else{
            cout << symbol_table.at(i).sym << "=" << symbol_table.at(i).val << endl;
        }
    }
    cout << endl;
}


int main(int argc, char* argv[])
{   
    filename = argv[1];
    file.open(filename); // input file (needs to be updated)
    pass1();
    file.close();
    printSymTable();
    reset_global_variables();
    file.open(filename);
    pass2();
    file.close();
    return 0;
}
