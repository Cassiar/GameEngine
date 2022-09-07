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

void Material::SetVertexShader(std::shared_ptr<SimpleVertexShader> in_vs)
{
    vs = in_vs;
}

std::shared_ptr<SimplePixelShader> Material::GetPixelShader()
{
    return ps;
}

void Material::SetPixelShader(std::shared_ptr<SimplePixelShader> in_ps)
{
    ps = in_ps;
}

void Material::AddTextureSRV(std::string name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
{
    textureSRVs.insert({ name, srv });
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
