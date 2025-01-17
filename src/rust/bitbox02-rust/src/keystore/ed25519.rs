// Copyright 2021 Shift Crypto AG
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

use alloc::vec::Vec;

use bip32_ed25519::{Xprv, Xpub, ED25519_EXPANDED_SECRET_KEY_SIZE};

fn get_seed() -> Result<zeroize::Zeroizing<Vec<u8>>, ()> {
    bitbox02::keystore::get_ed25519_seed()
}

fn get_xprv(keypath: &[u32]) -> Result<Xprv, ()> {
    let root = get_seed()?;
    Ok(Xprv::from_normalize(
        &root[..ED25519_EXPANDED_SECRET_KEY_SIZE],
        &root[ED25519_EXPANDED_SECRET_KEY_SIZE..],
    )
    .derive_path(keypath))
}

pub fn get_xpub(keypath: &[u32]) -> Result<Xpub, ()> {
    Ok(get_xprv(keypath)?.public())
}

pub struct SignResult {
    pub signature: [u8; 64],
    pub public_key: ed25519_dalek::PublicKey,
}

pub fn sign(keypath: &[u32], msg: &[u8; 32]) -> Result<SignResult, ()> {
    let xprv = get_xprv(keypath)?;
    let secret_key = ed25519_dalek::ExpandedSecretKey::from_bytes(&xprv.expanded_secret_key()[..])
        .or(Err(()))?;
    let public_key = ed25519_dalek::PublicKey::from(&secret_key);
    Ok(SignResult {
        signature: secret_key.sign(msg, &public_key).to_bytes(),
        public_key,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    use bip32_ed25519::HARDENED_OFFSET;
    use bitbox02::testing::{mock_unlocked, mock_unlocked_using_mnemonic};

    #[test]
    fn test_get_seed() {
        // Test vectors taken from:
        // https://github.com/cardano-foundation/CIPs/blob/6c249ef48f8f5b32efc0ec768fadf4321f3173f2/CIP-0003/Ledger.md#test-vectors
        // See also: https://github.com/cardano-foundation/CIPs/pull/132

        mock_unlocked_using_mnemonic(
            "recall grace sport punch exhibit mad harbor stand obey short width stem awkward used stairs wool ugly trap season stove worth toward congress jaguar",
            "",
        );
        assert_eq!(
            get_seed().unwrap().as_slice(),
            b"\xa0\x8c\xf8\x5b\x56\x4e\xcf\x3b\x94\x7d\x8d\x43\x21\xfb\x96\xd7\x0e\xe7\xbb\x76\x08\x77\xe3\x71\x89\x9b\x14\xe2\xcc\xf8\x86\x58\x10\x4b\x88\x46\x82\xb5\x7e\xfd\x97\xde\xcb\xb3\x18\xa4\x5c\x05\xa5\x27\xb9\xcc\x5c\x2f\x64\xf7\x35\x29\x35\xa0\x49\xce\xea\x60\x68\x0d\x52\x30\x81\x94\xcc\xef\x2a\x18\xe6\x81\x2b\x45\x2a\x58\x15\xfb\xd7\xf5\xba\xbc\x08\x38\x56\x91\x9a\xaf\x66\x8f\xe7\xe4",
        );

        // Multiple loop iterations.
        mock_unlocked_using_mnemonic(
            "correct cherry mammal bubble want mandate polar hazard crater better craft exotic choice fun tourist census gap lottery neglect address glow carry old business",
            "",
        );
        assert_eq!(
            get_seed().unwrap().as_slice(),
            b"\x58\x7c\x67\x74\x35\x7e\xcb\xf8\x40\xd4\xdb\x64\x04\xff\x7a\xf0\x16\xda\xce\x04\x00\x76\x97\x51\xad\x2a\xbf\xc7\x7b\x9a\x38\x44\xcc\x71\x70\x25\x20\xef\x1a\x4d\x1b\x68\xb9\x11\x87\x78\x7a\x9b\x8f\xaa\xb0\xa9\xbb\x6b\x16\x0d\xe5\x41\xb6\xee\x62\x46\x99\x01\xfc\x0b\xed\xa0\x97\x5f\xe4\x76\x3b\xea\xbd\x83\xb7\x05\x1a\x5f\xd5\xcb\xce\x5b\x88\xe8\x2c\x4b\xba\xca\x26\x50\x14\xe5\x24\xbd",
        );

        mock_unlocked_using_mnemonic(
            "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon art",
            "foo",
        );
        assert_eq!(
            get_seed().unwrap().as_slice(),
            b"\xf0\x53\xa1\xe7\x52\xde\x5c\x26\x19\x7b\x60\xf0\x32\xa4\x80\x9f\x08\xbb\x3e\x5d\x90\x48\x4f\xe4\x20\x24\xbe\x31\xef\xcb\xa7\x57\x8d\x91\x4d\x3f\xf9\x92\xe2\x16\x52\xfe\xe6\xa4\xd9\x9f\x60\x91\x00\x69\x38\xfa\xc2\xc0\xc0\xf9\xd2\xde\x0b\xa6\x4b\x75\x4e\x92\xa4\xf3\x72\x3f\x23\x47\x20\x77\xaa\x4c\xd4\xdd\x8a\x8a\x17\x5d\xba\x07\xea\x18\x52\xda\xd1\xcf\x26\x8c\x61\xa2\x67\x9c\x38\x90",
        );
    }

    #[test]
    fn test_get_xpub() {
        bitbox02::keystore::lock();
        assert!(get_xpub(&[]).is_err());

        mock_unlocked();

        let xpub = get_xpub(&[]).unwrap();
        assert_eq!(xpub.pubkey_bytes(), b"\x1c\xc2\xc8\x0d\x6f\xb0\x3e\xc0\x9e\x8a\x26\x8b\xaa\x45\xd4\xca\x2a\xfe\x5c\x5a\xc4\xdb\x3e\xe2\x9c\x7a\xd2\x37\x55\xab\xdc\x14");
        assert_eq!(xpub.chain_code(), b"\xf0\xa5\x91\x06\x42\xd0\x77\x98\x17\x40\x2e\x5e\x7a\x75\x54\x95\xe7\x44\xf5\x5c\xf1\x1e\x49\xee\xfd\x22\xa4\x60\xe9\xb2\xf7\x53");

        let xpub = get_xpub(&[10 + HARDENED_OFFSET, 10]).unwrap();
        assert_eq!(xpub.pubkey_bytes(), b"\xab\x58\xbd\x94\x7e\x2b\xf6\x64\xa7\xc0\x66\xde\x2e\xf0\x24\x0e\xfc\x24\xf3\x6e\xfd\x50\x2d\xf8\x83\x93\xe1\x96\xaf\x3c\x91\x8e");
        assert_eq!(xpub.chain_code(), b"\xf2\x00\x13\x38\x58\x02\xa6\xf9\xc0\x5e\xe7\xb0\x36\x16\xad\xf6\x9f\x5f\x9e\xc4\x32\x53\xa5\xd0\x8b\xe9\x65\x79\x81\x90\x83\xbb");
    }
}
