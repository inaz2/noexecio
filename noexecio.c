#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/user.h>

int is_executable(pid_t pid, ulong addr, ulong len)
{
    char buf[512];
    FILE *fp;
    char *line;
    size_t n;
    ulong vm_start;
    ulong vm_end;
    char prot[4];
    int result;

    snprintf(buf, sizeof(buf), "/proc/%d/maps", pid);
    fp = fopen(buf, "r");

    line = NULL;
    result = 0;
    while (getline(&line, &n, fp) != -1) {
        sscanf(line, "%lx-%lx %4s", &vm_start, &vm_end, prot);
        if ((vm_start <= addr && addr < vm_end)
            || (vm_start <= (addr+len) && (addr+len) < vm_end)) {
            if (prot[2] == 'x') {
                result = 1;
            }
            break;
        }
    }

    free(line);
    fclose(fp);

    return result;
}

int main(int argc, char *argv[])
{
    pid_t pid;
    int status;
    struct user_regs_struct regs;
    FILE *fp;

    pid = fork();
    if (pid == -1) {
        exit(1);
    } else if (pid == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execvp(argv[1], argv+1);
        perror("noexecio");
        exit(1);
    } else {
        while (1) {
            waitpid(pid, &status, 0);
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                break;
            }

            ptrace(PTRACE_GETREGS, pid, NULL, &regs);
            switch (regs.orig_rax) {
            case 0:  /* __NR_read */
            case 1:  /* __NR_write */
                if (is_executable(pid, regs.rsi, regs.rdx)) {
                    fp = fopen("/dev/tty", "w");
                    fprintf(fp, "*** executable io detected ***: addr=%lx, len=%lx\n", regs.rsi, regs.rdx);
                    fclose(fp);
                    ptrace(PTRACE_KILL, pid, NULL, NULL);
                    exit(1);
                }
                break;
            }
            ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
        }
    }

    return 0;
}
