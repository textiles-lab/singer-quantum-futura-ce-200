#include "send-path.hpp"
#include <cassert>
#include <iostream>
#include <unistd.h>

int main(int argc, char **argv) {
	std::vector< int32_t > xys;

	{ // input list of xys:
		std::string tok;
		while (std::cin >> tok) {
			xys.emplace_back(std::stoi(tok,0,0));
			std::cout << to_hex16(xys.back()) << std::endl; //DEBUG
		}
	}

	//---- libusb init ----

	libusb_context *ctx = nullptr;
	{ //init:
		int ret = libusb_init(&ctx);
		if (ret != 0) {
			throw std::runtime_error("Error initializing libusb: " + std::string(libusb_strerror((libusb_error)ret)));
		}
		libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, 10000);
	}

	//---- device init ----

	libusb_device_handle *dev = open_machine(ctx);

	do_handshake(dev);

	//---- send path ----

	send_path(dev, xys);

	//---- wait for path to finish ----
	while (1) {
		poll_index(dev, 0x8001, 1); //<-- returns 0f when not sewing 4f (6f?) when sewing, 2f when done(?)
		poll_index(dev, 0x8001, 1);
		poll_index(dev, 0x8101, 1);
		poll_index(dev, 0x8201, 1);

		sleep(1);
	}

}
