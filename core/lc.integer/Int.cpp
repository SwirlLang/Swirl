#include <iostream>
#include <string>
#include <memory>

class Int {
private:
    int value;

public:
    Int(int value) {
        if (value > INT_MAX) {
            this->value = static_cast<long long>(this->value);
        }
    }

    Int(): value(0) {}

    void operator++() { this->value++; }
    void operator--() { this->value--; }
    void operator+=(int value) { this->value += value; }
    void operator-=(int value) { this->value -= value; }
    void operator*=(int value) { this->value *= value; }
    void operator/=(int value) { this->value /= value; }
    void operator%=(int value) { this->value %= value; }
    void operator^=(int value) { this->value ^= value; }
    void operator=(int value) { this->value = value; }

    int operator+(int value) {
        if (this->value + value > INT_MAX) {
            return static_cast<long long>(this->value) + value;
        }

        return this->value + value;
    }
    int operator-(int value) { return this->value - value; }
    int operator*(int value) { return this->value * value; }
    int operator/(int value) { return this->value / value; }
    int operator%(int value) { return this->value % value; }

    bool operator>(int value)  { return this->value > value; }
    bool operator<(int value)  { return this->value < value; }
    bool operator>=(int value) { return this->value >= value; }
    bool operator<=(int value) { return this->value <= value; }
    bool operator==(int value) { return this->value == value; }
    bool operator!=(int value) { return this->value != value; }

    int getValue() const { return this->value; }
    void setValue(int value) { this->value = value; }

    int square() { return this->value * this->value; }
    int cube() { return this->value * this->value * this->value; }
    int power(int value) { return this->value ^ value; }

    int sin()     { return std::sin(this->value); }
    int cos()     { return std::cos(this->value); }
    int tan()     { return std::tan(this->value); }

    int cot()     { return 1 / std::tan(this->value); }
    int sec()     { return 1 / std::cos(this->value); }
    int csc()     { return 1 / std::sin(this->value); }
    int cosec()   { return 1 / std::sin(this->value); }

    std::string toString() { std::to_string(this->value); }
    double toFloat() { return static_cast<double>(this->value); }
};
