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

enum PROC_CYCLES
{
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

// VALUES (Cant Store in ENUM as 410 occurs twice)
const unsigned int int_readwrite = 1;
const unsigned int int_contextswitch = 130;
const unsigned int int_procexit = 1230;
const unsigned int int_maps = 350;
const unsigned int int_unmaps = 410;
const unsigned int int_ins = 3200;
const unsigned int int_outs = 2750;
const unsigned int int_fins = 2350;
const unsigned int int_fouts = 2800;
const unsigned int int_zeros = 150;
const unsigned int int_segv = 440;
const unsigned int int_segprot = 410;

// Max number of page table entries
const unsigned int NUM_PTE = 64;

// VMA Range For Lazy Page initialization
typedef struct vma_range
{
    unsigned int START : 7;
    unsigned int END : 7;
    unsigned int WRITE_PROTECT : 1;
    unsigned int FILEMAPPED : 1; // File mapped Flag
};

// VMA Page Bitfield structure -> Contains Protection Flags metadata and frame_number
typedef struct pte_t
{
    // Custom Protection / Flag bits
    unsigned int UNUSED_BITS : 9;
    unsigned int EXISTS : 1;
    unsigned int PID : 8;
    unsigned int NOT_FIRST_ACCESS : 1;

    // Required Protection / Flag Bits
    unsigned int PAGEDOUT : 1;
    unsigned int PRESENT : 1;
    unsigned int REFERENCED : 1;
    unsigned int MODIFIED : 1;
    unsigned int WRITE_PROTECT : 1;
    unsigned int FILEMAPPED : 1;

    // Page number bits (Supports 128 frames as maximum)
    unsigned int frame_number : 7;
};

// Frame Table Struct - Stores Data for Reverse Mapping frame -> page
typedef struct frame_t
{
    // Frame number
    unsigned int frame_number : 7;

    // Process id -- Extra bits used here so sizeof(frame_t) == sizeof(pte_t)
    short process_id = -1;

    // Frame number bits (Supports 64 frames as maximum) -> using signed for -1 to indicate free
    short VMA_page_number = -1;
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
        vma_arr[vma_num].FILEMAPPED = file_mapped;
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

    unsigned int check_present_valid(int vpage)
    {
        return page_table_arr[vpage].PRESENT;
    }

    bool vpage_can_be_accessed(int vpage)
    {
        // First check the Page table page entry and see if exists has been
        // Turned on in the bit-field
        if (page_table_arr[vpage].EXISTS)
        {
            return true;
        }
        // Otherwise iterate through the VMA ranges saved in the Process spec as metadata
        return vma_exists(vpage);
    }

    void set_referenced(int vpage)
    {
        page_table_arr[vpage].REFERENCED = 1;
    }

    void set_frame_num(int vpage, int framenum)
    {
        page_table_arr[vpage].frame_number = framenum;
    }

    void set_write(unsigned int vpage)
    {
        page_table_arr[vpage].MODIFIED = 1;
    }

    unsigned int write_protect_enabled(unsigned int vpage)
    {
        return page_table_arr[vpage].WRITE_PROTECT;
    }

    // TODO: Update for desired output
    void print_process_table()
    {
        std::string hashtag = "#";
        std::string star = "*";
        std::string dash = "-";
        std::string R = "R";
        std::string M = "M";
        std::string S = "S";

        for (int i = 0; i < NUM_PTE; i++)
        {
            pte_t entry = page_table_arr[i];
            if (entry.PRESENT)
            {
                const char *r = entry.REFERENCED ? R.c_str() : dash.c_str();
                const char *s = entry.PAGEDOUT ? S.c_str() : dash.c_str();
                const char *m = entry.MODIFIED ? M.c_str() : dash.c_str();
                printf(" %d:%s%s%s", i, r, m, s);
            }
            else
            {
                if (entry.PAGEDOUT)
                {
                    // PTEs that are not valid are represented by a ‘#’ if they have been swapped out
                    printf(" %s", hashtag.c_str());
                }
                else
                {
                    // Or a ‘*’ if it does not have a swap area associated with.
                    printf(" %s", star.c_str());
                }
            }
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
                   vma_arr[i].START, vma_arr[i].END, vma_arr[i].WRITE_PROTECT, vma_arr[i].FILEMAPPED);
        }
    }

    void pte_init(int vpage_num)
    {
        for (int i = 0; i < num_vmas; i++)
        {
            vma_range temp = vma_arr[i];
            if (vpage_num >= temp.START && vpage_num <= temp.END)
            {

                page_table_arr[vpage_num].UNUSED_BITS = 0;
                page_table_arr[vpage_num].EXISTS = 1;
                page_table_arr[vpage_num].PID = 0;
                page_table_arr[vpage_num].NOT_FIRST_ACCESS = 0;

                page_table_arr[vpage_num].PAGEDOUT = 0;
                page_table_arr[vpage_num].PRESENT = 0;
                page_table_arr[vpage_num].REFERENCED = 0;
                page_table_arr[vpage_num].MODIFIED = 0;
                page_table_arr[vpage_num].WRITE_PROTECT = temp.WRITE_PROTECT;
                page_table_arr[vpage_num].FILEMAPPED = temp.FILEMAPPED;

                page_table_arr[vpage_num].frame_number = 0;
            }
        }
    }
    pte_t *get_vpage(int vpage_num)
    {
        if (!page_table_arr[vpage_num].EXISTS)
        {
            pte_init(vpage_num);
        }
        return &page_table_arr[vpage_num];
    }

    void allocate_cost(PROC_CYCLES cost_type)
    {
        switch (cost_type)
        {
        case UNMAPS:
            unmaps++;
            break;
        case MAPS:
            maps++;
            break;
        case INS:
            ins++;
            break;
        case OUTS:
            outs++;
            break;
        case FINS:
            fins++;
            break;
        case FOUTS:
            fouts++;
            break;
        case ZEROS:
            zeros++;
            break;
        case SEGV:
            segv++;
            break;
        case SEGPROT:
            segprot++;
            break;
        }
    }

    void print_stats()
    {
        printf("U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu\n", unmaps, maps, ins, outs, fins, fouts, zeros, segv, segprot);
    }
    unsigned int get_pid()
    {
        return pid;
    }

    unsigned long long calc_total_cost()
    {
        total_cost += unmaps * int_unmaps;
        total_cost += maps * int_maps;
        total_cost += ins * int_ins;
        total_cost += outs * int_outs;
        total_cost += fins * int_fins;
        total_cost += fouts * int_fouts;
        total_cost += zeros * int_zeros;
        total_cost += segv * int_segv;
        total_cost += segprot * int_segprot;
        return total_cost;
    }

private:
    unsigned int pid = 0;
    unsigned int num_vmas = 0;
    vma_range *vma_arr;
    pte_t page_table_arr[NUM_PTE];
    unsigned long long total_cost = 0;
    unsigned long unmaps = 0;
    unsigned long maps = 0;
    unsigned long ins = 0;
    unsigned long outs = 0;
    unsigned long fins = 0;
    unsigned long fouts = 0;
    unsigned long zeros = 0;
    unsigned long segv = 0;
    unsigned long segprot = 0;
};

int Process::counter = 0;

#endif