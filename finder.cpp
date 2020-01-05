#include <array>
#include <bit>
#include <cstring>
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

    void compute(std::string_view input, std::array<unsigned char, 32> output) {
        EVP_DigestInit_ex(context, algorithm, NULL);
        EVP_DigestUpdate(context, input.data(), input.size());
        EVP_DigestFinal_ex(context, reinterpret_cast<unsigned char *>(output.data()), NULL);
    }
};

struct marshaller {
    std::basic_ostream<unsigned char> &output;

    void write_size(size_t value) {
        // TODO std::bit_cast
        std::array<unsigned char, sizeof(value)> value_bytes;
        std::memcpy(&value_bytes, &value, sizeof(value));

        output.write(value_bytes.data(), value_bytes.size());
    }

    template <typename Char>
    void write_bytes(const Char *data, size_t size) {
        output.write(data, size);
    }
};

struct entry {
    std::string filename;
    std::array<unsigned char, 32> digest;

    bool operator<(const entry &rhs) const noexcept {
        // This compare operation doesn't neccessary make sense, think of
        // little endian systems, it just has to be some ordering such that
        // a list of entries can be sorted.
        //
        // The compiler isn't allowed to do the optimization below, because
        // on little endian systems it would change the outcome therefor the
        // optimization is done in code.
        //
        // TODO Measure.

        for(size_t idx = 0; idx < digest.size(); idx += sizeof(size_t)) {
            // TODO std::bit_cast
            size_t lhs_value;
            std::memcpy(&lhs_value, &digest[idx], sizeof(size_t));

            // TODO std::bit_cast
            size_t rhs_value;
            std::memcpy(&rhs_value, &rhs.digest[idx], sizeof(size_t));

            if(lhs_value < rhs_value) {
                return true;
            }
        }

        return false;
    }
};

void marshall_database(std::vector<entry> &entries, marshaller output) {
    // Ensure that the entries are sorted to begin with, such that they can
    // be searched very quickly.
    std::sort(entries.begin(), entries.end());

    // This expression evaluates to the offset of the lookup table.
    size_t offset = sizeof(size_t) + (32 + sizeof(size_t)) * entries.size();

    output.write_size(entries.size());

    // Write the index.
    for(const auto &entry : entries) {
        output.write_bytes(entry.digest.data(), entry.digest.size());
        output.write_size(offset);

        offset += sizeof(size_t) + entry.filename.size();
    }

    // Write the lookup table.
    for(const auto &entry : entries) {
        output.write_size(entry.filename.size());
        output.write_bytes(reinterpret_cast<const unsigned char*>(entry.filename.data()), entry.filename.size());
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
