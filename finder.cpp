#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

#include <fmt/format.h>

namespace fs = std::filesystem;

constexpr std::size_t       finder_abi_version = 3;
const     fs::path          finder_cache_path  = ".findercache";

constexpr std::string_view  finder_help_page = 1 + R"(
usage: finder [OPTIONS] [FILENAME]

OPTIONS
        --help          display usage information
        --version       display version information

    -r, --rebuild       rebuild cache
    -l, --list          list all filepaths from the cache
)";

constexpr std::string_view  finder_version_page = 1 + R"(
finder 0.0.1

Copyright (C) 2020 Paul Scharnofske; Licensed under the terms of MIT.
)";

struct encoder {
private:
    std::ostream &output;

public:
    encoder(std::ostream &output)
        : output(output) { }

    void write(const std::size_t value) {
        std::array<char, sizeof(value)> value_bytes;
        std::memcpy(&value_bytes, &value, sizeof(value));

        output.write(value_bytes.data(), value_bytes.size());
    }

    void write(const char *data, const std::size_t size) {
        output.write(data, size);
    }
};

// Represents a database of filepaths. It is encoded as hashtable where the hashes of the filenames,
// not the filepaths, are stored; and a lookup table for the corresponding filepaths. Thus this database
// can be searched very quickly, but only for exact matches.
struct database {
private:
    struct entry {
        std::string path;
        std::size_t digest;

        [[nodiscard]]
        constexpr bool operator<(const entry &rhs) const noexcept {
            return digest < rhs.digest;
        }
    };

    std::vector<entry> entries;

public:
    void add(const fs::path path) {
        const std::size_t digest = fs::hash_value(path.filename());
        entries.push_back(entry{ path.native(), digest });
    }

    void marshall(std::ostream &output) {
        encoder enc{output};

        // Sorted lists can be searched faster.
        std::sort(entries.begin(), entries.end());

        // This expression evaluates to the offset of the lookup table.
        std::size_t offset = (1 + entries.size()) * 2 * sizeof(std::size_t);

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

    void decode(char *data, const std::size_t size) {
        input.read(data, size);
    }

    void seek(const std::size_t offset) {
        input.seekg(offset);
    }
};

// This database is lazy because only the hashes are decoded, and the filepaths are extracted on demand.
struct lazy_database {
    struct entry {
        std::size_t digest;
        std::size_t offset;

        [[nodiscard]]
        constexpr bool operator<(const std::size_t rhs) const noexcept {
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
            std::cerr << "error: cache was build by a different version of finder\n";
            std::exit(EXIT_FAILURE);
        }

        std::size_t count;
        dec.decode(count);
        entries.resize(count);

        for(std::size_t idx=0; idx < count; ++idx) {
            dec.decode(entries[idx].digest);
            dec.decode(entries[idx].offset);
        }
    }

    fs::path lookup(const std::size_t offset) const {
        dec.seek(offset);

        std::size_t length;
        dec.decode(length);

        std::string path;
        path.resize(length);
        dec.decode(path.data(), length);

        return path;
    }

    std::vector<fs::path> locate(const fs::path filename) const {
        std::vector<fs::path> matches;

        const size_t digest = hash_value(filename);

        auto iter = std::lower_bound(entries.begin(), entries.end(), digest);

        while(iter != entries.end() && iter->digest == digest) {
            matches.push_back( lookup(iter->offset) );
            ++iter;
        }

        return matches;
    }
};

void rebuild_cache() {
    database db;
    for( const fs::path &path : fs::recursive_directory_iterator(".") ) {
        if(not fs::is_directory(path)) {
            db.add(path);
        }
    }

    std::fstream output_stream{finder_cache_path, std::ios::out};
    db.marshall(output_stream);
}

std::fstream open_database_stream() {
    if(not fs::exists(finder_cache_path)) {
        std::cerr << "error: no cache has been build for this directory\n";
        std::exit(EXIT_FAILURE);
    }

    std::fstream stream{finder_cache_path, std::ios::in};

    // `std::fstream` instances can't be copied or moved, this works because of the copy ellipson
    // optimization with is guaranteed by the standard.
    return stream;
}

void locate_exact_filename(const fs::path filename) {
    if(filename.has_parent_path()) {
        std::cerr << "error: can't search for paths\n";
        std::exit(EXIT_FAILURE);
    }

    auto input_stream = open_database_stream();

    const lazy_database db{input_stream};
    for(const auto &path : db.locate(filename)) {
        std::cout << fmt::format("{}\n",
                                 path.native());
    }
}

void list_all_filepaths() {
    auto input_stream = open_database_stream();
    decoder dec{input_stream};

    std::size_t abi_version;
    dec.decode(abi_version);

    if(abi_version != finder_abi_version) {
        std::cerr << "error: cache was build by a different version of finder\n";
        std::exit(EXIT_FAILURE);
    }

    std::size_t count;
    dec.decode(count);

    // Seek to the start of the lookup table.
    dec.seek( 2 * sizeof(std::size_t) * (1 + count) );

    for(std::size_t idx = 0; idx < count; ++idx) {
        std::size_t length;
        dec.decode(length);

        std::string filepath;
        filepath.resize(length);

        dec.decode(filepath.data(), length);

        std::cout << filepath << '\n';
    }
}

int main(int argc, const char **argv) {
    bool do_rebuild = false,
         do_list = false,
         do_not_interpret_arguments = false;

    std::vector<std::string_view> positional_arguments;

    // Start at index one such that the executable name is skipped.
    for(size_t idx = 1; idx < static_cast<size_t>(argc); ++idx) {
        const auto argument = std::string_view{argv[idx]};

        if(do_not_interpret_arguments) {
            positional_arguments.push_back(argument);
            continue;
        }
        if(argument == "--help") {
            std::cout << finder_help_page;
            std::exit(EXIT_SUCCESS);
        }
        if(argument == "--version") {
            std::cout << finder_version_page;
            std::exit(EXIT_SUCCESS);
        }
        if(argument == "--rebuild") {
            do_rebuild = true;
            continue;
        }
        if(argument == "--list") {
            do_list = true;
            continue;
        }
        if(argument == "--") {
            do_not_interpret_arguments = true;
            continue;
        }

        if(argument.starts_with("-")) {
            for(const char flag : argument.substr(1)) {
                if(flag == 'r') {
                    do_rebuild = true;
                    continue;
                }

                if(flag == 'l') {
                    do_list = true;
                    continue;
                }

                std::cout << fmt::format("error: unrecognized command line option '{}'\n\n{}",
                                         argument,
                                         finder_help_page);
                std::exit(EXIT_FAILURE);
            }
        } else {
            positional_arguments.push_back(argument);
        }
    }

    if(do_rebuild) {
        rebuild_cache();
    }

    if(positional_arguments.size() == 0 and do_list) {
        list_all_filepaths();
    } else if(positional_arguments.size() == 1 and not do_list) {
        locate_exact_filename(positional_arguments[0]);
    } else {
        std::cerr << fmt::format("error: invalid arguments\n\n{}",
                                 finder_help_page);
        std::exit(EXIT_FAILURE);
    }
}
