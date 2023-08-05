# 在xv6操作系统中，总文件夹下的Makefile是一个用于自动化构建系统的文件。
# Makefile文件描述了如何编译和构建xv6操作系统内核以及用户程序。
# 通过运行make命令，Makefile会根据定义的规则和依赖关系来编译源代码，并生成可执行文件和内核映像。
# Makefile包含了一系列的规则，每个规则定义了如何生成一个目标文件。这些规则指定了目标文件的依赖关系和生成方式。
# 当运行make命令时，它会查看每个目标文件的依赖关系，并按照规则来构建目标文件。
# 如果某个目标文件的依赖文件已经更新，那么make会重新编译该目标文件。

# To compile and run with a lab solution, set the lab name in lab.mk
# (e.g., LAB=util).  Run make grade to test solution with the lab's
# grade script (e.g., grade-lab-util).
# 要使用实验室解决方案编译和运行，请在lab.mk中设置实验室名称（例如LAB=util）。使用实验室的grade脚本（例如，grade-lab-util）。

-include conf/lab.mk

K=kernel
U=user

# 定义了一个名为OBJS的变量，该变量包含了一系列文件对象的路径。
# 每个对象文件表示内核的一个组件或模块，这些组件将在后续的编译和链接过程中用于构建xv6内核。
# $K/entry.o将被展开为kernel/entry.o，以此类推。
OBJS = \
  $K/entry.o \          # 内核入口代码，用于从引导加载程序进入内核。
  $K/kalloc.o \         # 内核内存分配器。
  $K/string.o \         # 字符串处理函数。
  $K/main.o \           # 内核的主要部分，包含初始化和其他系统任务。
  $K/vm.o \             # 虚拟内存管理。
  $K/proc.o \           # 进程管理。
  $K/swtch.o \          # 内核线程上下文切换。
  $K/trampoline.o \     # 处理系统调用的入口。
  $K/trap.o \           # 中断和异常处理。
  $K/syscall.o \        # 系统调用处理。
  $K/sysproc.o \        # 处理系统调用的辅助函数。
  $K/bio.o \            # 块I/O层，处理块设备的读写操作。
  $K/fs.o \             # 文件系统。
  $K/log.o \            # 文件系统日志（log）的实现。
  $K/sleeplock.o \      # 睡眠锁，用于进程同步。
  $K/file.o \           # 文件操作。
  $K/pipe.o \           # 管道实现。
  $K/exec.o \           # 进程执行新程序。
  $K/sysfile.o \        # 与文件系统相关的系统调用处理函数。
  $K/kernelvec.o \      # 内核向量表。
  $K/plic.o \           # 外部中断控制器。
  $K/virtio_disk.o      # 虚拟磁盘驱动。

# 定义了一个名为 OBJS_KCSAN 的变量，它包含了一系列文件对象的路径。
# 这些对象文件用于在启用了 KCSAN（Kernel Concurrency Sanitizer）时构建 xv6 内核。
OBJS_KCSAN = \          # 内核的启动代码，初始化内核的运行环境。
  $K/start.o \          # 控制台输出函数的实现，用于向终端输出字符。
  $K/console.o \        # 格式化输出函数 printf 的实现，支持将格式化的字符串打印到终端。
  $K/printf.o \         # 与串口通信（UART）相关的实现，用于与硬件串口进行通信。
  $K/uart.o \           # 与串口通信（UART）相关的实现，用于与硬件串口进行通信。
  $K/spinlock.o         # 自旋锁的实现，用于实现简单的同步机制，保护共享数据免受并发访问的干扰。

OBJS_KCSAN = \          # 内核的启动代码，初始化内核的运行环境。
  $K/start.o \          # 控制台输出函数的实现，用于向终端输出字符。
  $K/console.o \        # 格式化输出函数 printf 的实现，支持将格式化的字符串打印到终端。
  $K/printf.o \         # 与串口通信（UART）相关的实现，用于与硬件串口进行通信。
  $K/uart.o \           # 与串口通信（UART）相关的实现，用于与硬件串口进行通信。
  $K/spinlock.o         # 自旋锁的实现，用于实现简单的同步机制，保护共享数据免受并发访问的干扰。


# 条件编译
# ifdef 检查是否定义了 KCSAN 变量，如果定义了，则执行 ifdef 和 endif 之间的代码块。
# 如果 KCSAN 变量被定义了，会将 $K/kcsan.o 添加到 OBJS_KCSAN 变量中。
ifdef KCSAN
OBJS_KCSAN += \
	$K/kcsan.o
endif

# ifeq 检查 LAB 变量是否等于 "lock"，如果相等，则执行 ifeq 和 endif 之间的代码块。
# 如果 LAB 变量的值是 "lock"，则会将 $K/stats.o 和 $K/sprintf.o 添加到 OBJS 变量中。
ifeq ($(LAB),$(filter $(LAB), lock))
OBJS += \
	$K/stats.o\
	$K/sprintf.o
endif

# ifeq 检查 LAB 变量是否等于 "net"，如果相等，则执行 ifeq 和 endif 之间的代码块。
# 如果 LAB 变量的值是 "net"，则会将 $K/e1000.o、$K/net.o、$K/sysnet.o 和 $K/pci.o 添加到 OBJS 变量中。
ifeq ($(LAB),net)
OBJS += \
	$K/e1000.o \
	$K/net.o \
	$K/sysnet.o \
	$K/pci.o
endif


# riscv64-unknown-elf- or riscv64-linux-gnu-
# perhaps in /opt/riscv/bin
#TOOLPREFIX = 

# Try to infer the correct TOOLPREFIX if not set
ifndef TOOLPREFIX
TOOLPREFIX := $(shell if riscv64-unknown-elf-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-unknown-elf-'; \
	elif riscv64-linux-gnu-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-linux-gnu-'; \
	elif riscv64-unknown-linux-gnu-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-unknown-linux-gnu-'; \
	else echo "***" 1>&2; \
	echo "*** Error: Couldn't find a riscv64 version of GCC/binutils." 1>&2; \
	echo "*** To turn off this error, run 'gmake TOOLPREFIX= ...'." 1>&2; \
	echo "***" 1>&2; exit 1; fi)
endif

QEMU = qemu-system-riscv64

CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)gas
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

CFLAGS = -Wall -Werror -O -fno-omit-frame-pointer -ggdb

ifdef LAB
LABUPPER = $(shell echo $(LAB) | tr a-z A-Z)
XCFLAGS += -DSOL_$(LABUPPER) -DLAB_$(LABUPPER)
endif

CFLAGS += $(XCFLAGS)
CFLAGS += -MD
CFLAGS += -mcmodel=medany
CFLAGS += -ffreestanding -fno-common -nostdlib -mno-relax
CFLAGS += -I.
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)

ifeq ($(LAB),net)
CFLAGS += -DNET_TESTS_PORT=$(SERVERPORT)
endif

ifdef KCSAN
CFLAGS += -DKCSAN
KCSANFLAG = -fsanitize=thread
endif

# Disable PIE when possible (for Ubuntu 16.10 toolchain)
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]no-pie'),)
CFLAGS += -fno-pie -no-pie
endif
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]nopie'),)
CFLAGS += -fno-pie -nopie
endif

LDFLAGS = -z max-page-size=4096

$K/kernel: $(OBJS) $(OBJS_KCSAN) $K/kernel.ld $U/initcode
	$(LD) $(LDFLAGS) -T $K/kernel.ld -o $K/kernel $(OBJS) $(OBJS_KCSAN)
	$(OBJDUMP) -S $K/kernel > $K/kernel.asm
	$(OBJDUMP) -t $K/kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $K/kernel.sym

$(OBJS): EXTRAFLAG := $(KCSANFLAG)

$K/%.o: $K/%.c
	$(CC) $(CFLAGS) $(EXTRAFLAG) -c -o $@ $<


$U/initcode: $U/initcode.S
	$(CC) $(CFLAGS) -march=rv64g -nostdinc -I. -Ikernel -c $U/initcode.S -o $U/initcode.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o $U/initcode.out $U/initcode.o
	$(OBJCOPY) -S -O binary $U/initcode.out $U/initcode
	$(OBJDUMP) -S $U/initcode.o > $U/initcode.asm

tags: $(OBJS) _init
	etags *.S *.c

ULIB = $U/ulib.o $U/usys.o $U/printf.o $U/umalloc.o

ifeq ($(LAB),$(filter $(LAB), lock))
ULIB += $U/statistics.o
endif

_%: %.o $(ULIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $@ $^
	$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym

$U/usys.S : $U/usys.pl
	perl $U/usys.pl > $U/usys.S

$U/usys.o : $U/usys.S
	$(CC) $(CFLAGS) -c -o $U/usys.o $U/usys.S

$U/_forktest: $U/forktest.o $(ULIB)
	# forktest has less library code linked in - needs to be small
	# in order to be able to max out the proc table.
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $U/_forktest $U/forktest.o $U/ulib.o $U/usys.o
	$(OBJDUMP) -S $U/_forktest > $U/forktest.asm

mkfs/mkfs: mkfs/mkfs.c $K/fs.h $K/param.h
	gcc $(XCFLAGS) -Werror -Wall -I. -o mkfs/mkfs mkfs/mkfs.c

# Prevent deletion of intermediate files, e.g. cat.o, after first build, so
# that disk image changes after first build are persistent until clean.  More
# details:
# http://www.gnu.org/software/make/manual/html_node/Chained-Rules.html
.PRECIOUS: %.o

# 定义了一个名为 UPROGS 的变量，包含了一系列用户程序的路径。
UPROGS=\
	$U/_cat\
	$U/_echo\
	$U/_forktest\
	$U/_grep\
	$U/_init\
	$U/_kill\
	$U/_ln\
	$U/_ls\
	$U/_mkdir\
	$U/_rm\
	$U/_sh\
	$U/_stressfs\
	$U/_usertests\
	$U/_grind\
	$U/_wc\
	$U/_zombie\




ifeq ($(LAB),$(filter $(LAB), lock))
UPROGS += \
	$U/_stats
endif

ifeq ($(LAB),traps)
UPROGS += \
	$U/_call\
	$U/_bttest
endif

ifeq ($(LAB),lazy)
UPROGS += \
	$U/_lazytests
endif

ifeq ($(LAB),cow)
UPROGS += \
	$U/_cowtest
endif

ifeq ($(LAB),thread)
UPROGS += \
	$U/_uthread

$U/uthread_switch.o : $U/uthread_switch.S
	$(CC) $(CFLAGS) -c -o $U/uthread_switch.o $U/uthread_switch.S

$U/_uthread: $U/uthread.o $U/uthread_switch.o $(ULIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $U/_uthread $U/uthread.o $U/uthread_switch.o $(ULIB)
	$(OBJDUMP) -S $U/_uthread > $U/uthread.asm

ph: notxv6/ph.c
	gcc -o ph -g -O2 $(XCFLAGS) notxv6/ph.c -pthread

barrier: notxv6/barrier.c
	gcc -o barrier -g -O2 $(XCFLAGS) notxv6/barrier.c -pthread
endif

ifeq ($(LAB),pgtbl)
UPROGS += \
	$U/_pgtbltest
endif

ifeq ($(LAB),lock)
UPROGS += \
	$U/_kalloctest\
	$U/_bcachetest
endif

ifeq ($(LAB),fs)
UPROGS += \
	$U/_bigfile
endif



ifeq ($(LAB),net)
UPROGS += \
	$U/_nettests
endif

UEXTRA=
ifeq ($(LAB),util)
	UEXTRA += user/xargstest.sh
endif


fs.img: mkfs/mkfs README $(UEXTRA) $(UPROGS)
	mkfs/mkfs fs.img README $(UEXTRA) $(UPROGS)

-include kernel/*.d user/*.d

clean: 
	rm -f *.tex *.dvi *.idx *.aux *.log *.ind *.ilg \
	*/*.o */*.d */*.asm */*.sym \
	$U/initcode $U/initcode.out $K/kernel fs.img \
	mkfs/mkfs .gdbinit \
        $U/usys.S \
	$(UPROGS) \
	ph barrier

# try to generate a unique GDB port
GDBPORT = $(shell expr `id -u` % 5000 + 25000)
# QEMU's gdb stub command line changed in 0.11
QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
	then echo "-gdb tcp::$(GDBPORT)"; \
	else echo "-s -p $(GDBPORT)"; fi)
ifndef CPUS
CPUS := 3
endif
ifeq ($(LAB),fs)
CPUS := 1
endif

FWDPORT = $(shell expr `id -u` % 5000 + 25999)

QEMUOPTS = -machine virt -bios none -kernel $K/kernel -m 128M -smp $(CPUS) -nographic
QEMUOPTS += -drive file=fs.img,if=none,format=raw,id=x0
QEMUOPTS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

ifeq ($(LAB),net)
QEMUOPTS += -netdev user,id=net0,hostfwd=udp::$(FWDPORT)-:2000 -object filter-dump,id=net0,netdev=net0,file=packets.pcap
QEMUOPTS += -device e1000,netdev=net0,bus=pcie.0
endif

qemu: $K/kernel fs.img
	$(QEMU) $(QEMUOPTS)

.gdbinit: .gdbinit.tmpl-riscv
	sed "s/:1234/:$(GDBPORT)/" < $^ > $@

qemu-gdb: $K/kernel .gdbinit fs.img
	@echo "*** Now run 'gdb' in another window." 1>&2
	$(QEMU) $(QEMUOPTS) -S $(QEMUGDB)

ifeq ($(LAB),net)
# try to generate a unique port for the echo server
SERVERPORT = $(shell expr `id -u` % 5000 + 25099)

server:
	python3 server.py $(SERVERPORT)

ping:
	python3 ping.py $(FWDPORT)
endif

##
##  FOR testing lab grading script
##

ifneq ($(V),@)
GRADEFLAGS += -v
endif

print-gdbport:
	@echo $(GDBPORT)

grade:
	@echo $(MAKE) clean
	@$(MAKE) clean || \
          (echo "'make clean' failed.  HINT: Do you have another running instance of xv6?" && exit 1)
	./grade-lab-$(LAB) $(GRADEFLAGS)

##
## FOR web handin
##


WEBSUB := https://6828.scripts.mit.edu/2021/handin.py

handin: tarball-pref myapi.key
	@SUF=$(LAB); \
	curl -f -F file=@lab-$$SUF-handin.tar.gz -F key=\<myapi.key $(WEBSUB)/upload \
	    > /dev/null || { \
		echo ; \
		echo Submit seems to have failed.; \
		echo Please go to $(WEBSUB)/ and upload the tarball manually.; }

handin-check:
	@if ! test -d .git; then \
		echo No .git directory, is this a git repository?; \
		false; \
	fi
	@if test "$$(git symbolic-ref HEAD)" != refs/heads/$(LAB); then \
		git branch; \
		read -p "You are not on the $(LAB) branch.  Hand-in the current branch? [y/N] " r; \
		test "$$r" = y; \
	fi
	@if ! git diff-files --quiet || ! git diff-index --quiet --cached HEAD; then \
		git status -s; \
		echo; \
		echo "You have uncomitted changes.  Please commit or stash them."; \
		false; \
	fi
	@if test -n "`git status -s`"; then \
		git status -s; \
		read -p "Untracked files will not be handed in.  Continue? [y/N] " r; \
		test "$$r" = y; \
	fi

UPSTREAM := $(shell git remote -v | grep -m 1 "xv6-labs-2021" | awk '{split($$0,a," "); print a[1]}')

tarball: handin-check
	git archive --format=tar HEAD | gzip > lab-$(LAB)-handin.tar.gz

tarball-pref: handin-check
	@SUF=$(LAB); \
	git archive --format=tar HEAD > lab-$$SUF-handin.tar; \
	git diff $(UPSTREAM)/$(LAB) > /tmp/lab-$$SUF-diff.patch; \
	tar -rf lab-$$SUF-handin.tar /tmp/lab-$$SUF-diff.patch; \
	gzip -c lab-$$SUF-handin.tar > lab-$$SUF-handin.tar.gz; \
	rm lab-$$SUF-handin.tar; \
	rm /tmp/lab-$$SUF-diff.patch; \

myapi.key:
	@echo Get an API key for yourself by visiting $(WEBSUB)/
	@read -p "Please enter your API key: " k; \
	if test `echo "$$k" |tr -d '\n' |wc -c` = 32 ; then \
		TF=`mktemp -t tmp.XXXXXX`; \
		if test "x$$TF" != "x" ; then \
			echo "$$k" |tr -d '\n' > $$TF; \
			mv -f $$TF $@; \
		else \
			echo mktemp failed; \
			false; \
		fi; \
	else \
		echo Bad API key: $$k; \
		echo An API key should be 32 characters long.; \
		false; \
	fi;


.PHONY: handin tarball tarball-pref clean grade handin-check
