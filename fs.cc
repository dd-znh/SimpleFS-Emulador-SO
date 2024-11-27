#include "fs.h"

int INE5412_FS::fs_format()
{
	union fs_block block;

	disk->read(0, block.data);

	class fs_superblock superblock = block.super;

	// Disco já formatado ou montado, retorna 0
	if (block.super.magic == FS_MAGIC) {
		return 0; // Erro de formatação
	}

	superblock.magic = FS_MAGIC; // Setta o magic number no superbloco
	superblock.nblocks = disk->size(); // Número total de blocos no disco
	superblock.ninodeblocks = superblock.nblocks / 10 + (superblock.nblocks % 10 != 0); // 10% dos blocos, arredondando pra cima
	superblock.ninodes = superblock.ninodeblocks * INODES_PER_BLOCK; // Número total de inodes

	// Apagar tabelas de inodes
	for(int i=1 ; i < block.super.ninodeblocks + 1; i++) {
		disk->read(i, block.data);
		for(int j=0 ; j < INODES_PER_BLOCK ; j++) {
			block.inode[j].isvalid = 0;
		}
	}
	return 1; // Formatação bem sucedida
}

void INE5412_FS::fs_debug()
{
	union fs_block block;

	disk->read(0, block.data);

	// Salvar o superbloco para extrair informações dele
	fs_superblock superblock = block.super;

	cout << "superblock:\n";
	cout << "    " << (superblock.magic == FS_MAGIC ? "magic number is valid\n" : "magic number is invalid!\n");
 	cout << "    " << superblock.nblocks << " blocks on disk\n";
	cout << "    " << superblock.ninodeblocks << " blocks for inodes\n";
	cout << "    " << superblock.ninodes << " inodes total\n";	

	// Iterar sobre os blocos de inodes
	for(int i=1 ; i < superblock.ninodeblocks +1; i++) {
		disk->read(i+1, block.data); // Lê cada bloco de inode
		// Iterar sobre os inodes do bloco
		for(int j=0 ; j < INODES_PER_BLOCK ; j++) {
			fs_inode inode = block.inode[j]; 
			// Para cada inode, se ele for válido, imprime seus blocos diretos
			if(inode.isvalid == 1) {
				cout << "inode " << (i-1)*INODES_PER_BLOCK+j+1 << ":\n";
				cout << "\tsize: " << inode.size << " bytes\n";
				cout << "\tdirect blocks:";
				for(int k=0 ; k < POINTERS_PER_INODE ; k++) {
					if(inode.direct[k] != 0) {
						cout << " " << inode.direct[k];
					}
				}
				// Se houver bloco indireto, acessa o blocos indireto e imprime os blocos de dados para os quais ele aponta
				cout << "\n";
				if(inode.indirect != 0) {
					cout << "\tindirect block: " << inode.indirect << "\n";
					disk->read(inode.indirect, block.data); // Lê o bloco indireto
					cout << "\tindirect data blocks:";
					// Itera sobre os blocos de dados para os quais o bloco indireto aponta
					for(int k=0 ; k < POINTERS_PER_BLOCK ; k++) {
						if(block.pointers[k] != 0) {
							cout << " " << block.pointers[k];
						}
					}
					cout << "\n";
					disk->read(i, block.data); // Por fim, restaura o bloco de inodes para encontrar o próximo inode
				}
			}
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
