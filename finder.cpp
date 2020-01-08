#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

#include <fmt/format.h>

#include "database.cpp"

namespace fs = std::filesystem;

constexpr std::size_t finder_abi_version = 2;
const fs::path finder_cache_path = ".findercache";

constexpr std::string_view finder_usage_page = 1 + R"(
usage: finder [OPTIONS] FILENAME [DIRECTORY]
       finder [OPTIONS] --rebuild [DIRECTORY]
       finder [OPTIONS] --list [DIRECTORY]

OPTIONS
        --help          display usage information
        --version       display version information

    -r, --rebuild       rebuild cache
    -l, --list          list all filepaths from the cache
)";

constexpr std::string_view finder_version_page = 1 + R"(
finder 0.0.1

Copyright (C) 2020 Paul Scharnofske; Licensed under the terms of MIT.
)";

std::fstream open_database_output_stream(fs::path directory) {
    const auto cache_filepath = directory / finder_cache_path;

    std::fstream stream{cache_filepath, std::ios::out};
    encoder enc{stream};

    enc.write(finder_abi_version);

    return stream; // CE
}

std::fstream open_database_input_stream(fs::path directory) {
    const auto cache_filepath = directory / finder_cache_path;

    if(not fs::exists(cache_filepath)) {
        std::cerr << "error: no cache has been build for this directory\n";
        std::exit(EXIT_FAILURE);
    }

    std::fstream stream{cache_filepath, std::ios::in};
    decoder dec{stream};

    std::size_t abi_version;
    dec.decode(abi_version);

    if(abi_version != finder_abi_version) {
        std::cerr << "error: cache was build by a different version of finder\n";
        std::exit(EXIT_FAILURE);
    }

    return stream; // CE
}

void rebuild_cache(fs::path directory) {
    database db;
    for( const fs::path &path : fs::recursive_directory_iterator(directory) ) {
        db.add(path);
    }

    std::fstream output_stream = open_database_output_stream(directory);
    db.marshall(output_stream);
}

void locate_exact_filename(fs::path filename, fs::path directory) {
    if(filename.has_parent_path()) {
        std::cerr << "error: can't search for paths\n";
        std::exit(EXIT_FAILURE);
    }

    auto input_stream = open_database_input_stream(directory);

    lazy_database db{input_stream};
    for(const auto &path : db.locate(filename)) {
        std::cout << fmt::format("{}\n",
                                 path.native());
    }
}

void list_all_filepaths(fs::path directory) {
    auto input_stream = open_database_input_stream(directory);

    decoder dec{input_stream};

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
         do_list = false;

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
            do_rebuild = true;
            continue;
        }

        if(argument == "--list" or argument == "-l") {
            do_list = true;
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
        } else if(positional_arguments.size() == 1) {
            rebuild_cache(positional_arguments[0]);
        } else {
            std::cerr << fmt::format("error: invalid arguments\n\n{}",
                                     finder_usage_page);
            std::exit(EXIT_FAILURE);
        }
    }

    // Notice: It's possible to both rebuild the cache and the list all files in the same
    // command. The cache is rebuild before the filepaths are listed.

    if(do_list) {
        if(positional_arguments.size() == 0) {
            list_all_filepaths(".");
        } else if(positional_arguments.size() == 1) {
            list_all_filepaths(positional_arguments[0]);
        } else {
            std::cerr << fmt::format("error: invalid arguments\n\n{}",
                                     finder_usage_page);
            std::exit(EXIT_FAILURE);
        }
    }

    if(!do_list and !do_rebuild) {
        if(positional_arguments.size() == 1) {
            locate_exact_filename(positional_arguments[0], ".");
        } else if(positional_arguments.size() == 2) {
            locate_exact_filename(positional_arguments[0],
                                  positional_arguments[1]);
        } else {
            std::cerr << fmt::format("error: invalid arguments\n\n{}",
                                     finder_usage_page);
            std::exit(EXIT_FAILURE);
        }
    }
}
