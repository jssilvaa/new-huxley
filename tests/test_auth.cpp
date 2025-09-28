// tests/test_database.cpp
#include "Database.h"
#include <iostream>

int main() {
    Database db("test.db");
    db.insertUser("bob", "1234");
    std::string pw;
    if (db.findUser("bob", pw)) {
        std::cout << "Found bob with pw=" << pw << "\n";
    } else {
        std::cout << "bob not found!\n";
    }
    return 0;
}
