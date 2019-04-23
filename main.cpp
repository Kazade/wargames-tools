#include <fstream>
#include <set>
#include <map>
#include <string>
#include <vector>
#include <iostream>


std::set<std::string> kwargs = {
    "--in", "--out"
};

std::map<std::string, std::string> parsed_kwargs;


bool decompress_file(const std::string& infile, std::vector<uint8_t>& output) {
    const int HISTORY_SIZE = 4096;
    const int HISTORY_MASK = HISTORY_SIZE - 1;
    const int UPPER_MASK = 0xF0;
    const int LOWER_MASK = 0x0F;
    const int SHIFT_UPPER = 16;

    std::ifstream file(infile);

    if(!file) {
        std::cerr << "Unable to open input file" << std::endl;
        return false;
    }

    file.seekg(0, std::ios::end);
    auto length = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(length);
    file.read((char*) &buffer[0], length);

    output.clear();  // Prepare ourselves!

    auto history_offset = 0;
    std::vector<uint8_t> history(HISTORY_SIZE, 0);

    if(buffer[0] != 'F' || buffer[1] != 'L' || buffer[2] != 'A' || buffer[3] != '2') {
        std::cerr << "Input file not an FLA2 file" << std::endl;
        return false;
    }

    auto size = *((int32_t*)&buffer[4]);

    auto it = &buffer[0] + 8;

    auto decompress_byte_to_output = [&](const uint8_t value) {
        output.push_back(value);
        history[history_offset] = value;
        history_offset = (history_offset + 1) & HISTORY_MASK;
    };

    while(file.good()) {
        auto flag_byte = *it++;

        for(uint8_t i = 0; i < 8; ++i) {
            if(flag_byte & 0x80) {
                auto count = *it++;
                if(count == 0) {
                    return true; // Done
                }

                auto shift = (count & UPPER_MASK) * SHIFT_UPPER;
                auto offset = HISTORY_SIZE - (shift + *it++);
                count &= LOWER_MASK;
                count += 2;

                while(count--) {
                    decompress_byte_to_output(
                        history[(history_offset + offset) & HISTORY_MASK]
                    );
                }
            } else {
                decompress_byte_to_output(*it++);
            }

            flag_byte += flag_byte;
        }
    }

    return true;
}


int main(int argc, char** argv) {
    for(int i = 1; i < argc; ++i) {
        if(kwargs.count(argv[i]) && i < argc - 1) {
            parsed_kwargs[argv[i]] = argv[i + 1];
            ++i;
        }
    }

    if(!parsed_kwargs.count("--in")) {
        std::cout << "You must specify an input file" << std::endl;
        return 1;
    }

    if(!parsed_kwargs.count("--out")) {
        std::cout << "You must specify an output file" << std::endl;
        return 2;
    }

    auto input = parsed_kwargs["--in"];
    auto output = parsed_kwargs["--out"];

    std::vector<uint8_t> decompressed_data;

    if(decompress_file(input, decompressed_data)) {
        std::ofstream fileout(output, std::ios::binary);
        fileout.write((char*) &decompressed_data[0], decompressed_data.size());
    }

    return 0;
}
