# finder

This is essentially locate from findutils but for local directories. The program recursivly
finds all files and writes them in a database `.findercache`. This database can then be
queried for filenames.

## Usage

~~~none
usage: finder [OPTIONS] FILENAME [DIRECTORY]
       finder [OPTIONS] --rebuild [DIRECTORY]

OPTIONS
        --help          display usage information
        --version       display version information

    -r, --rebuild       rebuild cache
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
~~~
