# CMPS385: Assignment 6
## Authors:
Ahmad Ghizzawi and Chukri Soueidi.  

## Run Instructions
    $ make
    $ ./main

## Changes Overview 
### Task 1:
- We followed the instructions in asst6-snippets.cpp
- It was straighforward with few errors here and there
### Task 2:
- We read Chapters 14 and 15
- We created a new texture normal vec4 variable inside the normal fragment shader from _uTexNormal_, _vTexCoord_
- We scaled the newly created normal by multiplying with 2 and subtratcing one to convert it from range [0,1] to [-1,1]
- We used the formula provided to calculate the new normal. vNTMat holds already the inv(M)^t*T*n
