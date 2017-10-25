#include <signal.h>


static void handler(int signum)
{
    /* Take appropriate actions for signal delivery */
}


int main()
{
    struct sigaction sa;


    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; /* Restart functions if
                                 interrupted by handler */
    if (sigaction(SIGINT, &sa, NULL) == -1)
        /* Handle error */;


    /* Further code */
}