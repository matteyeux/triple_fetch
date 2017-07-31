triple_fetch - ianbeer

This is an exploit for CVE-2017-7047, a logic error in libxpc which allowed
malicious message senders to send xpc_data objects that were backed by shared memory.
Consumers of xpc messages did not seem to expect that the backing buffers of xpc_data objects
could be modified by the sender whilst being processed by the receiver.

This project exploits CVE-2017-7047 to build a proof-of-concept remote lldb debugserver
stub capable of attaching to and allowing the remote debugging all userspace
processes on iOS.

This is a high-level overview of the exploit, an in-depth writeup may follow
at a later date. For now please see the code for further details :)

Part I

The exploit targets NSXPC, an Objective-C Remote Procedure Call implementation
used by many iOS services (1). An NSXPC message consists of a bplist16 serialized object
inside an xpc serialized xpc_data object inside a mach message.

Amongst other things the bplist16 object contains an Objective-C type encoding string (2)
which will be parsed by the function ___NSMS1 in CoreFoundation.
This function does not expect the contents of the string it is parsing to change and this
exploit uses that to construct a heap overflow primitive by exploiting the fact that
a certain part of the string will be fetched from memory three times. By switching between
three different, carefully chosen values we are able to overflow out of a
chosen heap allocation size with arbitrary bytes.

minibplist16.c contains a minimal implementation of bplist serialization and a discussion of
how it works.

The outer xpc message contains the heap groom. It uses a custom implementation of the
XPC serialization protocol to groom the heap by crafting xpc dictionaries with colliding keys
to build allocation and free primitives.
The outer xpc message also contains a heap spray (using multiple copies of a shared memory
object to keep the memory usage low) and a mach port send right name spray.

The overflow points the isa Class pointer of an Objective-C object to a heap-sprayed
fake object such that when a method is called on that fake object the stack is pivoted
to a small ROP stack. The ROP brute-forces through the sprayed mach port send right names
trying to send the target’s send right to its own task port to each of the candidate sprayed
send right names. The exploit listens on all the sprayed ports and if the exploit is successful
it receives a send right to the target’s task port, at which point it has full control
over the target task.

Interlude

The exploit targets the com.apple.CoreAuthentication.daemon service hosted by the coreauthd
daemon which runs as root. This service can be reached from the app sandbox.
A bit of experimentation after I initially got the exploit to work revealed that
from the context of coreauthd the processor_set_tasks API is able to get send rights to the
task ports for all userspace processes running on the device. This has been public knowledge
since at least 2012 and the history is covered in depth by the prominent iOS internals researcher
Jonathan Levin on his site (3).
The code which Levin uploaded in 2015 still works today - it does not require a jailbroken device,
just root on a stock device.

Part II

The main goal for the debugger I wanted to build with this exploit was to be able to attach
to an arbitrary process, set breakpoints and inspect and alter register and memory state
when they are hit. Rather than implement the gdb or lldb remote protocol from scratch
I decided to make the changes required to the lldb debugserver project then use the exploit to run it.

Rather than using software breakpoints which require either disabling or defeating code signing
the debugserver is patched to exclusively use hardware breakpoints. ARM64 has 16 hardware breakpoint
registers which means you can only have a maximum of 16 active breakpoints.

Prototype support for ARM hardware breakpoints did exist in the lldb debugserver code
but it required some hacking to get it to work. For example I had to add code which patches the
pthread_introspection_hook function pointer in the debugee to always crash so that I can detect
the creation of new threads and propagate the hardware breakpoint state into newly created threads and
continue as if it never crashed.

I have also patched the attach and continue code to directly suspend and resume the task via the task port
rather than using ptrace and signals.

Build tips

Everything *should* work for all iOS devices running 10.0 to 10.3.2 inclusive. I have tested on:

iPhone 7 + 10.3.2
iPod Touch + 10.1.1
iPad Mini 2 + 10.2

I have included a pre-built debugserver binary which I suggest you use but
the patch for lldb’s debugserver is also included in debugserver.diff.

Building the debugserver isn’t too hard. I was working off of the following git revisions:
lldb: 0db640c4cd1ec4e4c2580336fa5f53be029c5bc7
llvm: ec48fd127774a4b67c72ea7c3057b5c964375e77
clang: b6e778e0bfa2fc32f8821c6b33762f5cb6724659

apply the supplied debugserver.diff.

For the build you need (recent) cmake and ninja, you can get these from source or binaries
from your favourite mac package manager.

(4) has a guide for how to set up a normal llvm build on MacOS which may be helpful.

You will need to symlink a bunch of header files into your iOS SDK, at least:

xpc/
launchd.h
libproc.h
sys/proc_info.h
sys/kern_control.h
net/route.h
mach/mach_vm.h
mach/shared_region.h
sys/ptrace.h
crt_externs.h

the following cmake incantation should give you all the hints you need:

cmake -G "Ninja" -DCMAKE_OSX_ARCHITECTURES="armv7;armv7s;arm64" -DCMAKE_TOOLCHAIN_FILE=../cmake/platforms/iOS.cmake -DCMAKE_BUILD_TYPE=Release -DLLVM_BUILD_RUNTIME=Off -DLLVM_INCLUDE_TESTS=Off -DLLVM_INCLUDE_EXAMPLES=Off -DLLVM_ENABLE_BACKTRACES=Off ../

ninja debugserver

You’ll then need to sign or fakesign the debugserver binary and replace the one in the supplied xcode project.

Codesigning

The exploit project by default will install an improved version of the mach_portal amfid hook
(this time with working support for fat files and no hardcoded offsets :) )

If you just want to debug stuff you should be able to sign the debugserver binary with your own
cert and disable the amfid hook.

If you do use the amfid hook bear in mind that the app it’s running in is still subject to
background code execution limits. The app does request more time via beginBackgroundTaskWithName.

Usage:

Connect your host and target iDevice to the same wireless network and note the iDevice’s IP address.

Build and run the exploit app. I recommend doing this inside xcode but it will work standalone.

Wait a bit. If it doesn’t work after a couple of minutes hard-reboot the device, wait a bit then try again.

If it works it should print “patched debugserver listening on port 1234”

If you click the “get process listing” button you should see the output of ps

(its easier to see the output if you use xcode but the exploit will also show the output)

Look for the target process you're interested in debugging and note its pid.

on the host launch lldb from the command line:
$ lldb
(lldb)

set the platform to ios remote:
(lldb) platform select remote-ios

connect to the debugserver stub:
(lldb) process connect connect://192.168.0.172:1234

(where 192.168.0.172 is the IP address of the iDevice)

attach to the process you’re interested in:
(lldb) attach 55

  ...wait a bit, debugserver is running in verbose mode...

Process 55 stopped

Executable module set to "/usr/libexec/backboardd".

you’re attached :)

set a breakpoint:
(lldb) break set --name malloc
Breakpoint 1: 4 locations.

continue:

(lldb) c
Process 55 resuming
Process 55 stopped
* thread #11, stop reason = breakpoint 1.3
frame #0: 0x00000001936161e0 libsystem_malloc.dylib`malloc
libsystem_malloc.dylib`malloc:
->  0x1936161e0 <+0>: stp    x20, x19, [sp, #-0x20]!
0x1936161e4 <+4>: stp    x29, x30, [sp, #0x10]
0x1936161e8 <+8>: add    x29, sp, #0x10            ; =0x10

get a backtrace:

(lldb) bt
* thread #11, stop reason = breakpoint 1.3
* frame #0: 0x00000001936161e0 libsystem_malloc.dylib`malloc
frame #1: 0x0000000100099648 backboardd`_mh_execute_header + 71240
frame #2: 0x00000001945bf218 CoreFoundation`__CFRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE1_PERFORM_FUNCTION__ + 56
frame #3: 0x00000001945be9cc CoreFoundation`__CFRunLoopDoSource1 + 436
frame #4: 0x00000001945bc4b0 CoreFoundation`__CFRunLoopRun + 1840
frame #5: 0x00000001944ea2b8 CoreFoundation`CFRunLoopRunSpecific + 444
frame #6: 0x0000000194537b44 CoreFoundation`CFRunLoopRun + 112
frame #7: 0x00000001000a5ba8 backboardd`_mh_execute_header + 121768
frame #8: 0x00000001000a5bec backboardd`_mh_execute_header + 121836
frame #9: 0x00000001936a5850 libsystem_pthread.dylib`_pthread_body + 240
frame #10: 0x00000001936a5760 libsystem_pthread.dylib`_pthread_start + 284
frame #11: 0x00000001936a2d94 libsystem_pthread.dylib`thread_start + 4

Troubleshooting:

Install the latest version of XCode (developed and tested with 8.3.3)
If lldb connect fails make sure you have the SDK for the target iOS version installed

Caveats:

 * you can only set breakpoints at 16 addresses
 * detach doesn’t work yet
 * attach only works by pid
 * 64-bit targets only
 * you can get the task port for launchd and read/write memory but attach hangs at the moment
 * if you are using the amfid hook you won't be able to debug amfid!

I hope to fix these but am very busy at the moment, sorry!

Running other PoCs:

If you drop a binary in the pocs folder in this project you can get the exploit to exec it by selecting it in the UI
on the device and clicking "exec bundle binary"."

It will still run inside the app sandbox, but if the app has a symbol called privileged_task_port
it will be given a send right to launchd's task port. The triple_fetch_sdk folder contains a sample
project showing how you can use this to build PoCs, for example to trigger behaviour in interesting processes that you
can then debug using the debugserver.

It also contains the remote call/ports/files/memory APIs which let you easily call functions in other processes
and move file descriptors, mach ports and memory around.

(1) [https://developer.apple.com/library/content/documentation/MacOSX/Conceptual/BPSystemStartup/Chapters/CreatingXPCServices.html]
(2) [https://developer.apple.com/library/content/documentation/Cocoa/Conceptual/ObjCRuntimeGuide/Articles/ocrtTypeEncodings.html]
(3) [http://newosxbook.com/articles/PST2.html]
(4) [https://gist.github.com/thlorenz/a068c202f2487ec13809]

This project contains code (debugserver.diff) and a binary (debugserver) based on lldb
which is subject to the following license:

University of Illinois/NCSA
Open Source License

Copyright (c) 2010 Apple Inc.
All rights reserved.

Developed by:

LLDB Team

http://lldb.llvm.org/

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal with
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

* Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimers.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimers in the
documentation and/or other materials provided with the distribution.

* Neither the names of the LLDB Team, copyright holders, nor the names of
its contributors may be used to endorse or promote products derived from
this Software without specific prior written permission.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE
SOFTWARE.
