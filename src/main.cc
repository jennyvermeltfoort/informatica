//
#define _TIMED 0

#if _TIMED == 1
#include <chrono>
#endif
#include <cstdlib>
#include <ctime>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

/* Configuration. */
const char DEFAULT_CELL_ALIVE_C = '&';
const char DEFAULT_CELL_DEAD_C = ' ';
const char DEFAULT_CELL_BORDER_C = '@';
const uint16_t DEFAULT_REFRESH_RATE = 1000;
const uint16_t DEFAULT_REFRESH_RATE_INTRO = 100;
const uint16_t INTRO_DURATION_S = 3;
const uint16_t WORLD_SIZE_Y = 1000;
const uint16_t WORLD_SIZE_X = 1000;
const uint16_t VIEW_SIZE_Y = 20;
const uint16_t VIEW_SIZE_X = 100;

typedef char view_storage_t[VIEW_SIZE_Y][VIEW_SIZE_X];

#pragma pack(1)
typedef struct CELL_T {
    uint8_t neighbour_count : 4;
    bool value;
} cell_t;
#pragma pack()
typedef cell_t world_storage_t[WORLD_SIZE_Y][WORLD_SIZE_X];

typedef struct POINT_T {
    uint16_t y;
    uint16_t x;
} point_t;

inline bool anscii_is_int(uint8_t c) {
    const uint8_t ANSCII_NUMBER_0 = 48;
    const uint8_t ANSCII_NUMBER_9 = 57;
    return (c >= ANSCII_NUMBER_0 && c <= ANSCII_NUMBER_9);
}
inline bool anscii_is_whitespace(uint8_t c) {
    return (c == ' ' || c == '\t');
}

inline bool anscii_is_symbol(uint8_t c) {
    const uint8_t ANSCII_SYMBOL_START = 33;
    const uint8_t ANSCII_SYMBOL_END = 126;
    return (c >= ANSCII_SYMBOL_START && c <= ANSCII_SYMBOL_END);
}

class View {
   private:
    point_t size = {VIEW_SIZE_Y, VIEW_SIZE_X};
    view_storage_t view = {};
    std::mutex mutex_cursor;
    point_t user_cursor = {VIEW_SIZE_Y + 1, VIEW_SIZE_X + 1};
    std::function<void(void)> cb_print_info;
    const uint8_t size_info_line = 3;
    uint16_t line_input = size.y + size_info_line + 3;
    uint16_t line_info = size.y + 2;
    bool flag_cb_print_info_set = false;

    const char info_frame_character = '-';
    const std::string input_start_string = ">> ";

    void cursor_move(const point_t point) {
        std::cout << "\033[" << +point.y << ";" << +point.x << "H"
                  << std::flush;
    }
    void cursor_hide(void) { std::cout << "\033[?25l" << std::flush; }
    void cursor_show(void) { std::cout << "\033[?25h" << std::flush; }
    void cursor_move_start(void) { cursor_move(point_t{0, 0}); }
    void cursor_move_end(void) { cursor_move(size); }
    void cursor_move_input(void) {
        point_t p = {line_input, 1};
        cursor_move(p);
    }
    void cursor_move_info(void) {
        point_t p = {line_info, 1};
        cursor_move(p);
    }

    void cursor_erase_display(const uint8_t n) {
        std::cout << "\033[" << +n << "J" << std::flush;
    }
    void cursor_erase_line(const uint8_t n) {
        std::cout << "\033[" << +n << "K" << std::flush;
    }

    void cursor_save(void) { std::cout << "\033[s" << std::flush; }
    void cursor_restore(void) { std::cout << "\033[u" << std::flush; }

   public:
    View() {
        cursor_erase_display(2);
        cursor_hide();
    }
    void reset(void) { draw_frame(); }

    void set_print_callback(std::function<void(void)> cb) {
        cb_print_info = cb;
        flag_cb_print_info_set = true;
    }

    void draw_frame(void) {
        const std::lock_guard<std::mutex> lock(mutex_cursor);
        uint16_t i;

        cursor_erase_display(2);
        cursor_move(point_t{static_cast<uint16_t>(size.y + 1), 1});

        for (i = 0; i < size.x; i++) {
            std::cout << info_frame_character;
        }
        for (i = 0; i <= size_info_line; i++) {
            std::cout << std::endl;
        }
        for (uint16_t i = 0; i < size.x; i++) {
            std::cout << info_frame_character;
        }

        std::cout << std::endl;
        std::cout << input_start_string;
        cursor_show();
    }

    void refresh_info(void) {
        cursor_save();
        cursor_move_info();
        cursor_erase_line(0);
        if (flag_cb_print_info_set) {
            cb_print_info();
        }
        cursor_restore();
    }

    void refresh_input(void) {
        const std::lock_guard<std::mutex> lock(mutex_cursor);
        cursor_move_input();
        cursor_erase_line(0);
        std::cout << input_start_string << std::flush;
    }

    point_t get_pos_cursor_user(void) { return user_cursor; }

    void set_user_cursor_pos(const point_t pos) {
        user_cursor = {
            static_cast<uint16_t>((pos.y > size.y - 1) ? size.y - 1
                                                       : pos.y),
            static_cast<uint16_t>((pos.x > size.x - 1) ? size.x - 1
                                                       : pos.x),
        };
    }

    point_t get_size(void) { return size; }

    void update(const point_t pos, const char c) {
        view[pos.y][pos.x] = c;
    }

    void refresh_view(void) {
        const std::lock_guard<std::mutex> lock(mutex_cursor);
        uint16_t x;
        uint16_t y;
        char *view_ptr = &view[0][0];

        cursor_save();
        cursor_move_end();
        cursor_erase_display(1);
        cursor_move_start();

        for (y = size.y; y > 0; y--) {
            for (x = size.x; x > 0; x--) {
                std::cout << *view_ptr++;
            }
            std::cout << std::endl;
        }

        cursor_move(
            point_t{static_cast<uint16_t>(user_cursor.y + 1),
                    static_cast<uint16_t>(user_cursor.x + 1)}); //terminal cursor starts at 1,1.
        std::cout << "\b\033[105;31m"
                  << view[user_cursor.y][user_cursor.x]
                  << "\033[39;49m";

        std::cout << std::flush;
        cursor_restore();
    }
};

class World {
   private:
    char cell_alive = DEFAULT_CELL_ALIVE_C;
    char cell_dead = DEFAULT_CELL_DEAD_C;
    char cell_border = DEFAULT_CELL_BORDER_C;
    uint32_t infest_random_view_cell_default = 1000;
    uint32_t infest_random_world_cell_default = 1000000;

    world_storage_t world = {};
    const point_t world_size = {WORLD_SIZE_Y, WORLD_SIZE_X};
    point_t world_size_life = {WORLD_SIZE_Y - 2, WORLD_SIZE_X - 2}; // world has dead at x or y = 0 and x or y is world_size.
    point_t world_start_pos = point_t{1, 1};

    cell_t **events = new cell_t *[WORLD_SIZE_X * WORLD_SIZE_Y]; // allocate to heap instead of stack.
    cell_t **event_current = events;

    View view;
    const point_t view_size = view.get_size();
    point_t view_step_size = {view_size.y, view_size.x};
    point_t view_pos = {
        static_cast<uint16_t>(world_size.y / 2 - view_size.y / 2),
        static_cast<uint16_t>(world_size.x / 2 - view_size.x / 2),
    };
    uint16_t refresh_rate = DEFAULT_REFRESH_RATE;
    std::mutex mutex_world_update;
    uint32_t world_alive_counter = 0;
    uint32_t view_alive_counter = 0;
    bool flag_stop = false;
    point_t world_cursor_position;

    void print_info(void) {
        std::cout << "Cursor[y,x]: '" << +world_cursor_position.y
                  << "," << +world_cursor_position.x << "'; ";
        std::cout << "Step size[y,x]: '" << +view_step_size.y << ","
                  << +view_step_size.x << "'; ";
        std::cout << "Refresh rate: '" << +refresh_rate << "'s; ";
        std::cout << std::endl;
        std::cout << "Cell symbol[alive, dead, "
                     "border]: '"
                  << cell_alive << "," << cell_dead << ","
                  << cell_border << "'; ";
        std::cout << "Cells alive[world,view]: '"
                  << +world_alive_counter << ","
                  << +view_alive_counter << "'; ";
        std::cout << std::endl;
        std::cout << "Default random infest cell count[view,world]: '"
                  << +infest_random_view_cell_default << ","
                  << +infest_random_world_cell_default << "'; ";
    }

    /* Calculates neighbours for cell.
    * 
    */
    void point_set_value(cell_t * cell, const bool value) {
        int8_t increment_value = (value) ? 1 : -1;
        world_alive_counter += increment_value;
        cell_t* neighbour_top = cell - WORLD_SIZE_X-1;
        cell_t* neighbour_bottom = cell + WORLD_SIZE_X-1;
        for (uint8_t i = 0 ; i < 3 ; i++) {
            neighbour_top++->neighbour_count += increment_value;
            neighbour_bottom++->neighbour_count += increment_value;
        }
        (cell-1)->neighbour_count +=increment_value;
        (cell+1)->neighbour_count +=increment_value;
        cell->value = value;
    }

    void set_cell_character(char &cell, const char c) {
        if (anscii_is_symbol(c) || c == ' ') {
            cell = c;
            view_refresh_world();
            view_draw_border();
            view.refresh_info();
        }
    }

    point_t transform_pos_view_boundaries(const point_t pos) {
        return {
            (pos.y + view_size.y > world_size.y)
                ? static_cast<uint16_t>(world_size.y - view_size.y +
                                        1)
                : pos.y,
            (pos.x + view_size.x > world_size.x)
                ? static_cast<uint16_t>(world_size.x - view_size.x +
                                        1)
                : pos.x,
        };
    }

    inline void events_clear(void) { event_current = events; }
    inline void event_add(cell_t *e) { *event_current++ = e; }
    void events_process(void) {
        while (event_current-- != events) {
            point_set_value(*event_current, !(*event_current)->value);
        }
        events_clear();
    }

    void points_health_check(void) {
        static const uint16_t limit_y = world_size.y - 2;
        static const uint16_t limit_x = world_size.x - 2;
        uint16_t x;
        uint16_t y;
        cell_t *world_ptr = &world[1][1]; // world starts at 1,1.

        for (y = limit_y; y > 0 ; y--) {
            for (x = limit_x; x > 0; x--) {
                if (((world_ptr->value) &&
                     (world_ptr->neighbour_count > 3 ||
                      world_ptr->neighbour_count < 2)) ||
                    (!(world_ptr->value) &&
                     world_ptr->neighbour_count == 3)) {
                    event_add(world_ptr);
                }
                world_ptr++;
            }
            world_ptr += 2; // skip border right on y and left on y+1
        }
    }

    void world_generate(void) {
        const std::lock_guard<std::mutex> lock(mutex_world_update);
        points_health_check();
        events_process();
    }

    void view_draw_border(void) {
        point_t i;
        char border = (view_pos.y == 0) ? cell_border : ' ';
        for (i.y = i.x = 0; i.x < view_size.x; i.x++) {
            view.update(point_t{i.y, i.x}, border);
        }
        border = (view_pos.x == 0) ? cell_border : ' ';
        for (i.y = i.x = 0; i.y < view_size.y; i.y++) {
            view.update(point_t{i.y, i.x}, border);
        }
        border = (view_pos.y + view_size.y - 1 == world_size.y)
                     ? cell_border
                     : ' ';
        for (i.y = view_size.y - 1, i.x = 0; i.x < view_size.x;
             i.x++) {
            view.update(point_t{i.y, i.x}, border);
        }
        border = (view_pos.x + view_size.x - 1 == world_size.x)
                     ? cell_border
                     : ' ';
        for (i.y = 0, i.x = view_size.x - 1; i.y < view_size.y;
             i.y++) {
            view.update(point_t{i.y, i.x}, border);
        }
    }

    void view_draw_world(void) {
        const uint16_t limit_y = view_size.y - 1;
        const uint16_t limit_x = view_size.x - 1;
        cell_t *world_ptr = &world[view_pos.y + 1][view_pos.x + 1];
        uint16_t y;
        uint16_t x;
        const uint16_t a = world_size.x - view_size.x + 2; //remove multiplication operation from wrold[x][y] select by addition of a to ptr.

        view_alive_counter = 0;
        for (y = 1; y < limit_y; y++) {
            for (x = 1; x < limit_x; x++) {
                view_alive_counter += world_ptr->value;
                view.update(point_t{y,x}, (world_ptr->value) ? cell_alive : cell_dead);
                world_ptr++;
            }
            world_ptr += a;
        }
    }

    void view_refresh_world(void) {
        view_draw_world();
        view.refresh_view();
    }

    void clear(const point_t pos, const point_t size) {
        const std::lock_guard<std::mutex> lock(mutex_world_update);
        point_t i = {};
        point_t p = transform_pos_view_boundaries(pos);
        cell_t * world_ptr;
        uint16_t limit_y = p.y + size.y;
        uint16_t limit_x = p.x + size.x;

        events_clear();

        for (i.y = p.y; i.y < limit_y; i.y++) {
            world_ptr = &world[i.y][p.x];
            for (i.x = p.x; i.x < limit_x; i.x++) {
                if (world_ptr->value) {
                    point_set_value(world_ptr, false);
                }
                world_ptr++;
            }
        }

        view_refresh_world();
    }

    void fill(const point_t pos, const point_t size) {
        const std::lock_guard<std::mutex> lock(mutex_world_update);
        point_t i = {};
        point_t p = transform_pos_view_boundaries(pos);
        cell_t * world_ptr;
        uint16_t limit_y = p.y + size.y;
        uint16_t limit_x = p.x + size.x;

        events_clear();

        for (i.y = p.y; i.y < limit_y; i.y++) {
            world_ptr = &world[i.y][p.x];
            for (i.x = p.x; i.x < limit_x; i.x++) {
                point_set_value(world_ptr, true);
                world_ptr++;
            }
        }

        view_refresh_world();
    }

    void intro(void) {
        infest_random_view(1000);
        const char intro_line_1[14] = {"world of life"};
        const char intro_line_2[22] = {"by jenny vermeltfoort"};
        uint8_t i;
        uint8_t x;
        uint8_t y;

        for (i = 0; i < INTRO_DURATION_S * 10; i++) {
            world_generate();
            view_refresh_world();
            for (x = 0; x < 28; x++) {
                for (y = 0; y < 4; y++) {
                    view.update(
                        point_t{static_cast<uint16_t>(
                                    view_size.y / 2 - 2 + y),
                                static_cast<uint16_t>(
                                    view_size.x / 2 - 14 + x)},
                        ' ');
                }
            }
            for (x = 0; x < view_size.y; x++) {
                for (y = 0; y < view_size.x; y++) {
                    if (x == 0 || y == 0 || x == view_size.y - 1 ||
                        y == view_size.x - 1) {
                        view.update(point_t{x, y}, '@');
                    }
                }
            }
            for (x = 0; x < 13; x++) {
                view.update(point_t{static_cast<uint16_t>(
                                        view_size.y / 2 - 1),
                                    static_cast<uint16_t>(
                                        view_size.x / 2 - 7 + x)},
                            intro_line_1[x]);
            }
            for (x = 0; x < 21; x++) {
                view.update(
                    point_t{static_cast<uint16_t>(view_size.y / 2),
                            static_cast<uint16_t>(view_size.x / 2 -
                                                  11 + x)},
                    intro_line_2[x]);
            }
            view.refresh_view();
            std::this_thread::sleep_for(std::chrono::milliseconds(
                DEFAULT_REFRESH_RATE_INTRO));
        }
    }

   public:
    World() {
        view.set_print_callback(std::bind(&World::print_info, this));
    }

    void view_refresh_input(void) { view.refresh_input(); }

    void set_cell_alive(const char c) {
        set_cell_character(cell_alive, c);
    }
    void set_cell_dead(const char c) {
        set_cell_character(cell_dead, c);
    }
    void set_cell_border(const char c) {
        set_cell_character(cell_border, c);
    }
    void set_infest_random_view_cell_default(const uint32_t cells) {
        infest_random_view_cell_default = cells;
    }
    void set_infest_random_world_cell_default(const uint32_t cells) {
        infest_random_world_cell_default = cells;
    }
    void set_view_step_size_y(const uint32_t y) {
        view_step_size.y = y;
        view.refresh_info();
    }
    void set_view_step_size_x(const uint32_t x) {
        view_step_size.x = x;
        view.refresh_info();
    }
    void set_refresh_rate(const uint16_t rate) {
        refresh_rate = rate;
        view.refresh_info();
    }
    void set_cursor_pos(const point_t pos) {
        view.set_user_cursor_pos(pos);
        view.refresh_info();
        world_cursor_position = {
            static_cast<uint16_t>(view_pos.y +
                                  get_pos_cursor_view().y),
            static_cast<uint16_t>(view_pos.x +
                                  get_pos_cursor_view().x),
        };
    }

    point_t get_pos_cursor_view(void) {
        return view.get_pos_cursor_user();
    }

    void view_move(const point_t pos) {
        view_pos = transform_pos_view_boundaries(pos);
        view_draw_world();
        view_draw_border();
        view.refresh_view();
    }

    void view_move_left(void) {
        view_move(point_t{
            view_pos.y,
            static_cast<uint16_t>((view_pos.x < view_step_size.x)
                                      ? 0
                                      : view_pos.x -
                                            view_step_size.x),
        });
    }
    void view_move_right(void) {
        view_move(point_t{
            view_pos.y,
            static_cast<uint16_t>(view_pos.x + view_step_size.x),
        });
    }
    void view_move_top(void) {
        view_move(point_t{
            static_cast<uint16_t>((view_pos.y < view_step_size.y)
                                      ? 0
                                      : view_pos.y -
                                            view_step_size.y),
            view_pos.x,
        });
    }
    void view_move_bottom(void) {
        view_move(point_t{
            static_cast<uint16_t>(view_pos.y + view_step_size.y),
            view_pos.x,
        });
    }

    void infest_random(const point_t pos, const point_t size,
                       const uint32_t cells) {
        const std::lock_guard<std::mutex> lock(mutex_world_update);
        uint32_t i;
        point_t p;

        for (i = 0; i < std::min(WORLD_SIZE_X * WORLD_SIZE_Y -
                                     world_alive_counter,
                                 cells);
             i++) {
            p = {
                static_cast<uint16_t>(rand() % size.y + pos.y),
                static_cast<uint16_t>(rand() % size.x + pos.x),
            };
            point_set_value(&world[p.y][p.x], !world[p.y][p.x].value);
        }

        view_refresh_world();
    }
    void infest_random_view(uint32_t cells) {
        infest_random(
            view_pos, view_size,
            (cells == 0) ? infest_random_view_cell_default : cells);
    }
    void infest_random_world(uint32_t cells) {
        infest_random(
            world_start_pos, world_size_life,
            (cells == 0) ? infest_random_world_cell_default : cells);
    }

    void toggle_cursor_value(void) {
        const std::lock_guard<std::mutex> lock(mutex_world_update);
        event_add(&world[world_cursor_position.y][world_cursor_position.x]);
        events_process();
        view_refresh_world();
    }

    void clear_world(void) {
        clear(world_start_pos, world_size_life);
    }
    void clear_view(void) { clear(view_pos, view_size); }

    void reset_view(void) {
        view.reset();
        view.draw_frame();
        view.refresh_info();
        view_draw_border();
        view_refresh_world();
        view.set_user_cursor_pos(
            {static_cast<uint16_t>(view_size.y / 2),
             static_cast<uint16_t>(view_size.x / 2)});
    }

    void run(void) {
        intro();
        clear_world();
        reset_view();

        while (!flag_stop) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(refresh_rate));
            //fill(world_start_pos, world_size_life);
#if _TIMED == 1
            auto strt = std::chrono::high_resolution_clock::now();
#endif
            world_generate();
            view_refresh_world();
            view.refresh_info();
#if _TIMED == 1
            auto stp = std::chrono::high_resolution_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::microseconds>(
                    stp - strt);
            std::cout << std::endl << "Time taken by function: "
                      << duration.count() << " microseconds"
                      << std::endl;
#endif
            view.refresh_input();
        }
    }

    void stop(void) { flag_stop = true; }
};

void input_get_int(uint32_t **arr) {
    const uint8_t ANSCII_NUMBER_MASK = 0XF;
    char c;
    uint32_t number = 0;

    if (*arr == NULL || std::cin.eof()) {
        return;
    }

    std::cin.get(c);

    while (anscii_is_int(c) && !std::cin.eof()) {
        if (number * 10 +
                static_cast<uint16_t>(c & ANSCII_NUMBER_MASK) >
            number) {
            number = number * 10 +
                     static_cast<uint16_t>(c & ANSCII_NUMBER_MASK);
        }
        std::cin.get(c);
    }

    **arr = number;
    return input_get_int(++arr);
}

void cb_input_cursor_w(World &world) {
    point_t p = world.get_pos_cursor_view();
    if (p.y > 0) {
        p.y--;
    };
    world.set_cursor_pos(p);
}

void cb_input_cursor_s(World &world) {
    point_t p = world.get_pos_cursor_view();
    p.y++;
    world.set_cursor_pos(p);
}

void cb_input_cursor_a(World &world) {
    point_t p = world.get_pos_cursor_view();
    if (p.x > 0) {
        p.x--;
    };
    world.set_cursor_pos(p);
}

void cb_input_cursor_d(World &world) {
    point_t p = world.get_pos_cursor_view();
    p.x++;
    world.set_cursor_pos(p);
}

void cb_input_cursor_t(World &world) { world.toggle_cursor_value(); }
void cb_input_move_view_left(World &world) { world.view_move_left(); }
void cb_input_move_view_right(World &world) {
    world.view_move_right();
}
void cb_input_move_view_top(World &world) { world.view_move_top(); }
void cb_input_move_view_bottom(World &world) {
    world.view_move_bottom();
}

void cb_input_stop(World &world) { world.stop(); }
void cb_input_reset_view(World &world) { world.reset_view(); }

void cb_input_clear(World &world) {
    char c = std::cin.get();
    std::map<char, std::function<void(World &)>> map_callback{
        {'v', &World::clear_view},
        {'w', &World::clear_world},
    };

    if (map_callback.find(c) != map_callback.end()) {
        map_callback[c](world);
    }
}

void cb_input_infest(World &world) {
    char c = std::cin.get();
    uint32_t cells = 0;
    uint32_t *arr[2] = {&cells, NULL};
    std::map<char, std::function<void(World &, uint32_t)>>
        map_callback{
            {'v', &World::infest_random_view},
            {'w', &World::infest_random_world},
        };

    if (map_callback.find(c) != map_callback.end()) {
        input_get_int(arr);
        map_callback[c](world, cells);
    }
}

void cb_input_move_view(World &world) {
    point_t p;
    uint32_t y = 0;
    uint32_t x = 0;
    uint32_t *arr[3] = {&y, &x, NULL};
    p.y = y;
    p.x = x;
    input_get_int(arr);
    world.view_move(p);
}

void cb_input_parameter_refresh_rate(World &world) {
    uint32_t refresh_rate = 0;
    uint32_t *arr[2] = {&refresh_rate, NULL};

    input_get_int(arr);

    if (refresh_rate > 0) {
        world.set_refresh_rate(refresh_rate);
    }
}

void cb_input_parameter_view(World &world) {
    char c = std::cin.get();
    uint32_t number = 0;
    uint32_t *arr[2] = {&number, NULL};
    std::map<char, std::function<void(World &, uint32_t)>>
        map_callback{
            {'y', &World::set_view_step_size_y},
            {'x', &World::set_view_step_size_x},
        };

    if (map_callback.find(c) != map_callback.end()) {
        input_get_int(arr);
        map_callback[c](world, number);
    }
}

void cb_input_parameter_infest(World &world) {
    char c = std::cin.get();
    uint32_t number = 0;
    uint32_t *arr[2] = {&number, NULL};
    std::map<char, std::function<void(World &, uint32_t)>>
        map_callback{
            {'v', &World::set_infest_random_view_cell_default},
            {'w', &World::set_infest_random_world_cell_default},
        };

    if (map_callback.find(c) != map_callback.end()) {
        input_get_int(arr);
        map_callback[c](world, number);
    }
}

void cb_input_parameter_cell(World &world) {
    char c = std::cin.get();
    char v = std::cin.get();
    std::map<char, std::function<void(World &, char)>> map_callback{
        {'a', &World::set_cell_alive},
        {'d', &World::set_cell_dead},
        {'b', &World::set_cell_border},
    };

    if (map_callback.find(c) != map_callback.end()) {
        map_callback[c](world, v);
    }
}

void cb_input_parameter(World &world) {
    char c = std::cin.get();
    std::map<char, std::function<void(World &)>> map_callback{
        {'c', cb_input_parameter_cell},
        {'i', cb_input_parameter_infest},
        {'v', cb_input_parameter_view},
        {'r', cb_input_parameter_refresh_rate},
    };

    if (map_callback.find(c) != map_callback.end()) {
        map_callback[c](world);
    }
}

void loop_input(World &world) {
    char c;
    std::map<char, std::function<void(World &)>> map_callback{
        {'e', cb_input_stop},
        {'r', cb_input_reset_view},
        {'m', cb_input_move_view},
        {'c', cb_input_clear},
        {'i', cb_input_infest},
        {'w', cb_input_cursor_w},
        {'a', cb_input_cursor_a},
        {'s', cb_input_cursor_s},
        {'d', cb_input_cursor_d},
        {'t', cb_input_cursor_t},
        {'p', cb_input_parameter},
        {'8', cb_input_move_view_top},
        {'6', cb_input_move_view_right},
        {'4', cb_input_move_view_left},
        {'5', cb_input_move_view_bottom},
    };

    while (true) {
        std::cin.get(c);

        if (map_callback.find(c) != map_callback.end()) {
            map_callback[c](world);
            if (c == 'e') {
                break;
            }
        }

        world.view_refresh_input();
    }
}

int main() {
    World world;

    std::srand(std::time(nullptr));
    std::thread thread_world(&World::run, &world);

    loop_input(world);

    thread_world.join();

    return 1;
}