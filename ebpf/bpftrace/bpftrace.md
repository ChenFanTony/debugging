### bpftrace

#### bpftrace vs systemtap

The underlying kernel infrastructure used by both SystemTap and
bpftrace is largely the same: KProbes, for dynamically tracing
kernel functions, tracepoints for static kernel instrumentation,
Uprobes for dynamic instrumentation of user-level functions,
and user-level statically defined tracing (USDT) for static user-space
instrumentation. Both systems allow instrumenting the kernel
and user-space programs through a "script" in a high-level
language that can be used to specify what needs to be probed and how.

The important design distinction between the two is that SystemTap
translates the user-supplied script into C code, which is then
compiled and loaded as a module into a running Linux kernel.
Instead, bpftrace converts the script to LLVM intermediate representation,
which is then compiled to BPF. Using BPF has several advantages:
creating and running a BPF program is significantly faster than
building and loading a kernel module. Support for data structures
consisting of key/value pairs can be easily added by using BPF maps.
The BPF verifier ensures that BPF programs will not cause the system
to crash, while the kernel module approach used by SystemTap implies
the need for implementing various safety checks in the runtime.
On the other hand, using BPF makes certain features hard to implement,
for example, a custom stack walker, as we shall see later in the article.

#### installation

SystemTap requires the Linux kernel headers to be installed in order to work,
while bpftrace does not. For user-space software, both SystemTap and
bpftrace require the debugging symbols of the software under examination.

For kernel instrumentation, SystemTap requires the kernel debugging symbols
to be installed in order to use the advanced features of the tool, such as
looking up the arguments or local variables of a function, as well as instrumenting
specific lines of code within the function body. while bpftrace does not.

#### features

An important problem affecting bpftrace is that it cannot generate user-space stack traces,
unless the program being traced was built with frame pointers.

For kernel, bpftrace can use "print(kstack)".

Another important advantage of SystemTap over bpftrace is that it allows
accessing function arguments and local variables by their name. With bpftrace,
arguments can only be accessed by name when instrumenting the kernel, and
specifically when using static kernel tracepoints or the experimental kfunc
feature that is available for recent kernels. 

#### conclusion

BPF is an obviously good choice for tracing tools, as it provides a fast and
safe environment to base observability tools on.
On the other hand, SystemTap provides several distinguishing features such as:
generating user-space backtraces without the need for frame pointers, accessing
function arguments and local variables by name, and the ability to probe arbitrary
statements. Both would seem to have their place for diagnosing problems in today's
Linux systems.

#### bpftool: examination program
use bpftool can exam prog in BPF, which can also help to manager BPF map and object.
