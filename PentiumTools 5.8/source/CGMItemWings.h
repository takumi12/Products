#pragma once
class CUSTOM_WING_INFO;

typedef std::vector<CUSTOM_WING_INFO> type_wing_info;

class CGMItemWings
{
public:
	CGMItemWings();
	virtual~CGMItemWings();
	static CGMItemWings* Instance();


	void LoadDataWings(CUSTOM_WING_INFO* info);

	bool IsWingsByIndex(int nType);
	CUSTOM_WING_INFO* Find(int nType);
private:
	type_wing_info UniqueWingsData;
};


#define GMItemWings			(CGMItemWings::Instance())