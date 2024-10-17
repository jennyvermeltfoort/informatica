

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <map>
#include <thread>
#include <vector>
#include <mutex>

const char CELL_ALIVE = '&';
const char CELL_DEAD = ' ';
const char CELL_BORDER = '@';
const uint16_t REFRESH_RATE_WORLD_MS = 200;
const uint16_t WORLD_SIZE_Y = 1000;
const uint16_t WORLD_SIZE_X = 1000;
const uint16_t VIEW_SIZE_Y = 10;
const uint16_t VIEW_SIZE_X = 150;
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

typedef struct EVENT_T {
    point_t pos;
    bool value;
} event_t;

typedef void (*callback_input_t)(World &world);
typedef void (World::*callback_move_view_half)(void);
typedef void (World::*callback_infest_random)(uint32_t cells);

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
    std::mutex mutex_cursor;
    point_t user_cursor = {VIEW_SIZE_Y +1, VIEW_SIZE_X + 1};

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

    point_t get_user_cursor(void) {
        return user_cursor;
    }

    void set_user_cursor(point_t pos){
        pos.y = (pos.y > size.y - 1) ? size.y - 1: pos.y;
        pos.x = (pos.x > size.x - 1) ? size.x - 1 : pos.x;
        user_cursor = pos;
    }

    point_t get_size(void) { return size; }

    void update(const point_t pos, const char c) { view[pos.y][pos.x] = c; }

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
                if (i_x == user_cursor.x && i_y == user_cursor.y)
                {
                    std::cout << "\033[105;31m" << view[i_y][i_x] << "\033[39;49m";
                }
                else {
                    std::cout << view[i_y][i_x];
                }
            }
            std::cout << std::endl;
        }
        std::cout << std::flush;
        cursor_restore();
    }

    void refresh_input(void) {
        const std::lock_guard<std::mutex> lock(mutex_cursor);
        cursor_move_input();
        cursor_erase_line(0);
    }
};

class World {
   private:
    point_t world_size = {WORLD_SIZE_Y, WORLD_SIZE_X};
    world_storage_t world = {{0}};
    View view;
    point_t view_size = view.get_size();
    point_t view_pos = {
        world_size.y / 2 - view_size.y / 2,
        world_size.x / 2 - view_size.x / 2,
    };
    uint16_t refresh_rate = REFRESH_RATE_WORLD_MS;
    point_t world_start_pos = point_t{1, 1};
    point_t world_size_life = {world_size.y - 2, world_size.x - 2};
    std::mutex mutex_world_update;

    bool flag_stop = false;

    std::vector<event_t> events = {};

    bool point_get_value(const point_t pos) { return world[pos.y][pos.x]; }
    void point_set_value(const point_t pos, bool value) { world[pos.y][pos.x] = value; }
    void point_toggle(const point_t pos) { point_set_value(pos, !point_get_value(pos)); }

    uint16_t point_count_neighbour(const point_t p) {
        return world[p.y - 1][p.x - 1] + world[p.y - 1][p.x] + world[p.y - 1][p.x + 1] +
               world[p.y][p.x - 1] + world[p.y][p.x + 1] + world[p.y + 1][p.x - 1] +
               world[p.y + 1][p.x] + world[p.y + 1][p.x + 1];
    }

    void events_process(void) {
        point_t p ={};

        for(const event_t e : events) {
            if (!(e.pos.x == 0 || e.pos.y ==0 || e.pos.x == world_size.x || e.pos.y == world_size.y)){
                point_set_value(e.pos, e.value);

                if (view_pos.y <= e.pos.y && e.pos.y < view_pos.y + view_size.y && view_pos.x <= e.pos.x &&
                    e.pos.x < view_pos.x + view_size.x) {
                    p = {
                        e.pos.y - view_pos.y,
                        e.pos.x - view_pos.x,
                    };
                    view_update(p, e.value);
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
                if (point_get_value(i) && (neighbour_count > 3 || neighbour_count < 2)) {
                    events.push_back(event_t{i, !point_get_value(i)});
                } else if (!point_get_value(i) && neighbour_count == 3) {
                    events.push_back(event_t{i, !point_get_value(i)});
                }
            }
        }
    }

    void update_world(void) {
        const std::lock_guard<std::mutex> lock(mutex_world_update);
        points_health_check();
        events_process();
    }

    void view_update(const point_t pos, bool value) {
        view.update(pos, (value) ? CELL_ALIVE : CELL_DEAD);
    }

   public:
    void refresh_input(void) { view.refresh_input(); }

    point_t view_pos_normalize_max(const point_t pos) {
        return {
            (pos.y + view_size.y > world_size.y) ? world_size.y - view_size.y + 1 : pos.y,
            (pos.x + view_size.x > world_size.x) ? world_size.x - view_size.x + 1 : pos.x,
        };
    }

    void view_move(const point_t pos) {
        point_t i = {};
        view_pos = view_pos_normalize_max(pos);

        for (i.y = view_pos.y; i.y < view_pos.y + view_size.y; i.y++) {
            for (i.x = view_pos.x; i.x < view_pos.x + view_size.x; i.x++) {
                if (i.y == 0 || i.y == world_size.y || i.x == 0 || i.x == world_size.x) {
                    view.update(point_t{i.y - view_pos.y, i.x - view_pos.x}, CELL_BORDER);
                } else {
                    view_update(point_t{i.y - view_pos.y, i.x - view_pos.x}, point_get_value(i));
                }
            }
        }

        view.refresh();
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

    void infest_random(const point_t pos, const point_t size, uint32_t cells) {
        const std::lock_guard<std::mutex> lock(mutex_world_update);
        uint32_t i;
        point_t p;

        cells = (1000000 < cells) ? 1000000: cells;

        for (i = 0; i < cells; i++) {
            p = {
                rand() % size.y + pos.y,
                rand() % size.x + pos.x,
            };
            point_set_value(p, true);
        }
        
    }

    void infest_random_view(uint32_t cells) {
        infest_random(view_pos, view_size, (cells == 0) ? 1000 : cells);
    }
    void infest_random_world(uint32_t cells) {
        infest_random(world_start_pos, world_size_life, (cells == 0) ? 1000000 : cells);
    }

    void clear(const point_t pos, const point_t size) {
        const std::lock_guard<std::mutex> lock(mutex_world_update);
        point_t i = {};
        point_t p = view_pos_normalize_max(pos);

        events.clear();

        for (i.y = p.y; i.y < p.y + size.y ; i.y++) {
            for (i.x = p.x; i.x < p.x + size.x; i.x++) {
                events.push_back(event_t{i, false});
            }
        }

        events_process();
    }

    void clear_world(void) { clear(world_start_pos, world_size_life); }
    void clear_view(void) { clear(view_pos, view_size);}

    void set_cursor_pos(point_t pos) {
        view.set_user_cursor(pos);
    }

    point_t get_cursor_pos(void) {
        return view.get_user_cursor();
    }

    void toggle_cursor(void){
        const std::lock_guard<std::mutex> lock(mutex_world_update);
        const point_t p = {
            view_pos.y + get_cursor_pos().y,
            view_pos.x + get_cursor_pos().x,
        };
        events.push_back(event_t{p, !point_get_value(p)});
        events_process();
        view.refresh();
    }

    void run(void) {
        view_move(view_pos);
        view.set_user_cursor({view_size.y /2, view_size.x / 2});

        while (!flag_stop) {
            std::this_thread::sleep_for(std::chrono::milliseconds(refresh_rate));
            update_world();
            view.refresh();
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
        if (number * 10 + static_cast<uint16_t>(c & ANSCII_NUMBER_MASK) > number){
            number = number * 10 + static_cast<uint16_t>(c & ANSCII_NUMBER_MASK);
        }
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


void cb_input_move_view(World &world) {
    point_t p = {};
    uint32_t *arr[3] = {&p.y, &p.x, NULL};
    char c;
    std::map<char, callback_move_view_half> map_callback{
            {'l', &World::view_move_half_left},
            {'r', &World::view_move_half_right},
            {'t', &World::view_move_half_top},
            {'b', &World::view_move_half_bottom},
        };

    if (anscii_is_int(std::cin.peek())) {
        input_get_int(arr);
        world.view_move(p);
    } else {
        c = std::cin.get();

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
    point_t p = world.get_cursor_pos();
    if (p.y > 0) {p.y--;};
    world.set_cursor_pos(p);
}

void cb_input_cursor_s(World &world) {
    point_t p = world.get_cursor_pos();
    p.y++;
    world.set_cursor_pos(p);
}

void cb_input_cursor_a(World &world) {
    point_t p = world.get_cursor_pos();
    if (p.x > 0) {p.x--;};
    world.set_cursor_pos(p);
}

void cb_input_cursor_d(World &world) {
    point_t p = world.get_cursor_pos();
    p.x++;
    world.set_cursor_pos(p);
}

void cb_input_cursor_t(World &world) {
    world.toggle_cursor();
}

void loop_input(World &world) {
    char c;
    std::map<char, callback_input_t> map_callback{
        {'e', cb_input_stop},
        {'r', cb_input_refresh_rate},
        {'m', cb_input_move_view},
        {'c', cb_input_clear},
        {'i', cb_input_infest},
        {'w', cb_input_cursor_w},
        {'a', cb_input_cursor_a},
        {'s', cb_input_cursor_s},
        {'d', cb_input_cursor_d},
        {'t', cb_input_cursor_t},
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