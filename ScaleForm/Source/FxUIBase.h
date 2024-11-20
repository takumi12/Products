#ifndef _NEWUIBASE_H_
#define _NEWUIBASE_H_
#include "GFxPlayer.h"
#include "CEICommandContainer.h"

class CFrameEIHandler;


/*class CEICommandHandler
{
	enum ValueType
	{
		VT_Undefined = 0x00,
		VT_Null = 0x01,
		VT_Boolean = 0x02,
		VT_Number = 0x03,
		VT_String = 0x04,
		VT_StringW = 0x05,
		VT_Object = 0x06,
		VT_Array = 0x07,
		VT_DisplayObject = 0x08,
	};
public:
	CEICommandHandler(bool data) : valueType(VT_Boolean), Boolean(data) {
		Size = 0;
		commands = NULL;
	}
	CEICommandHandler(double data) : valueType(VT_Number), Number(data) {
		Size = 0;
		commands = NULL;
	}
	CEICommandHandler(const char* data) : valueType(VT_String) {
		strcpy(String, data);
		Size = 0;
		commands = NULL;
	}
	CEICommandHandler(char* data) : valueType(VT_String) {
		strcpy(String, data);
		Size = 0;
		commands = NULL;
	}
	CEICommandHandler(){
		valueType = VT_Object;
		Size = 0;
		commands = NULL;
	}
	~CEICommandHandler() {
		Size = 0;
		SAFE_DELETE_ARRAY(commands);
	}

	void Addargumento(const char* name, CEICommandAsValue value)
	{
		myObjectAsValue add;
		strncpy(add.HashName, name, sizeof(add.HashName) - 1); // Aseguramos que no haya desbordamientos
		add.HashName[sizeof(add.HashName) - 1] = '\0'; // Aseguramos la terminación nula
		add.HashValue = value;

		int newSize = Size + 1;

		if(commands != NULL)
		{
			myObjectAsValue* temp = new myObjectAsValue[newSize]; // Creamos el nuevo bloque con el nuevo tamaño
			for (int i = 0; i < Size; ++i)
			{
				temp[i] = commands[i]; // Copiamos los elementos existentes, puedes sobrecargar el operador si es necesario
			}
			temp[Size] = add; // Agregamos el nuevo elemento
			SAFE_DELETE_ARRAY(commands); // Liberamos la memoria antigua
			commands = temp; // Apuntamos al nuevo bloque de memoria
		}
		else
		{
			// Si no existe, solo creamos el bloque nuevo
			commands = new myObjectAsValue[newSize];
			commands[Size] = add;
		}
		Size = newSize;
	}
	int GetNumArgs()
	{
		return Size;
	}
private:
	int valueType;
	bool Boolean;
	double Number;
	char String[MAX_PATH];

	int Size;
	myObjectAsValue* commands;
};*/


class CExternalBase
{
public:
	virtual bool funcAction() = 0;
	virtual bool funcAction(CFrameEIHandler* inheritance, const char* methodName, const GFxValue* args, unsigned int numArgs) = 0;
};

class CFscommandBase
{
public:
	virtual bool funcAction(const char* pcommand, const char* parg) = 0;
};

class CInternalmethod : public CFscommandBase
{
public:
	CInternalmethod() {};
	virtual~CInternalmethod(){};
};

class CExternalmethod : public CExternalBase
{
public:
	CExternalmethod() {
		memset(methodName, 0, sizeof(methodName));
	};
	/*virtual~CExternalmethod() {
		(&s)->clear();
	};
	int size() {
		return (&s)->size();
	}
	void clear() {
		(&s)->clear();
	}
	void push_back(CEICommandHandler info) {
		(&s)->push_back(info);
	};
	CEICommandHandler& GetCommand(int index) {
		return s[index];
	}
	void SetName(const std::string& name) {
		methodName = name;
	}
	const std::string& GetName() {
		return methodName;
	}*/
private:
	char methodName[125];
	//std::vector<CEICommandHandler> s;
};

class MEMBER_VALUE
{
public:
	MEMBER_VALUE(const char* name, GFxValue val):name(name), member(val){
	}
	const char* GetName()
	{
		return name.c_str();
	}
	int GetHashType()
	{
		return member.GetType();
	}
	const char* GetString()
	{
		return member.GetString();
	}
	double GetNumber()
	{
		return member.GetNumber();
	}
	bool GetBoolean()
	{
		return member.GetBool();
	}
	std::string name;
	GFxValue member;
};

typedef std::vector<MEMBER_VALUE> type_member_value;


class CFxUIBase
{
public:
	virtual bool Render() = 0;
	virtual bool Update() = 0;
	virtual bool UpdateMouseEvent(int key) = 0;
	virtual bool UpdateKeyEvent() = 0;
	virtual void* GetMovie() = 0;
	virtual void InvokeObj(const char* pmethodName, CEICommandContainer* Obj) = 0;
	virtual void InvokeArray(const char* pmethodName, CEICommandContainer* Obj, int size) = 0;
	virtual void Invoke(const char* pmethodName, const char* pargFmt, va_list args) = 0;
	virtual bool OnCreateDevice(SInt bufw, SInt bufh, SInt left, SInt top, SInt w, SInt h, UInt flags = 0) = 0;
	virtual float GetLayerDepth() = 0;
	virtual float GetKeyEventOrder() = 0;

	virtual bool IsVisible() const = 0;
	virtual bool IsEnabled() const = 0;
};

class CFxUIObj : public CFxUIBase
{
public:
	CFxUIObj() : m_bRender(true), m_bUpdate(true) {}
	virtual ~CFxUIObj() {}
private:
		bool m_bRender, m_bUpdate;
public:
	void Show(bool bShow) {
		m_bRender = bShow;
	}
	void Enable(bool bEnable) {
		m_bUpdate = bEnable;
	}

	bool IsVisible() const {
		return m_bRender;
	}
	bool IsEnabled() const {
		return m_bUpdate; 
	}
	virtual float GetKeyEventOrder() { return 3.0f; }
};
#endif // _NEWUIBASE_H_