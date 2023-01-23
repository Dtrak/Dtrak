# dtrak
DTRAK

DTRAK is a zero trust device vulnerability assessment tool that checks a user device on a set of predefined parameters.

DTRAK has two major parts. Config file, with all the parameters, and DTRAK, that checks whether those parameters are met, and generates a report based on it which can be later implemented by third party services, to ensure device security and avoid data breach because of a compromised device. Config file is secured at a server written in python, and the device test is performed at the client end in C++.

Step 1:Key generation:-

To implement the DTRAK, first you need to generate your own RSA keypair. You can generate it by any method you want, or follow the below steps in order to generate it:-
Run the following command in your terminal:-
openssl genrsa -des3 -out private.pem 2048
It’ll ask for a passphrase to generate your key pair. Provide one and remember it.Now run this command:-
openssl rsa -in private.pem -outform PEM -pubout -out public.pem

This will generate private.pem and public.pem files.

Step 2:Implementing config encryption:-

Now, you need a system on your server to sign the config file, so that it can be verified for any manipulations on the client end. We highly recommend this step to ensure the safety measures have been taken and trustlevel is calculated on the predefined parameters. This part of code is written in python, so you need to do some setup.

Requirements for python code:- 
Python >= 3.9
Pip
Setup:-

We would prefer if you create a new virtual environment, using “python -m virtualenv venv_name”, and start it with “venv_name\Scripts\activate” (On windows) or “source venv_name/bin/activate”(On Ubuntu).

Run the following command to install the dependencies:-
pip install cffi==1.15.1 pyasn1==0.4.8 pycparser==2.21 pycryptodome-test-vectors==1.0.11 pycryptodomex==3.16.0 rsa==4.9

Download and extract the folder crypto from git (https://github.com/Dtrak/dtrak/tree/main/crypto).

Replace private_key.pem with the private key you generated in step 1.

Make and run:-

Run the code by “python sign.py”

Step 3: Verification and Trust level calculation
Once the config file is signed, we can make sure it has not been tampered with, and check the trustlevel based on the parameters of the config file.

Requirements for C++ code:- 
g++ compiler

Setup:-

Download the C++ code from github (https://github.com/Dtrak/dtrak/tree/main/Interceptor)
Download cryptopp library from it’s official website (https://cryptopp.com/#download)
Download cryptopp PEM pack from Git (https://github.com/noloader/cryptopp-pem)
Extract the C++ code of Interceptor in a folder.(We’ll call it Interceptor)
Create a folder inside the vendor folder. Extract cryptopp library there. (We’ll call it cryptopp870 for example)
Extract the files of PEM pack into the folder as well (Into cryptopp870)
Make sure you can find the pem.h file into the cryptopp870 folder.
Open terminal in cryptopp870.
Write the command “make”.
This will create a “libcryptopp.a” file into the cryptopp870 folder.
Copy this file, and paste it into the libs, outside the cryptopp870 folder.

Make and run commands:-
Open the Interceptor folder, open terminal, and run command “premake5 gmake2”
Run “make config=debug”
Navigate to src/Debug-linux-x86_64
Run ./Interceptor.
Run ./Interceptor chrome-extension=authvr5
This will write the trustlevel to the result.txt file


Usage:- 

You can now use the result.txt file to trigger an API that can verify the machine’s trust level. You can also determine how much access should be given to that machine, and if that machine is prone to be compromised.






