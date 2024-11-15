
#include "stack.h"

#ifndef __GUARD_WORLD_H
#define __GUARD_WORLD_H

typedef struct CELL_T cell_t;

struct CELL_T {
    bool is_bomb;
    int temp;
    cell_t *north_east;
    cell_t *north;
    cell_t *north_west;
    cell_t *west;
    cell_t *east;
    cell_t *south_east;
    cell_t *south;
    cell_t *south_west;
};

class CellBoard {
   private:
    cell_t *start;
    const unsigned int board_size_x;
    const unsigned int board_size_y;
    cell_t *init_cell_east(cell_t *cell, const unsigned int x,
                           const unsigned int y);
    cell_t *init_cell_south(cell_t *cell, const unsigned int x,
                            const unsigned int y);
    cell_t *init_cell_south_east(cell_t *cell, const unsigned int x,
                                 const unsigned int y);
    void set_cell_east(cell_t *cell);
    void set_cell_south(cell_t *cell);
    bool is_south_edge(const unsigned int x, const unsigned int y);
    bool is_east_edge(const unsigned int x, const unsigned int y);
    bool is_south_west_corner(const unsigned int x,
                              const unsigned int y);

   public:
    CellBoard(const unsigned int size_x, const unsigned int size_y);
    ~CellBoard(void);

    void print();
};

class World {
   private:
    CellBoard world;

   public:
    World(void);
    ~World(void);
};

#endif  // __GUARD_WORLD_H
