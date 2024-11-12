# SimpleFS - Emulador de Sistema de Arquivos

Este projeto implementa um sistema de arquivos emulado chamado **SimpleFS**, baseado na estrutura de inodes dos sistemas Unix. O projeto foi desenvolvido para a disciplina de **Sistemas Operacionais I** na Universidade Federal de Santa Catarina.

## Objetivo

O projeto tem como propósito:

- Aprender sobre as estruturas de dados e implementação de sistemas de arquivos.
- Implementar um sistema de arquivos similar ao Unix, incluindo operações de leitura e escrita com um emulador de disco.
- Desenvolver experiência em C++ com foco em orientação a objetos e gerenciamento de memória.
- Implementar um regime rigoroso de testes para garantir o funcionamento e integridade do sistema.

## Estrutura do Projeto

O projeto é composto pelos seguintes componentes:

1. **Shell de Comando**: Interpreta os comandos do usuário para formatar, montar e manipular arquivos.
2. **Sistema de Arquivos (SimpleFS)**: Implementação em C++ responsável pela lógica de armazenamento e recuperação de dados.
3. **Disco Emulado**: Armazena dados em blocos de 4KB, permitindo simulação de um disco físico.

## Funcionalidades Implementadas

- `fs_format()`: Formata o disco, criando o superbloco e alocando blocos de inodes.
- `fs_mount()`: Monta o sistema de arquivos, validando o superbloco e construindo o bitmap de blocos livres.
- `fs_create()`: Cria um novo arquivo, alocando um inode vazio.
- `fs_delete()`: Exclui um arquivo e libera seus blocos.
- `fs_read() / fs_write()`: Permite leitura e escrita de dados em arquivos.
- `fs_debug()`: Varre e reporta as estruturas de dados do sistema de arquivos, como o superbloco e inodes.

## Requisitos

- **Linguagem**: C++
- **Biblioteca Gráfica**: SFML (para interface gráfica)

### Instalação da Biblioteca SFML no Ubuntu

```bash
sudo apt-get install libsfml-dev
```

## Executando o Projeto

1. Compile o código com o comando:

    ```bash
    make
    ```

2. Execute o SimpleFS com uma imagem de disco:

    ```bash
    ./simplefs mydisk 25
    ```

3. Use os comandos do shell para manipular o sistema de arquivos. A lista de comandos está disponível digitando `help` no shell.

## Testes

Foram incluídos três exemplos de imagens de disco (image.5, image.20 e image.200) para teste. Utilize o comando `debug` no shell para verificar a organização interna das estruturas de dados do sistema de arquivos.

---
