

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <map>
#include <thread>
#include <vector>

const char CELL_ALIVE = '&';
const char CELL_DEAD = ' ';
const char CELL_BORDER = '@';
const uint16_t REFRESH_RATE_WORLD_MS = 200;
const uint16_t WORLD_SIZE_Y = 1000;
const uint16_t WORLD_SIZE_X = 1000;
const uint16_t VIEW_SIZE_Y = 20;
const uint16_t VIEW_SIZE_X = 200;
typedef char view_storage_t[VIEW_SIZE_Y][VIEW_SIZE_X];
typedef bool world_storage_t[WORLD_SIZE_Y][WORLD_SIZE_X];

const uint8_t ANSCII_NUMBER_0 = 48;
const uint8_t ANSCII_NUMBER_9 = 57;
const uint8_t ANSCII_NUMBER_MASK = 0XF;

class World;
class View;

typedef struct POINT_T {
    uint32_t y;
    uint32_t x;
} point_t;

typedef void (*callback_input_t)(World &world);

inline bool anscii_is_int(uint8_t c) { return (c >= ANSCII_NUMBER_0 && c <= ANSCII_NUMBER_9); }
inline bool anscii_is_whitespace(uint8_t c) { return (c == ' ' || c == '\t'); }

typedef enum ERRNO {
    ERRNO_OK = 0,
    ERRNO_INVALID,
} errno_e;

class View {
   private:
    point_t size = {VIEW_SIZE_Y, VIEW_SIZE_X};
    view_storage_t view = {};

    void cursor_move(const point_t point) {
        std::cout << "\033[" << +point.y << ";" << +point.x << "H" << std::flush;
    }
    void cursor_move_start(void) { cursor_move(point_t{0, 0}); }
    void cursor_move_end(void) { cursor_move(size); }
    void cursor_move_input(void) {
        point_t p = {size.y + 1, 1};
        cursor_move(p);
    }

    void cursor_erase_display(uint8_t n) { std::cout << "\033[" << +n << "J" << std::flush; }
    void cursor_erase_line(uint8_t n) { std::cout << "\033[" << +n << "K" << std::flush; }

    void cursor_save(void) { std::cout << "\033[s" << std::flush; }
    void cursor_restore(void) { std::cout << "\033[u" << std::flush; }

   public:
    View() {
        cursor_erase_display(2);
        cursor_move_input();
    }

    point_t get_size(void) { return size; }

    void update(const point_t pos, const char c) { view[pos.y][pos.x] = c; }

    void refresh(void) {
        uint8_t i_x;
        uint8_t i_y;

        cursor_save();
        cursor_move_end();
        cursor_erase_display(1);
        cursor_move_start();

        for (i_y = 0; i_y < size.y; i_y++) {
            for (i_x = 0; i_x < size.x; i_x++) {
                std::cout << view[i_y][i_x];
            }
            std::cout << std::endl;
        }
        std::cout << std::flush;
        cursor_restore();
    }

    void refresh_input(void) {
        cursor_move_input();
        cursor_erase_line(0);
    }
};

// TODO make events with value.
class World {
   private:
    point_t world_size = {WORLD_SIZE_Y, WORLD_SIZE_X};
    point_t world_size_limited = {world_size.y - 2, world_size.x - 2};
    world_storage_t world = {{0}};
    View view;
    point_t view_size = view.get_size();
    point_t view_pos = {
        world_size.y / 2 - view_size.y / 2,
        world_size.x / 2 - view_size.x / 2,
    };
    uint16_t refresh_rate = REFRESH_RATE_WORLD_MS;
    point_t world_start_pos = point_t{1, 1};

    bool flag_stop = false;

    std::vector<point_t> events_toggle = {};

    bool point_get_value(const point_t pos) { return world[pos.y][pos.x]; }
    void point_set_value(const point_t pos, bool value) { world[pos.y][pos.x] = value; }
    void point_toggle(const point_t pos) { point_set_value(pos, !point_get_value(pos)); }

    uint16_t point_count_neighbour(const point_t p) {
        return world[p.y - 1][p.x - 1] + world[p.y - 1][p.x] + world[p.y - 1][p.x + 1] +
               world[p.y][p.x - 1] + world[p.y][p.x + 1] + world[p.y + 1][p.x - 1] +
               world[p.y + 1][p.x] + world[p.y + 1][p.x + 1];
    }

    void events_toggle_process(void) {
        for (const point_t pos : events_toggle) {
            point_toggle(pos);
        }
    }

    void points_health_check(void) {
        point_t i = {};
        uint16_t neighbour_count;

        for (i.y = 1; i.y < world_size.y - 1; i.y++) {
            for (i.x = 1; i.x < world_size.x - 1; i.x++) {
                neighbour_count = point_count_neighbour(i);
                if (point_get_value(i) && (neighbour_count > 3 || neighbour_count < 2)) {
                    events_toggle.push_back(i);
                } else if (!point_get_value(i) && neighbour_count == 3) {
                    events_toggle.push_back(i);
                }
            }
        }
    }

    void view_update(const point_t pos, bool value) {
        view.update(pos, (value) ? CELL_ALIVE : CELL_DEAD);
    }

   public:
    void view_update() {
        point_t p;
        point_t pos;

        while (!events_toggle.empty()) {
            pos = events_toggle.back();
            events_toggle.pop_back();
            if (view_pos.y <= pos.y && pos.y < view_pos.y + view_size.y && view_pos.x <= pos.x &&
                pos.x < view_pos.x + view_size.x) {
                p = {
                    pos.y - view_pos.y,
                    pos.x - view_pos.x,
                };
                view_update(p, point_get_value(pos));
            }
        }

        view.refresh();
    }

    void refresh_input(void) { view.refresh_input(); }

    point_t view_pos_make_normalized(const point_t pos) {
        return {
            (pos.y + view_size.y > world_size.y) ? world_size.y - view_size.y + 1 : pos.y,
            (pos.x + view_size.x > world_size.x) ? world_size.x - view_size.x + 1 : pos.x,
        };
    }

    void view_move(const point_t pos) {
        point_t i = {};
        view_pos = view_pos_make_normalized(pos);

        for (i.y = view_pos.y; i.y < view_pos.y + view_size.y; i.y++) {
            for (i.x = view_pos.x; i.x < view_pos.x + view_size.x; i.x++) {
                if (i.y == 0 || i.y == world_size.y || i.x == 0 || i.x == world_size.x) {
                    view.update(point_t{i.y - view_pos.y, i.x - view_pos.x}, CELL_BORDER);
                } else {
                    view_update(point_t{i.y - view_pos.y, i.x - view_pos.x}, point_get_value(i));
                }
            }
        }
    }

    void view_move_half_left(void) {
        view_move(point_t{
            view_pos.y,
            (static_cast<int32_t>(view_pos.x) - static_cast<int32_t>(view_size.x) / 2 < 0)
                ? 0
                : view_pos.x - view_size.x / 2,
        });
    }

    void view_move_half_right(void) {
        view_move(point_t{
            view_pos.y,
            view_pos.x + view_size.x / 2,
        });
    }

    void view_move_half_top(void) {
        view_move(point_t{
            (static_cast<int32_t>(view_pos.y) - static_cast<int32_t>(view_size.y) / 2 < 0)
                ? 0
                : view_pos.y - view_size.y / 2,
            view_pos.x,
        });
    }

    void view_move_half_bottom(void) {
        view_move(point_t{
            view_pos.y + view_size.y / 2,
            view_pos.x,
        });
    }

    void infest_random(const point_t pos, const point_t size, uint32_t groups) {
        uint32_t i;
        point_t p;

        for (i = 0; i < groups; i++) {
            p = {
                rand() % size.y + pos.y,
                rand() % size.x + pos.x,
            };
            point_set_value(p, 1);
        }
    }

    void clear(const point_t pos, const point_t size) {
        point_t i = {};
        point_t p = view_pos_make_normalized(pos);

        std::cout << +p.y << "," << +p.x << std::endl;
        for (i.y = p.y; i.y < size.y - 1; i.y++) {
            for (i.x = p.x; i.x < size.x - 1; i.x++) {
                events_toggle.push_back(i);
            }
        }

        view_move(view_pos);
    }

    void clear_world(void) { clear(world_start_pos, world_size); }
    void clear_view(void) { clear(view_pos, view_size); }

    void run(void) {
        infest_random(world_start_pos, world_size_limited, 1000000);
        view_move(view_pos);
        view_update();

        while (!flag_stop) {
            std::this_thread::sleep_for(std::chrono::milliseconds(refresh_rate));
            points_health_check();
            events_toggle_process();
            view_update();
            // std::cout << "pos " << view_pos.y << "," << view_pos.x << std::endl;
        }
    }

    point_t get_world_size(void) { return world_size; }
    point_t get_view_size(void) { return view_size; }

    void set_refresh_rate(uint16_t rate) { refresh_rate = rate; }

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
        number = number * 10 + static_cast<uint16_t>(c & ANSCII_NUMBER_MASK);
        std::cin.get(c);
    }

    **arr = number;
    return input_get_int(++arr);
}

void cb_input_stop(World &world) { world.stop(); }

void cb_input_refresh_rate(World &world) {
    uint32_t refresh_rate = 0;
    uint32_t *arr[2] = {&refresh_rate, NULL};

    input_get_int(arr);

    if (refresh_rate > 0) {
        world.set_refresh_rate(refresh_rate);
    }
}
typedef void (World::*callback_move_view_half)(void);

void cb_input_move_view(World &world) {
    point_t p = {};
    uint32_t *arr[3] = {&p.y, &p.x, NULL};
    char c;

    if (anscii_is_int(std::cin.peek())) {
        input_get_int(arr);
        world.view_move(p);
    } else {
        c = std::cin.get();
        std::map<char, callback_move_view_half> map_callback{
            {'l', &World::view_move_half_left},
            {'r', &World::view_move_half_right},
            {'t', &World::view_move_half_top},
            {'b', &World::view_move_half_bottom},
        };

        if (map_callback.find(c) != map_callback.end()) {
            (world.*map_callback[c])();
        }
    }
}

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

void loop_input(World &world) {
    char c;
    std::map<char, callback_input_t> map_callback{
        {'s', cb_input_stop},
        {'r', cb_input_refresh_rate},
        {'m', cb_input_move_view},
        {'c', cb_input_clear},
    };

    while (true) {
        std::cin.get(c);

        if (map_callback.find(c) != map_callback.end()) {
            map_callback[c](world);
            if (c == 's') {
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