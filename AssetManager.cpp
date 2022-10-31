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
	m_srvMaps[SRVMaps::AO]			 = std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>();
	m_srvMaps[SRVMaps::Normal]		 = std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>();
	m_srvMaps[SRVMaps::Metalness]	 = std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>();
	
	m_srvMaps[SRVMaps::ToonAlbedo]	  = std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>();
	m_srvMaps[SRVMaps::ToonAO]		  = std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>();
	m_srvMaps[SRVMaps::ToonMetalness] = std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>();
}

void AssetManager::InitTextures()
{
	//make sure there's space for CreateWICTextureFromFile
	albedoMaps.push_back(nullptr);
	roughnessMaps.push_back(nullptr);
	aoMaps.push_back(nullptr);
	normalMaps.push_back(nullptr);
	metalnessMaps.push_back(nullptr);

	//load textures
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Medieval_Floor_Albedo.tif").c_str(), nullptr, albedoMaps[albedoMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Medieval_Floor_Roughness.tif").c_str(), nullptr, roughnessMaps[roughnessMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Medieval_Floor_AO.tif").c_str(), nullptr, aoMaps[aoMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Medieval_Floor_Normal.tif").c_str(), nullptr, normalMaps[normalMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/noMetal.png").c_str(), nullptr, metalnessMaps[metalnessMaps.size() - 1].GetAddressOf());

	albedoMaps.push_back(nullptr);
	roughnessMaps.push_back(nullptr);
	aoMaps.push_back(nullptr);
	normalMaps.push_back(nullptr);
	metalnessMaps.push_back(nullptr);

	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/SciFi_Panel_Albedo.tif").c_str(), nullptr, albedoMaps[albedoMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/SciFi_Panel_Roughness.tif").c_str(), nullptr, roughnessMaps[roughnessMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/SciFi_Panel_AO.tif").c_str(), nullptr, aoMaps[aoMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/SciFi_Panel_Normal.tif").c_str(), nullptr, normalMaps[normalMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/SciFi_Panel_Metalness.tif").c_str(), nullptr, metalnessMaps[metalnessMaps.size() - 1].GetAddressOf());

	albedoMaps.push_back(nullptr);
	roughnessMaps.push_back(nullptr);
	aoMaps.push_back(nullptr);
	normalMaps.push_back(nullptr);
	metalnessMaps.push_back(nullptr);
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Brick_Wall_Albedo.tif").c_str(), nullptr, albedoMaps[albedoMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Brick_Wall_Roughness.tif").c_str(), nullptr, roughnessMaps[roughnessMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Brick_Wall_AO.tif").c_str(), nullptr, aoMaps[aoMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Brick_Wall_Normal.tif").c_str(), nullptr, normalMaps[normalMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/noMetal.png").c_str(), nullptr, metalnessMaps[metalnessMaps.size() - 1].GetAddressOf());

	albedoMaps.push_back(nullptr);
	roughnessMaps.push_back(nullptr);
	aoMaps.push_back(nullptr);
	normalMaps.push_back(nullptr);
	metalnessMaps.push_back(nullptr);
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Bronze_Albedo.tif").c_str(), nullptr, albedoMaps[albedoMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Bronze_Roughness.tif").c_str(), nullptr, roughnessMaps[roughnessMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Bronze_AO.tif").c_str(), nullptr, aoMaps[aoMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Bronze_Normal.tif").c_str(), nullptr, normalMaps[normalMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Bronze_Metallic.tif").c_str(), nullptr, metalnessMaps[metalnessMaps.size() - 1].GetAddressOf());

	//toon materials, currently using defaults for many of them
	//while we figure out if we need them for toon shading
	toonAlbedoMaps.push_back(nullptr);
	toonRoughnessMaps.push_back(nullptr);
	toonAoMaps.push_back(nullptr);
	toonMetalnessMaps.push_back(nullptr);
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Tree_Albedo.tif").c_str(), nullptr, toonAlbedoMaps[toonAlbedoMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/noMetal.png").c_str(), nullptr, toonRoughnessMaps[toonRoughnessMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/allMetal.png").c_str(), nullptr, toonAoMaps[toonAoMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/noMetal.png").c_str(), nullptr, toonMetalnessMaps[toonMetalnessMaps.size() - 1].GetAddressOf());

	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Ramp_Texture.png").c_str(), nullptr, rampTexture.GetAddressOf());

	//load cube map
	skybox = CreateCubemap(
		GetFullPathTo_Wide(L"../../Assets/Textures/Sky/planet_right.png").c_str(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Sky/planet_left.png").c_str(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Sky/planet_up.png").c_str(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Sky/planet_down.png").c_str(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Sky/planet_front.png").c_str(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Sky/planet_back.png").c_str());
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

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> AssetManager::LoadSRV(const wchar_t* texturePath)
{
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture;
	LoadSRV(texturePath, texture);

	return texture;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> AssetManager::LoadSRV(const wchar_t* texturePath, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureDestination)
{
	CreateWICTextureFromFile(m_device.Get(), m_context.Get(),
		GetFullPathTo_Wide(texturePath).c_str(), nullptr, textureDestination.GetAddressOf());

	return textureDestination;
}

void AssetManager::AddSRVToMap(SRVMaps mapTypeName, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvToAdd)
{
	m_srvMaps[mapTypeName].push_back(srvToAdd);
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> AssetManager::GetSRV(SRVMaps map, int srvIndex)
{
	return m_srvMaps[map][srvIndex];
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