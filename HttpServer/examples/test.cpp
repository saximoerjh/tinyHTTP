#include <iostream>
#include "../include/utils/db/sqlConnectionPool.h"

int main() {
    std::cout << "Hello, World!" << std::endl;
    sqlConnectionPool& pool = sqlConnectionPool::getInstance();
    return 0;
}