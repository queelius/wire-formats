// test_list.cpp - Test recursive list

#include <pfc/pfc.hpp>
#include <pfc/algebraic.hpp>
#include <iostream>

using namespace pfc;

int main() {
    std::cout << "Testing PackedList...\n";

    // Test empty list
    using IntList = PackedList<PackedU32<>>;

    std::cout << "Creating empty list..." << std::endl;
    IntList empty;
    std::cout << "Empty list created. Is empty? " << empty.empty() << std::endl;

    // Test list with one element
    std::cout << "Creating list with one element..." << std::endl;
    std::vector<uint32_t> one_elem = {42};
    IntList list1{one_elem};
    std::cout << "List created." << std::endl;

    // Get value back
    auto values = list1.value();
    std::cout << "Values size: " << values.size() << std::endl;
    if (!values.empty()) {
        std::cout << "First value: " << values[0] << std::endl;
    }

    std::cout << "Test complete!" << std::endl;

    return 0;
}