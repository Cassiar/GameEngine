#include "Material.h"

Material::Material(DirectX::XMFLOAT4 in_color, float roughness, std::shared_ptr<SimpleVertexShader> in_vs, std::string vsfilename, std::string vsname, std::shared_ptr<SimplePixelShader> in_ps, std::string psfilename, std::string psname)
{
    colorTint = in_color;
    this->roughness = roughness;
    vs = in_vs;
    vsFileName = vsfilename;
    vsName = vsname;

    ps = in_ps;
    psFileName = psfilename;
    psName = psname;
}

Material::~Material()
{
}

DirectX::XMFLOAT4 Material::GetColorTint()
{
    return colorTint;
}

void Material::SetColorTint(DirectX::XMFLOAT4 in_color)
{
    colorTint = in_color;
}

float Material::GetRoughness() {
    return roughness;
}

void Material::SetRoughness(float amount) {
    roughness = amount;
}

std::shared_ptr<SimpleVertexShader> Material::GetVertexShader()
{
    return vs;
}

void Material::SetVertexShader(std::shared_ptr<SimpleVertexShader> in_vs, std::string filename, std::string name)
{
    vs = in_vs;
    vsFileName = filename;
    vsName = name;
}

std::shared_ptr<SimplePixelShader> Material::GetPixelShader()
{
    return ps;
}

void Material::SetPixelShader(std::shared_ptr<SimplePixelShader> in_ps, std::string filename, std::string name)
{
    ps = in_ps;
    psFileName = filename;
    psName = name;
}

void Material::AddTextureSRV(std::string name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv, std::string filename, SRVMaps srvType)
{
    textureSRVs.insert({ name, srv });
    textureFiles.insert({ name, filename });
    textureTypes.insert({ name, srvType });
}

void Material::AddSampler(std::string name, Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler)
{
    samplers.insert({ name, sampler });
}

void Material::PrepareMaterial()
{
    //bind textures
    for (auto& t : textureSRVs) {
        ps->SetShaderResourceView(t.first.c_str(), t.second);
    }

//    ps->SetShaderResourceView("SurfaceTexture", textureSRVs["SurfaceTexture"]);/*
  //  ps->SetShaderResourceView("NormalTexture", textureSRVs["NormalTexture"]);*/

    //bind samplers
    for (auto& s : samplers) {
        ps->SetSamplerState(s.first.c_str(), s.second);
    }
}

std::string Material::SerializeToString()
{
    ClearSerial();

    Write(Vec4, std::to_string(colorTint.x) + "\n" 
        + std::to_string(colorTint.y) + "\n" 
        + std::to_string(colorTint.z) + "\n" 
        + std::to_string(colorTint.w));

    Write(Float, std::to_string(roughness));

    for (auto& t : textureFiles) {
        Write(MatTex, t.first + "\n" + t.second);
    }

    //for (auto& s : samplers) {
    //    D3D11_SAMPLER_DESC desc;
    //    s.second.Get()->GetDesc(&desc);
    //
    //    Write(Sampler, s.first + "\n" + s.second.);
    //}

    return GetFull();
}

void Material::WriteToBinary(std::wstring filePath) {
    std::ofstream wStream(filePath, std::ios::out | std::ios::binary);
    if (!wStream) {
        std::cout << "Cannot open file!" << std::endl;
        return;
    }
    /*
#pragma region viaStruct
    MaterialSerialData data;
    data.colorTint = colorTint;
    data.roughness = roughness;
    int index = 0;
    for (auto kv : textureSRVs)
    {
        if (index == MATERIAL_MAX_SERIAL_SRVS)
            break;

        std::string key = kv.first;
        data.srvTypes[index] = textureTypes[key];
        data.srvNames[index] = "RoughnessTextur";//key;
        std::string file = textureFiles[kv.first];
        data.srvFileNames[index] = file;

        index++;
    }

    data.vsFileName = vsFileName; //AssetManager::GetInstance()->StringToWide(vsFileName);
    data.vsName = vsName;

    data.psFileName = psFileName; //AssetManager::GetInstance()->StringToWide(psFileName);
    data.psName = psName;

    index = 0;
    for (auto kv : samplers)
    {
        if (index == MATERIAL_MAX_SERIAL_SAMPLERS)
            break;

        data.samplerNames[index] = kv.first + '\0';
        index++;
    }

    wStream.write((char*)&data, sizeof(data));
#pragma endregion*/

#pragma region viaWrite
    wStream.write((char*)&colorTint, sizeof(DirectX::XMFLOAT4));
    wStream.write((char*)&roughness, sizeof(float));

    char stackStringBuffer[512];
    int index = 0;
    int numSRVs = textureSRVs.size();
    wStream.write(reinterpret_cast<const char*>(&numSRVs), sizeof(int));
    for (auto kv : textureSRVs)
    {
        if (index == MATERIAL_MAX_SERIAL_SRVS)
            break;

        std::string key = kv.first;
        wStream.write((char*)&textureTypes[key], sizeof(SRVMaps));

        WriteString(wStream, key);

        WriteString(wStream, textureFiles[kv.first]);

        index++;
    }

    WriteString(wStream, vsFileName);
    WriteString(wStream, vsName);
    
    WriteString(wStream, psFileName);
    WriteString(wStream, psName);

    int numSamplers = samplers.size();
    wStream.write(reinterpret_cast<const char*>(&numSamplers), sizeof(int));
    for (auto kv : samplers)
    {
        if (index == MATERIAL_MAX_SERIAL_SAMPLERS)
            break;

        WriteString(wStream, kv.first);
    }

    //wStream.write((char*)&data, sizeof(data));
#pragma endregion

    wStream.close();
    if (!wStream.good()) {
        std::cout << "Error occurred at writing time!" << std::endl;
    }
}

MaterialSerialData Material::ReadBinary(std::wstring filePath, MaterialSerialData& data) {
    std::ifstream rStream(filePath, std::ios::in | std::ios::binary);

    if (!rStream) {
        std::cout << "Cannot open file!" << std::endl;
    }

    /*
#pragma region viaStructRead
    //MaterialSerialData data;
    rStream.read((char*)&data, sizeof(MaterialSerialData));
#pragma endregion
*/

#pragma region viaRead
    rStream.read((char*)&data.colorTint, sizeof(DirectX::XMFLOAT4));
    rStream.read((char*)&data.roughness, sizeof(float));

    int numSrvs = 0;
    rStream.read(reinterpret_cast<char*>(&numSrvs), sizeof(int));
    for (int index = 0; index < numSrvs; index++)
    {
        if (index == MATERIAL_MAX_SERIAL_SRVS)
            break;

        rStream.read((char*)&data.srvTypes[index], sizeof(SRVMaps));

        data.srvNames[index] = ReadString(rStream);

        data.srvFileNames[index] = ReadString(rStream);
    }

    data.vsFileName = ReadString(rStream);
    data.vsName     = ReadString(rStream);

    data.psFileName = ReadString(rStream);
    data.psName     = ReadString(rStream);

    int numSamplers = 0;
    rStream.read(reinterpret_cast<char*>(&numSamplers), sizeof(int));
    for (int index = 0; index < numSamplers; index++)
    {
        if (index == MATERIAL_MAX_SERIAL_SAMPLERS)
            break;

        data.samplerNames[index] = ReadString(rStream);
    }
#pragma endregion

    
    rStream.close();
    if (!rStream.good()) {
        std::cout << "Error occurred at reading time!" << std::endl;
    }

    //data = *reinterpret_cast<MaterialSerialData*>(readBuff);

    //delete[] readBuff;
    return data;
}
