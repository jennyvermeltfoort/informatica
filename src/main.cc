/**
 * g++ (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0
 *
 * A application that simulates a cellular automaton, specifically
 * Conway's Game of Life. It allows users to interact with the
 * simulation through a command-line interface, providing various
 * options to control the simulation, view, and cell parameters.
 *
 * Date: 11-7-2024
 * Author: Jenny Vermeltfoort
 * Number: 3787494
 */

#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_map>

/**
 * @struct POINT_T
 * @brief Represents a point in a 2D space with x and y coordinates.
 */
typedef struct POINT_T {
    int16_t y;
    int16_t x;
} point_t;

/**
 * @brief Checks if a character is an ASCII integer.
 * @param c The character to check.
 * @return True if the character is an ASCII integer, false otherwise.
 */
inline bool ascii_is_int(uint8_t c) {
    const uint8_t ASCII_NUMBER_0 = 48;
    const uint8_t ASCII_NUMBER_9 = 57;
    return (c >= ASCII_NUMBER_0 && c <= ASCII_NUMBER_9);
}

/**
 * @brief Checks if a character is an ASCII whitespace.
 * @param c The character to check.
 * @return True if the character is an ASCII whitespace, false
 * otherwise.
 */
inline bool ascii_is_whitespace(uint8_t c) {
    return (c == ' ' || c == '\t');
}

/**
 * @brief Checks if a character is an ASCII symbol.
 * @param c The character to check.
 * @return True if the character is an ASCII symbol, false otherwise.
 */
inline bool ascii_is_symbol(uint8_t c) {
    const uint8_t ASCII_SYMBOL_START = 33;
    const uint8_t ASCII_SYMBOL_END = 126;
    return (c >= ASCII_SYMBOL_START && c <= ASCII_SYMBOL_END);
}

/**
 * @brief Generates a random number using a LCG (x_1 = (a * x_0 + 1) %
 * mod) with parameters a = 22695477, c = 1 and mod = 2147483648. The
 * output is a 14 bit integer, bits 16 .. 30 of x_1.
 * @return A 14-bit integer.
 */
int16_t random_gen() {
    static uint32_t num = std::time(NULL);
    num = (22695477 * num + 1) % 2147483648;
    return static_cast<int16_t>(num >> 16 & 0x7FFF);
}

/**
 * @brief Returns the minimum of two integers.
 * @param n1 The first integer.
 * @param n2 The second integer.
 * @return The minimum of the two integers.
 */
int16_t min(int16_t n1, int16_t n2) { return (n1 < n2) ? n1 : n2; }

/**
 * @brief Returns the maximum of two integers.
 * @param n1 The first integer.
 * @param n2 The second integer.
 * @return The maximum of the two integers.
 */
int16_t max(int16_t n1, int16_t n2) { return (n1 > n2) ? n1 : n2; }

/**
 * @brief Limits a number to be within a specified range.
 * @param n The number to limit.
 * @param upper The upper limit.
 * @param lower The lower limit.
 * @return The limited number.
 */
int16_t limit(int16_t n, int16_t upper, int16_t lower) {
    return max(min(n, upper), lower);
}

/**
 * @class View
 * @brief Represents a view of the world.
 */
class View {
   private:
    const std::string input_start_string = ">> ";
    point_t view_size;
    uint8_t *view_ptr;
    uint32_t view_ptr_line_distance;
    std::mutex mutex_cursor;
    point_t view_user_cursor;
    std::function<void(void)> cb_print_info;
    const uint8_t size_info_line = 3;
    int16_t line_input;
    int16_t line_info;
    bool flag_cb_print_info_set = false;

    /**
     * @brief Moves the cursor to a specified point.
     * @param point The point to move the cursor to.
     */
    void cursor_move(const point_t point) {
        std::cout << "\033[" << +point.y << ";" << +point.x << "H"
                  << std::flush;
    }

    /**
     * @brief Hides the cursor.
     */
    void cursor_hide(void) { std::cout << "\033[?25l" << std::flush; }

    /**
     * @brief Shows the cursor.
     */
    void cursor_show(void) { std::cout << "\033[?25h" << std::flush; }

    /**
     * @brief Moves the cursor to the start of the view.
     */
    void cursor_move_start(void) { cursor_move(point_t{0, 0}); }

    /**
     * @brief Moves the cursor to the end of the view.
     */
    void cursor_move_end(void) { cursor_move(view_size); }

    /**
     * @brief Moves the cursor to the input line.
     */
    void cursor_move_input(void) {
        point_t p = {line_input, 1};
        cursor_move(p);
    }

    /**
     * @brief Moves the cursor to the info line.
     */
    void cursor_move_info(void) {
        point_t p = {line_info, 1};
        cursor_move(p);
    }

    /**
     * @brief Erases the display from the cursor to the end of the
     * screen.
     * @param n The erase mode (0: to end of screen, 1: to beginning
     * of screen, 2: entire screen).
     */
    void cursor_erase_display(const uint8_t n) {
        std::cout << "\033[" << +n << "J" << std::flush;
    }

    /**
     * @brief Erases the line from the cursor to the end of the line.
     * @param n The erase mode (0: to end of line, 1: to beginning of
     * line, 2: entire line).
     */
    void cursor_erase_line(const uint8_t n) {
        std::cout << "\033[" << +n << "K" << std::flush;
    }

    /**
     * @brief Saves the current cursor position.
     */
    void cursor_save(void) { std::cout << "\033[s" << std::flush; }

    /**
     * @brief Restores the saved cursor position.
     */
    void cursor_restore(void) { std::cout << "\033[u" << std::flush; }

   public:
    /**
     * @brief Constructs a View object.
     * @param view Pointer to the view data.
     * @param size Size of the view.
     * @param distance Distance between lines in the view data.
     */
    View(uint8_t *view, point_t size, uint32_t distance) {
        set_view_parameters(view, size, distance);
        view_user_cursor = {static_cast<int16_t>(view_size.y + 1),
                            static_cast<int16_t>(view_size.x + 1)};
        line_input = view_size.y + size_info_line + 3;
        line_info = view_size.y + 2;

        cursor_erase_display(2);
        cursor_hide();
    }

    /**
     * @brief Resets the view.
     */
    void reset(void) { draw_frame(); }

    /**
     * @brief Sets the view parameters.
     * @param view Pointer to the view data.
     * @param size Size of the view.
     * @param distance Distance between lines in the view data.
     */
    void set_view_parameters(uint8_t *view, point_t size,
                             uint32_t distance) {
        view_ptr = view;
        view_size = size;
        view_ptr_line_distance =
            distance;  // the distance from view_ptr at line (y,x) to
                       // view_ptr at line (y+1,x)
    }

    /**
     * @brief Sets the print callback function.
     * @param cb The callback function.
     */
    void set_print_callback(std::function<void(void)> cb) {
        cb_print_info = cb;
        flag_cb_print_info_set = true;
    }

    /**
     * @brief Draws the frame around the view.
     */
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

    /**
     * @brief Refreshes the info line.
     */
    void refresh_info(void) {
        cursor_save();
        cursor_move_info();
        cursor_erase_line(0);
        if (flag_cb_print_info_set) {
            cb_print_info();
        }
        cursor_restore();
    }

    /**
     * @brief Refreshes the input line.
     */
    void refresh_input(void) {
        const std::lock_guard<std::mutex> lock(mutex_cursor);
        cursor_move_input();
        cursor_erase_line(0);
        std::cout << input_start_string << std::flush;
    }

    /**
     * @brief Gets the position of the user cursor.
     * @return The position of the user cursor.
     */
    point_t get_pos_cursor_user(void) { return view_user_cursor; }

    /**
     * @brief Sets the position of the user cursor.
     * @param pos The new position of the user cursor.
     */
    void set_user_cursor_pos(const point_t pos) {
        view_user_cursor = {
            limit(pos.y, view_size.y - 1, 0),
            limit(pos.x, view_size.x - 1, 0),
        };
    }

    /**
     * @brief Gets the size of the view.
     * @return The size of the view.
     */
    point_t get_size(void) { return view_size; }

    /**
     * @brief Refreshes the view.
     */
    void refresh_view(void) {
        const std::lock_guard<std::mutex> lock(mutex_cursor);
        uint8_t *ptr = view_ptr;
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

/**
 * @class World
 * @brief Represents the world of cells, including the view and user
 * interactions.
 */
class World {
   private:
    char cell_aliv = '&';
    char cell_dead = ' ';
    char cell_bord = '@';
    uint32_t infest_cell_view = 1000;
    uint32_t infest_cell_world = 1000000;
    uint16_t refresh_rate = 1000;

    /**
     * @struct WORLD_STORAGE_T
     * @brief The world is made up of cells. It consists of two
     * matrices, one which contains all the boolean values of all
     * cells, and one which contains all the char values of all cells.
     * The struct is packed to a singel byte, thus the distance
     * between the boolean value and the char value of a cell is the
     * size of one matrix (world_size.y * world_size.x).
     */
#pragma pack(1)
    typedef struct WORLD_STORAGE_T {
        uint8_t *alive;
        uint8_t *value;
    } world_storage_t;
#pragma pack()

    uint8_t *world_flat;
    world_storage_t *world;
    point_t world_size;

    uint8_t **events;
    uint8_t **event_current;

    point_t view_size;
    point_t view_step_size;
    point_t view_pos;
    View *view;
    uint32_t world_alive_counter = 0;

    bool flag_stop = false;
    uint32_t generations = 1;
    bool run_auto = 1;
    std::mutex mutex_world;

    /**
     * @brief Retrieves the value pointer that relates to the given
     * bool pointer.
     * @param ptr The bool pointer.
     * @return The value pointer.
     */
    uint8_t *world_storage_get_value_ptr(uint8_t *ptr) {
        return ptr + world_size.y * world_size.x * sizeof(bool);
    }

    /**
     * @brief Prints information about the world.
     */
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
                  << cell_aliv << "," << cell_dead << "," << cell_bord
                  << "'; ";
        std::cout << "Cells alive[world]: '" << +world_alive_counter
                  << "'; ";
        std::cout << std::endl;
        std::cout << "Default random infest cell count[view,world]: '"
                  << +infest_cell_view << "," << +infest_cell_world
                  << "'; ";
        std::cout << "Run mode, generations: '" << +run_auto << ","
                  << +generations << "'; ";
    }

    /**
     * @brief Sets the character for given cell property.
     * @param cell The cell to set.
     * @param c The character to set.
     */
    void set_cell_char(char &cell, const char c) {
        if (ascii_is_symbol(c) || c == ' ') {
            cell = c;
            world_init();  // reinitialze world to replace characters.
            view->refresh_view();
            view->refresh_info();
        }
    }

    /**
     * @brief Sets the position of the cursor.
     * @param pos The new position of the cursor.
     */
    void set_cursor_pos(point_t pos) {
        pos.y = limit(pos.y, view_size.y - 2, 1);
        pos.x = limit(pos.x, view_size.x - 2, 1);
        view->set_user_cursor_pos(pos);
        view->refresh_info();
        view->refresh_view();
    }

    /**
     * @brief Moves the view to a specified position.
     * @param p The new position of the view.
     */
    void view_move(const point_t p) {
        view_pos.y = limit(p.y, world_size.y - view_size.y, 0);
        view_pos.x = limit(p.x, world_size.x - view_size.x, 0);
        view->set_view_parameters(
            &world->value[view_pos.y * world_size.x + view_pos.x],
            view_size, world_size.x);
        set_cursor_pos(view->get_pos_cursor_user());
    }

    /**
     * @brief Sets the state of a cell.
     * @param cell_ptr Pointer to the cell.
     * @param alive The new state of the cell.
     */
    void world_set_cell(uint8_t *cell_ptr, const bool alive) {
        world_alive_counter += (alive) ? 1 : -1;
        *cell_ptr = alive;
        *world_storage_get_value_ptr(cell_ptr) =
            (alive) ? cell_aliv : cell_dead;
    }

    /**
     * @brief Initializes the world.
     */
    void world_init(void) {
        const std::lock_guard<std::mutex> lock(mutex_world);
        point_t i;
        uint8_t *cell_ptr = &world->alive[0];

        for (i.y = 0; i.y < world_size.y; i.y++) {
            for (i.x = 0; i.x < world_size.x; i.x++) {
                uint8_t *value_ptr =
                    world_storage_get_value_ptr(cell_ptr);
                if (i.y == 0 || i.x == 0 || i.y == world_size.y - 1 ||
                    i.x == world_size.x - 1) {
                    *value_ptr = cell_bord;
                } else if (!*cell_ptr) {
                    *value_ptr = cell_dead;
                } else {
                    *value_ptr = cell_aliv;
                }
                cell_ptr++;
            }
        }
    }

    /**
     * @brief Clears the events list.
     */
    inline void events_clear(void) { event_current = events; }

    /**
     * @brief Adds an event to the events list.
     * @param e The event to add.
     */
    inline void event_add(uint8_t *e) {
        event_current++;
        *event_current = e;
    }

    /**
     * @brief Processes the events in the events list.
     */
    void events_process(void) {
        const std::lock_guard<std::mutex> lock(mutex_world);
        while (event_current != events) {
            world_set_cell(*event_current, !**event_current);
            event_current--;
        };
    }

    /**
     * @brief Validates the cells in the world.
     */
    void world_validate_cells(void) {
        const std::lock_guard<std::mutex> lock(mutex_world);
        static const uint16_t limit_y = world_size.y - 2;
        static const uint16_t limit_x = world_size.x - 2;
        uint8_t *cell_ptr = &world->alive[1 * world_size.x + 1];
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

    /**
     * @brief Clears the world from a specified position to a
     * specified size.
     * @param pos The starting position.
     * @param size The size of the area to clear.
     */
    void world_clear(point_t pos, const point_t size) {
        const std::lock_guard<std::mutex> lock(mutex_world);
        uint16_t limit_y;
        uint16_t limit_x;
        point_t i = {};

        pos.y = limit(pos.y, world_size.y - view_size.y, 0);
        pos.x = limit(pos.x, world_size.x - view_size.x, 0);

        limit_y = pos.y + size.y;
        limit_x = pos.x + size.x;

        events_clear();

        for (i.y = pos.y; i.y < limit_y; i.y++) {
            uint8_t *cell_ptr =
                &world->alive[i.y * world_size.x + pos.x];
            for (i.x = pos.x; i.x < limit_x; i.x++) {
                if (*cell_ptr) {
                    world_set_cell(cell_ptr, false);
                }
                cell_ptr++;
            }
        }

        view->refresh_view();
    }

    /**
     * @brief Infests a specified area with random cells.
     * @param pos The starting position.
     * @param size The size of the area to infest.
     * @param cells The number of cells to infest.
     */
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
    /**
     * @brief Constructs a World object.
     * @param _world_size The size of the world.
     * @param _view_size The size of the view.
     */
    World(const point_t _world_size, const point_t _view_size)
        : world_size(_world_size), view_size(_view_size) {
        uint64_t world_flat_size = world_size.y * world_size.x * 2;
        uint64_t event_size = world_size.x * world_size.y;

        world_flat = new uint8_t[world_flat_size];
        world = new world_storage_t;

        world->alive = reinterpret_cast<uint8_t *>(&world_flat[0]);
        world->value = reinterpret_cast<uint8_t *>(
            &world_flat[world_size.y * world_size.x]);

        view_step_size = {view_size.y, view_size.x};
        view_pos = {
            static_cast<int16_t>(world_size.y / 2 - view_size.y / 2),
            static_cast<int16_t>(world_size.x / 2 - view_size.x / 2),
        };
        view = new View(
            &world->value[view_pos.y * world_size.x + view_pos.x],
            view_size, world_size.x - 1);
        view->set_print_callback(std::bind(&World::print_info, this));

        events = new uint8_t *[event_size];
        event_current = events;
    }

    /**
     * @brief Destructs the World object.
     */
    ~World() {
        delete[] world_flat;
        delete world;
        delete[] events;
        delete view;
    }

    /**
     * @brief Sets the number of generations.
     * @param v The number of generations.
     */
    void set_generations(uint32_t v) { generations = v; }

    /**
     * @brief Sets the character for an alive cell.
     * @param c The character for an alive cell.
     */
    void set_cell_aliv(const char c) { set_cell_char(cell_aliv, c); }

    /**
     * @brief Sets the character for a dead cell.
     * @param c The character for a dead cell.
     */
    void set_cell_dead(const char c) { set_cell_char(cell_dead, c); }

    /**
     * @brief Sets the character for a border cell.
     * @param c The character for a border cell.
     */
    void set_cell_bord(const char c) { set_cell_char(cell_bord, c); }

    /**
     * @brief Sets the number of cells to infest in the view.
     * @param v The number of cells to infest.
     */
    void set_infest_cell_view(const uint32_t v) {
        infest_cell_view = v;
    }

    /**
     * @brief Sets the number of cells to infest in the world.
     * @param v The number of cells to infest.
     */
    void set_infest_cell_world(const uint32_t v) {
        infest_cell_world = v;
    }

    /**
     * @brief Sets the view step size in the y direction.
     * @param y The view step size in the y direction.
     */
    void set_view_step_size_y(const uint32_t y) {
        view_step_size.y = y;
        view->refresh_info();
    }
    /**
     * @brief Sets the view step size in the x direction.
     * @param x The view step size in the x direction.
     */
    void set_view_step_size_x(const uint32_t x) {
        view_step_size.x = x;
        view->refresh_info();
    }

    /**
     * @brief Sets the refresh rate.
     * @param rate The refresh rate in milliseconds.
     */
    void set_refresh_rate(const uint16_t rate) {
        refresh_rate = limit(rate, 10000, 10);
        view->refresh_info();
    }

    /**
     * @brief Toggles the value of the cell highlighted by the cursor.
     */
    void toggle_cursor_value(void) {
        const std::lock_guard<std::mutex> lock(mutex_world);
        const point_t cursor = get_pos_cursor_world();
        world_set_cell(
            &world->alive[cursor.y * world_size.x + cursor.x],
            !world->alive[cursor.y * world_size.x + cursor.x]);
        view->refresh_view();
    }

    /**
     * @brief Toggles the run mode.
     */
    void toggle_run_mode(void) {
        generations = 1;
        run_auto = !run_auto;
    }

    /**
     * @brief Gets the position of the cursor in the view.
     * @return The position of the cursor in the view.
     */
    point_t get_pos_cursor_view(void) {
        return view->get_pos_cursor_user();
    }

    /**
     * @brief Gets the position of the cursor in the world.
     * @return The position of the cursor in the world.
     */
    point_t get_pos_cursor_world(void) {
        const point_t cursor = view->get_pos_cursor_user();
        return {
            static_cast<int16_t>(view_pos.y + cursor.y),
            static_cast<int16_t>(view_pos.x + cursor.x),
        };
    }

    /**
     * @brief Moves the cursor up.
     */
    void cursor_move_up() {
        set_cursor_pos(
            {static_cast<int16_t>(view->get_pos_cursor_user().y + 1),
             view->get_pos_cursor_user().x});
    }

    /**
     * @brief Moves the cursor down.
     */
    void cursor_move_down() {
        set_cursor_pos(
            {static_cast<int16_t>(view->get_pos_cursor_user().y - 1),
             view->get_pos_cursor_user().x});
    }

    /**
     * @brief Moves the cursor left.
     */
    void cursor_move_left() {
        set_cursor_pos({view->get_pos_cursor_user().y,
                        static_cast<int16_t>(
                            view->get_pos_cursor_user().x - 1)});
    }

    /**
     * @brief Moves the cursor right.
     */
    void cursor_move_right() {
        set_cursor_pos({view->get_pos_cursor_user().y,
                        static_cast<int16_t>(
                            view->get_pos_cursor_user().x + 1)});
    }

    /**
     * @brief Moves the view up.
     */
    void view_move_up(void) {
        view_move(
            {static_cast<int16_t>(view_pos.y - view_step_size.y),
             view_pos.x});
    }

    /**
     * @brief Moves the view down.
     */
    void view_move_down(void) {
        view_move(
            {static_cast<int16_t>(view_pos.y + view_step_size.y),
             view_pos.x});
    }

    /**
     * @brief Moves the view left.
     */
    void view_move_left(void) {
        view_move({view_pos.y, static_cast<int16_t>(
                                   view_pos.x - view_step_size.x)});
    }

    /**
     * @brief Moves the view right.
     */
    void view_move_right(void) {
        view_move({view_pos.y, static_cast<int16_t>(
                                   view_pos.x + view_step_size.x)});
    }

    /**
     * @brief Refreshes the input line.
     */
    void view_refresh_input(void) { view->refresh_input(); }

    /**
     * @brief Resets the view.
     */
    void view_reset(void) {
        view_move(view_pos);
        view->reset();
        view->draw_frame();
        view->refresh_info();
        view->refresh_view();
    }

    /**
     * @brief Sets the value of a cell in the world.
     * @param p The position of the cell.
     * @param value The value of the cell (0: dead, 1: alive).
     */
    void world_set_cell(point_t p, uint8_t value) {
        if (p.x < world_size.x && p.y < world_size.y) {
            world_set_cell(&world->alive[p.y * world_size.x + p.x],
                           value);
        }
    }

    /**
     * @brief Gets the value of a cell in the world.
     * @param p The position of the cell.
     * @return The value of the cell, or NULL if the position is out
     * of bounds.
     */
    const uint8_t *world_get_cell(point_t p) {
        if (p.x < world_size.x && p.y < world_size.y) {
            return &world->alive[p.y * world_size.x + p.x];
        }
        return NULL;
    }

    /**
     * @brief Infests the view with random cells.
     * @param cells The number of cells to infest. If 0, the default
     * value is used.
     */
    void infest_random_view(uint32_t cells) {
        infest_random(view_pos, view_size,
                      (cells == 0) ? infest_cell_view : cells);
    }

    /**
     * @brief Infests the world with random cells.
     * @param cells The number of cells to infest. If 0, the default
     * value is used.
     */
    void infest_random_world(uint32_t cells) {
        infest_random(point_t{1, 1},
                      {static_cast<int16_t>(world_size.x - 2),
                       static_cast<int16_t>(world_size.y - 2)},
                      (cells == 0) ? infest_cell_world : cells);
    }

    /**
     * @brief Clears the world.
     */
    void clear_world(void) {
        world_clear(point_t{1, 1},
                    {static_cast<int16_t>(world_size.x - 2),
                     static_cast<int16_t>(world_size.y - 2)});
    }

    /**
     * @brief Clears the view.
     */
    void clear_view(void) { world_clear(view_pos, view_size); }

    /**
     * @brief Runs the world simulation.
     */
    void run(void) {
        world_init();
        view->set_user_cursor_pos(
            {static_cast<int16_t>(view_size.y / 2),
             static_cast<int16_t>(view_size.x / 2)});
        view_reset();

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

    /**
     * @brief Stops the world simulation.
     */
    void stop(void) { flag_stop = true; }
};

/**
 * @brief Reads integers from standard input.
 * @param arr Pointer to an array to store the integers.
 */
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

/**
 * @brief Toggles the value of the cell highlighted by the cursor.
 * @param w The world object.
 */
void cbi_cursor_toggle(World &w) { w.toggle_cursor_value(); }

/**
 * @brief Moves the cursor up.
 * @param w The world object.
 */
void cbi_cursor_up(World &w) { w.cursor_move_up(); }

/**
 * @brief Moves the cursor down.
 * @param w The world object.
 */
void cbi_cursor_down(World &w) { w.cursor_move_down(); }

/**
 * @brief Moves the cursor left.
 * @param w The world object.
 */
void cbi_cursor_left(World &w) { w.cursor_move_left(); }

/**
 * @brief Moves the cursor right.
 * @param w The world object.
 */
void cbi_cursor_right(World &w) { w.cursor_move_right(); }

/**
 * @brief Moves the view left.
 * @param w The world object.
 */
void cbi_move_view_left(World &w) { w.view_move_left(); }

/**
 * @brief Moves the view right.
 * @param w The world object.
 */
void cbi_move_view_right(World &w) { w.view_move_right(); }

/**
 * @brief Moves the view up.
 * @param w The world object.
 */
void cbi_move_view_up(World &w) { w.view_move_up(); }

/**
 * @brief Moves the view down.
 * @param w The world object.
 */
void cbi_move_view_down(World &w) { w.view_move_down(); }

/**
 * @brief Stops the world simulation.
 * @param w The world object.
 */
void cbi_stop(World &w) { w.stop(); }

/**
 * @brief Resets the view.
 * @param w The world object.
 */
void cbi_reset_view(World &w) { w.view_reset(); }

/**
 * @brief Sets the number of generations.
 * @param world The world object.
 */
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
/**
 * @brief Clear submenu.
 * @param world The world object.
 */
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

/**
 * @brief Infests the world or view with random cells.
 * @param world The world object.
 */
void cbi_infest(World &world) {
    char c = std::cin.get();
    uint32_t cells = 0;
    std::unordered_map<char, std::function<void(World &, uint32_t)>>
        map_callback{
            {'v', &World::infest_random_view},
            {'w', &World::infest_random_world},
        };

    if (map_callback.find(c) != map_callback.end()) {
        uint32_t *arr[2] = {&cells, NULL};
        input_get_int(arr);
        map_callback[c](world, cells);
    }
}

/**
 * @brief Sets the refresh rate.
 * @param world The world object.
 */
void cbi_parameter_refresh_rate(World &world) {
    uint32_t refresh_rate = 0;
    uint32_t *arr[2] = {&refresh_rate, NULL};

    input_get_int(arr);

    if (refresh_rate > 0) {
        world.set_refresh_rate(refresh_rate);
    }
}

/**
 * @brief View submenu.
 * @param world The world object.
 */
void cbi_parameter_view(World &world) {
    char c = std::cin.get();
    uint32_t number = 0;
    std::unordered_map<char, std::function<void(World &, uint32_t)>>
        map_callback{
            {'y', &World::set_view_step_size_y},
            {'x', &World::set_view_step_size_x},
        };

    if (map_callback.find(c) != map_callback.end()) {
        uint32_t *arr[2] = {&number, NULL};
        input_get_int(arr);
        map_callback[c](world, number);
    }
}

/**
 * @brief Infest submenu.
 * @param world The world object.
 */
void cbi_parameter_infest(World &world) {
    char c = std::cin.get();
    uint32_t number = 0;
    std::unordered_map<char, std::function<void(World &, uint32_t)>>
        map_callback{
            {'v', &World::set_infest_cell_view},
            {'w', &World::set_infest_cell_world},
        };

    if (map_callback.find(c) != map_callback.end()) {
        uint32_t *arr[2] = {&number, NULL};
        input_get_int(arr);
        map_callback[c](world, number);
    }
}

/**
 * @brief Cell submenu.
 * @param world The world object.
 */
void cbi_parameter_cell(World &world) {
    char c = std::cin.get();
    char v = std::cin.get();
    std::unordered_map<char, std::function<void(World &, char)>>
        map_callback{
            {'a', &World::set_cell_aliv},
            {'d', &World::set_cell_dead},
            {'b', &World::set_cell_bord},
        };

    if (map_callback.find(c) != map_callback.end()) {
        map_callback[c](world, v);
    }
}

/**
 * @brief Parameter submenu.
 * @param world The world object.
 */
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

/**
 * @brief Prints the help message.
 * @param world The world object.
 */
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

/**
 * @brief Loads a glider gun pattern into the world from file.
 * @param world The world object.
 */
void cbi_glider_gun(World &world) {
    const point_t w = world.get_pos_cursor_world();
    std::fstream fs;
    point_t p = w;

    fs.open("glidergun.txt", std::fstream::in);
    if (!fs.is_open()) {
        std::cout << "Failed to open ./glidergun.txt, make sure "
                     "it exists! "
                  << std::endl;
        return;
    }

    do {
        const uint8_t *value = world.world_get_cell(p);
        char c = fs.get();
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

    world.view_reset();
}

/**
 * @brief Handles user input.
 * @param world The world object.
 */
void loop_input(World &world) {
    char c;
    std::unordered_map<char, std::function<void(World &)>>
        map_callback{
            {'h', cbi_print_help},      {'u', cbi_glider_gun},
            {'g', cbi_generations},     {'e', cbi_stop},
            {'r', cbi_reset_view},      {'c', cbi_clear},
            {'i', cbi_infest},          {'w', cbi_cursor_up},
            {'s', cbi_cursor_down},     {'a', cbi_cursor_left},
            {'d', cbi_cursor_right},    {'t', cbi_cursor_toggle},
            {'p', cbi_parameter},       {'8', cbi_move_view_up},
            {'6', cbi_move_view_right}, {'4', cbi_move_view_left},
            {'5', cbi_move_view_down},
        };

    while (true) {
        std::cin.get(c);
        world.view_refresh_input();
        if (map_callback.find(c) != map_callback.end()) {
            map_callback[c](world);
            if (c == 'e') {
                break;
            }
        }
    }
}

/**
 * @brief The main function.
 * @return 1 on success.
 */
int main() {
    const point_t WORLD_SIZE = {1000, 1000};
    const point_t VIEW_SIZE = {10, 100};
    World world(WORLD_SIZE, VIEW_SIZE);

    std::thread thread_world(&World::run, &world);
    loop_input(world);
    thread_world.join();

    return 1;
}