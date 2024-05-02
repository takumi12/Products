#pragma once
class CGMJSon
{
public:
	CGMJSon(const std::string& filename) : filename(filename)
	{
		index = 0;
		JsonElement = json();
		JsonTemplate = json();
	}

	CGMJSon& operator++()
	{
		index++;
		JsonElement = JsonTemplate[index];
		return (*this);
	}
private:
	int index;
	json JsonElement;
	json JsonTemplate;
	std::string filename;
public:
	bool OpenFile();
	json GetElement();
	json OpenTemplate();
};

