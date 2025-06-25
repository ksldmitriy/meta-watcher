# Meta-Watcher

This is a Linux kernel module that watches memory addresses and prints stack traces when that memory is read or written to.

> [!NOTE]
> x86 Architecture does not support separate breakpoints for read operations,
> thus the module saves the previous value and compares it to the new value to distinguish read and write operations.
> This method can give false negatives for write operations.

## Build Instructions (Linux)

To build the project, run the following commands in `bash` or `sh`.

Clone the repository to your Poky directory:
```bash
cd /path/to/poky/
git clone https://github.com/ksldmitriy/meta-watcher
```

Add the meta-watcher layer to your `bblayers.conf` file by adding this line:
```bash
BBLAYERS += "/path/to/poky/meta-watcher"
```

Add the following line to your `local.conf` file:
```bash
MACHINE ??= "qemux86"
IMAGE_INSTALL:append = " kernel-module-watcher memory-provider"
```

Then run the following commands:
```bash
source oe-init-build-env
bitbake core-image-minimal
```

## Testing

The project provides a `memory-provider` program for testing the module. Here is an example of how to test it:

> [!NOTE]
> On your system, the `6.12.31-yocto-standard` directory may be named differently.

```bash
root@qemux86:~# memory-provider &
PID: 269, &var1: 4915208, &var2: 4915220
root@qemux86:~# insmod /lib/modules/6.12.31-yocto-standard/updates/watcher.ko target_pid=269 target_addr=4915208
[   46.650796] watcher: loading out-of-tree module taints kernel.
[   46.656833] target_addr updated to 4915208
```

Then you can check the logs. Every two seconds you will see read and write operation callbacks with backtraces.

You can change the target address by running the following command:

```bash
echo 4915220 >> /sys/module/watcher/parameters/target_addr
```
