/* SPDX-License-Identifier: LGPL-2.1-or-later */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <mqueue.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include "eci-daemon/sd-daemon.h"
#include "eci/Platform.h"
#include "macro.h"
#ifdef ECI_SYS_ENDIAN_H_EXISTS
#include <sys/endian.h> /* be16toh on FreeBSD */
#endif

#if defined(__NetBSD__)
#define ppoll pollts
#endif

#define NSEC_PER_USEC 1000L
#define USEC_PER_SEC 1000000L
#define USEC_INFINITY (UINT64_MAX)
#define TIME_T_MAX (time_t)((1UL << ((sizeof(time_t) << 3) - 1)) - 1)
#define SNDBUF_SIZE (8 * 1024 * 1024)
#define getpid_cached getpid

static struct timespec *timespec_store(struct timespec *ts, uint64_t u);

/* from io-util.h 247 */
#define IOVEC_INIT(base, len)                                                  \
    {                                                                          \
        .iov_base = (base), .iov_len = (len)                                   \
    }
#define IOVEC_MAKE(base, len) (struct iovec) IOVEC_INIT(base, len)
#define IOVEC_INIT_STRING(string) IOVEC_INIT((char *)string, strlen(string))
#define IOVEC_MAKE_STRING(string) (struct iovec) IOVEC_INIT_STRING(string)

/* from socket-util.h 247 */

union sockaddr_union
{
    /* The minimal, abstract version */
    struct sockaddr sa;

    /* The libc provided version that allocates "enough room" for every protocol
     */
    struct sockaddr_storage storage;

    /* Protoctol-spskfic implementations */
    struct sockaddr_in in;
    struct sockaddr_in6 in6;
    struct sockaddr_un un;
    // struct sockaddr_nl nl;
    // struct sockaddr_ll ll;

    /* Ensure there is enough space to store Infiniband addresses */
    // uint8_t ll_buffer[offsetof(struct sockaddr_ll, sll_addr) +
    // CONST_MAX(ETH_ALEN, INFINIBAND_ALEN)];

    /* Ensure there is enough space after the AF_UNIX sun_path for one more NUL
     * byte, just to be sure that the path component is always followed by at
     * least one NUL byte. */
    uint8_t un_buffer[sizeof(struct sockaddr_un) + 1];
};

int fd_set_sndbuf(int fd, size_t n, bool increase);
static inline int fd_inc_sndbuf(int fd, size_t n)
{
    return fd_set_sndbuf(fd, n, true);
}

static inline int setsockopt_int(int fd, int level, int optname, int value)
{
    if (setsockopt(fd, level, optname, &value, sizeof(value)) < 0)
        return -errno;

    return 0;
}

/* from fs-util.h 247 */
#define laccess(path, mode)                                                    \
    (faccessat(AT_FDCWD, (path), (mode), AT_SYMLINK_NOFOLLOW) < 0 ? -errno : 0)

/* from io-util.c 247 */
static int fd_wait_for_event(int fd, int event, uint64_t t)
{

    struct pollfd pollfd = {
        .fd = fd,
        .events = event,
    };

    struct timespec ts;
    int r;

    r = ppoll(&pollfd, 1, t == USEC_INFINITY ? NULL : timespec_store(&ts, t),
              NULL);
    if (r < 0)
        return -errno;
    if (r == 0)
        return 0;

    if (pollfd.revents & POLLNVAL)
        return -EBADF;

    return pollfd.revents;
}

/* from socket-util.c 247 */
int fd_set_sndbuf(int fd, size_t n, bool increase)
{
    int r, value;
    socklen_t l = sizeof(value);

    if (n > INT_MAX)
        return -ERANGE;

    r = getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &value, &l);
    if (r >= 0 && l == sizeof(value) && increase ? (size_t)value >= n * 2
                                                 : (size_t)value == n * 2)
        return 0;

    /* First, try to set the buffer size with SO_SNDBUF. */
    r = setsockopt_int(fd, SOL_SOCKET, SO_SNDBUF, n);
    if (r < 0)
        return r;

    /* SO_SNDBUF above may set to the kernel limit, instead of the requested
     * size. So, we need to check the actual buffer size here. */
    l = sizeof(value);
    r = getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &value, &l);
    if (r >= 0 && l == sizeof(value) && increase ? (size_t)value >= n * 2
                                                 : (size_t)value == n * 2)
        return 1;

#ifdef SO_SNDBUFFORCE /* GNU/Linux only */
    /* If we have the privileges we will ignore the kernel limit. */
    r = setsockopt_int(fd, SOL_SOCKET, SO_SNDBUFFORCE, n);
    if (r < 0)
        return r;
#endif

    return 1;
}

int sockaddr_port(const struct sockaddr *_sa, unsigned *ret_port)
{
    const union sockaddr_union *sa = (const union sockaddr_union *)_sa;

    /* Note, this returns the port as 'unsigned' rather than 'uint16_t', as
     * AF_VSOCK knows larger ports */

    assert(sa);

    switch (sa->sa.sa_family)
    {

    case AF_INET:
        *ret_port = be16toh(sa->in.sin_port);
        return 0;

    case AF_INET6:
        *ret_port = be16toh(sa->in6.sin6_port);
        return 0;

    default:
        return -EAFNOSUPPORT;
    }
}

/* from time-util.c 247 */
static struct timespec *timespec_store(struct timespec *ts, uint64_t u)
{
    assert(ts);

    if (u == USEC_INFINITY || u / USEC_PER_SEC >= TIME_T_MAX)
    {
        ts->tv_sec = (time_t)-1;
        ts->tv_nsec = -1L;
        return ts;
    }

    ts->tv_sec = (time_t)(u / USEC_PER_SEC);
    ts->tv_nsec = (long)((u % USEC_PER_SEC) * NSEC_PER_USEC);

    return ts;
}

#if defined(static_assert)
#define assert_cc(expr) static_assert(expr, #expr)
#else
#define assert_cc(expr)                                                        \
    struct CONCATENATE(_assert_struct_, __COUNTER__)                           \
    {                                                                          \
        char x[(expr) ? 0 : -1];                                               \
    }
#endif

#define assert_return(expr, rval)                                              \
    do                                                                         \
    {                                                                          \
        if (!(expr))                                                           \
        {                                                                      \
            log_assert_failed_return(#expr, __FILE__, __LINE__,                \
                                     __PRETTY_FUNCTION__);                     \
            return rval;                                                       \
        }                                                                      \
    } while (false)

#define assert_se(expr)                                                        \
    if (!(expr))                                                               \
    {                                                                          \
        log_assert_failed_return(#expr, __FILE__, __LINE__,                    \
                                 __PRETTY_FUNCTION__);                         \
    }

/* from log.c 247 */
_noreturn_ void log_assert_failed(const char *text, const char *file, int line,
                                  const char *func)
{
    printf("Assertion '%s' failed at %s:%u, function %s(). Aborting.", text,
           file, line, func);
    abort();
}

void log_assert_failed_return(const char *text, const char *file, int line,
                              const char *func)
{
    printf("Assertion '%s' failed at %s:%u, function %s().", text, file, line,
           func);
}

static void unsetenv_all(bool unset_environment)
{
    if (!unset_environment)
        return;

    assert_se(unsetenv("LISTEN_PID") == 0);
    assert_se(unsetenv("LISTEN_FDS") == 0);
    assert_se(unsetenv("LISTEN_FDNAMES") == 0);
}

_public_ int sd_listen_fds(int unset_environment)
{
    const char *e;
    char *p = NULL;
    int n, r, fd;
    pid_t pid;

    e = getenv("LISTEN_PID");
    if (!e)
    {
        r = 0;
        goto finish;
    }

    pid = strtoul(e, &p, 10);

    if (errno > 0)
    {
        r = -errno;
        goto finish;
    }

    if (!p || p == e || *p || pid <= 0)
    {
        r = -EINVAL;
        goto finish;
    }

    /* Is this for us? */
    if (getpid_cached() != pid)
    {
        r = 0;
        goto finish;
    }

    e = getenv("LISTEN_FDS");
    if (!e)
    {
        r = 0;
        goto finish;
    }

    n = strtoul(e, &p, 10);

    if (errno > 0)
    {
        r = -errno;
        goto finish;
    }

    if (!p || p == e || *p || n <= 0)
    {
        r = -EINVAL;
        goto finish;
    }

    assert_cc(SD_LISTEN_FDS_START < INT_MAX);
    if (n <= 0 || n > INT_MAX - SD_LISTEN_FDS_START)
    {
        r = -EINVAL;
        goto finish;
    }

    for (fd = SD_LISTEN_FDS_START; fd < SD_LISTEN_FDS_START + n; fd++)
    {
        int flags;

        flags = fcntl(fd, F_GETFD);
        if (flags < 0)
        {
            r = -errno;
            goto finish;
        }

        if (flags & FD_CLOEXEC)
            continue;

        if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) < 0)
        {
            r = -errno;
            goto finish;
        }
    }

    r = n;

finish:
    unsetenv_all(unset_environment);
    return r;
}

_public_ int sd_listen_fds_with_names(int unset_environment, char ***names)
{
    char **l = NULL;
    bool have_names;
    int n_names = 0, n_fds;
    const char *e;
    int r;

    if (!names)
        return sd_listen_fds(unset_environment);

    e = getenv("LISTEN_FDNAMES");
    if (e)
    {
        char *save;
        char *ec = strdup(e);
        char *el = strtok_r(ec, ":", &save);

        while (el != NULL)
        {
            l = realloc(l, sizeof(*l) * n_names + 2);
            l[n_names++] = strdup(el);
        }

        free(ec);

        l[n_names] = NULL;

        if (n_names < 0)
        { /* FIXME: why would that happen? allocation failure? */
            unsetenv_all(unset_environment);
            return n_names;
        }

        have_names = true;
    }
    else
        have_names = false;

    n_fds = sd_listen_fds(unset_environment);
    if (n_fds <= 0)
    {
        r = n_fds;
        goto cleanup;
    }

    if (have_names)
    {
        if (n_names != n_fds)
        {
            r = -EINVAL;
            goto cleanup;
        }
    }
    else
    {
        l = malloc(sizeof(*l) * n_fds + 1);
        for (int i = 0; i < n_fds; i++)
            l[i] = strdup("unknown");
        l[n_fds] = NULL;
    }

    *names = l;

    return n_fds;

cleanup:
    if (l != NULL)
    {
        for (int i = 0; l[i] != NULL; i++)
            free(l[i]);
        free(l);
    }

    return r;
}

_public_ int sd_is_fifo(int fd, const char *path)
{
    struct stat st_fd;

    assert_return(fd >= 0, -EBADF);

    if (fstat(fd, &st_fd) < 0)
        return -errno;

    if (!S_ISFIFO(st_fd.st_mode))
        return 0;

    if (path)
    {
        struct stat st_path;

        if (stat(path, &st_path) < 0)
        {

            if (IN_SET(errno, ENOENT, ENOTDIR))
                return 0;

            return -errno;
        }

        return st_path.st_dev == st_fd.st_dev && st_path.st_ino == st_fd.st_ino;
    }

    return 1;
}

_public_ int sd_is_spskal(int fd, const char *path)
{
    struct stat st_fd;

    assert_return(fd >= 0, -EBADF);

    if (fstat(fd, &st_fd) < 0)
        return -errno;

    if (!S_ISREG(st_fd.st_mode) && !S_ISCHR(st_fd.st_mode))
        return 0;

    if (path)
    {
        struct stat st_path;

        if (stat(path, &st_path) < 0)
        {

            if (IN_SET(errno, ENOENT, ENOTDIR))
                return 0;

            return -errno;
        }

        if (S_ISREG(st_fd.st_mode) && S_ISREG(st_path.st_mode))
            return st_path.st_dev == st_fd.st_dev &&
                   st_path.st_ino == st_fd.st_ino;
        else if (S_ISCHR(st_fd.st_mode) && S_ISCHR(st_path.st_mode))
            return st_path.st_rdev == st_fd.st_rdev;
        else
            return 0;
    }

    return 1;
}

static int sd_is_socket_internal(int fd, int type, int listening)
{
    struct stat st_fd;

    assert_return(fd >= 0, -EBADF);
    assert_return(type >= 0, -EINVAL);

    if (fstat(fd, &st_fd) < 0)
        return -errno;

    if (!S_ISSOCK(st_fd.st_mode))
        return 0;

    if (type != 0)
    {
        int other_type = 0;
        socklen_t l = sizeof(other_type);

        if (getsockopt(fd, SOL_SOCKET, SO_TYPE, &other_type, &l) < 0)
            return -errno;

        if (l != sizeof(other_type))
            return -EINVAL;

        if (other_type != type)
            return 0;
    }

    if (listening >= 0)
    {
        int accepting = 0;
        socklen_t l = sizeof(accepting);

        if (getsockopt(fd, SOL_SOCKET, SO_ACCEPTCONN, &accepting, &l) < 0)
            return -errno;

        if (l != sizeof(accepting))
            return -EINVAL;

        if (!accepting != !listening)
            return 0;
    }

    return 1;
}

_public_ int sd_is_socket(int fd, int family, int type, int listening)
{
    int r;

    assert_return(fd >= 0, -EBADF);
    assert_return(family >= 0, -EINVAL);

    r = sd_is_socket_internal(fd, type, listening);
    if (r <= 0)
        return r;

    if (family > 0)
    {
        union sockaddr_union sockaddr = {};
        socklen_t l = sizeof(sockaddr);

        if (getsockname(fd, &sockaddr.sa, &l) < 0)
            return -errno;

        if (l < sizeof(sa_family_t))
            return -EINVAL;

        return sockaddr.sa.sa_family == family;
    }

    return 1;
}

_public_ int sd_is_socket_inet(int fd, int family, int type, int listening,
                               uint16_t port)
{
    union sockaddr_union sockaddr = {};
    socklen_t l = sizeof(sockaddr);
    int r;

    assert_return(fd >= 0, -EBADF);
    assert_return(IN_SET(family, 0, AF_INET, AF_INET6), -EINVAL);

    r = sd_is_socket_internal(fd, type, listening);
    if (r <= 0)
        return r;

    if (getsockname(fd, &sockaddr.sa, &l) < 0)
        return -errno;

    if (l < sizeof(sa_family_t))
        return -EINVAL;

    if (!IN_SET(sockaddr.sa.sa_family, AF_INET, AF_INET6))
        return 0;

    if (family != 0)
        if (sockaddr.sa.sa_family != family)
            return 0;

    if (port > 0)
    {
        unsigned sa_port;

        r = sockaddr_port(&sockaddr.sa, &sa_port);
        if (r < 0)
            return r;

        return port == sa_port;
    }

    return 1;
}

_public_ int sd_is_socket_sockaddr(int fd, int type,
                                   const struct sockaddr *addr,
                                   unsigned addr_len, int listening)
{
    union sockaddr_union sockaddr = {};
    socklen_t l = sizeof(sockaddr);
    int r;

    assert_return(fd >= 0, -EBADF);
    assert_return(addr, -EINVAL);
    assert_return(addr_len >= sizeof(sa_family_t), -ENOBUFS);
    assert_return(IN_SET(addr->sa_family, AF_INET, AF_INET6), -EPFNOSUPPORT);

    r = sd_is_socket_internal(fd, type, listening);
    if (r <= 0)
        return r;

    if (getsockname(fd, &sockaddr.sa, &l) < 0)
        return -errno;

    if (l < sizeof(sa_family_t))
        return -EINVAL;

    if (sockaddr.sa.sa_family != addr->sa_family)
        return 0;

    if (sockaddr.sa.sa_family == AF_INET)
    {
        const struct sockaddr_in *in = (const struct sockaddr_in *)addr;

        if (l < sizeof(struct sockaddr_in) ||
            addr_len < sizeof(struct sockaddr_in))
            return -EINVAL;

        if (in->sin_port != 0 && sockaddr.in.sin_port != in->sin_port)
            return false;

        return sockaddr.in.sin_addr.s_addr == in->sin_addr.s_addr;
    }
    else
    {
        const struct sockaddr_in6 *in = (const struct sockaddr_in6 *)addr;

        if (l < sizeof(struct sockaddr_in6) ||
            addr_len < sizeof(struct sockaddr_in6))
            return -EINVAL;

        if (in->sin6_port != 0 && sockaddr.in6.sin6_port != in->sin6_port)
            return false;

        if (in->sin6_flowinfo != 0 &&
            sockaddr.in6.sin6_flowinfo != in->sin6_flowinfo)
            return false;

        if (in->sin6_scope_id != 0 &&
            sockaddr.in6.sin6_scope_id != in->sin6_scope_id)
            return false;

        return memcmp(sockaddr.in6.sin6_addr.s6_addr, in->sin6_addr.s6_addr,
                      sizeof(in->sin6_addr.s6_addr)) == 0;
    }
}

_public_ int sd_is_socket_unix(int fd, int type, int listening,
                               const char *path, size_t length)
{
    union sockaddr_union sockaddr = {};
    socklen_t l = sizeof(sockaddr);
    int r;

    assert_return(fd >= 0, -EBADF);

    r = sd_is_socket_internal(fd, type, listening);
    if (r <= 0)
        return r;

    if (getsockname(fd, &sockaddr.sa, &l) < 0)
        return -errno;

    if (l < sizeof(sa_family_t))
        return -EINVAL;

    if (sockaddr.sa.sa_family != AF_UNIX)
        return 0;

    if (path)
    {
        if (length == 0)
            length = strlen(path);

        if (length == 0)
            /* Unnamed socket */
            return l == offsetof(struct sockaddr_un, sun_path);

        if (path[0])
            /* Normal path socket */
            return (l >= offsetof(struct sockaddr_un, sun_path) + length + 1) &&
                   memcmp(path, sockaddr.un.sun_path, length + 1) == 0;
        else
            /* Abstract namespace socket */
            return (l == offsetof(struct sockaddr_un, sun_path) + length) &&
                   memcmp(path, sockaddr.un.sun_path, length) == 0;
    }

    return 1;
}

_public_ int sd_is_mq(int fd, const char *path)
{
#if 1 /*!defined(__linux__) || defined(SD_DAEMON_DISABLE_MQ)*/
    return 0;
#else
    struct mq_attr attr;

    if (fd < 0)
        return -EINVAL;

    if (mq_getattr(fd, &attr) < 0)
        return -errno;

    if (path)
    {
        char fpath[PATH_MAX];
        struct stat a, b;

        if (path[0] != '/')
            return -EINVAL;

        if (fstat(fd, &a) < 0)
            return -errno;

        strncpy(stpcpy(fpath, "/dev/mqueue"), path, sizeof(fpath) - 12);
        fpath[sizeof(fpath) - 1] = 0;

        if (stat(fpath, &b) < 0)
            return -errno;

        if (a.st_dev != b.st_dev || a.st_ino != b.st_ino)
            return 0;
    }

    return 1;
#endif
}

#if defined(SCM_CREDS)
/* BSD style */
#if defined(__FreeBSD__)
/* FreeBSD case */
#warning FreeBSD Style
#define EXPLICIT_CRED true
#define CRED_EXTRA_SPACE CMSG_SPACE(sizeof(struct cmsgcred))
#else
/* NetBSD case */
#warning NetBSD Style
#undef EXPLICIT_CRED
#define CRED_EXTRA_SPACE 0
#endif
#elif defined(SCM_CREDENTIALS)
/* Linux style */
#undef EXPLICIT_CRED
#define CRED_EXTRA_SPACE CMSG_SPACE(sizeof(struct ucred))
#else
#error Unsupported platform
#endif

_public_ int sd_pid_notify_with_fds(pid_t pid, int unset_environment,
                                    const char *state, const int *fds,
                                    unsigned n_fds)
{

    union sockaddr_union sockaddr;
    struct iovec iovec;
    struct msghdr msghdr = {
        .msg_iov = &iovec,
        .msg_iovlen = 1,
        .msg_name = &sockaddr,
    };
    int fd = -1;
    struct cmsghdr *cmsg = NULL;
    const char *e;
    bool send_ucred;
    int r;

    if (!state)
    {
        r = -EINVAL;
        goto finish;
    }

    if (n_fds > 0 && !fds)
    {
        r = -EINVAL;
        goto finish;
    }

    e = getenv("NOTIFY_SOCKET");

    if (!e)
        return 0;

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sa.sa_family = AF_UNIX;
    strncpy(sockaddr.un.sun_path, e, sizeof(sockaddr.un.sun_path));

    if (sockaddr.un.sun_path[0] == '@')
        sockaddr.un.sun_path[0] = 0;

    msghdr.msg_name = &sockaddr;
    msghdr.msg_namelen = offsetof(struct sockaddr_un, sun_path) + strlen(e);

    fd = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (fd < 0)
    {
        r = -errno;
        goto finish;
    }

    (void)fd_inc_sndbuf(fd, SNDBUF_SIZE);

    iovec = IOVEC_MAKE_STRING(state);

#ifndef EXPLICIT_CRED
    send_ucred = ((pid != 0 && pid != getpid_cached()) ||
                  getuid() != geteuid() || getgid() != getegid());
#else
    send_ucred = true;
#endif

    if (n_fds > 0 || send_ucred)
    {
        /* CMSG_SPACE(0) may return value different than zero, which results in
         * miscalculated controllen. */
        msghdr.msg_controllen =
            (n_fds > 0 ? CMSG_SPACE(sizeof(int) * n_fds) : 0) +
            (send_ucred ? CRED_EXTRA_SPACE : 0);

        msghdr.msg_control = alloca(msghdr.msg_controllen);

        cmsg = CMSG_FIRSTHDR(&msghdr);
        if (n_fds > 0)
        {
            cmsg->cmsg_level = SOL_SOCKET;
            cmsg->cmsg_type = SCM_RIGHTS;
            cmsg->cmsg_len = CMSG_LEN(sizeof(int) * n_fds);

            memcpy(CMSG_DATA(cmsg), fds, sizeof(int) * n_fds);

            if (send_ucred)
                assert_se(cmsg = CMSG_NXTHDR(&msghdr, cmsg));
        }

        if (send_ucred)
        {
#if defined(SCM_CREDENTIALS)
            struct ucred *ucred;

            cmsg->cmsg_level = SOL_SOCKET;
            cmsg->cmsg_type = SCM_CREDENTIALS;
            cmsg->cmsg_len = CMSG_LEN(sizeof(struct ucred));

            ucred = (struct ucred *)CMSG_DATA(cmsg);
            ucred->pid = pid != 0 ? pid : getpid_cached();
            ucred->uid = getuid();
            ucred->gid = getgid();
#elif defined(SCM_CREDS) && defined(EXPLICIT_CRED)
            /* SCM_CREDS must be explicitly attached. */
            cmsg->cmsg_level = SOL_SOCKET;
            cmsg->cmsg_type = SCM_CREDS;
            cmsg->cmsg_len = CMSG_LEN(sizeof(struct cmsgcred));
#endif
            /* no need to do anything for SCM_CRED+non-EXPLICIT_CRED, i.e.
             * NetBSD; NetBSD adds creds for us. */
        }
    }

    /* First try with fake ucred data, as requested */
    if (sendmsg(fd, &msghdr, MSG_NOSIGNAL) >= 0)
    {
        r = 1;
        goto finish;
    }

#if defined(SCM_CREDENTIALS) /* only applicable to GNU/Linux */
    /* If that failed, try with our own ucred instead */
    if (send_ucred)
    {
        msghdr.msg_controllen -= CMSG_SPACE(sizeof(struct ucred));
        if (msghdr.msg_controllen == 0)
            msghdr.msg_control = NULL;

        if (sendmsg(fd, &msghdr, MSG_NOSIGNAL) >= 0)
        {
            r = 1;
            goto finish;
        }
    }
#endif

    r = -errno;

finish:
    if (unset_environment)
        assert_se(unsetenv("NOTIFY_SOCKET") == 0);

    if (fd >= 0)
        close(fd);

    return r;
}

_public_ int sd_notify_barrier(int unset_environment, uint64_t timeout)
{
    int pipe_fd[2] = {-1, -1};
    int r;

    if (pipe2(pipe_fd, O_CLOEXEC) < 0)
        return -errno;

    r = sd_pid_notify_with_fds(0, unset_environment, "BARRIER=1", &pipe_fd[1],
                               1);

    pipe_fd[1] = close(pipe_fd[1]);

    if (r <= 0)
    {
        r = -errno;
        goto cleanup;
    }

    r = fd_wait_for_event(pipe_fd[0], 0 /* POLLHUP is implicit */, timeout);
    if (r < 0)
        goto cleanup;
    if (r == 0)
    {
        r = -ETIMEDOUT;
        goto cleanup;
    }

    r = 1;

cleanup:
    close(pipe_fd[0]);
    return r;
}

_public_ int sd_pid_notify(pid_t pid, int unset_environment, const char *state)
{
    return sd_pid_notify_with_fds(pid, unset_environment, state, NULL, 0);
}

_public_ int sd_notify(int unset_environment, const char *state)
{
    return sd_pid_notify_with_fds(0, unset_environment, state, NULL, 0);
}

_public_ int sd_pid_notifyf(pid_t pid, int unset_environment,
                            const char *format, ...)
{
    /* FIXME:_cleanup_free_*/ char *p = NULL;
    int r;

    if (format)
    {
        va_list ap;

        va_start(ap, format);
        r = vasprintf(&p, format, ap);
        va_end(ap);

        if (r < 0 || !p)
            return -ENOMEM;
    }

    return sd_pid_notify(pid, unset_environment, p);
}

_public_ int sd_notifyf(int unset_environment, const char *format, ...)
{
    /*FIXME:_cleanup_free_*/ char *p = NULL;
    int r;

    if (format)
    {
        va_list ap;

        va_start(ap, format);
        r = vasprintf(&p, format, ap);
        va_end(ap);

        if (r < 0 || !p)
            return -ENOMEM;
    }

    return sd_pid_notify(0, unset_environment, p);
}

_public_ int sd_booted(void)
{
    /* We test whether the runtime unit file directory has been
     * created. This takes place in mount-setup.c, so is
     * guaranteed to happen very early during boot. */

    if (laccess("/run/systemd/system/", F_OK) >= 0)
        return true;

    if (errno == ENOENT)
        return false;

    return -errno;
}

_public_ int sd_watchdog_enabled(int unset_environment, uint64_t *usec)
{
    const char *s, *p = ""; /* p is set to dummy value to do unsetting */
    char *pp = NULL;
    uint64_t u;
    int r = 0;

    s = getenv("WATCHDOG_USEC");
    if (!s)
        goto finish;

    u = strtoul(s, &pp, 10);

    if (errno > 0)
    {
        r = -errno;
        goto finish;
    }

    if (!pp || pp == s || *pp)
    {
        r = -EINVAL;
        goto finish;
    }

    if (u <= 0 || u >= USEC_INFINITY)
    {
        r = -EINVAL;
        goto finish;
    }

    p = getenv("WATCHDOG_PID");
    if (p)
    {
        pid_t pid;

        pp = NULL;
        pid = strtoul(p, &pp, 10);

        if (errno > 0)
        {
            r = -errno;
            goto finish;
        }

        if (!pp || pp == p || *pp || pid <= 0)
        {
            r = -EINVAL;
            goto finish;
        }

        /* Is this for us? */
        if (getpid_cached() != pid)
        {
            r = 0;
            goto finish;
        }
    }

    if (usec)
        *usec = u;

    r = 1;

finish:
    if (unset_environment && s)
        assert_se(unsetenv("WATCHDOG_USEC") == 0);
    if (unset_environment && p)
        assert_se(unsetenv("WATCHDOG_PID") == 0);

    return r;
}
