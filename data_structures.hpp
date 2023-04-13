
#ifndef DATA_STRUCTURES
#define DATA_STRUCTURES

// Max number of page table entries
const unsigned int NUM_PTE = 64;

// VMA Page Bitfield structure -> Contains Protection Flags metadata and frame_number
typedef struct pte_t
{
    // Custom Protection / Flag bits
    unsigned int UNUSED : 12;
    unsigned int PID : 8;

    // Required Protection / Flag Bits
    unsigned int PRESENT : 1;
    unsigned int REFERENCED : 1;
    unsigned int MODIFIED : 1;
    unsigned int WRITE_PROTECT : 1;
    unsigned int PAGEDOUT : 1;

    // Page number bits (Supports 128 frames as maximum)
    unsigned int frame_number : 7;
};

// Frame Table Struct - Stores Data for Reverse Mapping frame -> page
typedef struct frame_t
{
    // Process id
    unsigned short process_id;

    // Page number bits (Supports 128 frames as maximum)
    unsigned short VMA_frame_number;
};

class Process
{
    static int counter;

public:
    Process()
    {
        pid = counter++;
    }

private:
    unsigned int pid;
    pte_t page_table_arr[NUM_PTE];
};

#endif