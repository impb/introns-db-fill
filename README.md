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

### Database initialization

Database must be initialized by schema before first use. Database schema
provided in file `create_database.sql`. To run this code in MySQL:

```
mysql -p -u USER_NAME DATABASE_NAME < create_database.sql
```

Where:

 * `USER_NAME` - database user name
 
 * `DATABASE_NAME` - database instance name
 

For other possible parameters see MySQL Reference.

**Note 3: ** database initialization will drop all previously created and
filled up tables!. Use with care.


### Common usage

```
introns_db_fill [OPTIONS] FILENAMES
```

 * `FILENAMES` - a list of GBK (&#42;.gbk) file names to
 be processed. It is possible to pass a wildcard instead of list, e.g.
 `*.gbk` or something like this

 * `OPTIONS` - optional additional parameters (see below)

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

 * `--logfile=LOG_FILE_NAME` - store error information into plain text file
 `LOG_FILE_NAME`. If not specified, error messages will be passed into standard
 error stream (e.g. terminal).


### Run example

Suppose we are in the some working directory, where subdirectory `Callithrix_jacchus` 
exists. To process all gbk files from this directory, and use file `Callithrix_jacchus.bio`
as additional data just run:

```
introns_db_fill --db=introns --user=introns --pass=qwerty456 \
                --use_data=Callithrix_jacchus.bio \
                --seqdir=../out_sequences 
                --logfile=Callithrix_jacchus.log \
                Callithrix_jacchus/*.gbk
```

After this run the following will be created:

 1. File `Callithrix_jacchus.log` (might be empty) containing error messages
 
 2. Subdirectory `callithrix_jacchus` (as of species name) in directory 
 `../out_sequences`, which contains directories `1`, `2`, ... `22`, 
 `x`, `y`, `unknown` named by chromesome name. Each of these directory contains
 files named like `nw_XXXXXXXX.raw.txt`, where file name base corresponds to 
 each sequence locus name. File contents is a RAW text sequence (with no spaces,
 newlines etc.) matching correspongind locus.
 
 3. In the database specified by command line arguments data according to 
 processed data appears.