//
//  main.cpp
//  VMM
//
//  Created by Chuhan Chen on 11/16/20.
//

#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <queue>

using namespace std;

enum pte_status {
    PRESENT = 7,
    REFERENCED = 8,
    MODIFIED = 9,
    WRITE_PROTECT = 10,
    PAGEDOUT = 11,
    FILE_MAPPED = 12
};

const int FRAME_BIT_MASK = 127;
const int NUM_PTE = 64;
const int MAX_FRAMES = 128;
int free_frame_index = 0;

struct pte_t;
struct frame_t;

//frame_t* get_frame();
//frame_t* allocate_frame_from_free_list();

struct pte_t {
    int pte;
    
    pte_t() :pte(0) {};
    
    int get_frame() {
        return pte & FRAME_BIT_MASK;
    }
    
    void set_frame(int frame) {
        pte &= (~FRAME_BIT_MASK); // mask frame bits all to 0s
        pte |= frame;
    }
    
    bool get_status(pte_status status) {
        return (pte >> status & 1) == 1;
    }
    
    void set_status(pte_status status, bool val) {
        if(val) {
            pte |= (1 << status);
        } else {
            if(get_status(status)) {
                pte ^= (1 << status);
            }
        }
    }
};

struct frame_t {
    int proc_id;
    int vpage;
    bool occupied;
    
    frame_t() :proc_id(-1), vpage(-1), occupied(false){};
};

struct vma {
    int start_vpage;
    int end_vpage;
    bool write_protected;
    bool file_mapped;
};

class Process {
public:
    int pid;
    vector<vma*> vmas;
    pte_t *page_table[NUM_PTE];
    
    string to_string();
};

string Process::to_string() {
    string result = "Process:\n";
    result += "  process id: " + std::to_string(pid) + "\n";
    result += "  vmas:\n";
    for(int i = 0; i <  vmas.size(); i++) {
        result += "    start_vpage: " + std::to_string(vmas.at(i) -> start_vpage) + ", "
                 + "end_vpage: " + std::to_string(vmas.at(i) -> end_vpage) + ",\n    "
                 + "write_protected: " + (vmas.at(i) -> write_protected ? "true" : "false") + ", "
        + "file_mapped: " + (vmas.at(i) -> file_mapped ? "true" : "false") + "\n";
    }
    return result;
}

vector<Process*> process_vec;
Process* cur_process = nullptr;

class Pager {
public:
    Pager(): frame_table_index(0), num_free_frame(MAX_FRAMES) {};
    
    void add_frame_to_used(frame_t* frame) {
        used_frames.push(frame);
    }
    
    virtual frame_t* select_victim_frame() =0;
//    frame_t* allocate_frame_from_free_list();
    
    
protected:
    int frame_table_index;
    int num_free_frame;
    queue<frame_t*> used_frames;
};

//frame_t* Pager::allocate_frame_from_free_list() {
//    // TODO
//    frame_t* free_frame = nullptr;
//    if(num_free_frame > 0) {
//        while(frame_table_index <= MAX_FRAMES) {
//            if(frame_table[frame_table_index].occupied == false) {
//                free_frame = &frame_table[frame_table_index];
//            }
//            frame_table_index++;
//            if(frame_table_index == MAX_FRAMES) {
//                frame_table_index = 0;
//            }
//        }
//    }
//    return free_frame;
//}

class Frame_Table {
public:
    int frame_size;
    frame_t frame_table[MAX_FRAMES];
    Pager* pager;
    
    Frame_Table(int frame_size, Pager* pager) {
        this -> frame_size = frame_size;
        this -> pager = pager;
    }
    
    frame_t* get_frame() {
        frame_t* frame = allocate_frame_from_free_list();
        if(frame == nullptr) {
            frame = pager -> select_victim_frame();
        }
        return frame;
    }

    frame_t* allocate_frame_from_free_list() {
        if(free_frame_index < MAX_FRAMES) {
            frame_table[free_frame_index] = frame_t();
            return &frame_table[free_frame_index++];
        }
        return nullptr;
    }
};

class FIFO_Pager: public Pager {
public:
    
    FIFO_Pager() {
//        cout << frame_table_index << ", " << num_free_frame << endl;
    }
    
    frame_t* select_victim_frame();
    
};

frame_t* FIFO_Pager::select_victim_frame() {
    // TODO
    frame_t* frame = used_frames.front();
    used_frames.pop(); // Pop the frame from the front of queue
    used_frames.push(frame); // Add the frame back to the end of queue
    cout << "frame removed from used_frame" << endl;
    return frame;
}
 
class Simulation {
public:
    string input_path;
    Pager* pager;
    Frame_Table* frame_table;
    
    Simulation(string input_path, Pager* pager, Frame_Table* frame_table) {
        this -> input_path = input_path;
        this -> pager = pager;
        this -> frame_table = frame_table;
    }
    
    void simulation();
    void process_input(ifstream &file);
    void parse_instruction(ifstream &file);
    void execute_instruction(char type, int num, int count, bool a);
};

void Simulation::simulation() {
    ifstream file(input_path);
    if(file.is_open()) {
        // process the input to parse each process
        process_input(file);
        
        // parse all the instructions
        parse_instruction(file);
        
        // Testing the processes
        for(int i = 0; i < process_vec.size(); i++) {
            cout << process_vec.at(i) -> to_string() << endl;
        }
        
        // close the input file
        file.close();
    } else {
        cout << "Unable to open input file" << endl;
    }
}

void Simulation::process_input(ifstream &file) {
    string line;
    int num_processes = 0;
    // parse number of processes
    while(getline(file, line)) {
        // find the first line not starting with '#'
        if(line.at(0) != '#') {
            num_processes = stoi(line);
            break;
        }
    }
    // parse each process
    for(int n = 0; n < num_processes; n++) {
        Process *proc_ptr = new Process();
        proc_ptr -> pid = n;
        process_vec.push_back(proc_ptr);
        int num_vmas = 0;
        // parse number of vmas by finding the first line not starting with '#'
        while(getline(file, line)) {
            if(line.at(0) != '#') {
                break;
            }
        }
        num_vmas = stoi(line);
        for(int i = 0; i< num_vmas; i++) {
            getline(file, line);
            int params[4];
            int slow = 0, fast = 0, index = 0;
            // parse each param, start_vpage, end_vpage, writed_protected, file_mapped
            while(fast < line.size()) {
                while(fast < line.size() && line.at(fast) != ' ') {
                    fast++;
                }
                params[index++] = stoi(line.substr(slow, fast - slow));
                
                while(fast < line.size() && line.at(fast) == ' ') {
                    fast++;
                }
                slow = fast;
            }
            vma *vma_ptr = new vma();
            vma_ptr -> start_vpage = params[0];
            vma_ptr -> end_vpage = params[1];
            vma_ptr -> write_protected = params[2] == 1;
            vma_ptr -> file_mapped = params[3] == 1;
            (proc_ptr -> vmas).push_back(vma_ptr);
        }
    } // end of parse each process
}

void Simulation::parse_instruction(ifstream &file) {
    string line;
    int count = 0;
    while(getline(file, line)) {
        if(line.at(0) == '#') {
            break;
        }
        cout << line << endl;
        count++;
    }
}

void Simulation::execute_instruction(char type, int num, int count, bool a) {
    if(type != 'c' && type != 'w' && type != 'r') {
        cout << "Instrcution type not allowed. Should be one of c, r or w " << endl;
        return;
    }
    
    cout << count << ": ==> " << type << " " << num << endl;
    switch (type) {
        case 'c':
            cur_process = process_vec.at(num);
            break;
            
        default: // Case r and w almost identical
            // Check if current process and vpage is valid
            bool found = false;
            pte_t* cur_pte = nullptr;
            for(int i = 0; i < (cur_process -> vmas).size(); i++) {
                vma* cur_vma = (cur_process -> vmas).at(i);
                if(num < cur_vma -> start_vpage) {
                    cout << "SEGV" << endl;
                    return;
                }
                if(num >= cur_vma -> start_vpage && num <= cur_vma -> end_vpage) {
                    cur_pte = (cur_process -> page_table)[num];
                    cur_pte -> set_status(WRITE_PROTECT, cur_vma -> write_protected);
                    cur_pte -> set_status(FILE_MAPPED, cur_vma -> file_mapped);
                    
                    // TODO thinks what else needs to be done after retriving the cur pte
                }
            }
            if(!found) {
                cout << "SEGV" << endl;
                return;
            }
            
            frame_t* frame = frame_table -> get_frame();
            if(frame -> occupied == false) { // frame hasn't been mapped
                frame -> occupied = true;
            } else { // frame was previously mapped
                int prev_proc_id = frame -> proc_id;
                int prev_vpage = frame -> vpage;
                // Get which proc and vpage to unmap (diff from cur proc and vpage)
                
                // check if prev page needs to be swapped out
                
                // check whether there is thing to swapped in or zero
                
                // Set proc_id & vpage
                
                // check for SEGPROT
                
            }
            
            
            
            break;
    }
}

int main(int argc, const char * argv[]) {
    // insert code here...
    std::cout << "Hello, World!\n";
    
    string input_path = "/Users/dlinhao/Documents/Linhao Ding/Mokokotchi/Operating System/Lab3/VMM/inputs/in1";
    
    int frame_size = 32;
    
    Pager* pager = new FIFO_Pager();
    Frame_Table* frame_table = new Frame_Table(frame_size, pager);
    Simulation* simulation = new Simulation(input_path, pager, frame_table);
    
    
    
//    for(int i = 0; i <= MAX_FRAMES; i++) {
//        frame_t* frame = get_frame();
//        pager -> add_frame_to_used(frame);
//    }

    
    return 0;
}
