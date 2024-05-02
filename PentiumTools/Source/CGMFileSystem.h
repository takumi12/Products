#pragma once
class BITMAP_t;
class FileSystemNode;

typedef std::vector<FileSystemNode> type_vect_node;

class FileSystemNode
{
public:
	FileSystemNode();
	~FileSystemNode();
public:
	std::string name;
	std::string filename;
	bool isDirectory;
	BITMAP_t* bitmap;
	type_vect_node children;

	static bool CompareNodes(const FileSystemNode& a, const FileSystemNode& b)
	{
		return a.name == b.name && a.filename == b.filename && a.isDirectory == b.isDirectory;
	}
};

class CGMFileSystem
{
public:
	CGMFileSystem();
	~CGMFileSystem();
	static CGMFileSystem* Instance();
private:
	bool IsLoaderLink;
	std::string LinkHome;
	std::string LinkCurrent;
	FileSystemNode SystemRoot;
	FileSystemNode* currentNode;
public:
	std::string GetLinkHome();
	void LoadDirectory();
	void GetCurrentPath();
	void PreLoadDirectory();
	void OpenNewDirectory();
	bool CreateFileDialog(std::string& FileLink, LPCWSTR pszTitle, REFCLSID rclsid, COMDLG_FILTERSPEC fileType, bool option = false, FILEOPENDIALOGOPTIONS fos = FOS_OVERWRITEPROMPT);
	void FillFileSystemStructure(const std::string& FileLink, FileSystemNode& node);
	//--
	void ReSortTreeNode();
	void ReSortNode(FileSystemNode& node);
	void RenderTreeNode();
	bool RenderNode(FileSystemNode& node, FileSystemNode*& selectedNode);
	//--
	void ReloadDirectory(std::string selectItem = "");
	void FindNode(std::string selectItem);
	bool SelectNode(FileSystemNode& node, std::string selectItem, FileSystemNode*& selectedNode);

	//--
	void SaveImage();
	bool IsNodoSelect();
	const char* GetFileName();
	void CleanFileSystem();
};

#define LinkFileHome(link)	(((CGMFileSystem::Instance())->GetLinkHome()+link))
#define GMFileSystem        (CGMFileSystem::Instance())