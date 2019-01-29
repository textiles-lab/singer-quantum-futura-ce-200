#include <iostream>
#include <vector>

void test(std::string const &data_in) {
	std::vector< uint8_t > data;
	{
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


	uint16_t val_sum = 0;
	for (uint32_t i = 0; i + 2 < data.size(); i += 1) {
		val_sum += data[i];
	}

	std::cout << hexify8(val_sum >> 8) << hexify8(val_sum) << std::endl;


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

}
