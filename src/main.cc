//
#define _TIMED 0

#if _TIMED == 1
#include <chrono>
#endif
#include <cstdlib>
#include <ctime>
#include <fstream>
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
const uint16_t DEFAULT_VIEW_SIZE_Y = 10;
const uint16_t DEFAULT_VIEW_SIZE_X = 100;

#pragma pack(1)
typedef struct CELL_T {
    bool alive;
    char value;
} cell_t;
#pragma pack()
typedef cell_t world_storage_t[WORLD_SIZE_Y][WORLD_SIZE_X];

typedef struct POINT_T {
    uint16_t y;
    uint16_t x;
} point_t;

inline bool ascii_is_int(uint8_t c) {
    const uint8_t ASCII_NUMBER_0 = 48;
    const uint8_t ASCII_NUMBER_9 = 57;
    return (c >= ASCII_NUMBER_0 && c <= ASCII_NUMBER_9);
}
inline bool ascii_is_whitespace(uint8_t c) {
    return (c == ' ' || c == '\t');
}

inline bool ascii_is_symbol(uint8_t c) {
    const uint8_t ASCII_SYMBOL_START = 33;
    const uint8_t ASCII_SYMBOL_END = 126;
    return (c >= ASCII_SYMBOL_START && c <= ASCII_SYMBOL_END);
}

typedef enum VIEW_MOVE_DIRECTION_T {
    VIEW_MOVE_UP,
    VIEW_MOVE_DOWN,
    VIEW_MOVE_LEFT,
    VIEW_MOVE_RIGHT,
} view_move_direction_t;

class View {
   private:
    const std::string input_start_string = ">> ";
    point_t view_size;
    cell_t *view_ptr;
    uint32_t view_ptr_line_distance;
    std::mutex mutex_cursor;
    point_t view_user_cursor;
    std::function<void(void)> cb_print_info;
    const uint8_t size_info_line = 3;
    uint16_t line_input;
    uint16_t line_info;
    bool flag_cb_print_info_set = false;

    void cursor_move(const point_t point) {
        std::cout << "\033[" << +point.y << ";" << +point.x << "H"
                  << std::flush;
    }
    void cursor_hide(void) { std::cout << "\033[?25l" << std::flush; }
    void cursor_show(void) { std::cout << "\033[?25h" << std::flush; }
    void cursor_move_start(void) { cursor_move(point_t{0, 0}); }
    void cursor_move_end(void) { cursor_move(view_size); }
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
    View(cell_t *view, point_t size, uint32_t distance) {
        set_view_parameters(view, size, distance);
        view_user_cursor = {static_cast<uint16_t>(view_size.y + 1),
                            static_cast<uint16_t>(view_size.x + 1)};
        line_input = view_size.y + size_info_line + 3;
        line_info = view_size.y + 2;

        cursor_erase_display(2);
        cursor_hide();
    }
    void reset(void) { draw_frame(); }
    void set_view_parameters(cell_t *view, point_t size,
                             uint32_t distance) {
        view_ptr = view;
        view_size = size;
        view_ptr_line_distance =
            distance;  // the distance from view_ptr at line (y,x) to
                       // view_ptr at line (y+1,x)
    }

    void set_print_callback(std::function<void(void)> cb) {
        cb_print_info = cb;
        flag_cb_print_info_set = true;
    }

    void draw_frame(void) {
        const std::lock_guard<std::mutex> lock(mutex_cursor);
        const char info_frame_character = '-';
        uint16_t i;

        cursor_erase_display(2);
        cursor_move(
            point_t{static_cast<uint16_t>(view_size.y + 1), 1});

        for (i = 0; i < view_size.x; i++) {
            std::cout << info_frame_character;
        }
        for (i = 0; i <= size_info_line; i++) {
            std::cout << std::endl;
        }
        for (i = 0; i < view_size.x; i++) {
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

    point_t get_pos_cursor_user(void) { return view_user_cursor; }

    void set_user_cursor_pos(const point_t pos) {
        view_user_cursor = {
            static_cast<uint16_t>(
                (pos.y > view_size.y - 1) ? view_size.y - 1 : pos.y),
            static_cast<uint16_t>(
                (pos.x > view_size.x - 1) ? view_size.x - 1 : pos.x),
        };
    }

    point_t get_size(void) { return view_size; }

    void refresh_view(void) {
        const std::lock_guard<std::mutex> lock(mutex_cursor);
        cell_t *ptr = view_ptr;
        uint16_t x;
        uint16_t y;

        cursor_save();
        cursor_move_end();
        cursor_erase_display(1);
        cursor_move_start();

        for (y = view_size.y; y > 0; y--) {
            for (x = view_size.x; x > 0; x--) {
                std::cout << ptr->value;
                ptr++;
            }
            std::cout << std::endl;
            ptr += view_ptr_line_distance - view_size.x;
        }

        cursor_move(point_t{
            static_cast<uint16_t>(view_user_cursor.y + 1),
            static_cast<uint16_t>(
                view_user_cursor.x +
                2)});  // terminal cursor starts at 1,1 and the next
                       // character is removed thus an additional x+1.
        std::cout << "\b\033[105;31m"
                  << (view_ptr +
                      view_ptr_line_distance * view_user_cursor.y +
                      view_user_cursor.x)
                         ->value
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
    point_t world_size_life = {
        WORLD_SIZE_Y - 2,
        WORLD_SIZE_X - 2};  // world has dead cells at borders.
    point_t world_start_pos = point_t{1, 1};

    cell_t **events = new cell_t
        *[WORLD_SIZE_X * WORLD_SIZE_Y];  // allocate to heap
                                         // instead of stack.
    cell_t **event_current = events;

    point_t view_size = {DEFAULT_VIEW_SIZE_Y, DEFAULT_VIEW_SIZE_X};
    point_t view_step_size = {view_size.y, view_size.x};
    point_t view_pos = {
        static_cast<uint16_t>(world_size.y / 2 - view_size.y / 2),
        static_cast<uint16_t>(world_size.x / 2 - view_size.x / 2),
    };
    View *view = new View(&world[view_pos.y][view_pos.x], view_size,
                          world_size.x - 1);
    uint16_t refresh_rate = DEFAULT_REFRESH_RATE;
    std::mutex mutex_world_update;
    uint32_t world_alive_counter = 0;
    uint32_t view_alive_counter = 0;
    bool flag_stop = false;
    point_t world_cursor_position = {
        static_cast<uint16_t>(view_pos.y + get_pos_cursor_view().y),
        static_cast<uint16_t>(view_pos.x + get_pos_cursor_view().x),
    };
    uint32_t generations = 1;
    bool run_auto = 1;

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
        std::cout << "Run mode, generations: '" << +run_auto << ","
                  << +generations << "'; ";
    }

    void point_set_value(cell_t *cell, const bool alive) {
        world_alive_counter += (alive) ? 1 : -1;
        cell->alive = alive;
        cell->value = (alive) ? cell_alive : cell_dead;
    }

    void set_cell_character(char &cell, const char c) {
        if (ascii_is_symbol(c) || c == ' ') {
            cell = c;
            view->refresh_view();
            view->refresh_info();
        }
    }

    point_t limit_pos_to_world_size(const point_t pos) {
        return {
            (pos.y + view_size.y > world_size.y)
                ? static_cast<uint16_t>(world_size.y - view_size.y)
                : pos.y,
            (pos.x + view_size.x > world_size.x)
                ? static_cast<uint16_t>(world_size.x - view_size.x)
                : pos.x,
        };
    }

    inline void events_clear(void) { event_current = events; }
    inline void event_add(cell_t *e) {
        event_current++;
        *event_current = e;
    }
    void events_process(void) {
        while (event_current != events) {
            point_set_value(*event_current, !(*event_current)->alive);
            event_current--;
        };
    }

    void points_health_check(void) {
        static const uint16_t limit_y = world_size.y - 2;
        static const uint16_t limit_x = world_size.x - 2;
        cell_t *world_ptr = &world[1][1];  // world starts at 1,1.
        uint8_t count;
        uint16_t x;
        uint16_t y;

        for (y = limit_y; y > 0; y--) {
            for (x = limit_x; x > 0; x--) {
                count = ((world_ptr - WORLD_SIZE_X - 1)->alive +
                         (world_ptr - WORLD_SIZE_X)->alive) +
                        ((world_ptr - WORLD_SIZE_X + 1)->alive +
                         (world_ptr - 1)->alive) +
                        ((world_ptr + 1)->alive +
                         (world_ptr + WORLD_SIZE_X - 1)->alive) +
                        ((world_ptr + WORLD_SIZE_X)->alive +
                         (world_ptr + WORLD_SIZE_X + 1)
                             ->alive);  // keep brackets to allow ???
                                        // todo name

                if ((world_ptr->alive && count != 2 && count != 3) ||
                    (!world_ptr->alive && count == 3)) {
                    event_add(world_ptr);
                }
                world_ptr++;
            }
            world_ptr += 2;  // skip border right on y and left on y+1
        }
    }

    void world_generate(void) {
        const std::lock_guard<std::mutex> lock(mutex_world_update);
        points_health_check();
        events_process();
    }

    void world_init(void) {
        point_t i;
        cell_t *world_ptr = &world[0][0];

        for (i.y = 0; i.y < world_size.y; i.y++) {
            for (i.x = 0; i.x < world_size.x; i.x++) {
                if (i.y == 0 || i.x == 0 || i.y == world_size.y - 1 ||
                    i.x == world_size.x - 1) {
                    world_ptr->value = cell_border;
                } else {
                    world_ptr->value = cell_dead;
                }
                world_ptr++;
            }
        }
    }

    void clear(const point_t pos, const point_t size) {
        const std::lock_guard<std::mutex> lock(mutex_world_update);
        point_t p = limit_pos_to_world_size(pos);
        uint16_t limit_y = p.y + size.y;
        uint16_t limit_x = p.x + size.x;
        point_t i = {};
        cell_t *world_ptr;

        events_clear();

        for (i.y = p.y; i.y < limit_y; i.y++) {
            world_ptr = &world[i.y][p.x];
            for (i.x = p.x; i.x < limit_x; i.x++) {
                if (world_ptr->alive) {
                    point_set_value(world_ptr, false);
                }
                world_ptr++;
            }
        }

        view->refresh_view();
    }

   public:
    World() {
        view->set_print_callback(std::bind(&World::print_info, this));
    }

    ~World() { delete events; }

    void view_refresh_input(void) { view->refresh_input(); }

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
        view->refresh_info();
    }
    void set_view_step_size_x(const uint32_t x) {
        view_step_size.x = x;
        view->refresh_info();
    }
    void set_refresh_rate(const uint16_t rate) {
        refresh_rate = rate;
        view->refresh_info();
    }
    void set_cursor_pos(const point_t pos) {
        view->set_user_cursor_pos(pos);
        world_cursor_position = {
            static_cast<uint16_t>(view_pos.y +
                                  get_pos_cursor_view().y),
            static_cast<uint16_t>(view_pos.x +
                                  get_pos_cursor_view().x),
        };
        view->refresh_info();
        view->refresh_view();
    }

    point_t get_pos_cursor_view(void) {
        return view->get_pos_cursor_user();
    }
    point_t get_pos_cursor_world(void) {
        return {
            static_cast<uint16_t>(view_pos.y +
                                  get_pos_cursor_view().y),
            static_cast<uint16_t>(view_pos.x +
                                  get_pos_cursor_view().x),
        };
    }

    void view_move(const point_t p) {
        view_pos = p;
        view->set_view_parameters(&world[view_pos.y][view_pos.x],
                                  view_size, world_size.x);
        view->refresh_view();
        set_cursor_pos(view->get_pos_cursor_user());
    }

    void view_move_vertical(bool up) {
        point_t p = view_pos;
        if (up && p.y < view_step_size.y) {
            p.y = 0;
        } else if (!up && p.y + view_step_size.y >= world_size.y) {
            p.y = static_cast<uint16_t>(world_size.y - view_size.y);
        } else {
            p.y += view_step_size.y * ((up == true) ? -1 : 1);
        }
        view_move(p);
    }

    void view_move_horizontal(bool left) {
        point_t p = view_pos;
        if (left && p.x < view_step_size.x) {
            p.x = 0;
        } else if (!left && p.x + view_step_size.x >= world_size.x) {
            p.x = static_cast<uint16_t>(world_size.x - view_size.x);
        } else {
            p.x += view_step_size.x * ((left == true) ? -1 : 1);
        }
        view_move(p);
    }

    void view_move_generic(view_move_direction_t direction) {
        switch (direction) {
            case (VIEW_MOVE_DOWN):
                view_move_vertical(0);
                break;
            case (VIEW_MOVE_UP):
                view_move_vertical(1);
                break;
            case (VIEW_MOVE_LEFT):
                view_move_horizontal(1);
                break;
            case (VIEW_MOVE_RIGHT):
                view_move_horizontal(0);
                break;
        }
    }

    bool point_in_world_life(point_t p) {
        return (p.x > 0 && p.x < world_size.x && p.y > 0 &&
                p.y < world_size.y);
    }

    void set_point_value(point_t p, bool value) {
        if (point_in_world_life(p)) {
            point_set_value(&world[p.y][p.x], value);
        }
    }

    bool get_point_value(point_t p, cell_t &value) {
        if (point_in_world_life(p)) {
            value = world[p.y][p.x];
            return true;
        }
        return false;
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
            point_set_value(&world[p.y][p.x], !world[p.y][p.x].alive);
        }

        view->refresh_view();
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
        point_set_value(
            &world[world_cursor_position.y][world_cursor_position.x],
            !world[world_cursor_position.y][world_cursor_position.x]
                 .alive);
        view->refresh_view();
    }

    void clear_world(void) {
        clear(world_start_pos, world_size_life);
    }
    void clear_view(void) { clear(view_pos, view_size); }

    void reset_view(void) {
        view->reset();
        view->draw_frame();
        view->refresh_info();
        view->refresh_view();
    }

    void toggle_run_mode(void) {
        generations = 1;
        run_auto = !run_auto;
    }

    void set_generations(uint32_t gens) { generations = gens; }

    void run(void) {
        world_init();
        view->set_user_cursor_pos(
            {static_cast<uint16_t>(view_size.y / 2),
             static_cast<uint16_t>(view_size.x / 2)});
        reset_view();

        while (!flag_stop) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(refresh_rate));

            if (generations > 0) {
                if (run_auto == false) {
                    generations--;
                }
                world_generate();
                view->refresh_view();
                view->refresh_info();
                view->refresh_input();
            }
        }
    }

    void stop(void) { flag_stop = true; }
};

void input_get_int(uint32_t **arr) {
    const uint8_t ASCII_NUMBER_MASK = 0XF;
    char c;
    uint32_t number = 0;

    if (*arr == NULL || std::cin.eof()) {
        return;
    }

    std::cin.get(c);

    while (ascii_is_int(c) && !std::cin.eof()) {
        if (number * 10 +
                static_cast<uint16_t>(c & ASCII_NUMBER_MASK) >
            number) {
            number = number * 10 +
                     static_cast<uint16_t>(c & ASCII_NUMBER_MASK);
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
void cb_input_move_view_left(World &world) {
    world.view_move_generic(VIEW_MOVE_LEFT);
}
void cb_input_move_view_right(World &world) {
    world.view_move_generic(VIEW_MOVE_RIGHT);
}
void cb_input_move_view_up(World &world) {
    world.view_move_generic(VIEW_MOVE_UP);
}
void cb_input_move_view_down(World &world) {
    world.view_move_generic(VIEW_MOVE_DOWN);
}

void cb_input_generations(World &world) {
    char c = std::cin.get();
    uint32_t generations = 0;
    uint32_t *arr[2] = {&generations, NULL};

    if (c == 'a') {
        world.toggle_run_mode();
    } else if (c == 's') {
        input_get_int(arr);
        world.set_generations(generations);
    }
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

void cb_input_print_help(World &world) {
    std::cout << "See the list below for all options, input is "
                 "parsed after each <enter>."
              << std::endl;
    std::cout << "Usage example: 'pca@\\n' sets the alive cell "
                 "representation to '@'."
              << std::endl;
    std::cout << "<h> \t\t\t\t this help." << std::endl;
    std::cout << "<e> \t\t\t\t stop the programm." << std::endl;
    std::cout << "<g><a/s> \t\t\t generations sub-menu." << std::endl;
    std::cout << "\t <a> \t\t\t toggle auto run mode." << std::endl;
    std::cout << "\t <s>[num] \t\t when in run_mode = '0', perform "
                 "[num] amount of generations."
              << std::endl;
    std::cout << "<r> \t\t\t\t reset the view." << std::endl;
    std::cout << "<m>[num1];[num2] \t\t move the view to coordinates "
                 "y = [num1], x = [num2], example: 'm10;10'."
              << std::endl;
    std::cout << "<8,6,4,5> \t\t\t move the view left(4), right(6), "
                 "top(8), bottom(5) by configured step size, see "
                 "<p><v>."
              << std::endl;
    std::cout << "<w,a,s,d> \t\t\t move the the cursor left(a), "
                 "right(d), top(w), bottom(s)."
              << std::endl;
    std::cout << "<t> \t\t\t\t toggle the cell highlighted by the "
                 "cursor (pink)."
              << std::endl;
    std::cout << "<i><v/v[num]/w/w[num]> \t\t infest sub-menu."
              << std::endl;
    std::cout << "\t <v> \t\t\t randomly infest the view with "
                 "default infest cell count, see <p><i><v>."
              << std::endl;
    std::cout << "\t <v>[num] \t\t randomly infest the view with "
                 "[num] amount of cells."
              << std::endl;
    std::cout << "\t <w> \t\t\t randomly infest the world with "
                 "default infest cell count, see <p><i><w>."
              << std::endl;
    std::cout << "\t <v>[num] \t\t randomly infest the world with "
                 "[num] amount of cells."
              << std::endl;
    std::cout << "<p><c/i/v/r> \t\t\t parameter sub-menu."
              << std::endl;
    std::cout << "\t <c><a/d/b> \t\t cell sub-menu." << std::endl;
    std::cout << "\t\t <a>[char] \t set alive cell representation to "
                 "[char], example: 'pca@'."
              << std::endl;
    std::cout << "\t\t <d>[char] \t set dead cell representation "
                 "to [char]."
              << std::endl;
    std::cout << "\t\t <b>[char] \t set border cell representation "
                 "to [char]."
              << std::endl;
    std::cout << "\t <i><v/w> \t\t infest sub-menu." << std::endl;
    std::cout << "\t\t <v>[num] \t\t set default infest cell count "
                 "to [num], view, see <i><v>."
              << std::endl;
    std::cout << "\t\t <w>[num] \t\t set default infest cell count "
                 "to [num], world, see <i><w>."
              << std::endl;
    std::cout << "\t <v><y/x> \t\t view sub-menu" << std::endl;
    std::cout << "\t\t <y>[num] \t set view move step size, y axis, "
                 "see <8,6,4,5>."
              << std::endl;
    std::cout << "\t\t <x>[num] \t set view move step size, x axis, "
                 "see <8,6,4,5>."
              << std::endl;
    std::cout << "\t <r>[num] \t\t set the refresh rate to [num] "
                 "milliseconds, example 'pr100'."
              << std::endl;
}

void cb_input_glider_gun(World &world) {
    std::fstream fs;
    char c;
    fs.open("glidergun.txt", std::fstream::in);
    point_t p = world.get_pos_cursor_world();
    cell_t value;

    if (!fs.is_open()) {
        std::cout << "Failed to open ./glidergun.txt, make sure "
                     "it exists! "
                  << std::endl;
        return;
    }

    do {
        c = fs.get();
        if (!world.get_point_value(p, value)) {
            break;
        }

        if (c == ' ' && value.alive == true) {
            world.set_point_value(p, false);
        } else if (c == 'x' && value.alive == false) {
            world.set_point_value(p, true);
        } else if (c == '\n') {
            p.y++;
            p.x = world.get_pos_cursor_world().x - 1;
        }
        p.x++;
    } while (!fs.eof());

    world.reset_view();
}

void loop_input(World &world) {
    char c;
    std::map<char, std::function<void(World &)>> map_callback{
        {'h', cb_input_print_help},
        {'u', cb_input_glider_gun},
        {'g', cb_input_generations},
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
        {'8', cb_input_move_view_up},
        {'6', cb_input_move_view_right},
        {'4', cb_input_move_view_left},
        {'5', cb_input_move_view_down},
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