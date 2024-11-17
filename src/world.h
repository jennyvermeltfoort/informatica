
#include "stack.h"

#ifndef __GUARD_WORLD_H
#define __GUARD_WORLD_H

typedef struct CELL_T cell_t;
typedef struct CELL_INFO_T {
    bool is_bomb;
    bool is_flag;
    bool is_open;
    bool is_cursor;
    unsigned int bomb_count;
} cell_info_t;

typedef struct PLAYER_INFO_T {
    bool is_dead;
} player_info_t;

struct CELL_T {
    cell_info_t info;
    cell_t *north_east;
    cell_t *north;
    cell_t *north_west;
    cell_t *west;
    cell_t *east;
    cell_t *south_east;
    cell_t *south;
    cell_t *south_west;
};

typedef enum BOARD_RETURN_T {
	BOARD_RETURN_OK = 0,
	BOARD_RETURN_NO_FLAGS,
	BOARD_RETURN_IS_OPEN,
	BOARD_RETURN_STOP,
	BOARD_RETURN_IS_FLAG,
} board_return_t;

class Board {
   private:
    const unsigned int board_size_x;
    const unsigned int board_size_y;
    cell_t * const board_start = new cell_t;
    cell_t * board_cursor;
    int flag_count;
    int open_count;
    bool is_show_bomb;
    bool is_dead;
    bool is_done;

    cell_t *init_grid(void);
    void init_bomb(unsigned int bomb_count);
    cell_t *grid_get_cell(unsigned int x, unsigned int y);
    void set_cursor(cell_t* new_cursor);

   public:
    Board(const unsigned int size_x, const unsigned int size_y,
              const unsigned int bomb_count);
    ~Board(void);

    board_return_t cursor_set_open(void);
    board_return_t cursor_set_flag(void);
    void cursor_move_north(void);
    void cursor_move_south(void);
    void cursor_move_west(void);
    void cursor_move_east(void);
    void toggle_show_bomb(void);

    void print();
};

#endif  // __GUARD_WORLD_H
