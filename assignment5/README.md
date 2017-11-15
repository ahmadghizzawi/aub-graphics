# CMPS385: Assignment 4
## Authors:
Ahmad Ghizzawi and Chukri Soueidi.  

## Run Instructions
    $ make
    $ ./main

## Changes
### Task 1:
Added global vars:

    // Keyframe Animation variables 
    static vector<RigTForm> g_frames;
    static vector<shared_ptr<SgRbtNode> > g_rbtNodes;

## Known Issues:
- When using the spacebar + right mouse click combo to translate in the
  Z-directions, the behavior is not the expected one.

- When picking the sky using the picker interface, an error occurs. To reproduce
the error, press 'p' and then click on the blue section of the screen. However,
you can select the sky by clicking on the ground instead.
