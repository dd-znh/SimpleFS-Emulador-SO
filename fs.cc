#include "fs.h"

int INE5412_FS::fs_format()
{
	return 0;
}

void INE5412_FS::fs_debug()
{
	union fs_block block;

	disk->read(0, block.data);

	cout << "superblock:\n";
	cout << "    " << (block.super.magic == FS_MAGIC ? "magic number is valid\n" : "magic number is invalid!\n");
 	cout << "    " << block.super.nblocks << " blocks on disk\n";
	cout << "    " << block.super.ninodeblocks << " blocks for inodes\n";
	cout << "    " << block.super.ninodes << " inodes total\n";

	for(int i=0 ; i < block.super.ninodeblocks ; i++) {
		disk->read(i+1, block.data);
		for(int j=0 ; j < INODES_PER_BLOCK ; j++) {
			//cout << "Reading inode " << j << " from inode block " << i << '\n';
			fs_inode inode = block.inode[j];
			if(inode.isvalid == 1) {
				cout << "inode " << i*INODES_PER_BLOCK+j << ":\n";
				cout << "\tsize: " << inode.size << " bytes\n";
				cout << "\tdirect blocks:";
				for(int k=0 ; k < POINTERS_PER_INODE ; k++) {
					if(inode.direct[k] != 0) {
						cout << " " << inode.direct[k];
					}
				}
				cout << "\n";
				if(inode.indirect != 0) {
					cout << "\tindirect block: " << inode.indirect << "\n";
					disk->read(inode.indirect, block.data);
					cout << "\tindirect data blocks:";
					for(int k=0 ; k < POINTERS_PER_BLOCK ; k++) {
						if(block.pointers[k] != 0) {
							cout << " " << block.pointers[k];
						}
					}
					cout << "\n";
				}
			}
		disk->read(i+1, block.data);
		}
	}
}

int INE5412_FS::fs_mount()
{
	return 0;
}

int INE5412_FS::fs_create()
{
	return 0;
}

int INE5412_FS::fs_delete(int inumber)
{
	return 0;
}

int INE5412_FS::fs_getsize(int inumber)
{
	return -1;
}

int INE5412_FS::fs_read(int inumber, char *data, int length, int offset)
{
	return 0;
}

int INE5412_FS::fs_write(int inumber, const char *data, int length, int offset)
{
	return 0;
}
