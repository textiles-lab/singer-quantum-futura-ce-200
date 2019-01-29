
#include <stdexcept>
#include <string>
#include <iostream>
#include <vector>

#include <unistd.h>
#include <libusb.h>

std::string to_hex(uint16_t value) {
	static char const *hex = "0123456789abcdef";
	std::string ret;
	ret += hex[(value / 16 / 16 / 16) % 16];
	ret += hex[(value / 16 / 16) % 16];
	ret += hex[(value / 16) % 16];
	ret += hex[(value) % 16];
	return ret;
}
std::string to_hex(uint8_t value) {
	static char const *hex = "0123456789abcdef";
	std::string ret;
	ret += hex[(value / 16) % 16];
	ret += hex[(value) % 16];
	return ret;
}

int main(int argc, char **argv) {
	libusb_context *ctx = nullptr;
	{ //init:
		int ret = libusb_init(&ctx);
		if (ret != 0) {
			throw std::runtime_error("Error initializing libusb: " + std::string(libusb_strerror((libusb_error)ret)));
		}
		libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, 10000);
	}
	
	{ //list devices
		libusb_device **devices;
		ssize_t count = libusb_get_device_list(ctx, &devices);
		if (count < 0) {
			libusb_exit(ctx);
			throw std::runtime_error("Error listing devices: " + std::string(libusb_strerror((libusb_error)count)));
		}
		for (int i = 0; devices[i] != nullptr; ++i) {
			libusb_device *dev = devices[i];
			struct libusb_device_descriptor desc;
			int ret = libusb_get_device_descriptor(dev, &desc);
			if (ret != 0) {
				throw std::runtime_error("Failed to get device descriptor: " + std::string(libusb_strerror((libusb_error)ret)));
			}
			std::cout << to_hex(desc.idVendor) << ":" << to_hex(desc.idProduct)
				<< " (bus " << (int)libusb_get_bus_number(dev) << ", device " << (int)libusb_get_device_address(dev) << ")" << std::endl;
		}
		libusb_free_device_list(devices, 1);
	}
	

	libusb_device_handle *dev = nullptr;
	{ //open embroidery machine device:
		constexpr uint16_t vendor_id = 0x1320;
		constexpr uint16_t product_id = 0x0001;
		dev = libusb_open_device_with_vid_pid(ctx, vendor_id, product_id);
		if (dev == nullptr) {
			std::cout << "Failed to open device with Vendor ID " << to_hex(vendor_id) << " and Product ID " << to_hex(product_id) << std::endl;
			return 1;
		}

		//check configuration:
		{
			int config = -1;
			int ret = libusb_get_configuration(dev, &config);
			if (ret != 0) {
				throw std::runtime_error("Failed to get configuration: " + std::string(libusb_strerror((libusb_error)ret)));
			}
			std::cout << "Configuration: " << config << std::endl;
			//reset config:
			ret = libusb_set_configuration(dev, config);
			if (ret != 0) {
				throw std::runtime_error("Failed to set configuration: " + std::string(libusb_strerror((libusb_error)ret)));
			}
		}
		{ //reset device:
			int ret = libusb_reset_device(dev);
			if (ret != 0) {
				throw std::runtime_error("Failed to reset device: " + std::string(libusb_strerror((libusb_error)ret)));
			}
	
		}

		//claim interface 0(?):
		int ret = libusb_claim_interface(dev, 0);
		if (ret != 0) {
			throw std::runtime_error("Failed to claim interface 0: " + std::string(libusb_strerror((libusb_error)ret)));
		}
	}

	auto poll_index = [&](uint16_t index, uint16_t request) {
		//some sort of version check?
		std::cout << "Index " << to_hex(index) << ": ";
		{ //ask for index:
			uint8_t data[1];
			int ret = libusb_control_transfer(dev,
				0xc0, //bmRequestType
				request, //bRequest
				0x0000, //wValue
				index, //wIndex
				data, //data
				1, //wLength
				1000 * 5 //timeout
			);
			if (ret != 1) {
				std::cerr << "Failed to get status, error " << ret << " == " << libusb_strerror((libusb_error)ret) << std::endl;
			}
			std::cout << "status: " << (int)data[0]; std::cout.flush();
		}

		{ //read bulk data:
			uint8_t data[512];
			int transferred = 0;
			int ret = libusb_bulk_transfer(dev,
				0x82, //endpoint
				data, //data
				512, //length
				&transferred, //transferred
				1000 * 5//timeout
			);
			if (ret != 0) {
				std::cerr << "Failed to get bulk info after status, error " << ret << " == " << libusb_strerror((libusb_error)ret) << std::endl;
			}
			std::cout << " got:";
			for (int i = 0; i < transferred; ++i) {
				std::cout << ' ' << to_hex(data[i]);
			}
			std::cout << std::endl;

			return std::vector< uint8_t >(data, data + transferred);
		}
	};
	//seems to return "COMPUCON" string:
	//poll_index(0x8e0d, 0);

	std::cout << "handshake(?): ";
	{
		uint8_t data[1];
		int ret = libusb_control_transfer(dev,
			0xc0, //bmRequestType
			0, //bRequest
			0x000d, //wValue == transfer length?
			0x8f01, //wIndex
			data, //data
			1, //wLength
			1000 * 5 //timeout
		);
		if (ret != 1) {
			std::cerr << "Failed to get status, error " << ret << " == " << libusb_strerror((libusb_error)ret) << std::endl;
		}
		std::cout << "status: " << (int)data[0]; std::cout.flush();
	}
	{ //write some sort of handshake?
		uint8_t data[13] = {0xb9, 0x43, 0x4f, 0x4d, 0x50, 0x55, 0x43, 0x4f, 0x4e, 0x01, 0xba, 0xd8, 0x03};
		int transferred = 0;
		int ret = libusb_bulk_transfer(dev,
			0x01, //endpoint
			data, //data
			13, //length
			&transferred, //transferred
			1000 * 5//timeout
		);
		if (ret != 0) {
			std::cerr << "WARNING: Failed to transfer handshake, error " << ret << " == " << libusb_strerror((libusb_error)ret) << std::endl;
		} else if (transferred != 13) {
			std::cerr << "WARNING: Failed to transfer whole handshake, just sent " << transferred << "." << std::endl;
		}
	}
	{ //read bulk data:
		uint8_t data[512];
		int transferred = 0;
		int ret = libusb_bulk_transfer(dev,
			0x82, //endpoint
			data, //data
			512, //length
			&transferred, //transferred
			1000 * 5//timeout
		);
		if (ret != 0) {
			std::cerr << "Failed to get bulk info after status, error " << ret << " == " << libusb_strerror((libusb_error)ret) << std::endl;
		}
		std::cout << " got:";
		for (int i = 0; i < transferred; ++i) {
			std::cout << ' ' << to_hex(data[i]);
		}
		std::cout << std::endl;
	}

	//seems to return "ver" string:
	//poll_index(0xf00a, 0);


	//this seems to happen before sending data:
	poll_index(0x8601, 1);

	//------------------------------------

	//this seems to send data:
	{
		uint8_t data[1];
		int ret = libusb_control_transfer(dev,
			0xc0, //bmRequestType
			1, //bRequest
			0x0080, //wValue <-- maybe len(path)?
			0x0001, //wIndex
			data, //data
			1, //wLength
			1000 * 5 //timeout
		);
		if (ret != 1) {
			std::cerr << "Failed to get status, error " << ret << " == " << libusb_strerror((libusb_error)ret) << std::endl;
		}
		std::cout << " path status: " << (int)data[0];
	}

	{ //write some sort of path?
		uint8_t data[129] =
		"\xb9\x9c\x40\x00\x00"
		"\x38\xff\x30\xff\xbd\xc6\x00\x00\x00\x41\xf7\x00\x00\x00\x00\xbf"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\xba\x2f\x08";

		int transferred = 0;
		int ret = libusb_bulk_transfer(dev,
			0x01, //endpoint
			data, //data
			128, //length
			&transferred, //transferred
			1000 * 5//timeout
		);
		if (ret != 0) {
			std::cerr << "WARNING: Failed to transfer path, error " << ret << " == " << libusb_strerror((libusb_error)ret) << std::endl;
		} else if (transferred != 128) {
			std::cerr << "WARNING: Failed to transfer whole path, just sent " << transferred << "." << std::endl;
		}
	}

	{ //read bulk data:
		uint8_t data[512];
		int transferred = 0;
		int ret = libusb_bulk_transfer(dev,
			0x82, //endpoint
			data, //data
			512, //length
			&transferred, //transferred
			1000 * 5//timeout
		);
		if (ret != 0) {
			std::cerr << "Failed to get bulk info after path, error " << ret << " == " << libusb_strerror((libusb_error)ret) << std::endl;
		}
		std::cout << " path got:";
		for (int i = 0; i < transferred; ++i) {
			std::cout << ' ' << to_hex(data[i]);
		}
		std::cout << std::endl;
	}

	//this seems to be the standby loop:
	while (1) {
		poll_index(0x8001, 1); //<-- returns 0f when not sewing 4f (6f?) when sewing, 2f when done(?)
		poll_index(0x8001, 1);
		poll_index(0x8101, 1);
		poll_index(0x8201, 1);

		sleep(1);

	}


	{ //exit:
		libusb_exit(ctx);
		ctx = nullptr;
	}
}
