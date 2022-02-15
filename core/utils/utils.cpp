#include <vector>
#include <iostream>
#include <string.h>
#include <sstream>

template<typename Any>
bool _checkMembership(Any iterable, Any value)
{
    for (Any iter : iterable)
    {
        if (iter == value)
        {
            return true;
        }
    }
    return false;
}
