#include "atype.h"
#include "time_fn.h"
#include <stdio.h>
#include <stdlib.h>
#include <stack>

static std::stack<struct timespec> t_stack;

struct tm *get_loc_time()
{
	time_t raw;
	struct tm *loc_time;

	time(&raw);
	loc_time = localtime(&raw);

	return loc_time;
}

time_t sNow()
{
	time_t raw;
	time(&raw);
	return raw;
}

void printTime(time_t time)
{
	char buf[MAX_U8];
	strftime(buf, MAX_U8, "%g %B %d  %H:%M:%S", localtime(&time));
}

void start_timer(void)
{
	struct timespec tm;
	timespec_get(&tm, TIME_UTC);
	t_stack.push(tm);
}

struct timespec stop_timer(void)
{
	struct timespec tm = {.tv_sec = 0, .tv_nsec = 0};
	struct timespec prev;
	if (t_stack.empty()) return tm;

	timespec_get(&tm, TIME_UTC);
	prev = t_stack.top();
	t_stack.pop();
	tm.tv_nsec -= prev.tv_nsec;
	if (tm.tv_nsec < 0)
	{
		tm.tv_nsec += 1000000000L;
		tm.tv_sec -= 1;
	}
	tm.tv_sec -= prev.tv_sec;

	return tm;
}

void print_elapsed_time(void)
{
	struct timespec tm = stop_timer();
	printf("%2lu.%06lu seconds passed\n", tm.tv_sec, tm.tv_nsec / 1000);
}
