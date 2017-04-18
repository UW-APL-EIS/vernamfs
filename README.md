# VernamFS - A secure filesystem based on one-time pads.

## Introduction

The Vernam Filesystem (VernamFS) is a FUSE-based filesystem for
storing data securely.  It is suited to remote deployment of systems
('sensors') that collect/store data that should not be readable by
others.  It uses a one-time pad (OTP) file, or whole device, for its
persistent storage.  The pad is used up as data is written to it.

The basic idea is that you create two identical copies of a OTP.  One
copy goes into the field (we use the term *remote unit*) to hold files
which can be written but not read.  The other copy goes in a *vault*.
After the remote unit is recovered, its OTP is combined with the vault
copy to recover the remote data.

The VernamFS is a write-only filesystem.  Data written to it is
unreadable, even by the VernamFS code itself!  The only information
leaked by the filesystem is knowledge of allocated file counts and
percentage of the remote OTP used.  No information is leaked about
file content, nor even file sizes (!) nor even file names (!!).

Due to the simple bitwise-XOR operation at the center of everything in
the OTP domain, the VernamFS is also very cheap in terms of CPU cycles
and thus power consumption.  It is much cheaper than traditional disk
encrypting techniques, which typically use block ciphers such as AES.
Further, it needs no cryptographic keys, so the key distribution
problem is moot.  Finally, it is not reliant on the quality (or lack
thereof!) of any random number generator (RNG). It needs only
bitwise-XOR.

## Versioning

This git repository follows [Driessen's](http://nvie.com/posts/a-successful-git-branching-model/) strategies
for branching and release management.  The latest tagged release is
[1.0.1](https://github.com/UW-APL-EIS/vernamfs/tree/1.0.1), which is
also the head of the [master](https://github.com/UW-APL-EIS/vernamfs/tree/master) branch. The
[ChangeLog](https://github.com/UW-APL-EIS/vernamfs/blob/master/ChangeLog) gives a
release history. The [develop](https://github.com/UW-APL-EIS/vernamfs/tree/develop) branch likely
contains ideas/code newer than that most recent tag.

Though VernamFS is a standalone application and is unlikely to become
a dependency of any other work, we follow the model of
[semantic versioning](http://semver.org/) for all tagged releases.

## Prerequisites

VernamFS is standard Unix/C code, packaged with a Makefile. It depends
upon, and uses, FUSE (Filesystem in Userspace). It has so far been
built and tested on both Debian and Fedora variants of Linux.
It may build on other systems (there are FUSE ports for Mac OS, but
not for Windows), but is untested.  In theory, the OTP logic in
VernamFS could be separated from the FUSE parts and packaged as an
standalone API/library. This has not been done to date.

To build VernamFS on Linux, these tools/libraries are required:

1. GNU make

2. gcc

3. pkg-config, used in the Makefile

4. FUSE header (.h) and library (.a, .so) files

5. Useful by not required: xxd, strings for binary data inspection

For FUSE, check existence of /usr/include/fuse.h. If missing, install
fuse development package. On Fedora-based systems:

```
$ sudo yum install fuse-devel
```

On Debian-based systems:

```
$ sudo apt-get install libfuse-dev
```


## Build

We have built VernamFS for both x86 and ARM environments (our
remotely-deployed sensor is ARM-based).  Most likely, you want to try
it on your laptop/desktop, a so-called 'native' environment.  The code
is built and runs on the same the machine/architecture:

```
$ cd /path/to/vernamfs

$ make

$ ./vernamfs
```

should produce a short usage/help summary.

As with git, there is just a single binary for VernamFS, namely
'vernamfs'.  It does different tasks based on subcommands.

## Usage

### One Time Pad Creation

First step is to create a one-time pad file.  Traditional Linux way is
via /dev/random (best, but slowest) or /dev/urandom (inferior, but
faster).  This dd will create a 1MB one-time pad in file OTP:

```
$ dd if=/dev/urandom of=OTP count=2048
```

For a larger one-time pad, using /dev/random in particular could take
a looooong time.

An alternative method for OTP creation is available within VernamFS
itself, via the generate subcommand. This uses a cryptographically
secure pseudo-random number generator (CSPRNG), namely the block
cipher AES in CTR mode.  The cipher is applied to output 16 bytes as
many times as is required to produce the desired length of OTP, with
the CTR incremented at each step. The user just supplies the AES key.
A canned one, of all zeros, is also available for testing.

The generate subcommand must also be fed the desired one-time pad
size.  We do this by supplying the base2 log of that size as a command
argument.  Here's how to generate a 1MB (2^20) OTP using the all-zeros
test key.  The result of the generator is written to stdout, so use a
redirect to capture it to a file/device:

```
$ ./vernamfs generate -z 20 > OTP

$ ./vernamfs generate -z 30 > /dev/sdCard
```

In the case of a real, user-derived key, the generator expects a
hex-encoded key on stdin. A 16-byte key is easily obtained by taking
the MD5 sum of any text:

```
$ echo The cat sat on the mat | md5sum | cut -b 1-32 > KEY
```

and that key fed to the generator via pipe or redirect:

```
$ ./vernamfs generate 20 < KEY > OTP

$ cat KEY | ./vernamfs generate 30 > /dev/sdCard
```

## VernamFS Initialization

All filesystems need some 'boot-sector'-like data structure for
housekeeping.  In VernamFS, this is a minimal 'header' of about 128
bytes.  It is written at the beginning of the OTP.  This header is the _only_
area of the OTP which is subsequently read-write, and so could leak
information to an adversary.  The remainder of the OTP will be
write-only.

The init subcommand is used to initialize the OTP.  We have to decide
the maximum number of files we expect to create on the OTP, say 1024,
at initialization time:

```
$ ./vernamfs init OTP 1024
```

We can inspect the resultant 'header' on the OTP via the info
subcommand, which simply pretty-prints the stored header:

```
$ ./vernamfs info OTP

// Expert-mode, shows all header members
$ ./vernamfs info OTP -e
```

The info command can be run at any time, not just after initialization.

## Vault Copy

We now make a copy of the OTP and put it in a 'vault' for safe keeping:

```
$ cp OTP OTP.V
$ mv OTP.V /some/safe/place
```

The vault copy must _never_ be written, so we might at least also do this:

```
$ chmod a-w OTP.V
```

A truly random OTP and one generated by a CSPRNG (e.g. AES/CTR) method
actually have different vault storage requirements.  In the truly
random case, the entire OTP must be stored, since it cannot be
re-created.  In the CSPRNG case, only the initial key needs
safe-guarding.  The actual OTP need not be stored at all; it can be
regenerated at will.

The initial OTP is then deployed to the remote unit.  Henceforth we
shall use 'remote' and 'vault' in the shell prompt string to denote
where commands are run.

## Remote Runtime Setup

With the original OTP now installed in the remote unit, along with the
vernamfs program, we make the OTP available for writing processes.
This uses the magic of FUSE, making the VernamFS largely transparent
to the remote filesystem user.  The mount subcommand initiates a FUSE daemon
process:

```
# Create a mount point
remote$ mkdir mnt

# This backgrounds to become a daemon process
remote$ vernamfs mount OTP mnt

# Check the mount point is available
remote$ mount
```

To see debug, run in the foreground, in a dedicated terminal:

```
remote$ vernamfs mount OTP -f mnt
```

## Remote Runtime Use

We can now store data onto our OTP as easily as writing to any other
location.  This sequence stores data for three files into the
VernamFS:

```
remote$ echo MyData > mnt/File1
remote$ cp File2 mnt
remote$ seq 1000 > mnt/File3
```

Operations on the VernamFS are of course not limited to existing
programs (echo,cp,seq,etc).  Any program which uses the open/write/close
system call sequence can use the VernamFS.

One unfortunate property of the VernamFS, due entirely to how FUSE and
the Linux VFS subsystem work, is that no directories are possible
under the mountpoint.  We have to make do with a single directory:

``` 
remote$ mkdir mnt/someDir 
mkdir: cannot create directory `mnt/someDir':File exists 
``` 

is the rather confusing response from a mkdir attempt.  If directories
were allowed, the filesystem would have to be able to distinguish
between names that denoted regular files and names that denoted
directories.  This in turn implies that the filesystem can remember
names. It cannot. They are XOR-encrypted when written to disk, just
like the file content.

VernamFS isn't really a filesystem at all.  It doesn't really deal in
'files' as we know them.  Instead, a VernamFS is more a growing list of
elements E where each E is a pair: a name string S which resembles a
file path and some content C (a byte sequence) associated with S.
Elements can be added, but never removed.  Moreover, elements cannot
be listed (an unlistable list!), so we cannot print any name S.  Nor
can we read any C.  There are no owners, groups, permissions, creation
times, etc.  S and C is all there is for any given 'file'.

The filesystem is truly write-only.  These read operation fail:

```
remote$ ls mnt
Operation not supported
remote$ cat mnt/File1
Operation not supported
```

## Remote Shutdown

The VernamFS mountpoint is closed down like any other FUSE-based filesystem:

```
remote$ fusermount -u mnt

# Check the mount point is now unavailable
remote$ mount
```

which is likely performed during system shutdown. The unmount will
cause the VernamFS daemon to exit.

## Remote Analysis

Of course, it is trivial to simply side-step the VernamFS (which just
controls the mountpoint) and inspect the backing store OTP file
itself:

```
remote$ xxd -l 128 OTP
```

will reveal the VernamFS header.  It still has the magic number at
offset 0 installed by the init subcommand.  At specific offsets are
two cursors pointing to (1) the next free file in the 'file allocation
table' and (2) the next free data chunk.  You could monitor them
changing as files are written to the VernamFS. 

Once past the header however, nothing about any of the write
operations above (echo, cp, seq) is locatable, neither by file name:

```
# We know we wrote to 'File1', the target of the echo command
remote$ strings OTP | grep File1
```

nor by file content:

```
# We know we wrote the content '1000', produced by the seq command
remote$ strings OTP | grep 1000
```


## End-of-Mission Recovery

We have seen above that the VernamFS can be written to but never read.
Thus data in the field is completely unintelligible to anyone gaining
access to the remote unit.

Once the field unit, with its OTP, is recovered and transported back
to the vault location, it is simply combined with the vault copy and the
original stored data recovered.  This is essentially one large XOR
operation over the entire (used portion) of the OTPs.  The VernamFS
recover subcommand handles this operation.  We supply a directory to
hold all the newly-created files:

```
vault$ ship remote OTP to here
vault$ mv /some/safe/place/OTP.V .
vault$ mkdir data
vault$ vernamfs recover OTP OTP.V data
```

As per the requirements of any OTP, we can _never_ re-use the pad data.
After the recovery is complete:

```
vault$ rm OTP OTP.V
```

## In-Mission Recovery

In some cases, it might be possible to query the remote unit during
deployment, i.e. before final recovery.  If so, it is likely also
possible to send back portions of the remote data to the local vault
location.  Obviously the data stored on the remote unit OTP is
encrypted.  We cannot understand it, but if we can access it, and can
ship it back to the vault location, we can apply the recover operation
above to selected parts of the OTPs only.

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

The rls result would then be saved to a file and shipped back to the
vault location:

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
We supply the rls result as a file argument:

```
vault$ vernamfs vls OTP.V rls.result

/File1 0x2000 0x4
/File2 0x3000 0x0
/File3 0x4000 0xf35
```

The original file names, together with content offsets and lengths,
are produced on stdout.  The result above says that File1 is 4 bytes in
length, and sits 8192 (0x2000) bytes into the remote OTP.

To aid in-lab testing, where remote and vault locations can in fact be
the same directory on a single host, vls can also accept the rls
result on stdin.  Then either a Unix pipe or file redirect can be
used:

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

The true file meta data is clearly identifiable in the
listing. Contrast this with the earlier remote-only result:

```
lab$ vernamfs rls OTP | xxd
```

### File Content

The vls result is inspected for file names of interest.  The offset
and length of relevant files are then fed into rcat (remote cat),
currently one at a time.  The VernamFS rcat subcommand retrieves the
still-enciphered remote data at the correct location in the remote
OTP.  We are accessing the remote content by offset, which is
possible, rather than by name, which is not.

This example extracts the content, still encrypted, of File1:

```
# Transcribe the offset, length values for File1 from vls to here
remote$ vernamfs rcat OTP 0x2000 4 | xxd
```

The xxd listing shows the still-enciphered content of File1.

In practice, the rcat output is stored to a file, which is then
shipped back to the vault location:

```
# The temp file name used is arbitrary
remote$ vernamfs rcat OTP 0x2000 4 > rcat.0x2000.4

remote$ ship rcat.0x2000.4 to vault

# Delete the temp file so as to not leak any offset,length info
remote$ rm/scrub rcat.0x2000.4
```

The final step is to XOR this blob with the vault OTP at the matching
location.  The vcat (vault cat) subcommand is used for this:

```
vault$ vernamfs vcat OTP.V rcat.0x2000.4
```

and the result printed to stdout.  We'd likely re-direct this to a
local name matching that in the vls listing:

```
vault$ vernamfs vcat OTP.V rcat.0x2000.4 > File1
```

If the rls result is still available, it can automate the saving of
the recovered data to a file whose name matches that on the remote
unit.  It is passed as an optional third argument to vcat:

```
vault$ vernamfs vcat OTP.V rcat.0x2000.4 rls.result
```

will create and populate ./File1.
 
## How It Works

VernamFS uses a file or whole device which starts life as a one-time
pad, i.e a stream of random bytes.  VernamFS imposes a basic structure
on that file: a header, a file metadata area, and a content area.

For every file allocation and file content write, the following
happens.  First, we allocate a new file by using the next available
file table entry in the file allocation table.  This available entry
is stored in the header.  The file name to be stored is XOR'ed with
the contents of the OTP at that location, and the results written back
to the OTP at the same location.  A similar sequence of 'allocate
space, read OTP content, XOR with new file content, write results back
to OTP' is performed for file content storage.


## Concerns

On SD cards (and other persistent storage device types), is there any
remnant of the original OTP byte B once overwritten with the XOR'ed
result C of B with data byte D??  If yes, then having C and B reveals
D!


# Acknowledgments, References, Links

VernamFS named after Gilbert Vernam, a pioneer in the field of data
security and one-time-pads.

* https://en.wikipedia.org/wiki/Pseudorandom_number_generator

* https://en.wikipedia.org/wiki/Gilbert_Vernam

* http://fuse.sourceforge.net/

# Credit/Contact

Original Idea for the OneTimePad FileSystem by Richard
Campbell. This implementation by Stuart Maclean.

Stuart Maclean

Applied Physics Laboratory

University of Washington

mail2 stuart AT apl DOT washington DOT edu





