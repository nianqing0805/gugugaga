#include <iostream>
#include "student.hpp"

int main(){
    Student stu1("Alice", "00261001", 85.5);

    stu1.printInfo();
    stu1.setScore(92.0);
    stu1.printInfo();
    return 0;
}