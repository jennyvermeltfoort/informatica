

#include <iostream>
#include <thread>

const uint8_t VIEW_SIZE_Y = 20;
const uint8_t VIEW_SIZE_X = 100;
typedef char view_storage_t[VIEW_SIZE_Y][VIEW_SIZE_X];

class CliView {
   private:
    uint16_t size_y = VIEW_SIZE_Y;
    uint16_t size_x = VIEW_SIZE_X;
    view_storage_t arr = {};
    bool stop = false;

    void cursor_move(const uint16_t y, const uint16_t x) {
        std::cout << "\033[" << +y << ";" << +x << "H" << std::flush;
    }
    void cursor_move_start(void) { cursor_move(1, 1); }
    void cursor_move_end(void) { cursor_move(size_y, size_x); }
    void cursor_move_input(void) { cursor_move(size_y + 1, 1); }

    void cursor_erase_display(uint8_t n) { std::cout << "\033[" << +n << "J" << std::flush; }
    void cursor_erase_line(uint8_t n) { std::cout << "\033[" << +n << "K" << std::flush; }

    void cursor_save(void) { std::cout << "\033[s" << std::flush; }
    void cursor_restore(void) { std::cout << "\033[u" << std::flush; }

    void refresh(void) {
        uint8_t i_x;
        uint8_t i_y;

        cursor_save();
        cursor_move_end();
        cursor_erase_display(1);
        cursor_move_start();

        for (i_y = 0; i_y < size_y; i_y++) {
            for (i_x = 0; i_x < size_x; i_x++) {
                std::cout << arr[i_y][i_x];
            }
            std::cout << std::endl;
        }
        std::cout << std::flush;
        cursor_restore();
    }

   public:
    CliView() { ; }

    void update(const uint16_t pos_y, const uint16_t pos_x, const char c) { arr[pos_y][pos_x] = c; }
    char get(const uint16_t pos_y, const uint16_t pos_x) { return arr[pos_y][pos_x]; }

    void refresh_input(void) {
        cursor_move_input();
        cursor_erase_line(0);
    }

    void run(void) {
        cursor_erase_display(2);
        cursor_move_input();

        while (!stop) {
            refresh();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    void end(void) { stop = true; }
};

void loop_input(CliView &view) {
    char c;
    int b = 0;

    while (true) {
        std::cin.get(c);
        std::cout << "\r" << std::flush;
        view.refresh_input();
        if (c == 's') {
            view.end();
            break;
        }

        for (int y = 0; y < VIEW_SIZE_Y; y++) {
            for (int i = 0; i < VIEW_SIZE_X; i++) {
                view.update(y, i, view.get(y, i) + 1);
            }
        }
        b++;
    }
}

int main() {
    CliView view;

    for (int y = 0; y < VIEW_SIZE_Y; y++) {
        for (int i = 0; i < VIEW_SIZE_X; i++) {
            view.update(y, i, (i == 3) ? '.' : ',');
        }
    }

    std::thread thread_view(&CliView::run, &view);

    loop_input(view);

    thread_view.join();

    return 1;
}