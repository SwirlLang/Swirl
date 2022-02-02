/* Contain functions and classes to allow LC to interact with Operating system.

Copyright (C) 2022 Lambda Code Organization

This file is part of the Lambda Code programming language

Lambda Code is free software: you can redistribute it and/or modify it under the terms of the
GNU General Public License as published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version.

Lambda Code is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program.
If not, see https://www.gnu.org/licenses/.
*/

#ifndef LAMBDA_CODE_MATH_H
#define LAMBDA_CODE_MATH_H

#include <math.h>

namespace math
{
    int abs(double _number);

    double sqrt(double _number);

    double cbrt(double _number);

    double log(double _number);

    double loge(double _number);

    double pow(double _base, double _exponent);

    double sin(double _rad);

    double cos(double _rad);

    double tan(double _rad);

    double sini(double _rad);

    double cosi(double _rad);

    double tani(double _rad);

    int round(double _number);

    double ceil(double _number);

    double floor(double _number);
}

#endif
