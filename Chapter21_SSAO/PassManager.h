#ifndef PASSMANAGER_H
#define PASSMANAGER_H

#include "d3dUtil.h"
#include "IPass.h"


class PassManager
{
public:

	
	std::unordered_map<std::string, IPass*> m_Pass;
};

#endif // !PASSMANAGER_H
