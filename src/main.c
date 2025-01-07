
#include <immintrin.h>

#include "stdint.h"
#include "stdio.h"

#define SIZE_WORLD_X_CONF 2  // actual size = val * sizeof(segment_t)
#define SIZE_WORLD_Y_CONF 2  // actual size = val * GEN_CHUNK_SIZE

#define SIZE_SEGMENT (sizeof(segment_t) * 8)
#define SIZE_BIT_MAP 9
#define GEN_CHUNK_SIZE 8
#define SIZE_WORLD_Y SIZE_WORLD_Y_CONF* GEN_CHUNK_SIZE
#define SIZE_WORLD SIZE_WORLD_X_CONF* SIZE_WORLD_Y

typedef uint32_t segment_t;

typedef enum {
    cell_segment_read = 0,
    cell_segment_write = 1,
} cell_segment_rw_t;

typedef enum {
    cell_value_dead = 0,
    cell_value_alive = 1,
} cell_value_t;

typedef union {
    __m256i flat;
    uint32_t index[8];
} u256i_t;
typedef segment_t segment_chunk_t[8];

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
char segment_format[1 << 8][8];
// 0 1 2
// 3 4 5
// 6 7 8
cell_value_t bitmap_translator[1 << 9];

inline uint8_t get_index(const cell_segment_rw_t rw, const uint16_t x,
                         const uint16_t y) {
    uint8_t oob = (y >= SIZE_WORLD_Y) ||
                  (x / SIZE_SEGMENT >= SIZE_WORLD_X_CONF);
    uint8_t index =
        (SIZE_WORLD_X_CONF * y + x / SIZE_SEGMENT) * !oob +
        SIZE_WORLD * oob + rw * oob;
    return index;
}

inline segment_t get_cell_segment(const segment_t* const buf,
                                  const uint16_t x,
                                  const uint16_t y) {
    return buf[get_index(cell_segment_read, x, y)];
}

inline void set_cell_segment(segment_t* const buf,
                             const segment_t segment,
                             const uint16_t x, const uint16_t y) {
    buf[get_index(cell_segment_write, x, y)] = segment;
}

inline void set_cell_chunk(segment_t* const buf,
                           const segment_chunk_t chunk,
                           const uint16_t x, const uint16_t y) {
    buf[get_index(cell_segment_write, x, y + 0)] = chunk[0];
    buf[get_index(cell_segment_write, x, y + 1)] = chunk[1];
    buf[get_index(cell_segment_write, x, y + 2)] = chunk[2];
    buf[get_index(cell_segment_write, x, y + 3)] = chunk[3];
    buf[get_index(cell_segment_write, x, y + 4)] = chunk[4];
    buf[get_index(cell_segment_write, x, y + 5)] = chunk[5];
    buf[get_index(cell_segment_write, x, y + 6)] = chunk[6];
    buf[get_index(cell_segment_write, x, y + 7)] = chunk[7];
}

void set_cell_value(segment_t* const buf, const cell_value_t value,
                    const uint16_t x, const uint16_t y) {
    const uint8_t shift = (x % SIZE_SEGMENT);
    segment_t segment = get_cell_segment(buf, x, y);
    segment &= (~0 - (1 << shift));
    segment |= (value << shift);
    set_cell_segment(buf, segment, x, y);
}

void print_world(void) {
    char buf[SIZE_WORLD * (8 * 4 + 1 + 1)];
    char* ptr = buf;
    ptr += sprintf(ptr, "\033[1;1H");
    for (uint8_t i = 0; i < SIZE_WORLD; i++) {
        segment_t segment = world[i];
        ptr += sprintf(ptr, "%c%.8s%.8s%.8s%.8s", newline_table[i],
                       segment_format[(segment)&0XFF],
                       segment_format[(segment >> 8) & 0XFF],
                       segment_format[(segment >> 16) & 0XFF],
                       segment_format[(segment >> 24) & 0XFF]);
    }
    fwrite(buf, 1, ptr - buf, stdout);
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
    }
}

void generate_newline_table(void) {
    for (uint8_t i = 1; i < SIZE_WORLD; i++) {
        newline_table[i] = (i % SIZE_WORLD_X_CONF) ? '\0' : '\n';
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

inline __m256i avx_msk_lsh(__m256i val, const uint32_t mask,
                           const uint32_t lsh) {
    __m256i acc = _mm256_and_si256(val, _mm256_set1_epi32(mask));
    return _mm256_slli_epi32(acc, lsh);
}

inline __m256i avx_rsh_msk_lsh(__m256i val, const uint32_t rsh,
                               const uint32_t mask,
                               const uint32_t lsh) {
    __m256i acc = _mm256_srli_epi32(val, rsh);
    acc = _mm256_and_si256(acc, _mm256_set1_epi32(mask));
    return _mm256_slli_epi32(acc, lsh);
}

inline __m256i avx_calc_segments(__m256i top, __m256i mid,
                                 __m256i bot) {
    __m256i acc = avx_msk_lsh(top, 0x7, 0);
    __m256i acc_r = _mm256_add_epi32(_mm256_setzero_si256(), acc);
    acc = avx_msk_lsh(mid, 0x7, 2);
    acc_r = _mm256_add_epi32(acc_r, acc);
    acc = avx_msk_lsh(bot, 0x7, 5);
    acc_r = _mm256_add_epi32(acc_r, acc);
    return acc_r;
}

inline __m256i avx_calc_right_edge_segments(__m256i top, __m256i mid,
                                            __m256i bot,
                                            __m256i edge_top,
                                            __m256i edge_mid,
                                            __m256i edge_bot) {
    __m256i acc = avx_msk_lsh(top, 0x3, 0);
    __m256i acc_r = _mm256_add_epi32(_mm256_setzero_si256(), acc);
    acc = avx_msk_lsh(mid, 0x3, 3);
    acc_r = _mm256_add_epi32(acc_r, acc);
    acc = avx_msk_lsh(bot, 0x3, 6);
    acc_r = _mm256_add_epi32(acc_r, acc);
    acc = avx_msk_lsh(edge_top, 0x1, 2);
    acc_r = _mm256_add_epi32(acc_r, acc);
    acc = avx_msk_lsh(edge_mid, 0x1, 5);
    acc_r = _mm256_add_epi32(acc_r, acc);
    acc = avx_msk_lsh(edge_bot, 0x1, 8);
    acc_r = _mm256_add_epi32(acc_r, acc);
    return acc_r;
}

inline __m256i avx_calc_left_edge_segments(__m256i top, __m256i mid,
                                           __m256i bot,
                                           __m256i edge_top,
                                           __m256i edge_mid,
                                           __m256i edge_bot) {
    __m256i acc = avx_msk_lsh(top, 0x3, 1);
    __m256i acc_r = _mm256_add_epi32(_mm256_setzero_si256(), acc);
    acc = avx_msk_lsh(mid, 0x3, 4);
    acc_r = _mm256_add_epi32(acc_r, acc);
    acc = avx_msk_lsh(bot, 0x3, 7);
    acc_r = _mm256_add_epi32(acc_r, acc);
    acc = avx_rsh_msk_lsh(edge_top, (SIZE_SEGMENT - 1), 0x1, 0);
    acc_r = _mm256_add_epi32(acc_r, acc);
    acc = avx_rsh_msk_lsh(edge_mid, (SIZE_SEGMENT - 1), 0x1, 3);
    acc_r = _mm256_add_epi32(acc_r, acc);
    acc = avx_rsh_msk_lsh(edge_bot, (SIZE_SEGMENT - 1), 0x1, 6);
    acc_r = _mm256_add_epi32(acc_r, acc);
    return acc_r;
}

void fill_segments(segment_t* buf, uint8_t x, uint8_t y,
                   segment_chunk_t top, segment_chunk_t mid,
                   segment_chunk_t bot) {
    mid[0] = get_cell_segment(buf, x, y + 0);
    mid[1] = get_cell_segment(buf, x, y + 1);
    mid[2] = get_cell_segment(buf, x, y + 2);
    mid[3] = get_cell_segment(buf, x, y + 3);
    mid[4] = get_cell_segment(buf, x, y + 4);
    mid[5] = get_cell_segment(buf, x, y + 5);
    mid[6] = get_cell_segment(buf, x, y + 6);
    mid[7] = get_cell_segment(buf, x, y + 7);

    top[0] = get_cell_segment(buf, x, y - 1);
    top[1] = mid[0];
    top[2] = mid[1];
    top[3] = mid[2];
    top[4] = mid[3];
    top[5] = mid[4];
    top[6] = mid[5];
    top[7] = mid[6];

    bot[0] = mid[1];
    bot[1] = mid[2];
    bot[2] = mid[3];
    bot[3] = mid[4];
    bot[4] = mid[5];
    bot[5] = mid[6];
    bot[6] = mid[7];
    bot[7] = get_cell_segment(buf, x, y + 8);
}

void generate_segment(segment_t* buf, segment_chunk_t chunk,
                      uint8_t x, uint8_t y) {
    u256i_t bitmaps[SIZE_SEGMENT] = {};

    segment_chunk_t top;
    segment_chunk_t mid;
    segment_chunk_t bot;
    fill_segments(buf, x, y, top, mid, bot);

    segment_chunk_t left_top;
    segment_chunk_t left_mid;
    segment_chunk_t left_bot;
    fill_segments(buf, x - 1, y, left_top, left_mid, left_bot);

    segment_chunk_t right_top;
    segment_chunk_t right_mid;
    segment_chunk_t right_bot;
    fill_segments(buf, x + 1, y, right_top, right_mid, right_bot);

    __m256i lst = _mm256_load_si256((__m256i*)left_top);
    __m256i st = _mm256_load_si256((__m256i*)top);
    __m256i rst = _mm256_load_si256((__m256i*)right_top);

    __m256i lsm = _mm256_load_si256((__m256i*)left_mid);
    __m256i sm = _mm256_load_si256((__m256i*)mid);
    __m256i rsm = _mm256_load_si256((__m256i*)right_mid);

    __m256i lsb = _mm256_load_si256((__m256i*)left_bot);
    __m256i sb = _mm256_load_si256((__m256i*)bot);
    __m256i rsb = _mm256_load_si256((__m256i*)right_bot);

    bitmaps[0].flat =
        avx_calc_left_edge_segments(st, sm, sb, lst, lsm, lsb);
    bitmaps[SIZE_SEGMENT - 1].flat =
        avx_calc_right_edge_segments(st, sm, sb, rst, rsm, rsb);

    for (uint8_t i = 1; i < (SIZE_SEGMENT - 1); i++) {
        bitmaps[i].flat = avx_calc_segments(st, sm, sb);
        st = _mm256_srli_epi32(st, 1);
        sm = _mm256_srli_epi32(sm, 1);
        sb = _mm256_srli_epi32(sb, 1);
    }

    for (uint8_t i = 0; i < SIZE_SEGMENT; i++) {
        chunk[0] +=
            ((uint32_t)bitmap_translator[bitmaps[i].index[0]] << i);
        chunk[1] +=
            ((uint32_t)bitmap_translator[bitmaps[i].index[1]] << i);
        chunk[2] +=
            ((uint32_t)bitmap_translator[bitmaps[i].index[2]] << i);
        chunk[3] +=
            ((uint32_t)bitmap_translator[bitmaps[i].index[3]] << i);
        chunk[4] +=
            ((uint32_t)bitmap_translator[bitmaps[i].index[4]] << i);
        chunk[5] +=
            ((uint32_t)bitmap_translator[bitmaps[i].index[5]] << i);
        chunk[6] +=
            ((uint32_t)bitmap_translator[bitmaps[i].index[6]] << i);
        chunk[7] +=
            ((uint32_t)bitmap_translator[bitmaps[i].index[7]] << i);
    }
}

void generate_iteration(void) {
    segment_t* buf = world;
    world = world_buffer;
    world_buffer = buf;

    for (uint8_t y = 0; y < SIZE_WORLD_Y_CONF; y++) {
        for (uint8_t x = 0; x < SIZE_WORLD_X_CONF; x++) {
            segment_chunk_t chunk = {};
            generate_segment(world_buffer, chunk, x, y * 2);
            set_cell_chunk(world, chunk, x, y * 2);
        }
    }
}

int main(void) {
    generate_segment_format('x', '-');
    generate_newline_table();
    generate_bitmap_translation_table();
    set_cell_value(world, cell_value_alive, 2, 0);
    set_cell_value(world, cell_value_alive, 2, 1);
    set_cell_value(world, cell_value_alive, 2, 2);

    printf("\033[2J");
    printf("\033[?25l");

    for (uint32_t i = 0; i < 10000; i++) {
        print_world();
        generate_iteration();
    }
    printf("\n");
    printf("\033[?25h");

    return 0;
}