#include <iostream>
#include <string_view>

using namespace std::literals::string_view_literals;

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
