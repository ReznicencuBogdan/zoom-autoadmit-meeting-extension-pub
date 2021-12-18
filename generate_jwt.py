import time
import datetime
import jwt

iat = int(time.time())
exp = int((datetime.datetime.today() + datetime.timedelta(days=2)).timestamp())
tokenExp = exp

appKey = ""
secret = ""

payload = {
   'appKey': appKey,
   'iat': iat,
   'exp': exp,
   'tokenExp': tokenExp
}
encoded = jwt.encode(payload, secret, algorithm='HS256')

print(encoded)