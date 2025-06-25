#include <linux/init.h>
#include <linux/hw_breakpoint.h>
#include <linux/stacktrace.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/sched/debug.h>
#include <linux/mm.h>
#include <linux/pid.h>
#include <linux/sched/mm.h>
#include <linux/moduleparam.h>
#include <linux/uaccess.h>
#include <linux/highmem.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Memory Reader");
MODULE_DESCRIPTION("Read memory from user space process");
MODULE_VERSION("1.0");

#define MAX_STACK_ENTRIES 64

static int target_pid = 0;
static unsigned long target_addr = 0;
static uint32_t prev_value = 0;

static int set_target_addr(const char *val, const struct kernel_param *kp);
static int get_target_addr(char *buffer, const struct kernel_param *kp);
static const struct kernel_param_ops target_addr_ops = {.set = set_target_addr,
                                                        .get = get_target_addr};

module_param(target_pid, int, 0);
module_param_cb(target_addr, &target_addr_ops, &target_addr, 0644);

static struct perf_event *bp = 0;

static int read_int_from_process(int pid, unsigned long addr);
static void read_callback(void);
static void write_callback(int new_value);
static void bp_callback(struct perf_event *bp, struct perf_sample_data *data, struct pt_regs *regs);
static void register_breakpoint(void);
static void unregister_breakpoint(void);

static int set_target_addr(const char *val, const struct kernel_param *kp) {
    int ret = kstrtoul(val, 0, &target_addr);
    if (ret == 0) {
        pr_info("target_addr updated to %lu\n", target_addr);
        register_breakpoint();
    } else {
        pr_err("Failed to parse target_addr, argument is: %s\n", val);
    }

    return ret;
}

static int get_target_addr(char *buffer, const struct kernel_param *kp) {
    return sprintf(buffer, "%lu", target_addr);
}

static int read_int_from_process(int pid, unsigned long addr) {
    rcu_read_lock();
    struct task_struct *task = pid_task(find_vpid(pid), PIDTYPE_PID);
    if (!task) {
        rcu_read_unlock();
        return -1;
    }

    get_task_struct(task);
    rcu_read_unlock();

    struct mm_struct *mm = get_task_mm(task);
    if (!mm) {
        put_task_struct(task);
        return -1;
    }

    if (addr % sizeof(int) != 0) {
        mmput(mm);
        put_task_struct(task);
        return -1;
    }

    int value;
    int ret = access_process_vm(task, addr, &value, sizeof(int), FOLL_FORCE);

    mmput(mm);
    put_task_struct(task);

    if (ret != sizeof(int)) {
        return -1;
    }

    return value;
}

static void read_callback(void) {
    printk(KERN_DEBUG "Read breakpoint was hit\n");
}

static void write_callback(int new_value) {
    printk(KERN_DEBUG "Write breakpoint was hit, new value is %d\n", new_value);
}

static void
bp_callback(struct perf_event *bp, struct perf_sample_data *data, struct pt_regs *regs) {
    int new_value = read_int_from_process(target_pid, target_addr);

    if (prev_value == new_value) {
        read_callback();
    } else {
        write_callback(new_value);
        prev_value = new_value;
    }

    dump_stack_lvl(KERN_DEBUG);
}

static void register_breakpoint(void) {
    unregister_breakpoint();

    struct perf_event_attr attr;
    struct task_struct *task;
    struct pid *pid_struct;

    printk(KERN_DEBUG "Loading breakpoint module for PID %d, addr %lu\n", target_pid, target_addr);

    pid_struct = find_get_pid(target_pid);
    if (!pid_struct) {
        printk(KERN_ERR "Could not find PID %d\n", target_pid);
        return;
    }

    task = pid_task(pid_struct, PIDTYPE_PID);
    if (!task) {
        printk(KERN_ERR "Could not find task for PID %d\n", target_pid);
        put_pid(pid_struct);
        return;
    }

    prev_value = read_int_from_process(target_pid, target_addr);

    hw_breakpoint_init(&attr);
    attr.bp_addr = target_addr;
    attr.bp_len = HW_BREAKPOINT_LEN_4;
    attr.bp_type = HW_BREAKPOINT_RW;
    bp = register_user_hw_breakpoint(&attr, bp_callback, NULL, task);

    put_pid(pid_struct);

    if (IS_ERR(bp)) {
        printk(KERN_ERR "Failed to register breakpoint: %ld\n", PTR_ERR(bp));
        bp = 0;
        return;
    }

    printk(KERN_DEBUG "Hardware breakpoint registered successfully, initial value is %d\n",
           prev_value);
}

static void unregister_breakpoint(void) {
    if (bp) {
        unregister_hw_breakpoint(bp);
        bp = 0;
        printk(KERN_DEBUG "Hardware breakpoint unregistered\n");
    }
}

static int __init memory_reader_init(void) {
    return 0;
}

static void __exit memory_reader_exit(void) {
    unregister_breakpoint();
}

module_init(memory_reader_init);
module_exit(memory_reader_exit);
