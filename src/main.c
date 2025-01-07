
#include "stdint.h"
#include "stdio.h"

#define SIZE_SEGMENT (sizeof(segment_t) * 8)
#define SIZE_BIT_MAP 9
#define SIZE_WORLD_X 2  // actual size = val * sizeof(segment_t)
#define SIZE_WORLD_Y 20
#define SIZE_WORLD SIZE_WORLD_X* SIZE_WORLD_Y

typedef uint32_t segment_t;

typedef enum {
    cell_segment_read = 0,
    cell_segment_write = 1,
} cell_segment_rw_t;

typedef enum {
    cell_value_dead = 0,
    cell_value_alive = 1,
} cell_value_t;

const cell_value_t alive_cell_rules[] = {
    // index is amount of neighbours.
    [0] = cell_value_dead,  [1] = cell_value_dead,
    [2] = cell_value_alive, [3] = cell_value_alive,
    [4] = cell_value_dead,  [5] = cell_value_dead,
    [6] = cell_value_dead,  [7] = cell_value_dead,
    [8] = cell_value_dead,
};
const cell_value_t dead_cell_rules[] = {
    // index is amount of neighbours.
    [0] = cell_value_dead, [1] = cell_value_dead,
    [2] = cell_value_dead, [3] = cell_value_alive,
    [4] = cell_value_dead, [5] = cell_value_dead,
    [6] = cell_value_dead, [7] = cell_value_dead,
    [8] = cell_value_dead,
};
const cell_value_t* const cell_rules[] = {
    // index is the cell value.
    [cell_value_dead] = dead_cell_rules,
    [cell_value_alive] = alive_cell_rules,
};

segment_t buffer_1[SIZE_WORLD + 2];  // n + 1 read stub, n + 2 write
segment_t buffer_2[SIZE_WORLD + 2];  // n + 1 read stub, n + 2 write
segment_t* world = buffer_1;
segment_t* world_buffer = buffer_2;
// stub. ZII
char newline_table[SIZE_WORLD];
char segment_format[1 << 8][9];
// 0 1 2
// 3 4 5
// 6 7 8
cell_value_t bitmap_translator[1 << 9];

uint8_t get_index(const cell_segment_rw_t rw, const uint16_t x,
                  const uint16_t y) {
    uint8_t oob =
        (y >= SIZE_WORLD_Y) || (x / SIZE_SEGMENT >= SIZE_WORLD_X);
    uint8_t index = (SIZE_WORLD_X * y + x / SIZE_SEGMENT) * !oob +
                    SIZE_WORLD * oob + rw * oob;
    return index;
}

segment_t get_cell_segment(const segment_t* const buf,
                           const uint16_t x, const uint16_t y) {
    return buf[get_index(cell_segment_read, x, y)];
}

void set_cell_segment(segment_t* const buf, const segment_t segment,
                      const uint16_t x, const uint16_t y) {
    buf[get_index(cell_segment_write, x, y)] = segment;
}

void set_cell_value(segment_t* const buf, const cell_value_t value,
                    const uint16_t x, const uint16_t y) {
    const uint8_t shift = (x % SIZE_SEGMENT);
    segment_t segment = get_cell_segment(buf, x, y);
    segment &= (~0 - (1 << shift));
    segment |= (value << shift);
    set_cell_segment(buf, segment, x, y);
}

cell_value_t get_cell_value(const segment_t* const buf,
                            const int16_t x, const uint16_t y) {
    return (cell_value_t)(get_cell_segment(buf, x, y) >>
                              (x % SIZE_SEGMENT) &
                          0x1);
}

uint8_t calc_neighbours(const segment_t* const buf, const uint16_t x,
                        const uint16_t y) {
    return get_cell_value(buf, x - 1, y - 1) +
           get_cell_value(buf, x, y - 1) +
           get_cell_value(buf, x + 1, y - 1) +
           get_cell_value(buf, x - 1, y) +
           get_cell_value(buf, x + 1, y) +
           get_cell_value(buf, x - 1, y + 1) +
           get_cell_value(buf, x, y + 1) +
           get_cell_value(buf, x + 1, y + 1);
}

void print_world(void) {
    for (uint8_t i = 0; i < SIZE_WORLD; i++) {
        segment_t segment = world[i];
        printf("%c", newline_table[i]);
        printf("%s", segment_format[(segment)&0XFF]);
        printf("%s", segment_format[(segment >>= 8) & 0XFF]);
        printf("%s", segment_format[(segment >>= 8) & 0XFF]);
        printf("%s", segment_format[(segment >>= 8) & 0XFF]);
        printf("%s", segment_format[(segment >>= 8) & 0XFF]);
        printf("%s", segment_format[(segment >>= 8) & 0XFF]);
        printf("%s", segment_format[(segment >>= 8) & 0XFF]);
        printf("%s", segment_format[(segment >>= 8) & 0XFF]);
    }
}

void generate_segment_format(const char alive, const char dead) {
    for (uint16_t i = 0; i < (1 << 8); i++) {
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
    for (uint8_t i = 0; i < SIZE_WORLD; i++) {
        newline_table[i] = (i % SIZE_WORLD_X) ? '\0' : '\n';
    }
}

void generate_bitmap_translation_table(void) {
    for (uint16_t i = 0; i < (1 << 9); i++) {
        uint8_t neighbours = (i >> 0 & 0x1) + (i >> 1 & 0x1) +
                             (i >> 2 & 0x1) + (i >> 3 & 0x1) +
                             (i >> 5 & 0x1) + (i >> 6 & 0x1) +
                             (i >> 7 & 0x1) + (i >> 8 & 0x1);
        uint8_t cell_value = (i >> 4 & 0x1);
        bitmap_translator[i] = cell_rules[cell_value][neighbours];
    }
}

segment_t generate_segment(segment_t* buf, uint8_t x, uint8_t y) {
    uint8_t oyt = y - 1;
    uint8_t oyb = y + 1;

    segment_t segment_top = get_cell_segment(buf, x, oyt);
    segment_t segment_middle = get_cell_segment(buf, x, y);
    segment_t segment_bottom = get_cell_segment(buf, x, oyb);

    segment_t attach_segment_top = get_cell_segment(buf, x - 1, oyt);
    segment_t attach_segment_middle = get_cell_segment(buf, x - 1, y);
    segment_t attach_segment_bottom =
        get_cell_segment(buf, x - 1, oyb);

    uint16_t bitmap = ((attach_segment_top >> 31) & 0x1) |
                      ((segment_top & 0x3) << 1) |
                      (((attach_segment_middle >> 31) & 0x1) << 3) |
                      ((segment_middle & 0x3) << 4) |
                      (((attach_segment_bottom >> 31) & 0x1) << 6) |
                      ((segment_bottom & 0x3) << 7);
    segment_t segment = bitmap_translator[bitmap];

    for (uint8_t i = 1; i < 31; i++) {
        bitmap = ((segment_top)&0x7) | (((segment_middle)&0x7) << 2) |
                 (((segment_bottom)&0x7) << 5);
        segment |= ((uint32_t)bitmap_translator[bitmap] << i);
        segment_top >>= 1;
        segment_middle >>= 1;
        segment_bottom >>= 1;
    }

    attach_segment_top = get_cell_segment(buf, x + 1, oyt);
    attach_segment_middle = get_cell_segment(buf, x + 1, y);
    attach_segment_bottom = get_cell_segment(buf, x + 1, oyb);

    bitmap = ((segment_top)&0x3) | ((attach_segment_top & 0x1) << 2) |
             (((segment_middle)&0x3) << 3) |
             ((attach_segment_middle & 0x1) << 5) |
             (((segment_bottom)&0x3) << 6) |
             ((attach_segment_bottom & 0x1) << 8);
    segment |= ((uint32_t)bitmap_translator[bitmap] << 31);

    return segment;
}

void generate_iteration(void) {
    segment_t* buf = world;
    world = world_buffer;
    world_buffer = buf;

    for (uint8_t y = 0; y < SIZE_WORLD_Y; y++) {
        for (uint8_t x = 0; x < SIZE_WORLD_X; x++) {
            // uint8_t neighbours = calc_neighbours(world_buffer, x,
            // y); cell_value_t value = get_cell_value(world_buffer,
            // x, y); set_cell_value(world,
            // cell_rules[value][neighbours], x,
            //                y);
            segment_t segment = generate_segment(world_buffer, x, y);
            set_cell_segment(world, segment, x, y);
        }
    }
}

int main(void) {
    generate_segment_format('x', '-');
    generate_newline_table();
    generate_bitmap_translation_table();
    set_cell_value(world, cell_value_alive, 30, 0);
    set_cell_value(world, cell_value_alive, 30, 1);
    set_cell_value(world, cell_value_alive, 30, 2);

    printf("\033[2J");
    printf("\033[?25l");

    for (uint32_t i = 0; i < 10000; i++) {
        printf("\033[1;1H");
        print_world();
        generate_iteration();
    }
    printf("\n");
    printf("\033[?25h");

    return 0;
}