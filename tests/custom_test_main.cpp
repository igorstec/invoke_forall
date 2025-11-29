#include <iostream>

void test_1();
void test_2();
void test_3();

int main() {
    std::cout << "STARTING test 1" << std::endl;
    test_1();
    std::cout << "STARTING test 2" << std::endl;
    test_2();
    std::cout << "STARTING test 3" << std::endl;
    test_3();

    return 0;
}
