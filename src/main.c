
#include "stdint.h"
#include "stdio.h"

#define SIZE_WORLD_X 2
#define SIZE_WORLD_Y 8

uint8_t cells[SIZE_WORLD_X * SIZE_WORLD_Y];
char newline_table[SIZE_WORLD_X * SIZE_WORLD_Y];
char segment_format[1 << 8][9];

uint8_t get_cell_segment(uint16_t x, uint16_t y) {
    return cells[SIZE_WORLD_X * y + x/8];
}

void set_cell_segment(uint16_t x, uint16_t y, uint8_t segment) {
    cells[SIZE_WORLD_X * y + x / 8] = segment;
}

uint8_t get_cell_value(uint16_t x, uint16_t y) {
    uint8_t segment = get_cell_segment(x, y);
    return (segment >> (x % 8)) & 0x1;
}

void set_cell_value(uint16_t x, uint16_t y, uint8_t value) {
    uint8_t segment = get_cell_segment(x, y);
    uint8_t shift = (x % 8);
    segment &= ~(0 << shift);
    segment |= ((value & 0x1) << shift);
    set_cell_segment(x, y, segment);
}

uint8_t calc_neighbours(uint16_t x, uint16_t y) {
    return get_cell_value(x-1, y-1) 
        + get_cell_value(x, y-1)
        + get_cell_value(x+1, y-1)
        + get_cell_value(x-1, y)
        + get_cell_value(x+1, y)
        + get_cell_value(x-1, y+1)
        + get_cell_value(x, y+1)
        + get_cell_value(x+1, y+1);
}

void print_world(void) {
    for (uint8_t i = 0 ; i < SIZE_WORLD_X * SIZE_WORLD_Y ; i++) {
        uint8_t segment = cells[i];
        printf("%c", newline_table[i]);
        printf("%s", segment_format[segment]);
    }

}

void generate_segment_format(const char alive, const char dead) {
    for (uint16_t i = 0 ; i < (1 << 8) ; i++) {
        segment_format[i][0] = (i & (1 << 0)) ? alive : dead;
        segment_format[i][1] = (i & (1 << 1)) ? alive : dead;
        segment_format[i][2] = (i & (1 << 2)) ? alive : dead;
        segment_format[i][3] = (i & (1 << 3)) ? alive : dead;
        segment_format[i][4] = (i & (1 << 4)) ? alive : dead;
        segment_format[i][5] = (i & (1 << 5)) ? alive : dead;
        segment_format[i][6] = (i & (1 << 6)) ? alive : dead;
        segment_format[i][7] = (i & (1 << 7)) ? alive : dead;
        segment_format[i][8] = '\0';
    }
}

void generate_newline_table(void) {
    for (uint8_t i = 0 ; i < SIZE_WORLD_X * SIZE_WORLD_Y ; i++) {
        newline_table[i] = (i % SIZE_WORLD_X) ? '\0' : '\n';
    }
}

void generate_iteration(void) {
    for (uint8_t y = 0 ; y < SIZE_WORLD_Y ; y++) {
    for (uint8_t x = 0 ; x < SIZE_WORLD_X * 8  ; x++) {
        if (calc_neighbours(x, y) == 2 || calc_neighbours(x, y) == 3)
        {
            set_cell_value(x,y,1);

        } else {
            set_cell_value(x,y,0);
        }
    }}
}

int main(void) {
    generate_segment_format('x', ' ');
    generate_newline_table();

    set_cell_value(3,3,1);

    for (;;){
        generate_iteration();
        print_world();
    }
}