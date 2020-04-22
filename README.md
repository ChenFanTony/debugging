ftrace and systemtap
----------------------------
## 1. Using qemu debug Linux core
   references: https://consen.github.io/2018/01/17/debug-linux-kernel-with-qemu-and-gdb/
## 2. Manual compiling a minimum filesystem busybox
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


cd dev
mknod console c 5 1
mknod null c 1 3

## 3. compiling kernel
  1) modify initramfs source = busybox_root / using initramfs cpio
  2) Enable CONFIG_GDB_SCRIPTS , disable CONFIG_RANDOMIZE_BASE(KASLR)/or cmdline + nokaslr
  3) save to .config
  4) make bzImage

  initramfs:
     cd sample/busybox_root/
     find . -print0 | cpio --null -ov --format=newc | gzip -9 > ../initramfs.cpio.gz


## 4. using qemu to debug
  qemu-system-x86_64 --nographic -m 1024 -kernel linux/arch/x86_64/boot/bzImage -initrd initramfs.cpio.gz -append "rdinit=/linuxrc console=ttyS0 loglevel=8" -S -s

## 5. gdb
    shoud enable python support:
    1) manual by source:
    git clone git://sourceware.org/git/binutils-gdb.git
    then compiling it

    mkdir BUILD
    cd BUILD
    ../configure --with-python (need install python2-devel gcc-c++)
    make -j8

    2) by install:
    install python-debuginfo
    vim ~/.gdbinit
    #python gdb.COMPLETE_EXPRESSION = gdb.COMPLETE_SYMBOL
    #add-auto-load-safe-path /root/data/src/linux/scripts/gdb/vmlinux-gdb.py

    #for extend to support C++ std::
    # wget https://scaron.info/files/gdbinit 
    add `source gdbinit` to ~/.gdbinit

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
  
#    sudo yum install yum-utils
#    sudo debuginfo-install glibc
    sudo yum install gdb python-debuginfo 
    
  tested on Centos 7. python-debuginfo is installable after the first two commands.
  
## 7. debug compiling modules
   1) build single module

     make modules_prepare
     make -j4 M=drivers/virtio modules
     ## also can specify the config flag to compile
     ## like: make -C /lib/modules/XX/build CONFIG_BCACHE=m M=`pwd`/drivers/md/bcache modules
     mkdir /tmp/staging
     make M=drivers/virtio INSTALL_MOD_PATH=/tmp/staging modules_install

     if we want to disable gcc optimization, add cflags in module Makefile:
     KBUILD_CFLAGS += -Og

   2) build all modules (not work)

     make modules 
     make INSTALL_MOD_PATH=/tmp/staging modules_install

   3) we can easily copy the modules to initramfs

     cp -r /tmp/staging/* busybox_root/

## 8. add modules support for qemu + 9p
   1) Compile qemu with enable-virtfs:
    ./configure --target-list=x86_64-softmmu --enable-virtfs

   2) kernel enable features:
     CONFIG_NET_9P=y
     CONFIG_9P_FS=y
     CONFIG_VIRTIO_PCI=y
     CONFIG_NET_9P_VIRTIO=y
     CONFIG_9P_FS_POSIX_ACL=y
     CONFIG_NET_9P_DEBUG=y (Optional)

   3) qemu boot args:
     -fsdev local,security_model=passthrough,id=fsdev0,path=/tmp/share -device virtio-9p-pci,id=fs0,fsdev=fsdev0,mount_tag=hostshare /
     -fsdev local,security_model=passthrough,id=fsdev0,path=/tmp/share,mount_tag=hostshare

   4) now we can copy modules to /tmp/share and in guest mount the share directorys
     mount -t 9p -o trans=virtio,version=9p2000.L hostshare /tmp/host_files

## 9. create a cloud-init iso for logining in a cloud image. sometimes we need a whole OS.
   1) generate the cloud-init iso:
    # the default user name for different distro is centos/fedora/etc.
    cd cloud-init
    genisoimage -output cloud-init.iso -volid cidata -joliet -rock user-data meta-data

   2) kernel enable features:
      CONFIG_XFS_FS=y
      CONFIG_VIRTIO_BLK=y

   3) download the cloud images for centos:
      wget http://cloud.centos.org/centos/7/images/CentOS-7-x86_64-GenericCloud-1907.qcow2.xz


##NOTES:
  1) the whole qemu comdline:
    
  ./qemu/x86_64-softmmu/qemu-system-x86_64 --nographic -m 1024 -smp 2 --enable-kvm \
  -kernel kernel/linux/arch/x86_64/boot/bzImage \
  -initrd kernel/debugging/samples/initramfs.cpio.gz \
# using rdinit=/linuxrc only can be used in initramfs because of linuxrc call etc/init.d/rcS which pid is not 1,
# using init=/init to switch root to boot cloud image
# we also can use systemd.mask to mask service to reduce the boot time systemd.mask=gssproxy.service
# systemd.mask=systemd-networkd.service systemd.mask=systemd-networkd.socket
  -append "rdinit=/linuxrc console=ttyS0 loglevel=8 cloud-init=disabled nokaslr" \
  -s -S -fsdev local,security_model=passthrough,id=fsdev0,path=./share \
  -device virtio-9p-pci,id=fs0,fsdev=fsdev0,mount_tag=hostshare \
  -drive file=/home/chenfan/images/CentOS-7-x86_64-GenericCloud-1907.qcow2,if=virtio \
  -cdrom kernel/debugging/cloud-init/cloud-init.iso


  2) kernel init process
   cmdline "init=" Run specified binary instead of /sbin/init as init process.
   cmdline "rdinit=" Run specified binary instead of /init from the ramdisk,
                        used for early userspace startup. See initrd.

   kernel init order by default:
     /sbin/init, /etc/init, /bin/init, /bin/sh
