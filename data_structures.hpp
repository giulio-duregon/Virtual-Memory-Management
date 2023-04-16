#include <iostream>

#ifndef DATA_STRUCTURES
#define DATA_STRUCTURES

// Max number of page table entries
const unsigned int NUM_PTE = 64;

// VMA Page Bitfield structure -> Contains Protection Flags metadata and frame_number
typedef struct
{
    // Custom Protection / Flag bits
    unsigned int UNUSED_BITS : 12;
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

    void set_vma_range(int start_vpage, int end_vpage, int write_protected, int file_mapped)
    {
        for (int i = start_vpage; i < (end_vpage + 1); i++)
        {
            page_table_arr[i].PRESENT = 1;
            page_table_arr[i].WRITE_PROTECT = write_protected;
            page_table_arr[i].PAGEDOUT = file_mapped;
        }
    }

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

private:
    unsigned int pid;
    pte_t page_table_arr[NUM_PTE];
};

int Process::counter = 0;

#endif