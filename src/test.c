#include <stddef.h>
#include <stdio.h>

// Function to safely access array elements without branching
int safe_access(const int* array, size_t size, size_t index) {
    // Create a mask that ensures valid indices are clamped
    size_t mask = (index < size) ? index : size;
    return array[mask];
}

int main() {
    // Define an array with an extra "stub" element
    int array[6] = {1, 2, 3,
                    4, 5, 0};  // Last element (0) is the stub

    size_t size = 5;  // Actual size of the array without the stub

    // Test accesses
    for (size_t i = 0; i < 10; i++) {
        printf("Index %zu: %d\n", i, safe_access(array, size, i));
    }

    return 0;
}