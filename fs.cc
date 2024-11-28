#include "fs.h"

int INE5412_FS::fs_format()
{
	union fs_block block;

	disk->read(0, block.data);

	// Disco já formatado ou montado, retorna 0
	if (block.super.magic == FS_MAGIC) {
		return 0; // Erro de formatação
	}

	union fs_block new_block_zero; // Cria um bloco de dados vazio

	new_block_zero.super.magic = FS_MAGIC; // Setta o magic number no superbloco
	new_block_zero.super.nblocks = disk->size(); // Número total de blocos no disco
	new_block_zero.super.ninodeblocks = disk->size() / 10 + (disk->size() % 10 != 0); // 10% dos blocos, arredondando pra cima
	new_block_zero.super.ninodes = new_block_zero.super.ninodeblocks * INODES_PER_BLOCK; // Número total de inodes

	int ninodeblocks = new_block_zero.super.ninodeblocks; // Salva o número de blocos de inodes

	disk->write(0, new_block_zero.data); // Escreve o novo bloco zero no disco

	// Apagar tabelas de inodes
	for(int i=1 ; i < ninodeblocks + 1; i++) {
		union fs_block new_inode_block; // Cria um bloco de inodes vazio

		//Invalida todos os inodes do bloco
		for(int j=0 ; j < INODES_PER_BLOCK ; j++) {
			new_inode_block.inode[j].isvalid = 0;
		}
		disk->write(i, new_inode_block.data); // Escreve o bloco de inodes vazio no disco
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

	fs_inode *inode;

	// Iterar sobre todos os inodes
	for (int i=1 ; i < superblock.ninodes ; i++){
		inode_load(i, inode); // Carrega o inode
		if(inode->isvalid != 0){
			cout << "inode " << i << ":\n";
			cout << "    size: " << inode->size << " bytes\n";
			cout << "    direct blocks:";
			for(int j=0 ; j < POINTERS_PER_INODE ; j++) {
				if(inode->direct[j] != 0) {
					cout << " " << inode->direct[j];
				}
			}
			cout << "\n";
			if(inode->indirect != 0) {
				cout << "    indirect block: " << inode->indirect << "\n";
				disk->read(inode->indirect, block.data); // Lê o bloco indireto
				cout << "    indirect data blocks:";
				for(int j=0 ; j < POINTERS_PER_BLOCK ; j++) {
					if(block.pointers[j] != 0) {
						cout << " " << block.pointers[j];
					}
				}
				cout << "\n";
			}
		}
	}
}

int INE5412_FS::fs_mount()
{
	union fs_block block;

	disk->read(0, block.data);

	free_blocks.resize(block.super.nblocks, 0); // Inicializa o vetor de blocos livres

	// Disco não formatado, retorna 0
	if (block.super.magic != FS_MAGIC) {
		return 0; // Erro de montagem
	}

	// Iterar sobre os blocos de inodes
	for(int i=1 ; i < block.super.ninodeblocks + 1; i++) {
		disk->read(i, block.data); // Lê cada bloco de inode
		// Itera sobre os inodes do bloco
		for(int j=0 ; j < INODES_PER_BLOCK ; j++) {
			fs_inode inode = block.inode[j]; 
			// Para cada inode, se ele for válido, marca os bits referentes a seus blocos diretos no vetor de blocos livres
			if(inode.isvalid == 1) {
				for(int k=0 ; k < POINTERS_PER_INODE ; k++) {
					if(inode.direct[k] != 0) {
						free_blocks[inode.direct[k]] = 1; // Marca o bloco de dados como ocupado
					}
				}
				// Se houver bloco indireto, acessa o blocos indiretos e  marca os índices
				// dos blocos de dados para os quais ele aponta como 1 no vetor de blocos livres
				if(inode.indirect != 0) {
					disk->read(inode.indirect, block.data); // Lê o bloco indireto
					// Itera sobre os blocos de dados para os quais o bloco indireto aponta
					for(int k=0 ; k < POINTERS_PER_BLOCK ; k++) {
						if(block.pointers[k] != 0) {
							free_blocks[block.pointers[k]] = 1; // Marca o bloco de dados como ocupado
						}
					}
					disk->read(i, block.data); // Por fim, restaura o bloco de inodes para encontrar o próximo inode
				}
			}
		}
	}
	return 1; // Montagem bem sucedida
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

int INE5412_FS::inode_load(int inumber, fs_inode *inode)
{
	if (inumber < 1 || inumber > disk->size() * INODES_PER_BLOCK) {) {
		return 0;
	}
	inumber = inumber - 1; // Ajusta o inumber para a indexação começar em 0
	union fs_block block;
	int blocknum = 1 + inumber / INODES_PER_BLOCK; // Acha o bloco de inode ao qual o inode buscado pertence
	int inode_index = inumber % INODES_PER_BLOCK; // Acha o índice do inode no bloco de inode

	cout << "blocknum: " << blocknum << " inode_index: " << inode_index << endl;

	disk->read(blocknum, block.data); // Lê o bloco de inode

	*inode = block.inode[inode_index]; // Copia o endereço do inode para o ponteiro passado como argumento

	return 1;
}

int INE5412_FS::inode_save(int inumber, fs_inode *inode)
{
	if (inumber < 1 || inumber > disk->size() / 10 * INODES_PER_BLOCK) {
		return 0;
	}
	inumber = inumber - 1; // Ajusta o inumber para a indexação começar em 0
	union fs_block block; 
	int blocknum = 1 + inumber / INODES_PER_BLOCK; // Acha o bloco de inode ao qual o inode buscado pertence
	int inode_index = inumber % INODES_PER_BLOCK; // Acha o índice do inode no bloco de inode

	disk->read(blocknum, block.data); // Lê o bloco de inode

	block.inode[inode_index] = *inode; // Copia o inode para o bloco de inode

	disk->write(blocknum, block.data); // Escreve o bloco de inode no disco

	return 1;
}