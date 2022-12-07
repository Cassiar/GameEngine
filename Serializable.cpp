#include "Serializable.h"

// Returns the compiled together string
void Serializable::Write(TypeKey writeType, std::string data) {
	m_numDataEntries++;

	m_readKey += std::to_string(writeType) + "\n";

	m_data += std::to_string(data.length()) + "\n";
	m_data += data + "\n";

	m_full = "";
	m_full += std::to_string(m_numDataEntries) + "\n";
	m_full += m_readKey;
	m_full += m_data;
}

std::string Serializable::GetFull() {
	return m_full;
}

std::string Serializable::SerializeSamplerDesc(D3D11_SAMPLER_DESC desc) {

	std::string serialized;

	//serialized += desc.Filter + "\n";
	//serialized += desc.AddressU + "\n";
	//serialized += desc.AddressV + "\n";
	//serialized += desc.AddressW + "\n";
	//serialized += std::to_string(desc.MipLODBias) + "\n";
	//serialized += std::to_string(desc.MaxAnisotropy) + "\n";
	//serialized += std::to_string(desc.BorderColor[0]) + "\n";
	//serialized += std::to_string(desc.BorderColor[1]) + "\n";
	//serialized += std::to_string(desc.BorderColor[2]) + "\n";
	//serialized += std::to_string(desc.BorderColor[3]) + "\n";
	//serialized += std::to_string(desc.MinLOD) + "\n";
	//serialized += std::to_string(desc.MaxAnisotropy);

	return serialized;
}

D3D11_SAMPLER_DESC DeSerializeSamplerDesc(std::string serialized) {
	D3D11_SAMPLER_DESC desc = {};
	int pos = (int)serialized.find("\n");
	//desc.Filter = static_cast<D3D11_FILTER>(serialized.substr(0, pos));

	return desc;
}