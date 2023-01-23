#include <iostream>
#include <string>
#include "pem.h"
#include "rsa.h"
#include "sha.h"
#include "base64.h"
#include "secblock.h"
// using CryptoPP::SecByteBlock;
// using CryptoPP::PEM_Load;

#include "osrng.h"
// using CryptoPP::AutoSeededRandomPool;
#include "files.h"
#include "pssr.h"
#include "hex.h"
#include "base64.h"

using namespace CryptoPP;


RSA::PublicKey loadPublicKey() {
    const std::string publickey_str =
    "-----BEGIN PUBLIC KEY-----\n"
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAz4cetS8lMhYsx/YstOmZ\n"
    "a49us75YWzd6dy/8/NdQlb/fK+WG64Od16Y5BNlKg0fGBcCT6tX/FxoM9qIHnnda\n"
    "C4bbVwoCwR8QE1zV/c1I8RrHbgA/y+/HNY56oS0i7Ye12Ip42PJGGRLyYTHBXpaL\n"
    "adqqlhU09x91xY1jt258sKt6S+o1y8g9rMEB8wmUF/c9vWeCW25I4HHY7Z7tyAUz\n"
    "Fz6pUrHFtlqZ1apAqvD+k3Iu33LGIeHqQbJMUEfiahi/lLU4cuOrJskdY4597jHT\n"
    "EHbwyXbMCxsmB52FLduSe0BzVtYGvtmCWjY8HT/9h5O5WHcfd+N3OoaKtV58qX39\n"
    "YQIDAQAB\n"
    "-----END PUBLIC KEY-----\n";

    RSA::PublicKey publicKey;

    try
    {
        StringSource source(publickey_str, true);
        PEM_Load(source, publicKey);
    }
    catch(const Exception& ex)
    {
        std::cerr << ex.what() << std::endl;
    }
    return publicKey;
}

RSA::PrivateKey loadPrivateKey () {
    RSA::PrivateKey privatekey;

    try
    {
        FileSource source("keypair.pem", true);
        PEM_Load(source, privatekey);
    }
    catch(const Exception& ex)
    {
        std::cerr << ex.what() << std::endl;
    }
    return privatekey;
}


int main_back () {
    // Generate key material

    AutoSeededRandomPool prng;
    CryptoPP::RSA::PublicKey publicKey = loadPublicKey();

    std::string signature;
   
    FileSource fs2("/home/srishti/Desktop/neo/bin/Debug-linux-x86_64/zero.dat.sig", true,
        new Base64Decoder(
            new StringSink(signature)
        ) // Base64Decoder
    ); // StringSource
    // Print the signature
    std::cout << "Signature: ";
    StringSource(signature, true, new HexEncoder(new FileSink(std::cout)));
    std::cout << std::endl;
    // Create a verifier
    byte result = 0;
    RSASS<PKCS1v15, SHA256>::Verifier verifier(publicKey);
    SignatureVerificationFilter filter(verifier, new ArraySink(&result, sizeof(result)));

    // Wrap the data in sources
    StringSource ss(signature, true);
    FileSource fs("/home/srishti/Desktop/neo/bin/Debug-linux-x86_64/interceptor_control.config.json", true);

    // Add the data to the filter
    ss.TransferTo(filter);
    fs.TransferTo(filter);

    // Signal end of data. The filter will
    // verify the signature on the file
    filter.MessageEnd();

    if (result)
        std::cout << "Verified signature on file" << std::endl;
    else
        std::cout << "Failed to signature hash on file" << std::endl;
return 0;
}

