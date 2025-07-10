                   -- README --

File Structure:
- 'Assgn2Src-CS23BTECH11044.cpp' : The whole code is in this file, which has implementations of sequential, TAS, CAS and BoundedCAS.
- 'inp.txt' : Contains thread count, sudoku size, taskInc and the sudoku.
- 'Assgn2Report-CS23BTECH11044.pdf' : Report which contains brief explanation of code, design decisions, tables, and analysis of tables.

How to execute?
1) Input file should be named as "inp.txt" and should be present in the same directory as this Cpp - code file.
First line contains K(thread count) N(size of sudoku) taskInc(count of tasks assigned), followed by sudoku grid with each row in one line.
Ex of input file:
        8 9 2
        3 1 6 2 8 9 5 7 4
        7 8 2 6 5 4 3 1 9
        5 4 7 9 3 1 8 2 6
        6 7 1 9 2 3 4 5 8
        2 9 5 4 7 8 6 3 1
        8 3 4 5 1 6 2 9 7
        1 2 3 8 4 7 9 6 5
        4 6 7 3 9 5 1 8 2
        9 5 8 1 6 2 7 4 3
2) Run the following command in terminal:
            g++ Assgn2Src-CS23BTECH11044.cpp
3) Run the a.out executable file with command line instructions:
            ./a.out <lockType>
    where lockType is a digit signifying the type of mutual exclusion that should be used.
                    0 - TAS
                    1 - CAS
                    2 - Bounded-CAS
                    3 - Sequential
4) Based on the method user has entered, "output.txt" shows for that method.
The output contains the threads in order line by line i.e, first displays all the operations done by thread1 followed by thread 2, 3 ...
