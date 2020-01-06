#include <algorithm>
#include <array>
#include <cstring>
#include <exception>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include "sha.cpp"

constexpr size_t finder_version = 0;

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
        std::array<char, 32> digest;

        entry() {
        }

        entry(std::string_view filename, const std::array<char, 32> &digest)
            : filename(filename), digest(digest) {
        }

        bool operator<(const entry &rhs) const noexcept {
            for(std::size_t idx = 0; idx < 32; ++idx) {
                if(digest[idx] < rhs.digest[idx]) {
                    return true;
                }
            }

            return false;
        }
    };

    template <typename Container>
    static void marshall(Container &&entries, std::ostream &output) {
        encoder enc{output};

        // Sorted lists can be searched faster.
        std::sort(entries.begin(), entries.end());

        // This expression evaluates to the offset of the lookup table.
        size_t offset = 2 * sizeof(size_t) + (32 + sizeof(size_t)) * entries.size();

        enc.write(finder_version);
        enc.write(entries.size());

        // Write the index table.
        for(const auto &entry : entries) {
            enc.write(entry.digest.data(), entry.digest.size());
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
    std::istream input;

public:
    decoder(std::istream &&input)
        : input{std::move(input)} { }

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
        std::array<char, 32> digest;
        size_t offset;
    };

    std::vector<entry> entries;
    mutable decoder dec;

    lazy_database(std::istream &&input)
        : dec{std::move(input)}
    {
        size_t version;
        dec.decode(version);

        if(version != finder_version) {
            throw std::runtime_error{"invalid or outdated file format"};
        }

        size_t count;
        dec.decode(count);
        entries.resize(count);

        for(size_t idx=0; idx < count; ++idx) {
            dec.decode(entries[idx].digest.data(), 32);
            dec.decode(entries[idx].offset);
        }
    }

    std::string lookup(size_t offset) const {
        dec.seek(offset);

        size_t length;
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
