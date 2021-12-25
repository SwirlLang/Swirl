# include <iostream>
# include <fstream>
# include <string>

# define len(x) sizeof(x)
# define print(x) stdOut(x);

using namespace std;

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

