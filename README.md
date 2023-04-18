# OS-Lab3 MMU Scheduler - Giulio Duregon - gjd9961

## How to run `des_mmu`

**Short answer: run `make` from the CLI.**
My code uses the gcc compiler with the following flags:

```c++
CXX=g++
CXXFLAGS=-g -std=c++11 -Wall -pedantic -lstdc++
BIN=des_mmu
```

If running on linserv1, make sure to load in gcc version 9.2.0:

```bash
> module load gcc-9.2.0
> module unload gcc-4.8.5 # Unload the old version
```

To run `des_mmu`, simply call the executible with the input file you would like it to run on:

```bash
# How to run:
> ./des_mmu -a <Algorithm Type> -f <Number of Frames> -o <Optional Args for output> <your_input_file_name> <random_values_file_name (for Random Algo)>
# i.e.
> ./des_mmu ...
```

## Dependencies

`des_mmu.cpp` has 2 dependencies: `data_structures.hpp` and `mmu_pagers.hpp`. Make sure to compile and link these object files to produce a correct executable.
