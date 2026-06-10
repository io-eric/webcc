#include "framework.h"

int main()
{
    std::cout << "WebCC test suite\n================\n";
    return webcc_test::run_all();
}
