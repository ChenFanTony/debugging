ftrace and systemtap
----------------------------
## 1. Using qemu debug Linux core
   references: https://consen.github.io/2018/01/17/debug-linux-kernel-with-qemu-and-gdb/
## 2. Compiling a minimum filesystem busybox
    https://busybox.net/downloads/busybox-1.31.0.tar.bz2
   1) decompress it, similar to kernel
   
      make menuconfig
      Configure Busybox according the following:
      Busybox Settings ---> Build Options ---> Build BusyBox as a static binary (no shared libs) ---> yes
      need yum install glibc-static
      make 
      make CONFIG_PREFIX=`pwd`/ROOT install
   2) create dir

       1) mkdir etc dev mnt
       2) mkdir -p etc/init.d
       3) cd init.d

    vim rcS
   ------------
    mkdir -p /proc
    mkdir -p /tmp
    mkdir -p /sys
    mkdir -p /mnt
    mount -a
    mkdir -p /dev/pts
    mount -t devpts devpts /dev/pts
    #echo /sbin/mdev > /proc/sys/kernel/hotplug
    mdev -s

   chmod +x rcS
   
   3)  etc/fstab
   
    proc /proc proc defaults 0 0
    tempfs /tmp tmpfs defaults 0 0
    sysfs /sys sysfs defaults 0 0
    tempfs /dev tempfs defaults 0 0
    debugfs /sys/kernel/debug debugfs defaults 0 0

  4) etc/inittab

    proc /proc proc defaults 0 0
    tempfs /tmp tmpfs defaults 0 0
    sysfs /sys sysfs defaults 0 0
    tempfs /dev tempfs defaults 0 0
    debugfs /sys/kernel/debug debugfs defaults 0 0

cd dev
mknod console c 5 1
mknod null c 1 3

## 3. compiling kernel
  1) modify initramfs source = busybox_root / using initramfs cpio
  2) Enable CONFIG_GDB_SCRIPTS , disable CONFIG_RANDOMIZE_BASE
  3) save to .config
  4) make bzImage

  initramfs:
     find . -print0 | cpio --null -ov --format=newc | gzip -9 > ../initramfs.cpio.gz


## 4. using qemu to debug
  qemu-system-x86_64 --nographic -m 1024 -kernel linux/arch/x86_64/boot/bzImage -initrd initramfs.cpio.gz -append "rdinit=/linuxrc console=ttyS0 loglevel=8" -S -s

## 5. gdb
    shoud enable python support / reference part 6:
    by source:
    git clone git://sourceware.org/git/binutils-gdb.git
    then compiling it

    mkdir BUILD
    cd BUILD
    ../configure --with-python (need install python2-devel gcc-c++
    make -j8

    by install: install python2-debuginfo

    vim ~/.gdbinit
    add-auto-load-safe-path /root/data/src/linux/scripts/gdb/vmlinux-gdb.py

    cd linux && gdb --tui vmlinux
    (gdb) target remote localhost:1234
    (gdb) hb start_kernel
    (gdb) c
    (gdb) apropos lx (support lx add symbos .etc)

## 6. support gdb python scripts

  You need to have gdb on your system and Python debugging extensions. Extensions package includes debugging symbols and adds

  Python-specificcommands into gdb. On a modern Linux system, you can easily install these with:

  Fedora:
  
    sudo yum install gdb python2-debuginfo 
    
  Ubuntu:
  
    sudo apt-get install gdb python2.7-dbg 
    
  Centos*:
  
    sudo yum install yum-utils
    sudo debuginfo-install glibc
    sudo yum install gdb python2-debuginfo 
    
  tested on Centos 7. python-debuginfo is installable after the first two commands.
  
  For gdb support on legacy systems, look at the end of this page. 
