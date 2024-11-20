#pragma once

typedef struct
{
	int offset;
	int inSize;
	std::string varType;
	std::string varName;
} t_data_element;

class data_element
{
public:
	data_element() {
		name = "";
	}
	~data_element() {
		data.clear();
	}

	std::string name;
	std::vector<t_data_element> data;
};

class EncManager
{
public:
	void ReadData();
private:
	void Read(std::string filename, std::vector<t_data_element>& container_data);

	data_element Con[10];
};

