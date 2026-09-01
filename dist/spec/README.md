# Комплект прошивок Betaflight-BOOST-SPEC

Имя прошивки в Betaflight Configurator: **Betaflight-BOOST-SPEC** (версия 2026.6.1).

Регламент пилота: `PILOT_SPEC.md` (настоящий каталог или корень репозитория).  
Описание режима Boost: `BOOST_MODE.md`.  
Протокол ленты UART: `LEDSTRIP_UART.md`.  
Пример прошивки Arduino Nano: `arduino/ledstrip_uart.ino`.

---

## 1. Именование файлов

**Ядро МК:**

```
betaflight_2026.6.1_<MCU>.hex
```

Файл выбирается по типу микроконтроллера (`version`). Назначения выводов пустые — после прошивки загружается **`dump all`**.

Ядро STM32F722 включает GPS, MAVLink и CRSF.

---

## 2. Повторная сборка ядер

В среде WSL из корня репозитория:

```
bash scripts/build_spec_mcu.sh
```

| Файл | Назначение |
|---|---|
| `dist/spec_mcu_build.log` | Журнал сборки |
| `dist/spec_ok.txt` | Успешно собранные ядра |
| `dist/spec_failed.txt` | Ядра, не поместившиеся во flash или не собравшиеся |

Опции ядра STM32F722: `scripts/spec_options_f722.txt`.
