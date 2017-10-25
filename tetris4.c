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

// just can't get it to compile with -std=c99 compile option - errors with sigaction, popen, pclose definitions
// it works fine with default settings; can't understand what's different...

// fun debug technique - redirect stderr messages to another screen:
// 1. find the device name of the other screen by typing "tty". It will o/p eg /dev/pts/1
// 2. on cmd line redirect any error output to this device - eg ./kilo 2>/dev/pts/1
// 3. write debug messages to stderr, eg by using the debug macro below


// for the 2x defines below, see http://man7.org/linux/man-pages/man7/feature_test_macros.7.html
#define _POSIX_C_SOURCE 199309L	
#define _BSD_SOURCE

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#include <stddef.h>	// for offsetof macro

// following inspired by http://stackoverflow.com/questions/3576396/variadic-macros-with-0-arguments-in-c99
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define debug(...) fprintf(stderr, "\nDEBUG " __FILE__ ":" TOSTRING(__LINE__) ": " __VA_ARGS__);

struct itimerval delay;

void timerCallback(int signum) {
	//debug("called back\n");
	delay.it_value.tv_usec -= delay.it_value.tv_usec/3000;
	setitimer(ITIMER_REAL, &delay, NULL);
}

	//SA_NOCLDSTOP is 1
	//SA_NOCLDWAIT  is 2
	//SA_NODEFER is 1073741824
	//SA_ONSTACK is 134217728
	//SA_RESETHAND is -2147483648
	//SA_RESTART is 268435456
	//SA_SIGINFO  is 4
	
void setTimer(void) {	// timer also forces getchar() to return
	struct sigaction sa;
	struct sigaction sOld;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = timerCallback;
	sa.sa_flags = 0;//was SA_NOCLDWAIT; SA_RESTART stops the alarm signal from cancelling the getchar() call (as expected)
	//signal(SIGALRM, timerCallback);	// timer ticks, but doesn't cause getchar() to timeout
	sigaction(SIGALRM, &sa, &sOld);		// original was sigvec(14,v,0); but glibc library not available.
	//debug("Old flags: %i\n", sOld.sa_flags);	// if signal() called 1st then SA_RESTART & SA_NODEFER are set; otherwise 0... Problem finally resolved.

	timerCallback(0);
}

int inChar,
level,
score,
grid[276],
displayShadow[276],
*piece,
x=17,		// starting position for piece at top of screen
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



void updateDisplay(void) {
	int currentCharAttr = -1;
	int currentCursorAddr = -1;
	for(int offset=11;++offset<264;) {
		int newCharAttr=grid[offset];
		if(newCharAttr != displayShadow[offset]) {
			// mismatch between virtual Display and real display, update the physical display
			displayShadow[offset]=newCharAttr;
			
			// need to update the cursor address?
			if( (offset != ++currentCursorAddr) || offset%12<1) {		// update if different addr, or it's on a new line
				currentCursorAddr=offset;
				printf("\033[%d;%dH",offset/12,offset%12*2+28);		// direct cursor addressing line, column
			}
			// need to update the attribute
			if (currentCharAttr != newCharAttr) {
				printf("\033[%dm", newCharAttr);
			}
			currentCharAttr=newCharAttr;
			
			// finally update the screen
			printf("  ");
		}
	}
	inChar=getchar();			// also stalls refresh by the timeout duration
	displayShadow[263] =	-1;	// "dirty" the last position to force the cursor there on repaint.
}

int isValidPosition(int b){
	for(int i=4;i--;) {
		if(grid[i?b+piece[i]:b])	// offset 0 points to the next rotation, replace by 0 offset
			return 0;
	}
	return 1;
}

void updatePosition(int b){
	for(int i=4;i--;)
		grid[i?x+piece[i]:x]=b;
}

int main(int argc, char **argv) {
	
	//printf("offsetof it_value.tv_sec is %i\n", offsetof(struct itimerval, it_value.tv_sec)/4); 
	//printf("offsetof it_value.tv_usec is %i\n", offsetof(struct itimerval, it_value.tv_usec)/4);
	//printf("offsetof it_interval.tv_sec is %i\n", offsetof(struct itimerval, it_interval.tv_sec)/4);
	//printf("offsetof it_interval.tv_usec is %i\n", offsetof(struct itimerval, it_interval.tv_usec)/4);
	
	//printf("SIGALRM       is %i\n", SIGALRM     );

	delay.it_value.tv_usec = 1000000/(level=argc>1?atoi(argv[1]):2); // start speed selectable by giving n as first argument, where n is the number of drops per second (default=2).
	char *keySel=argc>2?argv[2]:"jkl pq";	// controls selectable by optional 2nd argument a string consisting of 6 characters: left,rotate,right,drop,pause,quit (default="jkl pq")
	
	int *nn = grid;
	for(int i=276;i;i--) {
		*nn++ = i<25||i%12<2 ? 7 : 0;	// draw the border, 7 = reverse video attribute
	}
	
	srand(getpid());
	system("stty cbreak -echo stop u");
	
	setTimer();
	
	//debug("EINTR has value %i\n", EINTR); // outputs 4
	
	puts("\033[H\033[J");		// [H = direct cursor addr (0,0); [J = erase from cursor to end of screen
	piece=pieces+rand()%7*4;
	for(;;) {
		if(inChar<0){
			//debug("errno is %i\n", errno);	// outputs 4
			if(isValidPosition(x+12)) {	// piece can move down, so do so.
				x += 12;
			} else {
				updatePosition(7);
				++score;
				for(int j=0; j<252; j=12*(j/12+1)) {
					// scan the row, rely on the border to stop the loop
					for(; grid[++j];) {	
						if(j%12==10){
							// the row is full, clear the row
							for(;j%12; j--)
								grid[j]=0;
							updateDisplay();
							// move the rest of the display down by a row to fill the bank line
							for(;--j; )
								grid[j+12]=grid[j];
							updateDisplay();
						}
					}

				}
									// now choose a new random piece
					piece=pieces+rand()%7*4;
					if(!isValidPosition(x=17))
						inChar=keySel[5];	// if new piece can't be placed, simulate quit key pressed
			}
		}
		if(inChar==*keySel) {// left
			if(!isValidPosition(--x))
				++x;
		}
		if(inChar==keySel[1]) {	// rotate
			int *savePiece = piece;
			piece=pieces+4*piece[0];	// first position gives the index to the pieces next rotational orientation
			if(!isValidPosition(x))
				(piece=savePiece);
		}
		if(inChar==keySel[2]){	// right
			if(!isValidPosition(++x))
				--x;
		}
		if(inChar==keySel[3]) {	// drop - keep moving down until no longer valid
			for(;isValidPosition(x+12);) {
				x+=12;
				++score;
			}
		}
		if(inChar==keySel[4]||inChar==keySel[5]) {	// pause or quit
		
			int s=sigblock(8192);		// Hex 2000	- turn timer off? http://unix.superglobalmegacorp.com/Net2/newsrc/sys/signal.h.html - #define	SIGALRM	14
			printf("\033[H\033[J\033[0m%d\n",score);	// [H = cursor to (0,0); clear screen; set Character Attributes; output score
			if(inChar==keySel[5])			// quit
				break;
			int j;
			for(j=264;j--;)	// reset the displayBuff so that it's completely redrawn
				displayShadow[j]=0;
			while(getchar() != keySel[4]);
			puts("\033[H\033[J\033[7m");		// [H = cursor to (0,0); clear screen; set Character Attributes - 7=Reverse Video On
			sigsetmask(s);
		}
		updatePosition(7);
		updateDisplay();
		updatePosition(0);
	};
	
	system("stty -cbreak echo stop \023");
	FILE *ff=popen("sort -mnr -o HI - HI; cat HI","w");
	fprintf(ff,"%4d from level %1d by %s\n",score,level,getlogin());
	// -m = merge
	// -n = numeric sort
	// -r = reverse the result of comparisons
	// -o HI = output file
	// - H = read standard input, then file HI
	// So it reads from stdin (the pipe) until it closes, then it reads HI by merging and the re-writes the file. Finally it displays the file.
	pclose(ff);
	return 0;
}