

The code is capable of generating 1000 generations per second for a {64 * segment_size} by {64 * chunk_size} world.
One segment consists of 32 bits and one chunk is 8 lines. 
Thus one line is 64 * 32 = 2048 cells, the world consists of 64 * 8 = 512 lines, making the wolrd 1.048.576 cells.
This translates to 1.048.576 * 1000 = 1.048.576.000 cells generated per second.

Most execution time is spend printing the cells to the terminal. Above result is with a 64 cells x 64 lines = 4096 cells sized screen.