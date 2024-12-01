#include "fs.h"

int INE5412_FS::fs_format() {
    union fs_block block;

    disk->read(0, block.data);

    // Disco já formatado ou montado, retorna 0
    if (block.super.magic == FS_MAGIC) {
        return 0;  // Erro de formatação
    }

    union fs_block new_block_zero;  // Cria um bloco de dados vazio

    new_block_zero.super.magic = FS_MAGIC;                                                // Setta o magic number no superbloco
    new_block_zero.super.nblocks = disk->size();                                          // Número total de blocos no disco
    new_block_zero.super.ninodeblocks = disk->size() / 10 + (disk->size() % 10 != 0);     // 10% dos blocos, arredondando pra cima
    new_block_zero.super.ninodes = new_block_zero.super.ninodeblocks * INODES_PER_BLOCK;  // Número total de inodes

    int ninodes = new_block_zero.super.ninodes;  // Salva o número de blocos de inodes

    disk->write(0, new_block_zero.data);  // Escreve o novo bloco zero no disco

	fs_inode inode;

    // Apagar tabelas de inodes
	for (int i = 1; i <= ninodes; i++) {
        inode_load(i, inode);  // Carrega o inode
		inode.isvalid = 0;     // Invalida o inode
		inode_save(i, inode);  // Salva o inode
	}
    return 1;  // Formatação bem sucedida
}

void INE5412_FS::fs_debug() {
    union fs_block block;

    disk->read(0, block.data);

    // Salvar o superbloco para extrair informações dele
    fs_superblock superblock = block.super;

    cout << "superblock:\n";
    cout << "    " << (superblock.magic == FS_MAGIC ? "magic number is valid\n" : "magic number is invalid!\n");
    cout << "    " << superblock.nblocks << " blocks on disk\n";
    cout << "    " << superblock.ninodeblocks << " blocks for inodes\n";
    cout << "    " << superblock.ninodes << " inodes total\n";

    fs_inode inode;

    // Iterar sobre todos os inodes
    for (int i = 1; i <= superblock.ninodes; i++) {
        inode_load(i, inode);  // Carrega o inode
        if (inode.isvalid != 0) {
            cout << "inode " << i << ":\n";
            cout << "    size: " << inode.size << " bytes\n";
            cout << "    direct blocks:";
            for (int j = 0; j < POINTERS_PER_INODE; j++) {
                if (inode.direct[j] != 0) {
                    cout << " " << inode.direct[j];
                }
            }
            cout << "\n";
            if (inode.indirect != 0) {
                cout << "    indirect block: " << inode.indirect << "\n";
                disk->read(inode.indirect, block.data);  // Lê o bloco indireto
                cout << "    indirect data blocks:";
                for (int j = 0; j < POINTERS_PER_BLOCK; j++) {
                    if (block.pointers[j] != 0) {
                        cout << " " << block.pointers[j];
                    }
                }
                cout << "\n";
            }
        }
    }
}

int INE5412_FS::fs_mount() {
    union fs_block block;

    disk->read(0, block.data);

    free_blocks.resize(block.super.nblocks, 0);  // Inicializa o vetor de blocos livres

    // Disco não formatado, retorna 0
    if (block.super.magic != FS_MAGIC) {
        return 0;  // Erro de montagem
    }

	fs_inode inode;
	// Iterar sobre todos os inodes
	for (int i = 1; i <= block.super.ninodes; i++) {
		inode_load(i, inode);  // Carrega o inode
		 // Para cada inode, se ele for válido, marca os bits referentes a seus blocos diretos no vetor de blocos livres
		if (inode.isvalid == 1) {
			for (int j = 0; j < POINTERS_PER_INODE; j++) {
				if (inode.direct[j] != 0) {
					free_blocks[inode.direct[j]] = 1;  // Marca o bloco de dados direto como ocupado
				}
			}
			// Se houver bloco indireto, acessa os blocos indiretos e  marca os índices
			// dos blocos de dados para os quais ele aponta como 1 no vetor de blocos livres
			if (inode.indirect != 0) {
				disk->read(inode.indirect, block.data);  // Lê o bloco indireto
				// Itera sobre os blocos de dados para os quais o bloco indireto aponta
				for (int j = 0; j < POINTERS_PER_BLOCK; j++) {
					if (block.pointers[j] != 0) {
						free_blocks[block.pointers[j]] = 1;  // Marca o bloco de dados indireto como ocupado
					}
				}
			}
		}
	}
    return 1;  // Montagem bem sucedida
}

int INE5412_FS::fs_create() {
	// Iterar sobre todos os inodes, até encontrar um inode inválido
	fs_inode inode;
	for (int i = 1 ; i < (disk->size() / 10 + (disk->size() % 10 != 0)) ; i++) {
		inode_load(i, inode);  // Carrega o inode
		if (inode.isvalid == 0) {
			inode.isvalid = 1;  // Marca o inode como válido
			inode.size = 0;      // Zera o tamanho do arquivo
			for (int j = 0; j < POINTERS_PER_INODE; j++) {
				inode.direct[j] = 0;  // Zera os ponteiros diretos
			}
			inode.indirect = 0;  // Zera o ponteiro indireto
			inode_save(i, inode);  // Salva o inode
			return i;  // Retorna o número do inode criado
		}
	}
	return 0;
}

int INE5412_FS::fs_delete(int inumber) { 
	fs_inode inode;
	if (inode_load(inumber, inode) == 1) {
		inode.isvalid = 0;  // Invalida o inode
		inode.size = 0;      // Zera o tamanho do arquivo
		for (int j = 0; j < POINTERS_PER_INODE; j++) {
			if (inode.direct[j] != 0) {
				free_blocks[inode.direct[j]] = 0;  // Marca o bloco de dados direto como livre
				inode.direct[j] = 0;  // Zera os ponteiros diretos
			}
		}
		if (inode.indirect != 0) {
			union fs_block block;
			disk->read(inode.indirect, block.data);  // Lê o bloco indireto
			for (int j = 0; j < POINTERS_PER_BLOCK; j++) {
				if (block.pointers[j] != 0) {
					free_blocks[block.pointers[j]] = 0;  // Marca o bloco de dados indireto como livre
				}
			}
			free_blocks[inode.indirect] = 0;  // Marca o bloco indireto como livre
			inode.indirect = 0;  // Zera o ponteiro indireto
		}
        if (inode_save(inumber, inode) == 1) {
            return 1;
        }
	}
    return 0;
}

int INE5412_FS::fs_getsize(int inumber) {
	fs_inode inode;
	if (inode_load(inumber, inode) == 1) {
		return inode.size;
	}
    return -1;
}

int INE5412_FS::fs_read(int inumber, char *data, int length, int offset) {
    fs_inode inode;
    // Carrega o inode e retorna 0 se o número do inode for inválido
    if (inode_load(inumber, inode) == 0) {
        cout << "inode_load failed!\n";
        return 0;
    }
    // Retorna 0 se o offset for maior ou igual ao tamanho do arquivo
    if (offset >= inode.size) {
        cout << "offset >= inode.size\n";
        return 0;
    }

    int read = 0; // Contador para a quantidade de dados lidos
    int blocknum = offset / Disk::DISK_BLOCK_SIZE; // Número do bloco inicial
    int blockoff = offset % Disk::DISK_BLOCK_SIZE; // Offset dentro do bloco inicial
    union fs_block block;

    while (read < length) {
        // Leitura a partir dos ponteiros diretos
        if (blocknum < POINTERS_PER_INODE) {
            if (inode.direct[blocknum] == 0) { // Nenhum bloco alocado
                break;
            }
            disk->read(inode.direct[blocknum], block.data); // Lê o bloco direto
        }
        // Leitura a partir dos ponteiros indiretos
        else {
            if (inode.indirect == 0) { // Nenhum bloco indireto alocado
                break;
            }
            disk->read(inode.indirect, block.data); // Lê o bloco indireto
            if (block.pointers[blocknum - POINTERS_PER_INODE] == 0) { // Nenhum bloco de dados alocado
                break;
            }
            disk->read(block.pointers[blocknum - POINTERS_PER_INODE], block.data); // Lê o bloco de dados
        }

        // Calcula quanto deve ser lido do bloco atual (ou o que falta ler, ou o que cabe no bloco)
        int toread = std::min(length - read, Disk::DISK_BLOCK_SIZE - blockoff);

        // Copia os dados lidos para o ponteiro data de saída
        copy(block.data + blockoff, block.data + blockoff + toread, data + read);

        // Atualiza os contadores
        read += toread;
        blockoff = 0; // Somente o primeiro bloco pode ter offset
        blocknum++;
    }

    return read; // Retorna o número de bytes lidos
}

int INE5412_FS::fs_write(int inumber, const char *data, int length, int offset) {
    fs_inode inode;
    // Carrega o inode, retorna 0 se inválido
    if (inode_load(inumber, inode) == 0) {
        return 0;
    }
    // Retorna 0 se o offset for maior que o tamanho do arquivo
    if (offset > inode.size) {
        return 0;
    }
    // Ajusta o tamanho da escrita
    if (offset + length > inode.size) {
        length = inode.size - offset;
    }

    int written = 0;
    int blocknum = offset / Disk::DISK_BLOCK_SIZE;
    int blockoff = offset % Disk::DISK_BLOCK_SIZE;
    union fs_block block;

    while (written < length) {
        int towrite = std::min(length - written, Disk::DISK_BLOCK_SIZE - blockoff);

        if (blocknum < POINTERS_PER_INODE) {
            if (inode.direct[blocknum] == 0) {
                int free_block = find_free_block();
                if (free_block == -1) return written; // Sem blocos livres
                inode.direct[blocknum] = free_block;
            }
            disk->read(inode.direct[blocknum], block.data);
        } else {
            if (inode.indirect == 0) {
                int free_block = find_free_block();
                if (free_block == -1) return written; // Sem blocos livres
                inode.indirect = free_block;
            }
            disk->read(inode.indirect, block.data);
            if (block.pointers[blocknum - POINTERS_PER_INODE] == 0) {
                int free_block = find_free_block();
                if (free_block == -1) return written; // Sem blocos livres
                block.pointers[blocknum - POINTERS_PER_INODE] = free_block;
                disk->write(inode.indirect, block.data);
            }
            disk->read(block.pointers[blocknum - POINTERS_PER_INODE], block.data);
        }

        std::copy(data + written, data + written + towrite, block.data + blockoff);
        if (blocknum < POINTERS_PER_INODE) {
            disk->write(inode.direct[blocknum], block.data);
        } else {
            disk->write(block.pointers[blocknum - POINTERS_PER_INODE], block.data);
        }

        written += towrite;
        blockoff = 0;
        blocknum++;
    }

    inode.size = std::max(inode.size, offset + written);
    inode_save(inumber, inode);
    return written;
}


int INE5412_FS::inode_load(int inumber, fs_inode &inode) {
    if (inumber < 1 || inumber > disk->size() * INODES_PER_BLOCK) {
        return 0;
    }
    inumber = inumber - 1;  // Ajusta o inumber para a indexação começar em 0
    union fs_block block;
    int blocknum = 1 + inumber / INODES_PER_BLOCK;  // Acha o bloco de inode ao qual o inode buscado pertence
    int inode_index = inumber % INODES_PER_BLOCK;   // Acha o índice do inode no bloco de inode

    disk->read(blocknum, block.data);  // Lê o bloco de inode

    inode = block.inode[inode_index];  // Copia o endereço do inode para o ponteiro passado como argumento

    return 1;
}

int INE5412_FS::inode_save(int inumber, fs_inode &inode) {
    if (inumber < 1 || inumber > disk->size() / 10 * INODES_PER_BLOCK) {
        return 0;
    }
    inumber = inumber - 1;  // Ajusta o inumber para a indexação começar em 0
    union fs_block block;
    int blocknum = 1 + inumber / INODES_PER_BLOCK;  // Acha o bloco de inode ao qual o inode buscado pertence
    int inode_index = inumber % INODES_PER_BLOCK;   // Acha o índice do inode no bloco de inode

    disk->read(blocknum, block.data);  // Lê o bloco de inode

    block.inode[inode_index] = inode;  // Copia o inode para o bloco de inode

    disk->write(blocknum, block.data);  // Escreve o bloco de inode no disco

    return 1;
}

int INE5412_FS::find_free_block() {
    for (int i = 1; i < disk->size(); ++i) { // Começa de 1 para ignorar o bloco 0 (geralmente reservado)
        if (free_blocks[i] == 0) { // Verifica se o bloco está livre
            free_blocks[i] = 1; // Marca o bloco como ocupado
            return i; // Retorna o índice do bloco livre
        }
    }
    return -1; // Sem blocos livres
}
