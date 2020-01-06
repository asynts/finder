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

        bool operator<(size_t rhs) const noexcept {
            return digest < rhs;
        }
    };

private:
    // Sorted lists can be searched faster, thus this list shall always be
    // sorted.
    std::vector<entry> entries;
    std::hash<std::string_view> hasher;

public:
    void add(std::string_view filename) {
        std::size_t digest = hasher(filename);

        const auto iter = std::lower_bound(entries.begin(), entries.end(), digest);
        entries.emplace(iter, entry{ std::string{filename}, digest });
    }

    void marshall(std::ostream &output) {
        encoder enc{output};

        // This expression evaluates to the offset of the lookup table.
        std::size_t offset = 2 * sizeof(std::size_t) * (1 + entries.size());

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

// This database is lazy because only the hashes are decoded, and strings are
// extracted on demand using their offset.
struct lazy_database {
    struct entry {
        std::size_t digest;
        std::size_t offset;

        bool operator<(std::size_t rhs) const noexcept {
            return digest < rhs;
        }
    };

private:
    std::vector<entry> entries;
    mutable decoder dec;
    std::hash<std::string_view> hasher;

public:
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
        std::vector<std::string> matches;

        const size_t digest = hasher(filename);

        auto iter = std::lower_bound(entries.begin(), entries.end(), digest);

        while(iter != entries.end() && iter->digest == digest) {
            matches.push_back(lookup(iter->offset));

            ++iter;
        }

        return matches;
    }
};

int main() {
}
