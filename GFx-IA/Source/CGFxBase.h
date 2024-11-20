#ifndef _NEWUIBASE_H_
#define _NEWUIBASE_H_
#include "Vector.h"

class CEICommandAsValue
{
	enum valueType
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
	CEICommandAsValue();
	CEICommandAsValue(bool data);
	CEICommandAsValue(int data);
	CEICommandAsValue(float data);
	CEICommandAsValue(double data);
	CEICommandAsValue(__int64 data);
	CEICommandAsValue(unsigned __int64 data);
	CEICommandAsValue(DWORD data);
	CEICommandAsValue(char* data);
	CEICommandAsValue(const char* data);
	~CEICommandAsValue() {}

	int GetValueType();
	bool GetBooleano();
	double GetNumber();
	const char* GetString();
private:
	int valueType;
	bool _Boolean;
	double _Number;
	char _String[MAX_PATH];
};

typedef struct
{
	const char* GetHashName(){
		return HashName;
	}
	int GetHashType(){
		return HashValue.GetValueType();
	}
	bool GetHashBoolean(){
		return HashValue.GetBooleano();
	}
	double GetHashNumber(){
		return HashValue.GetNumber();
	}
	const char* GetHashString(){
		return HashValue.GetString();
	}

	void SetName(const char* data) {
		strncpy(HashName, data, sizeof(HashName) - 1);
		HashName[sizeof(HashName) - 1] = '\0';
	}
	char HashName[MAX_PATH];
	CEICommandAsValue HashValue;
}myObjectAsValue;

typedef std::vector<myObjectAsValue> type_value_map;

class CEICommandHandler
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
	CEICommandHandler(bool data) : valueType(VT_Boolean), Boolean(data) { }
	CEICommandHandler(double data) : valueType(VT_Number), Number(data) { }
	CEICommandHandler(const char* data) : valueType(VT_String), String(data) { }
	CEICommandHandler(const std::string& data) : valueType(VT_String), String(data) { }
	CEICommandHandler(){
		valueType = VT_Object;
	}
	~CEICommandHandler() { commands.clear(); }

	void Addargumento(const char* name, CEICommandAsValue value)
	{
		myObjectAsValue add;
		add.SetName(name);
		add.HashValue = value;
		commands.push_back(add);
	}
	int GetNumArgs()
	{
		return commands.size();
	}
private:
	int valueType;
	bool Boolean;
	double Number;
	std::string String;
	type_value_map commands;
};

class CExternalBase
{
public:
	virtual bool funcAction() = 0;
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
	CExternalmethod() {};
	virtual~CExternalmethod() {
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
	CEICommandHandler& GetCommand(int index){
		return s[index];
	}
	void SetName(const std::string& name) {
		methodName = name;
	}
	const std::string& GetName() {
		return methodName;
	}
private:
	std::string methodName;
	std::vector<CEICommandHandler> s;
};

class CEICommandContainer
{
public:
	/*CEICommandContainer() {
		//commands.clear();
	}
	~CEICommandContainer() {
		//commands.clear();
	}
	void AddCommand(const char* name, CEICommandAsValue value)
	{
		myObjectAsValue add;
		add.SetName(name);
		add.HashValue = value;
		commands.push_back(add);
	}*/
	int GetHashType(int i) {
		return commands[i].GetHashType();
	}
	bool GetBoolean(int i) {
		return commands[i].HashValue.GetBooleano();
	}
	double GetNumber(int i) {
		return commands[i].HashValue.GetNumber();
	}
	const char* GetString(int i) {
		return commands[i].HashValue.GetString();
	}
	const char* GetHashName(int i) {
		return commands[i].HashName;
	}

	// Obtener un valor por nombre
	int NumArgs() {
		return commands.size();
	}
private:
	std_vector<myObjectAsValue> commands;
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


class CGFxBase
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

class CNewUIObj : public CGFxBase
{
public:
	CNewUIObj() : m_bRender(true), m_bUpdate(true) {}
	virtual ~CNewUIObj() {}
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