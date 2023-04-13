
#ifndef DATA_STRUCTURES
#define DATA_STRUCTURES

// Max number of page table entries
const unsigned int NUM_PTE = 64;

// VMA Bitfield structure
struct pte_t
{
    // Custom Protection / Flag bits
    unsigned int UNUSED : 19;
    unsigned int PID : 1;

    // Required Protection / Flag Bits
    unsigned int PRESENT : 1;
    unsigned int REFERENCED : 1;
    unsigned int MODIFIED : 1;
    unsigned int WRITE_PROTECT : 1;
    unsigned int PAGEDOUT : 1;

    // Page number bits (Supports 128 frames as maximum)
    unsigned int frame_number : 7;
};

class Page_Table
{
public:
    pte_t get_page_table_entry(int entry)
    {
        return page_table_arr[entry];
    }

private:
    pte_t page_table_arr[NUM_PTE];
};

#endif