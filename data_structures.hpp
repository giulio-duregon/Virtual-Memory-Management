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

// VALUES (Cant Store in ENUM as 410 occurs twice)
// May not be necessry afte rall
// const int int_readwrite = 1;
// const int int_contextswitch = 130;
// const int int_procexit = 1230;
// const int int_maps = 350;
// const int int_unmaps = 410;
// const int int_ins = 3200;
// const int int_outs = 2750;
// const int int_fins = 2350;
// const int int_fouts = 2800;
// const int int_zeros = 150;
// const int int_segv = 440;
// const int int_segprot = 410;

// Max number of page table entries
const unsigned int NUM_PTE = 64;

// VMA Range For Lazy Page initialization
typedef struct
{
    unsigned int START;
    unsigned int END;
    unsigned int WRITE_PROTECT : 1;
    unsigned int FILEMAPPED : 1; // File mapped Flag
} vma_range;

// VMA Page Bitfield structure -> Contains Protection Flags metadata and frame_number
typedef struct
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
} pte_t;

// Frame Table Struct - Stores Data for Reverse Mapping frame -> page
typedef struct
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

    bool check_present_valid(int vpage)
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
                printf(" %d:%s%s%s ", i, r, s, m);
            }
            else
            {
                if (entry.PAGEDOUT)
                {
                    // PTEs that are not valid are represented by a ‘#’ if they have been swapped out
                    printf(" %s ", hashtag.c_str());
                }
                else
                {
                    // Or a ‘*’ if it does not have a swap area associated with.
                    printf(" %s ", star.c_str());
                }
            }

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
                   vma_arr[i].START, vma_arr[i].END, vma_arr[i].WRITE_PROTECT, vma_arr[i].FILEMAPPED);
        }
    }

    pte_t *get_vpage(int vpage_num)
    {
        return &page_table_arr[vpage_num];
    }

    void allocate_cost(PROC_CYCLES cost_type)
    {
        switch (cost_type)
        {
        case UNMAPS:
            unmaps += 1;
            break;
        case MAPS:
            maps += 1;
            break;
        case INS:
            ins += 1;
            break;
        case OUTS:
            outs += 1;
            break;
        case FINS:
            fins += 1;
            break;
        case FOUTS:
            fouts += 1;
            break;
        case ZEROS:
            zeros += 1;
            break;
        case SEGV:
            segv += 1;
            break;
        case SEGPROT:
            segprot += 1;
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