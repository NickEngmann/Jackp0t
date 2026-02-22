import sys
from unittest.mock import MagicMock

# Mock all hardware dependencies BEFORE any import
sys.modules['hal_gpio'] = MagicMock()
sys.modules['hal_rf'] = MagicMock()
sys.modules['hal_uart'] = MagicMock()
sys.modules['hal_timer'] = MagicMock()
sys.modules['hal_eeprom'] = MagicMock()

# Mock common C headers (used in C code but not needed for Python tests)
sys.modules['avr/io.h'] = MagicMock()
sys.modules['avr/interrupt.h'] = MagicMock()
sys.modules['util/delay.h'] = MagicMock()
sys.modules['string.h'] = MagicMock()
sys.modules['stdlib.h'] = MagicMock()
sys.modules['stdio.h'] = MagicMock()
sys.modules['stdbool.h'] = MagicMock()
sys.modules['stdint.h'] = MagicMock()
sys.modules['stdint'] = MagicMock()
sys.modules['stdbool'] = MagicMock()
sys.modules['string'] = MagicMock()
sys.modules['stdlib'] = MagicMock()
sys.modules['stdio'] = MagicMock()

# Provide mock functions for GPIO/RF that tests can override
hal_gpio_mock = sys.modules['hal_gpio']
hal_rf_mock = sys.modules['hal_rf']
hal_uart_mock = sys.modules['hal_uart']

hal_gpio_mock.gpio_init = MagicMock()
hal_gpio_mock.gpio_set = MagicMock()
hal_gpio_mock.gpio_get = MagicMock(return_value=0)
hal_gpio_mock.gpio_toggle = MagicMock()

hal_rf_mock.rf_init = MagicMock()
hal_rf_mock.rf_send_packet = MagicMock(return_value=True)
hal_rf_mock.rf_receive_packet = MagicMock(return_value=None)
hal_rf_mock.rf_set_frequency = MagicMock()
hal_rf_mock.rf_enable_interrupt = MagicMock()

hal_uart_mock.uart_init = MagicMock()
hal_uart_mock.uart_write = MagicMock()
hal_uart_mock.uart_read = MagicMock(return_value=b'')