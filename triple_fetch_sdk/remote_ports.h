#ifndef remote_ports_h
#define remote_ports_h

#include <mach/mach.h>

#ifdef __cplusplus
extern "C" {
#endif

mach_port_t
pull_remote_port(mach_port_t task_port,
                 mach_port_name_t remote_port_name,
                 mach_port_right_t disposition);    // eg MACH_MSG_TYPE_COPY_SEND

mach_port_name_t
push_local_port(mach_port_t remote_task_port,
                mach_port_t port_to_push,
                mach_port_right_t disposition);

kern_return_t
remote_set_exception_ports(mach_port_t task_port,
                           exception_mask_t exception_mask,
                           mach_port_t new_port, // should be a send right
                           exception_behavior_t behavior,
                           thread_state_flavor_t new_flavor);

#ifdef __cplusplus
}
#endif

#endif /* remote_ports_h */
