On MacOS comile with:

g++ -std=c++11 bgsub.cpp $(pkg-config --cflags --libs opencv4) -o bgsub

NOTE: The libraries MUST be listed after the source files above.

