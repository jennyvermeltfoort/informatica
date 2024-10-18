

#include <chrono>
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
const uint16_t INTRO_DURATION_S = 5;
const uint16_t WORLD_SIZE_Y = 1000;
const uint16_t WORLD_SIZE_X = 1000;
const uint16_t VIEW_SIZE_Y = 40;
const uint16_t VIEW_SIZE_X = 150;

typedef char view_storage_t[VIEW_SIZE_Y][VIEW_SIZE_X];
typedef bool world_storage_t[WORLD_SIZE_Y][WORLD_SIZE_X];

const uint8_t ANSCII_SYMBOL_START = 33;
const uint8_t ANSCII_SYMBOL_END = 126;
const uint8_t ANSCII_NUMBER_0 = 48;
const uint8_t ANSCII_NUMBER_9 = 57;
const uint8_t ANSCII_NUMBER_MASK = 0XF;

class World;
class View;

typedef struct POINT_T {
    uint32_t y;
    uint32_t x;
} point_t;

typedef struct EVENT_T {
    point_t pos;
    bool value;
} event_t;

inline bool anscii_is_int(uint8_t c) {
    return (c >= ANSCII_NUMBER_0 && c <= ANSCII_NUMBER_9);
}
inline bool anscii_is_whitespace(uint8_t c) {
    return (c == ' ' || c == '\t');
}

inline bool anscii_is_symbol(uint8_t c) {
    return (c >= ANSCII_SYMBOL_START && c <= ANSCII_SYMBOL_END);
}

typedef enum ERRNO {
    ERRNO_OK = 0,
    ERRNO_INVALID,
} errno_e;

class View {
   private:
    point_t size = {VIEW_SIZE_Y, VIEW_SIZE_X};
    view_storage_t view = {};
    std::mutex mutex_cursor;
    point_t user_cursor = {VIEW_SIZE_Y + 1, VIEW_SIZE_X + 1};
    std::function<void(void)> cb_print_info;
    uint32_t line_input = size.y + 5;
    uint32_t line_info = size.y + 2;
    bool flag_cb_print_info_set = false;

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

    void cursor_erase_display(uint8_t n) {
        std::cout << "\033[" << +n << "J" << std::flush;
    }
    void cursor_erase_line(uint8_t n) {
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
        cursor_erase_display(2);
        cursor_move(point_t{size.y + 1, 1});
        for (uint16_t i = 0; i < size.x; i++) {
            std::cout << "-";
        }
        std::cout << std::endl << std::endl << std::endl;
        for (uint16_t i = 0; i < size.x; i++) {
            std::cout << "-";
        }
        std::cout << std::endl;
        std::cout << ">> ";
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
        std::cout << ">> ";
    }

    point_t get_pos_cursor_user(void) { return user_cursor; }

    void set_user_cursor_pos(point_t pos) {
        pos.y = (pos.y > size.y - 1) ? size.y - 1 : pos.y;
        pos.x = (pos.x > size.x - 1) ? size.x - 1 : pos.x;
        user_cursor = pos;
    }

    point_t get_size(void) { return size; }

    void update(const point_t pos, const char c) {
        view[pos.y][pos.x] = c;
    }

    void refresh_view(void) {
        const std::lock_guard<std::mutex> lock(mutex_cursor);
        uint8_t i_x;
        uint8_t i_y;

        cursor_save();
        cursor_move_end();
        cursor_erase_display(1);
        cursor_move_start();

        for (i_y = 0; i_y < size.y; i_y++) {
            for (i_x = 0; i_x < size.x; i_x++) {
                if (i_x == user_cursor.x && i_y == user_cursor.y) {
                    std::cout << "\033[105;31m" << view[i_y][i_x]
                              << "\033[39;49m";
                } else {
                    std::cout << view[i_y][i_x];
                }
            }
            std::cout << std::endl;
        }
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
    point_t world_size = {WORLD_SIZE_Y, WORLD_SIZE_X};
    world_storage_t world = {{0}};
    View view;
    point_t view_size = view.get_size();
    point_t view_step_size = {view_size.y - 5, view_size.x - 5};
    point_t view_pos = {
        world_size.y / 2 - view_size.y / 2,
        world_size.x / 2 - view_size.x / 2,
    };
    uint16_t refresh_rate = DEFAULT_REFRESH_RATE;
    point_t world_start_pos = point_t{1, 1};
    point_t world_size_life = {world_size.y - 2, world_size.x - 2};
    std::mutex mutex_world_update;
    uint32_t world_alive_counter = 0;
    uint32_t view_alive_counter = 0;
    std::vector<event_t> events = {};
    bool flag_stop = false;

    void print_info(void) {
        std::cout << "Cursor[y,x]: '" << +get_pos_cursor_world().y
                  << "," << +get_pos_cursor_world().x << "'; ";
        std::cout << "Cell symbol[alive, dead, "
                     "border]: '"
                  << cell_alive << "," << cell_dead << ","
                  << cell_border << "'; ";
        std::cout << "Refresh rate: '" << +refresh_rate << "'; ";
        std::cout << "Step size[y,x]: '" << +view_step_size.y << ","
                  << +view_step_size.x << "'; ";
        std::cout << std::endl;
        std::cout << "Cells alive[world,view]: '"
                  << +world_alive_counter << ","
                  << +view_alive_counter << "'; ";
        std::cout << "Default generation count[world,view]: '"
                  << +infest_random_view_cell_default << ","
                  << +infest_random_world_cell_default << "'; ";
    }

    bool point_get_value(const point_t pos) {
        return world[pos.y][pos.x];
    }
    void point_set_value(const point_t pos, bool value) {
        world_alive_counter += (value) ? 1 : -1;
        world[pos.y][pos.x] = value;
    }
    bool is_point_in_view(const point_t pos) {
        return (
            view_pos.y <= pos.y && pos.y < view_pos.y + view_size.y &&
            view_pos.x <= pos.x && pos.x < view_pos.x + view_size.x);
    }
    bool is_point_on_edge(const point_t pos) {
        return (pos.x == 0 || pos.y == 0 || pos.x == world_size.x ||
                pos.y == world_size.y);
    }
    uint16_t point_count_neighbour(const point_t p) {
        return world[p.y - 1][p.x - 1] + world[p.y - 1][p.x] +
               world[p.y - 1][p.x + 1] + world[p.y][p.x - 1] +
               world[p.y][p.x + 1] + world[p.y + 1][p.x - 1] +
               world[p.y + 1][p.x] + world[p.y + 1][p.x + 1];
    }

    point_t transform_pos_view_boundaries(const point_t pos) {
        return {
            (pos.y + view_size.y > world_size.y)
                ? world_size.y - view_size.y + 1
                : pos.y,
            (pos.x + view_size.x > world_size.x)
                ? world_size.x - view_size.x + 1
                : pos.x,
        };
    }

    void view_update_bool(const point_t pos, bool value) {
        view.update(pos, (value) ? cell_alive : cell_dead);
    }

    point_t transform_pos_world_to_view(point_t pos) {
        return {pos.y - view_pos.y, pos.x - view_pos.x};
    }

    void events_process(void) {
        for (const event_t e : events) {
            if (!is_point_on_edge(e.pos)) {
                point_set_value(e.pos, e.value);

                if (is_point_in_view(e.pos)) {
                    view_alive_counter += (e.value) ? 1 : -1;
                    view_update_bool(
                        transform_pos_world_to_view(e.pos), e.value);
                }
            }
        }
        events.clear();
    }

    void points_health_check(void) {
        point_t i = {};
        uint16_t neighbour_count;
        auto start = std::chrono::high_resolution_clock::now();

        for (i.y = 1; i.y < world_size.y - 1; i.y++) {
            for (i.x = 1; i.x < world_size.x - 1; i.x++) {
                neighbour_count = point_count_neighbour(i);
                if (point_get_value(i) &&
                    (neighbour_count > 3 || neighbour_count < 2)) {
                    events.push_back(event_t{i, false});
                } else if (!point_get_value(i) &&
                           neighbour_count == 3) {
                    events.push_back(event_t{i, true});
                }
            }
        }

        auto sp = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(
                sp - start);
        std::cout << duration.count() << std::endl;
    }

    void update_world(void) {
        const std::lock_guard<std::mutex> lock(mutex_world_update);
        points_health_check();
        events_process();
    }

    void set_cell_character(char &cell, char c) {
        if (anscii_is_symbol(c) || c == ' ') {
            cell = c;
            view_refresh();
        }
    }

   public:
    World() {
        view.set_print_callback(std::bind(&World::print_info, this));
    }

    void view_refresh_input(void) { view.refresh_input(); }

    void set_cell_alive(char c) { set_cell_character(cell_alive, c); }
    void set_cell_dead(char c) { set_cell_character(cell_dead, c); }
    void set_cell_border(char c) {
        set_cell_character(cell_border, c);
    }
    void set_infest_random_view_cell_default(uint32_t cells) {
        infest_random_view_cell_default = cells;
    }
    void set_infest_random_world_cell_default(uint32_t cells) {
        infest_random_world_cell_default = cells;
    }
    void set_view_step_size_y(uint32_t y) {
        view_step_size.y = y;
        view.refresh_info();
    }
    void set_view_step_size_x(uint32_t x) {
        view_step_size.x = x;
        view.refresh_info();
    }
    void set_refresh_rate(uint16_t rate) {
        refresh_rate = rate;
        view.refresh_info();
    }
    void set_cursor_pos(point_t pos) {
        view.refresh_info();
        view.set_user_cursor_pos(pos);
    }

    point_t get_pos_cursor_world(void) {
        return {
            view_pos.y + get_pos_cursor_view().y,
            view_pos.x + get_pos_cursor_view().x,
        };
    }
    point_t get_pos_cursor_view(void) {
        return view.get_pos_cursor_user();
    }
    point_t get_world_size(void) { return world_size; }
    point_t get_view_size(void) { return view_size; }

    void view_move(const point_t pos) {
        point_t i = {};

        view_pos = transform_pos_view_boundaries(pos);
        view_alive_counter = 0;

        for (i.y = view_pos.y; i.y < view_pos.y + view_size.y;
             i.y++) {
            for (i.x = view_pos.x; i.x < view_pos.x + view_size.x;
                 i.x++) {
                if (is_point_on_edge(i)) {
                    view.update(
                        point_t{i.y - view_pos.y, i.x - view_pos.x},
                        cell_border);
                } else {
                    view_alive_counter += point_get_value(i);
                    view_update_bool(
                        point_t{i.y - view_pos.y, i.x - view_pos.x},
                        point_get_value(i));
                }
            }
        }

        view.refresh_view();
    }

    void view_refresh(void) { view_move(view_pos); }
    void view_move_left(void) {
        view_move(point_t{
            view_pos.y,
            (view_pos.x < view_step_size.x)
                ? 0
                : view_pos.x - view_step_size.x,
        });
    }
    void view_move_right(void) {
        view_move(point_t{
            view_pos.y,
            view_pos.x + view_step_size.x,
        });
    }
    void view_move_top(void) {
        view_move(point_t{
            (view_pos.y < view_step_size.y)
                ? 0
                : view_pos.y - view_step_size.y,
            view_pos.x,
        });
    }
    void view_move_bottom(void) {
        view_move(point_t{
            view_pos.y + view_step_size.y,
            view_pos.x,
        });
    }

    void infest_random(const point_t pos, const point_t size,
                       uint32_t cells) {
        const std::lock_guard<std::mutex> lock(mutex_world_update);
        uint32_t i;
        point_t p;

        for (i = 0; i < cells; i++) {
            p = {
                rand() % size.y + pos.y,
                rand() % size.x + pos.x,
            };
            point_set_value(p, !point_get_value(p));
        }
        view_refresh();
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
        const point_t p = get_pos_cursor_world();
        events.push_back(event_t{p, !point_get_value(p)});
        events_process();
        view_refresh();
    }

    void clear(const point_t pos, const point_t size) {
        const std::lock_guard<std::mutex> lock(mutex_world_update);
        point_t i = {};
        point_t p = transform_pos_view_boundaries(pos);

        events.clear();

        for (i.y = p.y; i.y < p.y + size.y; i.y++) {
            for (i.x = p.x; i.x < p.x + size.x; i.x++) {
                if (point_get_value(i)) {
                    events.push_back(event_t{i, false});
                }
            }
        }

        events_process();
    }

    void clear_world(void) {
        clear(world_start_pos, world_size_life);
    }
    void clear_view(void) { clear(view_pos, view_size); }
    void reset_view(void) {
        view.reset();
        view_refresh();
    }

    void intro(void) {
        infest_random_view(1000);
        const char intro_line_1[14] = {"world of life"};
        const char intro_line_2[22] = {"by jenny vermeltfoort"};
        uint8_t i;
        uint8_t x;
        uint8_t y;

        for (i = 0; i < INTRO_DURATION_S * 10; i++) {
            update_world();
            for (x = 0; x < 28; x++) {
                for (y = 0; y < 4; y++) {
                    view.update(point_t{view_size.y / 2 - 2 + y,
                                        view_size.x / 2 - 14 + x},
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
                view.update(point_t{view_size.y / 2 - 1,
                                    view_size.x / 2 - 7 + x},
                            intro_line_1[x]);
            }
            for (x = 0; x < 21; x++) {
                view.update(point_t{view_size.y / 2,
                                    view_size.x / 2 - 11 + x},
                            intro_line_2[x]);
            }
            view.refresh_view();
            std::this_thread::sleep_for(
                std::chrono::milliseconds(100));
        }
    }

    void run(void) {
        intro();
        clear_world();
        reset_view();
        view.draw_frame();
        view.refresh_info();
        view.set_user_cursor_pos({view_size.y / 2, view_size.x / 2});

        while (!flag_stop) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(refresh_rate));
            update_world();
            view.refresh_view();
            view.refresh_info();
        }
    }

    void stop(void) { flag_stop = true; }
};

void input_get_int(uint32_t **arr) {
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
    point_t p = {};
    uint32_t *arr[3] = {&p.y, &p.x, NULL};
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

typedef struct NODE_T node_t;
typedef struct NODE_T {
    node_t *next_y;
    node_t *next_x;
    node_t *next_xy;
    node_t *prev;
    point_t p;
    uint16_t d;
} node_t;

node_t *node_list = NULL;

node_t *node_make_new(point_t pos, uint16_t min_pos) {
    return new node_t{
        .next_y = NULL,
        .next_x = NULL,
        .next_xy = NULL,
        .prev = NULL,
        .p = pos,
        .d = min_pos,
    };
}

bool node_is_pos_equal(node_t *node, point_t pos) {
    return (node->p.y == pos.y && node->p.x == pos.x);
}

bool node_is_before_pos_xy(node_t *node, point_t pos) {
    return (node->p.y < pos.y && node->p.x < pos.x);
}

bool node_is_before_pos_y(node_t *node, point_t pos) {
    return (node->p.y < pos.y);
}

bool node_is_before_pos_x(node_t *node, point_t pos) {
    return (node->p.x < pos.x);
}

bool node_is_after_pos(node_t *node, point_t pos) {
    return (node->p.y > pos.y || node->p.x > pos.x);
}

bool node_is_after_pos_xy(node_t *node, point_t pos) {
    return (node->p.y > pos.y && node->p.x > pos.x);
}

bool node_is_after_pos_y(node_t *node, point_t pos) {
    return (node->p.y > pos.y);
}

bool node_is_after_pos_x(node_t *node, point_t pos) {
    return (node->p.x > pos.x);
}

node_t *node_list_add(node_t **node_ptr, point_t pos) {
    node_t *node_new;
    node_t *node_current = *node_ptr;
    uint16_t min_pos = (pos.x < pos.y) ? pos.x : pos.y;

    if (node_current == NULL) {
        *node_ptr = node_make_new(pos, min_pos);
        return NULL;
    }

    if (node_is_pos_equal(node_current, pos)) {
        return NULL;
    }

    if (node_current->d > min_pos) {
        node_new = node_make_new(pos, min_pos);
        node_new->next_xy = node_current;
        node_current->prev = node_new;
        *node_ptr = node_new;
        return NULL;
    } else if (node_current->d == min_pos &&
               node_is_after_pos(node_current, pos)) {
        // on same diagnal
        node_new = node_make_new(pos, min_pos);
        if (node_is_after_pos_x(node_current, pos)) {
            node_new->next_x = node_current;
            node_new->next_xy = node_current->next_xy;
        } else if (node_is_after_pos_y(node_current, pos)) {
            node_new->next_y = node_current;
            node_new->next_xy = node_current->next_xy;
        }
        node_current->next_xy = NULL;
        node_current->prev = node_new;
        *node_ptr = node_new;
        return NULL;
    }

    if (node_is_before_pos_xy(node_current, pos)) {
        node_new = node_list_add(&node_current->next_xy, pos);
    } else if (node_is_before_pos_x(node_current, pos)) {
        node_new = node_list_add(&node_current->next_x, pos);
    } else {
        node_new = node_list_add(&node_current->next_y, pos);
    }

    node_new->prev = node_current;
    return node_new;
}

void node_print_list(node_t **node_ptr) {
    node_t *node = *node_ptr;

    if (node == NULL) {
        return;
    }

    std::cout << "Node: " << node->p.y << "," << node->p.x << ";"
              << std::endl;

    node_print_list(&node->next_x);
    node_print_list(&node->next_y);
    node_print_list(&node->next_xy);
}

int main() {
    node_list_add(&node_list, point_t{1, 1});
    node_list_add(&node_list, point_t{0, 5});
    node_list_add(&node_list, point_t{0, 4});
    node_list_add(&node_list, point_t{1, 0});

    node_print_list(&node_list);
    return 1;

    World world;

    std::srand(std::time(nullptr));

    std::thread thread_world(&World::run, &world);

    loop_input(world);

    thread_world.join();

    return 1;
}