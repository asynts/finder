#include <algorithm>
#include <array>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string_view>

namespace fs = std::filesystem;

#include <fmt/format.h>

constexpr std::size_t finder_abi_version = 2;
const fs::path finder_cache_path = ".findercache";

constexpr std::string_view finder_usage_page = 1 + R"(
usage: finder [OPTIONS] FILENAME [DIRECTORY]
       finder [OPTIONS] --rebuild [DIRECTORY]

FILENAME                filename to locate; must not have a parent directory
DIRECTORY               defaults to '.'

OPTIONS
        --help          display usage information
        --version       display version information

    -r, --rebuild       rebuild cache
)";

constexpr std::string_view finder_version_page = 1 + R"(
finder 1.0.0
)";

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

    // Sorted lists can be searched faster, thus this list shall always be sorted.
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

// This database is lazy because only the hashes are decoded, and the paths are extracted on demand.
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

void rebuild_cache(fs::path directory) {
    database db;

    for(const fs::path &path : fs::recursive_directory_iterator(directory)) {
        db.add(path);
    }

    std::fstream output{directory / finder_cache_path, std::ios::out};
    db.marshall(output);
}

void locate_exact_filename(fs::path filename, fs::path directory) {
    if(filename.has_parent_path()) {
        std::cerr << "error: this tool only indexes filenames, therefor paths can't be located\n";
        std::exit(EXIT_FAILURE);
    }

    const auto cache_filepath = (directory / finder_cache_path).lexically_normal();

    if(not fs::exists(cache_filepath)) {
        std::cerr << fmt::format("error: no cache has been build for this directory, the file '{}' doesn't exist\n",
                                 cache_filepath.native());
        std::exit(EXIT_FAILURE);
    }

    std::fstream input{cache_filepath, std::ios::in};
    lazy_database db{input};

    for(const auto &path : db.locate(filename)) {
        std::cout << fmt::format("{}\n",
                                 path.native());
    }
}

int main(int argc, const char **argv) {
    bool do_rebuild = false;

    std::vector<std::string_view> positional_arguments;

    // Start at index one such that the executable name is skipped.
    for(size_t idx = 1; idx < static_cast<size_t>(argc); ++idx) {
        const auto argument = std::string_view{argv[idx]};

        if(argument == "--help") {
            std::cout << finder_usage_page;
            std::exit(EXIT_SUCCESS);
        }

        if(argument == "--version") {
            std::cout << finder_version_page;
            std::exit(EXIT_SUCCESS);
        }

        if(argument == "--rebuild" or argument == "-r") {
            if(do_rebuild) {
                std::cerr << fmt::format("error: invalid command line option '{}'\n\n{}",
                                         argument,
                                         finder_usage_page);
                std::exit(EXIT_FAILURE);
            }

            do_rebuild = true;
            continue;
        }

        if(argument.starts_with("-")) {
            std::cerr << fmt::format("error: invalid command line option '{}'\n\n{}",
                                     argument,
                                     finder_usage_page);
            std::exit(EXIT_FAILURE);
        }

        positional_arguments.push_back(argument);
    }

    if(do_rebuild) {
        if(positional_arguments.size() == 0) {
            rebuild_cache(".");
            std::exit(EXIT_SUCCESS);
        } else if(positional_arguments.size() == 1) {
            rebuild_cache(positional_arguments[0]);
            std::exit(EXIT_SUCCESS);
        } else {
            std::cerr << fmt::format("error: expected 1 positional argument, got {}\n\n{}",
                                     positional_arguments.size(),
                                     finder_usage_page);
            std::exit(EXIT_FAILURE);
        }
    } else {
        if(positional_arguments.size() == 1) {
            locate_exact_filename(positional_arguments[0], ".");
            std::exit(EXIT_SUCCESS);
        } else if(positional_arguments.size() == 2) {
            locate_exact_filename(positional_arguments[0],
                                  positional_arguments[1]);
            std::exit(EXIT_SUCCESS);
        } else {
            std::cerr << fmt::format("error: expected 1 or 2 positional arguments, got {}\n\n{}",
                                     positional_arguments.size(),
                                     finder_usage_page);
            std::exit(EXIT_FAILURE);
        }
    }
}
