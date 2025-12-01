#include <sodium.h>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <sys/stat.h>

static constexpr size_t KEYLEN = 32;

int main()
{
    if (sodium_init() < 0) {
        std::cerr << "libsodium init failed\n";
        return 1;
    }

    const std::string dir = "/etc/huxley";
    const std::string masterPath  = dir + "/master.key";
    const std::string sessionPath = dir + "/session.key.enc";

    // Ensure directory exists
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) {
        std::cerr << "Cannot create " << dir << ": " << ec.message() << "\n";
        return 1;
    }

    // Tighten permissions
    chmod(dir.c_str(), 0700);

    // If master already exists, do NOT overwrite it
    unsigned char master[KEYLEN];
    bool masterExists = std::filesystem::exists(masterPath);

    if (!masterExists) {
        crypto_secretbox_keygen(master);

        std::ofstream mf(masterPath, std::ios::binary);
        if (!mf) {
            std::cerr << "Cannot write master key.\n";
            sodium_memzero(master, KEYLEN);
            return 1;
        }
        mf.write((char*)master, KEYLEN);
        mf.close();

        chmod(masterPath.c_str(), 0600);

        std::cout << "[+] Generated master key\n";
    } else {
        std::ifstream mf(masterPath, std::ios::binary);
        if (!mf) {
            std::cerr << "Cannot read existing master key.\n";
            return 1;
        }
        mf.read((char*)master, KEYLEN);
        mf.close();

        std::cout << "[*] Using existing master key\n";
    }

    // Generate session key
    unsigned char session[KEYLEN];
    crypto_secretbox_keygen(session);

    // Encrypt session with master
    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    randombytes_buf(nonce, sizeof nonce);

    unsigned char sealed[crypto_secretbox_MACBYTES + KEYLEN];
    crypto_secretbox_easy(sealed, session, KEYLEN, nonce, master);

    // Write sealed session key
    {
        std::ofstream out(sessionPath, std::ios::binary);
        if (!out) {
            std::cerr << "Cannot write sealed session key.\n";
            sodium_memzero(master, KEYLEN);
            sodium_memzero(session, KEYLEN);
            return 1;
        }

        out.write((char*)nonce, sizeof nonce);
        out.write((char*)sealed, sizeof sealed);
        out.close();
    }
    chmod(sessionPath.c_str(), 0600);

    // Zero memory
    sodium_memzero(master, KEYLEN);
    sodium_memzero(session, KEYLEN);

    std::cout << "[+] Session key generated and sealed\n";
    std::cout << "[DONE] huxley-keygen completed successfully.\n";

    return 0;
}