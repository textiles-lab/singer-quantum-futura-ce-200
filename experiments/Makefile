
usb-bird-bit : usb-bird-bit.cpp send-path.hpp
	clang++ -o '$@' -Wall -Werror -std=c++14 '$<' -I/usr/local/include/libusb-1.0 -lusb-1.0

checksum : checksum.cpp
	clang++ -o '$@' -Wall -Werror -std=c++14 '$<' -I/usr/local/include/libusb-1.0 -lusb-1.0

format : format.cpp
	clang++ -o '$@' -Wall -Werror -std=c++14 '$<' -I/usr/local/include/libusb-1.0 -lusb-1.0

send-path : send-path.cpp
	clang++ -o '$@' -Wall -Werror -std=c++14 '$<' -I/usr/local/include/libusb-1.0 -lusb-1.0
