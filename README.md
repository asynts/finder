# finder

## Features

  - Build a database of files contained in a directory. This database is stored
    in a user specific directory, but not the directory beeing indexed.

  - Allow the user to query the database for specific file names or prefixes
    of filenames. This operation should be very quick, definitly faster than
    `find`.

## Usage

~~~none
usage: finder [OPTIONS] PATTERN [DIRECTORY]

OPTIONS:
        --version   print version information
        --help      print this page

    -r, --rebuild   rebuild the database
    -s, --stats     print stats about the database
    -p, --prefix    search for files starting with this prefix, instead of
                    exact matches.
~~~
