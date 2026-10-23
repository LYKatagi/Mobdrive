# Arquitetura do compilador

O compilador é dividido em etapas pequenas e implementado em C. O fluxo atual é:

```text
arquivos .mbd
    ↓
carregador de projeto e imports
    ↓
lexer → tokens
    ↓
parser → estruturas de programa
    ↓
typechecker + analisador semântico
    ↓
gerador de C
    ↓
GCC/Clang → executável nativo ou ELF
```

## Mapa de arquivos

| Pasta/arquivo | Responsabilidade |
| --- | --- |
| `MB/src/cli/main.c` | Argumentos `check`, `build` e `build-elf`. |
| `MB/src/frontend/lexer.c` | Divide o texto em identificadores, números, strings, símbolos e EOF; registra linhas e erros léxicos. |
| `MB/src/frontend/parser.c` | Analisa declarações e instruções e constrói a representação `Program`. |
| `MB/src/project/project.c` | Lê arquivos, resolve imports recursivos e monta unidades de projeto. |
| `MB/src/sema/typecheck.c` | Verifica expressões, símbolos locais, chamadas, tipos, atribuições, retornos e contexto unsafe. |
| `MB/src/sema/semantic.c` | Verifica declarações repetidas, símbolos `nochk`, dependências `bin`, herança e contratos de interface. |
| `MB/src/codegen/codegen.c` | Emite C11, transforma métodos/classes e gera o ponto de entrada freestanding quando solicitado. |
| `MB/src/driver/toolchain.c` | Executa GCC/Clang e verifica o cabeçalho ELF no modo freestanding. |
| `MB/src/compiler/compiler.c` | Orquestra leitura, semântica, codegen e toolchain. |
| `MB/include/mobdrive/` | Headers públicos entre componentes C do compilador. |
| `MB/stdlib/` | Módulos `.mbd` e implementações nativas C. |

## Representação interna

O lexer mantém uma lista de tokens com tipo, texto e linha. O parser constrói listas encadeadas de funções, classes, interfaces, imports, regras de verificação e statements. Expressões são guardadas como texto tokenizado novamente pelo typechecker e pelo codegen; não há uma AST completa de expressões.

O carregador percorre imports antes das análises. Unidades importadas são combinadas na mesma compilação; não há módulos binários independentes nem cache incremental.

## Verificação

O typechecker mantém uma tabela simples de símbolos locais e valida tipos escalares, chamadas e aridade, retornos, atribuições, comparações, condições e operações básicas de ponteiro. O analisador semântico faz verificações em todo o projeto, como duplicatas, base de classe existente e assinatura de método exigida por interface.

As duas etapas são protótipos: não implementam ainda todas as regras de um sistema de tipos, fluxos de controle ou segurança de memória.

## Geração de código e toolchain

O codegen emite um arquivo intermediário `<saída>.mobdrive.c`. Se a compilação tiver sucesso, esse arquivo é removido; se falhar, é mantido para inspeção. O driver chama o compilador C com `posix_spawnp` em POSIX ou `_spawnvp` no Windows.

No modo normal, o C gerado usa libc para `print`. No modo freestanding, o codegen declara rotinas de saída externas, emite `_start` e espera suporte de inicialização fornecido pelo alvo. A biblioteca padrão não é automaticamente vinculada: o usuário seleciona os runtimes via `--link-options`.

## Extensão

Para adicionar uma palavra-chave, atualize o lexer/parser e, quando pertinente, as análises semânticas e o codegen. Recursos de linguagem devem ser documentados em `docs/02-referencia-da-linguagem.md` e limitações em `docs/06-status-e-limitacoes.md`. APIs nativas precisam de declaração `.mbd`, implementação C, configuração de link em Makefile e documentação na referência da stdlib.
