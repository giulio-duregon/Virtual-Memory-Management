#include "data_structures.hpp"
#include <stdexcept>
#include <deque>

#ifndef MMU_PAGERS
#define MMU_PAGERS

/* You have to compute the overall execution time in cycles,
where the cost of operations (in terms of cycles)
are as follows: read/write (load/store) instructions count as 1,
context_switches instructions=130, process exits instructions=1230.*/

enum PAGER_CYCLES
{
    READ_WRITE = 1,
    CONTEXT_SWITCH = 130,
    PROC_EXIT = 1230,
};

enum ESC_NRU_PAGE_CLASSES
{
    CLASS_0 = 0,
    CLASS_1 = 1,
    CLASS_2 = 2,
    CLASS_3 = 3,
};

enum WS_FRAME_CLASSES
{
    WS_CLASS_0 = 0,
    WS_CLASS_1 = 1,
    WS_CLASS_2 = 2,
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
}

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
    Pager(PAGER_TYPES ptype_, unsigned int NUM_FRAMES_, bool O_, bool a_)
    {
        NUM_FRAMES = NUM_FRAMES_;
        ptype = ptype_;
        O = O_;
        a = a_;

        // Dynamically create the frame table array based on input args
        FRAME_TABLE = new frame_t[NUM_FRAMES];

        // Upon Initialization, All frames are free
        for (int i = 0; i < NUM_FRAMES; i++)
        {
            // Assing a frame number
            FRAME_TABLE[i].frame_number = i;
            FRAME_TABLE[i].age = 0;

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
    virtual void map_frame(Process *process, int vpage_num, frame_t *free_frame)
    {
        // Allocate cost of MAPS + grab vpage
        process->allocate_cost(MAPS);
        pte_t *vpage = process->get_vpage(vpage_num);

        // Update VMA page mapping
        vpage->frame_number = free_frame->frame_number;

        // Update present / referenced / exist bits
        vpage->PRESENT = 1;
        vpage->REFERENCED = 1;
        vpage->EXISTS = 1;

        // See if reading in from file mapped page
        if (vpage->FILEMAPPED)
        {
            process->allocate_cost(FINS);
            if (O)
            {
                printf(" FIN\n");
            }
        }
        // See if we're reading from swap disk
        else if (vpage->PAGEDOUT)
        {
            // Just bill / print the correct amount based on File Mapping
            process->allocate_cost(INS);
            if (O)
            {
                printf(" IN\n");
            }
        }
        // Otherwise: An operating system must zero pages on first access (unless filemapped) to guarantee consistent behavior
        else
        {
            if (O)
            {
                printf(" ZERO\n");
            }
            process->allocate_cost(ZEROS);
        }
        // Update physical frame to reverse map to page
        FRAME_TABLE[free_frame->frame_number].process_id = process->get_pid();
        FRAME_TABLE[free_frame->frame_number].VMA_page_number = vpage_num;

        // If output option, display filenumber that is mapped
        if (O)
        {
            printf(" MAP %d\n", free_frame->frame_number);
        }
    };

    virtual void unmap_frame(unsigned int pid, unsigned int old_page_num)
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
            printf(" UNMAP %d:%d\n", pid, vpage);
        }

        // If modified it must be written out
        if (page->MODIFIED)
        {
            if (page->FILEMAPPED)
            {
                process->allocate_cost(FOUTS);
                if (O)
                {
                    printf(" FOUT\n");
                }
            }
            else
            {
                process->allocate_cost(OUTS);
                // Set PAGEDOUT bit
                page->PAGEDOUT = 1;
                if (O)
                {
                    printf(" OUT\n");
                }
            }

            // Reset modified bit
            page->MODIFIED = 0;
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
            inst_count++;
            break;
        case CONTEXT_SWITCH:
            inst_count++;
            ctx_switches++;
            break;
        case PROC_EXIT:
            inst_count++;
            process_exits++;
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
        cost += inst_count - process_exits - ctx_switches;
        cost += ctx_switches * CONTEXT_SWITCH;
        cost += process_exits * PROC_EXIT;
        for (int i = 0; i < num_processes; i++)
        {
            cost += process_arr[i].calc_total_cost();
        }

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
                printf(" *");
            }
            else
            {
                printf(" %d:%d", pid, page_num);
            }
        }
        printf("\n");
    }

    void print_process_ptes()
    {
        for (int i = 0; i < num_processes; i++)
        {
            printf("PT[%d]:", i);
            process_arr[i].print_process_table();
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
    int CLOCK_HAND = 0;
    int query_len = 0;
    unsigned int NUM_FRAMES = 0;
    bool O = false;
    bool a = false;
    frame_t *FRAME_TABLE;
    std::deque<frame_t *> free_list;
    unsigned long long cost = 0;
    unsigned long inst_count = 0;
    unsigned long ctx_switches = 0;
    unsigned long process_exits = 0;
    Process *process_arr;
    int num_processes = 0;
    void increment_clock_hand()
    {
        CLOCK_HAND++;
        if (CLOCK_HAND >= NUM_FRAMES)
        {
            CLOCK_HAND = 0;
        }
    }
};

// FIFO Pager Implementation
class FIFO_Pager : Pager
{
public:
    FIFO_Pager(int NUM_FRAMES, bool O, bool a) : Pager(FIFO, NUM_FRAMES, O, a){};
    frame_t *select_victim_frame()
    {
        // Select victim frame in clocklike fashion indexing into Frame Table
        frame_t *free_frame = nullptr;
        free_frame = &FRAME_TABLE[CLOCK_HAND];
        increment_clock_hand();
        // If option selected, output victim frame
        if (a)
        {
            printf("ASELECT %d\n", free_frame->frame_number);
        }
        return free_frame;
    };
};

// Random Algorithm Implementation

class Random_Pager : Pager
{
public:
    Random_Pager(int NUM_FRAMES, int array_size_, int *randvals_, bool O, bool a) : Pager(Random, NUM_FRAMES, O, a)
    {
        array_size = array_size_;
        randvals = randvals_;
    };

    frame_t *select_victim_frame()
    {
        // Select victim frame in clocklike fashion indexing into Frame Table
        frame_t *free_frame = nullptr;
        int randval = gen_randval();
        free_frame = &FRAME_TABLE[randval];

        // If option selected, output victim frame
        if (a)
        {
            printf("ASELECT %d\n", free_frame->frame_number);
        }
        return free_frame;
    }

private:
    int offset = 0;
    int array_size = 0;
    int *randvals;

    int gen_randval()
    {
        if (offset >= array_size)
        {
            offset = 0;
        }
        int victim_frame = (randvals[offset] % NUM_FRAMES);
        offset++;
        // Grab random value
        return victim_frame;
    }
};

// FIFO Pager Implementation
class Clock_Pager : Pager
{
public:
    Clock_Pager(int NUM_FRAMES, bool O, bool a) : Pager(Clock, NUM_FRAMES, O, a){};

    frame_t *select_victim_frame()
    {
        // Helper variables
        Process *temp;
        query_len = 0;

        // Select victim frame in clocklike fashion indexing into Frame Table
        frame_t *free_frame = nullptr;
        while (!free_frame)
        {
            // Select candidate victim frame
            free_frame = &FRAME_TABLE[CLOCK_HAND];

            // Grab relevant Process / Frame
            temp = &process_arr[free_frame->process_id];
            pte_t *page = temp->get_vpage(free_frame->VMA_page_number);

            // Inspect Referenced bit
            if (page->REFERENCED)
            {
                // If it has been referenced, reset the R bit
                page->REFERENCED = 0;

                // Reset free_frame to keep looping
                free_frame = nullptr;
            }

            // No Else statement needed, if free_frame NOTEQ nullptr
            // Then loop exits and free frame returned
            increment_clock_hand();
            query_len++;
        }
        // If option selected, output victim frame
        if (a)
        {
            printf("ASELECT %d %d\n", free_frame->frame_number, query_len);
        }
        return free_frame;
    };

    int get_query_len()
    {
        return query_len;
    }
};

// ESC_NRU Pager
class ESC_NRU_Pager : Pager
{
public:
    ESC_NRU_Pager(int NUM_FRAMES, bool O, bool a) : Pager(ESC_NRU, NUM_FRAMES, O, a){};

    frame_t *select_victim_frame()
    {
        bool reset_ref = check_reset_ref_bit();
        int start_hand_pos = CLOCK_HAND;
        int victim_class = 0;
        query_len = 0;
        Process *temp;
        frame_t *free_frame = nullptr;
        frame_t *potential_victim_frame = nullptr;

        // if reset_ref we must scan all entries and reset all bits
        // Search frame list -> O(n) where n = NUM_FRAMES
        while (!free_frame)
        {
            // Check if we've already completed a whole frame scan
            // Each frame only needs to be visited once
            if (start_hand_pos == CLOCK_HAND && (query_len > 0))
            {
                break;
            }

            // Select candidate victim frame
            potential_victim_frame = &FRAME_TABLE[CLOCK_HAND];

            // Grab relevant Process / Frame
            temp = &process_arr[potential_victim_frame->process_id];
            pte_t *page = temp->get_vpage(potential_victim_frame->VMA_page_number);

            // First check for class 0
            if (is_class_zero(page))
            {
                free_frame = potential_victim_frame;
                victim_class = CLASS_0;
            }

            // Otherwise check what class
            else
            {
                ESC_NRU_PAGE_CLASSES page_class = get_class(page);

                // Update the class pointer if we haven't found one like it
                switch (page_class)
                {
                case CLASS_1:
                    if (class_one_frame == nullptr)
                    {
                        class_one_frame = potential_victim_frame;
                    }
                    break;
                case CLASS_2:
                    if (class_two_frame == nullptr)
                    {
                        class_two_frame = potential_victim_frame;
                    }
                    break;
                case CLASS_3:
                    if (class_three_frame == nullptr)
                    {
                        class_three_frame = potential_victim_frame;
                    }
                    break;
                }
            }

            // Reset the referenced bit if that's whats required
            if (reset_ref)
            {
                page->REFERENCED = 0;

                // If we've found a free frame, finish resetting all bits
                if (free_frame)
                {
                    finish_resetting_ref_bit(start_hand_pos);
                }
            }

            // Increment hand and continue
            increment_clock_hand();
            query_len++;
        }

        // If we haven't found a Class 0 frame and we've visited all frames, select lowest class as vitcim
        if (free_frame == nullptr)
        {
            if (class_one_frame != nullptr)
            {
                free_frame = class_one_frame;
                victim_class = CLASS_1;
                increment_clock_hand();
            }
            else if (class_two_frame != nullptr)
            {
                free_frame = class_two_frame;
                victim_class = CLASS_2;
                increment_clock_hand();
            }
            else
            {
                free_frame = class_three_frame;
                victim_class = CLASS_3;
                increment_clock_hand();
            }
        }

        // Output desired information
        if (a)
        {
            printf("ASELECT hand=%2d %d | %d %2d %2d\n", start_hand_pos, reset_ref, victim_class, free_frame->frame_number, query_len);
        }

        // Increment hand before next invocation, clear class pointers
        clear_class_pointers();
        CLOCK_HAND = free_frame->frame_number + 1;

        if (CLOCK_HAND >= NUM_FRAMES)
        {
            CLOCK_HAND = 0;
        }
        // Return free frame
        return free_frame;
    }

    // Helper function that continues sweep if class=0 is found early
    // But frames still need to be traversed to reset all reference bits  to 0
    void finish_resetting_ref_bit(int starting_pos)
    {
        // Helper vars
        int starting_clock_hand = CLOCK_HAND;
        frame_t *free_frame;
        Process *temp;
        pte_t *page;

        // Increment the clock hand before and after for desired behvaior
        increment_clock_hand();

        while (CLOCK_HAND != starting_pos)
        {
            // Select candidate victim frame
            free_frame = &FRAME_TABLE[CLOCK_HAND];

            // Grab relevant Process / Frame
            temp = &process_arr[free_frame->process_id];
            page = temp->get_vpage(free_frame->VMA_page_number);

            // Reset R bit
            page->REFERENCED = 0;
            increment_clock_hand();
        }

        // Reset clock hand to where it was as it was entered
        CLOCK_HAND = starting_clock_hand;
    }

    // Helper function to see if enough instructions have passed
    // that we should reset the reference bit
    bool check_reset_ref_bit()
    {
        // First instruction is instruction 0, makin this an annoying off-by 1 error
        if ((inst_count - last_sweep_inst_count) >= (RESET_REFBIT_THRESHOLD))
        {
            last_sweep_inst_count = inst_count;
            return true;
        }
        return false;
    }

    // Helper function to determine if page belongs to class 0
    bool is_class_zero(pte_t *page)
    {
        if ((!page->MODIFIED) && (!page->REFERENCED))
        {
            return true;
        }
        return false;
    }

    // Get page classes (1-3) from page table entry
    ESC_NRU_PAGE_CLASSES get_class(pte_t *page)
    {
        if ((!page->REFERENCED) && (page->MODIFIED))
        {
            return CLASS_1;
        }
        else if ((page->REFERENCED) && (!page->MODIFIED))
        {
            return CLASS_2;
        }
        else if ((page->REFERENCED) && (page->MODIFIED))
        {
            return CLASS_3;
        }
    }

    // Helper function to clear class pointers between invocations
    void clear_class_pointers()
    {
        class_one_frame = nullptr;
        class_two_frame = nullptr;
        class_three_frame = nullptr;
    }

private:
    const int RESET_REFBIT_THRESHOLD = 50;
    unsigned long last_sweep_inst_count = 0;
    frame_t *class_one_frame = nullptr;
    frame_t *class_two_frame = nullptr;
    frame_t *class_three_frame = nullptr;

    void increment_clock_hand()
    {
        CLOCK_HAND++;
        if (CLOCK_HAND >= NUM_FRAMES)
        {
            CLOCK_HAND = 0;
        }
    }
};

class Aging_Pager : Pager
{
public:
    Aging_Pager(int NUM_FRAMES, bool O, bool a) : Pager(Aging, NUM_FRAMES, O, a){};

    frame_t *select_victim_frame()
    {
        // Helper vars
        int start_hand_pos = CLOCK_HAND;
        frame_t *free_frame = nullptr;
        frame_t *temp_youngest = nullptr;
        unsigned int youngest_age = 0;

        // Rest query len
        query_len = 0;

        if (a)
        {
            if (start_hand_pos == 0)
            {
                printf("ASELECT %d-%d | ", start_hand_pos, NUM_FRAMES - 1);
            }
            else
            {
                printf("ASELECT %d-%d | ", start_hand_pos, start_hand_pos - 1);
            }
        }
        // Iterate over all frames only once
        while (!free_frame)
        {
            if (start_hand_pos == CLOCK_HAND && (query_len > 0))
            {
                break;
            }

            // Grab frame / age
            frame_t *potential_victim_frame = &FRAME_TABLE[CLOCK_HAND];

            // Age the frame we just considered
            age_frame(potential_victim_frame);
            unsigned int potential_victim_age = potential_victim_frame->age;
            if (a)
            {
                printf("%d:%X ", potential_victim_frame->frame_number, potential_victim_frame->age);
            }

            // If this is the first, it counts as the youngest (temporarily)
            if (youngest_age == 0)
            {
                youngest_age = potential_victim_age;
                temp_youngest = potential_victim_frame;
            }
            else
            {
                // Update youngest found if current is younger
                if (potential_victim_age < youngest_age)
                {
                    youngest_age = potential_victim_age;
                    temp_youngest = potential_victim_frame;
                }
            }

            // Keep searching
            increment_clock_hand();
            query_len++;
        }

        free_frame = temp_youngest;
        // Reset clock hand and return free frame
        CLOCK_HAND = free_frame->frame_number;
        increment_clock_hand();

        if (a)
        {
            printf("| %d\n", free_frame->frame_number);
        }

        return free_frame;
    };

    void map_frame(Process *process, int vpage_num, frame_t *free_frame)
    {
        // Reset Age every MAP operation
        free_frame->age = 0;

        Pager::map_frame(process, vpage_num, free_frame);
    }

    void unmap_frame(unsigned int pid, unsigned int old_page_num)
    {
        Pager::unmap_frame(pid, old_page_num);
    }

private:
    void age_frame(frame_t *potential_victim_frame)
    {
        // Get VPAGE
        Process *temp = &process_arr[potential_victim_frame->process_id];
        pte_t *page = temp->get_vpage(potential_victim_frame->VMA_page_number);

        // Shift Right
        shift_age_right(potential_victim_frame);

        // Add R bit to leading
        if (page->REFERENCED)
        {
            set_leading_bit_to_one(potential_victim_frame);
        }
        // If its 1, must be reset to 0
        page->REFERENCED = 0;
    }

    inline void set_leading_bit_to_one(frame_t *frame)
    {
        frame->age = frame->age | 0x80000000;
    }

    inline void shift_age_right(frame_t *frame)
    {
        frame->age = frame->age >> 1;
    }

    void increment_clock_hand()
    {
        CLOCK_HAND++;
        if (CLOCK_HAND >= NUM_FRAMES)
        {
            CLOCK_HAND = 0;
        }
    }
};

class Working_Set_Pager : Pager
{
public:
    Working_Set_Pager(int NUM_FRAMES, bool O, bool a) : Pager(Working_Set, NUM_FRAMES, O, a){};

    frame_t *select_victim_frame()
    {
        // Helper vars
        frame_t *free_frame = nullptr;
        int start_hand_pos = CLOCK_HAND;
        query_len = 0;

        // Keep track of candidate victim frames
        frame_t *oldest_class_one_frame = nullptr;
        frame_t *oldest_class_two_frame = nullptr;
        unsigned int oldest_class_one_age = 0;
        unsigned int oldest_class_two_age = 0;

        // Option for verbose output
        if (a)
        {
            if (start_hand_pos == 0)
            {
                printf("ASELECT %d-%d | ", start_hand_pos, NUM_FRAMES - 1);
            }
            else
            {
                printf("ASELECT %d-%d | ", start_hand_pos, start_hand_pos - 1);
            }
        }

        // Begin our search for our victim frame
        while (!free_frame)
        {
            // Make sure we only visit each frame only once
            if (start_hand_pos == CLOCK_HAND && (query_len > 0))
            {
                break;
            }

            // Helper variables
            frame_t *potential_victim_frame = &FRAME_TABLE[CLOCK_HAND];
            Process *temp_proc = &process_arr[potential_victim_frame->process_id];
            pte_t *page = temp_proc->get_vpage(potential_victim_frame->VMA_page_number);

            // Verbose output print options
            if (a)
            {
                printf("%d(%d %d:%d %d) ", CLOCK_HAND, page->REFERENCED, potential_victim_frame->process_id,
                       potential_victim_frame->VMA_page_number, potential_victim_frame->age);
            }

            // Check if we've found a frame with class = 0
            // Class = 0 means frame age >= TAU and REF = 0
            if (is_class_zero(page, potential_victim_frame))
            {
                free_frame = potential_victim_frame;
                if (a)
                {
                    printf("STOP(%d) ", query_len);
                }

                break;
            }
            else
            {
                // Otherwise get frame class and decide what to do
                WS_FRAME_CLASSES frame_class = get_frame_ws_class(page, potential_victim_frame);

                switch (frame_class)
                {
                // Class 1: A unreferenced page with age < TAU
                case WS_CLASS_1:
                    // If this is the first frame we've seen of this class, by default it is the oldest
                    if (oldest_class_one_age == 0)
                    {
                        oldest_class_one_age = potential_victim_frame->age;
                        oldest_class_one_frame = potential_victim_frame;
                    }
                    else
                    {
                        // Otherwise, check if oldest needs to be updated
                        if (potential_victim_frame->age < oldest_class_one_age)
                        {
                            oldest_class_one_age = potential_victim_frame->age;
                            oldest_class_one_frame = potential_victim_frame;
                        }
                    }
                    break;

                // Class 2, a referenced page
                case WS_CLASS_2:
                    if (!oldest_class_two_frame)
                    {
                        oldest_class_two_age = potential_victim_frame->age;
                        oldest_class_two_frame = potential_victim_frame;
                    }

                    // Reset R bit and set age
                    mark_frame_and_page(page, potential_victim_frame);
                    break;
                }
            }

            // Keep track of how many frames we're visiting / move hand
            query_len++;
            increment_clock_hand();
        }

        // If we didn't find a frame in the loop, then we select the youngest
        if (!free_frame)
        {
            if (oldest_class_one_frame)
            {
                free_frame = oldest_class_one_frame;
            }
            else
            {
                free_frame = oldest_class_two_frame;
            }
        }

        // Set clock hand back to appropriate spot
        CLOCK_HAND = free_frame->frame_number;
        increment_clock_hand();
        if (a)
        {
            printf("| %d\n", free_frame->frame_number);
        }

        return free_frame;
    }

    void map_frame(Process *process, int vpage_num, frame_t *free_frame)
    {
        // When mapping a frame set its age to instruction count
        // -1 as inst_count is incremented before frames are allocated
        // due to the accounting i've set up with billing read/writes
        free_frame->age = inst_count - 1;

        Pager::map_frame(process, vpage_num, free_frame);
    }

private:
    const unsigned int TAU = 49;

    // Helper function to determine if page belongs to class 0
    bool is_class_zero(pte_t *page, frame_t *frame)
    {
        if ((!page->REFERENCED) && (inst_count - frame->age - 1) > TAU)
        {
            return true;
        }
        return false;
    }

    // Helper function to get class of frame
    WS_FRAME_CLASSES get_frame_ws_class(pte_t *page, frame_t *frame)
    {
        if (page->REFERENCED)
        {
            return WS_CLASS_2;
        }
        return WS_CLASS_1;
    }

    // Helper function to reset REF bit and set age
    inline void mark_frame_and_page(pte_t *page, frame_t *frame)
    {
        page->REFERENCED = 0;
        // -1 as count gets incremented on read/write allocation before frame is allocated
        frame->age = inst_count - 1;
    }
};

// Helper function to build pager based on CLI input
Pager *build_pager(PAGER_TYPES pager_type, int NUM_FRAMES, int array_size, int *randvals, bool O, bool a)
{
    switch (pager_type)
    {
    case FIFO:
        return (Pager *)new FIFO_Pager(NUM_FRAMES, O, a);
    case Random:
        return (Pager *)new Random_Pager(NUM_FRAMES, array_size, randvals, O, a);
    case Clock:
        return (Pager *)new Clock_Pager(NUM_FRAMES, O, a);
    case ESC_NRU:
        return (Pager *)new ESC_NRU_Pager(NUM_FRAMES, O, a);
    case Aging:
        return (Pager *)new Aging_Pager(NUM_FRAMES, O, a);
    case Working_Set:
        return (Pager *)new Working_Set_Pager(NUM_FRAMES, O, a);
    }
}
#endif