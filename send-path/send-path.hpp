#pragma once

#include <vector>
#include <iostream>
#include <string>
#include <cassert>

#include <libusb.h>

//absolute limits accepted by the machine:
constexpr const int32_t MinX = 0xfeb8;
constexpr const int32_t MaxX = 0xffff;
constexpr const int32_t MinY = 0xfe1b;
constexpr const int32_t MaxY = 0xffff;

inline void add_checksum(std::vector< uint8_t > &data) {
	assert(data.size() == 126 && "must pass an un-checksummed packet");
	uint16_t sum = 0;
	for (uint8_t d : data) {
		sum += uint16_t(d);
	}
	data.emplace_back(sum % 256);
	data.emplace_back(sum / 256);
	assert(data.size() == 128);
};

inline std::string to_hex8(uint8_t value) {
	static char const *hex = "0123456789abcdef";
	std::string ret;
	ret += hex[(value / 16) % 16];
	ret += hex[(value) % 16];
	return ret;
}


std::string to_hex16(uint16_t value) {
	static char const *hex = "0123456789abcdef";
	std::string ret;
	ret += hex[(value / 16 / 16 / 16) % 16];
	ret += hex[(value / 16 / 16) % 16];
	ret += hex[(value / 16) % 16];
	ret += hex[(value) % 16];
	return ret;
}


std::vector< uint8_t > poll_index(libusb_device_handle *dev, uint16_t index, uint16_t request) {
	//some sort of version check?
	std::cout << "Index " << to_hex16(index) << ": ";
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
			std::cout << ' ' << to_hex8(data[i]);
		}
		std::cout << std::endl;

		return std::vector< uint8_t >(data, data + transferred);
	}
}

//send_path requires a *valid* path; that is, it should:
// - start (and remain) in the valid range
// - contain steps of at most 0x3f (== 63)
// - contain at least two points (we haven't figured out how to sew exactly one needle-down)

inline void send_path(libusb_device_handle *dev, std::vector< int32_t > const &xys) {

	//----- build path data from xys ----
	assert(xys.size() % 2 == 0);
	assert(xys.size() >= 4);

	int32_t start_x = xys[0];
	int32_t start_y = xys[1];
	assert(MinX <= xys[0] && xys[0] <= MaxX);
	assert(MinY <= xys[1] && xys[1] <= MaxY);

	std::vector< uint8_t > data;

	data.emplace_back(0x9c); //magic value
	data.emplace_back(0x40); //magic value
	data.emplace_back(0x00); //?? value
	data.emplace_back(0x00); //?? value
	data.emplace_back(start_x % 256); //start X (big endian)
	data.emplace_back(start_x / 256);
	data.emplace_back(start_y % 256); //start Y (big endian)
	data.emplace_back(start_y / 256);

	//0xbd 0xcX is a faster kind of path(?), but doesn't end cleanly(?)
	data.emplace_back(0xbd);
	data.emplace_back(0xc6);

	for (uint32_t i = 2; i + 1 < xys.size(); i += 2) {

		assert(MinX <= xys[i] && xys[i] <= MaxX);
		assert(MinY <= xys[i+1] && xys[i+1] <= MaxY);

		if (i + 4 == xys.size() && i != 0) {
			data.emplace_back(0xf7);
		}

		int32_t step_x = xys[i] - xys[i-2];
		int32_t step_y = xys[i+1] - xys[i+1-2];
		assert(-0x1c <= step_x && step_x <= 0x1c);
		assert(-0x1c <= step_y && step_y <= 0x1c);
		if (step_x < 0) data.emplace_back(0x40 | -step_x);
		else data.emplace_back(step_x);
		if (step_y < 0) data.emplace_back(0x40 | -step_y);
		else data.emplace_back(step_y);
	}
	data.emplace_back(0xbf); //end marker

	//----- divide path data into packets and send -----

	//packets are 128 bytes, start with b9 and end with bb/ba and a two-byte checksum.
	// thus, each packet can contain at most 128-4 = 124 bytes.

	while (data.size() % 124 != 0) data.emplace_back(0x00);

	//seems to happen before sending data, generally:
	poll_index(dev, 0x8601, 1);

	for (uint32_t base = 0; base < data.size(); base += 124) {
		bool is_last_packet = (base + 124 == data.size());

		assert(base + 124 <= data.size());
		std::vector< uint8_t > packet;
		packet.reserve(128);
		packet.emplace_back(0xb9); //all packets start with it
		packet.insert(packet.end(), data.begin() + base, data.begin() + base + 124);
		packet.emplace_back((is_last_packet ? 0xba : 0xbb));
		add_checksum(packet);

		//--- send the packet ---

		{ //set up xfer (?)
			uint8_t result[1];
			int ret = libusb_control_transfer(dev,
				0xc0, //bmRequestType
				1, //bRequest
				(is_last_packet ? 0x0080 : 0x0180), //wValue == len(path), with 0x100 as continue bit?
				0x0001, //wIndex
				result, //data
				1, //wLength
				1000 * 5 //timeout
			);
			if (ret != 1) {
				std::cerr << "ERROR: Failed to get status, error " << ret << " == " << libusb_strerror((libusb_error)ret) << std::endl;
				return;
			}
			if (result[0] != 0) {
				std::cerr << "WARNING: Expecting result of 0, got " << uint32_t(result[0]) << std::endl;
			}
			//std::cout << " path status: " << (int)result[0];
		}

		//DEBUG: dump the packet
		for (auto d : packet) {
			std::cout << ' ' << to_hex8(d);
		}
		std::cout << std::endl;

		//transfer the packet:
		assert(packet.size() == 128);
		int transferred = 0;
		int ret = libusb_bulk_transfer(dev,
			0x01, //endpoint
			packet.data(), //data
			packet.size(), //length
			&transferred, //transferred
			1000 * 5//timeout
		);
		if (ret != 0) {
			std::cerr << "WARNING: Failed to transfer packet, error " << ret << " == " << libusb_strerror((libusb_error)ret) << std::endl;
		} else if (transferred != packet.size()) {
			std::cerr << "WARNING: Failed to transfer whole packet, just sent " << transferred << "." << std::endl;
		}

		{ //read back (something?):
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
				std::cout << ' ' << to_hex8(data[i]);
			}
			std::cout << std::endl;
		}

	}


}

inline libusb_device_handle *open_machine(libusb_context *ctx) {
	libusb_device_handle *dev = nullptr;

	constexpr uint16_t vendor_id = 0x1320;
	constexpr uint16_t product_id = 0x0001;
	dev = libusb_open_device_with_vid_pid(ctx, vendor_id, product_id);
	if (dev == nullptr) {
		throw std::runtime_error("Failed to open device with Vendor ID " + to_hex16(vendor_id) + " and Product ID " + to_hex16(product_id));
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

	return dev;
}



void do_handshake(libusb_device_handle *dev) {
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
			std::cout << ' ' << to_hex8(data[i]);
		}
		std::cout << std::endl;
	}
}
