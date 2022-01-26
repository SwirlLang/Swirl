namespace __LambdaCode__ {
    class Complex {

    private:
        long long re, im;

    public:
        Complex();

        Complex(long long r, long long i);

        Complex add(Complex other);

        Complex sub(Complex other);

        Complex mul(Complex other);

        Complex div(Complex other);

        long long real();

        long long imaginary();
    };
}