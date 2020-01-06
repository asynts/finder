#include <algorithm>
#include <array>
#include <cstring>
#include <exception>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

constexpr std::size_t finder_abi_version = 0;

struct encoder {
private:
    std::ostream &output;

public:
    encoder(std::ostream &output)
        : output(output) { }

    void write(std::size_t value) {
        std::array<char, sizeof(value)> value_bytes;
        std::memcpy(&value_bytes, &value, sizeof(value));

        output.write(value_bytes.data(), value_bytes.size());
    }

    void write(const char *data, std::size_t size) {
        output.write(data, size);
    }
};

struct database {
    struct entry {
        std::string filename;
        std::size_t digest;

        bool operator<(const entry &rhs) const noexcept {
            return digest < rhs.digest;
        }
    };

    template <typename Container>
    static void marshall(Container &&entries, std::ostream &output) {
        encoder enc{output};

        // Sorted lists can be searched faster.
        std::sort(entries.begin(), entries.end());

        // This expression evaluates to the offset of the lookup table.
        size_t offset = 2 * sizeof(size_t) + 2 * sizeof(size_t) * entries.size();

        enc.write(finder_abi_version);
        enc.write(entries.size());

        // Write the index table.
        for(const auto &entry : entries) {
            enc.write(entry.digest);
            enc.write(offset);

            offset += sizeof(size_t) + entry.filename.size();
        }

        // Write the lookup table.
        for(const auto &entry : entries) {
            enc.write(entry.filename.size());
            enc.write(entry.filename.data(), entry.filename.size());
        }
    }
};

struct decoder {
private:
    std::istream &input;

public:
    decoder(std::istream &input)
        : input{input} { }

    void decode(size_t &value) {
        std::array<char, sizeof(std::size_t)> value_bytes;

        input.read(value_bytes.data(), sizeof(std::size_t));
        std::memcpy(&value, &value_bytes, sizeof(std::size_t));
    }

    void decode(char *data, std::size_t size) {
        input.read(data, size);
    }

    void seek(std::size_t offset) {
        input.seekg(offset);
    }
};

struct lazy_database {
    struct entry {
        std::size_t digest;
        std::size_t offset;
    };

    std::vector<entry> entries;
    mutable decoder dec;

    lazy_database(std::istream &input)
        : dec{input}
    {
        std::size_t abi_version;
        dec.decode(abi_version);

        if(abi_version != finder_abi_version) {
            throw std::runtime_error{"invalid or outdated file format"};
        }

        std::size_t count;
        dec.decode(count);
        entries.resize(count);

        for(std::size_t idx=0; idx < count; ++idx) {
            dec.decode(entries[idx].digest);
            dec.decode(entries[idx].offset);
        }
    }

    std::string lookup(std::size_t offset) const {
        dec.seek(offset);

        std::size_t length;
        dec.decode(length);

        std::string filename;
        filename.resize(length);

        dec.decode(filename.data(), length);

        return filename;
    }

    std::vector<std::string> locate(std::string_view filename) const {
        std::terminate(); // TODO
    }
};


int main() {
}
