#ifndef drop_payload_h
#define drop_payload_h

#include <mach/mach.h>

void drop_payload(mach_port_t privileged_task_port, mach_port_t spawn_context_task_port);
void start_debugserver(mach_port_t privileged_task_port, mach_port_t spawn_context_task_port);
void cache_privileged_port(mach_port_t privileged_port);
void cache_spawn_context_port(mach_port_t spawn_context_port);
void run_poc(char* name);

char* ps();
#endif
