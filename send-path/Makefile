
#From: https://stackoverflow.com/questions/714100/os-detecting-makefile
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	CPP=g++ -std=c++14 -Wall -Werror -g -O2 -I/usr/include/libusb-1.0
else
	CPP=clang++ -std=c++14 -Wall -Werror -g -O2 -I/usr/local/include/libusb-1.0
endif


send-path : send-path.cpp send-path.hpp
	$(CPP) -o '$@' '$<' -lusb-1.0
