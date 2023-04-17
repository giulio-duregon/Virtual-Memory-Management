#include <iostream>

#ifndef DATA_STRUCTURES
#define DATA_STRUCTURES

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

private:
    unsigned int pid;
    unsigned int num_vmas;
    vma_range *vma_arr;
    pte_t page_table_arr[NUM_PTE];
};

int Process::counter = 0;

#endif