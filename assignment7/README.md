# CMPS385: Assignment 7
## Authors:
Ahmad Ghizzawi and Chukri Soueidi.  

## Run Instructions
    $ make
    $ ./main

## Changes Overview 

##Part 1
### Task 1
- We read the cube.mesh and converted the quads to triangles by dividing each quad into 2 trianle in _getVertices()_
- We added the cube to the scenegraph 
### Task 2
- We recaluclated new normals at the vertices by adding to each vertex's normal the face normal then we divided by 2. 
### Task 3
- Added a sin function that animates the cube using a GLUT timer. The sin function consider the vertex and the time on order not to have a zoom in zoom out effect.
##Part 2:
### Task 1
- We copied the snippets into the project and it ran without any errors.
- We implemented _updateShellGeometry()_ to render the hair on every vertex. 
- We implemented initial version of function _updateShellGeometry()_ that uses _VertexPNX_ to assign the required textures
### Task 2
- We implemented _hairsSimulationCallback()_ that updates _g tipPos_ and _g tipVelocity_
- We added a new global bool _gShellNeedsUpdate_  in order to avoid updating the shells when not needed.
### Task 3
 - We updated _updateShellGeometry()_ to show the hair curve based on the figure in the assignment.
 - We added a new data structure, an array of Cvec3 vectors _gcalculatedShellVertices_ in order to keep all calculated points on each shell.
 
##Difficulties/Assumptions:
- In Part 1, task 2: We divided by 2 instead of calculating the valency at each vertex.
- In Part 2, task 3: We derived d and got _((t - p - n * gnumShells) /  (gnumShells))_ The hairs were rendering too long so we minimized it to become _((t - p - n * gnumShells) /  (gnumShells * gnumShells/2))_
- We used make OPT=1 since the code was running very slow without it.