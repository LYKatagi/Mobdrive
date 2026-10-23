# Instalação e compilação

## Requisitos

O compilador Mobdrive é escrito em C e usa um compilador C hospedeiro para se compilar. São necessários:

- GCC ou Clang com suporte a C11;
- GNU Make;
- ambiente de shell compatível com as receitas do Makefile.

Para compilar os exemplos gráficos no Linux, instale também o pacote de desenvolvimento X11. Em Debian/Ubuntu, por exemplo:

```sh
sudo apt install build-essential make libx11-dev
```

No Windows, o projeto foi configurado para MinGW-w64/MSYS2. As bibliotecas de UI Win32/GDI fazem parte do toolchain MinGW.

## Compilar o compilador e os exemplos

Execute na raiz do repositório:

```sh
# Linux
make

# Windows / MSYS2
mingw32-make
```

O alvo `all` compila `mobdrive`, `mobdrive-elf` e os exemplos. Artefatos ficam em `build/`. A extensão executável é escolhida pelo sistema: `.exe` no Windows e nenhuma extensão no Linux.

Outros alvos:

```sh
make clean       # remove os artefatos da pasta build
make kernel-elf  # gera build/kernel_minimal.elf
```

No Windows, substitua `make` por `mingw32-make`. Se o cross-compiler ELF tiver outro nome ou não estiver no PATH, informe-o:

```sh
mingw32-make kernel-elf ELF_CC=x86_64-elf-gcc
```

## Verificar e compilar fontes

`check` carrega os imports e executa as verificações sem gerar um programa:

```sh
build/mobdrive check examples/hello.mbd
```

Os comandos disponíveis são:

```text
mobdrive check <fonte.mbd>
mobdrive build <fonte.mbd> [-o programa] [--cc gcc] [--link-options item ...]
mobdrive build-elf <fonte.mbd> [-o kernel.elf] [--cc compilador-elf] [--link-options item ...]
mobdrive-elf build <fonte.mbd> [-o kernel.elf] [--cc compilador-elf]
```

Sem `-o`, o modo nativo usa `a.exe` no Windows e `a.out` em POSIX. O modo ELF usa `kernel.elf`. `mobdrive-elf` e `build-elf` selecionam o mesmo backend freestanding.

`build` gera o C intermediário e chama o compilador nativo:

```sh
build/mobdrive build examples/hello.mbd -o build/hello
./build/hello
```

No Windows:

```powershell
build\mobdrive.exe check examples\hello.mbd
build\mobdrive.exe build examples\hello.mbd -o build\hello.exe
build\hello.exe
```

O nome de saída nativo deve terminar em `.exe` no Windows. No Linux, pode ser um nome de executável comum; nomes terminados em `.elf` são reservados ao comando freestanding.

Para selecionar outro compilador C, use `--cc`:

```sh
build/mobdrive build examples/hello.mbd -o build/hello --cc clang
```

O compilador C é chamado com C11, otimização `-O3` e LTO (`-flto`). No modo aplicativo também recebe `-march=native`; no modo ELF, flags freestanding específicas.

## Linkar código externo

Passe arquivos objeto, fontes C ou bibliotecas por `--link-options`. A opção pode ser repetida:

```sh
build/mobdrive build examples/ffi.mbd -o build/ffi \
  --link-options examples/ffi.c
```

Exemplo de biblioteca:

```sh
build/mobdrive build app.mbd -o build/app \
  --link-options native.o \
  --link-options -lm
```

O driver encaminha esses argumentos ao compilador C. Arquivos `.c` fornecidos dessa forma são compilados na mesma chamada do programa gerado.

## Diretório da biblioteca padrão

Imports como `enter(/math.mbd)` procuram o módulo no diretório apontado por `MOBDRIVE_STDLIB`. Se a variável não estiver definida ou não apontar para um módulo correspondente, o compilador tenta `MB/stdlib` e `stdlib` a partir do diretório de trabalho atual.

Linux/macOS shell:

```sh
export MOBDRIVE_STDLIB="$PWD/MB/stdlib"
```

PowerShell:

```powershell
$env:MOBDRIVE_STDLIB = "$PWD\MB\stdlib"
```

## Windows e Linux

O compilador e runtimes base usam APIs portáveis. O runtime de tempo usa `GetTickCount64`/`Sleep` no Windows e relógio monotônico/`nanosleep` em sistemas POSIX. O runtime gráfico usa Win32/GDI no Windows e X11 no Linux.

O backend Linux da UI precisa de um servidor X acessível pela variável `DISPLAY`. Em sessões Wayland, a disponibilidade depende do XWayland. Se não for possível abrir uma tela X, `ui_window_open` retorna falso.

## Saída ELF freestanding

Linux GCC normalmente produz ELF diretamente. Gere um arquivo freestanding com:

```sh
build/mobdrive-elf build examples/kernel_minimal.mbd \
  -o build/kernel_minimal.elf --cc gcc
```

O Makefile fornece o atalho `make kernel-elf`. No Windows, use um compilador cross-targeting ELF, como `x86_64-elf-gcc`.

O executável gerado é um ELF x86-64 sem libc, com símbolo `_start`. Ele ainda não possui cabeçalho Multiboot/Limine nem código de inicialização de plataforma; portanto, não é automaticamente inicializável por um bootloader.
