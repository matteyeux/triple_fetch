triple_fetch - ianbeer

This is an exploit for CVE-2017-7047, a logic error in libxpc which allowed
malicious message senders to send xpc_data objects that were backed by shared memory.
Consumers of xpc messages did not seem to expect that the backing buffers of xpc_data objects
could be modified by the sender whilst being processed by the receiver.

This project exploits CVE-2017-7047 to build a proof-of-concept remote lldb debugserver
stub capable of attaching to and allowing the remote debugging all userspace
processes on iOS 10.0 to 10.3.2.

Please see the README in the nsxpc2pc folder in the attached archive for further discussion and details.
