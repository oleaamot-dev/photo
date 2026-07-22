int	changespeed P__((int, int));
int	opentty P__((char *));
int	readtty P__((int, u_char *, int));
void	flushtty P__((int));

#define	writetty(fd, b, l)	write(fd, b, l)
#define	closetty(fd)		close(fd)
