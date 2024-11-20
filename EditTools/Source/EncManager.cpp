#include "framework.h"
#include "MemScript.h"
#include "EncManager.h"


void EncManager::ReadData()
{
}

void EncManager::Read(std::string filename, std::vector<t_data_element>& container_data)
{
	CMemScript* lpMemScript = new CMemScript;

	if (lpMemScript == 0)
	{
		printf(MEM_SCRIPT_ALLOC_ERROR, filename.c_str());
		return;
	}

	if (lpMemScript->SetBuffer(filename.c_str()) == 0)
	{
		printf(lpMemScript->GetLastError());
		delete lpMemScript;
		return;
	}

	try
	{
		while (true)
		{
			unsigned int token = lpMemScript->GetToken();

			if (token == TOKEN_END)
			{
				break;
			}

			if (strcmp("end", lpMemScript->GetString()) == 0)
			{
				break;
			}

			t_data_element element;

			element.offset = lpMemScript->GetNumber();

			element.varType = lpMemScript->GetAsString();

			element.inSize = lpMemScript->GetAsNumber();

			element.varName = lpMemScript->GetAsString();

			container_data.push_back(element);
		}
	}
	catch (...)
	{
		printf(lpMemScript->GetLastError());
	}

	delete lpMemScript;
}