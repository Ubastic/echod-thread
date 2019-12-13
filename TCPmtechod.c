/* TCPmtechod.c - main, TCPechod, prstats */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <linux/tcp.h>
#include <net/tcp_states.h>

#define	QLEN		  32	/* maximum connection queue length	*/
#define	BUFSIZE		128
#define MAXSOCK 4
#define	INTERVAL	5	/* secs */
#define THREAD_COUNT 3


struct {
	pthread_mutex_t	st_mutex;
	unsigned int	st_concount; /* undeal */
	unsigned int	st_concurrent; /* current running */
	unsigned int	st_contotal; /*total finished */
	unsigned long	st_contime; /* connection time */
	unsigned long	st_bytecount; /* total bytes */
	int	sockfd[MAXSOCK];
	int i;
} stats;

void	prstats(void);
int	TCPechod();
int	errexit(const char *format, ...);
int	passiveTCP(const char *service, int qlen);
pthread_t	th[50];
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int handler(int sig){
	printf("complete 1 socket\n");
	usleep(2);
	printf("have %d",stats.st_concount);
	fflush(stdout);
/*	if(stats.st_concount!=0)
	pthread_cond_signal(&cond);
	*/
}
pthread_t tid;
/*------------------------------------------------------------------------
 * main - Concurrent TCP server for ECHO service
 *------------------------------------------------------------------------
 */
int
main(int argc, char *argv[])
{

	pthread_attr_t	ta;
	pthread_t tj;
	char	*service = "echo";	/* service name or port number	*/
	struct	sockaddr_in fsin;	/* the address of a client	*/
	unsigned int	alen;		/* length of client's address	*/
	int	msock;			/* master server socket		*/
	int	ssock;			/* slave server socket		*/
	int i=0,j;
	tid=pthread_self();
/* 	signal(10,handler);
	signal(EINTR,SIG_IGN); */
	switch (argc) {
	case	1:
		break;
	case	2:
		service = argv[1];
		break;
	default:
		errexit("usage: TCPechod [port]\n");
	}

	msock = passiveTCP(service, QLEN);

	(void) pthread_attr_init(&ta);
	(void) pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);
	(void) pthread_mutex_init(&stats.st_mutex, 0);

	if (pthread_create(&tj, &ta, (void * (*)(void *))prstats, 0) < 0)
		errexit("pthread_create(prstats): %s\n", strerror(errno));

	for(j=0;j<THREAD_COUNT;j++){
	    if (pthread_create(&th[j], &ta, (void * (*)(void *))TCPechod, 0) < 0)
		errexit("pthread_create(prstats): %s\n", strerror(errno));
	}
	while (1) {
		alen = sizeof(fsin);
		ssock = accept(msock, (struct sockaddr *)&fsin, &alen);
		if (ssock < 0) {
			if (errno == EINTR)
				continue;
			errexit("accept: %s\n", strerror(errno));
		}
		pthread_mutex_lock(&stats.st_mutex);
		stats.sockfd[i]=ssock;
		printf("got %d\n",ssock);
		fflush(stdout);
		stats.st_concount++;
	  pthread_mutex_unlock(&stats.st_mutex);
		if(i==MAXSOCK-1)
			i=0;
		else
			i++;
		pthread_cond_signal(&cond);
	}
}

/*------------------------------------------------------------------------
 * TCPechod - echo data until end of file
 *------------------------------------------------------------------------
 */
int
TCPechod()
{
	time_t	start;
	char	buf[BUFSIZ];
	int	cc;
	int fd;
	struct tcp_info info;
  int len=sizeof(info);
	while(1){
	start = time(0);
	(void) pthread_mutex_lock(&stats.st_mutex);
	printf("waiting");
	fflush(stdout);
	while(stats.st_concount==0)
	pthread_cond_wait(&cond,&stats.st_mutex);
	fd = stats.sockfd[stats.i];
	stats.st_concount--;
	stats.st_concurrent++;
	if(stats.i==MAXSOCK-1)
		stats.i=0;
	else
		stats.i++;
	(void) pthread_mutex_unlock(&stats.st_mutex);
	printf("sub running");
		fflush(stdout);
	while (cc = read(fd, buf, sizeof buf)) {
		printf("%d",fd);
		fflush(stdout);
		if (write(fd, buf, cc) < 0)
			errexit("echo write: %s\n", strerror(errno));
		(void) pthread_mutex_lock(&stats.st_mutex);
		stats.st_bytecount += cc;
		(void) pthread_mutex_unlock(&stats.st_mutex);
	}
	if (cc < 0)
		errexit("echo read: %s\n", strerror(errno));
	/*
	getsockopt(fd, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len);
	if((info.tcpi_state!=TCP_ESTABLISHED)){
		printf("connection close\n");
		break;
	}
	*/
	(void) close(fd);
	(void) pthread_mutex_lock(&stats.st_mutex);
	stats.st_concurrent--;
  stats.st_contotal++;
	stats.st_contime += time(0) - start;
	(void) pthread_mutex_unlock(&stats.st_mutex);
	/* pthread_kill(tid,10); */
	}

}

/*------------------------------------------------------------------------
 * prstats - print server statistical data
 *------------------------------------------------------------------------
 */
void
prstats(void)
{
	time_t	now;

	while (1) {
		(void) sleep(INTERVAL);

		(void) pthread_mutex_lock(&stats.st_mutex);
		now = time(0);
		(void) printf("--- %s", ctime(&now));
		(void) printf("%-32s: %u\n", "Current connections",
			stats.st_concurrent);
		(void) printf("%-32s: %u\n", "Completed connections",
			stats.st_contotal);
		if (stats.st_contotal) {
			(void) printf("%-32s: %.2f (secs)\n",
				"Average complete connection time",
				(float)stats.st_contime /
				(float)stats.st_contotal);
			(void) printf("%-32s: %.2f\n",
				"Average byte count",
				(float)stats.st_bytecount /
				(float)(stats.st_contotal +
				stats.st_concount));
		}
		(void) printf("%-32s: %lu\n\n", "Total byte count",
			stats.st_bytecount);
		(void) pthread_mutex_unlock(&stats.st_mutex);

	}
}
