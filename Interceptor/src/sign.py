from Cryptodome.Signature import pkcs1_15
from Cryptodome.Hash import SHA256
from Cryptodome.PublicKey import RSA
import base64

message = open("zero.dat", "rb").read()

key = RSA.import_key(open('keypair.pem').read())

h = SHA256.new(message)

signature = pkcs1_15.new(key).sign(h)

print(base64.b64encode(signature).decode())
print(signature.hex())

with open("zero.dat.sig", "w") as file:
    file.write(base64.b64encode(signature).decode())

# key = RSA.import_key(open('public.pem', "r").read())

# h = SHA256.new(message)

# sig_c = bytes.fromhex("CABBB90980AD29B04170923C7FA6DA37138556EBAA62DC3C0AAB6C01CE86066AC17DC2FB69FB00D617A7EBCD7BF5271D95AC52FBA78E41E44BA038BF0099CEB2CA729EE1D35B76B58C0542E3D3F5311DB6E9852694E309E18175B4A0D05E65A63361F699F34FA636B5A253F7818DC064B27501C8BC2BA17FF11421A60CE9F7652C9E51C5A0EDC6ACC55561728D5FE352762FA1897E0C720A5C3B77DDE0479BAE58D10F0EC8E1E2043090D8D789B55E5101474B10BB3158712F9AA00D69B606CADD399D8DDE0E2709B6673F521CE586CC97A4104A2A7CB1C4749192A8019FE63F01914A51199F2345AF265A60F289DCF24EFC64D8CADF31B531AF34CC771AB8CD")


# try:

#     pkcs1_15.new(key).verify(h, sig_c)

#     print("The signature is valid.")

# except (ValueError, TypeError):

#    print("The signature is not valid.")