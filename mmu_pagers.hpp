#include "data_structures.hpp"
#include <stdexcept>
#include <deque>

#ifndef MMU_PAGERS
#define MMU_PAGERS

// Define Pager Algo Types via Enum
enum PAGER_TYPES
{
    FIFO,
    Random,
    Clock,
    ESC_NRU,
    Aging,
    Working_Set
};

// Helper function
PAGER_TYPES parse_pager_type_from_input(char *ptype)
{
    switch (std::toupper(*ptype))
    {
    case 'F':
        return FIFO;
    case 'R':
        return Random;
    case 'C':
        return Clock;
    case 'E':
        return ESC_NRU;
    case 'A':
        return Aging;
    case 'W':
        return Working_Set;
    default:
        throw std::invalid_argument("Invalid Algorithm Argument, Options Are: F/R/C/E/A/W");
    };
}

// Helper function for debugging
char *GET_PAGER_NAME_FROM_ENUM(int enum_code)
{
    static char *enum_name[] = {
        (char *)"FIFO",
        (char *)"Random",
        (char *)"Clock",
        (char *)"ESC_NRU",
        (char *)"Aging",
        (char *)"Working_Set"};
    return enum_name[enum_code];
};

// Custom Exception
class NotImplemented : public std::logic_error
{
public:
    NotImplemented() : std::logic_error("Function not yet implemented"){};
};

// Base Class for Pager Algos -> defines interface
class Pager
{
public:
    Pager(int NUM_FRAMES, PAGER_TYPES ptype_)
    {
        ptype = ptype_;

        // Dynamically create the frame table array based on input args
        FRAME_TABLE = new frame_t[NUM_FRAMES];

        // Upon Initialization, All frames are free
        for (int i = 0; i < NUM_FRAMES; i++)
        {
            free_list.push_back(i);
        }
    };

    // Virtual Function to be implemented by derived classes
    // Selects VMA to be removed from physical frame
    // Physical Frame gets added to the free list
    virtual frame_t *select_victim_frame() { throw new NotImplemented; };

    // Maps a physical frame to a VMA page
    // pte_t struct -> frame_t struct
    void map_frame(pte_t &vpage, unsigned int vpage_number, unsigned int frame_number, unsigned int pid)
    {
        // Update VMA page mapping
        vpage.frame_number = frame_number;

        // TODO: Some other logic about modified / referenced bits

        // Update physical frame to reverse map to page
        FRAME_TABLE[frame_number].process_id = pid;
        FRAME_TABLE[frame_number].VMA_frame_number = vpage_number;
    };

    void unmap_frame(pte_t &page)
    {
        // Retrieve Physical Frame Number
        int frame_num = page.PAGEDOUT;

        // TODO: Implement logic to handle page getting unmapped from frame
        //  i.e. what to do if modified, referenced etc.

        // Clear Physical Frame
        clear_mapping(frame_num);

        page.PRESENT = 0;
    };

    // Clears previous physical frames (reverse) mapping
    // To a process id / virtual frame number
    void clear_mapping(int frame_number)
    {
        // Retrieve Frame
        frame_t frame = FRAME_TABLE[frame_number];

        // Reset frame Numbers
        frame.process_id = -1;
        frame.VMA_frame_number = -1;
    }
    PAGER_TYPES ptype;

protected:
    frame_t *FRAME_TABLE;
    std::deque<int> free_list;
};

// FIFO Pager Implementation
class FIFO_Pager : Pager
{
public:
    FIFO_Pager(int NUM_FRAMES) : Pager(NUM_FRAMES, FIFO){};
    frame_t *select_victim_frame()
    {
        int free_frame_num;
        frame_t *free_frame = nullptr;
        if (free_list.empty())
        {
            // TODO: Implement FIFO "Clock-Like" Selection Logic
            printf("Todo!\n");
        }
        else
        {
            // Retrieve the frame_t from free list
            free_frame_num = free_list.front();

            // Remove value from free list
            free_list.pop_front();
            free_frame = &FRAME_TABLE[free_frame_num];
        }
        return free_frame;
    };
};

// Helper function to build pager based on CLI input
Pager *build_pager(PAGER_TYPES pager_type, int NUM_FRAMES)
{
    switch (pager_type)
    {
    case FIFO:
        return (Pager *)new FIFO_Pager(NUM_FRAMES);
    case Random:
        throw new NotImplemented;
    case Clock:
        throw new NotImplemented;
    case ESC_NRU:
        throw new NotImplemented;
    case Aging:
        throw new NotImplemented;
    case Working_Set:
        throw new NotImplemented;
    }
}
#endif