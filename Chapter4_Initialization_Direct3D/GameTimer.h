//***************************************************************************************
// CpuTimer.h by Frank Luna (C) 2011 All Rights Reserved.
// Modify name from GameTimer.cpp
// CPU计时器
//***************************************************************************************

#ifndef GAMETIMER_H
#define GAMETIMER_H

class GameTimer
{
public:
	GameTimer();

	float TotalTime()const;	// in seconds
	float DeltaTime() const;// in seconds

	void Reset();
	void Start();
	void Stop();
	void Tick();

private:
	double m_SecondPerCount;
	double m_DeltaTime;

	__int64 m_BaseTime;
	__int64 m_PauseTime;
	__int64 m_StopTime;
	__int64 m_PrevTime;
	__int64 m_CurrTime;

	bool m_Stopped;

};

#endif
