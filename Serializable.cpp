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