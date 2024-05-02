#pragma once

class CGMActionLog
{
public:
	CGMActionLog(std::size_t maxSize) : maxSize(maxSize)
	{
	}

	void Push(const std::string& entry);

	void RenderLog();
	void RenderChildLog();

	static CGMActionLog* Instance() {
		static CGMActionLog tc(100); return &tc;
	}
private:
	std::deque<std::string> logDeque;
	std::size_t maxSize;
};

#define GMActionLog			(CGMActionLog::Instance())