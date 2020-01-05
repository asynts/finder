#include <array>
#include <iostream>
#include <string_view>
#include <vector>

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

    void compute(std::string_view input, std::array<std::byte, 32> output) {
        EVP_DigestInit_ex(context, algorithm, NULL);
        EVP_DigestUpdate(context, input.data(), input.size());
        EVP_DigestFinal_ex(context, reinterpret_cast<unsigned char *>(output.data()), NULL);
    }
};

struct marshaller {
    void write_size(size_t value);

    template <typename Iterator>
    void write_bytes(Iterator begin, Iterator end);
};

struct entry {
    std::string filename;
    std::array<std::byte, 32> digest;
};

void marshall_database(std::vector<entry> &entries, marshaller output) {
    // TODO sort

    // This expression evaluates to the offset of the lookup table.
    size_t offset = sizeof(size_t) + (32 + sizeof(size_t)) * entries.size();

    output.write_size(entries.size());

    // Write the index.
    for(const auto &entry : entries) {
        output.write_bytes(entry.digest.begin(), entry.digest.end());
        output.write_size(offset);

        offset += sizeof(size_t) + entry.filename.size();
    }

    // Write the lookup table.
    for(const auto &entry : entries) {
        output.write_size(entry.filename.size());
        output.write_bytes(entry.filename.data(), entry.filename.data() + entry.filename.size());
    }
}

/*

void print_usage(std::ostream &out) {
    out << 1 + R"(
usage: finder [OPTIONS] [PATTERN] [DIRECTORY]

        --version       print version information
        --help          print this page

    -r, --rebuild       rebuild the database
    -s, --stats         print stats about the database
    -p, --prefix        search for files starting with this prefix, instead of
                        exact matches.
)";
}

void print_version() {
    std::cout << "finder 0.0.1\n";
}

int main(int argc, const char **argv) {
    bool do_rebuild = false,
         do_stats = false,
         do_prefix = false;

    for(int idx = 0; idx < argc; ++idx) {
        std::string_view argument = argv[idx];

        if(argument == "--version") {
            print_version();
            exit(EXIT_SUCCESS);
        } else if(argument == "--help") {
            print_usage(std::cout);
            exit(EXIT_SUCCESS);
        } else if(argument == "--rebuild" or argument == "-r") {
            do_rebuild = true;
        } else if(argument == "--stats" or argument == "-s") {
            do_stats = true;
        } else if(argument == "--prefix" or argument == "-p") {
            do_prefix = true;
        } else {
            print_usage(std::cerr);
            exit(EXIT_FAILURE);
        }
    }
}

*/

int main() {
}
