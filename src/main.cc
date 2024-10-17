

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

const uint16_t REFRESH_RATE_WORLD_MS = 200;
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

typedef void (*callback_input_t)(World &world);
typedef void (World::*callback_move_view_half)(void);
typedef void (World::*callback_infest_random)(uint32_t cells);
typedef void (World::*callback_parameter_cell)(char c);
typedef void (World::*callback_parameter_view)(uint32_t n);
typedef void (World::*callback_parameter_infest)(uint32_t n);

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

typedef struct VIEW_INFO_T {
    point_t pos_cursor_world;
    point_t step_size;
    uint32_t refresh_rate;
    char cells_alive_char;
    char cells_border_char;
    char cells_dead_char;
    uint32_t alive_counter;
} view_info_t;

class View {
   private:
    point_t size = {VIEW_SIZE_Y, VIEW_SIZE_X};
    view_storage_t view = {};
    std::mutex mutex_cursor;
    point_t user_cursor = {VIEW_SIZE_Y + 1, VIEW_SIZE_X + 1};
    view_info_t info = {};

    void cursor_move(const point_t point) {
        std::cout << "\033[" << +point.y << ";" << +point.x << "H"
                  << std::flush;
    }
    void cursor_move_start(void) { cursor_move(point_t{0, 0}); }
    void cursor_move_end(void) { cursor_move(size); }
    void cursor_move_input(void) {
        point_t p = {size.y + 4, 1};
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
        refresh_info();
        refresh_input();
    }

    void refresh_info() {
        const std::lock_guard<std::mutex> lock(mutex_cursor);
        cursor_save();
        cursor_move(point_t{size.y + 1, 1});
        for (uint16_t i = 0; i < size.x; i++) {
            std::cout << "-";
        }

        std::cout << std::endl;
        cursor_erase_line(0);
        std::cout << "Cursor[y,x]: '" << +info.pos_cursor_world.y
                  << "," << +info.pos_cursor_world.x << "'; ";
        std::cout << "Cell symbol[alive, dead, "
                     "border]: '"
                  << info.cells_alive_char << ","
                  << info.cells_dead_char << ","
                  << info.cells_border_char << "'; ";
        std::cout << "Refresh rate: '" << +info.refresh_rate << "'; ";
        std::cout << "Step size[y,x]: '" << +info.step_size.y << ","
                  << +info.step_size.x << "'; ";
        std::cout << "Cells alive: '" << +info.alive_counter << "'; ";

        std::cout << std::endl;

        for (uint16_t i = 0; i < size.x; i++) {
            std::cout << "-";
        }
        std::cout << std::endl;
        cursor_restore();
    }

    void refresh_input(void) {
        const std::lock_guard<std::mutex> lock(mutex_cursor);
        cursor_move_input();
        cursor_erase_line(0);
        std::cout << ">> ";
    }

    view_info_t get_info(void) { return info; }
    void set_info(const view_info_t &i) {
        info = i;
        refresh_info();
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

    void refresh(void) {
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
    char cell_alive = '&';
    char cell_dead = ' ';
    char cell_border = '@';
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
    uint16_t refresh_rate = REFRESH_RATE_WORLD_MS;
    point_t world_start_pos = point_t{1, 1};
    point_t world_size_life = {world_size.y - 2, world_size.x - 2};
    std::mutex mutex_world_update;
    uint32_t alive_counter = 0;

    bool flag_stop = false;

    std::vector<event_t> events = {};

    bool point_get_value(const point_t pos) {
        return world[pos.y][pos.x];
    }
    void point_set_value(const point_t pos, bool value) {
        alive_counter += (value) ? 1 : -1;
        world[pos.y][pos.x] = value;
    }

    uint16_t point_count_neighbour(const point_t p) {
        return world[p.y - 1][p.x - 1] + world[p.y - 1][p.x] +
               world[p.y - 1][p.x + 1] + world[p.y][p.x - 1] +
               world[p.y][p.x + 1] + world[p.y + 1][p.x - 1] +
               world[p.y + 1][p.x] + world[p.y + 1][p.x + 1];
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
    void refresh_input(void) { view.refresh_input(); }

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
        view_update_info();
    }
    void set_view_step_size_x(uint32_t x) {
        view_step_size.x = x;
        view_update_info();
    }
    void set_refresh_rate(uint16_t rate) {
        refresh_rate = rate;
        view_update_info();
    }
    void set_cursor_pos(point_t pos) {
        view_update_info();
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

    void view_update_info(void) {
        view_info_t info;
        info = view.get_info();
        info.pos_cursor_world = get_pos_cursor_world();
        info.cells_alive_char = cell_alive;
        info.cells_dead_char = cell_dead;
        info.cells_border_char = cell_border;
        info.refresh_rate = refresh_rate;
        info.step_size = view_step_size;
        info.alive_counter = alive_counter;
        view.set_info(info);
    }

    void view_move(const point_t pos) {
        point_t i = {};
        view_pos = transform_pos_view_boundaries(pos);

        for (i.y = view_pos.y; i.y < view_pos.y + view_size.y;
             i.y++) {
            for (i.x = view_pos.x; i.x < view_pos.x + view_size.x;
                 i.x++) {
                if (is_point_on_edge(i)) {
                    view.update(
                        point_t{i.y - view_pos.y, i.x - view_pos.x},
                        cell_border);
                } else {
                    view_update_bool(
                        point_t{i.y - view_pos.y, i.x - view_pos.x},
                        point_get_value(i));
                }
            }
        }

        view_update_info();
        view.refresh();
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

        cells = (1000000 < cells) ? 1000000 : cells;

        for (i = 0; i < cells; i++) {
            p = {
                rand() % size.y + pos.y,
                rand() % size.x + pos.x,
            };
            point_set_value(p, !point_get_value(p));
        }
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

    void run(void) {
        view_move(view_pos);
        view.set_user_cursor_pos({view_size.y / 2, view_size.x / 2});

        while (!flag_stop) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(refresh_rate));
            update_world();
            view_update_info();
            view.refresh();
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

void cb_input_stop(World &world) { world.stop(); }

void cb_input_clear(World &world) {
    char c = std::cin.get();
    std::map<char, callback_move_view_half> map_callback{
        {'v', &World::clear_view},
        {'w', &World::clear_world},
    };

    if (map_callback.find(c) != map_callback.end()) {
        (world.*map_callback[c])();
    }
}

void cb_input_infest(World &world) {
    char c = std::cin.get();
    uint32_t cells = 0;
    uint32_t *arr[2] = {&cells, NULL};
    std::map<char, callback_infest_random> map_callback{
        {'v', &World::infest_random_view},
        {'w', &World::infest_random_world},
    };

    if (map_callback.find(c) != map_callback.end()) {
        input_get_int(arr);
        (world.*map_callback[c])(cells);
    }
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
    std::map<char, callback_parameter_view> map_callback{
        {'y', &World::set_view_step_size_y},
        {'x', &World::set_view_step_size_x},
    };

    if (map_callback.find(c) != map_callback.end()) {
        input_get_int(arr);
        (world.*map_callback[c])(number);
    }
}

void cb_input_parameter_infest(World &world) {
    char c = std::cin.get();
    uint32_t number = 0;
    uint32_t *arr[2] = {&number, NULL};
    std::map<char, callback_parameter_infest> map_callback{
        {'v', &World::set_infest_random_view_cell_default},
        {'w', &World::set_infest_random_world_cell_default},
    };

    if (map_callback.find(c) != map_callback.end()) {
        input_get_int(arr);
        (world.*map_callback[c])(number);
    }
}

void cb_input_parameter_cell(World &world) {
    char c = std::cin.get();
    char v = std::cin.get();
    std::map<char, callback_parameter_cell> map_callback{
        {'a', &World::set_cell_alive},
        {'d', &World::set_cell_dead},
        {'b', &World::set_cell_border},
    };

    if (map_callback.find(c) != map_callback.end()) {
        (world.*map_callback[c])(v);
    }
}

void cb_input_parameter(World &world) {
    char c = std::cin.get();
    std::map<char, callback_input_t> map_callback{
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
    std::map<char, callback_input_t> map_callback{
        {'e', cb_input_stop},
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

        world.refresh_input();
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