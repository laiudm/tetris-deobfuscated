Tetris
======

This is deobfuscation of a tetris implementation found [here](http://tromp.github.io/tetris.html)

The final, deobfuscated code is tetris.; the original code is in tetris0.c, and intermediate versions in tetris[n].c. The final code is
c99 compliant, but all earlier versions aren't so you would need to remove the -std=c99 flag from the Makefile to compile those.

The orginal code would not run on ARM, which is not surprising as the author implemented it "throwing portability out of the window". However,
it only required a small change to get it running - replacing a call to sigvec() with sigaction() instead.

The most interesting thing is the use of alarm signals to have the calls to getchar() return on a timeout. This is accomplished by ensuring
that the signal has SA_RESTART reset, which requires the call to sigaction(). The older signal() call automatically sets SA_RESTART, so
can't be used.



