#include "Material.h"

Material::Material(DirectX::XMFLOAT4 in_color, float roughness, std::shared_ptr<SimpleVertexShader> in_vs, std::shared_ptr<SimplePixelShader> in_ps)
{
    colorTint = in_color;
    this->roughness = roughness;
    vs = in_vs;
    ps = in_ps;
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

    int temp = sizeof(data);
    int temp2 = sizeof(MaterialSerialData);

    wStream.close();
    if (!wStream.good()) {
        std::cout << "Error occurred at writing time!" << std::endl;
    }
}
MaterialSerialData Material::ReadBinary(std::wstring filePath) {
    std::ifstream rStream(filePath, std::ios::in | std::ios::binary);

    if (!rStream) {
        std::cout << "Cannot open file!" << std::endl;
    }

    int temp3 = sizeof(MaterialSerialData);
    MaterialSerialData data;
    rStream.read((char*)&data, sizeof(MaterialSerialData));
    rStream.close();
    if (!rStream.good()) {
        std::cout << "Error occurred at reading time!" << std::endl;
    }

    //data = *reinterpret_cast<MaterialSerialData*>(readBuff);

    //delete[] readBuff;
    return data;
}
