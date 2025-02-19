// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#ifdef __linux__
#include <sys/prctl.h>

// Enable compilation with older header that doesn't contain this constant
// for running on newer libraries that do support it
#ifndef PR_SET_PTRACER
#define PR_SET_PTRACER 0x59616d61
#endif
#endif

/* For OpenBSD */
#ifndef EPROTO
# define EPROTO EINVAL
#endif

extern char **environ;

static int qtcFd;
static char *sleepMsg;
static int chldPipe[2];
static int blockingPipe[2];
static int isDebug;
static volatile int isDetached;
static volatile int chldPid;

static void __attribute__((noreturn)) doExit(int code)
{
    tcsetpgrp(0, getpid());
    puts(sleepMsg);
    const char *rv = fgets(sleepMsg, 2, stdin); /* Minimal size to make it wait */
    (void)rv; // Q_UNUSED
    exit(code);
}

static void sendMsg(const char *msg, int num)
{
    int pidStrLen;
    int ioRet;
    char pidStr[64];

    pidStrLen = sprintf(pidStr, msg, num);
    if (!isDetached && (ioRet = write(qtcFd, pidStr, pidStrLen)) != pidStrLen) {
        fprintf(stderr, "Cannot write to creator comm socket: %s\n",
                        (ioRet < 0) ? strerror(errno) : "short write");
        isDetached = 2;
    }
}

enum {
    ArgCmd = 0,
    ArgAction,
    ArgSocket,
    ArgMsg,
    ArgDir,
    ArgEnv,
    ArgPid,
    ArgExe
};

/* Handle sigchld */
static void sigchldHandler(int sig)
{
    int chldStatus;
    /* Currently we have only one child, so we exit in case of error. */
    int waitRes;
    (void)sig;
    for (;;) {
        waitRes = waitpid(-1, &chldStatus, WNOHANG);
        if (!waitRes)
            break;
        if (waitRes < 0) {
            perror("Cannot obtain exit status of child process");
            doExit(3);
        }
        if (WIFSTOPPED(chldStatus)) {
            /* The child stopped. This can be the result of the initial SIGSTOP handling.
             * We won't need the notification pipe any more, as we know that
             * the exec() succeeded. */
            close(chldPipe[0]);
            close(chldPipe[1]);
            chldPipe[0] = -1;
            if (isDetached == 2 && isDebug) {
                /* qtcreator was not informed and died while debugging, killing the child */
                kill(chldPid, SIGKILL);
            }
        } else if (WIFEXITED(chldStatus)) {
            sendMsg("exit %d\n", WEXITSTATUS(chldStatus));
            doExit(0);
        } else {
            sendMsg("crash %d\n", WTERMSIG(chldStatus));
            doExit(0);
        }
    }
}


/* syntax: $0 {"run"|"debug"} <pid-socket> <continuation-msg> <workdir> <env-file> <exe> <args...> */
/* exit codes: 0 = ok, 1 = invocation error, 3 = internal error */
int main(int argc, char *argv[])
{
    int errNo, hadInvalidCommand = 0;
    char **env = 0;
    struct sockaddr_un sau;
    struct sigaction act;

    memset(&act, 0, sizeof(act));

    if (argc < ArgEnv) {
        fprintf(stderr, "This is an internal helper of Qt Creator. Do not run it manually.\n");
        return 1;
    }
    sleepMsg = argv[ArgMsg];

    /* Connect to the master, i.e. Creator. */
    if ((qtcFd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("Cannot create creator comm socket");
        doExit(3);
    }
    memset(&sau, 0, sizeof(sau));
    sau.sun_family = AF_UNIX;
    strncpy(sau.sun_path, argv[ArgSocket], sizeof(sau.sun_path) - 1);
    if (connect(qtcFd, (struct sockaddr *)&sau, sizeof(sau))) {
        fprintf(stderr, "Cannot connect creator comm socket %s: %s\n", sau.sun_path, strerror(errno));
        doExit(1);
    }

    isDebug = !strcmp(argv[ArgAction], "debug");
    isDetached = 0;

    if (*argv[ArgDir] && chdir(argv[ArgDir])) {
        /* Only expected error: no such file or direcotry */
        sendMsg("err:chdir %d\n", errno);
        return 1;
    }

    if (*argv[ArgEnv]) {
        FILE *envFd;
        char *envdata, *edp, *termEnv;
        long size;
        int count;
        if (!(envFd = fopen(argv[ArgEnv], "r"))) {
            fprintf(stderr, "Cannot read creator env file %s: %s\n",
                    argv[ArgEnv], strerror(errno));
            doExit(1);
        }
        fseek(envFd, 0, SEEK_END);
        size = ftell(envFd);
        if (size < 0) {
            perror("Failed to get size of env file");
            doExit(1);
        }
        rewind(envFd);
        envdata = malloc(size + 1);
        if (fread(envdata, 1, size, envFd) != (size_t)size) {
            perror("Failed to read env file");
            doExit(1);
        }
        envdata[size] = '\0';
        fclose(envFd);
        assert(!size || !envdata[size - 1]);
        for (count = 0, edp = envdata; edp < envdata + size; ++count)
            edp += strlen(edp) + 1;
        env = malloc((count + 2) * sizeof(char *));
        for (count = 0, edp = envdata; edp < envdata + size; ++count) {
            env[count] = edp;
            edp += strlen(edp) + 1;
        }
        if ((termEnv = getenv("TERM")))
            env[count++] = termEnv - 5;
        env[count] = 0;
    }

    /* send our pid after we read the environment file (creator will get rid of it) */
    sendMsg("spid %ld\n", (long)getpid());

    /*
     * set up the signal handlers
     */
    {
        /* Ignore SIGTTOU. Without this, calling tcsetpgrp() from a background
         * process group (in which we will be, once as child and once as parent)
         * generates the mentioned signal and stops the concerned process. */
        act.sa_handler = SIG_IGN;
        if (sigaction(SIGTTOU, &act, 0)) {
            perror("sigaction SIGTTOU");
            doExit(3);
        }

        /* Handle SIGCHLD to keep track of what the child does without blocking */
        act.sa_handler = sigchldHandler;
        if (sigaction(SIGCHLD, &act, 0)) {
            perror("sigaction SIGCHLD");
            doExit(3);
        }
    }

    /* Create execution result notification pipe. */
    if (pipe(chldPipe)) {
        perror("Cannot create status pipe");
        doExit(3);
    }

    /* The debugged program is not supposed to inherit these handles. But we cannot
     * close the writing end before calling exec(). Just handle both ends the same way ... */
    fcntl(chldPipe[0], F_SETFD, FD_CLOEXEC);
    fcntl(chldPipe[1], F_SETFD, FD_CLOEXEC);

    if (isDebug) {
        /* Create execution start notification pipe. The child waits on this until
           the parent writes to it, triggered by an 'c' message from Creator */
        if (pipe(blockingPipe)) {
            perror("Cannot create blocking pipe");
            doExit(3);
        }
    }

    switch ((chldPid = fork())) {
        case -1:
            perror("Cannot fork child process");
            doExit(3);
        case 0:
            close(qtcFd);

            /* Remove the SIGCHLD handler from the child */
            act.sa_handler = SIG_DFL;
            sigaction(SIGCHLD, &act, 0);

            /* Put the process into an own process group and make it the foregroud
             * group on this terminal, so it will receive ctrl-c events, etc.
             * This is the main reason for *all* this stub magic in the first place. */
            /* If one of these calls fails, the world is about to end anyway, so
             * don't bother checking the return values. */
            setpgid(0, 0);
            tcsetpgrp(0, getpid());

#ifdef __linux__
            prctl(PR_SET_PTRACER, atoi(argv[ArgPid]));
#endif
            /* Block to allow the debugger to attach */
            if (isDebug) {
                char buf;
                int res = read(blockingPipe[0], &buf, 1);
                if (res < 0)
                    perror("Could not read from blocking pipe");
                close(blockingPipe[0]);
                close(blockingPipe[1]);
            }

            if (env)
                environ = env;

            execvp(argv[ArgExe], argv + ArgExe);
            /* Only expected error: no such file or direcotry, i.e. executable not found */
            errNo = errno;
            /* Only realistic error case is SIGPIPE */
            if (write(chldPipe[1], &errNo, sizeof(errNo)) != sizeof(errNo))
                perror("Error passing errno to child");
            _exit(0);
        default:
            sendMsg("pid %d\n", chldPid);
            for (;;) {
                char buffer[100];
                int nbytes;

                nbytes = read(qtcFd, buffer, 100);
                if (nbytes <= 0) {
                    if (nbytes < 0 && errno == EINTR)
                        continue;
                    if (!isDetached) {
                        isDetached = 2;
                        if (nbytes == 0)
                            fprintf(stderr, "Lost connection to QtCreator, detaching from it.\n");
                        else
                            perror("Lost connection to QtCreator, detaching from it");
                    }
                    break;
                } else {
                    int i;
                    char c = 'i';
                    for (i = 0; i < nbytes; ++i) {
                        switch (buffer[i]) {
                        case 'k':
                            if (chldPid > 0) {
                                kill(chldPid, SIGTERM);
                                sleep(1);
                                kill(chldPid, SIGKILL);
                            }
                            break;
                        case 'i':
                            if (chldPid > 0) {
                                int res = kill(chldPid, SIGINT);
                                if (res)
                                    perror("Stub could not interrupt inferior");
                            }
                            break;
                        case 'c': {
                            int res = write(blockingPipe[1], &c, 1);
                            if (res < 0)
                                perror("Could not write to blocking pipe");
                            break;
                        }
                        case 'd':
                            isDetached = 1;
                            break;
                        case 's':
                            exit(0);
                        default:
                            if (!hadInvalidCommand) {
                                fprintf(stderr, "Ignoring invalid commands from QtCreator.\n");
                                hadInvalidCommand = 1;
                            }
                        }
                    }
                }
            }
            if (isDetached) {
                for (;;)
                    pause(); /* will exit in the signal handler... */
            }
    }
    assert(0);
    return 0;
}
