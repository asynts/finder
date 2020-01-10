# finder

This is essentially locate from findutils but for local directories.

The program recursivly searches for files and builds a database which can be
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

## Examples

Finder can be used to locate all filepaths in a directory that have the same filename:

<pre><code><b>$ mkdir -p a/b/c</b>
<b>$ touch foo.txt a/bar.txt a/b/foo.txt a/b/c/bar.txt</b>
<b>$ finder --rebuild                                      # rebuild the cache</b>
<b>$ finder foo.txt                                        # search for 'foo.txt'</b>
./foo.txt
./a/b/foo.txt
<b>$ finder --list                                         # list all cached files</b>
./foo.txt
./a/bar.txt
./a/b/foo.txt
./a/b/c/bar.txt
<b>$ touch bar.txt</b>
<b>$ finder --list                                         # finder doesn't know about './bar.txt'</b>
./foo.txt                                              <b># until the cache is rebuild</b>
./a/bar.txt
./a/b/foo.txt
./a/b/c/bar.txt
</code></pre>

The `--list` option can be used with `grep` or other utilities to search for more complex patterns:

<pre><code><b>$ mkdir -p a/b/c</b>
<b>$ touch foo.txt a/bar.txt a/b/foo.png</b>
<b>$ finder --rebuild --list | grep '.txt$'                 # find all the files that have the file</b>
./foo.txt                                               <b># extension '.txt'</b>
./a/bar.txt
</code></pre>
