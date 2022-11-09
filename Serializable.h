#pragma once

#include <d3d11.h>
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

class Serializable
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
	std::string SerializeSamplerDesc(D3D11_SAMPLER_DESC desc);
	D3D11_SAMPLER_DESC DeSerializeSamplerDesc(std::string serialized);
};