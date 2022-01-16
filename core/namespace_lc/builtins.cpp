/* 
Copyright (C) 2022 Lambda Code Organization

This file is part of the Lambda Code programming language

Lambda Code is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Lambda Code is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. 
*/


# include <iostream>
# include <fstream>
# include <string>

# define len(x)      __LambdaCode__::sizeof(x);
# define print(x)    __LambdaCode__::stdOut(x);
# define string(x)   __LambdaCode__::__string(x);
# define int(x)      __LambdaCode__::__int(x);
# define float(x)    __LambdaCode::__double(x);


using namespace std;


namespace __LambdaCode__ 
{
    void writeToFile(std::string& path, std::string& text)
    {
        ofstream File(path);
        File << text;
        File.close();
    }

    template<typename p_type>
    void stdOut(p_type args[], string&  end = '\n')
    {
        std::cout << args << end;
    }


    string& input(std::string& prompt)
    {
        string* input;
        cout << prompt;
        getline(cin, *input);
        return *input;
    }

    template<typename T>
    std::string& __string(T var)
    {
        return (std::string)((const char*)var);
    }
}
