#pragma once

#include <iostream>
#include <string>

enum TypeKey {
	Int,
	Float,
	String,
	WString,
	Vec3,
	Vec4,
	MatTex,
	Sampler
};

class StringSerializable
{
private:
	int m_numDataEntries = 0;
	std::string m_readKey;
	std::string m_data;
	std::string m_full;

public:
	virtual std::string SerializeToString() = 0;

	void ClearSerial() { m_readKey = ""; m_data = ""; m_numDataEntries = 0; }

	void Write(TypeKey writeType, std::string data);
	std::string GetFull();
};