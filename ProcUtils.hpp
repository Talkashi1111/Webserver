#pragma once
#include <sys/types.h>
#include <sys/wait.h>

int doWaitpid(pid_t pid, int options);
