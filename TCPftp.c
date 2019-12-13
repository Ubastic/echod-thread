/* TCPftp.c - main, TCPftp */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

int	TCPftp(const char *host, const char *service);
int	errexit(const char *format, ...);
int	connectTCP(const char *host, const char *service);

#define errno 1
#define	LINELEN		128
#define BUFSIZE        128
/*------------------------------------------------------------------------
 * main - TCP client for tiny ftp service
 *------------------------------------------------------------------------
 */
int
main(int argc, char *argv[])
{
	char	*host = "localhost";	/* host to use if none supplied	*/
	char	*service = "ftp";	/* default service name		*/
	char	filename[LINELEN+1]="";	/* buffer for one line of text	*/

	switch (argc) {
	case 3:
		host = argv[2];
	case 2:
		strncpy(filename,argv[1],strlen(argv[1]));
		break;
		/* FALL THROUGH */
	default:
		fprintf(stderr, "usage:TCPftp filename <IP> \n");
		exit(1);
	}
        printf("%s\n",filename);
	FILE *fp = fopen(filename,"wb"); /* ab */
	if(fp==NULL){
	    perror("can't open file\n");
	    exit(1);
	}

	TCPfile(host,service, filename,fp);
	exit(0);
}

/*------------------------------------------------------------------------
 * TCPftp - send filename to ftp service on specified host and save file
 *------------------------------------------------------------------------
 */
int
TCPfile(const char *host, const char * service,char * filename,FILE *fp)
{

	int	s, n;			/* socket descriptor, read count*/
	int	outchars, inchars;	/* characters sent and received	*/
	char buff[BUFSIZE] ={0};
	s = connectTCP(host, service);
	filename[LINELEN] = '\0';  /* insure line null-terminated	*/
        outchars = strlen(filename);
	(void) write(s, filename, outchars);
	while(n = read(s, buff, BUFSIZE)){
                /* printf("%s",buff); */
		/* read file back */
		if(fwrite(buff,1,n,fp)!=n){
		 perror("write file error\n");
		 exit(1);
		}
		printf("writing file...%d\n",n);
		memset(buff,0,BUFSIZE);
	}
	if (n < 0){
			errexit("socket read failed: %s\n",
			strerror(errno));
	}
}
