#include "unittest.hpp"

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    std::cout << "cppbase testing" << std::endl;

    return RUN_ALL_TESTS();
}
