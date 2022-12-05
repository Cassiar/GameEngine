#pragma once

#include <string>
#include <fstream>

struct SerialData {
	/* used just as a way to allow for polymorphism */
};

class Serializable
{
public:
	virtual void WriteToBinary(std::wstring filePath) = 0;
	//virtual SerialData ReadBinary(std::wstring filePath) = 0;

	void WriteString(std::ofstream& wStream, std::string str);
	std::string ReadString(std::ifstream& rStream);
};