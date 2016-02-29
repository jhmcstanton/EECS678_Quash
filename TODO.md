# TODO

All of these are pulled from the rubric.

1. ~~Executables~~
  1. ~~Without args~~
  1. ~~With args~~
  1. ~~Allows background processes~~

  ~~Background processes currently don't wait, which causes zombie processes! This should be fixed~~
  
1. ~~*exit* and *quit* commands~~
1. ~~Work location management~~
  1. ~~cd (with and without args) (chdir function? Could it be that easy?)~~
  1. ~~pwd (possibly just need to keep track of starting location and moves in a variable)~~
1. ~~Set for environmental variables~~

   ~~Specifically *HOME* and *PATH*~~ Works for arbitrary variables.
   
1. ~~echo~~

  Works with arbitrary variables (even in embedded)

1. ~~Jobs~~
  
1. ~~Process inheritance~~
 
  Environmental variables are passed to child processes. I assume that this is only
  the initial state of the process environmental variables and that parent and child
  can diverge.

  Seems to be working!


1. ~~Background / Foreground execution~~
1. ~~Pipes~~
1. Project Report

## Bonus Stuff

1. ~~Multiple pipes (shouldn't be too bad)~~
1. ~~kill command~~