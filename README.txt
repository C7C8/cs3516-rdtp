To compile the code, simply hit `make` (or `make all` or `make rdtp`, it doesn't make a difference).
The executable generated will be named "rdtp". Run it with any number of packets, a time between
packets of 500, and no bidirectionality  (e.g. ./rdtp 500000 0.25 0.25 0.25 500 1 1 0)

Please note that you will need to have zlib installed. This is usually a core library, and is installed
on both ccc.wpi.edu and rambo.wpi.edu. zlib is only included so that I may use the crc32 function, a
checksum function for error detection that zlib helpfully provides.

Ultimately, due to time concerns, the optional GBN protocol was not implemented. However, I have
written a seemingly robust implementation of RDT 3.0, to the point that it can handle 1,000,000
packets with 0.9 probability for all network attributes.

Since the output of the program for 50,000 packets is extensive, I've provided the required output
trace as a .txt file instead. I hope that's okay!
