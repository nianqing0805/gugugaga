#ifndef STUDENT_HPP
#define STUDENT_HPP

#include <string>
#include <iostream>

class Student {
private:
    std::string name;
    std::string id;
    double score;

public:
    Student(std::string studentName, const std::string& studentId, double studentScore);
    void printInfo() const;//print students'information
    
    void setScore(double newScore);//change score

    double getScore() const;
};

#endif // STUDENT_HPP