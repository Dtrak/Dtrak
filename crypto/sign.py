from Cryptodome.Signature import pkcs1_15
from Cryptodome.Hash import SHA256
from Cryptodome.PublicKey import RSA
import base64

message = open("infile.json", "rb").read()

key = RSA.import_key(open('private_key.pem').read())

h = SHA256.new(message)

signature = pkcs1_15.new(key).sign(h)

print(base64.b64encode(signature).decode())
print(signature.hex())

with open("outfile.sig", "w") as file:
    file.write(base64.b64encode(signature).decode())