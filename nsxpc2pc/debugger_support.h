#ifndef debugger_support_h
#define debugger_support_h

#include <stdio.h>
#include <mach/mach.h>

void
test_injection(mach_port_t task_port, mach_port_t installd_task_port);

#endif /* debugger_support_h */
