# La Femme Rouge (LFR)
La Femme Rouge is a minimal graph based scripting system intended for games.

![Example screenshot demonstrating a lot of nodes](doc/images/lfr_example.png)

## Features
 - Single header C file (with minimal dependencies)
 - Editor included (in separare header)
 - Nodes with program flow and data connections
 - Handfull of core instructions (math, debugging)
 - Supports adding custom instructions
 - All public functions should be documented (function without doc = 🐛)

## Build and run
Get your self all the dependencies, then do the following from the projects root folder.

```bash
$ make -C src
$ ./bin/game examples/game_script.txt
```

## Dependencies
The LFR 'runtime' single header file `lfr.h` has no dependencies exept for a handfull of standard C headers.

The editor uses [Nuklear](https://github.com/Immediate-Mode-UI/Nuklear) for UI widgets.

Both example applications use OpenGL, [GLEW](http://glew.sourceforge.net/) and [GLFW](https://www.glfw.org/).
The later two are both available from [Homebrew](https://brew.sh/) for macOS users.
There are most likely packages for your favourite flavour of Linux.
(It you're on Windows then I have no idea what you need to do 🤷‍♂️)

Examples are built with good ol' [GNU make](https://www.gnu.org/software/make/),
but you can use whatever you want for your own project since LFR is a header-only lib.

## Backlog / Whishlist
This project is still in it's infancy. Here are a few of the most obvious things that would be nice to have.

 - Mathematical nodes can use all numeric types (`int`, `float`, `vec2`)
 - Node instruction for division `/` (supporting all relevant types)
 - A `vec3` type (with appropriate support from existing math instructions)
 - Instructions for `bool` type (`and`, `or`, `not`)
 - Numerical (`int`, `float`, `vec2`) and logical (`bool`) expression instructions allowing for more sophisticated calculations in a single node
 - Join-instruction that forwards first once all nodes before it in the flow are done
 - Example using C++ (get the ` #ifdef __cplusplus__` exporting/linking thing right)
 - Editor safety checks that prevent users from connecting data slots of the wrong types
 - Editor debug console (for `print_own_id` and  `print_value` instructions)
 - Editor does some of the node placement and rearranging for you (focus on your game, not on unwrangling node scripts)
 - Test on other compilers than Clang
 - Automated builds on macOS/Linux/Windows
 - Hide data slots in node windows (like for flow links)
 - Draw a line (on top of everything else) when linking
 - Find a good way to exit linking mode(s)
 - Various other UI tweaks
 - Automated tests (where it makes most sense)

## In the wild
It would be really cool to know if anyone actually uses this for anything.
If you do then please don't hesitate to submitt a PR that adds your project
to the list below.

 - [Example project name](https://github.com/jordgubben/la-femme-rouge/) A short description of your project.
