# Copyright 2017 Linaro Limited
# Copyright (c) 2018-2019, Arm Limited.
# Copyright (c) 2024, GigaDevice Semiconductor Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Image signing and management.
"""

from . import version as versmod
import hashlib
import struct

MANIFEST_VERSION = 0
IMAGE_MAGIC = 0x96f3b83d
IMAGE_HEADER_SIZE = 32
TLV_HEADER_SIZE = 4
TLV_INFO_SIZE = 4
TLV_INFO_MAGIC = 0x6907

# Image Type.
IMG_TYPE_VALUES = {
        'MBL' : 0x1,
        'IMG' : 0x2,}

# Image Hash Algorithm.
IMG_HASH_VALUES = {
        'SHA256' : 0x1,
        'SHA512' : 0x2,}

# Image Signature Algorithm.
IMG_SIG_VALUES = {
        'ED25519' : 0x1,
        'ECDSA256': 0x2,}

# Image TLV types.
TLV_TYPE_VALUES = {
        'DIGEST'  : 0x01,
        'CERT_PK' : 0x02,
        'CERT'    : 0x03,
        'PK'      : 0x04,
        'SIG'     : 0x05,}



class TLV():
    def __init__(self):
        self.buf = bytearray()

    def add(self, kind, payload):
        """Add a TLV record.  Kind should be a string found in TLV_VALUES above."""
        buf = struct.pack('<BBH', TLV_TYPE_VALUES[kind], 0, len(payload))
        self.buf += buf
        self.buf += payload

    def get(self):
        header = struct.pack('<HH', TLV_INFO_MAGIC, TLV_INFO_SIZE + len(self.buf))
        return header + bytes(self.buf)

class Image():
    @classmethod
    def load(cls, path, included_header=False, **kwargs):
        """Load an image from a given file"""
        with open(path, 'rb') as f:
            payload = f.read()
        obj = cls(**kwargs)
        obj.payload = payload

        # Add the image header if needed.
        if not included_header and obj.header_size > 0:
            obj.payload = (b'\000' * obj.header_size) + obj.payload

        obj.check()
        return obj

    def __init__(self, type, version, header_size=IMAGE_HEADER_SIZE, algo_hash='SHA256', algo_sig='ECDSA256',
                 pad=0):
        self.type = type
        self.version = version
        self.header_size = header_size or IMAGE_HEADER_SIZE
        self.algo_hash = algo_hash
        self.algo_sig = algo_sig
        self.pad = pad

    def __repr__(self):
        return "<Image type={}, version={}, header_size={}, \
                 pad={}, payloadlen=0x{:x}>".format(
                self.type,
                self.version,
                self.header_size,
                self.pad,
                len(self.payload))

    def save(self, path):
        with open(path, 'wb') as f:
            f.write(self.payload)

    def check(self):
        """Perform some sanity checking of the image."""
        # If there is a header requested, make sure that the image
        # starts with all zeros.
        if self.header_size > 0:
            if any(v != 0 and v != b'\000' for v in self.payload[0:self.header_size]):
                raise Exception("Padding requested, but image does not start with zeros")

    def sign(self, key, cert_file, cert_key):
        #image_version = (str(self.version.major) + '.'
        #                + str(self.version.minor) + '.'
        #                + str(self.version.revision))
        image_version = (str((self.version >> 24) & 0xff) + '.'
                        + str((self.version >> 16) & 0xff) + '.'
                        + str(self.version & 0xffff))

        tlv = TLV()

        # Full TLV size needs to be calculated in advance, because the
        # header will be protected as well
        tlv_size = (TLV_INFO_SIZE + len(tlv.buf))

        if cert_file is not None:
            with open(cert_file, 'rb') as f:
                cert = f.read()
                cert_size = len(cert)
                if cert_key is not None:
                    cert_pubkey = cert_key.get_public_bytes()
                    tlv_size += (TLV_HEADER_SIZE + len(cert_pubkey))
                else:
                  print("The cert key is None. return.")
                  return
        else:
            cert_size = 0
            if key is not None:
                pubkey = key.get_public_bytes()
                tlv_size += (TLV_HEADER_SIZE + len(pubkey))
            else:
                print("The key is None. return.")
                return

        tlv_size += (TLV_HEADER_SIZE + cert_size)

        if self.algo_hash == 'SHA512':
            digest_sz = 64
        else:
            digest_sz = 32
        tlv_size += (TLV_HEADER_SIZE + digest_sz)
        tlv_size += (TLV_HEADER_SIZE + key.sig_len())

        # Add image header
        self.add_header(tlv_size)

        if cert_file is not None:
            # Add TLV - cert public key
            tlv.add('CERT_PK', cert_pubkey)
            # Add TLV - Cert
            tlv.add("CERT", cert)
        else:
            # Add TLV - public key
            tlv.add('PK', pubkey)

        # Add TLV - Image digest
        hashsrc = self.payload[(self.header_size - IMAGE_HEADER_SIZE):len(self.payload)]
        # print ("start = ", (self.header_size - IMAGE_HEADER_SIZE))
        # print ("end = ", len(self.payload))
        if self.algo_hash == 'SHA512':
            sha = hashlib.sha512()
            sha.update(hashsrc)
            digest = sha.digest()
        else:
            self.algo_hash = 'SHA256'
            sha = hashlib.sha256()
            sha.update(hashsrc)
            digest = sha.digest()
        tlv.add('DIGEST', digest)

        # Add TLV - Image digest signature
        # `sign` expects the full image payload (sha256 done internally),
        # while `sign_digest` expects only the digest of the payload
        if hasattr(key, 'sign'):
            sig = key.sign(hashsrc)
        else:
            sig = key.sign_digest(digest)
        tlv.add('SIG', sig)

        # Append TLV Header
        tlv_header = struct.pack('HH', TLV_INFO_MAGIC, tlv_size)
        self.payload += tlv_header

        # Append all TLVs
        self.payload += tlv.get()[TLV_INFO_SIZE:]

    def add_header(self, tlv_size):
        """Install the image header.

        The key is needed to know the type of signature, and
        approximate the size of the signature.
        Now use ED25519 as default.     """

        pbytes = b'\xff' * (self.header_size - IMAGE_HEADER_SIZE)

        fmt = ('<' +
            # type ImageHdr struct {
            'I' +    # Magic    uint32
            'I' +    # TotalSz  uint32
            'B' +    # ManiVer  uint8
            'B' +    # ImgType  uint8
            'B' +    # HashAlg  uint8
            'B' +    # SignAlg  uint8
            'H' +    # HdrSz    uint16
            'H' +    # PTLVSz   uint16
            'I' +    # ImgSz    uint32
            'B' +    # Major version   uint8
            'B' +    # Minor version   uint8
            'H' +    # Revision        uint16
            'I'      # Rsvd     uint32
                     # Checksum uint32
            ) # }
        #assert struct.calcsize(fmt) == IMAGE_HEADER_SIZE
        img_size = len(self.payload) - self.header_size
        tot_size = IMAGE_HEADER_SIZE + img_size + tlv_size
        #print("img_size : ", img_size)
        #print("tlv_size : ", tlv_size)
        #print("tot_size : ", tot_size, "  hex: ", hex(tot_size))
        major = (self.version >> 24)
        minor = ((self.version & 0xFFFFFF) >> 16)
        revision = (self.version & 0xFFFF)
        #print("major: ", major, " minor: ", minor, " rev: ", revision)
        #print("image type : ", IMG_TYPE_VALUES[self.type])
        header = struct.pack(fmt,
                IMAGE_MAGIC,
                tot_size,
                MANIFEST_VERSION,
                IMG_TYPE_VALUES[self.type],
                IMG_HASH_VALUES[self.algo_hash],
                IMG_SIG_VALUES[self.algo_sig],
                IMAGE_HEADER_SIZE,
                tlv_size,
                img_size,
                major,
                minor,
                revision,
                0)  # Rsvd

        n = 0
        chksum = 0
        while n < len(header):
            num = header[n]
            num += (header[n + 1]) << 8
            num += (header[n + 2]) << 16
            num += (header[n + 3]) << 24
            #print("num: ", num, "  hex: ", hex(num))
            chksum ^= num
            n += 4
            #print("chksum hex: ", hex(chksum))
        chksum &= 0xFFFFFFFF
        header += struct.pack('<I', chksum)

        #print("header : ", header)
        header_with_pad = pbytes + header

        self.payload = bytearray(self.payload)
        self.payload[:len(header_with_pad)] = header_with_pad

    def pad_to(self, size, align):
        """Pad the image to the given size, with the given flash alignment."""
        if align not in [1, 2, 4, 8]:
            raise Exception("Align must be one of 1, 2, 4 or 8.")
        remain = size % align
        if remain != 0:
          size += align - remain
        padding = size - len(self.payload)
        if padding < 0:
            msg = "Image size (0x{:x}) exceeds requested size 0x{:x}".format(
                    len(self.payload), size)
            raise Exception(msg)
        pbytes  = b'\xff' * padding
        self.payload += pbytes
