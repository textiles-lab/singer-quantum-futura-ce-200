#include <iostream>
#include <vector>
#include <functional>
#include <deque>

void test(std::string const &data_in) {
	std::vector< uint8_t > data;
	{ //make data string into bytes:
		bool high = true;
		for (char c : data_in) {
			if (isspace(c)) continue;
			uint8_t val = 0;
			if (c >= '0' && c <= '9') val = c - '0';
			else if (c >= 'a' && c <= 'f') val = c - 'a' + 10;
			else if (c >= 'A' && c <= 'F') val = c - 'A' + 10;
			else {
				assert(0 && " not hex(?)");
			}
			if (high) {
				data.emplace_back(val * 0x10);
				high = false;
			} else {
				data.back() |= val;
				high = true;
			}
		}
		assert(high == true && "must have even bytes");
	}

	auto hexify8 = [](uint8_t byte) {
		static const char *hex = "0123456789abcdef";
		return std::string() + hex[byte/16] + hex[byte%16];
	};

	std::cout << "Read " << data.size() << " bytes:";
	for (uint32_t i = 0; i < data.size(); ++i) {
		//static const char *hex = "0123456789abcdef";
		std::cout << ' ' << hexify8(data[i]); //hex[data[i]/16] << hex[data[i]%16];
	}
	std::cout << std::endl;

	//expecting something like b9 9c 40 ?? ??

	std::deque< std::function< void(uint8_t) > > next;

	char mode = '\0';

	for (uint32_t i = 0; i < data.size(); ++i) {
		uint8_t cmd = data[i];
		std::cout << ' ' << hexify8(cmd) << ": ";
		for (uint32_t bit = 7; bit < 8; --bit) {
			std::cout << (cmd & (1 << bit) ? '1' : '0');
		}
		std::cout << ' ';
		if (!next.empty()) {
			next.front()(cmd);
			next.pop_front();
			std::cout << std::endl;
			continue;
		}
		if (cmd & 0x80) {
			mode = '\0';
			if (cmd == 0xb9) {
				std::cout << "start of packet";
				assert(i == 0);
			} else if (cmd == 0xbd) {
				std::cout << "start of sewing";
				mode = 's';
				//next is some sort of speed setting?
				next.emplace_back([](uint8_t cmd){
					std::cout << " (speed?)";
				});
			} else if (cmd == 0xba) {
				std::cout << "end of packet (+ end of data)";
				assert(i + 3 == data.size());
				next.emplace_back([](uint8_t cmd){ std::cout << " (checksum [msb])"; });
				next.emplace_back([](uint8_t cmd){ std::cout << " (checksum [lsb])"; });
			} else if (cmd == 0xbb) {
				std::cout << "end of packet (more to come)";
				assert(i + 3 == data.size());
				next.emplace_back([](uint8_t cmd){ std::cout << " (checksum [msb])"; });
				next.emplace_back([](uint8_t cmd){ std::cout << " (checksum [lsb])"; });
			} else {
				std::cout << "unknown control";
			}
		} else if (mode == 's'){
			std::cout << "step x " << (cmd & 0x40 ? '-' : '+') << (cmd & 0x3f);
			next.emplace_back([](uint8_t cmd){
				assert((cmd & 0x80) == 0);
				std::cout << "step y " << (cmd & 0x40 ? '-' : '+') << (cmd & 0x3f);
			});
		}
		std::cout << std::endl;

	}


}

int main(int argc, char **argv) {

	test("b9 9c 40 00 00 38 ff 30 ff bd c6 00 00 00 41 f7 00 00 00 00 bf"
"00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
"00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
"00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
"00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
"00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
"00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
"00 00 00 00 00 00 00 00 ba 2f 08");

	test("b9 9c 40 00 00 44 ff 33 ff bd c6 00 00 45 41 45 42 45 41 45 41"
"f4 00 00 00 00 be 00 00 00 00 bd e5 46 41 45 41"
"05 42 06 42 ec 00 00 00 00 be 00 00 00 00 bd e6"
"05 42 05 42 05 41 05 42 05 42 05 41 f4 00 00 00"
"00 be 00 00 00 00 bd e5 06 42 05 42 41 00 00 41"
"f7 00 00 00 00 bf 00 00 00 00 00 00 00 00 00 00"
"00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
"00 00 00 00 00 00 00 00 ba 60 18");

}
