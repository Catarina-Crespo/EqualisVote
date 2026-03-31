//
// Created by Catarina on 02/06/2025.
//
#include <emscripten/emscripten.h>
#include <iostream>

extern "C" {
    EMSCRIPTEN_KEEPALIVE
    void say_hi() {
        std::cout << "Hello from Emscripten!" << std::endl;
    }
}
