with open("ks313ddf3a3r1die951a.dmp", "rb") as encrypted_file:
  
    file_bytes = bytearray(encrypted_file.read())
    
    for i in range(len(file_bytes)):
        file_bytes[i] ^= ord('w')
    
    with open("ks313ddf3a3r1die951a-decrypted.dmp", 'wb') as decrypted_file:
        decrypted_file.write(file_bytes)
        print("Done")
