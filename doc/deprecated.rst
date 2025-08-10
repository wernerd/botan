Deprecated Features
========================

Certain functionality is deprecated and is likely to be removed in
a future major release.

To help warn users, macros are used to annotate deprecated functions
and headers. These warnings are enabled by default, but can be
disabled by defining the macro ``BOTAN_NO_DEPRECATED_WARNINGS`` prior
to including any Botan headers.

.. warning::
    Not all of the functionality which is currently deprecated has an
    associated warning.

If you are using something which is currently deprecated and there
doesn't seem to be an obvious alternative, contact the developers to
explain your use case if you want to make sure your code continues to
work.

Platform Support Deprecations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Support for building for Windows systems prior to Windows 10 is deprecated.

TLS Protocol Deprecations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The following TLS protocol features are deprecated and will be removed
in a future major release:

- Support for point compression in TLS. This is supported in v1.2 but
  removed in v1.3. For simplicity it will be removed in v1.2 also.

- All CBC mode ciphersuites. This includes all available 3DES ciphersuites.
  This implies also removing Encrypt-then-MAC extension.

- All DHE ciphersuites

- Support for renegotiation in TLS v1.2

- All ciphersuites using static RSA key exchange

- ``Credentials_Manager::psk()`` to provide various TLS-specific keys and
  secrets, most notably "session-ticket", "dtls-cookie-secret" and the actual
  TLS PSKs for given identities and hosts. Instead, use the dedicated methods in
  ``Credentials_Manager`` and do not override the ``psk()`` method any longer.

Elliptic Curve Deprecations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

A number of features relating to elliptic curves are deprecated.  As a typical
user you would probably not notice these; their removal would not affect for
example using ECDSA signatures or TLS, but only applications doing usual things
such as custom elliptic curve parameters, or creating your own protocol using
elliptic curve points.

- Support for explicit ECC curve parameters and ImplicitCA encoded parameters in
  ``EC_Group`` and all users (including X.509 certificates and PKCS#8 private keys).

- Currently it is possible to create an ``EC_Group`` with cofactor > 1. None of
  the builtin groups have composite order, and in the future it will be
  impossible to create composite order ``EC_Group``.

- Currently it is possible to create an application specific ``EC_Group``
  with parameters of effectively arbitrary size. In a future release
  the maximum allowed bitlength of application provided groups will be
  at most 521 bits.

- Elliptic curve points can be encoded in several different ways.  The
  most common are "compressed" and "uncompressed"; both are widely
  used in various systems. Botan additionally supports a "hybrid"
  encoding format which is effectively uncompressed but with an
  additional indicator of the parity of the y coordinate. This
  format is quite obscure and seemingly rarely implemented. Support
  for this encoding will be removed in a future release.

- Botan currently contains support for a number of relatively weak or little
  used elliptic curves. These are deprecated. These include "secp160k1",
  "secp160r1", "secp160r2", "secp192k1", "secp224k1", "brainpool160r1",
  "brainpool192r1", "brainpool224r1", "brainpool320r1", "x962_p192v2",
  "x962_p192v3", "x962_p239v1", "x962_p239v2", "x962_p239v3"

- Currently `EC_Point` offers a wide variety of functionality almost
  all of which was intended only for internal implementation. In a
  future release, the only operations available for EC_Points will be
  to extract the byte encoding of their affine x and y coordinates.

- Currently it is possible to create an EC_Group with cofactor > 1.
  None of the builtin groups have composite order, and in the future
  it will be impossible to create composite order EC_Groups.

Deprecated Modules
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In a number of cases an entire module is deprecated. If the build is configured
with ``--disable-deprecated`` then these will not be included. In a future major
release the source for these modules will be entirely removed.

Deprecated modules include

- Kyber mode ``kyber_90s``: Kyber's "90s mode" is not in the NIST ML-KEM
  standard, and seems to have been never implemented widely.

- Dilithium mode ``dilithium_aes``: Similar situation to Kyber 90s mode.

- Block cipher ``gost_28147``: This cipher was obsolete 20 years ago.

- Block cipher ``noekeon``: An interesting design but not widely implemented.

- Block cipher ``lion``: Similar situation to Noekeon

- Hash function ``gost_3411``: Very weak and questionable hash function.

- Hash function ``streebog``: Incredibly sketchy situation with the sbox

- Hash function ``md4``: It's time to let go

- Signature scheme ``gost_3410``

- McEliece implementation ``mce``. Will be replaced by the proposal Classic
  McEliece.

- Stream cipher ``shake_cipher``. Note this deprecation affects only
  using SHAKE as a ``StreamCipher`` not as a hash or XOF

- `cryptobox`: A not unreasonable password based encryption utility
  but neither modern (these days) nor widely implemented.

- ``dlies``: DLIES is considered quite obsolete

- ``tpm`` (TPM 1.2 only, rarely tested)

Other Deprecated Functionality
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This section lists other functionality which will be removed in a future major
release, or where a backwards incompatible change is expected.

- The ``PBKDF`` class is deprecated in favor of ``PasswordHash`` and
  ``PasswordHashFamily``.

- Implicit conversion of a private key into a public key. Currently
  ``Private_Key`` derives from ``Public_Key`` (and likewise for each of the
  algorithm specfic classes, eg ``RSA_PrivateKey`` derives from
  ``RSA_PublicKey``). In a future release these derivations will not exist. To
  correctly extract the public key from a private key, use the function
  ``Private_Key::public_key()``

- Prior to 2.8.0, SM2 algorithms were implemented as two distinct key
  types, one used for encryption and the other for signatures. In 2.8,
  the two types were merged. However it is still possible to refer to
  SM2 using the split names of "SM2_Enc" or "SM2_Sig". In a future major
  release this will be removed, and only "SM2" will be recognized.

- DSA, ECDSA, ECGDSA, ECKCDSA, and GOST-34.10 previously (before Botan 3)
  required that the hash be named as "EMSA1(HASH_NAME)". This is no longer
  required. In a future major release, only "HASH_NAME" will be accepted.

- The ``Buffered_Computation`` base class. In a future release the
  class will be removed, and all of member functions instead declared
  directly on ``MessageAuthenticationCode`` and ``HashFunction``. So
  this only affects you if you are directly referencing
  ``Botan::Buffered_Computation`` in some way.

- GCM support for 64-bit tags

- All built in MODP groups < 2048 bits

- All pre-created DSA groups

- All support for loading, generating or using RSA keys with a public
  exponent larger than 2**64-1

Deprecated Headers
^^^^^^^^^^^^^^^^^^^^^^

These headers are currently publically available, but will be made
internal to the library in the future.

  PBKDF headers: ``bcrypt_pbkdf.h``, ``pbkdf2.h``, ``pgp_s2k.h``, ``scrypt.h``,
  and ``argon2.h``: Use the ``PasswordHash`` interface instead.

  Internal implementation headers - seemingly no reason for applications to use:
  ``curve_gfp.h``,
  ``reducer.h``,
  ``tls_algos.h``,
  ``tls_magic.h``

  Utility headers, nominally useful in applications but not a core part of
  the library API and most are just sufficient for what the library needs
  to implement other functionality.
  ``compiler.h``,
  ``uuid.h``,
