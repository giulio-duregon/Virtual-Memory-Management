#include "data_structures.hpp"
#include <stdexcept>
#include <deque>

#ifndef MMU_PAGERS
#define MMU_PAGERS

/* You have to compute the overall execution time in cycles,
where the cost of operations (in terms of cycles)
are as follows: read/write (load/store) instructions count as 1,
context_switches instructions=130, process exits instructions=1230.*/

const enum PAGER_CYCLES {
    READ_WRITE = 1,
    CONTEXT_SWITCH = 130,
    PROC_EXIT = 1230,
};

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
    Pager(int NUM_FRAMES_, PAGER_TYPES ptype_, bool O_)
    {
        NUM_FRAMES = NUM_FRAMES_;
        ptype = ptype_;
        O = O_;

        // Dynamically create the frame table array based on input args
        FRAME_TABLE = new frame_t[NUM_FRAMES];

        // Upon Initialization, All frames are free
        for (int i = 0; i < NUM_FRAMES; i++)
        {
            // Assing a frame number
            FRAME_TABLE[i].frame_number = i;

            // Add it to the free frame queue
            free_list.push_back(&FRAME_TABLE[i]);
        }
    };

    void init_process_metadata(int num_processes_, Process *process_arr_)
    {
        process_arr = process_arr_;
        num_processes = num_processes_;
    }

    // Main Functionality: Get a frame from the free frames queue
    // If one does not exist, call select_victim_frame
    frame_t *get_frame()
    {
        // If we have no free frames, select next victim frame
        if (free_list.empty())
        {
            return select_victim_frame();
        }

        // If we have free frames, pop & return the first one off
        frame_t *free_frame = free_list.front();
        free_list.pop_front();
        return free_frame;
    }

    // Virtual Function to be implemented by derived classes
    // Selects VMA to be removed from physical frame
    // Physical Frame gets added to the free list
    virtual frame_t *select_victim_frame() { throw new NotImplemented; };

    // Maps a physical frame to a VMA page
    // pte_t struct -> frame_t struct
    void map_frame(Process *process, int vpage_num, frame_t *free_frame)
    {
        process->allocate_cost(MAPS);
        pte_t *vpage = process->get_vpage(vpage_num);
        // Update VMA page mapping
        vpage->frame_number = free_frame->frame_number;

        // TODO: Some other logic about modified / referenced bits
        vpage->PRESENT = 1;
        vpage->REFERENCED = 1;

        // See if page has never been accessed, and is not file mapped
        if ((vpage->NOT_FIRST_ACCESS == 0) && (vpage->FILEMAPPED == 0))
        {
            if (O)
            {
                printf("ZERO\n");
            }
            // An operating system must zero pages on first access (unless filemapped) to guarantee consistent behavior
            process->allocate_cost(ZEROS);
            vpage->NOT_FIRST_ACCESS = 1;
        }
        // Update physical frame to reverse map to page
        FRAME_TABLE[free_frame->frame_number].process_id = process->get_pid();
        FRAME_TABLE[free_frame->frame_number].VMA_page_number = vpage_num;

        // If output option, display filenumber that is mapped
        if (O)
        {
            printf("MAP %d\n", free_frame->frame_number);
        }
    };

    void unmap_frame(unsigned int pid, unsigned int old_page_num)
    {
        // Get correct pointer to victim process + frame to be unmapped
        Process *process = &process_arr[pid];
        process->allocate_cost(UNMAPS);
        pte_t *page = process->get_vpage(old_page_num);

        // Retrieve Physical Frame Number
        int frame_num = page->frame_number;
        int vpage = FRAME_TABLE[frame_num].VMA_page_number;

        if (O)
        {
            printf("UNMAP %d:%d\n", pid, vpage);
        }

        // TODO: Implement logic to handle page getting unmapped from frame
        //  i.e. what to do if modified, referenced etc.

        // If modified it must be written out
        if (page->MODIFIED)
        {
            if (page->FILEMAPPED)
            {
                process->allocate_cost(FOUTS);
            }
            else
            {
                process->allocate_cost(OUTS);
            }

            // Reset modified bit
            page->MODIFIED = 0;

            // Set PAGEDOUT bit
            page->PAGEDOUT = 1;
        }
        // Clear Physical Frame mapping
        clear_mapping(frame_num);

        page->PRESENT = 0;
    };

    // Clears previous physical frames (reverse) mapping
    // To a process id / virtual frame number
    void clear_mapping(int frame_number)
    {
        // Reset frame Numbers
        FRAME_TABLE[frame_number].process_id = -1;
        FRAME_TABLE[frame_number].VMA_page_number = -1;
    }

    void allocate_cost(PAGER_CYCLES cost_type)
    {
        switch (cost_type)
        {
        case READ_WRITE:
            inst_count += READ_WRITE;
            break;
        case CONTEXT_SWITCH:
            ctx_switches += CONTEXT_SWITCH;
            break;
        case PROC_EXIT:
            process_exits += PROC_EXIT;
            break;
        }
    }

    void add_frame_to_free_list(frame_t *free_frame)
    {
        free_list.push_back(free_frame);
    }

    void add_frame_to_free_list(int frame_num)
    {
        frame_t *temp = &FRAME_TABLE[frame_num];
        free_list.push_back(temp);
    }

    // Output as described in the docs
    void print_total_cost()
    {
        // Incrementally add to avoid overflow
        cost += inst_count;
        cost += ctx_switches;
        cost += process_exits;

        // Print out total cost information
        printf("TOTALCOST %lu %lu %lu %llu %lu\n",
               inst_count, ctx_switches, process_exits, cost, sizeof(pte_t));
    }

    PAGER_TYPES ptype;

    void print_frame_table()
    {
        printf("FT:");
        for (int i = 0; i < NUM_FRAMES; i++)
        {
            int pid = FRAME_TABLE[i].process_id;
            int page_num = FRAME_TABLE[i].VMA_page_number;
            if (pid == -1 || page_num == -1)
            {
                printf(" * ");
            }
            else
            {
                printf(" %d:%d ", pid, page_num);
            }
        }
        printf("\n");
    }

    void print_process_ptes()
    {
        for (int i = 0; i < num_processes; i++)
        {
            printf("PT[%d]", i);
            process_arr[i].print_process_table();
            printf("\n");
        }
    }

    void print_per_process_stats()
    {
        for (int i = 0; i < num_processes; i++)
        {
            printf("PROC[%d]: ", i);
            process_arr[i].print_stats();
        }
    }

protected:
    int NUM_FRAMES;
    bool O;
    frame_t *FRAME_TABLE;
    std::deque<frame_t *> free_list;
    unsigned long long cost = 0;
    unsigned long inst_count = 0;
    unsigned long ctx_switches = 0;
    unsigned long process_exits = 0;
    Process *process_arr;
    int num_processes = 0;
};

// FIFO Pager Implementation
class FIFO_Pager : Pager
{
public:
    FIFO_Pager(int NUM_FRAMES, bool O, bool a_) : Pager(NUM_FRAMES, FIFO, O) { a = a_; };
    frame_t *select_victim_frame()
    {
        // Select victim frame in clocklike fashion indexing into Frame Table
        frame_t *free_frame = nullptr;
        free_frame = &FRAME_TABLE[CLOCK_HAND];

        // If option selected, output victim frame
        if (a)
        {
            printf("ASELECT: %d\n", CLOCK_HAND);
        }
        increment_clock_hand();
        return free_frame;
    };

private:
    bool a;
    void increment_clock_hand()
    {
        CLOCK_HAND++;
        if (CLOCK_HAND >= NUM_FRAMES)
        {
            CLOCK_HAND = 0;
        }
    }
    unsigned int CLOCK_HAND = 0;
};

// Helper function to build pager based on CLI input
Pager *build_pager(PAGER_TYPES pager_type, int NUM_FRAMES, bool O, bool a)
{
    switch (pager_type)
    {
    case FIFO:
        return (Pager *)new FIFO_Pager(NUM_FRAMES, O, a);
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