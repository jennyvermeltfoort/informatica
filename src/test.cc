#include <chrono>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

typedef bool world_storage_t[1000][1000];
world_storage_t world;

int function_1(void) {
    uint8_t i = 0;

    for (uint16_t x = 1; x < 1000 - 1; x++) {
        for (uint16_t y = 1; y < 1000 - 1; y++) {
            if (world[x][y]) {
                i = world[x][y];
            }
        }
    }

    return i;
}

int function_2(void) {
    uint8_t i = 0;
    bool *ptr = &world[1][1];

    for (uint16_t x = 0; x < 1000 - 2; x++) {
        for (uint16_t y = 0; y < 1000 - 2; y++) {
            if (*ptr) {
                i = *ptr++;
            }
        }
        ptr += 2;
    }

    return i;
}

int function_3(void) {
    uint32_t i = 0;
    for (uint32_t x = 0; x < 1000 * 1000; x++) {
        i = world[1][1] + world[1][2] + world[1][3] + world[3][1] +
            world[3][2] + world[3][3] + world[2][1] + world[2][3];
    }
    return i;
}

int function_4(void) {
    uint32_t r = 0;
    bool *ptr = &world[1][1];
    for (uint32_t x = 0; x < 1000 * 1000; x++) {
        r = 0;
        bool *neighbour_top = ptr - 1000 - 1;
        bool *neighbour_bottom = ptr + 1000 - 1;
        for (uint8_t i = 0; i < 3; i++) {
            r += *neighbour_top++;
            r += *neighbour_bottom++;
        }
        r += *(ptr - 1);
        r += *(ptr + 1);
    }

    return r;
}

int function_5(void) {
    uint32_t r = 0;
    bool *ptr = &world[1][1];
    for (uint32_t x = 0; x < 1000 * 1000; x++) {
        r = *((char *)(ptr - 1000 - 1)) &
            0x7 + *((char *)(ptr + 1000 - 1)) &
            0x7 + *((char *)(ptr - 1)) & 0x5;
    }

    return r;
}

int main() {
    auto strt = std::chrono::high_resolution_clock::now();
    function_1();

    auto stp = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::microseconds>(stp -
                                                              strt);
    std::cout << std::endl
              << "Time taken by function: " << duration.count()
              << " microseconds" << std::endl;

    strt = std::chrono::high_resolution_clock::now();
    function_2();
    stp = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(
        stp - strt);
    std::cout << std::endl
              << "Time taken by function: " << duration.count()
              << " microseconds" << std::endl;

    strt = std::chrono::high_resolution_clock::now();
    function_3();
    stp = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(
        stp - strt);
    std::cout << std::endl
              << "Time taken by function: " << duration.count()
              << " microseconds" << std::endl;

    strt = std::chrono::high_resolution_clock::now();
    function_4();
    stp = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(
        stp - strt);
    std::cout << std::endl
              << "Time taken by function: " << duration.count()
              << " microseconds" << std::endl;

    strt = std::chrono::high_resolution_clock::now();
    function_5();
    stp = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(
        stp - strt);
    std::cout << std::endl
              << "Time taken by function: " << duration.count()
              << " microseconds" << std::endl;
}