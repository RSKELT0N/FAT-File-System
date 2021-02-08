CP = g++ -std=c++14
NAME = -o filesystem
FLAGS = -Wall -Wextra -Wconstant-conversion
SRC = *.cpp

all: build

build:
	$(CP) $(NAME) $(FLAGS) $(SRC)

clean:
	rm filesystem
	rm disk.dat
	cd ..
	rm -r .vscode
	cd src
	clear