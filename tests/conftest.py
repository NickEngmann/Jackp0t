import sys
from unittest.mock import MagicMock

# Mock all hardware dependencies BEFORE any import
sys.modules['driver/gpio'] = MagicMock()
sys.modules['driver/spi'] = MagicMock()
sys.modules['driver/i2c'] = MagicMock()
sys.modules['driver/rf'] = MagicMock()
sys.modules['freertos/FreeRTOS'] = MagicMock()
sys.modules['freertos/task'] = MagicMock()
sys.modules['freertos/queue'] = MagicMock()
sys.modules['nvs_flash'] = MagicMock()
sys.modules['esp_log'] = MagicMock()
sys.modules['cJSON'] = MagicMock()

# Mock cJSON functions
cjson_mock = sys.modules['cJSON']
cjson_mock.cJSON_Parse = lambda s: {'valid': True, 'json': s} if s else None
cjson_mock.cJSON_GetObjectItem = lambda obj, key: {'string': key} if obj and key in ['challenge_id', 'target_uid', 'emulate'] else None
cjson_mock.cJSON_IsString = lambda item: True
cjson_mock.cJSON_GetStringValue = lambda item: 'DEADBEEF' if item and item.get('string') == 'target_uid' else '1'
cjson_mock.cJSON_Delete = lambda obj: None
cjson_mock.cJSON_CreateObject = lambda: {}
cjson_mock.cJSON_AddNumberToObject = lambda obj, key, val: None
cjson_mock.cJSON_AddStringToObject = lambda obj, key, val: None
cjson_mock.cJSON_Print = lambda obj: '{"challenge_id":1,"target_uid":"DEADBEEF","emulate":true}'