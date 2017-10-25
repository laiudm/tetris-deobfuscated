// http://tromp.github.io/tetris.html:
/* 
This program plays the familiar game of `TETRIS' with the following features:
	outputs vt100-like escape-sequences for
		cursor positioning
		normal/reverse video
	in curses like fashion (minimal output for screen updates)
	continuously increasing speed (except in pause)
	start speed selectable by giving n as first argument, where n is the number of drops per second (default=2).
	controls also selectable by giving as the second argument a string consisting of the following 6 charachters: left,rotate,right,drop,pause,quit (default="jkl pq")
	the screen is blanked during the pause and the score is shown
	finally, the program maintains a high-score table. giving a full path name for the table will result in a system-wide hiscore allowing a competition between users.

Obfuscation has been achieved by:
	making effects of signals hard to trace
	implicit flushing by getchar()
	tricky cursor-parking
	minimizing code length
	hard coding include-file constants
	faking include-file structures
	throwing portability out of the window

long h[4];t(){h[3]-=h[3]/3000;setitimer(0,h,0);}c,d,l,v[]={(int)t,0,2},w,s,I,K
=0,i=276,j,k,q[276],Q[276],*n=q,*m,x=17,f[]={7,-13,-12,1,8,-11,-12,-1,9,-1,1,
12,3,-13,-12,-1,12,-1,11,1,15,-1,13,1,18,-1,1,2,0,-12,-1,11,1,-12,1,13,10,-12,
1,12,11,-12,-1,1,2,-12,-1,12,13,-12,12,13,14,-11,-1,1,4,-13,-12,12,16,-11,-12,
12,17,-13,1,-1,5,-12,12,11,6,-12,12,24};u(){for(i=11;++i<264;)if((k=q[i])-Q[i]
){Q[i]=k;if(i-++I||i%12<1)printf("\033[%d;%dH",(I=i)/12,i%12*2+28);printf(
"\033[%dm  "+(K-k?0:5),k);K=k;}Q[263]=c=getchar();}G(b){for(i=4;i--;)if(q[i?b+
n[i]:b])return 0;return 1;}g(b){for(i=4;i--;q[i?x+n[i]:x]=b);}main(C,V,a)char*
*V,*a;{h[3]=1000000/(l=C>1?atoi(V[1]):2);for(a=C>2?V[2]:"jkl pq";i;i--)*n++=i<
25||i%12<2?7:0;srand(getpid());system("stty cbreak -echo stop u");sigvec(14,v,
0);t();puts("\033[H\033[J");for(n=f+rand()%7*4;;g(7),u(),g(0)){if(c<0){if(G(x+
12))x+=12;else{g(7);++w;for(j=0;j<252;j=12*(j/12+1))for(;q[++j];)if(j%12==10){
for(;j%12;q[j--]=0);u();for(;--j;q[j+12]=q[j]);u();}n=f+rand()%7*4;G(x=17)||(c
=a[5]);}}if(c==*a)G(--x)||++x;if(c==a[1])n=f+4**(m=n),G(x)||(n=m);if(c==a[2])G
(++x)||--x;if(c==a[3])for(;G(x+12);++w)x+=12;if(c==a[4]||c==a[5]){s=sigblock(
8192);printf("\033[H\033[J\033[0m%d\n",w);if(c==a[5])break;for(j=264;j--;Q[j]=
0);while(getchar()-a[4]);puts("\033[H\033[J\033[7m");sigsetmask(s);}}d=popen(
"stty -cbreak echo stop \023;sort -mnr -o HI - HI;cat HI","w");fprintf(d,
"%4d from level %1d by %s\n",w,l,getlogin());pclose(d);}
*/

// vdu codes: https://vt100.net/docs/vt100-ug/chapter3.html#S3.3

// 12 across - including borders, 1 each side
// about 21 high .
// 12 * 22 = 264
// 12 * 23 = 276


#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

long duration[4];

void timerCallback(int signum){
	duration[3]-=duration[3]/3000;
	setitimer(0,duration,0);
}

int c,d,l,
//v[]={	// http://man7.org/linux/man-pages/man2/sigaction.2.html
//	(int)t,		// void     (*sa_handler)(int);
//	0,			// void     (*sa_sigaction)(int, siginfo_t *, void *);
//	0,			// sigset_t   sa_mask;
//	2,			// int        sa_flags;
//	0			// void     (*sa_restorer)(void);
//},
score,
s,
I,
K=0,i=276,j,k,
q[276],
Q[276],
*n=q,*m,x=17,
f[]={7,-13,-12,1,8,-11,-12,-1,9,-1,1,
12,3,-13,-12,-1,12,-1,11,1,15,-1,13,1,18,-1,1,2,0,-12,-1,11,1,-12,1,13,10,-12,
1,12,11,-12,-1,1,2,-12,-1,12,13,-12,12,13,14,-11,-1,1,4,-13,-12,12,16,-11,-12,
12,17,-13,1,-1,5,-12,12,11,6,-12,12,24};

struct sigaction sa;

void u() {
	for(i=11;++i<264;)
		if((k=q[i])-Q[i]) {
			Q[i]=k;
			if(i - ++I || i%12<1)
				printf("\033[%d;%dH",(I=i)/12,i%12*2+28);		// direct cursor addressing line, column
			printf("\033[%dm  "+(K-k?0:5),k);			// set Character Attributes; search for "Character Attributes" in vt100 spec
														// 1 = Bold on, 4 = Underscore on, 5 = Blink on, 7 Reverse video on
														// don't understand the + though...Ohh, maybe that's an offset into the string!?
			K=k;
		}
	Q[263]=c=getchar();
}

int G(b){ 
	for(i=4;i--;)
		if(q[i?b+n[i]:b])
			return 0;
		return 1;
}

void g(b){
	for(i=4;i--;q[i?x+n[i]:x]=b);
}

int main(int argc, char **argv) {
	char *a;
	FILE *ff;
	duration[3]=1000000/(l=argc>1?atoi(argv[1]):2);	// start speed selectable by giving n as first argument, where n is the number of drops per second (default=2).
	for(a=argc>2?argv[2]:"jkl pq";i;i--)		// controls selectable by optional 2nd argument a string consisting of 6 characters: left,rotate,right,drop,pause,quit (default="jkl pq")
		*n++=i<25||i%12<2?7:0;srand(getpid());
	system("stty cbreak -echo stop u");
	
	memset(&sa, 0, sizeof sa);
	sa.sa_handler = timerCallback;
	sa.sa_flags = 2;
	sigaction(14,&sa,0);		// original was sigvec(14,v,0); but glibc library not available.
	timerCallback(0);
	puts("\033[H\033[J");		// [H = direct cursor addr (0,0); [J = erase from cursor to end of screen
	for(n=f+rand()%7*4;;g(7),u(),g(0)) {
		if(c<0){
			if(G(x+12))
				x+=12;
			else {
				g(7);
				++score;
				for(j=0;j<252;j=12*(j/12+1))
					for(;q[++j];)
						if(j%12==10){
							for(;j%12;q[j--]=0);
							u();
							for(;--j;q[j+12]=q[j]);
							u();
						}
						n=f+rand()%7*4;
						G(x=17)||(c=a[5]);
			}
		}
		if(c==*a)		// left
			G(--x)||++x;
		if(c==a[1])	// rotate
			n=f+4**(m=n),G(x)||(n=m);
		if(c==a[2])	// right
			G(++x)||--x;
		if(c==a[3])	// drop
			for(;G(x+12);++score)
				x+=12;
		if(c==a[4]||c==a[5]) {	// pause or quit
			s=sigblock(8192);
			printf("\033[H\033[J\033[0m%d\n",score);	// [H = cursor to (0,0); clear screen; set Character Attributes; output score
			if(c==a[5])			// quit
				break;
			for(j=264;j--;Q[j]=0);
			while(getchar()-a[4]);
			puts("\033[H\033[J\033[7m");		// [H = cursor to (0,0); clear screen; set Character Attributes
			sigsetmask(s);
		}
	};
	
	system("stty -cbreak echo stop \023");
	ff=popen("sort -mnr -o HI - HI;cat HI","w");
	fprintf(ff,"%4d from level %1d by %s\n",score,l,getlogin());
	pclose(ff);
}