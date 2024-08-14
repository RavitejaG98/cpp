#include <iostream>
#include <string>

int main() {
    std::string input;

    std::cout << "Enter a string: ";
    std::getline(std::cin, input);  // Reads a line of text from standard input

    std::cout << "You entered: " << input << std::endl;

    return 0;
}
