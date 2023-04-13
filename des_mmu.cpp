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
    bool O, P, F, S, x, y, a;
    const char *optional_args;
    int c;
    int NUM_FRAMES;
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

        case 'o':
            optional_args = optarg;
            break;

        case '?':
            fprintf(stderr,
                    "usage: %s [dcs<size>]\n", argv[0]);
            return 1;
        }
    }

    // Parse optional args
    for (int i = 0; optional_args[i] != NULL; i++)
    {
        switch (std::toupper(optional_args[i]))
        {
        case 'O':
            O = true;
            break;
        case 'P':
            P = true;
            break;
        case 'F':
            F = true;
            break;
        case 'S':
            S = true;
            break;
        case 'X':
            x = true;
            break;
        case 'Y':
            y = true;
            break;
        case 'A':
            a = true;
            break;
        }
    }

    // Grab input file name, random file name
    inputfile_name = argv[optind];
    randfile_name = argv[optind + 1];

    // To delete
    printf("Num Frames: %d, Sched Type: %s, Input Filename: %s, Rfile Name: %s\n", NUM_FRAMES, char_sched_type, inputfile_name.c_str(), randfile_name.c_str());
    printf("Optional Args: %s: A %d Y %d X %d S %d F %d P %d O %d \n", optional_args, a, y, x, S, F, P, O);

    // Process rfile
    int r_array_size;
    std::ifstream rfile;
    // rfile.open(randfile_name);

    return 0;
}