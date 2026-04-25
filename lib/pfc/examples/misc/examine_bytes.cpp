#include <pfc/pfc.hpp>
#include <iostream>
#include <iomanip>
#include <fstream>

using namespace pfc;

int main() {
    using EGInt = Packed<uint32_t, codecs::EliasGamma>;
    using Point = PackedPair<EGInt, EGInt>;
    
    // Create some test data
    Point point = {{42}, {1337}};
    
    // Encode to buffer
    uint8_t buffer[64] = {};
    BitWriter writer(buffer);
    Point::encode(point, writer);
    writer.align();
    
    size_t encoded_size = writer.ptr() - buffer;
    
    std::cout << "Encoded Point{{42}, {1337}} to " << encoded_size << " bytes:\n";
    std::cout << "Hex: ";
    for (size_t i = 0; i < encoded_size; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') 
                  << static_cast<unsigned>(buffer[i]) << " ";
    }
    std::cout << "\n";
    
    std::cout << "Binary: ";
    for (size_t i = 0; i < encoded_size; ++i) {
        for (int bit = 7; bit >= 0; --bit) {
            std::cout << ((buffer[i] >> bit) & 1);
        }
        std::cout << " ";
    }
    std::cout << "\n";
    
    // Write to file
    const char* filename = "example_point.bin";
    std::ofstream file(filename, std::ios::binary);
    file.write(reinterpret_cast<const char*>(buffer), encoded_size);
    file.close();
    
    std::cout << "Written to file: " << filename << "\n";
    
    // Read back and decode
    std::ifstream read_file(filename, std::ios::binary);
    std::vector<uint8_t> file_data(encoded_size);
    read_file.read(reinterpret_cast<char*>(file_data.data()), encoded_size);
    read_file.close();
    
    BitReader reader(file_data.data());
    Point decoded = Point::decode(reader);
    
    std::cout << "Decoded from file: Point{{" << std::dec << decoded.first.value()
              << "}, {" << std::dec << decoded.second.value() << "}}\n";
    
    // Show individual value encodings
    std::cout << "\nIndividual Elias Gamma encodings:\n";
    for (uint32_t val : {0, 1, 2, 7, 42, 1337}) {
        uint8_t val_buffer[16] = {};
        BitWriter val_writer(val_buffer);
        codecs::EliasGamma::encode(val, val_writer);
        val_writer.align();
        
        size_t val_size = val_writer.ptr() - val_buffer;
        std::cout << "Value " << val << " -> ";
        for (size_t i = 0; i < val_size; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                      << static_cast<unsigned>(val_buffer[i]) << " ";
        }
        std::cout << "(";
        for (size_t i = 0; i < val_size; ++i) {
            for (int bit = 7; bit >= 0; --bit) {
                std::cout << ((val_buffer[i] >> bit) & 1);
            }
        }
        std::cout << ")\n";
    }
    
    return 0;
}
