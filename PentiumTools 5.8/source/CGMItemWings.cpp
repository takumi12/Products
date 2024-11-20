#include "stdafx.h"
#include "supportingfeature.h"
#include "CGMItemWings.h"

CGMItemWings::CGMItemWings()
{
}

CGMItemWings::~CGMItemWings()
{
	UniqueWingsData.clear();
}

CGMItemWings* CGMItemWings::Instance()
{
	static CGMItemWings sInstance;
	return &sInstance;
}

void CGMItemWings::LoadDataWings(CUSTOM_WING_INFO* info)
{
	for (int i = 0; i < MAX_CUSTOM_WINGS; i++)
	{
		if (info[i].isUsable() == true)
		{
			UniqueWingsData.push_back(info[i]);
		}
	}
}

bool CGMItemWings::IsWingsByIndex(int nType)
{
	if (nType >= 0 && nType < MAX_ITEM)
	{
		for (size_t i = 0; i < UniqueWingsData.size(); i++)
		{
			if (UniqueWingsData[i].ItemIndex == nType)
			{
				return true;
			}
		}
	}
	return false;
}

CUSTOM_WING_INFO* CGMItemWings::Find(int nType)
{
	if (nType >= 0 && nType < MAX_ITEM)
	{
		for (size_t i = 0; i < UniqueWingsData.size(); i++)
		{
			if (UniqueWingsData[i].ItemIndex == nType)
			{
				return &UniqueWingsData[i];
			}
		}
	}
	return NULL;
}
