# Introns database builder

## Installation

Requires Qt version 4.x or 5.x with **MYSQL support**. Check your installation.

```
qmake
make
make install
```

**Note 1: ** in some distros you should type `qmake-qt4` or `qmake-qt5`
corresponding to Qt's version to use instead of `qmake` command.

**Note 2: ** default installation location is `/usr/local/bin`. You can
override the path by passing `PREFIX=somewhere` after `qmake` command.

## Usage

### Common usage

```
introns_db_fill [OPTIONS] FILENAMES
```

 * `FILENAMES` - a list of GBK of compressed GBK (*.gbk.gz) file names to
 be processed. It is possible to pass a wildcard instead of list, e.g.
 `*.gbk.gz` or something like this
 * `OPTIONS` - optional additional parameters

### Additional parametets

Database connections parameters:

 * `--host=HOST_NAME` - use `HOST_NAME` to connect MySQL. If not provided,
 then `localhost` will be used
 * `--user=USER_NAME` - use `USER_NAME` to connect MySQL. If not provided,
 then `root` will be used
 * `--pass=PASSWORD` - the password for user. Default is empty
 * `--db` - MySQL database name. Default is `introns`

Input parameters:
 * `--use-data=DATAFILE.ini` - use additional data from `DATAFILE.ini`. If
 not specified, then correspoding by name `.ini` file will be used for each
 GBK input, if exists

Output parameters:
 * `--seqdir=OUTPUT_DIR_NAME` - store origins into `OUT_DIR_NAME` direcory.
 If not specified, then origins **will not be stored**.

Processing parameters:
 * `--threads=NUM_THREADS` - use specified `NUM_THREADS` workers to process.
 By default, all processors/cores will be used

