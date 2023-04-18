#include <iostream>
#include <fstream>
#include <string>
#include <getopt.h>
#include "data_structures.hpp"
#include "mmu_pagers.hpp"

void update_offset(int &offset, int array_size)
{
    offset++;
    // Increment offset and if we pass the array size loop back around
    if (offset >= array_size)
    {
        offset = 0;
    }
}

int rand_burst(int frame_t_size, int *randvals, int &offset, int array_size)
{
    if (offset >= array_size)
    {
        offset = 0;
    }
    // Grab random value
    return (randvals[offset] % frame_t_size);
}

void read_write_logic(Pager *THE_PAGER, Process *CURRENT_PROCESS, const int vpage)
{
    // Add Read/Write cycle cost to pager for accounting
    THE_PAGER->allocate_cost(READ_WRITE);

    if (!CURRENT_PROCESS->check_present_valid(vpage))
    {
        // Page fault logic
        if (!CURRENT_PROCESS->vpage_can_be_accessed(vpage))
        {
            // TODO: Output SEGV Line?

            // Allocate cost of a segmentation violation
            CURRENT_PROCESS->allocate_cost(SEGV);
            return;
        }
        else
        {
            // Page can be accessed, so it must be allocated
            frame_t *frame = THE_PAGER->get_frame();

            // See if the frame is coming from free frames or victim frames
            if (frame->process_id != -1)
            {
                // Unmap Victim Frame
                THE_PAGER->unmap_frame(frame->process_id, frame->VMA_page_number);
            }

            // Update referenced bit, frame number on VPage
            THE_PAGER->map_frame(CURRENT_PROCESS, vpage, frame);
        }
    }
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
    std::string inputfile_name;
    std::string randfile_name;
    std::string line;
    Pager *THE_PAGER;
    Process *process_arr = nullptr;

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
    for (int i = 0; optional_args[i] != '\0'; i++)
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

    // TODO: To delete
    // printf("Num Frames: %d, Sched Type: %s, Input Filename: %s, Rfile Name: %s\n", NUM_FRAMES, char_sched_type, inputfile_name.c_str(), randfile_name.c_str());
    // printf("Optional Args: %s: A %d Y %d X %d S %d F %d P %d O %d \n", optional_args, a, y, x, S, F, P, O);

    // Process rfile
    int r_array_size;
    std::ifstream rfile;
    rfile.open(randfile_name);

    // Get Random Array Size
    rfile >> r_array_size;

    // Throw all the values of the file into array
    int offset = 0;
    int *randvals{new int[r_array_size]{}};
    for (int i = 0; i < r_array_size; i++)
    {
        rfile >> randvals[i];
    }
    rfile.close();

    // Initialize Pager Algorithm from Input
    PAGER_TYPES pager_type = parse_pager_type_from_input(char_sched_type);
    THE_PAGER = build_pager(pager_type, NUM_FRAMES, O, a);

    // TODO: DELETE
    // printf("Pager Algo (Enum): %d Pager Algo (Name): %s\n", THE_PAGER->ptype, GET_PAGER_NAME_FROM_ENUM(THE_PAGER->ptype));

    // Helper Variables for Process / VMA Construction
    std::ifstream input_file(inputfile_name);
    unsigned int num_processes = 0;
    unsigned int vma_lines_to_read = 0;
    int process_read_count = 0;
    unsigned int start_vpage = 0;
    unsigned int end_vpage = 0;
    unsigned int write_protected = 0;
    unsigned int file_mapped = 0;

    // Read in input from file -> Read in Num Process -> Make VMAs
    while (getline(input_file, line))
    {
        // Ignore line comments
        if (line.c_str()[0] != '#')
        {
            // Get num processes (Only Once)
            if (!num_processes)
            {
                // Read in number of processes
                sscanf(line.c_str(), "%d", &num_processes);

                // Initiliaze Process Array
                process_arr = new Process[num_processes];
            }
            else if (process_read_count < num_processes)
            {
                // Get how many lines of VMAs are present for process
                sscanf(line.c_str(), "%d", &vma_lines_to_read);
                process_arr[process_read_count].init_vma(vma_lines_to_read);
                // Read in however many VMA lines there are
                for (int vma_num = 0; vma_num < vma_lines_to_read; vma_num++)
                {
                    // Get the next line which Contains VMA specs
                    getline(input_file, line);

                    // Read in VMA Specifications
                    sscanf(line.c_str(), "%d %d %d %d", &start_vpage, &end_vpage, &write_protected, &file_mapped);

                    // Access Process Array and Set the VMA specs in the correct Process's page table
                    process_arr[process_read_count].add_vma(vma_num, start_vpage, end_vpage, write_protected, file_mapped);
                }
                process_read_count++;
            }
            else
            {
                break;
            }
        }
    }
    input_file.close();

    // Add process arr to pointer for easier accounting
    THE_PAGER->init_process_metadata(num_processes, process_arr);

    // ####################################
    // ######## Simulation Begins #########
    // ###################################

    // Helper variables for simulation
    char operation;
    int vpage;
    int current_process_num;
    int inst_count = 0;
    Process *CURRENT_PROCESS;
    input_file.open(inputfile_name);

    // Read instructions
    while (getline(input_file, line))
    {
        // Ignore line comments
        if (line.c_str()[0] != '#')
        {
            // Parse Operation + Vpage from input
            sscanf(line.c_str(), "%s %d", &operation, &vpage);

            // If O option print instruction details
            if ((operation == 'c' || operation == 'w' || operation == 'e' || operation == 'r'))
            {
                if (O)
                {
                    printf("%d: ==> %s %d\n", inst_count, &operation, vpage);
                }
                inst_count++;
            }

            switch (operation)
            {
            case 'c':
                // Update current process number
                current_process_num = vpage;
                // Add context-switching cycle cost to pager for accounting
                THE_PAGER->allocate_cost(CONTEXT_SWITCH);
                // Update the pointer to current Process
                CURRENT_PROCESS = &process_arr[current_process_num];
                break;

            case 'e':
                // Add Process-Exit cycle cost to pager for accounting
                THE_PAGER->allocate_cost(PROC_EXIT);

                // Traverse active process page table, each valid entry unmap the page
                for (int i = 0; i < NUM_PTE; i++)
                {
                    pte_t *temp = CURRENT_PROCESS->get_vpage(i);
                    if (temp->PRESENT)
                    {
                        // TODO: May have to add some printf's here
                        // Unmap frame
                        unsigned int frame_num = temp->frame_number;
                        THE_PAGER->clear_mapping(frame_num);

                        // Add frame to free list
                        THE_PAGER->add_frame_to_free_list(frame_num);

                        // Update accounting per instructions:
                        /*On process exit (instruction), you have to traverse the active process’s pagetable starting from
                         0..63 and for each valid entry UNMAP the page and FOUT modified filemapped pages.
                        Note that dirty non-fmapped (anonymous) pages are not written back (OUT) as the process exits.*/
                        CURRENT_PROCESS->allocate_cost(UNMAPS);
                        if (temp->FILEMAPPED && temp->MODIFIED)
                        {
                            CURRENT_PROCESS->allocate_cost(FOUTS);
                        }
                    }
                }
                break;
            case 'r':
                // Read instruction logic
                read_write_logic(THE_PAGER, CURRENT_PROCESS, vpage);
                CURRENT_PROCESS->set_referenced(vpage);
                break;
            case 'w':
                // Write instruction logic
                read_write_logic(THE_PAGER, CURRENT_PROCESS, vpage);

                // Check if write protect is enabled, if so raise SEGPROT
                if (CURRENT_PROCESS->write_protect_enabled(vpage))
                {
                    // Then we raise a SEGPROT error as we cannot write to this VMA
                    CURRENT_PROCESS->allocate_cost(SEGPROT);
                }
                else
                {
                    // Update Modified if written to successfully
                    CURRENT_PROCESS->set_write(vpage);
                }

                // Update ref bit
                CURRENT_PROCESS->set_referenced(vpage);
                break;
            }
        }
    }
    input_file.close();

    if (P)
    {
        THE_PAGER->print_process_ptes();
    }
    if (F)
    {
        THE_PAGER->print_frame_table();
    }
    if (S)
    {
        THE_PAGER->print_per_process_stats();
        THE_PAGER->print_total_cost();
    }

    return 0;
}