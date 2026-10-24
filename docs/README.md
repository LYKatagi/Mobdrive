# Documentação Mobdrive

Esta documentação descreve o comportamento do compilador presente neste repositório. Como Mobdrive ainda é um protótipo, os guias distinguem recursos implementados de ideias que ainda não fazem parte do compilador.

## Guias

1. [Instalação e compilação](01-instalacao-e-compilacao.md) — dependências, Makefile, comandos da CLI, Windows, Linux e saída ELF.
2. [Referência da linguagem](02-referencia-da-linguagem.md) — sintaxe suportada, tipos, expressões, funções, classes, interfaces, namespaces e contextos seguros.
3. [Módulos, interoperabilidade nativa e ELF](03-modulos-ffi-e-freestanding.md) — `enter`, `bin`, linkagem, `__c__`, `__asm__` e compilação freestanding.
4. [Biblioteca padrão](04-biblioteca-padrao.md) — módulos IO, matemática, GC, tempo e UI, com APIs e exemplos.
5. [Arquitetura do compilador](05-arquitetura-do-compilador.md) — responsabilidades do lexer ao toolchain e organização dos arquivos.
6. [Estado, limitações e planos](06-status-e-limitacoes.md) — implementação atual e limites conhecidos.

## Convenções

- Os exemplos usam nomes e caminhos relativos à raiz do repositório.
- `build/mobdrive` representa o executável do compilador no Linux. No Windows, use `build/mobdrive.exe`.
- Blocos marcados como “conceito” não significam que a funcionalidade correspondente esteja completamente implementada.

## Exemplo integrado

[language_showcase.mbd](../examples/language_showcase.mbd) combina sintaxe da linguagem, stdlib, ponteiros, funções nativas e interface gráfica. A execução abre uma janela do sistema.


## Guia do exemplo integrado

[Explicação do language_showcase.mbd](07-explicacao-language-showcase.md) descreve o showcase seção por seção, com o fluxo de execução e as limitações relevantes.
