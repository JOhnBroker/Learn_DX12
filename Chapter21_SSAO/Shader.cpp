#include "Shader.h"

using namespace std;
using namespace DirectX;

bool ShaderDefines::operator==(const ShaderDefines& other) const
{
    if(definesMap.size() !=  other.definesMap.size())
        return false;

    for (const auto& pair : definesMap) 
    {
        auto iter = other.definesMap.find(pair.first);
        if (iter == other.definesMap.end() || iter->second != pair.second) 
        {
            return false;
        }
    }

    return true;
}

void ShaderDefines::GetD3DShaderMacro(std::vector<D3D_SHADER_MACRO>& retMacros) const
{
    for (const auto& pair : definesMap) 
    {
        D3D_SHADER_MACRO macro;
        macro.Name = pair.first.c_str();
        macro.Definition = pair.second.c_str();
        retMacros.push_back(macro);
    }
	D3D_SHADER_MACRO macro;
	macro.Name = NULL;
	macro.Definition = NULL;
	retMacros.push_back(macro);
}

void ShaderDefines::SetDefine(const std::string& name, const std::string& definition)
{
    definesMap.insert_or_assign(name, definition);
}

Shader::Shader(ID3D12Device* device, std::string filename)
    : m_pDevice(device), m_FileName(filename)
{
}
