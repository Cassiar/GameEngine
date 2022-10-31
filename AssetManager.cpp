#include "AssetManager.h"

using namespace DirectX;

std::shared_ptr<AssetManager> AssetManager::s_instance;

void AssetManager::InitializeSingleton(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context)
{
	if (!s_instance.get()) {
		std::shared_ptr<AssetManager> newInstance(new AssetManager());
		s_instance = newInstance;
		s_instance->Init(device, context);
	}
}

std::shared_ptr<AssetManager> AssetManager::GetInstance()
{
	return s_instance;
}

void AssetManager::Init(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context)
{
	m_device = device;
	m_context = context;

	m_vertexShaders = std::unordered_map<std::string, std::shared_ptr<SimpleVertexShader>>();
	m_pixelShaders = std::unordered_map<std::string, std::shared_ptr<SimplePixelShader>>();

	// Some of the basic map lists that we already need
	m_srvMaps = std::unordered_map<SRVMaps, std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>>();

	m_srvMaps[SRVMaps::Albedo]		 = std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>();
	m_srvMaps[SRVMaps::Roughness]	 = std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>();
	m_srvMaps[SRVMaps::AO]			 = std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>();
	m_srvMaps[SRVMaps::Normal]		 = std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>();
	m_srvMaps[SRVMaps::Metalness]	 = std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>();
	
	m_srvMaps[SRVMaps::ToonAlbedo]	  = std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>();
	m_srvMaps[SRVMaps::ToonRoughness] = std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>();
	m_srvMaps[SRVMaps::ToonAO]		  = std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>();
	m_srvMaps[SRVMaps::ToonMetalness] = std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>();

	InitTextures();
	InitShaders();
}

void AssetManager::InitTextures()
{
	AddSRVToMap(Albedo,	LoadSRV(L"Medieval_Floor_Albedo.tif"));
	AddSRVToMap(Roughness, LoadSRV(L"Medieval_Floor_Roughness.tif"));
	AddSRVToMap(AO,		LoadSRV(L"Medieval_Floor_AO.tif"));
	AddSRVToMap(Normal,	LoadSRV(L"Medieval_Floor_Normal.tif"));
	AddSRVToMap(Metalness, LoadSRV(L"noMetal.png"));

	AddSRVToMap(Albedo,	LoadSRV(L"SciFi_Panel_Albedo.tif"));
	AddSRVToMap(Roughness, LoadSRV(L"SciFi_Panel_Roughness.tif"));
	AddSRVToMap(AO,		LoadSRV(L"SciFi_Panel_AO.tif"));
	AddSRVToMap(Normal,	LoadSRV(L"SciFi_Panel_Normal.tif"));
	AddSRVToMap(Metalness, LoadSRV(L"SciFi_Panel_Metalness.tif"));

	AddSRVToMap(Albedo,	LoadSRV(L"Brick_Wall_Albedo.tif"));
	AddSRVToMap(Roughness, LoadSRV(L"Brick_Wall_Roughness.tif"));
	AddSRVToMap(AO,		LoadSRV(L"Brick_Wall_AO.tif"));
	AddSRVToMap(Normal,	LoadSRV(L"Brick_Wall_Normal.tif"));
	AddSRVToMap(Metalness, LoadSRV(L"noMetal.png"));

	AddSRVToMap(Albedo,	LoadSRV(L"Bronze_Albedo.tif"));
	AddSRVToMap(Roughness, LoadSRV(L"Bronze_Roughness.tif"));
	AddSRVToMap(AO,		LoadSRV(L"Bronze_AO.tif"));
	AddSRVToMap(Normal,	LoadSRV(L"Bronze_Normal.tif"));
	AddSRVToMap(Metalness, LoadSRV(L"Bronze_Metallic.tif"));

	AddSRVToMap(ToonAlbedo,	LoadSRV(L"Tree_Albedo.tif"));
	AddSRVToMap(ToonRoughness, LoadSRV(L"noMetal.png"));
	AddSRVToMap(ToonAO,		LoadSRV(L"allMetal.png"));
	AddSRVToMap(ToonMetalness,	LoadSRV(L"noMetal.png"));

	AddSRVToMap(SampleTexture, LoadSRV(L"Ramp_Texture.png"));

	//load cube map
	AddSRVToMap(SkyBox, CreateCubemap(
							GetFullPathTo_Wide(L"../../Assets/Textures/Sky/planet_right.png").c_str(),
							GetFullPathTo_Wide(L"../../Assets/Textures/Sky/planet_left.png").c_str(),
							GetFullPathTo_Wide(L"../../Assets/Textures/Sky/planet_up.png").c_str(),
							GetFullPathTo_Wide(L"../../Assets/Textures/Sky/planet_down.png").c_str(),
							GetFullPathTo_Wide(L"../../Assets/Textures/Sky/planet_front.png").c_str(),
							GetFullPathTo_Wide(L"../../Assets/Textures/Sky/planet_back.png").c_str()));
}

// --------------------------------------------------------
// Loads shaders from compiled shader object (.cso) files
// and also created the Input Layout that describes our 
// vertex data to the rendering pipeline. 
// - Input Layout creation is done here because it must 
//    be verified against vertex shader byte code
// - We'll have that byte code already loaded below
// --------------------------------------------------------
void AssetManager::InitShaders()
{
	//using Chirs's simple shader
	m_vertexShaders["vertexShader"]				= MakeSimpleVertexShader(L"VertexShader.cso");
	m_vertexShaders["skyVertexShader"]			= MakeSimpleVertexShader(L"SkyVertexShader.cso");
	m_vertexShaders["shadowVertexShader"]		= MakeSimpleVertexShader(L"ShadowVertexShader.cso");
	m_vertexShaders["ppLightRaysVertexShader"]	= MakeSimpleVertexShader(L"PostProcessLightRaysVertexShader.cso");

	m_pixelShaders["pixelShader"]				= MakeSimplePixelShader(L"PixelShader.cso");
	m_pixelShaders["debugPixelShader"]			= MakeSimplePixelShader(L"DebugColorShader.cso");
	m_pixelShaders["skyPixelShader"]			= MakeSimplePixelShader(L"SkyPixelShader.cso");
	m_pixelShaders["shadowPixelShader"]			= MakeSimplePixelShader(L"ShadowPixelShader.cso");
	m_pixelShaders["toonPixelShader"]			= MakeSimplePixelShader(L"ToonPixelShader.cso");
	m_pixelShaders["ppLightRaysPixelShader"]	= MakeSimplePixelShader(L"PostProcessLightRaysPixelShader.cso");
	m_pixelShaders["a5PixelShader"]				= MakeSimplePixelShader(L"A5CustomPS.cso");
}

void AssetManager::InitMeshes()
{
	m_meshes.push_back(LoadMesh("cube.obj"));
	m_meshes.push_back(LoadMesh("cylinder.obj"));
	m_meshes.push_back(LoadMesh("helix.obj"));
	m_meshes.push_back(LoadMesh("sphere.obj"));
	m_meshes.push_back(LoadMesh("quad.obj"));

	m_toonMeshes.push_back(LoadMesh("Tree.obj"));
}


//helper function to create cube map from 6 textures.
//used with permission from Chris Cascioli
// --------------------------------------------------------
// Loads six individual textures (the six faces of a cube map), then
// creates a blank cube map and copies each of the six textures to
// another face. Afterwards, creates a shader resource view for
// the cube map and cleans up all of the temporary resources.
// --------------------------------------------------------
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> AssetManager::CreateCubemap(
	const wchar_t* right,
	const wchar_t* left,
	const wchar_t* up,
	const wchar_t* down,
	const wchar_t* front,
	const wchar_t* back)
{
	// Load the 6 textures into an array.
	// - We need references to the TEXTURES, not the SHADER RESOURCE VIEWS!
	// - Specifically NOT generating mipmaps, as we usually don't need them for the sky!
	// - Order matters here! +X, -X, +Y, -Y, +Z, -Z
	ID3D11Texture2D* textures[6] = {};
	CreateWICTextureFromFile(m_device.Get(), right, (ID3D11Resource**)&textures[0], 0);
	CreateWICTextureFromFile(m_device.Get(), left, (ID3D11Resource**)&textures[1], 0);
	CreateWICTextureFromFile(m_device.Get(), up, (ID3D11Resource**)&textures[2], 0);
	CreateWICTextureFromFile(m_device.Get(), down, (ID3D11Resource**)&textures[3], 0);
	CreateWICTextureFromFile(m_device.Get(), front, (ID3D11Resource**)&textures[4], 0);
	CreateWICTextureFromFile(m_device.Get(), back, (ID3D11Resource**)&textures[5], 0);

	// We'll assume all of the textures are the same color format and resolution,
	// so get the description of the first shader resource view
	D3D11_TEXTURE2D_DESC faceDesc = {};
	textures[0]->GetDesc(&faceDesc);

	// Describe the resource for the cube map, which is simply
	// a "texture 2d array". This is a special GPU resource format,
	// NOT just a C++ array of textures!!!
	D3D11_TEXTURE2D_DESC cubeDesc = {};
	cubeDesc.ArraySize = 6; // Cube map!
	cubeDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE; // We'll be using as a texture in a shader
	cubeDesc.CPUAccessFlags = 0; // No read back
	cubeDesc.Format = faceDesc.Format; // Match the loaded texture's color format
	cubeDesc.Width = faceDesc.Width; // Match the size
	cubeDesc.Height = faceDesc.Height; // Match the size
	cubeDesc.MipLevels = 1; // Only need 1
	cubeDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE; // A CUBE MAP, not 6 separate textures
	cubeDesc.Usage = D3D11_USAGE_DEFAULT; // Standard usage
	cubeDesc.SampleDesc.Count = 1;
	cubeDesc.SampleDesc.Quality = 0;

	// Create the actual texture resource
	ID3D11Texture2D* cubeMapTexture = 0;
	m_device->CreateTexture2D(&cubeDesc, 0, &cubeMapTexture);

	// Loop through the individual face textures and copy them,
	// one at a time, to the cube map texure
	for (int i = 0; i < 6; i++)
	{
		// Calculate the subresource position to copy into
		unsigned int subresource = D3D11CalcSubresource(
			0, // Which mip (zero, since there's only one)
			i, // Which array element?
			1); // How many mip levels are in the texture?

		// Copy from one resource (texture) to another
		if (cubeMapTexture && textures[i]) {
			m_context->CopySubresourceRegion(
				cubeMapTexture, // Destination resource
				subresource, // Dest subresource index (one of the array elements)
				0, 0, 0, // XYZ location of copy
				textures[i], // Source resource
				0, // Source subresource index (we're assuming there's only one)
				0); // Source subresource "box" of data to copy (zero means the whole thing)
		}
	}

	// At this point, all of the faces have been copied into the
	// cube map texture, so we can describe a shader resource view for it
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = cubeDesc.Format; // Same format as texture
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE; // Treat this as a cube!
	srvDesc.TextureCube.MipLevels = 1; // Only need access to 1 mip
	srvDesc.TextureCube.MostDetailedMip = 0; // Index of the first mip we want to see

	// Make the SRV
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cubeSRV;
	if (cubeMapTexture) {
		m_device->CreateShaderResourceView(cubeMapTexture, &srvDesc, cubeSRV.GetAddressOf());
	}

	// Now that we're done, clean up the stuff we don't need anymore
	cubeMapTexture->Release(); // Done with this particular reference (the SRV has another)
	for (int i = 0; i < 6; i++)
		textures[i]->Release();
	// Send back the SRV, which is what we need for our shaders
	return cubeSRV;
}



Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> AssetManager::LoadSRV(std::wstring texturePath, bool customLocation /*Default = false*/)
{
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture;
	LoadSRV(texturePath, texture);

	return texture;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> AssetManager::LoadSRV(std::wstring texturePath, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureDestination, bool customLocation /*Default = false*/)
{
	std::wstring path = (customLocation ? L"" : TEXTURE_FOLDER);
	path += texturePath;

	CreateWICTextureFromFile(m_device.Get(), m_context.Get(),
		GetFullPathTo_Wide(path).c_str(), nullptr, textureDestination.GetAddressOf());

	return textureDestination;
}

std::shared_ptr<Mesh> AssetManager::LoadMesh(std::string meshPath, bool customLocation /*Default = false*/)
{
	std::string path = (customLocation ? "" : MODEL_FOLDER);
	path += meshPath;
	return std::make_shared<Mesh>(GetFullPathTo(meshPath).c_str(), m_device, m_context);
}

void AssetManager::AddSRVToMap(SRVMaps mapTypeName, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvToAdd)
{
	m_srvMaps[mapTypeName].push_back(srvToAdd);
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> AssetManager::GetSRV(SRVMaps map, int srvIndex)
{
	return m_srvMaps[map][srvIndex];
}

std::shared_ptr<SimpleVertexShader> AssetManager::MakeSimpleVertexShader(std::wstring csoName)
{
	return std::make_shared<SimpleVertexShader>(m_device, m_context, GetFullPathTo_Wide(csoName).c_str());
}

std::shared_ptr<SimplePixelShader> AssetManager::MakeSimplePixelShader(std::wstring csoName)
{
	return std::make_shared<SimplePixelShader>(m_device, m_context, GetFullPathTo_Wide(csoName).c_str());
}



///------------------ Written by Chris Cascioli ------------------------------///

// --------------------------------------------------------------------------
// Gets the actual path to this executable
//
// - As it turns out, the relative path for a program is different when 
//    running through VS and when running the .exe directly, which makes 
//    it a pain to properly load external files (like textures)
//    - Running through VS: Current Dir is the *project folder*
//    - Running from .exe:  Current Dir is the .exe's folder
// - This has nothing to do with DEBUG and RELEASE modes - it's purely a 
//    Visual Studio "thing", and isn't obvious unless you know to look 
//    for it.  In fact, it could be fixed by changing a setting in VS, but
//    the option is stored in a user file (.suo), which is ignored by most
//    version control packages by default.  Meaning: the option must be
//    changed on every PC.  Ugh.  So instead, here's a helper.
// --------------------------------------------------------------------------
std::string AssetManager::GetExePath()
{
	// Assume the path is just the "current directory" for now
	std::string path = ".\\";

	// Get the real, full path to this executable
	char currentDir[1024] = {};
	GetModuleFileName(0, currentDir, 1024);

	// Find the location of the last slash charaacter
	char* lastSlash = strrchr(currentDir, '\\');
	if (lastSlash)
	{
		// End the string at the last slash character, essentially
		// chopping off the exe's file name.  Remember, c-strings
		// are null-terminated, so putting a "zero" character in 
		// there simply denotes the end of the string.
		*lastSlash = 0;

		// Set the remainder as the path
		path = currentDir;
	}

	// Toss back whatever we've found
	return path;
}


// ---------------------------------------------------
//  Same as GetExePath(), except it returns a wide character
//  string, which most of the Windows API requires.
// ---------------------------------------------------
std::wstring AssetManager::GetExePath_Wide()
{
	// Grab the path as a standard string
	std::string path = GetExePath();

	// Convert to a wide string
	wchar_t widePath[1024] = {};
	mbstowcs_s(0, widePath, path.c_str(), 1024);

	// Create a wstring for it and return
	return std::wstring(widePath);
}


// ----------------------------------------------------
//  Gets the full path to a given file.  NOTE: This does 
//  NOT "find" the file, it simply concatenates the given
//  relative file path onto the executable's path
// ----------------------------------------------------
std::string AssetManager::GetFullPathTo(std::string relativeFilePath)
{
	return GetExePath() + "\\" + relativeFilePath;
}



// ----------------------------------------------------
//  Same as GetFullPathTo, but with wide char strings.
// 
//  Gets the full path to a given file.  NOTE: This does 
//  NOT "find" the file, it simply concatenates the given
//  relative file path onto the executable's path
// ----------------------------------------------------
std::wstring AssetManager::GetFullPathTo_Wide(std::wstring relativeFilePath)
{
	return GetExePath_Wide() + L"\\" + relativeFilePath;
}

///---------------------------------------------------------------------------///