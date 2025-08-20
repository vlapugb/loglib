## Структура
```
.
├─ include/logger/       # Заголовки библиотеки
├─ src/                  # Реализация библиотеки
├─ apps/
│  ├─ log_app.cpp        # Консольное приложение для записи логов
│  └─ stats_collector.cpp# Приложение сбора статистик из сокета
├─ tests/
│  └─ test_logger.cpp    # Простейшие юнит-тесты без фреймворков
└─ CMakeLists.txt
```

## Сборка
Требования: `cmake`, `gcc` (g++), Linux (Ubuntu/Debian).

```bash
mkdir build && cd build
cmake .. -DLOGGER_BUILD_SHARED=ON   # по умолчанию ON: собирает и static, и shared варианты
cmake --build . -j
ctest --output-on-failure           # запустить юнит-тесты
```

В результате будут собраны:
- `logger_static` (`liblogger_static.a`) – статическая библиотека
- `logger_shared` (`liblogger_shared.so`) – динамическая библиотека
- `log_app` – консольная утилита для записи логов
- `stats_collector` – сбор статистики из сокета

> По умолчанию приложения линкуются со статической библиотекой. Чтобы линковать `log_app` с `liblogger.so`, добавьте флаг `-DLOG_APP_LINK_SHARED=ON` при вызове `cmake`.

## Использование библиотеки

### Инициализация
```cpp
#include "logger/logger.hpp"
using namespace logger;

auto sink = make_file_sink("app.log"); // или make_socket_sink("127.0.0.1", 5555);
Logger log(std::move(sink), LogLevel::Info);
```

### Запись сообщений
```cpp
log.log(LogLevel::Warn, "about to start"); // фильтруется по default уровню
log.log("uses default level");
log.set_default_level(LogLevel::Error);
```

- В журнале сохраняются: `время (UTC, ISO 8601)`, `уровень`, `текст`.
- При ошибках записи `log.log(...)` возвращает `Status::IOError`. Сообщение можно получить через `log.last_error()`.

## Протокол для сокета
Простой текстовый поток (TCP), одна запись на строку:
```
<epoch_ms>|<LEVEL>|<message>\n
```
Пример: `1724054876881|INFO|System ready`

## Приложение `log_app`
Пишет в файл **или** в сокет. Передача данных в рабочий поток через потокобезопасную очередь.
```
Usage:
  log_app --file <log.txt> --level <info|warn|error> [--socket <host:port>]

Interactive input format:
  [LEVEL] message
Examples:
  INFO System started
  WARN Low disk space
  Hello without level (uses default)

Commands:
  /level <info|warn|error>   - сменить уровень по умолчанию
  /quit                      - выйти
```

Примеры:
```bash
# 1) Запись в файл
./log_app --file app.log --level info

# 2) Запись в сокет
./log_app --socket 127.0.0.1:5555 --level warn
```

## Приложение `stats_collector`
Слушает сокет, принимает данные от библиотеки, печатает каждое принятое сообщение и поддерживает статистики:

- количество сообщений **всего**
- по уровням важности
- за **последний час** (скользящее окно)
- длины сообщений: **min / max / avg**

Вывод статистики:
- **после приема N-го сообщения**
- **после таймаута T секунд**, *если* статистика менялась с момента последней выдачи

Запуск:
```bash
./stats_collector --listen 0.0.0.0:5555 --n 10 --t 5
```
