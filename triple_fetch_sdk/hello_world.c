#include <stdio.h>
#include <mach/mach.h>

#include "task_ports.h"

kern_return_t mach_vm_region
(
        vm_map_t target_task,
        mach_vm_address_t *address,
        mach_vm_size_t *size,
        vm_region_flavor_t flavor,
        vm_region_info_t info,
        mach_msg_type_number_t *infoCnt,
        mach_port_t *object_name
);

int main(int argc, char** argv) {
  printf("hello triple_fetch!\n");
  refresh_task_ports_list();

  mach_port_t launchd_task_port = find_task_port_for_path("/sbin/launchd");
  if (launchd_task_port == MACH_PORT_NULL) {
    printf("failed to get launchd task port\n");
    return 0;
  }

  printf("got task port for launchd: %x\n", launchd_task_port);

  // dump the vm regions:
  struct vm_region_basic_info_64 region;
  mach_msg_type_number_t region_count = VM_REGION_BASIC_INFO_COUNT_64;
  memory_object_name_t object_name = MACH_PORT_NULL; /* unused */

  mach_vm_size_t size = 0x1000;
  mach_vm_address_t addr = 0x0;
  kern_return_t err;

  for(;;){
    /* get the region info for the next region */
    err = mach_vm_region(launchd_task_port,
                         &addr,
                         &size,
                         VM_REGION_BASIC_INFO_64,
                         (vm_region_info_t)&region,
                         &region_count,
                         &object_name);

    if (err != KERN_SUCCESS){
      break;
    }
    printf("%016llx %016llx ", addr, size);

    if (region.protection & VM_PROT_READ){
			printf("r");
		} else {
      printf("-");
    }
    
    if (region.protection & VM_PROT_WRITE){
			printf("w");
		} else {
      printf("-");
    }
    
    if (region.protection & VM_PROT_EXECUTE){
			printf("x");
		} else {
      printf("-");
    }

    printf("\n");

    addr += size;
  }

  return 0;
}


