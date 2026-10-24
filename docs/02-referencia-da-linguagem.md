# Referência da linguagem

Este guia descreve a sintaxe que o parser e as etapas semânticas aceitam atualmente. A linguagem é case-sensitive. Blocos usam `{}` e declarações terminam em `;`.

## Comentários e literais

```mobdrive
// Comentário de linha
/* Comentário
   de bloco */

"texto"
'x'
42
3.14
true
false
null
```

Strings e caracteres reconhecem escapes durante a geração de C. O tratamento de escapes Unicode e o formato de caracteres multibyte não são especificados pelo protótipo.

## Tipos

Tipos primitivos aceitos:

| Categoria | Nomes aceitos | Tipo C gerado |
| --- | --- | --- |
| Sem retorno | `void` | `void` |
| Inteiro | `int`, `int8`, `int16`, `int32`, `int64` | tipos inteiros correspondentes |
| Inteiro sem sinal | `uint8`, `uint16`, `uint32`, `uint64` | tipos sem sinal correspondentes |
| Aliases | `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64` | tipos inteiros correspondentes |
| Ponto flutuante | `float`, `double` | `float`, `double` |
| Outros | `bool`, `char`, `string` | `_Bool`/`bool`, `char`, `const char *` |

Tipos definidos por classe também podem ser usados em declarações. Ponteiros são explícitos e usam `*`:

```mobdrive
var address: int32*;
var other: int32 *;
```

Os dois formatos são aceitos. Não há inferência de tipo para parâmetros ou campos.

## Variáveis e constantes

Declarações locais e globais podem usar `var`, `let` ou `const`. `let` é um alias de `var` no parser.

```mobdrive
const VERSION: string = "0.1";
var count: int32 = 0;
var greeting = "Olá"; // inferência para var
```

`const` exige tipo explícito e valor inicial. `var` sem tipo explícito infere o tipo da expressão inicial. Campos de classe exigem um tipo. Inicializadores de campos ainda não são suportados.

## Funções

```mobdrive
function add(left: int32, right: int32) -> int32 {
    return left + right;
}

function log_message(string message) -> void {
    print(message);
}
```

Parâmetros aceitam as formas `nome: tipo` e `tipo nome`. A função de entrada esperada pelo linker nativo é `main`; exemplos retornam `int` ou `int32`.

`return;` encerra uma função `void` sem valor. Em C, o codegen emite `return;`. Em funções com retorno não vazio, o typechecker rejeita `return;`. O typechecker valida o tipo do valor retornado quando existe um `return`, mas o protótipo não analisa de forma completa todos os caminhos de retorno.

## Expressões e operadores

Expressões suportam chamadas de função, literais, identificadores, parênteses, `new Tipo()`, acesso a campos e métodos, operadores aritméticos, comparações, operadores lógicos e atribuições.

Operadores reconhecidos pelo typechecker:

| Precedência (baixa para alta) | Operadores |
| --- | --- |
| Atribuição | `=`, `+=`, `-=` |
| OR lógico | `||` |
| AND lógico | `&&` |
| Comparação | `==`, `!=`, `<`, `>`, `<=`, `>=` |
| Adição | `+`, `-` |
| Multiplicação | `*`, `/`, `%` |

Unários reconhecidos: `&`, `*`, `!`, `~`, `+`, `-`, `++`, `--`. Incremento e decremento também podem aparecer após uma expressão numérica, como `index++`. O typechecker permite conversões entre tipos numéricos. A compatibilidade de ponteiros é limitada; consulte as restrições em [Estado e limitações](06-status-e-limitacoes.md).

## Controle de fluxo

```mobdrive
if (count > 0) {
    print(count);
} else {
    print("vazio");
}

while (count < 10) {
    count = count + 1;
}

for (var index: int32 = 0; index < 10; index++) {
    print(index);
}
```

`for` aceita uma declaração (`var`, `let`, `const` ou tipo primitivo explícito antes do nome), uma expressão de inicialização, ou uma cláusula vazia. A condição e a expressão de atualização também podem ser omitidas; por exemplo, `for (;;) { ... }`. A variável declarada no cabeçalho fica restrita ao loop. O corpo deve ser um bloco.

`break;` encerra o loop mais interno e `continue;` pula para a próxima iteração. Ambos funcionam em `for` e `while`; o analisador semântico rejeita seu uso fora de loops. `do/while` e `switch` ainda não fazem parte da gramática atual.

## Saída simples

`print(expressão);` é uma instrução da linguagem. O codegen escolhe uma rotina C para strings, números, ponto flutuante e valores inteiros/bool. Para uma API de biblioteca padrão explícita, importe `/io.mbd` e use `print_string`, `print_i64`, `print_f64` ou `print_bool`.

## Namespaces

Namespaces qualificam funções, classes, interfaces e globais. Use `::` para referenciar um símbolo qualificado:

```mobdrive
namespace math {
    function twice(value: int32) -> int32 {
        return value * 2;
    }
}

function main() -> int32 {
    return math::twice(21);
}
```

Namespaces aninhados são combinados, por exemplo `outer::inner::symbol`. Símbolos não qualificados podem ser resolvidos no namespace atual. A resolução de namespaces ainda é simples e não possui um sistema completo de visibilidade ou aliases.

## Classes, herança e métodos

```mobdrive
class Device {
    public var name: string;

    public function start() -> void {
        print(name);
    }
}

class Phone extends Device {
    function identify() -> void {
        print(name);
    }
}

function main() -> int32 {
    var phone: Phone = new Phone();
    phone.name = "Mobdrive";
    phone.start();
    return 0;
}
```

Classes são representadas como `struct` C. Métodos viram funções C com parâmetro `self` implícito. Campos herdados são armazenados no membro `base` da struct base. `new Type()` cria um valor zerado; construtores, argumentos para `new`, inicializadores de campo, despacho virtual e destrutores não estão implementados. Modificadores `public`, `private` e `protected` são aceitos sintaticamente, mas não impõem controle de acesso.

## Interfaces

`interface` e seu alias `socket` declaram assinaturas. Uma classe lista implementações com `implements` ou `plug`:

```mobdrive
interface Runnable {
    function run() -> void;
}

class App plug Runnable {
    function run() -> void {
        print("rodando");
    }
}
```

O analisador semântico exige métodos correspondentes com mesma assinatura. Interfaces verificam contratos em compilação; não há dispatch dinâmico de interface.

## Contextos `safe` e `unsafe`

```mobdrive
safe {
    var value: int32 = 10;
    print(value);
}

unsafe {
    var pointer: int32* = 0xB8000;
    *pointer = 0x2F4D;
}
```

`unsafe` autoriza inicialização/atribuição de ponteiro a partir de inteiro e as intrínsecas `__asm__` e `__c__`. O contexto não desliga a análise geral de nomes e tipos. `safe` delimita um bloco normal e não adiciona uma verificação de memória em runtime.

Blocos `safe`/`unsafe` no nível do módulo são executados pela inicialização gerada quando `main` começa. Blocos dentro de funções são executados no ponto em que aparecem.

## Verificação `nochk`

```mobdrive
nochk {
    render,
    system::read_port
}::strict;
```

Os níveis aceitos são `none`, `minchk` e `strict`. O analisador verifica se os nomes se referem a funções conhecidas. Atualmente, a escolha de nível não altera as verificações: o typechecker aplica o mesmo conjunto de regras.

## Classes gráficas

```mobdrive
graphic class Screen {
    public var title: string;

    public visual run() -> void {
        title = "Janela Mobdrive";
        draw_text(20, 30, "Olá");
    }
}
```

Um método `visual` sem `function` é aceito pelo parser. Quando pertence a uma `graphic class`, tem retorno `void` e chama `ui_window_open` com sucesso, o codegen inicia uma janela padrão de 800×600 e entra no loop de eventos. Se a classe tiver um campo `title`, seu valor é usado como título; caso contrário, usa-se o nome da classe. Veja a [referência da UI](04-biblioteca-padrao.md#ui-janelas-e-desenho).
