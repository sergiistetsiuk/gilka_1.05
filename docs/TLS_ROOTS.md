# TLS trust roots

Selected public Mozilla roots from the locally installed certifi CA store. No private keys or local interception roots are included.

- DigiCert Global Root G2
- USERTrust RSA Certification Authority
- USERTrust ECC Certification Authority
- ISRG Root X1
- ISRG Root X2
- GTS Root R1
- GTS Root R3
- GTS Root R4
- Sectigo Public Server Authentication Root E46
- Sectigo Public Server Authentication Root R46

TLS hostname and certificate verification remain enabled. A provider changing certificate chains may require a trust-store update.

- Amazon Root CA 1, added for Safecast (issuer Amazon RSA 2048 M04). Downloaded from [Amazon Trust Services](https://www.amazontrust.com/repository/AmazonRootCA1.pem) and matched against the local system trust store. SHA-256: `8ECDE6884F3D87B1125BA31AC3FCB13D7016DE7F57CC904FE1CB97C6AE98196E`.
