Summary
----------------------------
## 1. Using qemu debug Linux core
   references: https://consen.github.io/2018/01/17/debug-linux-kernel-with-qemu-and-gdb/
   images URLs(x86,ARM):
     https://cloud.centos.org/centos/7/images/
     https://cloud-images.ubuntu.com/
### 1.1 cross_compile aarch64 on x86
   1). download gcc-aarch64-linux-gnu
```
      wget --no-check-certificate https://releases.linaro.org/components/toolchain/binaries/latest-7/aarch64-linux-gnu/gcc-linaro-7.5.0-2019.12-x86_64_aarch64-linux-gnu.tar.xz
```
   2). decompress it and copy to binary directory
```
      tar xvf X && cp -rf gcc-linaro-7.5.0-2019.12-X /usr/bin/
```
   3). add aarch64 path environment
```
      export PATH=$PATH:/usr/bin/toolchain/gcc-linaro-7.5.0-2019.12-x86_64_aarch64-linux-gnu/bin/
```
   4). cross_compile
```
      make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- XXX
```

## 2. Manual compiling a minimum filesystem busybox
    https://busybox.net/downloads/busybox-1.31.0.tar.bz2
   1) decompress it, we will observe that its hierarchy dir similar to kernel dir
```
      // aarch64 need specify ARCH and CORSS_COMPILE
      make menuconfig
      Configure Busybox according the following:
      Busybox Settings ---> Build Options ---> Build BusyBox as a static binary (no shared libs) ---> yes
      need yum install glibc-static
      make 
      make CONFIG_PREFIX=`pwd`/ROOT install
```
   2) create dir

       1) mkdir etc
       2) mkdir -p etc/init.d
       3) cd init.d
       4) vim rcS

```
    mkdir -p /proc
    mkdir -p /tmp
    mkdir -p /sys
    mkdir -p /mnt
    mount -a
    mkdir -p /dev/pts
    mount -t devpts devpts /dev/pts
    #echo /sbin/mdev > /proc/sys/kernel/hotplug
    mdev -s
```
   chmod +x rcS
   
   3)  etc/fstab
```
proc /proc proc defaults 0 0
tempfs /tmp tmpfs defaults 0 0
sysfs /sys sysfs defaults 0 0
tempfs /dev tempfs defaults 0 0
debugfs /sys/kernel/debug debugfs defaults 0 0
```
  4) etc/inittab
```
cd dev
mknod console c 5 1
mknod null c 1 3
```
  5) init
refer to the init file

  6) packing initramfs:
```
     cd sample/busybox_root/
     find . -print0 | cpio --null -ov --format=newc | gzip -9 > ../initramfs.cpio.gz
```

## 3. compiling kernel
make menuconfig(all distros):
  1) enable CONFIG_DEBUG_INFO(DEBUG_INFO_DWARF_TOOLCHAIN_DEFAULT), CONFIG_GDB_SCRIPTS,
     disable CONFIG_RANDOMIZE_BASE(KASLR)/or cmdline + nokaslr
     for eBPF, enable CONFIG_BPF=y CONFIG_BPF_SYSCALL=y CONFIG_BPF_JIT=y CONFIG_HAVE_EBPF_JIT=y
     CONFIG_BPF_EVENTS=y CONFIG_KPROBES=y CONFIG_UPROBES=y 
  2) save to .config
  3) make bzImage (-jCPUNUM)/make vmlinux
  4) for support ebpf, need to build tools/bpf (for support BPF, need CONFIG_DEBUG_INFO_BTF=y)
     make tools/bpf

cross-compile building arm in x86(Only Ubuntu supported):
  1) apt-get install -y gcc-aarch64-linux-gnu libssl-dev build-essential git libncurses5-dev
  2) export ARCH=arm64
  3) export CROSS_COMPILE=aarch64_linux-gnu-
  4) make defconfig
  5) make Image (-jCPUNUM)

## 4. using qemu to debug
for X86:
```
  qemu-system-x86_64 --nographic -m 1024 -kernel linux/arch/x86_64/boot/bzImage -initrd initramfs.cpio.gz -append "rdinit=/linuxrc console=ttyS0 loglevel=8" -S -s
```

for ARM:
```
  qemu-system-aarch64 -nographic -full-screen -M virt -cpu cortex-a57 -m 4096 -smp 4 --bios QEMU_EFI.fd -kernel arm/Image -append "console=ttyAMA0 loglevel=8" -S -s
```

## 5. gdb
  shoud enable python support:
  1) manual by source:
  git clone git://sourceware.org/git/binutils-gdb.git
  then compiling it
```
    mkdir BUILD
    cd BUILD
    ../configure --with-python (need install python2-devel gcc-c++)
    make -j8
```
  2) install gdb:
  install python-debuginfo
  vim ~/.gdbinit
```    
    #python gdb.COMPLETE_EXPRESSION = gdb.COMPLETE_SYMBOL
    #add-auto-load-safe-path /root/data/src/linux/scripts/gdb/vmlinux-gdb.py

    #for extend to support C++ std::
    # wget https://scaron.info/files/gdbinit 
    add `source gdbinit` to ~/.gdbinit
```
  3) debug:
  cd linux && gdb --tui vmlinux
```
   gdb --tui vmlinux
  (gdb) target remote localhost:1234
  (gdb) c
  (gdb) apropos lx
```
  for cross compiling aarch64 on Ubuntu
```
  apt-get install gdb-multiarch
   gdb-multiarch --tui vmlinux
  (gdb) set architecture aarch64
  (gdb) target remote localhost:1234
  (gdb) hb start_kernel
  (gdb) c
  (gdb) apropos lx (support lx add symbos .etc)
```

## 6. support gdb python scripts

  You need to have gdb on your system and Python debugging extensions. Extensions package includes debugging symbols and adds

  Python-specificcommands into gdb. On a modern Linux system, you can easily install these with:

  Fedora:
```  
    sudo yum install gdb python2-debuginfo 
```    
  Ubuntu:
```  
    sudo apt-get install gdb python2.7-dbg 
```    
  Centos*:
```  
#    sudo yum install yum-utils
#    sudo debuginfo-install glibc
    sudo yum install gdb python-debuginfo 
```    
  tested on Centos 7. python-debuginfo is installable after the first two commands.
  
## 7. debug compiling modules
   1) build single module
```
     make modules_prepare
     make -j4 M=drivers/virtio modules
     ## also can specify the config flag to compile
     ## like: make -C /lib/modules/XX/build CONFIG_BCACHE=m M=`pwd`/drivers/md/bcache modules
     mkdir /tmp/staging
     make M=drivers/virtio INSTALL_MOD_PATH=/tmp/staging modules_install

     if we want to disable gcc optimization, add cflags in module Makefile:
     KBUILD_CFLAGS += -Og
```
   2) build all modules (not work)
```
     make modules 
     make INSTALL_MOD_PATH=/tmp/staging modules_install
```
   3) we can easily copy the module to initramfs
```
     cp -r /tmp/staging/* busybox_root/
```

## 8. add modules support for qemu + 9p
   1) Compile qemu with enable-virtfs:
```
x86:
    ./configure --target-list=x86_64-softmmu --enable-virtfs
arm:
    ./configure --target-list=aarch64-softmmu --enable-virtfs
```
   2) kernel enable features:
```
     CONFIG_NET_9P=y
     CONFIG_9P_FS=y
     CONFIG_VIRTIO_PCI=y
     CONFIG_NET_9P_VIRTIO=y
     CONFIG_9P_FS_POSIX_ACL=y
     CONFIG_NET_9P_DEBUG=y (Optional)
```
   3) qemu boot args:
```   
     -fsdev local,security_model=passthrough,id=fsdev0,path=/tmp/share -device virtio-9p-pci,id=fs0,fsdev=fsdev0,mount_tag=hostshare
```
   4) now we can copy modules to /tmp/share and in guest mount the share directorys
```
     mount -t 9p -o trans=virtio,version=9p2000.L hostshare /tmp/host_files
```

## 9. create a cloud-init iso for logining in a cloud image. sometimes we need a whole OS.
   1) generate the cloud-init iso:
```
    # the default user name for different distro is centos/fedora/etc.
    cd cloud-init
    genisoimage -output cloud-init.iso -volid cidata -joliet -rock user-data meta-data
```
   2) kernel enable features:
```
      CONFIG_XFS_FS=y
      CONFIG_VIRTIO_BLK=y
```
   3) download the cloud images for centos:
```
      wget http://cloud.centos.org/centos/7/images/CentOS-7-x86_64-GenericCloud-1907.qcow2.xz
```

##NOTES:
  1) the whole qemu comdline:
```
x86:
  ./qemu/x86_64-softmmu/qemu-system-x86_64 --nographic -m 1024 -smp 2 --enable-kvm \
  -kernel kernel/linux/arch/x86_64/boot/bzImage \
  -initrd kernel/debugging/samples/initramfs.cpio.gz \
  #using rdinit=/linuxrc only can be used in initramfs because of linuxrc call etc/init.d/rcS which pid is not 1,
  #using init=/init to switch root to boot cloud image
  #we also can use systemd.mask to mask service to reduce the boot time systemd.mask=gssproxy.service
  #systemd.mask=systemd-networkd.service systemd.mask=systemd-networkd.socket
  -append "rdinit=/linuxrc console=ttyS0 loglevel=8 cloud-init=disabled nokaslr" \
  -s -S -fsdev local,security_model=passthrough,id=fsdev0,path=./share \
  -device virtio-9p-pci,id=fs0,fsdev=fsdev0,mount_tag=hostshare \
  -drive file=/home/chenfan/images/CentOS-7-x86_64-GenericCloud-1907.qcow2,if=virtio \
  -cdrom kernel/debugging/cloud-init/cloud-init.iso

arm:
  # download bios bin or get a copy one in samples directory
  wget --no-check-certificate https://releases.linaro.org/components/kernel/uefi-linaro/latest/release/qemu64/QEMU_EFI.fd

  ./qemu/aarch64-softmmu/qemu-system-aarch64 -nographic -full-screen -M virt -cpu cortex-a57 -m 4096 -smp 4 \
  -kernel arm/Image \
  -initrd kernel/debugging/samples/initramfs-arm.cpio.gz \
  -append "console=ttyAMA0 loglevel=8 cloud-init=disabled nokaslr systemd.mask=systemd-networkd.service systemd.mask=systemd-networkd.socket systemd.mask=gssproxy.service" \
  -chardev socket,id=charmonitor,path=/tmp/monitor.sock,server,nowait -mon chardev=charmonitor,id=monitor \
  -fsdev local,security_model=passthrough,id=fsdev0,path=./share \
  -device virtio-9p-pci,id=fs0,fsdev=fsdev0,mount_tag=hostshare \
  -drive file=/home/chenfan/data/ssd/images/CentOS-7-aarch64-GenericCloud-2009.qcow2,if=none,id=drive_disk0 \
  -device virtio-blk-pci,drive=drive_disk0,id=virtio-blk0,scsi=off \
  -cdrom kernel/debugging/cloud-init/cloud-init.iso \
  -nic tap,model=e1000,mac=52:54:00:12:34:56
```

  2) kernel init process
   cmdline "init=" Run specified binary instead of /sbin/init as init process.
   cmdline "rdinit=" Run specified binary instead of /init from the ramdisk,
                        used for early userspace startup. See initrd.

   kernel init order by default:
```
     /sbin/init, /etc/init, /bin/init, /bin/sh
```
  3) configuration VM network
   qemu add comand line: -nic tap,model=e1000
  need to create file and add executable permission:
```
$ cat /etc/qemu-ifup 
#!/bin/sh

echo "Executing /etc/qemu-ifup"
echo "Bringing up $1 for bridged mode..."
sudo /usr/sbin/ip link set $1 up promisc on
echo "Adding $1 to br0..."
sudo /usr/sbin/brctl addif virbr0 $1
sleep 2

$ cat /etc/qemu-ifdown
#!/bin/sh

echo "Executing /etc/qemu-ifdown"
sudo /usr/sbin/ip link set $1 down
sudo /usr/sbin/brctl delif virbr0 $1
sudo /usr/sbin/ip link delete dev $1

```
