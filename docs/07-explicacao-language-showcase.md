# Leitura guiada do `language_showcase.mbd`

Este documento explica o programa [`examples/language_showcase.mbd`](../examples/language_showcase.mbd) na ordem em que ele está escrito. O showcase reúne recursos que o compilador Mobdrive implementa hoje e importa os módulos necessários para IO, matemática, coleta de lixo, relógio e interface gráfica.

> O projeto ainda é experimental. Algumas anotações são reconhecidas pela sintaxe, mas não mudam o comportamento da compilação. As diferenças entre sintaxe planejada e comportamento atual estão indicadas abaixo.

## Compilar e executar

Na raiz do repositório, verifique o projeto e compile o exemplo:

```sh
# Linux
make build/language_showcase
build/mobdrive check examples/language_showcase.mbd

# Windows com MSYS2
mingw32-make build/language_showcase.exe
build/mobdrive.exe check examples/language_showcase.mbd
```

O alvo também está incluído em `all`. Para executar no Linux:

```sh
./build/language_showcase
```

No Windows:

```powershell
.\build\language_showcase.exe
```

O programa imprime valores no terminal e depois abre uma janela nativa. Fechar a janela encerra o loop gráfico e permite que `main` termine. No Linux, é necessário ter X11 disponível e instalar os headers e a biblioteca de desenvolvimento X11 para compilar o runtime.

O Makefile linka os runtimes C de IO, GC, tempo e UI. Os imports `/...` localizam os módulos Mobdrive; as implementações nativas são passadas separadamente com `--link-options`.

## 1. Cabeçalho e imports

O comentário inicial explica que o showcase cobre recursos da implementação presente e que usa runtimes diferentes por plataforma.

```mobdrive
enter(/io.mbd) { const path "io.mbdb"; };
enter(/math.mbd) { const path "math.mbdb"; };
enter(/gc.mbd) { const path "gc.mbdb"; };
enter(/time.mbd) { const path "time.mbdb"; };
enter(/ui.mbd) { const path "ui.mbdb"; };
```

`enter` carrega outro arquivo `.mbd` para o mesmo projeto. O prefixo `/` significa “procurar a partir da raiz da stdlib Mobdrive”, então não é preciso escrever `MB/stdlib/`. O compilador procura em `MOBDRIVE_STDLIB`, `MB/stdlib` e `stdlib`, relativamente ao diretório de trabalho.

O bloco `const path` é metadado do artefato compilado associado. Embora o exemplo use nomes `.mbdb`, o compilador atual não gera bytecode `.mbdb`; os fontes importados são combinados e compilados juntos.

Cada import habilita uma API usada adiante:

- `io.mbd`: impressão tipada, como `print_string`, `print_i64` e `print_f64`.
- `math.mbd`: operações inteiras como `abs_i64`.
- `gc.mbd`: funções externas do coletor (`gc_alloc`, `gc_root` etc.).
- `time.mbd`: relógio monotônico e espera em milissegundos.
- `ui.mbd`: janela, desenho e loop de eventos.

## 2. Globais, funções externas e `nochk`

```mobdrive
const VERSION: string = "0.3.0";
var execution_count: int32 = 0;
var default_message = "Mobdrive suporta inferência local.";
```

`VERSION` é uma constante global com tipo explícito. `execution_count` é mutável. `default_message` mostra inferência de tipo: o compilador deduz `string` a partir do valor inicial.

As declarações seguintes ilustram os três modos de linkagem de uma função `bin`:

```mobdrive
bin function native_link_example()::link() -> void;
bin function native_nolink_example()::nolink() -> void;
bin function native_decide_example()::decide() -> int32;
```

`bin` declara uma função cuja implementação deve vir de código nativo. Neste programa, elas não são chamadas, portanto não precisam de implementações externas para o link final. O modo `nolink` exige pelo menos uma opção `--link-options` em qualquer build, e o Makefile já passa os runtimes necessários. A associação entre uma opção específica e cada função ainda não é verificada pelo compilador.

As três regras `nochk` cobrem os níveis aceitos pela sintaxe:

```mobdrive
nochk { add, native_link_example }::strict;
nochk { native_nolink_example }::minchk;
nochk { native_decide_example }::none;
```

O analisador confirma que os nomes existem. Atualmente, `strict`, `minchk` e `none` não alteram a intensidade nem o conjunto de verificações do typechecker.

## 3. Namespace e funções

```mobdrive
namespace demo {
    const BASE_VALUE: int32 = 40;

    function add_to_base(value: int32) -> int32 {
        return BASE_VALUE + value;
    }
}
```

O namespace qualifica `BASE_VALUE` e `add_to_base`. Fora dele, a função é chamada como `demo::add_to_base(2)`. O compilador converte o separador `::` em um nome C seguro durante a geração de código.

`add` demonstra parâmetros no formato `nome: tipo` e retorno explícito. `return_legacy_parameter` mostra a forma alternativa `tipo nome`, também aceita pelo parser:

```mobdrive
function add(left: int32, right: int32) -> int32 { ... }
function return_legacy_parameter(int32 value) -> int32 { ... }
```

`show_message` chama uma função importada do módulo de IO. `count_up` combina `while`, `if/else`, comparação, chamada de função e reatribuição. `sum_with_for` soma os inteiros anteriores a `limit` usando o loop `for`:

```mobdrive
for (var index: int32 = 0; index < limit; index++) {
    total += index;
}
```

O cabeçalho tem três partes: inicialização, condição e atualização, separadas por `;`. `index` só existe dentro do loop. `index++` incrementa o contador ao fim de cada iteração; `total += index` acumula o valor atual. Em `main`, `sum_with_for(6)` imprime `15`.

`return_first_even` mostra `continue` no `for`: ignora zero e números ímpares até retornar o primeiro par positivo. `sum_until(4)` demonstra `break` no `for` e imprime `6`; `count_up` mostra `continue` e `break` em um `while`.

## 4. Interfaces e classes

```mobdrive
interface Runnable { function run() -> void; }
socket Disposable { function close() -> void; }
```

`interface` e `socket` declaram contratos de método; `socket` é um alias sintático. O analisador semântico confere se uma classe que implementa a interface fornece método com a mesma assinatura.

`Device` introduz um campo e um método. `ConsoleDevice extends Device plug Runnable, Disposable` demonstra herança simples e implementação de múltiplas interfaces. O método `run` lê `name`, campo herdado da classe base. `new Device()` e `new ConsoleDevice()` criam valores zerados; não há construtores definidos pelo usuário.

No showcase, `base_device` é do tipo `Device`, recebe um nome e chama `describe`. `console_device` é instanciado separadamente e chama seus métodos próprios. Isso evita depender de dispatch virtual, que não está implementado.

Modificadores como `public` são aceitos, mas ainda não impõem regras de visibilidade.

## 5. Classe gráfica

`ShowcaseWindow` tem o modificador `graphic` e implementa `Runnable`. Seu método `public visual run()` enfileira comandos para desenhar um retângulo e duas linhas de texto:

```mobdrive
draw_rect(30, 50, 420, 180, 0x245A9C);
draw_text_color(50, 80, "...", 0xFFFFFF);
draw_text(50, 115, "...");
```

As cores estão no formato `0xRRGGBB`. Como a classe tem um campo `title`, esse valor é usado como título da janela. Ao retornar do método visual, o codegen abre a janela padrão de 800×600 e executa o loop de eventos até o usuário fechá-la.

No Windows, o backend usa Win32/GDI. No Linux, usa X11. A classe não tem acesso direto à GPU nem a eventos de teclado ou mouse; a UI atual implementa apenas janela e desenho básico.

## 6. Ponteiros e código de baixo nível

`pointer_example` cria uma variável local e obtém seu endereço:

```mobdrive
var value_pointer: int32* = &value;
*value_pointer = *value_pointer + 1;
```

`int32*` é um ponteiro explícito. `&value` obtém o endereço e `*value_pointer` lê ou escreve através dele. O bloco `unsafe` destaca que operações com endereços exigem cuidado. A função retorna `42` sem deixar escapar o ponteiro para a variável local.

`low_level_example` demonstra as duas intrínsecas:

```mobdrive
__c__("volatile int mobdrive_embedded_c_example = 1;");
__asm__("nop");
```

`__c__` insere o conteúdo da string diretamente no C gerado. `__asm__` emite assembly inline no formato aceito pelo compilador C. Ambas precisam estar em `unsafe`. A função não é chamada por `main`, por isso o exemplo não executa operações de baixo nível durante a demonstração normal. Para instruções privilegiadas como `hlt`, use somente um alvo/kernel apropriado.

## 7. O fluxo de `main`

### Bloco `safe`

O primeiro bloco reúne operações comuns: imprime mensagens, testa inferência e os dois estilos de parâmetro, chama a função do namespace e a função matemática, mostra `bool`, `double` e `char`, incrementa uma global e chama os exemplos de ponteiro e loop. Em seguida, `print_i64(sum_with_for(6))` imprime `15`, resultado de `0 + 1 + 2 + 3 + 4 + 5`. `finish_message` mostra um `return;` explícito dentro de uma função `void`; o codegen conserva essa saída como `return;` no C gerado.

`safe` delimita o contexto normal de código. Não é um verificador de memória em runtime e não implica isolamento de processo.

### Alocação gerenciada

```mobdrive
var managed_value: int32* = gc_alloc(16);
gc_root(&managed_value);
*managed_value = 123;
print_i64(*managed_value);
gc_collect();
gc_unroot(&managed_value);
```

`gc_alloc` reserva um bloco gerenciado. `gc_root` registra o endereço da variável local que contém o ponteiro; sem registrar a raiz, o coletor poderia considerar o objeto inacessível. O bloco é usado e uma coleta é solicitada enquanto a raiz ainda está registrada. `gc_unroot` remove o slot da lista de raízes antes de `main` terminar.

O GC é conservador e não varre a stack C. Todas as referências que precisem sobreviver à coleta devem estar registradas corretamente.

### Relógio

`time_now_ms` captura o relógio monotônico. `time_sleep_ms(5)` espera aproximadamente cinco milissegundos. `time_elapsed_ms(start_time)` calcula e imprime o intervalo decorrido. Esse relógio mede duração e não representa data ou hora civil.

### Objetos e janela

O programa instancia `Device` e `ConsoleDevice`, define um campo e chama métodos. Por fim, cria `ShowcaseWindow` e chama `window.run()`. Esse método desenha e abre a janela. O loop gráfico bloqueia até a janela ser fechada; depois disso, `main` retorna `0`.

## 8. Recursos mostrados e limites

| Recurso | Onde aparece | Observação |
| --- | --- | --- |
| Comentários de linha e bloco | Arquivo inteiro | Ambos são reconhecidos pelo lexer. |
| Imports e metadado `path` | Início | Fontes são unidas; `.mbdb` não é emitido. |
| `const`, `var`, `let`, inferência | Globais e `main` | `let` equivale a variável mutável no parser atual. |
| Tipos e retornos | Funções e `main` | Demonstra inteiros, string, bool, double, char e ponteiro. |
| `bin` e modos de link | Declarações externas | Declaradas, mas não invocadas. |
| `nochk` | Regras no topo | Níveis reconhecidos, ainda sem efeito distinto. |
| Namespace | `demo` | Chamada qualificada com `::`. |
| `if`, `else`, `while`, `for`, `break`, `continue` | `count_up`, `sum_with_for`, `return_first_even`, `sum_until` | `break` e `continue` só são válidos dentro de loops. |
| Interface e `socket` | `Runnable`, `Disposable` | Verificação estática, sem dispatch dinâmico. |
| Classe, herança e métodos | `Device`, `ConsoleDevice` | Herança única e `new` sem argumentos. |
| Classe gráfica e desenho | `ShowcaseWindow` | Abre janela nativa e aguarda seu fechamento. |
| `safe` e `unsafe` | `main`, funções de baixo nível | `unsafe` habilita operações pontuais; não desliga toda a análise. |
| GC e ponteiros | `pointer_example`, `main` | GC exige roots explícitas. |
| `__c__`, `__asm__` | `low_level_example` | Intrínsecas não chamadas no fluxo normal. |

Para detalhes normativos do que o compilador realmente aceita, consulte a [referência da linguagem](02-referencia-da-linguagem.md), a documentação da [stdlib](04-biblioteca-padrao.md) e a página de [limitações](06-status-e-limitacoes.md).