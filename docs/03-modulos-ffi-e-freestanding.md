# Módulos, FFI e modo freestanding

## Importar fontes com `enter`

`enter` carrega outro arquivo `.mbd` no mesmo projeto. Imports são recursivos e as fontes são compiladas juntas em um único executável.

```mobdrive
enter(mode.mbd) {
    const path "mode.mbdb";
};
```

O texto entre parênteses identifica o arquivo fonte. O bloco `const path` é opcional no parser e guarda metadado do artefato associado. O compilador atual não gera nem consome bytecode `.mbdb`; o valor `path` não altera a compilação.

### Importar da biblioteca padrão

Um caminho que começa com `/` é tratado como caminho relativo à raiz da stdlib:

```mobdrive
enter(/math.mbd) {
    const path "math.mbdb";
};
```

O compilador procura nesta ordem: `MOBDRIVE_STDLIB`, `MB/stdlib` e `stdlib`, em relação ao diretório de trabalho atual. Se o caminho absoluto indicado existir literalmente, ele é usado como arquivo absoluto. Imports comuns sem `/` são resolvidos em relação à pasta do arquivo que os declara.

Para configurar uma instalação fora da árvore do projeto, defina `MOBDRIVE_STDLIB` para o diretório que contém os `.mbd` da biblioteca.

## Funções externas com `bin`

`bin function` declara uma função que deve ser fornecida por uma unidade nativa:

```mobdrive
bin function native_hello()::link() -> void;
bin function custom_log(string text)::nolink() -> void;
bin function platform_start()::decide() -> int32;
```

Uma implementação C, por exemplo:

```c
#include <stdio.h>

void native_hello(void) {
    puts("Olá do C");
}
```

Compile e passe a implementação na etapa de build:

```sh
build/mobdrive build app.mbd -o build/app --link-options native.c
```

### Estratégias de linkagem declaradas

| Anotação | Intenção da sintaxe | Comportamento atual |
| --- | --- | --- |
| `::link()` | Resolver pela linkagem normal | Gera uma declaração externa C; o linker precisa resolver o símbolo. |
| `::nolink()` | Fornecer a implementação pelas opções de link | O analisador exige que exista pelo menos uma opção `--link-options` no comando. A associação específica entre opção e função não é verificada. |
| `::decide()` | Deixar a estratégia depender do alvo | Aceita a anotação, mas ainda não escolhe uma estratégia diferente. |

As anotações não adicionam implementação automaticamente. O nome e a ABI da função C precisam corresponder à declaração Mobdrive. Os nomes Mobdrive qualificados por namespace são emitidos como identificadores C com `__` no lugar de `::`.

## Intrínsecas de baixo nível

As intrínsecas abaixo só são aceitas dentro de `unsafe` e recebem exatamente uma string literal.

### Embutir C com `__c__`

```mobdrive
unsafe {
    __c__("volatile unsigned short *vga = (unsigned short *)0xB8000; vga[0] = 0x2F4D;");
}
```

O conteúdo da string é inserido diretamente no C gerado. Isso contorna parte da representação e análise de tipos Mobdrive; o código precisa ser C válido, usar os headers necessários e respeitar a ABI do alvo.

### Assembly com `__asm__`

```mobdrive
unsafe {
    __asm__("hlt");
}
```

O codegen mantém a chamada como assembly inline do compilador C (dialeto estilo GCC). Instruções, constraints e disponibilidade dependem do compilador e arquitetura escolhidos.

## Compilação freestanding (`mobdrive-elf`)

O modo freestanding pode ser chamado como:

```sh
build/mobdrive-elf build examples/kernel_minimal.mbd \
  -o build/kernel_minimal.elf --cc x86_64-elf-gcc
```

Ou usando `build-elf` com o compilador normal:

```sh
build/mobdrive build-elf examples/kernel_minimal.mbd \
  -o build/kernel_minimal.elf --cc x86_64-elf-gcc
```

O Makefile também oferece `make kernel-elf`. O codegen gera um `_start` que chama `main` e, ao retornar, entra em um loop `hlt`. O toolchain usa opções como `-ffreestanding`, `-nostdlib`, `-fno-pie` e `-march=x86-64`; o resultado é verificado pelo magic ELF.

Este modo é uma base para experimentação de baixo nível, não um kit de boot completo. É preciso fornecer linker script, protocolo/cabeçalho de boot, inicialização, drivers e suporte de runtime adequados à plataforma. A stdlib com libc, como IO, tempo ou UI, não é automaticamente compatível com um kernel freestanding.
