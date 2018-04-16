/*
 * process_helper.c
 *
 *  Created on: 2014骞�7鏈�26鏃�
 *      Author: 
 */

#include "process_helper.h"

//int send_fd(int fd, int fd_to_send)
//{
//    struct msghdr msg;
//    msg.msg_name = NULL;
//    msg.msg_namelen = 0;
//
//    struct cmsghdr *cmptr = NULL;
//    char buffer[BUF_SIZE];
//    struct iovec iov;
//
//    if (fd_to_send >= 0)
//    {
//        int cmsg_len = CMSG_LEN(sizeof(int));
//        cmptr = malloc(cmsg_len);
//
//        cmptr->cmsg_level = SOL_SOCKET;
//        cmptr->cmsg_type = SCM_RIGHTS;
//        cmptr->cmsg_len = cmsg_len;
//        *(int*)CMSG_DATA(cmptr) = fd_to_send;
//
//        msg.msg_control = cmptr;
//        msg.msg_controllen = cmsg_len;
//
//        sprintf(buffer, "OK!");
//    }
//    else
//    {
//
//        if (-1 == fd_to_send)
//            sprintf(buffer, "cannot open file!");
//        else
//            sprintf(buffer, "wrong command!");
//
//        msg.msg_control = NULL;
//        msg.msg_controllen = 0;
//    }
//
//    msg.msg_iov = &iov;
//    msg.msg_iovlen = 1;
//    iov.iov_base = buffer;
//    iov.iov_len = strlen(buffer);
//
//    sendmsg(fd, &msg, 0);
//    if (cmptr)
//        free(cmptr);
//    return 0;
//}
//
//int recv_fd(int fd, char *buffer, size_t size)
//{
//    struct cmsghdr *cmptr;
//    int cmsg_len = CMSG_LEN(sizeof(int));
//    cmptr = malloc(cmsg_len);
//
//    struct iovec iov;
//    iov.iov_base = buffer;
//    iov.iov_len = size;
//
//    struct msghdr msg;
//    msg.msg_name = NULL;
//    msg.msg_namelen = 0;
//    msg.msg_iov = &iov;
//    msg.msg_iovlen = 1;
//    msg.msg_control = cmptr;
//    msg.msg_controllen = cmsg_len;
//
//    int len = recvmsg(fd, &msg, 0);
//    if (len < 0)
//    {
//        printf("receve message error!\n");
//        exit(0);
//    }
//    else if (len == 0)
//    {
//        printf("connection closed by server!\n");
//        exit(0);
//    }
//
//    buffer[len] = '\0';
//    int cfd = -1;
//    if (cmptr->cmsg_type != 0)
//        cfd = *(int*)CMSG_DATA(cmptr);
//    free(cmptr);
//    return cfd;
//}

//int daemonize()
//{
//    pid_t pid;
//    if ((pid = fork()) < 0)
//        return (-1);
//    else if (pid != 0)
//        exit(0); /* parent exit */
//    /* child continues */
//    setsid(); /* become session leader */
//    chdir("/"); /* change working directory */
//    umask(0); /* clear file mode creation mask */
//    close(0); /* close stdin */
//    close(1); /* close stdout */
//    close(2); /* close stderr */
//    return (0);
//}


void InstallSignal()
{
    signal(SIGHUP  ,SIG_IGN );   /* hangup, generated when terminal disconnects */
    signal(SIGINT  ,SIG_IGN );   /* interrupt, generated from terminal special char */
    signal(SIGQUIT ,SIG_IGN );   /* (*) quit, generated from terminal special char */
    signal(SIGILL  ,SIG_IGN );   /* (*) illegal instruction (not reset when caught)*/
    signal(SIGTRAP ,SIG_IGN );   /* (*) trace trap (not reset when caught) */
    signal(SIGABRT ,SIG_IGN );   /* (*) abort process */
#ifdef D_AIX
    signal(SIGEMT  ,SIG_IGN );   /* EMT intruction */
#endif
    signal(SIGFPE  ,SIG_IGN );   /* (*) floating point exception */
    signal(SIGKILL ,SIG_IGN );   /* kill (cannot be caught or ignored) */
    signal(SIGBUS  ,SIG_IGN );   /* (*) bus error (specification exception) */
    signal(SIGSEGV ,SIG_IGN );   /* (*) segmentation violation */
    signal(SIGSYS  ,SIG_IGN );   /* (*) bad argument to system call */
    signal(SIGPIPE ,SIG_IGN );   /* write on a pipe with no one to read it */
    signal(SIGALRM ,SIG_IGN );   /* alarm clock timeout */
    //signal(SIGTERM ,stopproc );  /* software termination signal */
    signal(SIGURG  ,SIG_IGN );   /* (+) urgent contition on I/O channel */
    signal(SIGSTOP ,SIG_IGN );   /* (@) stop (cannot be caught or ignored) */
    signal(SIGTSTP ,SIG_IGN );   /* (@) interactive stop */
    signal(SIGCONT ,SIG_IGN );   /* (!) continue (cannot be caught or ignored) */
    //signal(SIGCHLD ,SIG_IGN);    /* (+) sent to parent on child stop or exit */
    signal(SIGTTIN ,SIG_IGN);    /* (@) background read attempted from control terminal*/
    signal(SIGTTOU ,SIG_IGN);    /* (@) background write attempted to control terminal */
    signal(SIGIO   ,SIG_IGN);    /* (+) I/O possible, or completed */
    signal(SIGXCPU ,SIG_IGN);    /* cpu time limit exceeded (see setrlimit()) */
    signal(SIGXFSZ ,SIG_IGN);    /* file size limit exceeded (see setrlimit()) */

#ifdef D_AIX
    signal(SIGMSG  ,SIG_IGN);    /* input data is in the ring buffer */
#endif

    signal(SIGWINCH,SIG_IGN);    /* (+) window size changed */
    signal(SIGPWR  ,SIG_IGN);    /* (+) power-fail restart */
    //signal(SIGUSR1 ,SIG_IGN);   /* user defined signal 1 */
    //signal(SIGUSR2 ,SIG_IGN);   /* user defined signal 2 */
    signal(SIGPROF ,SIG_IGN);    /* profiling time alarm (see setitimer) */

#ifdef D_AIX
    signal(SIGDANGER,SIG_IGN);   /* system crash imminent; free up some page space */
#endif

    signal(SIGVTALRM,SIG_IGN);   /* virtual time alarm (see setitimer) */

#ifdef D_AIX
    signal(SIGMIGRATE,SIG_IGN);  /* migrate process */
    signal(SIGPRE  ,SIG_IGN);    /* programming exception */
    signal(SIGVIRT ,SIG_IGN);    /* AIX virtual time alarm */
    signal(SIGALRM1,SIG_IGN);    /* m:n condition variables - RESERVED - DON'T USE */
    signal(SIGWAITING,SIG_IGN);  /* m:n scheduling - RESERVED - DON'T USE */
    signal(SIGCPUFAIL ,SIG_IGN); /* Predictive De-configuration of Processors - */
    signal(SIGKAP,SIG_IGN);      /* keep alive poll from native keyboard */
    signal(SIGRETRACT,SIG_IGN);  /* monitor mode should be relinguished */
    signal(SIGSOUND  ,SIG_IGN);  /* sound control has completed */
    signal(SIGSAK    ,SIG_IGN);  /* secure attention key */
#endif
}

int SetCoreMax( )
{
    struct rlimit rlim_new;
    struct rlimit rlim;
    /*
     *  First try raising to infinity; if that fails, try bringing
     *  the soft limit to the hard.
    */
    if (getrlimit(RLIMIT_CORE, &rlim) == 0)
    {
        rlim_new.rlim_cur = rlim_new.rlim_max = RLIM_INFINITY;
        if (setrlimit(RLIMIT_CORE, &rlim_new)!= 0) 
        {
            /* failed. try raising just to the old max */
            rlim_new.rlim_cur = rlim_new.rlim_max = rlim.rlim_max;
            (void)setrlimit(RLIMIT_CORE, &rlim_new);
        }
    }
	/*
	 * getrlimit again to see what we ended up with. Only fail if
	 * the soft limit ends up 0, because then no core files will be
	 * created at all.
	 */
	 if ((getrlimit(RLIMIT_CORE, &rlim) != 0) || rlim.rlim_cur == 0)
     {
        fprintf(stderr, "failed to ensure corefile creation\n");
        return -1;
     }
     return 0;
}


/**
 * @brief: daemonize 
 *
 * @param cmd
 * @param nochdir, 是否需要将进程的路劲设置为 "/";  大于0 标示 不用把路径改变为"/"
 * /          *if we want to ensure our ability to dump core, don't chdir to / *
 * @param noclose, 是否关闭标准的输入，输出。 大于0 标示不关闭。小于0 需要把标注输入输出重新定向.
 */
void daemonize(const char* cmd, int nochdir, int noclose)
{
    struct sigaction sa;
    pid_t pid;
    SetCoreMax(); //set dump core max
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGHUP, &sa, NULL) < 0)
    {
        fprintf(stderr, "Failed to ignore SIGHUP");
        exit(EXIT_FAILURE);
    }

    if ((pid = fork()) < 0)
    {
        fprintf(stderr,"fork() fail");
        exit(EXIT_FAILURE);
    }
    else if (pid != 0)
    {
        exit(EXIT_SUCCESS);
    }
    if (setsid() == -1)
    {
        exit(EXIT_FAILURE);
    }
    if (nochdir == 0) 
    {
        if (chdir("/") != 0)
        {
            perror("chdir");
            exit(EXIT_FAILURE);
        }
    }

    int fd = 0;
    if (noclose == 0 && (fd = open("/dev/null", O_RDWR, 0)) != -1)
    {
        if (dup2(fd, STDIN_FILENO) < 0) 
        {
            perror("dup2 stdin");
            exit(EXIT_FAILURE);
        }
        if(dup2(fd, STDOUT_FILENO) < 0)
        {
            perror("dup2 stdout");
            exit(EXIT_FAILURE);
        }
        if (dup2(fd, STDERR_FILENO) < 0)
        {
            perror("dump2 stderr");
            exit(EXIT_FAILURE);
        }
        if (fd > STDERR_FILENO)
        {
            if (close(fd) < 0)
            {
                perror("close");
                exit(EXIT_FAILURE);
            }
        }
    }
    InstallSignal();
}

int x_sock_set_block(int sock, int on)
{
        int             val;
        int             rv;

        val = fcntl(sock, F_GETFL, 0);
        if (on) {
                rv = fcntl(sock, F_SETFL, ~O_NONBLOCK&val);
        } else {
                rv = fcntl(sock, F_SETFL, O_NONBLOCK|val);
        }

        if (rv) {
                return (int)(errno);
        }

        return 0;
}

int send_fd(int sock_fd, int send_fd)
{
	int ret;
	struct msghdr msg;
	struct cmsghdr *p_cmsg;
	struct iovec vec;
	char cmsgbuf[CMSG_SPACE(sizeof(send_fd))];
	int *p_fds;
	char sendchar = 0;
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);
	p_cmsg = CMSG_FIRSTHDR(&msg);
	p_cmsg->cmsg_level = SOL_SOCKET;
	p_cmsg->cmsg_type = SCM_RIGHTS;
	p_cmsg->cmsg_len = CMSG_LEN(sizeof(send_fd));
	p_fds = (int*) CMSG_DATA(p_cmsg);
	*p_fds = send_fd;
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;
	msg.msg_flags = 0;
	vec.iov_base = &sendchar;
	vec.iov_len = sizeof(sendchar);
	ret = sendmsg(sock_fd, &msg, 0);
	int iErrno = errno;
	return(iErrno);
//	if (ret != 1)
//		;//ERR_EXIT("sendmsg");
}

int recv_fd(const int sock_fd) {
	int ret;
	struct msghdr msg;
	char recvchar;
	struct iovec vec;
	int recv_fd;
	char cmsgbuf[CMSG_SPACE(sizeof(recv_fd))];
	struct cmsghdr *p_cmsg;
	int *p_fd;
	vec.iov_base = &recvchar;
	vec.iov_len = sizeof(recvchar);
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);
	msg.msg_flags = 0;
	p_fd = (int*) CMSG_DATA(CMSG_FIRSTHDR(&msg));
	*p_fd = -1;
	ret = recvmsg(sock_fd, &msg, 0);
	if (ret != 1)
	{
        //ERR_EXIT("recvmsg");
	    return(-1);
	}
	p_cmsg = CMSG_FIRSTHDR(&msg);
	if (p_cmsg == NULL)
	{
		//ERR_EXIT("no passed fd");
        return(-1);
	}
	p_fd = (int*) CMSG_DATA(p_cmsg);
	recv_fd = *p_fd;
	if (recv_fd == -1)
	{
        //ERR_EXIT("no passed fd");
        return(-1);
	}
	return recv_fd;
}

int send_fd_with_attr(int sock_fd, int send_fd, void* addr, int addr_len, int send_fd_attr)
{
    int ret;
    struct msghdr msg;
    struct cmsghdr *p_cmsg;
    struct iovec vec[2];
    char cmsgbuf[CMSG_SPACE(sizeof(send_fd))];
    int *p_fds;
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);
    p_cmsg = CMSG_FIRSTHDR(&msg);
    p_cmsg->cmsg_level = SOL_SOCKET;
    p_cmsg->cmsg_type = SCM_RIGHTS;
    p_cmsg->cmsg_len = CMSG_LEN(sizeof(send_fd));
    p_fds = (int*) CMSG_DATA(p_cmsg);
    *p_fds = send_fd;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = vec;
    msg.msg_iovlen = 2;
    msg.msg_flags = 0;
    vec[0].iov_base = addr;
    vec[0].iov_len = addr_len;
    vec[1].iov_base = &send_fd_attr;
    vec[1].iov_len = sizeof(send_fd_attr);
    //passing file descriptor to another process
    ret = sendmsg(sock_fd, &msg, 0);
    int iErrno = errno;
    return(iErrno);
}

int recv_fd_with_attr(const int sock_fd, void* addr, int addr_len, int* send_fd_attr)
{
    int ret;
    struct msghdr msg;
    struct iovec vec[2];
    int recv_fd;
    char cmsgbuf[CMSG_SPACE(sizeof(recv_fd))];
    struct cmsghdr *p_cmsg;
    int *p_fd;
    vec[0].iov_base = addr;
    vec[0].iov_len = addr_len;
    vec[1].iov_base = send_fd_attr;
    vec[1].iov_len = sizeof(send_fd_attr);
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = vec;
    msg.msg_iovlen = 2;
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);
    msg.msg_flags = 0;
    p_fd = (int*) CMSG_DATA(CMSG_FIRSTHDR(&msg));
    *p_fd = -1;
    ret = recvmsg(sock_fd, &msg, 0);
    if (ret <= 0)
    {
        return(ret);
    }
    p_cmsg = CMSG_FIRSTHDR(&msg);
    if (p_cmsg == NULL)
    {
        return(-1);
    }
    p_fd = (int*) CMSG_DATA(p_cmsg);
    recv_fd = *p_fd;
    if (recv_fd == -1)
    {
        return(-1);
    }
    return recv_fd;
}

int readable_timeo(int fd, int sec)
{
    fd_set rset;
    struct timeval tv;

    FD_ZERO(&rset);
    FD_SET(fd, &rset);

    tv.tv_sec = sec;
    tv.tv_usec = 0;

    return select(fd+1, &rset, NULL, NULL, &tv);
}
