#pragma once

#include<iostream>
#include<ctime>

using namespace std;

class Profiler
{
private:
	static const char* m_samplename;

	static time_t m_start, m_end, m_time;


public:

	static void BeginSample(const char* name);

	static void EndSample();
};
