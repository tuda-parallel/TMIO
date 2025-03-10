#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "test_utils.h"

TEST_CASE("Example test case") {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    print_vector(vec);
    REQUIRE(vec.size() == 5);
}