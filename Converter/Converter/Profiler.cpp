#include"Profiler.h"

const char* Profiler::m_samplename;
time_t Profiler::m_start, Profiler::m_end, Profiler::m_time;

void Profiler::BeginSample(const char* name)
{
	m_samplename = name;
	m_start = clock();
}

void Profiler::EndSample()
{
	m_end = clock();

	m_time = m_end - m_start;

	cout << m_samplename<<":"<< m_time <<" ms"<< endl;
}