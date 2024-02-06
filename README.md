
The "simfs structure" project is a simulated file system designed as an array of blocks, each consisting of a fixed size. Metadata, including file entries (fentries) and file nodes (fnodes), is stored at the beginning of the file.

An fentry contains the file name (up to 11 characters), size of the file, and an index pointing to the corresponding fnode, which holds information about the file's data blocks.

A fnode holds the index of the data block storing file data and an index pointing to the next fnode, indicating the continuation of the file.

With fixed numbers of fentries and fnodes, the required space can be calculated. These arrays are stored consecutively in the file. The allocation of blocks depends on the number of files and nodes.

While it's ideal to adjust the maximum number of files and blocks to fit exactly within N blocks, it's not always feasible due to potential wasted space. Additionally, file data blocks for a single file may not be contiguous.

The project provides a flexible and efficient framework for managing files within a simulated file system.
