#include "Serializable.h"

void Serializable::WriteString(std::ofstream& wStream, std::string str)
{
    //Approach #1
    int strSize = str.size() * sizeof(char);
    //wStream.write(reinterpret_cast<const char*>(&strSize), sizeof(int));
    //wStream.write(str.c_str(), strSize);

    //Approach #2
    char stackStringBuffer[512];
    wStream.write(reinterpret_cast<const char*>(&strSize), sizeof(int));
    memcpy(stackStringBuffer, str.c_str(), sizeof(str));
    wStream.write(reinterpret_cast<const char*>(stackStringBuffer), strSize);
}

std::string Serializable::ReadString(std::ifstream& rStream)
{
    int strSize = 0;
    rStream.read(reinterpret_cast<char*>(&strSize), sizeof(int));

    char* temp = new char[strSize + 1];
    rStream.read(temp, strSize);
    temp[strSize] = '\0';
    std::string returnStr(temp);
    delete[] temp;

    return returnStr;
}
