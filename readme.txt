///////////////////////
//   HOW TO BUILD   //
/////////////////////

STEPS:
1. Extract to an appropriate folder inside the chai3d folder before making/running. (chai3d-3.2.0/aFolder/HapticBombDefuser)
2. Build the project by using the 'make' command in the command line.
3. Run ./bomb inside chai3d-3.2.0/bin/lin-x86_64

Dependencies:
	chai3d
	http://www.chai3d.org


Makefile build commands:
	make
		Builds the project and creates directory for object files
	make clean
		Deletes executable, object files and object directory

Run command:
	./bomb

///////////////////////
//  HOW TO USE  //
/////////////////////

Key commands:
  On Falcon:
  [+]\[-] = Rotate bomb
  [swirl] = Cut wires

  S = Toggle strike counter (3 or infinite lives)
  F = Toggle full screen
  M = Mirrored display
  Esc = Exit