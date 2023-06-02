#include <windows.h>
#include "GameTimer.h"

GameTimer::GameTimer() :m_SecondPerCount(0.0f), m_DeltaTime(-1.0f),
m_BaseTime(0), m_PauseTime(0), m_StopTime(0), m_PrevTime(0), m_CurrTime(0), m_Stopped(false)
{
	__int64 countsPerSec = 0;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	m_SecondPerCount = 1.0 / (double)countsPerSec;
}

float GameTimer::TotalTime() const
{
	if (m_Stopped)
	{
		//                     |<--paused time-->|
		// ----*---------------*-----------------*------------*------------*------> time
		//  mBaseTime       mStopTime        startTime     mStopTime    mCurrTime
		return (float)(((m_StopTime - m_PauseTime) - m_BaseTime) * m_SecondPerCount);
	}
	else
	{
		//  (mCurrTime - mPausedTime) - mBaseTime 
		//
		//                     |<--paused time-->|
		// ----*---------------*-----------------*------------*------> time
		//  mBaseTime       mStopTime        startTime     mCurrTime
		return (float)(((m_CurrTime - m_PauseTime) - m_BaseTime) * m_SecondPerCount);
	}
}

float GameTimer::DeltaTime() const
{
	return (float)m_DeltaTime;
}

void GameTimer::Reset()
{
	__int64 currTime = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	m_BaseTime = currTime;
	m_PrevTime = currTime;
	m_StopTime = 0;
	m_Stopped = false;
}

void GameTimer::Start()
{
	__int64 statrTime = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&statrTime);

	// Accumulate the time elapsed between stop and start pairs.
	//
	//                     |<-------d------->|
	// ----*---------------*-----------------*------------> time
	//  mBaseTime       mStopTime        startTime     

	if (m_Stopped)
	{
		m_PauseTime += (statrTime - m_StopTime);
		m_StopTime = 0;
		m_Stopped = false;
	}
}

void GameTimer::Stop()
{
	if (!m_Stopped)
	{
		__int64 currTime = 0;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
		m_StopTime = currTime;
		m_Stopped = true;
	}
}

void GameTimer::Tick()
{
	__int64 currTime = 0;

	if (m_Stopped)
	{
		m_DeltaTime = 0;
		goto Exit0;
	}

	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	m_CurrTime = currTime;

	m_DeltaTime = (m_CurrTime - m_PrevTime) * m_SecondPerCount;

	m_PrevTime = m_CurrTime;

	if (m_DeltaTime < 0.0f)
	{
		m_DeltaTime = 0.0f;
	}

Exit0:
	return;
}
