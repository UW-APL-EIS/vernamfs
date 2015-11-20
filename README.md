# VernamFS - A secure filesystem based on one-time-pads.

## Introduction

VernamFS is a FUSE-based filesystem for storing data securely.  It is
suited to remote deployment of systems ('sensors') that collect/store
data that should not be readable by others.  It uses a one-time-pad
(OTP) file (or whole device) for its persistent storage.  Sections of
the pad are used up as data is written to it.

The basic idea is that you create two identical copies of a OTP.  One
copy goes into the field (we use the term 'remote unit') to hold
secure data.  The other copy goes in a 'vault'.  After the remote unit
is recovered, its OTP is combined with the vault copy to recover the
stored data.

In the field, the VernamFS is a write-only filesystem.  Data written
to it is unreadable, even by the VernamFS code itself!  The only
'information leaks' are knowledge of allocated file counts and
percentage of the OTP used.  No information is leaked about file
content, nor even file size nor even file name.

Due to the simple binary-XOR operation at the center of everything in
the OTP domain, the VernamFS is also very cheap in terms of CPU
cycles.  It is much cheaper than traditional encrypting techniques
like AES, etc.  It also needs no keys stored on the remote unit, nor
is reliant on the quality (or lack thereof!) of any RNG on the remote
unit.

## Prerequisites

VernamFS is standard Unix/C code with a Makefile.  It has so far been
built and tested on both Debian and Fedora variants of Linux.  It may build on
other systems, but is untested.

To build on Linux, these tools/libraries are required:

1 GNU make

2 gcc

3 Fuse headers and library.  Check existence of
/usr/include/fuse.h. If not installed, install fuse development
package. On Fedora-based systems 'yum install fuse-devel'.  On
Debian-based systems 'apt-get install libfuse-dev'.

4 Useful by not required: the xxd, strings tools for binary data
inspection.

## Build

We have built VernamFS for both x86 and ARM environments.  Most
likely, you want to try it on your laptop/desktop, a so-called
'native' environment (code built and run on same
machine/architecture):

```
$ cd /path/to/vernamfs/target/native

$ make

$ vernamfs	(or ./vernamfs if . not in your PATH)
```
should produce a short usage/help summary.

As with git, there is just a single binary for VernamFS, namely
'vernamfs'.  It does different tasks based on subcommands.

## Usage

### OTP Creation

First step is to create a one-time-pad file.  One way is via
/dev/random or /dev/urandom.  This dd will create a 1MB one-time-pad
in file OTP:

```
$ dd if=/dev/urandom of=OTP count=2048
```

For a larger one-time-pad, using /dev/[u]random could take a looooong
time. An alternative way, using a 'cryptographic random number
generator', is available in VernamFS.  It uses the block cipher AES in
CTR mode to generate a pseudo-random OTP.  It expects a hex-encoded
key on stdin, and the base2 log of the desired one-time-pad size as
its argument.  The result is on stdout, so use a redirect to capture
it to a file/device:

```
$ echo The cat sat on the mat | md5sum | cut -b 1-32 > KEY

2^20 is 1MB, 2^30 is 1GB
$ vernamfs generate 20 < KEY > OTP
$ vernamfs generate 20 < KEY > /dev/sdcard
```

## VernamFS Initialization

All filesystems need some 'boot-sector'-like data structure for
housekeeping.  In VernamFS, this is a minimal 'header' of about 128
bytes.  It gets written at start of the OTP.  This header is the ONLY
area of the OTP which is subsequently read-write.  The remainder of
the OTP will be write-only.

We have to decide the maximum number of files we expect to create on
the OTP, say 1024, at initialization time:

```
$ vernamfs init OTP 1024
```

We can inspect the resultant 'header' on the OTP via the info
subcommand, which simply pretty-prints the stored header:

```
$ vernamfs info OTP
```

The info command can be run at any time, not just after initialization.

## Vault Copy

We now make a copy of the OTP and put it in a 'vault' for safe keeping:

```
$ cp OTP OTP.V
$ mv OTP.V /some/safe/place
```

The vault copy must NEVER be written, so we might at least also do this:

```
$ chmod a-w OTP.V
```

Henceforth we shall use 'remote' and 'vault' in the shell prompt
string to denote where commands are run.

## Runtime Setup


We deploy the original OTP into the field, and make it available for
writing processes.  This uses the magic of FUSE (Filesystem in
Userspace), making it transparent to the filesystem user:

```
remote$ mkdir mnt
remote$ ./vernamfs mount OTP mnt
remote$ mount
```

To see debug, run in the foreground in a dedicated terminal:

```
remote$ ./vernamfs mount OTP -f mnt
```

## Runtime Use

We can now store data onto our OTP as easy as writing to any other
location.  This sequence stores data for three files in the VernamFS:

```
remote$ echo MyData > mnt/File1
remote$ cp File2 mnt
remote$ seq 1000 > mnt/File3
```

Operations on the VernamFS are of course not limited to existing
programs (echo,cp,etc).  Any program which uses the open/write/close
system call sequence can use the VernamFS.

One unfortunate property of the VernamFS, and this is purely down to
how FUSE and the Linux VFS subsystem work, is that no directories are
possible under the mountpoint.  We have to make do with a single
directory:

``` 
remote$ mkdir mnt/someDir 
mkdir: cannot create directory `mnt/someDir':File exists 
``` 

is the rather confusing response from a mkdir attempt.  If directories
were allowed, the filesystem would have to be able to distinguish
between names that denoted regular files and names that denoted
directories.  This in turn implies that the filesystem can remember
names. It cannot. They are encrypted when written to disk!

VernamFS isn't really a filesystem at all, nor does it even really
deal in 'files'.  It's more a list of elements E where E is a pair: a
string S which resembles a file path and some content (a byte
sequence) C associated with S.  Elements can be added, but never
removed.  Moreover, elements cannot be listed (the unlistable list!)
-- we cannot print any S.  Nor can we read any C.  There are no
owners, groups, permissions, creation times.  S and C is all there is
for any given file.

The filesystem is truly write-only:

```
remote$ ls mnt
Operation not supported
remote$ cat mnt/File1
Operation not supported
```

## Shutdown

The VernamFS mountpoint is closed down like any other FUSE-based filesystem:

```
remote$ fusermount -u mnt
remote$ mount
```

## Analysis

Of course, it is trivial to simply 'side-step' the VernamFS (which
just controls the mountpoint) and inspect the OTP itself:

```
remote$ xxd -l 128 OTP
```

will reveal the VernamFS header.  It has a magic number at offset 0.
At specific offsets are two cursors pointing to (1) the next free file
in the 'file allocation table' and (2) the next free data chunk, but
that's about it.

Once past the header however, nothing about any of the write
operations above (echo, cp, seq) is locatable, neither by file name:

```
# This would locate file name 'File1', the target of the echo command
remote$ strings OTP | grep File1
```

nor by file content:

```
# This would locate content '1000', produced by the seq command
remote$ strings OTP | grep 1000
```


## End-of-Mission Recovery

We have seen above that the VernamFS can be written to but never read.
Thus data in the field is completely unintelligible to anyone gaining
access to the remote unit.

Once the field unit, with its OTP, is recovered, it is simply combined
with the vault copy and the original stored data produced.  This is
essentially one large XOR operation over the entire (used portion) of
the OTP.  We supply a directory to dump all the newly-created files:

```
vault$ copy remote pad OTP to here
vault$ mv /some/safe/place/OTP.V .
vault$ mkdir data
vault$ ./vernamfs recover OTP OTP.V data  (NOT DONE YET)
```

As per the requirements of any OTP, we can NEVER re-use the pads:

```
vault$ rm OTP OTP.V
```

## In-Mission Recovery

In some cases, it might be possible to query the remote unit during
deployment.  If so, it is likely possible to send back portions of the
remote data to the local vault location, before final recovery.
Obviously the data stored on the remote unit OTP is encrypted.  So we
cannot understand it, but if we can read it, and can ship it back to
the vault location, we can apply the recover operation above to
selected parts of the OTPs only.

### File Listing

The first step in this 'in-mission access' situation is to get the
remote unit's file table.  This is the meta data which describes file
names, content lengths and content offsets.  The rls (remote ls)
subcommand is used:

```
remote$ vernamfs rls OTP
```

which produces a binary blob.  Can at least inspect it using xxd:

```
remote$ vernamfs rls OTP | xxd
```

The rls result is actually saved to a file and then shipped back to
the vault location:

```
# The temp file name used is arbitrary
remote$ vernamfs rls OTP > rls.result

remote$ ship rls.result to vault

# Delete the temp file, though it leaks no more info than the header
remote$ rm/scrub rls.result
```

The rls listing reveals nothing about names File1, File2, File3.  This
is due to the logic inherent in the VernamFS. Each byte of those names
was XOR'ed with the original OTP and the result written back to the
same location in the pad, thus overwriting the original byte value.

However, combined with the vault (read-only!) copy of the original
OTP, the listing is deciphered. The vls (vault ls) subcommand is used.
We supply the rls result as a file:

```
vault $ vernamfs vls OTP.V rls.result

/File1 0x2000 0x4
/File2 0x3000 0x0
/File3 0x4000 0xf35
```

The original file names, together with content offsets and lengths,
are produced!

For lab testing, where remote and vault locations can in fact be the
same directory on a single host, vls can also accept the rls result on
stdin.  Then either a Unix pipe or file redirect can be used:

```
lab$ vernamfs rls OTP | vernamfs vls OTP.V

lab$ vernamfs rls OTP > rls.result

lab$ vernamfs vls OTP.V < rls.result
```

For direct comparison of the in-lab vls result with that of the
original rls result, we can use the -r option (print raw) to vls,
together with xxd:

```
lab$ vernamfs rls OTP | vernamfs vls -r OTP.V | xxd
```
The true file meta data is clearly identifiable in the listing. 


### File Content

The vls result is then inspected for file names of interest.  The
offset and length of relevant files are fed into rcat (remote cat),
currently one at a time.  The rcat subcommand retrieves the
still-enciphered remote data at the correct location in the remote
OTP.  This example extracts the content, still encrypted, of File1:

```
# Transcribe the offset and length values from vls to here
remote$ vernamfs rcat OTP 0x2000 4 | xxd
```

The xxd listing shows the still-enciphered content of File1.

The rcat output is in fact stored to a file, then shipped to the vault
location:

```
# The temp file name used is arbitrary
remote$ vernamfs rcat OTP 0x2000 4 > rcat.0x2000.4

remote$ ship rcat.0x2000.4 to vault

# Delete the temp file so as to not leak offset,length info
remote$ rm/scrub rcat.0x2000.4  

```

The final step is to XOR this blob with the vault OTP at the matching
location.  The vcat (vault cat) subcommand is used:

```
vault$ vernamfs vcat OTP.V rcat.0x2000.4
```

and the result printed to stdout.  We'd likely re-direct this to a
local name matching that in the vls listing:

```
vault$ vernamfs vcat OTP.V rcat.0x2000.4 > File1
```
 
## Concerns

On SD cards (and other persistent storage device types), is there any
remnant of the original OTP byte B once overwritten with the XOR'ed
result C of B with data byte D??  If yes, then having C and B reveals
D!


# Acknowledgments

VernamFS named after Gilbert Vernam, a pioneer in the field of data
security and one-time-pads.


Stuart Maclean
mailto:stuart@apl.washington.edu
November 2015







