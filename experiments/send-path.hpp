#pragma once

inline send_path(libusb_context *ctx, std::vector< int32_t > const &positions) {
	
	auto add_checksum = [ctx](std::vector< uint8_t > &data) {
		assert(data.size() == 126 && "must pass an un-checksummed packet");
		uint16_t sum = 0;
		for (uint8_t d : data) {
			sum += uint16_t(d);
		}
		data.emplace_back(sum >> 8);
		data.emplace_back(sum & 0xff);
		assert(data.size() == 128);
	};
}
