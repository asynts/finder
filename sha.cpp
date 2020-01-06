#pragma once

#include <array>
#include <string_view>

#include <openssl/evp.h>

struct SHA3_256 {
private:
    const EVP_MD *algorithm;
    EVP_MD_CTX *context;

public:
    SHA3_256() {
        algorithm = EVP_sha3_256();
        context = EVP_MD_CTX_new();
    }

    ~SHA3_256() {
        EVP_MD_CTX_free(context);
    }

    void compute(std::string_view input, std::array<char, 32> &output) {
        EVP_DigestInit_ex(context, algorithm, NULL);
        EVP_DigestUpdate(context, input.data(), input.size());
        EVP_DigestFinal_ex(context, reinterpret_cast<unsigned char *>(output.data()), NULL);
    }
};
