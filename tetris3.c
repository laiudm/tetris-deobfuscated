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

void setTimer(void) {
	//v[]={	// http://man7.org/linux/man-pages/man2/sigaction.2.html
	//	(int)t,		// void     (*sa_handler)(int);
	//	0,			// void     (*sa_sigaction)(int, siginfo_t *, void *);
	//	0,			// sigset_t   sa_mask;
	//	2,			// int        sa_flags;
	//	0			// void     (*sa_restorer)(void);
	//},	
	struct sigaction sa;
	memset(&sa, 0, sizeof sa);
	sa.sa_handler = timerCallback;
	sa.sa_flags = 2;
	sigaction(14,&sa,0);		// original was sigvec(14,v,0); but glibc library not available.
	timerCallback(0);
}

int inChar,d,level,

score,
s,
I,
i,
j,
grid[276],
displayShadow[276],
*piece,
x=17,
pieces[]={			// 1st col gives the index to the piece's next rotational orientation
	 7,-13,-12, 1,	// 0 -> 7 -> 0
	 8,-11,-12,-1,	// 1 -> 8 -> 1
	 9, -1,  1,12,	// 2 -> 10 -> 11 -> 2
	 3,-13,-12,-1,	// 3 -> 3	// to itself; must be the square
	12, -1, 11, 1,	// 4 -> 12 -> 13 -> 14 -> 4
	15, -1, 13, 1,	// 5 -> 15 -> 16 -> 17 -> 5
	18, -1,  1, 2,	// 6 -> 18 -> 6 // the first 6 are randomly selected
	
	 0,-12, -1,11,	// 7
	 1,-12,  1,13,	// 8
	10,-12,  1,12,	// 9
	11,-12, -1, 1,	// 10
	 2,-12, -1,12,	// 11
	13,-12, 12,13,	// 12
	14,-11, -1, 1,	// 13
	 4,-13,-12,12,	// 14
	16,-11,-12,12,	// 15
	17,-13,  1,-1,	// 16
	 5,-12, 12,11,	// 17
	 6,-12, 12,24};// 18



void updateDisplay() {
	int offset;
	static int K = 0;
	for(offset=11;++offset<264;) {
		int k=grid[offset];
		if(k != displayShadow[offset]) {
			displayShadow[offset]=k;
			if(offset - ++I || offset%12<1) {
				I=offset;
				printf("\033[%d;%dH",offset/12,offset%12*2+28);		// direct cursor addressing line, column
			}
			if (K == k) {
				printf("  ");
			} else {
				printf("\033[%dm  ", k);
			}
			//printf("\033[%dm  "+(K-k?0:5),k);	// set Character Attributes; search for "Character Attributes" in vt100 spec
												// 1 = Bold on, 4 = Underscore on, 5 = Blink on, 7 Reverse video on
												// don't understand the + though...Ohh, maybe that's an offset into the string!?
			K=k;
		}
	}
	displayShadow[263]=inChar=getchar();
}

int isValidPosition(b){ 
	for(i=4;i--;) {
		if(grid[i?b+piece[i]:b])
			return 0;
	}
	return 1;
}

void g(b){
	for(i=4;i--; grid[i?x+piece[i]:x]=b);
}

int main(int argc, char **argv) {
	int i;
	duration[3]=1000000/(level=argc>1?atoi(argv[1]):2);	// start speed selectable by giving n as first argument, where n is the number of drops per second (default=2).
	char *keySel=argc>2?argv[2]:"jkl pq";	// controls selectable by optional 2nd argument a string consisting of 6 characters: left,rotate,right,drop,pause,quit (default="jkl pq")
	
	int *nn = grid;
	for(i=276;i;i--) {
		*nn++=i<25||i%12<2?7:0;	// 7 = reverse video attribute
	}
	
	srand(getpid());
	system("stty cbreak -echo stop u");
	
	setTimer();
	
	puts("\033[H\033[J");		// [H = direct cursor addr (0,0); [J = erase from cursor to end of screen
	piece=pieces+rand()%7*4;
	for(;;) {
		if(inChar<0){
			if(isValidPosition(x+12))
				x+=12;
			else {
				g(7);
				++score;
				for(j=0;j<252;j=12*(j/12+1))
					for(; grid[++j];)
						if(j%12==10){
							for(;j%12; grid[j--]=0);
							updateDisplay();
							for(;--j; grid[j+12]=grid[j]);
							updateDisplay();
						}
						piece=pieces+rand()%7*4;
						isValidPosition(x=17)||(inChar=keySel[5]);	// quit?
			}
		}
		if(inChar==*keySel) // left
			isValidPosition(--x)||++x;
		if(inChar==keySel[1]) {	// rotate
			int *savePiece = piece;
			piece=pieces+4*piece[0];	// first position gives the index to the pieces next rotational orientation
			isValidPosition(x)||(piece=savePiece);
		}
		if(inChar==keySel[2])	// right
			isValidPosition(++x)||--x;
		if(inChar==keySel[3]) {	// drop
			for(;isValidPosition(x+12);++score)
				x+=12;
		}
		if(inChar==keySel[4]||inChar==keySel[5]) {	// pause or quit
		
			s=sigblock(8192);		// Hex 2000	- turn timer off? http://unix.superglobalmegacorp.com/Net2/newsrc/sys/signal.h.html - #define	SIGALRM	14
			printf("\033[H\033[J\033[0m%d\n",score);	// [H = cursor to (0,0); clear screen; set Character Attributes; output score
			if(inChar==keySel[5])			// quit
				break;
			for(j=264;j--;)	// reset the displayBuff
				displayShadow[j]=0;
			while(getchar()-keySel[4]);
			puts("\033[H\033[J\033[7m");		// [H = cursor to (0,0); clear screen; set Character Attributes - 7=Reverse Video On
			sigsetmask(s);
		}
		g(7);
		updateDisplay();
		g(0);
	};
	
	system("stty -cbreak echo stop \023");
	FILE *ff=popen("sort -mnr -o HI - HI;cat HI","w");
	fprintf(ff,"%4d from level %1d by %s\n",score,level,getlogin());
	pclose(ff);
}