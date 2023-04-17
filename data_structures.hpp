#include <iostream>

#ifndef DATA_STRUCTURES
#define DATA_STRUCTURES

/* You have to compute the overall execution time in cycles,
where the cost of operations (in terms of cycles)
are as follows: read/write (load/store) instructions count as 1,
context_switches instructions=130, process exits instructions=1230.
In addition if the following operations counts as follows:
maps=350, unmaps=410, ins=3200, outs=2750, fins=2350, fouts=2800,
zeros=150, segv=440, segprot=410 */

const enum PROC_CYCLES {
    MAPS,
    UNMAPS,
    INS,
    OUTS,
    FINS,
    FOUTS,
    ZEROS,
    SEGV,
    SEGPROT
};

// VALUES
const int int_readwrite = 1;
const int int_contextswitch = 130;
const int int_procexit = 1230;
const int int_maps = 350;
const int int_unmaps = 410;
const int int_ins = 3200;
const int int_outs = 2750;
const int int_fins = 2350;
const int int_fouts = 2800;
const int int_zeros = 150;
const int int_segv = 440;
const int int_segprot = 410;

// Max number of page table entries
const unsigned int NUM_PTE = 64;

// VMA Range For Lazy Page initialization
typedef struct
{
    unsigned int START;
    unsigned int END;
    unsigned int WRITE_PROTECT : 1;
    unsigned int PAGEDOUT : 1;
} vma_range;

// VMA Page Bitfield structure -> Contains Protection Flags metadata and frame_number
typedef struct
{
    // Custom Protection / Flag bits
    unsigned int UNUSED_BITS : 11;
    unsigned int EXISTS : 1;
    unsigned int PID : 8;

    // Required Protection / Flag Bits
    unsigned int PRESENT : 1;
    unsigned int REFERENCED : 1;
    unsigned int MODIFIED : 1;
    unsigned int WRITE_PROTECT : 1;
    unsigned int PAGEDOUT : 1;

    // Page number bits (Supports 128 frames as maximum)
    unsigned int frame_number : 7;
} pte_t;

// Frame Table Struct - Stores Data for Reverse Mapping frame -> page
typedef struct
{
    // Process id
    short process_id;

    // Page number bits (Supports 128 frames as maximum)
    short VMA_frame_number;
} frame_t;

class Process
{
    static int counter;

public:
    Process()
    {
        pid = counter++;
    }

    // Initializes array of VMA Ranges used for PTE creation on pagefault
    void init_vma(const int num_vmas_)
    {
        num_vmas = num_vmas_;
        vma_arr = new vma_range[num_vmas];
    }

    // Adds VMA range specs per input to the VMA array
    void add_vma(const unsigned int vma_num, const int start_vpage, const int end_vpage, const int write_protected, const int file_mapped)
    {
        // Set VMA range (NOT PTE) in Process
        vma_arr[vma_num].START = start_vpage;
        vma_arr[vma_num].END = end_vpage;
        vma_arr[vma_num]
            .WRITE_PROTECT = write_protected;
        vma_arr[vma_num].PAGEDOUT = file_mapped;
    }

    // Helper function to check if a VMA exists on pagefault
    bool vma_exists(const unsigned int vma_query)
    {
        for (int i = 0; i < num_vmas; i++)
        {
            vma_range temp = vma_arr[i];
            if (vma_query >= temp.START && vma_query <= temp.END)
            {
                return true;
            }
        }
        return false;
    }

    // TODO: Update for desired output
    void print_process_table()
    {
        std::string hashtag = "#";
        std::string dash = "-";

        for (int i = 0; i < NUM_PTE; i++)
        {
            const char *present_output = page_table_arr[i].PRESENT ? hashtag.c_str() : dash.c_str();
            printf("%s", present_output);
        }
        printf("\n");
    }

    // Temp function to help validate input
    void print_vma_ranges()
    {
        printf("Total Number of VMA Ranges: %d\n", num_vmas);
        for (int i = 0; i < num_vmas; i++)
        {
            printf("Start: %d Stop: %d Write: %d File: %d\n",
                   vma_arr[i].START, vma_arr[i].END, vma_arr[i].WRITE_PROTECT, vma_arr[i].PAGEDOUT);
        }
    }

    void allocate_cost(PROC_CYCLES cost_type)
    {
        switch (cost_type)
        {
        case UNMAPS:
            unmaps += int_unmaps;
        case MAPS:
            maps += int_maps;
        case INS:
            ins += int_ins;
        case OUTS:
            outs += int_outs;
        case FINS:
            fins += int_fins;
        case FOUTS:
            fouts += int_fouts;
        case ZEROS:
            zeros += int_zeros;
        case SEGV:
            segv += int_segv;
        case SEGPROT:
            segprot += int_segprot;
        }
    }

private:
    unsigned int pid;
    unsigned int num_vmas;
    vma_range *vma_arr;
    pte_t page_table_arr[NUM_PTE];
    unsigned long unmaps;
    unsigned long maps;
    unsigned long ins;
    unsigned long outs;
    unsigned long fins;
    unsigned long fouts;
    unsigned long zeros;
    unsigned long segv;
    unsigned long segprot;
};

int Process::counter = 0;

#endif