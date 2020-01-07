#include <algorithm>
#include <array>
#include <cstring>
#include <exception>
#include <vector>
#include <filesystem>
#include <iostream>
#include <fstream>

namespace fs = std::filesystem;

constexpr std::size_t finder_abi_version = 0;
const fs::path finder_cache_path = ".findercache";

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
private:
    struct entry {
        std::string path;
        std::size_t digest;

        bool operator<(size_t rhs) const noexcept {
            return digest < rhs;
        }
    };

    // Sorted lists can be searched faster, thus this list shall always be
    // sorted.
    std::vector<entry> entries;

public:
    void add(fs::path path) {
        std::size_t digest = fs::hash_value(path.filename());

        const auto iter = std::lower_bound(entries.begin(),
                                           entries.end(),
                                           digest);

        entries.emplace(iter, entry{ path.lexically_normal().native(), digest });
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

            offset += sizeof(size_t) + entry.path.size();
        }

        // Write the lookup table.
        for(const auto &entry : entries) {
            enc.write(entry.path.size());
            enc.write(entry.path.data(), entry.path.size());
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

// This database is lazy because only the hashes are decoded, and the paths are
// extracted on demand.
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

    fs::path lookup(std::size_t offset) const {
        dec.seek(offset);

        std::size_t length;
        dec.decode(length);

        std::string path;
        path.resize(length);

        dec.decode(path.data(), length);

        return path;
    }

    std::vector<fs::path> locate(fs::path filename) const {
        if(filename.has_parent_path()) {
            throw std::runtime_error{"can't search for paths"};
        }

        std::vector<fs::path> matches;

        const size_t digest = hash_value(filename);

        auto iter = std::lower_bound(entries.begin(), entries.end(), digest);

        while(iter != entries.end() && iter->digest == digest) {
            matches.push_back(lookup(iter->offset));

            ++iter;
        }

        return matches;
    }
};

void rebuild_database(fs::path directory) {
    database db;

    for(const fs::path &path : fs::recursive_directory_iterator(directory)) {
        db.add(path);
    }

    std::fstream output{directory / finder_cache_path, std::ios::out};
    db.marshall(output);
}

void locate_exact_filename(fs::path filename, fs::path directory) {
    std::fstream input{directory / finder_cache_path, std::ios::in};
    lazy_database db{input};

    for(const auto &path : db.locate(filename)) {
        std::cout << path.native() << '\n';
    }
}

int main() {
}
