#include <iostream>
#include <fstream>
#include <string>
#include <getopt.h>

void update_offset(int &offset, int array_size)
{
    offset++;
    // Increment offset and if we pass the array size loop back around
    if (offset >= array_size)
    {
        offset = 0;
    }
}

int rand_burst(int burst, int *randvals, int &offset, int array_size)
{
    if (offset >= array_size)
    {
        offset = 0;
    }
    // Grab random value
    return 1 + (randvals[offset] % burst);
}

int main(int argc, char **argv)
{
    /* ################### Config Instructions ############################################
    ---------------------------------------------------------------------------------------
    The ‘O’ (ooooh nooooh) option shall generate the required output as shown in output-1/3.
    The ‘P’ (pagetable option) should print after the execution of all instructions the state of the pagetable:
    As a single line for each process, you print the content of the pagetable pte entries.
    R (referenced), M (modified), S (swapped out) (note we don’t show the write protection bit as it is implied/inherited from the specified VMA.
    The ‘F’ (frame table option) should print after the execution and should show which frame is mapped at the end to which
    <pid:virtual page> or ‘*’ if not currently mapped by any virtual page.
    The ‘S’ option prints per process statistics “PROC[i]” and the summary line (“TOTALCOST ...”) described above.
    The ‘x’ option prints the current page table after each instructions (see example outputs) and this should help you
    significantly to track down bugs and transitions (remember you write the print function only once)
    The ‘y’ option is like ‘x’ but prints the page table of all processes instead.
    The ‘f’ option prints the frame table after each instruction.
    The ‘a’ option prints additional “aging” information during victim_selection and after each instruction for complex
    algorithms (not all algorithms have the details described in more detail below)
    -------------------------------------------------------------------------------
    */
    bool x, y, o;
    int NUM_FRAMES;
    bool O, P, R, M, S;
    int c;
    char *char_sched_type = nullptr;
    char *s = nullptr;
    std::string inputfile_name;
    std::string randfile_name;
    std::string line;

    // Arg parsing
    while ((c = getopt(argc, argv, "f:a:o:xy")) != -1)
    {
        switch (c)
        {
        case 'f':
            NUM_FRAMES = atoi(optarg);
            break;
        case 'a':
            char_sched_type = optarg;
            break;
        case 'x':
            x = true;
            break;
        case 'y':
            y = true;
            break;

        case 'o':
            break;

        case 's':
            s = optarg;
            break;

        case '?':
            fprintf(stderr,
                    "usage: %s [dcs<size>]\n", argv[0]);
            return 1;
        }
    }

    // Grab input file name, random file name
    inputfile_name = argv[optind];
    randfile_name = argv[optind + 1];

    // Process rfile
    int r_array_size;
    std::ifstream rfile;
    // rfile.open(randfile_name);
    printf("Num Frames: %d, Sched Type: %s\n", NUM_FRAMES, char_sched_type);
    return 0;
}