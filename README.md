# Mobdrive

Mobdrive é uma linguagem compilada experimental, com sintaxe inspirada em C, C++ e Java. O projeto inclui um compilador escrito em C, uma biblioteca padrão inicial e exemplos de compilação nativa para Windows e Linux.

O compilador transforma arquivos `.mbd` em C intermediário e usa GCC ou Clang para gerar o executável de destino. O modo freestanding gera um ELF x86-64 com ponto de entrada `_start`, voltado a experimentos de baixo nível.

> **Estado do projeto:** protótipo em evolução. A sintaxe e algumas APIs ainda podem mudar. Consulte [o que está implementado](docs/06-status-e-limitacoes.md) antes de depender de uma funcionalidade.

## Começar

### Requisitos

- GCC ou Clang compatível com C11.
- GNU Make.
- Windows: MSYS2/MinGW-w64. Linux: toolchain nativa da distribuição.
- UI no Linux: headers e biblioteca X11, por exemplo `libx11-dev` em distribuições Debian/Ubuntu.
- UI no Windows: bibliotecas Win32/GDI fornecidas pelo toolchain MinGW.

### Compilar o projeto

Na raiz do repositório:

```sh
# Linux
make

# Windows com MSYS2
mingw32-make
```

O comando cria o compilador, o compilador freestanding e os executáveis de exemplo em `build/`. No Windows, os binários recebem `.exe`; no Linux, usam nomes sem sufixo.

### Compilar e executar um programa

```sh
build/mobdrive check examples/hello.mbd
build/mobdrive build examples/hello.mbd -o build/hello
./build/hello
```

No Windows, use `build/mobdrive.exe`, forneça uma saída `.exe` e execute `build/hello.exe`.

Para uma compilação com uma implementação nativa externa:

```sh
build/mobdrive build examples/ffi.mbd -o build/ffi \
  --link-options examples/ffi.c
```

O argumento `--cc clang` seleciona Clang. Repita `--link-options` para passar mais objetos ou bibliotecas ao compilador nativo.

## Exemplo Mobdrive

```mobdrive
function add(left: int32, right: int32) -> int32 {
    return left + right;
}

function main() -> int32 {
    const greeting: string = "Olá, Mobdrive!";
    print(greeting);
    print(add(20, 22));
    return 0;
}
```

O programa [language_showcase.mbd](examples/language_showcase.mbd) reúne os recursos disponíveis no compilador e abre uma janela nativa. O Makefile também o compila junto dos outros exemplos.

## Documentação

- [Índice da documentação](docs/README.md)
- [Instalação e compilação](docs/01-instalacao-e-compilacao.md)
- [Referência da linguagem](docs/02-referencia-da-linguagem.md)
- [Módulos, interoperabilidade nativa e ELF](docs/03-modulos-ffi-e-freestanding.md)
- [Biblioteca padrão](docs/04-biblioteca-padrao.md)
- [Arquitetura do compilador](docs/05-arquitetura-do-compilador.md)
- [Estado e limitações](docs/06-status-e-limitacoes.md)
- [Explicação do exemplo completo](docs/07-explicacao-language-showcase.md)

## Estrutura do repositório

```text
MB/
├── include/mobdrive/   Interfaces C dos componentes
├── src/
│   ├── cli/             Interface de linha de comando
│   ├── frontend/        Lexer e parser
│   ├── project/         Carregamento de fontes e imports
│   ├── sema/            Análise semântica e verificação de tipos
│   ├── codegen/         Geração de C
│   ├── driver/          Invocação do compilador C nativo
│   └── compiler/        Orquestração das etapas
└── stdlib/              Módulos Mobdrive e runtimes C
examples/                Programas de demonstração
docs/                    Guias e referência
build/                   Artefatos gerados pelo Makefile
Makefile                 Build do compilador e dos exemplos
```

## Licença

Consulte o arquivo de licença do projeto, se fornecido pela distribuição que você recebeu. 
Arquivo: [LICENSA](LICENSE)
