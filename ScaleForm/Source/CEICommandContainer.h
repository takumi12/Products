#pragma once

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
	~CEICommandAsValue();
	CEICommandAsValue(char* data);
	CEICommandAsValue(const char* data);
	CEICommandAsValue(bool data);
	CEICommandAsValue(short data);
	CEICommandAsValue(int data);
	CEICommandAsValue(float data);
	CEICommandAsValue(double data);
	CEICommandAsValue(BYTE data);
	CEICommandAsValue(WORD data);
	CEICommandAsValue(DWORD data);
	CEICommandAsValue(unsigned __int64 data);
public:
	int GetType() const;
	bool to_boolean() const;
	double to_Number() const;
	const char* to_char() const;
private:
	int _Type;
	char _value[256];
};

struct CommandEntry
{
	char _Name[64];
	CEICommandAsValue _Value;

	int GetType() {
		return _Value.GetType();
	};
	bool to_boolean() {
		return _Value.to_boolean();
	};
	double to_Number() {
		return _Value.to_Number();
	};
	const char* to_char() {
		return _Value.to_char();
	};
	const char* to_Name() {
		return _Name;
	};
};

class CEICommandEntryContainer
{
public:
	CEICommandEntryContainer() {
	};
	~CEICommandEntryContainer() {
		_dataEntry.clear();
	}
	void AddCommand(const char* name, CEICommandAsValue Val) {
		CommandEntry buffer;

		strncpy_s(buffer._Name, name, sizeof(buffer._Name) - 1);
		buffer._Name[sizeof(buffer._Name) - 1] = '\0';
		buffer._Value = Val;

		_dataEntry.push_back(buffer);
	}
	const CommandEntry* data() const {
		return _dataEntry.data();
	}
	size_t size() const {
		return _dataEntry.size();
	}
private:
	std::vector<CommandEntry> _dataEntry;
};

class CEICommandContainer
{
public:
	CEICommandContainer() : Size(0), _dataEntry(nullptr) {}

	// Constructor que copia los datos del otro contenedor
	CEICommandContainer(const CEICommandEntryContainer& Entry) {
		Size = Entry.size();
		_dataEntry = new CommandEntry[Size]; // Copia profunda de los datos

		for (size_t i = 0; i < Size; ++i) {
			_dataEntry[i] = Entry.data()[i]; // Copiar cada comando
		}
	}

	~CEICommandContainer() {
		delete[] _dataEntry;
	}

	const CommandEntry* data() const {
		return _dataEntry;
	}

	size_t size() const {
		return Size;
	}

	CommandEntry GetEntry(int index)
	{
		return _dataEntry[index];
	}
public:
	CommandEntry operator[](int index)
	{
		return _dataEntry[index];
	}
private:
	size_t Size;
	CommandEntry* _dataEntry;
};