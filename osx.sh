gcc midifile.c midiutil.c main.c -o midisys -g -Wall -Wno-unused-variable -L. libbass.dylib libGLEW.a libassimp.3.dylib libSOIL.a -framework OpenGL -framework GLUT -framework Cocoa
