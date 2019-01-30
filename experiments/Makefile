
all : usb-bird-bit checksum format

usb-bird-bit : usb-bird-bit.cpp
	clang++ -o '$@' -Wall -Werror -std=c++14 '$<' -I/usr/local/include/libusb-1.0 -lusb-1.0

checksum : checksum.cpp
	clang++ -o '$@' -Wall -Werror -std=c++14 '$<' -I/usr/local/include/libusb-1.0 -lusb-1.0

format : format.cpp
	clang++ -o '$@' -Wall -Werror -std=c++14 '$<' -I/usr/local/include/libusb-1.0 -lusb-1.0
