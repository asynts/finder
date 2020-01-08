# finder

This is essentially locate from findutils but for local directories. The program recursivly
finds all files and writes them in a database `.findercache`. This database can then be
queried for filenames.

## Usage

~~~none
usage: finder [OPTIONS] FILENAME [DIRECTORY]
       finder [OPTIONS] --rebuild [DIRECTORY]
       finder [OPTIONS] --list [DIRECTORY]

OPTIONS
        --help          display usage information
        --version       display version information

    -r, --rebuild       rebuild cache
    -l, --list          list all filepaths from the cache
~~~

## Example

~~~none
$ tree
.
├── a
│   ├── b
│   │   ├── bar.txt
│   │   └── c
│   │       └── d
│   │           └── baz.txt
│   └── foo.txt
└── foo.txt

4 directories, 4 files
$ finder -r
$ finder foo.txt
foo.txt
a/foo.txt
$ finder --list
a/b/bar.txt
a/b/c/d/baz.txt
a/foo.txt
foo.txt
~~~
