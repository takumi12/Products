#include "framework.h"
#include "CGMLibrayMng.h"
#include "CGlobalBitmap.h"
#include "CGMActionLog.h"
#include "CGMFileSystem.h"

CGMFileSystem::CGMFileSystem()
{
	LinkCurrent = "";
	IsLoaderLink = false;
	currentNode = nullptr;

	SystemRoot.name = "Root";
	SystemRoot.isDirectory = true;
	GetCurrentPath();
}

CGMFileSystem::~CGMFileSystem()
{
	currentNode = nullptr;
}

CGMFileSystem* CGMFileSystem::Instance()
{
	static CGMFileSystem tc; return &tc;
}

bool CompareNodes(const FileSystemNode& a, const FileSystemNode& b)
{
	// Carpetas primero, luego archivos
	if (a.isDirectory && !b.isDirectory) {
		return true;
	}
	return false;
}

std::string CGMFileSystem::GetLinkHome()
{
	return LinkHome;
}

void CGMFileSystem::LoadDirectory()
{
	if (IsLoaderLink == false)
	{
		IsLoaderLink = true;
		this->PreLoadDirectory();
	}
}

void CGMFileSystem::GetCurrentPath()
{
	char buffer[MAX_PATH];
	GetModuleFileName(nullptr, buffer, MAX_PATH);

	std::string path(buffer);
	size_t lastSlash = path.find_last_of("\\/");

	if (lastSlash != std::string::npos)
	{
		LinkHome = path.substr(0, lastSlash + 1);
	}
	else
	{
		LinkHome = ".\\";
	}
}

void CGMFileSystem::PreLoadDirectory()
{
	if (LinkCurrent == "")
	{
		IFileDialog* pFileDialog;
		if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileDialog))))
		{
			// Configuración del diálogo (opcional)
			pFileDialog->SetTitle(L"Select a folder");
			pFileDialog->SetOptions(FOS_PICKFOLDERS);

			// Mostrar el diálogo
			if (SUCCEEDED(pFileDialog->Show(NULL)))
			{
				// Obtener el resultado
				IShellItem* pItem;
				if (SUCCEEDED(pFileDialog->GetResult(&pItem)))
				{
					PWSTR pszFolderPath;
					if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFolderPath)))
					{
						char ansiPath[MAX_PATH];
						WideCharToMultiByte(CP_ACP, 0, pszFolderPath, -1, ansiPath, MAX_PATH, NULL, NULL);
						LinkCurrent = ansiPath;
						CoTaskMemFree(pszFolderPath);
					}
					else
					{
						LinkCurrent = LinkHome;
					}

					pItem->Release();
				}
				else
				{
					LinkCurrent = LinkHome;
				}
			}
			else
			{
				LinkCurrent = LinkHome;
			}
			pFileDialog->Release();
		}
		FillFileSystemStructure(LinkCurrent, SystemRoot);

		ReSortTreeNode();
	}
}

void CGMFileSystem::OpenNewDirectory()
{
	IFileDialog* pFileDialog;
	if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileDialog))))
	{
		pFileDialog->SetTitle(L"Select a folder");
		pFileDialog->SetOptions(FOS_PICKFOLDERS);

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
					LinkCurrent = ansiPath;
					CoTaskMemFree(pszFolderPath);

					CleanFileSystem();

					FillFileSystemStructure(LinkCurrent, SystemRoot);

					ReSortTreeNode();
				}

				pItem->Release();
			}
		}
		pFileDialog->Release();
	}
}

bool CGMFileSystem::CreateFileDialog(std::string& FileLink, LPCWSTR pszTitle, REFCLSID rclsid, COMDLG_FILTERSPEC fileType, bool option, FILEOPENDIALOGOPTIONS fos)
{
	bool success = false;

	IFileDialog* pFileDialog;

	if (SUCCEEDED(CoCreateInstance(rclsid/*CLSID_FileOpenDialog*/, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileDialog))))
	{
		pFileDialog->SetTitle(pszTitle);
		pFileDialog->SetFileTypes(1, &fileType);

		if(option)
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

void CGMFileSystem::FillFileSystemStructure(const std::string& FileLink, FileSystemNode& node)
{
	WIN32_FIND_DATA findFileData;
	HANDLE hFind = FindFirstFile((FileLink + "\\*").c_str(), &findFileData);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			FileSystemNode childNode;
			childNode.name = findFileData.cFileName;
			childNode.filename = FileLink + "\\" + childNode.name;
			childNode.isDirectory = (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

			if (childNode.isDirectory && childNode.name != "." && childNode.name != "..")
			{
				// Si es un directorio, llena recursivamente la estructura de archivos
				FillFileSystemStructure(FileLink + "\\" + childNode.name, childNode);
			}

			if (childNode.name != "." && childNode.name != "..")
			{
				if (childNode.isDirectory)
				{
					node.children.push_back(childNode);
				}
				else
				{
					std::string fileName = childNode.name;
					std::string::size_type extPos = fileName.find_last_of('.');

					if (extPos != std::string::npos)
					{
						std::string fileExtension = fileName.substr(extPos);

						if (GMLibrayMng->IsFileValid(fileExtension))
						{
							node.children.push_back(childNode);
						}
					}
				}
			}
		} while (FindNextFile(hFind, &findFileData) != 0);
		FindClose(hFind);
	}
}

void CGMFileSystem::ReSortTreeNode()
{
	// Ordenar nodes
	std::sort(SystemRoot.children.begin(), SystemRoot.children.end(), CompareNodes);

	for (auto& node : SystemRoot.children)
	{
		ReSortNode(node);
	}
}

void CGMFileSystem::ReSortNode(FileSystemNode& node)
{
	if (node.isDirectory)
	{
		std::sort(node.children.begin(), node.children.end(), CompareNodes);

		for (auto& child : node.children)
		{
			ReSortNode(child);
		}
	}
}

void CGMFileSystem::RenderTreeNode()
{
	if (ImGui::TreeNode(SystemRoot.name.c_str()))
	{
		for (auto& node : SystemRoot.children)
		{
			RenderNode(node, currentNode);
		}
		ImGui::TreePop();
	}
	else
	{
		ImGui::TreeNodeOpen(SystemRoot.name.c_str());
	}
}

bool CGMFileSystem::RenderNode(FileSystemNode& node, FileSystemNode*& selectedNode)
{
	bool success = false;

	if (node.isDirectory)
	{
		if (ImGui::TreeNode(node.name.c_str()))
		{
			for (auto& child : node.children)
			{
				RenderNode(child, selectedNode);
			}
			ImGui::TreePop();
		}
	}
	else
	{
		bool Selected = (!ImGui::IsWindowFocused() && selectedNode == &node);

		if (ImGui::SelectableHovered(node.name.c_str(), Selected, ImGuiSelectableFlags_AllowDoubleClick))
		{
			if (selectedNode != (&node) && (&node)->isDirectory == false)
			{
				selectedNode = &node;
				GlobalBitmap->LoadImages(selectedNode->filename);
			}
		}
	}

	return success;
}

void CGMFileSystem::ReloadDirectory(std::string selectItem)
{
	CleanFileSystem();

	FillFileSystemStructure(LinkCurrent, SystemRoot);

	ReSortTreeNode();

	if (selectItem != "")
	{
		FindNode(selectItem);
	}
}

void CGMFileSystem::FindNode(std::string selectItem)
{
	if (selectItem != "")
	{
		for (auto& node : SystemRoot.children)
		{
			if (SelectNode(node, selectItem, currentNode))
			{
				return;
			}
		}
	}
}

bool CGMFileSystem::SelectNode(FileSystemNode& node, std::string selectItem, FileSystemNode*& selectedNode)
{
	if (node.isDirectory)
	{
		for (auto& child : node.children)
		{
			if (SelectNode(child, selectItem, selectedNode))
			{
				return true;
			}
		}
	}
	else
	{
		if (selectItem == node.filename)
		{
			selectedNode = &node;
			GlobalBitmap->LoadImages(selectedNode->filename);
			return true;
		}
	}
	return false;
}

void CGMFileSystem::SaveImage()
{
	if (IsNodoSelect())
	{
		std::string FileName = currentNode->filename;

		if (GlobalBitmap->SaveImage(FileName))
		{
			GMActionLog->Push("Image Convert: " + FileName);
			this->ReloadDirectory(FileName);
		}
		else
		{
			GMActionLog->Push("[Error] Image Convert: " + FileName);
		}
	}
}

bool CGMFileSystem::IsNodoSelect()
{
	return currentNode != nullptr && !currentNode->isDirectory;
}

const char* CGMFileSystem::GetFileName()
{
	if (IsNodoSelect() && currentNode->name != "")
	{
		return currentNode->name.c_str();
	}
	else
	{
		return "Image Convert";
	}
}

void CGMFileSystem::CleanFileSystem()
{
	currentNode = nullptr;
	SystemRoot.children.clear();
}

FileSystemNode::FileSystemNode()
{
	isDirectory = false;
	bitmap = nullptr;
}

FileSystemNode::~FileSystemNode()
{
	children.clear();
	if (bitmap && bitmap->uiBitmapIndex != ((GLuint)-1))
	{
		glDeleteTextures(1, &(bitmap->uiBitmapIndex));
		delete bitmap;
	}
}

/*void FileSystemNode::Render()
{
	if (bitmap && bitmap->uiBitmapIndex != ((GLuint)-1))
	{
		ImVec2 uv_min = ImVec2(0.0f, 0.0f);

		ImVec2 uv_max = ImVec2(1.0f, 1.0f);

		ImTextureID TextureId = (ImTextureID)(bitmap->uiBitmapIndex);

		ImGui::Image(TextureId, ImVec2(16, 16), uv_min, uv_max);

		ImGui::SameLine();
	}
}

void FileSystemNode::loadicon()
{
	SHFILEINFO fileInfo = { 0 };
	// Obtiene el ícono de una carpeta
	SHGetFileInfo(_T(filename.c_str()), FILE_ATTRIBUTE_DIRECTORY, &fileInfo, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES);

	if (fileInfo.hIcon)
	{
		ICONINFO iconInfo = { 0 };
		if (GetIconInfo(fileInfo.hIcon, &iconInfo))
		{
			if (iconInfo.hbmColor != nullptr)
			{
				DIBSECTION ds;
				GetObject(iconInfo.hbmColor, sizeof(ds), &ds);
				int byteSize = ds.dsBm.bmWidth * ds.dsBm.bmHeight * (ds.dsBm.bmBitsPixel / 8);

				if (byteSize > 0)
				{
					bitmap = new BITMAP_t;
					uint8_t* data = (uint8_t*)malloc(byteSize);
					GetBitmapBits(iconInfo.hbmColor, byteSize, data);
					bitmap->uiBitmapIndex = GlobalBitmap->LoadNodeIcon(ds.dsBm.bmWidth, ds.dsBm.bmHeight, data);

					bitmap->output_width = (float)ds.dsBm.bmWidth;
					bitmap->output_height = (float)ds.dsBm.bmHeight;

					free(data);
				}
				DeleteObject(iconInfo.hbmColor);
			}

			if (iconInfo.hbmMask)
				DeleteObject(iconInfo.hbmMask);
		}
		DestroyIcon(fileInfo.hIcon);
	}
}*/
