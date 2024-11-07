//
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_map>

typedef struct POINT_T {
    int16_t y;
    int16_t x;
} point_t;

typedef enum MOVE_DIRECTION_T {
    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT,
} move_direction_t;

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

/* Generates a random number using a LCG x_1 = ((a * x_0 + 1) % mod)
 * with parameters a = 22695477, c = 1 and mod = 2147483648. The
 * output is a 14 bit integer, bits 16 .. 30 from x_1.
 */
int16_t random_gen() {
    static uint32_t num = std::time(NULL);
    num = (22695477 * num + 1) % 2147483648;
    return static_cast<int16_t>(num >> 16 & 0x7FFF);
}

int16_t min(int16_t n1, int16_t n2) { return (n1 < n2) ? n1 : n2; }
int16_t max(int16_t n1, int16_t n2) { return (n1 > n2) ? n1 : n2; }
int16_t limit(int16_t n, int16_t upper, int16_t lower) {
    return max(min(n, upper), lower);
}

class View {
   private:
    const std::string input_start_string = ">> ";
    point_t view_size;
    char *view_ptr;
    uint32_t view_ptr_line_distance;
    std::mutex mutex_cursor;
    point_t view_user_cursor;
    std::function<void(void)> cb_print_info;
    const uint8_t size_info_line = 3;
    int16_t line_input;
    int16_t line_info;
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
    View(char *view, point_t size, uint32_t distance) {
        set_view_parameters(view, size, distance);
        view_user_cursor = {static_cast<int16_t>(view_size.y + 1),
                            static_cast<int16_t>(view_size.x + 1)};
        line_input = view_size.y + size_info_line + 3;
        line_info = view_size.y + 2;

        cursor_erase_display(2);
        cursor_hide();
    }
    void reset(void) { draw_frame(); }
    void set_view_parameters(char *view, point_t size,
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
            point_t{static_cast<int16_t>(view_size.y + 1), 1});

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
            limit(pos.y, view_size.y - 1, 0),
            limit(pos.x, view_size.x - 1, 0),
        };
    }

    point_t get_size(void) { return view_size; }

    void refresh_view(void) {
        const std::lock_guard<std::mutex> lock(mutex_cursor);
        char *ptr = view_ptr;
        uint16_t x;
        uint16_t y;

        cursor_save();
        cursor_move_end();
        cursor_erase_display(1);
        cursor_move_start();

        for (y = view_size.y; y > 0; y--) {
            for (x = view_size.x; x > 0; x--) {
                std::cout << *ptr;
                ptr++;
            }
            std::cout << std::endl;
            ptr += view_ptr_line_distance - view_size.x;
        }

        // Add cursor highlight.
        cursor_move(point_t{
            static_cast<int16_t>(view_user_cursor.y + 1),
            static_cast<int16_t>(
                view_user_cursor.x +
                2)});  // terminal cursor starts at 1,1 and the next
                       // character is removed thus an additional x+1.
        std::cout << "\b\033[105;31m"
                  << *(view_ptr +
                       view_ptr_line_distance * view_user_cursor.y +
                       view_user_cursor.x)

                  << "\033[39;49m";

        std::cout << std::flush;
        cursor_restore();
    }
};

class World {
   private:
    char cell_alive = '&';
    char cell_dead = ' ';
    char cell_border = '@';
    uint32_t default_infest_cell_view = 1000;
    uint32_t default_infest_cell_world = 1000000;
    uint16_t refresh_rate = 1000;

    /* The world is made up of cells. It consists of two matrices, one
     * which contains all the boolean values of all cells, and one
     * which contains all the char values of all cells. The struct is
     * packed to a singel byte, thus the distance between the boolean
     * value and the char value of a cell is the size of one matrix
     * (WORLD_SIZE_Y * WORLD_SIZE_X).*/
#pragma pack(1)
    typedef struct WORLD_STORAGE_T {
        bool *alive;
        char *value;
    } world_storage_t;
#pragma pack()

    uint16_t *world_flat;
    world_storage_t *world;
    point_t world_size;
    point_t world_size_life;

    bool **events;
    bool **event_current;

    point_t view_size;
    point_t view_step_size;
    point_t view_pos;
    View *view;
    uint32_t world_alive_counter = 0;

    bool flag_stop = false;
    uint32_t generations = 1;
    bool run_auto = 1;
    std::mutex mutex_world;

    /* Retrieves the value pointer that relates to the given bool
     * pointer.
     */
    char *world_storage_get_value_ptr(bool *ptr) {
        return reinterpret_cast<char *>(
            ptr + world_size.y * world_size.x * sizeof(bool));
    }

    void print_info(void) {
        const point_t cursor = get_pos_cursor_world();
        std::cout << "Cursor[y,x]: '" << +cursor.y << "," << +cursor.x
                  << "'; ";
        std::cout << "Step size[y,x]: '" << +view_step_size.y << ","
                  << +view_step_size.x << "'; ";
        std::cout << "Refresh rate: '" << +refresh_rate << "'s; ";
        std::cout << std::endl;
        std::cout << "Cell symbol[alive, dead, "
                     "border]: '"
                  << cell_alive << "," << cell_dead << ","
                  << cell_border << "'; ";
        std::cout << "Cells alive[world]: '" << +world_alive_counter
                  << "'; ";
        std::cout << std::endl;
        std::cout << "Default random infest cell count[view,world]: '"
                  << +default_infest_cell_view << ","
                  << +default_infest_cell_world << "'; ";
        std::cout << "Run mode, generations: '" << +run_auto << ","
                  << +generations << "'; ";
    }

    void set_cell_character(char &cell, const char c) {
        if (ascii_is_symbol(c) || c == ' ') {
            cell = c;
            world_init();  // reinitialze world to replace characters.
            view->refresh_view();
            view->refresh_info();
        }
    }

    void world_set_cell(bool *cell_ptr, const bool alive) {
        world_alive_counter += (alive) ? 1 : -1;
        *cell_ptr = alive;
        *world_storage_get_value_ptr(cell_ptr) =
            (alive) ? cell_alive : cell_dead;
    }

    void world_init(void) {
        const std::lock_guard<std::mutex> lock(mutex_world);
        point_t i;
        bool *cell_ptr = &world->alive[0];
        char *value_ptr;

        for (i.y = 0; i.y < world_size.y; i.y++) {
            for (i.x = 0; i.x < world_size.x; i.x++) {
                value_ptr = world_storage_get_value_ptr(cell_ptr);
                if (i.y == 0 || i.x == 0 || i.y == world_size.y - 1 ||
                    i.x == world_size.x - 1) {
                    *value_ptr = cell_border;
                } else if (!*cell_ptr) {
                    *value_ptr = cell_dead;
                } else {
                    *value_ptr = cell_alive;
                }
                cell_ptr++;
            }
        }
    }

    inline void events_clear(void) { event_current = events; }
    inline void event_add(bool *e) {
        event_current++;
        *event_current = e;
    }
    void events_process(void) {
        const std::lock_guard<std::mutex> lock(mutex_world);
        while (event_current != events) {
            world_set_cell(*event_current, !**event_current);
            event_current--;
        };
    }

    void world_validate_cells(void) {
        const std::lock_guard<std::mutex> lock(mutex_world);
        static const uint16_t limit_y = world_size.y - 2;
        static const uint16_t limit_x = world_size.x - 2;
        bool *cell_ptr = &world->alive[1 * world_size.x +
                                       1];  // world starts at 1,1.
        uint8_t count;
        uint16_t x;
        uint16_t y;

        for (y = limit_y; y > 0; y--) {
            for (x = limit_x; x > 0; x--) {
                count = (*(cell_ptr - world_size.x - 1) +
                         *(cell_ptr - world_size.x)) +
                        (*(cell_ptr - world_size.x + 1) +
                         *(cell_ptr - 1)) +
                        (*(cell_ptr + 1) +
                         *(cell_ptr + world_size.x - 1)) +
                        (*(cell_ptr + world_size.x) +
                         *(cell_ptr + world_size.x +
                           1));  // keep brackets to allow ???
                                 // todo name

                if ((*cell_ptr && count != 2 && count != 3) ||
                    (!*cell_ptr && count == 3)) {
                    event_add(cell_ptr);
                }
                cell_ptr++;
            }
            cell_ptr += 2;  // skip border right on y and left on y+1
        }
    }

    /* Clear the world from pos till size.*/
    void world_clear(point_t pos, const point_t size) {
        const std::lock_guard<std::mutex> lock(mutex_world);
        uint16_t limit_y;
        uint16_t limit_x;
        point_t i = {};
        bool *cell_ptr;

        pos.y = limit(pos.y, world_size.y - view_size.y, 0);
        pos.x = limit(pos.x, world_size.x - view_size.x, 0);

        limit_y = pos.y + size.y;
        limit_x = pos.x + size.x;

        events_clear();

        for (i.y = pos.y; i.y < limit_y; i.y++) {
            cell_ptr = &world->alive[i.y * world_size.x + pos.x];
            for (i.x = pos.x; i.x < limit_x; i.x++) {
                if (*cell_ptr) {
                    world_set_cell(cell_ptr, false);
                }
                cell_ptr++;
            }
        }

        view->refresh_view();
    }

    /* Infest the position + size with <cells> amount of random cells.
     * If the position exceeds boundaries the function is returned
     * without action taken. */
    void infest_random(const point_t pos, const point_t size,
                       const uint32_t cells) {
        const std::lock_guard<std::mutex> lock(mutex_world);
        uint32_t i;
        point_t p;

        if (pos.y + size.y > world_size.y ||
            pos.x + size.x > world_size.x) {
            return;
        }

        for (i = 0; i < cells; i++) {
            p = {
                static_cast<int16_t>(random_gen() % size.y + pos.y),
                static_cast<int16_t>(random_gen() % size.x + pos.x),
            };
            world_set_cell(&world->alive[p.y * world_size.x + p.x],
                           !world->alive[p.y * world_size.x + p.x]);
        }

        view->refresh_view();
    }

   public:
    World(const point_t size_world, const point_t size_view) {
        world_flat =
            new uint16_t[world_size.y * world_size.x * sizeof(bool) +
                         world_size.y * world_size.x * sizeof(char)];

        world_size = size_world;
        world_size_life = {static_cast<int16_t>(world_size.x - 2),
                           static_cast<int16_t>(world_size.y - 2)};
        world = new world_storage_t;

        world->alive = reinterpret_cast<bool *>(&world_flat[0]);
        world->value = reinterpret_cast<char *>(
            &world_flat[world_size.y * world_size.x]);

        view_size = size_view;
        view_step_size = {view_size.y, view_size.x};
        view_pos = {
            static_cast<int16_t>(world_size.y / 2 - view_size.y / 2),
            static_cast<int16_t>(world_size.x / 2 - view_size.x / 2),
        };

        view = new View(
            &world->value[view_pos.y * world_size.x + view_pos.x],
            view_size, world_size.x - 1);
        events = new bool
            *[world_size.x * world_size.y];  // allocate to heap
        // instead of stack.
        event_current = events;

        view->set_print_callback(std::bind(&World::print_info, this));
    }

    ~World() {
        delete[] world_flat;
        delete world;
        delete[] events;
        delete view;
    }

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
    void set_default_infest_cell_view(const uint32_t cells) {
        default_infest_cell_view = cells;
    }
    void set_default_infest_cell_world(const uint32_t cells) {
        default_infest_cell_world = cells;
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
    void set_cursor_pos(point_t pos) {
        pos.y = limit(pos.y, view_size.y - 2, 1);
        pos.x = limit(pos.x, view_size.x - 2, 1);
        view->set_user_cursor_pos(pos);
        view->refresh_info();
        view->refresh_view();
    }

    point_t get_pos_cursor_view(void) {
        return view->get_pos_cursor_user();
    }
    point_t get_pos_cursor_world(void) {
        const point_t cursor = view->get_pos_cursor_user();
        return {
            static_cast<int16_t>(view_pos.y + cursor.y),
            static_cast<int16_t>(view_pos.x + cursor.x),
        };
    }

    void view_move(const point_t p) {
        view_pos.y = limit(p.y, world_size.y - view_size.y, 0);
        view_pos.x = limit(p.x, world_size.x - view_size.x, 0);
        view->set_view_parameters(
            &world->value[view_pos.y * world_size.x + view_pos.x],
            view_size, world_size.x);
        set_cursor_pos(view->get_pos_cursor_user());
    }

    void view_move(move_direction_t direction) {
        point_t p = view_pos;
        switch (direction) {
            case (MOVE_DOWN):
                p.y += view_step_size.y;
                break;
            case (MOVE_UP):
                p.y -= view_step_size.y;
                break;
            case (MOVE_LEFT):
                p.x -= view_step_size.x;
                break;
            case (MOVE_RIGHT):
                p.x += view_step_size.x;
                break;
        }
        view_move(p);
    }

    /* Set cell value in the world at point.
        <value>: 0 (dead), 1 (alive).
        If <p> is outside of world boudaries no action is taken. */
    void world_set_cell(point_t p, bool value) {
        if (p.x < world_size.x && p.y < world_size.y) {
            world_set_cell(&world->alive[p.y * world_size.x + p.x],
                           value);
        }
    }

    /* Get cell in the world at point <p>.
     Returns NULL if <p> is ouside of world boundaries.*/
    const bool *world_get_cell(point_t p) {
        if (p.x < world_size.x && p.y < world_size.y) {
            return &world->alive[p.y * world_size.x + p.x];
        }
        return NULL;
    }

    /* Infest the view with <cells> amount of random cells. If cells
     * == 0 then the class variable default_infest_cell_view is
     * used. */
    void infest_random_view(uint32_t cells) {
        infest_random(
            view_pos, view_size,
            (cells == 0) ? default_infest_cell_view : cells);
    }

    /* Infest the world with <cells> amount of random cells. If cells
     * == 0 then the class variable default_infest_cell_world
     * is used. */
    void infest_random_world(uint32_t cells) {
        infest_random(
            point_t{1, 1}, world_size_life,
            (cells == 0) ? default_infest_cell_world : cells);
    }

    void clear_world(void) {
        world_clear(point_t{1, 1}, world_size_life);
    }
    void clear_view(void) { world_clear(view_pos, view_size); }

    void toggle_cursor_value(void) {
        const std::lock_guard<std::mutex> lock(mutex_world);
        const point_t cursor = get_pos_cursor_world();
        world_set_cell(
            &world->alive[cursor.y * world_size.x + cursor.x],
            !world->alive[cursor.y * world_size.x + cursor.x]);
        view->refresh_view();
    }

    void toggle_run_mode(void) {
        generations = 1;
        run_auto = !run_auto;
    }

    void set_generations(uint32_t value) { generations = value; }

    void reset_view(void) {
        view->reset();
        view->draw_frame();
        view->refresh_info();
        view->refresh_view();
    }

    void run(void) {
        world_init();
        view->set_user_cursor_pos(
            {static_cast<int16_t>(view_size.y / 2),
             static_cast<int16_t>(view_size.x / 2)});
        reset_view();

        while (!flag_stop) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(refresh_rate));

            if (generations > 0) {
                if (run_auto == false) {
                    generations--;
                }
                world_validate_cells();
                events_process();
                view->refresh_view();
                view->refresh_info();
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

void cursor_move(World &world, move_direction_t direction) {
    point_t p = world.get_pos_cursor_view();
    switch (direction) {
        case (MOVE_DOWN):
            p.y++;
            break;
        case (MOVE_UP):
            p.y--;
            break;
        case (MOVE_LEFT):
            p.x--;
            break;
        case (MOVE_RIGHT):
            p.x++;
            break;
    }
    world.set_cursor_pos(p);
}
void cbi_cursor_up(World &world) { cursor_move(world, MOVE_UP); }
void cbi_cursor_down(World &world) { cursor_move(world, MOVE_DOWN); }
void cbi_cursor_left(World &world) { cursor_move(world, MOVE_LEFT); }
void cbi_cursor_right(World &world) {
    cursor_move(world, MOVE_RIGHT);
}

void cbi_cursor_t(World &world) { world.toggle_cursor_value(); }
void cbi_move_view_left(World &world) { world.view_move(MOVE_LEFT); }
void cbi_move_view_right(World &world) {
    world.view_move(MOVE_RIGHT);
}
void cbi_move_view_up(World &world) { world.view_move(MOVE_UP); }
void cbi_move_view_down(World &world) { world.view_move(MOVE_DOWN); }

void cbi_generations(World &world) {
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

void cbi_stop(World &world) { world.stop(); }
void cbi_reset_view(World &world) { world.reset_view(); }

void cbi_clear(World &world) {
    char c = std::cin.get();
    std::unordered_map<char, std::function<void(World &)>>
        map_callback{
            {'v', &World::clear_view},
            {'w', &World::clear_world},
        };

    if (map_callback.find(c) != map_callback.end()) {
        map_callback[c](world);
    }
}

void cbi_infest(World &world) {
    char c = std::cin.get();
    uint32_t cells = 0;
    uint32_t *arr[2] = {&cells, NULL};
    std::unordered_map<char, std::function<void(World &, uint32_t)>>
        map_callback{
            {'v', &World::infest_random_view},
            {'w', &World::infest_random_world},
        };

    if (map_callback.find(c) != map_callback.end()) {
        input_get_int(arr);
        map_callback[c](world, cells);
    }
}

void cbi_move_view(World &world) {
    point_t p;
    uint32_t y = 0;
    uint32_t x = 0;
    uint32_t *arr[3] = {&y, &x, NULL};
    p.y = y;
    p.x = x;
    input_get_int(arr);
    world.view_move(p);
}

void cbi_parameter_refresh_rate(World &world) {
    uint32_t refresh_rate = 0;
    uint32_t *arr[2] = {&refresh_rate, NULL};

    input_get_int(arr);

    if (refresh_rate > 0) {
        world.set_refresh_rate(refresh_rate);
    }
}

void cbi_parameter_view(World &world) {
    char c = std::cin.get();
    uint32_t number = 0;
    uint32_t *arr[2] = {&number, NULL};
    std::unordered_map<char, std::function<void(World &, uint32_t)>>
        map_callback{
            {'y', &World::set_view_step_size_y},
            {'x', &World::set_view_step_size_x},
        };

    if (map_callback.find(c) != map_callback.end()) {
        input_get_int(arr);
        map_callback[c](world, number);
    }
}

void cbi_parameter_infest(World &world) {
    char c = std::cin.get();
    uint32_t number = 0;
    uint32_t *arr[2] = {&number, NULL};
    std::unordered_map<char, std::function<void(World &, uint32_t)>>
        map_callback{
            {'v', &World::set_default_infest_cell_view},
            {'w', &World::set_default_infest_cell_world},
        };

    if (map_callback.find(c) != map_callback.end()) {
        input_get_int(arr);
        map_callback[c](world, number);
    }
}

void cbi_parameter_cell(World &world) {
    char c = std::cin.get();
    char v = std::cin.get();
    std::unordered_map<char, std::function<void(World &, char)>>
        map_callback{
            {'a', &World::set_cell_alive},
            {'d', &World::set_cell_dead},
            {'b', &World::set_cell_border},
        };

    if (map_callback.find(c) != map_callback.end()) {
        map_callback[c](world, v);
    }
}

void cbi_parameter(World &world) {
    char c = std::cin.get();
    std::unordered_map<char, std::function<void(World &)>>
        map_callback{
            {'c', cbi_parameter_cell},
            {'i', cbi_parameter_infest},
            {'v', cbi_parameter_view},
            {'r', cbi_parameter_refresh_rate},
        };

    if (map_callback.find(c) != map_callback.end()) {
        map_callback[c](world);
    }
}

void cbi_print_help(World &world) {
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

void cbi_glider_gun(World &world) {
    const point_t w = world.get_pos_cursor_world();
    const bool *value;
    std::fstream fs;
    point_t p = w;
    char c;

    fs.open("glidergun.txt", std::fstream::in);
    if (!fs.is_open()) {
        std::cout << "Failed to open ./glidergun.txt, make sure "
                     "it exists! "
                  << std::endl;
        return;
    }

    do {
        c = fs.get();
        value = world.world_get_cell(p);
        if (value == NULL) {
            break;
        }

        if (c == ' ' && *value == true) {
            world.world_set_cell(p, false);
        } else if (c == 'x' && *value == false) {
            world.world_set_cell(p, true);
        } else if (c == '\n') {
            p.y++;
            p.x = w.x - 1;
        }
        p.x++;
    } while (!fs.eof());

    world.reset_view();
}

void loop_input(World &world) {
    char c;
    std::unordered_map<char, std::function<void(World &)>>
        map_callback{
            {'h', cbi_print_help},     {'u', cbi_glider_gun},
            {'g', cbi_generations},    {'e', cbi_stop},
            {'r', cbi_reset_view},     {'m', cbi_move_view},
            {'c', cbi_clear},          {'i', cbi_infest},
            {'w', cbi_cursor_up},      {'s', cbi_cursor_down},
            {'a', cbi_cursor_left},    {'d', cbi_cursor_right},
            {'t', cbi_cursor_t},       {'p', cbi_parameter},
            {'8', cbi_move_view_up},   {'6', cbi_move_view_right},
            {'4', cbi_move_view_left}, {'5', cbi_move_view_down},
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
    const point_t WORLD_SIZE = {1000, 1000};
    const point_t VIEW_SIZE = {10, 100};
    World world(WORLD_SIZE, VIEW_SIZE);

    std::thread thread_world(&World::run, &world);

    loop_input(world);
    thread_world.join();

    return 1;
}