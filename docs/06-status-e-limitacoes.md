# Estado e limitações

Mobdrive é um protótipo funcional para experimentação com sintaxe, análise estática inicial, interoperabilidade C e geração de executáveis. Esta página registra limites que afetam programas reais.

## Implementado no compilador atual

- Lexer de comentários, identificadores, números, strings, operadores e pontuação.
- Parser de imports, funções, variáveis, `if`, `else`, `while`, namespaces, classes, herança simples, interfaces, `safe`/`unsafe` e `nochk`.
- Verificações iniciais de tipos e declarações semânticas.
- Geração de C11 e invocação de GCC/Clang para executáveis nativos.
- Saída freestanding ELF x86-64 com `_start` gerado.
- Imports de módulos da stdlib usando `/nome.mbd` e runtimes iniciais de IO, matemática, GC, tempo e UI.
- Janelas Win32/GDI no Windows e X11 no Linux.

## Não implementado ou parcial

- **Bytecode `.mbdb`:** a extensão aparece como metadado em `enter`, mas nenhum bytecode é gerado ou executado.
- **Arquivo objeto isolado:** o comando atual gera executável; não há opção pública para parar em `.o`.
- **Loops `for`:** a gramática implementa `while`, não `for`.
- **Sistema de tipos completo:** não há inferência geral, generics, overload, tipos algébricos, arrays tipados ou verificação exaustiva de fluxo.
- **Ponteiros seguros:** `safe` não fornece verificação de limites, validade ou ownership; `unsafe` apenas abre regras pontuais.
- **Classes completas:** campos não aceitam inicializadores, `new` só aceita zero argumentos, não existem construtores/destrutores ou dispatch virtual. Modificadores de visibilidade são aceitos, sem enforcement.
- **Interfaces dinâmicas:** interfaces são verificadas em compilação; não produzem tabela de dispatch/runtime.
- **`nochk`:** nomes são validados, mas `none`, `minchk` e `strict` ainda não mudam as regras do typechecker.
- **Modos `bin`:** `link`, `nolink` e `decide` são analisados. A política ainda não escolhe estratégias diferentes de linkagem; `nolink` apenas exige pelo menos uma opção `--link-options` global.
- **GC:** não varre stack ou registradores, não é thread-safe e depende de roots explícitas. Falsos positivos conservadores podem manter objetos vivos.
- **UI:** somente retângulos e texto enfileirados; não há input, GPU, widgets, imagens, fontes configuráveis ou coordenação de threads. Linux depende de X11/XWayland.
- **Kernel:** ELF não tem protocolo de boot, linker script, inicialização de hardware, drivers ou suporte de runtime de kernel. O exemplo de VGA/`hlt` depende de ambiente x86 e não deve ser executado como aplicativo comum.
- **Diagnósticos:** mensagens e recuperação do parser são básicas; números de coluna e notas contextuais não estão disponíveis em todas as etapas.

## Alvos e dependências

O compilador hospedeiro é C11 e usa GCC/Clang. `-march=native` favorece desempenho no host, mas pode produzir binários que não rodam em CPUs diferentes. No Linux, o runtime UI liga com X11 (`-lX11`); no Windows, com Win32/GDI (`-luser32 -lgdi32`).

O modo freestanding é x86-64 e usa flags e assembly no estilo GCC. Para um kernel real, é necessário complementar a saída Mobdrive com as peças específicas do bootloader e da plataforma.

## Compatibilidade

Não há promessa de compatibilidade de linguagem ou ABI enquanto o projeto estiver nesta fase. Antes de atualizar, revise a referência e recompile módulos nativos externos com o mesmo alvo e convenções de chamada usados pelo compilador C selecionado.
