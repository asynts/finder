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

struct encoder {
private:
    std::basic_ostream<char> &output;

public:
    encoder(std::basic_ostream<char> &output)
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

struct decoder {
private:
    std::basic_istream<char> &input;

public:
    decoder(std::basic_istream<char> &input)
        : input(input) { }

    void read(size_t &value) {
        std::array<char, sizeof(std::size_t)> value_bytes;

        input.read(value_bytes.data(), sizeof(std::size_t));
        std::memcpy(&value, &value_bytes, sizeof(std::size_t));
    }

    void read(char *data, std::size_t size) {
        input.read(data, size);
    }

    void seek(std::size_t offset) {
        input.seekg(offset);
    }
};

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

struct database {
private:
    std::vector<entry> entries;
    SHA3_256 hasher;

public:
    static database load(std::basic_istream<char> &input) {
        database db;
        decoder dec{input};

        std::size_t count;
        dec.read(count);

        db.entries.resize(count);

        std::size_t offset = sizeof(std::size_t);
        for(std::size_t idx = 0; idx < count; ++idx) {
            entry &entry = db.entries[idx];

            dec.seek(offset);
            dec.read(entry.digest.data(), 32);

            std::size_t reference;
            dec.read(reference);

            dec.seek(reference);

            std::size_t length;
            dec.read(length);

            entry.filename.resize(length);
            dec.read(entry.filename.data(), length);

            offset += 32 + sizeof(size_t);
        }

        return db;
    }

    void save(std::basic_ostream<char> &output) {
        encoder enc{output};

        // Sorted lists can be searched faster.
        std::sort(entries.begin(), entries.end());

        // This expression evaluates to the offset of the lookup table.
        size_t offset = sizeof(size_t) + (32 + sizeof(size_t)) * entries.size();

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

    void add(std::string_view filename) {
        std::array<char, 32> digest;
        hasher.compute(filename, digest);

        entries.emplace_back(filename, digest);
    }

    std::vector<entry>& raw() {
        return entries;
    }

    const std::vector<entry>& raw() const {
        return entries;
    }
};

int main() {
}
