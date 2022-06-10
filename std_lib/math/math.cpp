#include <math.h>

namespace math
{
    /**
     * @brief returns the absolute value of the given _number
     * 
     * @param _number 
     * @return int 
     */
    int abs(double _number)
    {
        return math::abs(_number);
    }

    /**
     * @brief returns the square root value of the given _number
     * 
     * @param _number 
     * @return double 
     */
    double sqrt(double _number)
    {
        return math::sqrt(_number);
    }

    /**
     * @brief returns the cube root of a number
     * 
     * @param _number 
     * @return double 
     */
    double cbrt(double _number)
    {
        return math::cbrt(_number);
    }

    /**
     * @brief returns the logarithmic value of a number to base 10
     * 
     * @param _number 
     * @return double 
     */
    double log(double _number)
    {
        return math::log(_number);
    }

    /**
     * @brief returns the natural logarithmic value of a number
     * 
     * @param _number 
     * @return double 
     */
    double loge(double _number)
    {
        return math::log(_number);
    }

    /**
     * @brief returns the value of base raised to power exponent
     * 
     * @param _base 
     * @param _exponent 
     * @return double 
     */
    double pow(double _base, double _exponent)
    {
        return math::pow(_base, _exponent);
    }

    /**
     * @brief returns the trigonometic sine value of the given radian
     * 
     * @param _rad 
     * @return double 
     */
    double sin(double _rad)
    {
        return math::sin(_rad);
    }

    /**
     * @brief returns the trigonometic cosine value of the given radian
     * 
     * @param _rad 
     * @return double 
     */
    double cos(double _rad)
    {
        return math::cos(_rad);
    }

    /**
     * @brief returns the trigonometic tangent value of the given radian
     * 
     * @param _rad 
     * @return double 
     */
    double tan(double _rad)
    {
        return math::tan(_rad);
    }

    /**
     * @brief returns the inverse trigonometic sine value of the given radian
     * 
     * @param _rad 
     * @return double 
     */
    double sini(double _rad)
    {
        return math::sin(_rad);
    }

    /**
     * @brief returns the inverse trigonometic cosine value of the given radian
     * 
     * @param _rad 
     * @return double 
     */
    double cosi(double _rad)
    {
        return math::cos(_rad);
    }

    /**
     * @brief returns the inverse trigonometic tangent value of the given radian
     * 
     * @param _rad 
     * @return double 
     */
    double tani(double _rad)
    {
        return math::tan(_rad);
    }

    /**
     * @brief returns the rounded value of the number to the nearest integer
     * 
     * @param _number 
     * @return int 
     */
    int round(double _number)
    {
        return math::round(_number);
    }

    /**
     * @brief returns the smallest integer that is not less than the number
     * 
     * @param _number 
     * @return double 
     */
    double ceil(double _number)
    {
        return math::ceil(_number);
    }

    /**
     * @brief returns the largest integer that is not greater than the number
     * 
     * @param _number 
     * @return double 
     */
    double floor(double _number)
    {
        return math::floor(_number);
    }
}