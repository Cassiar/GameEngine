#include "Sky.h"
#include "Camera.h"
#include "DXCore.h" //for GetFullPathTo_Wide helper function

Sky::Sky(std::shared_ptr<Mesh> mesh, Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler,  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv, Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::shared_ptr<SimpleVertexShader> vs, std::shared_ptr<SimplePixelShader> ps)
{
	//create rasterizer state
	D3D11_RASTERIZER_DESC rsDesc = {};
	rsDesc.CullMode = D3D11_CULL_FRONT; //make sure only back is shown
	rsDesc.FillMode = D3D11_FILL_SOLID;
	device->CreateRasterizerState(&rsDesc, rasterizerState.GetAddressOf());

	//create depth stencil state
	D3D11_DEPTH_STENCIL_DESC depthDesc = {};
	depthDesc.DepthEnable = true;
	depthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL; //show pixel if depth is equal to or less
	device->CreateDepthStencilState(&depthDesc, depthState.GetAddressOf());

	this->context = context;
	this->mesh = mesh;

	//create simple shaders for the specific shaders
	this->vs = vs;//std::make_shared<SimpleVertexShader>(device, context, DXCore::GetFullPathTo_Wide(L"SkyVertexShader.cso").c_str());
	this->ps = ps;//std::make_shared<SimplePixelShader>(device, context, DXCore::GetFullPathTo_Wide(L"SkyPixelShader.cso").c_str());

	this->srv = srv;
	this->sampler = sampler;
}

Sky::~Sky(){}

void Sky::Draw(std::shared_ptr<Camera> camera)
{
	//change render state to draw inside
	context->RSSetState(rasterizerState.Get());
	context->OMSetDepthStencilState(depthState.Get(), 0);

	//prepare specific shaders
	vs->SetMatrix4x4("view", camera->GetViewMatrix());
	vs->SetMatrix4x4("proj", camera->GetProjectionMatrix());
	vs->CopyAllBufferData();
	vs->SetShader();
	
	ps->SetShaderResourceView("SkyBox", srv);
	ps->SetSamplerState("BasicSampler", sampler);
	ps->CopyAllBufferData();
	ps->SetShader();

	//draw the mesh
	mesh->Draw();

	//passing in 0 (nullptr) resets render states to default
	context->RSSetState(nullptr);
	context->OMSetDepthStencilState(nullptr, 0);
}
