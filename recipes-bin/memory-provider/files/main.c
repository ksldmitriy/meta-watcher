#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

// Global variable that we want to inspect from kernel module
int target_value = 42;
static int static_value = 123;

int32_t var1 = 999;
int32_t var2 = 0;

static void change_vars() {
    var1++;
    var2++;
}

static void read_vars() {
    if (var1 == -1) {
        sleep(1);
    }

    if (var2 == -1) {
        sleep(1);
    }
}

int main() {
    printf("PID: %d, &var1: %lu, &var2: %lu\n",
           getpid(),
           (unsigned long)&var1,
           (unsigned long)&var2);

    while (1) {
        sleep(1);
        change_vars();
        sleep(1);
        read_vars();
    }

    return 0;
}
