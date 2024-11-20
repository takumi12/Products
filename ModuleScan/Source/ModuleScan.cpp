// EncDec.cpp: define el punto de entrada de la aplicación de consola.
//

#include "stdafx.h"

int _tmain(int argc, _TCHAR* argv[])
{
	SetProcessDPIAware();

	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	if (FAILED(hr))
	{
		std::cerr << "Error al inicializar COM." << std::endl;
		return 1;
	}

	std::string Filename;

	COMDLG_FILTERSPEC fileType = { L"Allowed Files", L"*.exe" };

	bool success = CreateFileDialog(Filename, L"Open File", CLSID_FileOpenDialog, fileType, false, FOS_OVERWRITEPROMPT);

	if (success)
	{
		ListDLLDependencies(Filename.c_str());
		//ListDLLDependencies(Filename.c_str(), "dependencias_dll.txt");
	}

	return 0;
}

