#pragma once
#include "FxUIBase.h"

class CInterfaceNotice : public CInternalmethod, public CExternalmethod
{
public:
	CInterfaceNotice(void);
	virtual ~CInterfaceNotice(void);

	bool Create();
	bool funcAction();//- external
	bool funcAction(const char* pcommand, const char* parg);
	bool funcAction(CFrameEIHandler* inheritance, const char* methodName, const GFxValue* args, unsigned int numArgs); //--external
};

class CInterfacePlayer : public CInternalmethod, public CExternalmethod
{
public:
	CInterfacePlayer(void);
	virtual ~CInterfacePlayer(void);

	bool Create();
	bool funcAction();//- external
	bool funcAction(const char* pcommand, const char* parg);
	bool funcAction(CFrameEIHandler* inheritance, const char* methodName, const GFxValue* args, unsigned int numArgs); //--external
	void Connection(const char* methodName, const char* pargFmt);
};


class CInterfaceItemDrop : public CInternalmethod, public CExternalmethod
{
public:
	CInterfaceItemDrop(void);
	virtual ~CInterfaceItemDrop(void);

	bool Create();
	bool funcAction();//- external
	bool funcAction(const char* pcommand, const char* parg);
	bool funcAction(CFrameEIHandler* inheritance, const char* methodName, const GFxValue* args, unsigned int numArgs); //--external
	void Connection(const char* methodName, const char* pargFmt);
};

