#pragma once 

#include <cstddef>

class IZKProof
{
public:
    size_t statement_BYTES;
    size_t witness_BYTES;
    size_t proof_BYTES;
    virtual int prove(
        const unsigned char (*witness),
        const unsigned char (*statement),
        unsigned char (*proof)
    ) = 0;
    virtual bool verify(
        const unsigned char (*statement),
        const unsigned char (*proof)
    ) = 0;
};
