#ifndef SHADER_H
#define SHADER_H

#include "d3dUtil.h"
#include <d3d12shader.h>

struct ShaderDefines
{
public:
	bool operator==(const ShaderDefines& other)const;

	void GetD3DShaderMacro(std::vector<D3D_SHADER_MACRO>& retMacros) const;
	void SetDefine(const std::string& name,const std::string& definition);

public:
	std::unordered_map<std::string, std::string> definesMap;
};

struct ShaderInfo
{
	std::string strShaderName;
	std::string strFileName;
	ShaderDefines shaderDefines;
};

class Shader
{
};

#endif // !SHADER_H
