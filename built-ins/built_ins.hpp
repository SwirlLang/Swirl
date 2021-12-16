# include <iostream>
# include <fstream>

# define print(x) std::cout << x << endl;
# define str(x) (std::string) x
# define int(x) (int) x
# define float(x) (float) x
# define bool(x) (bool) x


void writeToFile(std::string path, std::string text)
{
    std::ofstream File(path);
    File << text;
    File.close();
}
