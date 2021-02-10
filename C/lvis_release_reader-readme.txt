OVERVIEW

This readme accompanies the IceBridge LVIS data reader:
lvis_release_reader.c

The lvis_release_reader.c program reads a binary data file from the
Operation IceBridge LVIS instrument and prints the records to standard
output (normally, the screen).  Data files from the LVIS instrument are
available as the ILVIS1B product and the ILVIS2 product at the National
Snow and Ice Data Center (NSIDC), at

http://nsidc.org/data/ilvis1b.html

and

http://nsidc.org/data/ilvis2.html

This software is available at

http://nsidc.org/data/icebridge/tools.html

DISCLAIMER

This software is provided as-is as a service to the user community in the
hope that it will be useful, but without any warranty of fitness for any
particular purpose or correctness.  Bug reports, comments, and suggestions
for improvement are welcome; please send to nsidc@nsidc.org.


USING THE SOFTWARE

There are six files in the downloaded tar file:

example_output.txt
  The first few lines of output from an example input file.

lvis_release_structures.h
  A header file needed to build the reader

lvis_release_reader.c
  The  C language source for the reader

lvis_release_reader-readme.txt
  This file

Makefile
  File used to compile the reader executable program: lvis_release_reader.

win32_HOWTO.txt
  Instructions for compiling the source code on a Windows platform.


To compile the reader on a Linux system:

Unpack the tar file (the "$" is the Linux command-line prompt):
  $ tar xvf C_lvis_release_reader_0.1.tar

To compile, use the "make" program:
  $ make


The result of these commands should be an executable program named
lvis_release_reader.

The program does not use any graphics systems, and so should be easy to
compile on other systems besides Linux.


Using the reader:

Print the first few lines of a file in ASCII (text) format:

  ./lvis_release_reader inputfile | head


Print the first two lines of a file in ASCII (text) delimited with commas:

  ./lvis_release_reader inputfile -n 2 -c


Convert an entire binary input file to a (possibly very large) text file:

  ./lvis_release_reader inputfile > example_output.txt
