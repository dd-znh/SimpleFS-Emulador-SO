#include "fs.h"

int INE5412_FS::fs_format() {
    // Lê o bloco 0 para verificar se o disco já está formatado
    union fs_block block;
    disk->read(0, block.data);

    // Disco já formatado ou montado
    if (block.super.magic == FS_MAGIC) {
        return 0;
    }

    // Cria um novo superbloco
    union fs_block new_block_zero;
    new_block_zero.super.magic = FS_MAGIC;
    new_block_zero.super.nblocks = disk->size();
    new_block_zero.super.ninodeblocks = disk->size() / 10 + (disk->size() % 10 != 0);
    new_block_zero.super.ninodes = new_block_zero.super.ninodeblocks * INODES_PER_BLOCK;

    // Escreve o novo superbloco no disco
    disk->write(0, new_block_zero.data);
    fs_inode inode;

    // Reinicializa o vetor de blocos livres
    free_blocks.clear();
    free_blocks.resize(disk->size(), 0);
    free_blocks[0] = 1;

    // Apagar tabelas de inodes
    for (int i = 1; i <= new_block_zero.super.ninodes; i++) {
        inode_load(i, inode);
        inode.isvalid = 0;
        inode_save(i, inode);
    }

    return 1;
}

stringstream INE5412_FS::fs_debug() {
    // Lê o superbloco
    union fs_block block;
    disk->read(0, block.data);
    fs_superblock superblock = block.super;

    stringstream result;

    // Imprime informações do superbloco
    result << "superblock:\n";
    result << "    " << (superblock.magic == FS_MAGIC ? "magic number is valid\n" : "magic number is invalid!\n");
    result << "    " << superblock.nblocks << " blocks on disk\n";
    result << "    " << superblock.ninodeblocks << " blocks for inodes\n";
    result << "    " << superblock.ninodes << " inodes total\n";

    fs_inode inode;

    // Iterar sobre todos os inodes
    for (int i = 1; i <= superblock.ninodes; i++) {
        // Carrega o inode
        inode_load(i, inode);

        // Imprime informações do inode
        if (inode.isvalid != 0) {
            result << "inode " << i << ":\n";
            result << "    size: " << inode.size << " bytes\n";
            result << "    direct blocks:";

            // Imprime os ponteiros diretos
            for (int j = 0; j < POINTERS_PER_INODE; j++) {
                if (inode.direct[j] != 0) {
                    result << " " << inode.direct[j];
                }
            }

            result << "\n";

            // Imprime os ponteiros indiretos
            if (inode.indirect != 0) {
                result << "    indirect block: " << inode.indirect << "\n";
                disk->read(inode.indirect, block.data);
                result << "    indirect data blocks:";
                for (int j = 0; j < POINTERS_PER_BLOCK; j++) {
                    if (block.pointers[j] != 0) {
                        result << " " << block.pointers[j];
                    }
                }
                result << "\n";
            }
        }
    }

    cout << result.str();
    return result;
}

int INE5412_FS::fs_mount() {
    // Lê o superbloco
    union fs_block block;
    disk->read(0, block.data);

    // Disco não formatado
    if (block.super.magic != FS_MAGIC || block.super.nblocks != disk->size()) {
        return 0;
    }

    // Reinicializa o vetor de blocos livres e marca o bloco 0 como ocupado
    free_blocks.clear();
    free_blocks.resize(block.super.nblocks, 0);
    free_blocks[0] = 1;

    fs_inode inode;

    // Marca blocos ocupados com base nos inodes
    for (int i = 1; i <= block.super.ninodes; i++) {
        if (inode_load(i, inode) && inode.isvalid) {
            for (int j = 0; j < POINTERS_PER_INODE; j++) {
                if (inode.direct[j] != 0) {
                    free_blocks[inode.direct[j]] = 1;
                }
            }

            // Marca blocos indiretos como ocupados
            if (inode.indirect != 0) {
                union fs_block indirect_block;
                disk->read(inode.indirect, indirect_block.data);
                free_blocks[inode.indirect] = 1;
                for (int j = 0; j < POINTERS_PER_BLOCK; j++) {
                    if (indirect_block.pointers[j] != 0) {
                        free_blocks[indirect_block.pointers[j]] = 1;
                    }
                }
            }
        }
    }

    mounted = true;

    return 1;
}

int INE5412_FS::fs_create() {
    // Carrega o superbloco
    fs_inode inode;
    union fs_block block;
    disk->read(0, block.data);
    int ninodes = block.super.ninodes;

    // Procura por um inode livre
    for (int i = 1; i <= ninodes; i++) {
        if (inode_load(i, inode) && inode.isvalid == 0) {
            inode.isvalid = 1;
            inode.size = 0;
            std::fill(std::begin(inode.direct), std::end(inode.direct), 0);
            inode.indirect = 0;
            inode_save(i, inode);
            return i;
        }
    }

    return 0;
}

int INE5412_FS::fs_delete(int inumber) {
    fs_inode inode;
    if (!mounted) {
        cout << "Mount before disk operations\n";
        return 0;
    }

    // Carrega o inode e retorna 0 se o número do inode for inválido
    if (inode_load(inumber, inode) && inode.isvalid) {
        inode.isvalid = 0;
        inode.size = 0;

        // Libera blocos diretos
        for (int j = 0; j < POINTERS_PER_INODE; j++) {
            if (inode.direct[j] != 0) {
                free_blocks[inode.direct[j]] = 0;
                inode.direct[j] = 0;
            }
        }

        // Libera blocos indiretos
        if (inode.indirect != 0) {
            union fs_block block;
            disk->read(inode.indirect, block.data);
            for (int j = 0; j < POINTERS_PER_BLOCK && block.pointers[j] != 0; j++) {
                free_blocks[block.pointers[j]] = 0;
            }
            free_blocks[inode.indirect] = 0;
            inode.indirect = 0;
        }

        return inode_save(inumber, inode);
    }
    return 0;
}

int INE5412_FS::fs_getsize(int inumber) {
    // Carrega o inode e retorna -1 se o número do inode for inválido
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

    int read = 0;                                   // Contador para a quantidade de dados lidos
    int blocknum = offset / Disk::DISK_BLOCK_SIZE;  // Número do bloco inicial
    int blockoff = offset % Disk::DISK_BLOCK_SIZE;  // Offset dentro do bloco inicial
    union fs_block block;

    while (read < length) {
        // Leitura a partir dos ponteiros diretos
        if (blocknum < POINTERS_PER_INODE) {
            if (inode.direct[blocknum] == 0) {  // Nenhum bloco alocado
                break;
            }
            disk->read(inode.direct[blocknum], block.data);  // Lê o bloco direto
        }
        // Leitura a partir dos ponteiros indiretos
        else {
            if (inode.indirect == 0) {  // Nenhum bloco indireto alocado
                break;
            }
            disk->read(inode.indirect, block.data);                    // Lê o bloco indireto
            if (block.pointers[blocknum - POINTERS_PER_INODE] == 0) {  // Nenhum bloco de dados alocado
                break;
            }
            disk->read(block.pointers[blocknum - POINTERS_PER_INODE], block.data);  // Lê o bloco de dados
        }

        // Calcula quanto deve ser lido do bloco atual (ou o que falta ler, ou o que cabe no bloco)
        int toread = std::min(length - read, Disk::DISK_BLOCK_SIZE - blockoff);

        // Copia os dados lidos para o ponteiro data de saída
        copy(block.data + blockoff, block.data + blockoff + toread, data + read);

        // Atualiza os contadores
        read += toread;
        blockoff = 0;  // Somente o primeiro bloco pode ter offset
        blocknum++;
    }

    return read;  // Retorna o número de bytes lidos
}

int INE5412_FS::fs_write(int inumber, const char *data, int length, int offset) {
    fs_inode inode;

    if (!mounted) {
        cout << "Mount before disk operations\n";
        return 0;
    }

    if (inode_load(inumber, inode) == 0 || offset > inode.size) {
        return 0;
    }

    int written = 0;
    int blocknum = offset / Disk::DISK_BLOCK_SIZE;
    int blockoff = offset % Disk::DISK_BLOCK_SIZE;
    union fs_block block;
    union fs_block indirect_block;

    while (written < length) {
        int towrite = std::min(length - written, Disk::DISK_BLOCK_SIZE - blockoff);

        cout << POINTERS_PER_BLOCK << " " << POINTERS_PER_INODE << " " << blocknum << " " << blockoff << " " << towrite << "\n";

        if (blocknum < POINTERS_PER_INODE) {
            if (inode.direct[blocknum] == 0) {
                int free_block = find_free_block();
                if (free_block == -1) {
                    cout << "Error: No free blocks available\n";
                    return written;
                }
                inode.direct[blocknum] = free_block;
                free_blocks[free_block] = 1;
            }
            disk->read(inode.direct[blocknum], block.data);
        } else {
            int indirect_index = blocknum - POINTERS_PER_INODE;
            cout << POINTERS_PER_INODE << " " << blocknum << " " << indirect_index << "\n";
            if (indirect_index >= POINTERS_PER_BLOCK) {
                cout << "Error: File exceeds maximum size\n";
                break;
            }
            if (inode.indirect == 0) {
                cout << "Creating indirect pointer\n";
                int free_block = find_free_block();
                if (free_block == -1) {
                    cout << "Error: No free blocks available for indirect pointer\n";
                    return written;
                }
                inode.indirect = free_block;
                free_blocks[free_block] = 1;
                std::fill(std::begin(indirect_block.pointers), std::end(indirect_block.pointers), 0);
                disk->write(inode.indirect, indirect_block.data);
            }
            disk->read(inode.indirect, indirect_block.data);
            if (indirect_block.pointers[indirect_index] == 0) {
                int free_block = find_free_block();
                if (free_block == -1) {
                    cout << "Error: No free blocks available\n";
                    return written;
                }
                indirect_block.pointers[indirect_index] = free_block;
                free_blocks[free_block] = 1;
                disk->write(inode.indirect, indirect_block.data);
            }
            cout << "Reading indirect block\n";
            for (int i = 0; i < POINTERS_PER_BLOCK; i++) {
                cout << indirect_block.pointers[i] << " ";
            }
            cout << indirect_block.pointers[indirect_index] << "\n";
            disk->read(indirect_block.pointers[indirect_index], block.data);
        }

        std::copy(data + written, data + written + towrite, block.data + blockoff);

        if (blocknum < POINTERS_PER_INODE) {
            cout << "Writing block 1" << inode.direct[blocknum] << "\n";
            disk->write(inode.direct[blocknum], block.data);
        } else {
            cout << "Writing block 2" << block.pointers[blocknum - POINTERS_PER_INODE] << "\n";
            disk->write(indirect_block.pointers[blocknum - POINTERS_PER_INODE], block.data);
        }

        written += towrite;
        blockoff = 0;
        blocknum++;
    }

    if (offset + written > inode.size) {
        inode.size = offset + written;
    }

    inode_save(inumber, inode);
    return written;
}

int INE5412_FS::inode_load(int inumber, fs_inode &inode) {
    // Carrega o superbloco
    union fs_block block;
    disk->read(0, block.data);
    fs_superblock superblock = block.super;

    // Verifica se o número do inode é válido
    if (inumber < 1 || inumber > superblock.ninodes) {
        return 0;
    }

    // Ajusta o índice do inode para indexação baseada em 0
    inumber = inumber - 1;

    // Calcula o número do bloco e o índice dentro do bloco
    int blocknum = 1 + inumber / INODES_PER_BLOCK;
    int inode_index = inumber % INODES_PER_BLOCK;

    // Lê o bloco de inode correspondente
    disk->read(blocknum, block.data);

    // Copia o inode solicitado para a variável fornecida
    inode = block.inode[inode_index];

    return 1;  // Sucesso
}

int INE5412_FS::inode_save(int inumber, fs_inode &inode) {
    // Carrega o superbloco
    union fs_block block;
    disk->read(0, block.data);
    fs_superblock superblock = block.super;

    // Verifica se o número do inode é válido
    if (inumber < 1 || inumber > superblock.ninodes) {
        return 0;
    }

    // Ajusta o índice do inode para indexação baseada em 0
    inumber = inumber - 1;

    // Calcula o bloco e o índice do inode
    int blocknum = 1 + inumber / INODES_PER_BLOCK;
    int inode_index = inumber % INODES_PER_BLOCK;

    // Lê o bloco de inode correspondente
    disk->read(blocknum, block.data);

    // Atualiza o inode no bloco
    block.inode[inode_index] = inode;

    // Escreve o bloco atualizado de volta no disco
    disk->write(blocknum, block.data);

    return 1;  // Sucesso
}

int INE5412_FS::find_free_block() {
    union fs_block block;

    disk->read(0, block.data);

    // Salvar o superbloco para extrair informações dele
    int ninodeblocks = block.super.ninodeblocks;

    for (int i = ninodeblocks + 1; i < disk->size(); ++i) {  // Começa de 1 para ignorar o bloco 0 (geralmente reservado)
        if (free_blocks[i] == 0) {                           // Verifica se o bloco está livre
            free_blocks[i] = 1;                              // Marca o bloco como ocupado
            return i;                                        // Retorna o índice do bloco livre
        }
    }
    return -1;  // Sem blocos livres
}
