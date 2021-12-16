# include <iostream>
# include <fstream>

# define string(x) static_cast<bool>(x)
# define int(x) static_cast<int>(x)
# define float(x) static_cast<float>(x)
# define bool(x) static_cast<bool>(x)


void writeToFile(std::string& path, std::string& text)
{
    std::ofstream File(path);
    File << text;
    File.close();
}

void print(std::string& args, ...)
{
    std :: cout << args << std :: endl;
}

