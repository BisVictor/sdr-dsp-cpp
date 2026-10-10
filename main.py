import requests
import hashlib
from requests.auth import HTTPDigestAuth

KEENETIC_IP = "192.168.1.1"
USERNAME = "admin"
PASSWORD = "Bisvictor13"          

session = requests.Session()

def keenetic_login():
    # 1. Получаем challenge
    resp = session.get(f"http://{KEENETIC_IP}/auth", timeout=5)
    
    if resp.status_code != 401:
        print("Неожиданный статус:", resp.status_code)
        return False
    
    realm = resp.headers.get("X-NDM-Realm")
    challenge = resp.headers.get("X-NDM-Challenge")
    
    print(f"Realm: {realm}")
    print(f"Challenge: {challenge}")
    
    if not realm or not challenge:
        print("Не получены нужные заголовки")
        return False
    
    # 2. Правильный расчёт хеша
    md5_hash = hashlib.md5(f"{USERNAME}:{realm}:{PASSWORD}".encode("utf-8")).hexdigest()
    sha_hash = hashlib.sha256((challenge + md5_hash).encode("utf-8")).hexdigest()
    
    print(f"MD5:  {md5_hash}")
    print(f"SHA:  {sha_hash}")
    
    # 3. Отправляем авторизацию
    auth_resp = session.post(
        f"http://{KEENETIC_IP}/auth",
        json={"login": USERNAME, "password": sha_hash},
        timeout=5
    )
    
    print("Статус авторизации:", auth_resp.status_code)
    
    if auth_resp.status_code == 200:
        print("✅ Успешный вход в Keenetic")
        return True
    else:
        print("❌ Ошибка входа")
        print(auth_resp.text[:300])
        return False


#keenetic_login()

if keenetic_login():
    print("✅ Вход выполнен")
    
    # Получаем информацию о системе
    r = session.get(f"http://{KEENETIC_IP}/rci/show/ip", timeout=5)
    print("Статус:", r.status_code)
    print(r.json())

    data = r.json()

    for name, info in data.items():
        if isinstance(info, dict):
            print(f"\nИнтерфейс: {name}")
            print(f"  Тип: {info.get('type')}")
            print(f"  Описание: {info.get('description')}")
            print(f"  Адрес: {info.get('address')}")
            print(f"  Маска: {info.get('mask')}")
            print(f"  Состояние: {info.get('state')}")
else:
    print("❌ Вход не удался")