# Game of Life

This is a parellel implementation of the __Game of Life__, a cellular automaton devised by John Conway in 1970. This game is "played" on a board, or a world, consisting of *m* rows, and *n* columns. An initial generation (generation 0) is either entered by the user, or randomly generated. 

This particular world runs on the following rules:
 * Any live cell with fewer than two live neighbors dies, as if caused by under-population.
 * Any live cell with two or three live neighbors lives on to the next generation.
 * Any live cell with more than three live neighbors dies, as if by over-population.
 * Any dead cell with exactly three live neighbors becomes a live cell, as if by reproduction.
 
In order to run this code, simply type the following code into your command line,
```
./pth_life <r> <c> <m> <n> <max> <'i'|'g'>
```
where:
 * r = number of rows of threads
 * c = number of cols of threads
 * m = number of rows in the world 
 * n = number of columns in the world
 * max = maximum number of generations the program should compute
 * 'i' = user will enter the initial world (generation 0) on stdin
 * 'g' = the program should use a random number generator to generate the initial world.
 
# Notes
This implementation uses a "toroidal world" in which the last row of cells is adjacent to the first row, and the last column of cells is adjacent to the first.
