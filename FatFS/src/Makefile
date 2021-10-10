CP = g++ -std=c++14
NAME = -o filesystem
FLAGS = -Wall -Wextra -Wconstant-conversion
DISK = Disk.cpp
LOG = Log.cpp
MAIN = main.cpp
TERMINAL = Terminal.cpp

all: build

build:
	$(CP) -c $(FLAGS) $(DISK)
	$(CP) -c $(FLAGS) $(LOG)
	$(CP) -c $(FLAGS) $(TERMINAL)
	$(CP) -c $(FLAGS) $(MAIN)
	mkdir ObjectFiles
	mv *.o ObjectFiles
	$(CP) $(NAME) $(FLAGS) ObjectFiles/*.o

clean:
	rm filesystem
	rm -r ObjectFiles/
	rm disk.dat
	cd ..
	rm -r .vscode
	cd src
	clear