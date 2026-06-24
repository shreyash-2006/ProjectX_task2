#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <deque>
#include <queue>
#include <memory>
#include <iomanip>

class CacheLevel {
private:
    std::deque<std::string> slots;
    size_t capacity;
public:
    CacheLevel(size_t cap) : capacity(cap) {}

    // Check if block is present (hit)
    bool contains(const std::string& block) const {
        for (const auto& b : slots)
            if (b == block) return true;
        return false;
    }

    // Insert block at the tail (most recent), evict from head if full
    void insert(const std::string& block) {
        if (slots.size() == capacity) {
            slots.pop_front();
        }
        slots.push_back(block);
    }

    std::string toString() const {
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < slots.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << slots[i];
        }
        oss << "]";
        return oss.str();
    }
};

struct Task {
    std::string id;
    int burst;                // remaining CPU cycles (pure compute)
    std::vector<std::string> memBlocks;   // list of blocks to access in order
    size_t nextMemIndex;      // next block to access (0..size)
    bool burstPhase;          // true when all memory accesses are done
    bool completed;

    Task(const std::string& i, int b, const std::vector<std::string>& mem)
        : id(i), burst(b), memBlocks(mem), nextMemIndex(0), burstPhase(false), completed(false) {}
    bool hasMoreMem() const {
        return !burstPhase && nextMemIndex < memBlocks.size();
    }
    std::string getNextMemBlock() {
        if (hasMoreMem())
            return memBlocks[nextMemIndex];
        return "";
    }
    void advanceMem() {
        if (!burstPhase && nextMemIndex < memBlocks.size()) {
            nextMemIndex++;
            if (nextMemIndex == memBlocks.size()) {
                // switch to burst phase
                burstPhase = true;
            }
        }
    }

    // Returns true if task is fully done
    bool doComputeStep() {
        if (burstPhase && burst > 0) {
            burst--;
            if (burst == 0) {
                completed = true;
                return true;
            }
        }
        return false;
    }

    bool isFinished() const {
        return completed;
    }
};

class CacheHierarchy {
private:
    CacheLevel L1, L2, L3;
    int ramAccesses;

public:
    CacheHierarchy() : L1(32), L2(128), L3(512), ramAccesses(0) {}

    int access(const std::string& block, bool& wasRamAccess) {
        wasRamAccess = false;

        // L1 hit
        if (L1.contains(block)) {
            // hit in L1 - nothing changes
            return 0; // L1 hit
        }

        // L2 hit
        if (L2.contains(block)) {
            // Promote block to L1 (FIFO insertion)
            L1.insert(block);
            return 1; // L2 hit
        }

        // L3 hit
        if (L3.contains(block)) {
            // Promote to L1
            L1.insert(block);
            // Also bring into L2 (since L2 didn't have it)
            L2.insert(block);
            return 2; // L3 hit
        }

        // Complete miss: fetch from RAM
        ramAccesses++;
        wasRamAccess = true;
        // Insert into all levels
        L1.insert(block);
        L2.insert(block);
        L3.insert(block);
        return 3; // RAM access
    }

    int getRamAccessCount() const { return ramAccesses; }

    // For printing cache states
    std::string L1toString() const { return L1.toString(); }
    std::string L2toString() const { return L2.toString(); }
    std::string L3toString() const { return L3.toString(); }
};

class SchedulerRR {
private:
    std::vector<std::shared_ptr<Task>> allTasks;
    std::queue<std::shared_ptr<Task>> readyQueue;
    std::shared_ptr<Task> currentTask;
    int quantum;          // steps per task
    int quantumCounter;
    std::string algoName;

public:
    SchedulerRR(int q = 3) : quantum(q), quantumCounter(0), algoName("Round Robin (quantum = " + std::to_string(q) + ")") {}

    void addTask(std::shared_ptr<Task> task) {
        allTasks.push_back(task);
        readyQueue.push(task);
    }

    // Returns true if a task was selected and is ready to run
    bool selectNextTask() {
        if (currentTask && !currentTask->isFinished()) {
            if (quantumCounter <= 0) {
                if (!currentTask->isFinished()) {
                    readyQueue.push(currentTask);
                }
                currentTask = nullptr;
            } else {
                return true;
            }
        }

        // If current task finished, also put aside
        if (currentTask && currentTask->isFinished()) {
            currentTask = nullptr;
        }

        // Select next from ready queue
        while (!readyQueue.empty()) {
            auto next = readyQueue.front();
            readyQueue.pop();
            if (!next->isFinished()) {
                currentTask = next;
                quantumCounter = quantum;
                return true;
            }
        }
        currentTask = nullptr;
        return false;
    }

    std::shared_ptr<Task> getCurrentTask() const {
        return currentTask;
    }

    void stepDone() {
        if (currentTask && !currentTask->isFinished()) {
            quantumCounter--;
        }
    }

    bool hasPendingWork() const {
        if (currentTask && !currentTask->isFinished()) return true;
        for (auto& t : allTasks)
            if (!t->isFinished()) return true;
        return false;
    }

    std::string getAlgoName() const { return algoName; }

    int getTasksCompleted() const {
        int cnt = 0;
        for (auto& t : allTasks)
            if (t->isFinished()) cnt++;
        return cnt;
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    std::ifstream infile(argv[1]);
    if (!infile.is_open()) {
        std::cerr << "Error: Cannot open input file " << argv[1] << std::endl;
        return 1;
    }

    std::vector<std::shared_ptr<Task>> tasks;
    std::string line;
    while (std::getline(infile, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::string token;
        iss >> token;
        if (token != "TASK") continue;

        std::string id;
        std::string burstKw;
        int burst;
        std::string memKw;
        iss >> id >> burstKw >> burst >> memKw;
        if (burstKw != "BURST" || memKw != "MEM") {
            std::cerr << "Invalid task line: " << line << std::endl;
            continue;
        }
        std::vector<std::string> blocks;
        std::string block;
        while (iss >> block) {
            blocks.push_back(block);
        }
        tasks.push_back(std::make_shared<Task>(id, burst, blocks));
    }
    infile.close();

    if (tasks.empty()) {
        std::cerr << "No tasks found in input file." << std::endl;
        return 1;
    }

    SchedulerRR scheduler(3);
    for (auto& t : tasks) scheduler.addTask(t);
    CacheHierarchy cache;

    int cycle = 1;
    int totalRamAccesses = 0;

    // Main simulation loop
    while (scheduler.hasPendingWork()) {
        if (!scheduler.selectNextTask()) {
            break;
        }

        auto current = scheduler.getCurrentTask();
        if (!current) break;

        bool didMemAccess = false;
        std::string accessedBlock;
        bool wasRam = false;

        if (current->hasMoreMem()) {
            accessedBlock = current->getNextMemBlock();
            int hitLevel = cache.access(accessedBlock, wasRam);
            if (wasRam) totalRamAccesses++;
            current->advanceMem();
            didMemAccess = true;

            // Print cycle info: memory request
            std::cout << "Cycle " << cycle << " - Running: " << current->id
                      << " Requesting: " << accessedBlock << " ";

            std::cout << "L1: " << cache.L1toString()
                      << " L2: " << cache.L2toString()
                      << " L3: " << cache.L3toString() << std::endl;
        } else if (current->burstPhase && current->burst > 0) {
            // Compute phase
            current->doComputeStep();
            // Print compute step (no memory access)
            std::cout << "Cycle " << cycle << " - Running: " << current->id
                      << " Compute (remaining burst " << current->burst << ") "
                      << "L1: " << cache.L1toString()
                      << " L2: " << cache.L2toString()
                      << " L3: " << cache.L3toString() << std::endl;
        } else {
            // Task is finished
            scheduler.stepDone();
            cycle++;
            continue;
        }

        // Update scheduler quantum tracking
        scheduler.stepDone();
        cycle++;
    }

    // Final results
    std::cout << "\n=== Final Results ===" << std::endl;
    std::cout << "Total Cycles: " << (cycle - 1) << std::endl;
    std::cout << "Tasks Completed: " << scheduler.getTasksCompleted() << std::endl;
    std::cout << "Scheduler: " << scheduler.getAlgoName() << std::endl;
    std::cout << "RAM Accesses: " << totalRamAccesses << std::endl;

    return 0;
}