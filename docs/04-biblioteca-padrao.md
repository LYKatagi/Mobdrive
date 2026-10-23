# Biblioteca padrão

Os módulos Mobdrive ficam em `MB/stdlib/`; os runtimes nativos C correspondentes ficam na mesma pasta. Importe-os pela raiz da stdlib, sem escrever o caminho físico:

```mobdrive
enter(/math.mbd) {
    const path "math.mbdb";
};
```

Veja como configurar `MOBDRIVE_STDLIB` em [Instalação e compilação](01-instalacao-e-compilacao.md#diretorio-da-biblioteca-padrao). O campo `path` é metadado e não representa um binário gerado atualmente.

## IO

Módulo: `/io.mbd`; runtime: `MB/stdlib/runtime.c`.

| Função | Assinatura | Descrição |
| --- | --- | --- |
| `print_i64` | `(i64 value) -> void` | Imprime um inteiro de 64 bits com sinal. |
| `print_f64` | `(double value) -> void` | Imprime um número de ponto flutuante. |
| `print_string` | `(string value) -> void` | Imprime uma string ou `(null)`. |
| `print_bool` | `(bool value) -> void` | Imprime `true` ou `false`. |
| `string_length` | `(string value) -> i64` | Retorna o comprimento em bytes; para `null`, retorna zero. |

O IO usa `stdio` do C e, portanto, depende do runtime C do alvo.

## Math

Módulo: `/math.mbd`. Implementado em Mobdrive, sem runtime C adicional.

| Função | Assinatura | Descrição |
| --- | --- | --- |
| `abs_i64` | `(i64 value) -> i64` | Valor absoluto. |
| `min_i64` | `(i64 left, i64 right) -> i64` | Menor dos dois valores. |
| `max_i64` | `(i64 left, i64 right) -> i64` | Maior dos dois valores. |

## GC conservador

Módulo: `/gc.mbd`; runtime: `MB/stdlib/gc_runtime.c`.

| Função | Assinatura | Descrição |
| --- | --- | --- |
| `gc_alloc` | `(i64 size) -> void*` | Aloca um bloco zerado gerenciado. Retorna `null` para tamanho não positivo ou falha. |
| `gc_root` | `(void* pointer_slot) -> void` | Registra o endereço de uma variável ponteiro que contém uma raiz. |
| `gc_unroot` | `(void* pointer_slot) -> void` | Remove a raiz previamente registrada. |
| `gc_collect` | `() -> void` | Executa a coleta mark-and-sweep. |

Exemplo:

```mobdrive
enter(/gc.mbd) {
    const path "gc.mbdb";
};

function main() -> int32 {
    var data: int32* = gc_alloc(16);
    gc_root(&data);
    *data = 42;

    print(*data);
    gc_collect();
    gc_unroot(&data);
    return 0;
}
```

A coleta automática é acionada quando o heap ultrapassa seu limite atual. O coletor examina slots de raiz registrados e valores alinhados ao tamanho de ponteiro dentro dos blocos marcados. Ele não varre a stack C. Registre todas as referências que precisam sobreviver à coleta e remova a raiz antes de a variável sair de escopo. O runtime não é thread-safe e a busca do heap é linear.

## Tempo

Módulo: `/time.mbd`; runtime: `MB/stdlib/time_runtime.c`.

| Função | Assinatura | Descrição |
| --- | --- | --- |
| `time_now_ms` | `() -> i64` | Milissegundos de relógio monotônico. Use para medir intervalos, não como data civil. |
| `time_sleep_ms` | `(i64 milliseconds) -> void` | Suspende a thread por pelo menos o intervalo solicitado; valores não positivos não fazem nada. |
| `time_elapsed_ms` | `(i64 start_ms) -> i64` | Diferença entre o relógio atual e o valor inicial. |

Windows usa `GetTickCount64` e `Sleep`; Linux/POSIX usa `CLOCK_MONOTONIC` e `nanosleep`.

## UI: janelas e desenho

Módulo: `/ui.mbd`; runtime: `MB/stdlib/ui_runtime.c`.

| Função | Assinatura | Descrição |
| --- | --- | --- |
| `ui_window_open` | `(string title, int32 width, int32 height) -> bool` | Abre uma janela nativa. Dimensões não positivas usam 800×600. |
| `ui_event_loop` | `() -> void` | Processa eventos até o fechamento da janela. |
| `ui_window_close` | `() -> void` | Solicita destruição da janela ativa. |
| `draw_rect` | `(int32 x, int32 y, int32 width, int32 height, uint32 color) -> void` | Enfileira retângulo preenchido. |
| `draw_text` | `(int32 x, int32 y, string text) -> void` | Enfileira texto preto. |
| `draw_text_color` | `(int32 x, int32 y, string text, uint32 color) -> void` | Enfileira texto colorido. |
| `ui_draw_rect`, `ui_draw_text` | Alias compatível | Chamam as funções de desenho correspondentes. |

Cores usam `0xRRGGBB`. Os desenhos são mantidos em uma fila e reapresentados após eventos de exposição da janela. A API ainda não oferece teclado, mouse, imagens, fontes configuráveis, GPU ou agendamento de UI thread.

Exemplo de janela automática:

```mobdrive
enter(/ui.mbd) {
    const path "ui.mbdb";
};

graphic class MainScreen {
    public var title: string;

    public visual run() -> void {
        title = "Minha janela";
        draw_rect(20, 20, 200, 100, 0x3366CC);
        draw_text_color(30, 40, "Olá, Mobdrive", 0xFFFFFF);
    }
}

function main() -> int32 {
    var screen: MainScreen = new MainScreen();
    screen.run();
    return 0;
}
```

O exemplo precisa linkar `ui_runtime.c` e a biblioteca de plataforma. O Makefile já faz isso. Para compilação manual, no Linux acrescente `--link-options MB/stdlib/ui_runtime.c --link-options -lX11`; no Windows, acrescente `--link-options MB/stdlib/ui_runtime.c --link-options -luser32 --link-options -lgdi32`.
