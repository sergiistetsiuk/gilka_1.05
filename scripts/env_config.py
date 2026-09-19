"""Read a literal .env subset and generate private firmware defaults, without logging values."""
import re
import shlex
from pathlib import Path

DEFAULTS = {
    'WIFI_SSID': '',
    'WIFI_PASSWORD': '',
    'OPENWEATHERMAP_API_KEY': '',
    'UKRAINEALARM_API_KEY': '',
    'UKRAINEALARM_REGION_NAME': 'Черкаська область',
    'SAVEECOBOT_API_KEY': '',
    'SAVEECOBOT_PUBLIC_JSON_URL': 'https://www.saveecobot.com/en/station/22800.json',
    'SAVEECOBOT_STATION': 'Cherkasy, Cherkasy Oblast',
    'SAVEECOBOT_RADIUS_KM': '25',
    'DEVICE_TIMEZONE': 'EET-2EEST,M3.5.0/3,M10.5.0/4',
}
LIMITS = {'WIFI_SSID': 32, 'WIFI_PASSWORD': 64,
          'OPENWEATHERMAP_API_KEY': 128, 'UKRAINEALARM_API_KEY': 128, 'UKRAINEALARM_REGION_NAME': 128, 'DEVICE_TIMEZONE': 80, 'SAVEECOBOT_API_KEY': 128, 'SAVEECOBOT_PUBLIC_JSON_URL': 192, 'SAVEECOBOT_STATION': 128, 'SAVEECOBOT_RADIUS_KM': 10}


def parse_env(text):
    values = {}
    for number, line in enumerate(text.splitlines(), 1):
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        if line.startswith('export '):
            line = line[7:].lstrip()
        key, separator, raw = line.partition('=')
        key = key.strip()
        if not separator or not re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]*', key):
            raise ValueError(f'Invalid .env assignment on line {number}')
        try:
            tokens = shlex.split(raw, comments=True, posix=True)
        except ValueError:
            raise ValueError(f'Invalid .env quoting on line {number}') from None
        if len(tokens) > 1:
            raise ValueError(f'Quote values containing spaces on .env line {number}')
        values[key] = tokens[0] if tokens else ''
    return values


def firmware_values(values):
    result = dict(DEFAULTS)
    for key in result:
        value = values.get(key, result[key])
        if key == 'DEVICE_TIMEZONE' and not value:
            value = DEFAULTS[key]
        if len(value.encode('utf-8')) > LIMITS[key] or any(ord(c) < 32 or ord(c) == 127 for c in value):
            raise ValueError(f'Invalid length or control characters in {key}')
        if key in ('OPENWEATHERMAP_API_KEY', 'UKRAINEALARM_API_KEY', 'SAVEECOBOT_API_KEY') and any(ord(c) <= 32 or ord(c) >= 127 for c in value):
            raise ValueError(f'Invalid characters in {key}')
        if key == 'DEVICE_TIMEZONE' and not re.fullmatch(r'[A-Za-z0-9+\-:,./<>_]+', value):
            raise ValueError('Invalid characters in DEVICE_TIMEZONE')
        if key == 'SAVEECOBOT_PUBLIC_JSON_URL' and value and not re.fullmatch(r'https://www\.saveecobot\.com/(?:en/)?station/[1-9][0-9]*\.json', value):
            raise ValueError('SAVEECOBOT_PUBLIC_JSON_URL must be a SaveEcoBot station JSON URL')
        if key == 'UKRAINEALARM_REGION_NAME' and not value.strip():
            raise ValueError('UKRAINEALARM_REGION_NAME cannot be empty')
        if key == 'SAVEECOBOT_STATION':
            value = value.strip()
            if len(value) < 2 or not any(c.isalpha() for c in value):
                raise ValueError('SAVEECOBOT_STATION must contain a city name')
        if key == 'SAVEECOBOT_RADIUS_KM':
            if not re.fullmatch(r'[0-9]+(?:\.[0-9]+)?', value) or not 0 < float(value) <= 500:
                raise ValueError('SAVEECOBOT_RADIUS_KM must be greater than 0 and at most 500')
            value = str(float(value))
        result[key] = value
    return result


def render_header(values):
    lines = ['#pragma once', '// Generated from .env; do not commit or print.', 'namespace FirmwareDefaults {']
    for key, value in firmware_values(values).items():
        if key == 'SAVEECOBOT_RADIUS_KM':
            lines.append(f'constexpr double {key} = {value};')
            continue
        # Fixed-width octal UTF-8 bytes avoid C++/shell injection and escaping ambiguity.
        literal = ''.join('\\%03o' % byte for byte in value.encode('utf-8'))
        lines.append(f'constexpr char {key}[] = "{literal}";')
    return '\n'.join(lines + ['}', ''])


def generate(source, destination):
    source, destination = Path(source), Path(destination)
    content = render_header(parse_env(source.read_text(encoding='utf-8')) if source.exists() else {})
    destination.parent.mkdir(parents=True, exist_ok=True)
    if not destination.exists() or destination.read_text(encoding='utf-8') != content:
        destination.write_text(content, encoding='utf-8')
    destination.chmod(0o600)
