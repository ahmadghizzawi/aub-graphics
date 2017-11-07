# Assignment 4

## Run Instructions
    $ make
    $ ./main

## Changes
### Task 1
- Integrated the code in asst4-snippets.cpp as described in the document.
### Task 2: Picker
When visiting a shape node, we applied the following snippet:
    // increment id counter
    idCounter_++;
    // Get the parent node (which should be an SgRbtNode object)
    shared_ptr<SgRbtNode> parent = dynamic_pointer_cast<SgRbtNode>(nodeStack_.back());
    // Associate the id with the parent
    addToMap(idCounter_, parent);
    // Retrieve the color of the object based on the id
    Cvec3 objectColor = Picker::idToColor(idCounter_);
    // Set the uniform variable to have the given color before drawing.
    safe_glUniform3f(drawer_.getCurSS().h_uIdColor, objectColor[0], objectColor[1],
                     objectColor[2]);

### Task 3: Transforming

### Task 4: Building the robot.
We constructed a robot that included the listed parts. Here is the code snippet
that we wrote to do this (taken from constructRobot function in main.cpp):
    ShapeDesc shapeDesc[NUM_SHAPES] = {
      {0, 0,         0, 0, TORSO_WIDTH, TORSO_LEN, TORSO_THICK, g_cube}, // torso
      {1, ARM_LEN/2, 0, 0, ARM_LEN, ARM_THICK, ARM_THICK, g_cube}, // upper right arm
      {2, ARM_LEN/2, 0, 0, ARM_LEN, ARM_THICK, ARM_THICK, g_cube}, // lower right arm
      {3, 0, HEAD_RAD, 0, HEAD_RAD, HEAD_RAD, HEAD_RAD, g_arcball}, // head
      {4, -ARM_LEN/2, 0, 0, ARM_LEN, ARM_THICK, ARM_THICK, g_cube}, // upper left arm
      {5, -ARM_LEN/2, 0, 0, ARM_LEN, ARM_THICK, ARM_THICK, g_cube}, // lower left arm
      {6, 0, -LEG_LEN/2, 0, LEG_THICK, LEG_LEN, LEG_THICK, g_cube}, // upper right Leg
      {7, 0, -LEG_LEN/2, 0, LEG_THICK, LEG_LEN, LEG_THICK, g_cube}, // lower right Leg
      {8, 0, -LEG_LEN/2, 0, LEG_THICK, LEG_LEN, LEG_THICK, g_cube}, // upper left Leg
      {9, 0, -LEG_LEN/2, 0, LEG_THICK, LEG_LEN, LEG_THICK, g_cube}, // lower left Leg
    };

    JointDesc jointDesc[NUM_JOINTS] = {
      {-1}, // torso
      {0,  TORSO_WIDTH/2, TORSO_LEN/2, 0},  // upper right arm
      {1,  ARM_LEN, 0, 0},                  // lower right arm
      {0, 0, TORSO_LEN/2, 0},               // head
      {0,  -TORSO_WIDTH/2, TORSO_LEN/2, 0}, // upper left arm
      {4,  -ARM_LEN, 0, 0},                 // lower left arm
      {0,  TORSO_WIDTH/2, -TORSO_LEN/2, 0}, // upper right leg
      {6,  0, -LEG_LEN, 0},                 // lower right leg
      {0,  -TORSO_WIDTH/2, -TORSO_LEN/2, 0},// upper left leg
      {8,  0, -LEG_LEN, 0}                  // lower left leg
    };

## Known Issues:
- When using the spacebar + right mouse click combo to translate in the
  Z-directions, the behavior is not the expected one.

- When picking the sky using the picker interface, an error occurs. To reproduce
the error, press 'p' and then click on the blue section of the screen. However,
you can select the sky by clicking on the ground instead.
