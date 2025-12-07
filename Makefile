all: xvisbell
xvisbell: xvisbell.cpp
	c++ -std=c++14 -Wall -Wextra -Werror -O3 -o xvisbell xvisbell.cpp -lX11
clean:
	rm -f xvisbell
install:
	install -s xvisbell /usr/local/bin
	install xflash /usr/local/bin
