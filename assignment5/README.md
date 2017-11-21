# CMPS385: Assignment 5
## Authors:
Ahmad Ghizzawi and Chukri Soueidi.  

## Run Instructions
    $ make
    $ ./main

## Changes Overview
### Part 1:
#### Task 1:
- Imported sgutils.h and looked at the dumpSgRbtNodes(..) function.
- Read and used the data structure (list) suggested in the appendix.
- Added global variables _g_currentKeyFrame_, an iterator of _g_keyFrames_ (a list of RigTForm vectors), and _g_rbtNodes_ (vector of pointers to SgRbtNodes).
- Implemented the hotkeys required. N.B.: Keys 'i' and 'w' import and export a file *key.frames*, which is expected to be located in the cwd. This required adding and implementing serialize/deserialize functions in quat.h, cvec3.h, and rigTForm.
#### Task 2:
- Added the function pow and cn to quat.h.
- Implemented slerp() and lerp() functions and added a function called slerpLerp() for convenience. We referred to the book for the exact implementation.
#### Task 3:
- Added and implemented the functions animateTimerCallback() and interpolateAndDisplay() as suggested.
- Implemented the hotkeys required to play/stop as well as '+' and '-' by decreasing/increasing _g_msBetweenKeyFrames_.
### Part 2:
- Added and implemented the functions catmullRomInterpolate() and catmullRomInterpolateAndDisplay() that animate using Catmull-Rom splines as explained in Chapter 9 of the book.
- Added the hotkey 't' to toggle between the linear and Catmull-Rom interpolation. Please note that Catmull-Rom is the default value.
