#if !defined(AFX_TIMER_H__DF94D50D_ED17_4DF1_9BD1_FA13662C8B74__INCLUDED_)
#define AFX_TIMER_H__DF94D50D_ED17_4DF1_9BD1_FA13662C8B74__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef CONSOLE
class CTimer
{
	BOOL	m_bUsePerformanceCounter;
	DWORD	m_mmTimerStart;
	DWORD	m_mmAbsTimerStart;
	float	m_resolution;
	__int64	m_pcTimerStart;
	__int64	m_pcAbsTimerStart;
	__int64	m_frequency;
public:
	CTimer();
	virtual ~CTimer();

	double GetAbsTime();
	void ResetTimer();
	double GetTimeElapsed();
};

class CTimer2
{
	DWORD m_dwStartTickCount, m_dwDelay;
	bool m_bTime;

public:
	CTimer2() : m_dwStartTickCount(0), m_dwDelay(0), m_bTime(false) {}
	~CTimer2() {}

	void SetTimer(DWORD dwDelay)
	{
		m_dwDelay = dwDelay;
		m_dwStartTickCount = 0;
	}
	DWORD GetDelay() const { return m_dwDelay; }
	void ResetTimer()
	{
		m_dwStartTickCount = 0;
	}
	void UpdateTime()
	{
		if (m_dwDelay == 0)
			m_bTime = true;
		else
		{
			m_bTime = false;
			if (m_dwStartTickCount == 0)
			{
				m_dwStartTickCount = ::GetTickCount();
				return;
			}
			if (::GetTickCount() - m_dwStartTickCount > m_dwDelay)
			{
				m_dwStartTickCount = ::GetTickCount();
				m_bTime = true;
			}
		}
	}
	bool IsTime() const { return m_bTime; }
};
#endif
#endif // !defined(AFX_TIMER_H__DF94D50D_ED17_4DF1_9BD1_FA13662C8B74__INCLUDED_)
