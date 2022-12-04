#pragma once

#include <string>

struct SerialData {
	/* used just as a way to allow for polymorphism */
};

class Serializable
{
public:
	virtual void WriteToBinary(std::wstring filePath) = 0;
	virtual SerialData ReadBinary(std::wstring filePath) = 0;
};