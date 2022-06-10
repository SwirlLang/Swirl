#include <iostream>

class Complex {
private:
    long long re, im;

public:
    Complex(): re(0), im(0) {}
    Complex(long long r, long long i): re(r), im(i) {}

    Complex add(Complex other) {
        return Complex(re + other.re, im + other.im);
    }

    Complex sub(Complex other) {
        return Complex(re - other.re, im - other.im);
    }

    Complex mul(Complex other) {
        return Complex(re * other.re - im * other.im, re * other.im + im * other.re);
    }

    Complex div(Complex other) {
        long long tmp = other.re * other.re + other.im * other.im;
        return Complex((re * other.re + im * other.im) / tmp, (im * other.re - re * other.im) / tmp);
    }

    long long real() {
        return re;
    }

    long long imaginary() {
        return im;
    }
};