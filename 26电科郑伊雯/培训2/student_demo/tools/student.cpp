#include "student.hpp"
Student::Student(std::string studentName, const std::string& studentId, double studentScore)
    : name(studentName), id(studentId),score(studentScore) {}
  
void Student::printInfo() const {
    std::cout << "---Student Info ---" << std::endl;
    std::cout << "Name:" << name <<std::endl;
    std::cout << "ID:" << id << std::endl;
    std::cout << "Score:" << score << std::endl;
    std::cout << "-------------------" << std::endl;
}

void Student::setScore(double newScore)
{
    score = newScore;
    std::cout << "Score updated to: " << score << std::endl;
}

double Student::getScore() const 
{
    return score;
}