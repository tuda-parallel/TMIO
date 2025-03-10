#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <iostream>
#include <vector>
#include <algorithm>

// Add common utility functions for tests here

void print_vector(const std::vector<int>& vec) {
    for (int val : vec) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
}

#endif // TEST_UTILS_H