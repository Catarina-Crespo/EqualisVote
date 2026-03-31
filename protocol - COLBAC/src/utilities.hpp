#pragma once

#include <cstdio>
#include <cstddef>
#include <string>
#include <vector>
#include <cstdint>

void printhex(unsigned char *buf, size_t buflen);
std::string hex_to_string(unsigned char *buf, size_t buflen);

unsigned char* hex_to_bytes(const std::string& hex);
std::string to_hex(const uint8_t* data, size_t length);
std::vector<uint8_t> from_hex(const std::string& hex);

std::vector<std::string> extract_tokens(const std::string& str, char delim);