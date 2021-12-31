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
