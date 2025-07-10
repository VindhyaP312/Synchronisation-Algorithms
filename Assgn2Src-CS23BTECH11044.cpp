#include <bits/stdc++.h>
using namespace std;

int C;                            // shared counter
atomic<bool> isSudokuValid{true}; // global var to check if sudoku is valid or not at final

int N, taskInc, K;
vector<vector<int>> sudoku;

/*
Takes index of row number to check if that row is valid row or not and returns
- true if the row is valid (contains all numbers from 1 to N exactly once)
- false otherwise
*/
bool validateRow(int rowNo)
{
    // Hash array initialised to zeroes
    int hasharr[N + 1];
    for (int i = 0; i <= N; i++)
    {
        hasharr[i] = 0;
    }
    // Traverses that row in sudoku
    for (int i = 0; i < N; i++)
    {
        if (sudoku[rowNo][i] > 0 && sudoku[rowNo][i] <= N)
            hasharr[sudoku[rowNo][i]] = 1; // If valid, mark its presence in the hash array
    }

    // traverses hash array to check if any elemnet didnot appear in that row
    for (int i = 1; i <= N; i++)
    {
        if (hasharr[i] != 1)
        {
            isSudokuValid.store(false); // sudoku invalid
            return false;
        }
    }
    return true;
}

// Takes index of column number to check if that col is valid col or not
bool validateColumn(int columnNo)
{
    int hasharr[N + 1];
    for (int i = 0; i <= N; i++)
    {
        hasharr[i] = 0;
    }
    for (int i = 0; i < N; i++)
    {
        if (sudoku[i][columnNo] > 0 && sudoku[i][columnNo] <= N)
            hasharr[sudoku[i][columnNo]] = 1;
    }
    for (int i = 1; i <= N; i++)
    {
        if (hasharr[i] != 1)
        {
            isSudokuValid.store(false); // sudoku invalid
            return false;
        }
    }
    return true;
}

/*
Takes index of grid number to check if that grid is valid grid or not as argument.
Total grids are N, it starts from 0 to N-1
ex- for 9*9 sudoku, upper 3 - 3*3 sized grids are named as 0, 1, 2 respectively from left to right
*/
bool validateGrid(int gridNo)
{
    // Hash array initialised to zeroes
    int hasharr[N + 1];
    for (int i = 0; i <= N; i++)
    {
        hasharr[i] = 0;
    }
    int rootN = sqrt(N);
    int startRowNo = (gridNo / rootN) * rootN;    // starting row number of that grid
    int startColumnNo = (gridNo % rootN) * rootN; // starting row number of that grid
    // traversing els of that grid
    for (int i = startRowNo; i < (startRowNo + rootN); i++)
    {
        for (int j = startColumnNo; j < (startColumnNo + rootN); j++)
        {
            if (sudoku[i][j] > 0 && sudoku[i][j] <= N)
                hasharr[sudoku[i][j]] = 1;
        }
    }
    // traverses hash array to check if any element didnot appear in that grid
    for (int i = 1; i <= N; i++)
    {
        if (hasharr[i] != 1)
        {
            isSudokuValid.store(false); // sudoku invalid
            return false;
        }
    }
    return true;
}

// base class
class Lock
{
public:
    virtual void lock(int) = 0;
    virtual void unlock(int) = 0;
};

// derived class
/*
Initially, isLocked variable is false and the first thread that requests CS acquires it and makes it true resulting in no other thread able to acquire till that thread unlocks it by making the value isLocked to false, once it finishes its work in critical section.*/
class TASLock : public Lock
{
    atomic<bool> isLocked{false}; // shared among threads

public:
    void lock(int idNo)
    {
        while (isLocked.exchange(true, memory_order_acquire))
            ;
    }
    // Since now again the lock is set free(false), other threads can use it.
    void unlock(int idNo)
    {
        isLocked.store(false, memory_order_release);
    }
};

/*
If isLocked is false, it sets to true and comes out this while loop(since function returns true as isLocked is equal to expected)
This function returns false when isLocked(true) is different from expected(false) and sets expected to true, so that's why expected
should be changed back to false inside the while loop.*/
class CASLock : public Lock
{
    atomic<bool> isLocked{false}; // shared among threads

public:
    void lock(int idNo)
    {
        bool expected = 0;
        while (!isLocked.compare_exchange_strong(expected, true, memory_order_acquire))
        {
            expected = 0;
        }
    }
    void unlock(int idNo)
    {
        // Once the thread finishes its work, it is set back to false.
        isLocked.store(false, memory_order_release);
    }
};

/*
It uses a waiting array, which stores true when a thread makes a request to enter CS. Initially this array is set to false.
Once the thread finishes its work, it checks for the next thread after it according to the thread number that is waiting by checking in the waiting array.*/
class Bounded_CASLock : public Lock
{
    atomic<bool> isLocked{false};                           // shared among threads
    vector<atomic<bool>> waiting = vector<atomic<bool>>(K); // monitors other threads waiting status

public:
    Bounded_CASLock()
    {
        for (int i = 0; i < K; i++)
        {
            waiting[i].store(false);
        }
    }
    // idNo should start from 0 in below fns argument
    void lock(int idNo)
    {
        waiting[idNo].store(true);
        bool expected = false;
        while (waiting[idNo].load())
        {
            if (isLocked.compare_exchange_strong(expected, true, memory_order_acquire))
            {
                break;
            }
            expected = false;
        }
        waiting[idNo].store(false);
    }

    void unlock(int idNo)
    {
        int j = (idNo + 1) % K;
        // checks for other thread that is waiting
        while (j != idNo && !waiting[j].load())
        {
            j = (j + 1) % K;
        }
        if (j == idNo)
        {
            isLocked.store(false, memory_order_release);
        }
        // gives chance for other threads
        else
        {
            waiting[j].store(false);
        }
    }
};

struct threadArgs
{
    int idNo;
    int lockType;
    Lock *lock;
};

// return type of thread
struct threadResult
{
    int idNo;
    vector<pair<int, time_t>> event_timing; // what event it is doing like requesting for CS(0), entering CS(1), leaving CS(2)
    vector<pair<map<char, pair<int, int>>, time_t>> assigned_work_time;
    vector<pair<char, pair<pair<int, bool>, time_t>>> validity_time; // r{ow/col/grid} char type | {{rowNo, isvalid}, atTime}
    vector<double> CSEntryTimes;
    vector<double> CSExitTimes;
};

void *validate(void *param)
{
    threadArgs *args = (threadArgs *)param;
    int idNo = args->idNo;
    int lockType = args->lockType;
    Lock *lock = args->lock;
    delete args;
    threadResult *res = new threadResult;
    res->idNo = idNo;
    do
    {
        if (!isSudokuValid.load())
        { // Early termination check
            pthread_exit(res);
        }
        map<char, pair<int, int>> work;

        // entry section
        time_t t1 = clock();
        res->event_timing.push_back({0, t1});
        lock->lock(idNo);

        // critical section: incrementing C by taskInc
        time_t t2 = clock();
        res->event_timing.push_back({1, t2});
        int start = C;
        C = C + taskInc;

        time_t t3 = clock();
        // exit section
        lock->unlock(idNo);
        time_t t4 = clock();
        res->event_timing.push_back({2, t4});
        double entryTime = double(t2 - t1) / CLOCKS_PER_SEC * 1000000;
        res->CSEntryTimes.push_back(entryTime);
        double exitTime = double(t4 - t3) / CLOCKS_PER_SEC * 1000000;
        res->CSExitTimes.push_back(exitTime);

        // remainder section
        if (!isSudokuValid.load())
        { // Early termination check
            pthread_exit(res);
        }
        else
        {
            // assigning the work
            if (start >= 3 * N)
                pthread_exit(res);
            else if (start <= 3 * N && start + taskInc >= 3 * N)
                work['G'] = {start - 2 * N, N};
            else if (start >= 2 * N && start + taskInc < 3 * N)
                work['G'] = {start - 2 * N, start - 2 * N + taskInc};
            else if (start < 2 * N && start + taskInc >= 2 * N)
            {
                work['G'] = {0, start + taskInc - 2 * N};
                if (start > N)
                    work['C'] = {start - N, N};
                else
                {
                    work['C'] = {0, N};
                    work['R'] = {start, N};
                }
            }
            else if (start >= N && start + taskInc < 2 * N)
                work['C'] = {start - N, start - N + taskInc};
            else if (start < N && start + taskInc > N)
            {
                work['C'] = {0, start + taskInc - N};
                work['R'] = {start, N};
            }
            else if (start < N)
                work['R'] = {start, start + taskInc};

            (res->assigned_work_time).push_back({work, clock()}); // storing in return result
        }
        // validating the assigned work
        for (auto it : work)
        {
            if (it.first == 'R')
            {
                for (int i = it.second.first; i < it.second.second; i++)
                {
                    if (!isSudokuValid.load())
                    { // Early termination check
                        pthread_exit(res);
                    }
                    bool isValid = validateRow(i);
                    (res->validity_time).push_back({'R', {{i, isValid}, clock()}});
                }
            }
            else if (it.first == 'C')
            {
                for (int i = it.second.first; i < it.second.second; i++)
                {
                    if (!isSudokuValid.load())
                    { // Early termination check
                        pthread_exit(res);
                    }
                    bool isValid = validateColumn(i);
                    (res->validity_time).push_back({'C', {{i, isValid}, clock()}});
                }
            }
            else if (it.first == 'G')
            {
                for (int i = it.second.first; i < it.second.second; i++)
                {
                    if (!isSudokuValid.load())
                    { // Early termination check
                        pthread_exit(res);
                    }
                    bool isValid = validateGrid(i);
                    (res->validity_time).push_back({'G', {{i, isValid}, clock()}});
                }
            }
        }

    } while (true);

    pthread_exit(res);
}

int main(int argc, char *argv[])
{
    if (argc != 2) // if Incorrect arguments passed
    {
        cout << "Incorrect arguments passed!" << endl;
        cout << "Second argument should specify locking algorithm type:\n0 - TAS\n1 - CAS\n2 - Bounded-CAS\n3 - Sequential\n";
        cout << "Enter in the form \"./a.out <Locking algorithm type>\"" << endl;
        return 1;
    }
    int lockType = stoi(argv[1]);
    // opening input file to read
    ifstream inFile("inp.txt");
    if (!inFile.is_open())
    {
        cout << "Error opening the file" << endl;
        return 1;
    }
    // reading file
    inFile >> K >> N >> taskInc;
    sudoku.resize(N, vector<int>(N));
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            inFile >> sudoku[i][j];
        }
    }

    inFile.close();

    Lock *lock;
    if (lockType == 0)
        lock = new TASLock();
    if (lockType == 1)
        lock = new CASLock();
    if (lockType == 2)
        lock = new Bounded_CASLock();

    // sequential
    if (lockType == 3)
    {
        ofstream outFile("output.txt");
        time_t startTime = clock();
        // rows
        for (int i = 0; i < N; i++)
        {
            if (!isSudokuValid.load())
                break;
            bool res = validateRow(i);
            outFile << "Row " << i + 1 << " is ";
            if (res)
                outFile << "valid at ";
            else
                outFile << "invalid at ";
            outFile << (double)(clock() - startTime) / CLOCKS_PER_SEC * 1000000 << " µs" << endl;
        }
        // columns
        for (int i = 0; i < N; i++)
        {
            if (!isSudokuValid.load())
                break;
            bool res = validateColumn(i);
            outFile << "Column " << i + 1 << " is ";
            if (res)
                outFile << "valid at ";
            else
                outFile << "invalid at ";
            outFile << (double)(clock() - startTime) / CLOCKS_PER_SEC * 1000000 << " µs" << endl;
        }
        // grids
        for (int i = 0; i < N; i++)
        {
            if (!isSudokuValid.load())
                break;
            bool res = validateGrid(i);
            outFile << "Grid " << i + 1 << " is ";
            if (res)
                outFile << "valid at ";
            else
                outFile << "invalid at ";
            outFile << (double)(clock() - startTime) / CLOCKS_PER_SEC * 1000000 << " µs" << endl;
        }
        time_t endTime = clock();
        double timeTaken = (double)(endTime - startTime) / CLOCKS_PER_SEC * 1000000;
        if (isSudokuValid.load())
            outFile << "Sudoku is valid\n";
        else
            outFile << "Sudoku is invalid\n";
        outFile << "The total time taken is " << timeTaken << " µs.";
        outFile.close();
    }
    // if TAS, CAS or BoundedCAS
    else
    {
        pthread_t tid[K];
        pthread_attr_t attr;
        pthread_attr_init(&attr);

        time_t startTime = clock();
        for (int i = 0; i < K; i++)
        {
            threadArgs *args = new threadArgs;
            args->idNo = i;
            args->lockType = lockType;
            args->lock = lock;
            // thread creation
            pthread_create(&tid[i], &attr, validate, (void *)args);
        }

        ofstream outFile("output.txt");
        vector<double> avgCSEntryTime;
        vector<double> avgCSExitTime;
        vector<double> maxCSEntryTime;
        vector<double> maxCSExitTime;
        for (int i = 0; i < K; i++)
        {
            threadResult *res;
            // thread joining
            pthread_join(tid[i], (void **)&res);

            for (auto it : res->event_timing)
            {
                outFile << "Thread " << (res->idNo) + 1;
                if (it.first == 0)
                    outFile << " requests to enter CS at ";
                else if (it.first == 1)
                    outFile << " enters CS at ";
                else if (it.first == 2)
                    outFile << " leaves CS at ";
                outFile << (double)(it.second - startTime) / CLOCKS_PER_SEC * 1000000 << " µs" << endl;
            }
            for (auto it : res->assigned_work_time)
            {
                int count = 0;
                outFile << "Thread " << (res->idNo) + 1 << " grabs ";
                for (auto it2 : it.first)
                {
                    if (it2.first == 'R')
                    {
                        for (int i = it2.second.first; i < it2.second.second; i++)
                        {
                            if (count == 0)
                                outFile << "row " << i + 1;
                            else
                                outFile << ", " << "row " << i + 1;
                            count++;
                        }
                    };
                    if (it2.first == 'C')
                    {
                        for (int i = it2.second.first; i < it2.second.second; i++)
                        {
                            if (count == 0)
                                outFile << "column " << i + 1;
                            else
                                outFile << ", " << "column " << i + 1;
                            count++;
                        }
                    };
                    if (it2.first == 'G')
                    {
                        for (int i = it2.second.first; i < it2.second.second; i++)
                        {
                            if (count == 0)
                                outFile << "grid " << i + 1;
                            else
                                outFile << ", " << "grid " << i + 1;
                            count++;
                        }
                    };
                }
                outFile << " at " << (double)(it.second - startTime) / CLOCKS_PER_SEC * 1000000 << " µs" << endl;
            }
            for (auto it : res->validity_time)
            {
                outFile << "Thread " << (res->idNo) + 1 << " completes checking of ";
                if (it.first == 'R')
                    outFile << "row ";
                else if (it.first == 'C')
                    outFile << "column ";
                else if (it.first == 'G')
                    outFile << "grid ";
                outFile << (it.second.first.first) + 1 << " at " << (double)(it.second.second - startTime) / CLOCKS_PER_SEC * 1000000 << " µs" << " and finds it as ";
                if (it.second.first.second)
                    outFile << "valid" << endl;
                else
                    outFile << "invalid" << endl;
            }
            if (!(res->CSEntryTimes).empty())
            {
                double avgEntryTimeByThread = accumulate((res->CSEntryTimes).begin(), (res->CSEntryTimes).end(), 0.0) / (res->CSEntryTimes).size();
                avgCSEntryTime.push_back(avgEntryTimeByThread);
                double maxEntryTimeByThread = *max_element((res->CSEntryTimes).begin(), (res->CSEntryTimes).end());
                maxCSEntryTime.push_back(maxEntryTimeByThread);
            }
            if (!(res->CSExitTimes).empty())
            {
                double avgExitTimeByThread = accumulate((res->CSExitTimes).begin(), (res->CSExitTimes).end(), 0.0) / (res->CSExitTimes).size();
                avgCSExitTime.push_back(avgExitTimeByThread);
                double maxExitTimeByThread = *max_element((res->CSExitTimes).begin(), (res->CSExitTimes).end());
                maxCSExitTime.push_back(maxExitTimeByThread);
            }
        }

        // sudoku statistics
        time_t endTime = clock();
        double timeTaken = (double)(endTime - startTime) / CLOCKS_PER_SEC * 1000000;
        if (isSudokuValid.load())
            outFile << "Sudoku is valid\n";
        else
            outFile << "Sudoku is invalid\n";
        outFile << "\nThe total time taken is " << timeTaken << " µs." << endl;
        double avgEntryTime = accumulate(avgCSEntryTime.begin(), avgCSEntryTime.end(), 0.0) / avgCSEntryTime.size();
        double avgExitTime = accumulate(avgCSExitTime.begin(), avgCSExitTime.end(), 0.0) / avgCSExitTime.size();
        double maxEntryTime = *max_element(maxCSEntryTime.begin(), maxCSEntryTime.end());
        double maxExitTime = *max_element(maxCSExitTime.begin(), maxCSExitTime.end());

        outFile << "Average time taken by a thread to enter the CS is " << avgEntryTime << " µs." << endl;
        outFile << "Average time taken by a thread to exit the CS is " << avgExitTime << " µs." << endl;
        outFile << "Worst-case time taken by a thread to enter the CS is " << maxEntryTime << " µs." << endl;
        outFile << "Worst-case time taken by a thread to exit the CS is " << maxExitTime << " µs." << endl;

        outFile.close();
    }
    return 0;
}