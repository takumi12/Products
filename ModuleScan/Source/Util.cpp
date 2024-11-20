#include "stdafx.h"
#include "Util.h"

bool CreateFileDialog(std::string& FileLink, LPCWSTR pszTitle, REFCLSID rclsid, COMDLG_FILTERSPEC fileType, bool option /*= false*/, FILEOPENDIALOGOPTIONS fos /*= FOS_OVERWRITEPROMPT*/)
{
	bool success = false;

	IFileDialog* pFileDialog;

	if (SUCCEEDED(CoCreateInstance(rclsid/*CLSID_FileOpenDialog*/, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileDialog))))
	{
		pFileDialog->SetTitle(pszTitle);
		pFileDialog->SetFileTypes(1, &fileType);

		if (option)
			pFileDialog->SetOptions(fos);

		if (SUCCEEDED(pFileDialog->Show(NULL)))
		{
			IShellItem* pItem;
			if (SUCCEEDED(pFileDialog->GetResult(&pItem)))
			{
				PWSTR pszFolderPath;
				if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFolderPath)))
				{
					char ansiPath[MAX_PATH];
					WideCharToMultiByte(CP_ACP, 0, pszFolderPath, -1, ansiPath, MAX_PATH, NULL, NULL);

					FileLink = ansiPath;

					success = true;

					CoTaskMemFree(pszFolderPath);
				}
				pItem->Release();
			}
		}
		pFileDialog->Release();
	}

	return success;
}

void ListDLLDependencies(const char* exePath, const char* outputFile)
{
	// Cargar el ejecutable en memoria sin ejecutarlo ni resolver referencias
	HMODULE hModule = LoadLibraryExA(exePath, NULL, DONT_RESOLVE_DLL_REFERENCES);

	if (hModule == NULL)
	{
		std::cerr << "Error al cargar el ejecutable para inspección: " << GetLastError() << std::endl;
		return;
	}

	// Obtener la dirección del encabezado de NT del ejecutable
	PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hModule;
	if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
	{
		std::cerr << "El archivo no es un ejecutable válido." << std::endl;
		FreeLibrary(hModule);
		return;
	}

	// Obtener el encabezado NT
	PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((DWORD_PTR)hModule + dosHeader->e_lfanew);
	if (ntHeaders->Signature != IMAGE_NT_SIGNATURE)
	{
		std::cerr << "Encabezado NT no válido." << std::endl;
		FreeLibrary(hModule);
		return;
	}

	// Localizar la tabla de importaciones
	IMAGE_DATA_DIRECTORY importDirectory = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	if (importDirectory.VirtualAddress == 0)
	{
		std::cerr << "No se encontraron dependencias de DLL." << std::endl;
		FreeLibrary(hModule);
		return;
	}

	// Localizar la tabla de importaciones en la memoria del módulo cargado
	PIMAGE_IMPORT_DESCRIPTOR importDesc = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD_PTR)hModule + importDirectory.VirtualAddress);

	// Abrir el archivo .txt para escribir los resultados
	std::ofstream output(outputFile);

	if (!output.is_open())
	{
		std::cerr << "Error al abrir el archivo de salida." << std::endl;
		FreeLibrary(hModule);
		return;
	}

	output << "Dependencias DLL encontradas en " << exePath << ":\n\n";


	output << "[Dependencias de Aplicación]\n";

	// Iterar sobre las entradas de la tabla de importaciones

	std::vector<std::string> LogError;
	std::vector<std::string> LogSuccess;

	while (importDesc->Name != 0)
	{
		const char* dllName = (const char*)((DWORD_PTR)hModule + importDesc->Name);

		std::string DLL_NAME = "DLL: ";

		DLL_NAME += dllName;
		//output << "DLL: " << dllName;

		// Intentar cargar la DLL para obtener su ruta completa
		HMODULE hDLL = LoadLibraryExA(dllName, NULL, DONT_RESOLVE_DLL_REFERENCES);
		if (hDLL != NULL)
		{
			char dllPath[MAX_PATH];
			if (GetModuleFileNameA(hDLL, dllPath, MAX_PATH))
			{
				DLL_NAME += " - Path: ";
				DLL_NAME += dllPath;

				//output << " - Path: " << dllPath << std::endl;  // Escribir ruta completa
				LogSuccess.push_back(DLL_NAME);
			}
			else
			{
				DLL_NAME += " - Ruta no encontrada";
				//output << " - Ruta no encontrada" << std::endl;
				LogError.push_back(DLL_NAME);
			}
			FreeLibrary(hDLL);  // Liberar la DLL después de obtener la ruta
		}
		else
		{
			DLL_NAME += " - No se pudo cargar la DLL para obtener la ruta.";
			//output << " - No se pudo cargar la DLL para obtener la ruta." << std::endl;
			LogError.push_back(DLL_NAME);
		}

		++importDesc;
	}

	output << "[Sistema]\n";
	for (size_t i = 0; i < LogSuccess.size(); i++)
	{
		output << LogSuccess[i].c_str() << std::endl;
	}

	output << "\n[Contigencias]\n";
	for (size_t i = 0; i < LogError.size(); i++)
	{
		output << LogError[i].c_str() << std::endl;
	}

	// Cerrar el archivo de salida
	output.close();

	//std::cout << "\nDependencias DLL y rutas escritas en: " << outputFile << std::endl;

	// Liberar el módulo cargado
	FreeLibrary(hModule);
}

void ListDLLDependencies(const char* exePath)
{
	const char* procmonPath = "C:\\ruta\\a\\Procmon.exe"; // Actualiza esto a la ruta correcta

	// Construir los argumentos para Process Monitor
	std::string args = "/Backingfile ";
	args += exePath;
	args = args.substr(0, args.size() - 4); // Elimina la extensión .exe
	args += ".pml /Quiet";

	// Ejecutar Process Monitor
	HINSTANCE result = ShellExecute(NULL, "open", procmonPath, args.c_str(), NULL, SW_SHOWNORMAL);

	// Verificar si hubo un error
	if ((int)result <= 32) {
		std::cerr << "Error al ejecutar Process Monitor. Código de error: " << (int)result << std::endl;
	}
	else {
		std::cout << "Process Monitor se ha iniciado correctamente." << std::endl;
	}
}
