import sys
import tempfile
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from env_config import parse_env, firmware_values, render_header, generate

class EnvConfigTest(unittest.TestCase):
    def test_literal_values(self):
        data = parse_env('export WIFI_SSID="Моя мережа" # comment\nWIFI_PASSWORD=\'pa$$#`x`$(id)\\word\'\nOTA_PASSWORD=\n')
        self.assertEqual(data['WIFI_SSID'], 'Моя мережа')
        self.assertEqual(data['WIFI_PASSWORD'], 'pa$$#`x`$(id)\\word')
        self.assertEqual(data['OTA_PASSWORD'], '')
        self.assertNotIn('OTA_PASSWORD', render_header(data))
        self.assertNotIn('$(id)', render_header(data))
    def test_invalid_no_secret_in_errors(self):
        for text in ['WIFI_PASSWORD="private', 'WIFI_PASSWORD=private value', 'bad-key=private']:
            with self.assertRaises(ValueError) as context:
                parse_env(text)
            self.assertNotIn('private', str(context.exception))
        for data in [{'WIFI_SSID': 'x' * 33}, {'UKRAINEALARM_API_KEY': 'a b'}, {'WIFI_PASSWORD': 'x\x00y'}, {'SAVEECOBOT_API_KEY': 'a b'}]:
            with self.assertRaises(ValueError):
                firmware_values(data)
    def test_saveecobot_key(self):
        header = render_header({'SAVEECOBOT_API_KEY': 'test-only'})
        self.assertIn('constexpr char SAVEECOBOT_API_KEY[]', header)
        self.assertNotIn('test-only', header)

    def test_saveecobot_area(self):
        self.assertEqual(firmware_values({})['SAVEECOBOT_STATION'], 'Cherkasy, Cherkasy Oblast')
        self.assertEqual(firmware_values({'SAVEECOBOT_STATION': 'Черкаси'})['SAVEECOBOT_STATION'], 'Черкаси')
        self.assertIn('SAVEECOBOT_RADIUS_KM = 2.5;', render_header({'SAVEECOBOT_RADIUS_KM': '2.5'}))
        for value in ['', '0', '-1', 'NaN', 'inf', '501', '1;bad']:
            with self.assertRaises(ValueError):
                render_header({'SAVEECOBOT_RADIUS_KM': value})
        for value in ['', ' ', '123']:
            with self.assertRaises(ValueError):
                render_header({'SAVEECOBOT_STATION': value})

    def test_public_json_url(self):
        self.assertIn('22800.json', firmware_values({})['SAVEECOBOT_PUBLIC_JSON_URL'])
        self.assertEqual(firmware_values({'SAVEECOBOT_PUBLIC_JSON_URL': ''})['SAVEECOBOT_PUBLIC_JSON_URL'], '')
        for value in ['http://www.saveecobot.com/en/station/1.json', 'https://other.com/1.json', 'https://www.saveecobot.com/en/station/1.json?apikey=x']:
            with self.assertRaises(ValueError):
                firmware_values({'SAVEECOBOT_PUBLIC_JSON_URL': value})

    def test_ukraine_alarm_defaults(self):
        self.assertEqual(firmware_values({})['UKRAINEALARM_REGION_NAME'], 'Черкаська область')
        self.assertEqual(firmware_values({'UNUSED_PROVIDER_TOKEN': 'old-provider-key'})['UKRAINEALARM_API_KEY'], '')
        self.assertNotIn('old-provider-key', render_header({'UNUSED_PROVIDER_TOKEN': 'old-provider-key'}))
        with self.assertRaises(ValueError):
            firmware_values({'UKRAINEALARM_REGION_NAME': ''})

    def test_missing_file_and_refresh(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / '.env'
            output = Path(directory) / 'generated' / 'FirmwareDefaults.h'
            generate(source, output)
            self.assertIn('WIFI_SSID[] = ""', output.read_text())
            source.write_text('WIFI_SSID=abc\n')
            generate(source, output)
            self.assertIn(r'\141\142\143', output.read_text())
            source.unlink()
            generate(source, output)
            self.assertIn('WIFI_SSID[] = ""', output.read_text())

if __name__ == '__main__':
    unittest.main()
