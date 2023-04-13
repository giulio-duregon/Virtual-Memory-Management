#include "data_structures.hpp"
#include <stdexcept>

#ifndef MMU_PAGERS
#define MMU_PAGERS
enum PAGER_TYPES
{
    FIFO,
    Random,
    Clock,
    ESC_NRU,
    Aging,
    Working_Set
};

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
    Pager(int NUM_FRAMES)
    {
        FRAME_TABLE = new pte_t[NUM_FRAMES];
    };
    virtual frame_t *select_victim_frame();
    pte_t *unmap_frame(frame_t frame);
    // Vpage -> frame
    void *map_frame(pte_t vpage);

protected:
    pte_t *FRAME_TABLE;
};

// FIFO Pager Implementation
class FIFO_Pager : Pager
{
public:
    FIFO_Pager(int NUM_FRAMES) : Pager(NUM_FRAMES){};
    frame_t *select_victim_frame();
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