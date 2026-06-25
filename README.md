# ProjectX_task2
# To compile 
Use command: g++ -std=c++17 -o simulator simulator.cpp && ./simulator input.txt

# Features
Single‑core CPU with Round‑Robin scheduling (configurable quantum).

3‑level cache (L1: 32 slots, L2: 128 slots, L3: 512 slots) with FIFO eviction.

Tasks first request memory blocks in order; after all blocks are loaded, they enter the compute phase.

On a cache miss: the block is promoted from lower levels or fetched from RAM, and inserted into L1 (and L2 if coming from L3, or all levels on RAM miss).

Detailed per‑cycle output – shows which task is running, what it’s doing (memory request or compute), and the state of L1, L2, and L3.

Final report – total cycles, tasks completed, scheduling algorithm name, and total RAM accesses.

Input file in the format described below.

Compilation and Execution (single command)
bash
g++ -std=c++17 -o simulator simulator.cpp && ./simulator input.txt
Replace simulator.cpp with your source file name and input.txt with the path to your task file.

# Output Explanation
For each cycle, the simulator prints:

Cycle number and the currently running task.

If the task is requesting a memory block, it prints Requesting: <block>, followed by the state of L1, L2, and L3 after the access.

If the task is in the compute phase, it prints Compute (remaining burst X), followed by the cache states.

After all tasks finish, a summary block is printed.

# Cache Parameters
Level	Capacity	Latency (conceptual)
L1	32 slots	4 cycles (not simulated)
L2	128 slots	12 cycles
L3	512 slots	40 cycles
RAM	unlimited	200 cycles
The simulator only counts RAM accesses; the latency numbers are not used in the output but can be extended.

