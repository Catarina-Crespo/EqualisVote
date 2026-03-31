#include "utilities.hpp"
#include <sstream>
#include <iomanip>
#include <vector>
#include <cstdint>

void printhex(unsigned char *buf, size_t buflen)
{
    for (int i = 0; i < buflen; i++)
    {
        printf("%02x", buf[i]);
    }
    printf("\n");
}

std::string hex_to_string(unsigned char *buf, size_t buflen)
{
    std::stringstream ss;
    for(auto i = 0; i < buflen; ++i)
    {
        ss << std::hex << std::setfill('0') << std::setw(2) << (int) buf[i];
    }
    return ss.str();
}

unsigned char* hex_to_bytes(const std::string& hex) {
    size_t len = hex.length();
    unsigned char* bytes = (unsigned char*)malloc(len / 2);
    for (size_t i = 0; i < len; i += 2) {
        std::string byteString = hex.substr(i, 2);
        bytes[i / 2] = (unsigned char) strtol(byteString.c_str(), nullptr, 16);
    }
    return bytes;
}

std::string to_hex(const uint8_t* data, size_t length) {
    std::ostringstream oss;
    for (size_t i = 0; i < length; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    }
    return oss.str();
}

std::vector<uint8_t> from_hex(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(strtol(byteString.c_str(), nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

std::vector<std::string> extract_tokens(const std::string& str, char delim) {
    std::stringstream ss(str);
    std::string item;
    std::vector<std::string> tokens;
    while (std::getline(ss, item, delim)) {
        tokens.push_back(item);
    }
    return tokens;
}
