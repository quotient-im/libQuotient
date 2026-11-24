use log::error;
use matrix_sdk_common::ruma::serde::base64::Standard;
use matrix_sdk_common::ruma::{DeviceId, UserId};
use matrix_sdk_crypto::OlmMachine;
use matrix_sdk_sqlite::SqliteCryptoStore;
use std::{mem::ManuallyDrop, path::Path};
use tracing_subscriber::{EnvFilter, fmt, prelude::*};

mod cryptomachine;
mod encryption_info;
mod file_crypto;
mod request;
mod verification;

use crate::ffi::InitError;
use cryptomachine::BackupRequestResult;
use cryptomachine::ConfirmRequestsResult;
use cryptomachine::CreatedSessionResult;
use cryptomachine::CryptoMachine;
use cryptomachine::EmojiResult;
use cryptomachine::EncryptionInfoResult;
use cryptomachine::Key;
use cryptomachine::KeyResult;
use cryptomachine::KeyVerificationRequestResult;
use cryptomachine::MissingSessionsResult;
use cryptomachine::OutgoingKeyVerificationRequestResult;
use cryptomachine::OutgoingRequestResult;
use cryptomachine::StringResult;
use cryptomachine::SyncChanges;
use cryptomachine::SyncChangesResult;
use cryptomachine::ToDeviceRequestResult;
use cryptomachine::U8Result;
use cryptomachine::CrossSigningBootstrapRequestsResult;
use file_crypto::DecryptResult;

fn init(
    user_id: String,
    device_id: String,
    path: String,
    pickle_key: String,
    account_pickle: String,
) -> Box<CryptoMachine> {
    // This will fail when initializing a second account, but that's ok
    let _ = tracing_subscriber::registry()
        .with(fmt::layer())
        .with(EnvFilter::from_default_env())
        .try_init();
    let rt = tokio::runtime::Runtime::new().expect("Failed to create runtime");
    let _ = rt.enter();

    enum InnerInitResult {
        Err(InitError, String),
        Ok(OlmMachine),
    }

    let result = rt.block_on(async {
        let user_id = match UserId::parse(&user_id) {
            Ok(user_id) => user_id,
            Err(err) => {
                error!(
                    "Failed to initialize cryptomachine: Invalid user id '{}'",
                    user_id
                );
                return InnerInitResult::Err(InitError::InvalidUserId, err.to_string());
            }
        };
        let device_id: Box<DeviceId> = device_id.into();

        let store = match SqliteCryptoStore::open(Path::new(&path), Some(&pickle_key)).await {
            Ok(store) => store,
            Err(err) => {
                error!("Failed to open crypto store: {:?}", err);
                return InnerInitResult::Err(InitError::StoreFailed, err.to_string());
            }
        };

        let account = if account_pickle.is_empty() {
            None
        } else {
            let decoded =
                matrix_sdk_common::ruma::serde::Base64::<Standard>::parse(pickle_key.as_bytes())
                    .unwrap();
            Some(
                matrix_sdk_crypto::vodozemac::olm::Account::from_libolm_pickle(&account_pickle, &decoded.as_bytes())
                    .unwrap(),
            )
        };

        match OlmMachine::with_store(&user_id, &device_id, store, account).await {
            Ok(machine) => InnerInitResult::Ok(machine),
            Err(err) => {
                error!("Failed to create olm machine: {:?}", err);
                return InnerInitResult::Err(InitError::DatabaseError, err.to_string());
            }
        }
    });

    Box::new(match result {
        InnerInitResult::Ok(machine) => CryptoMachine {
            error: InitError::Ok,
            error_string: "Success".to_string(),
            runtime: rt,
            machine: Some(ManuallyDrop::new(machine)),
            pending_backup_decryption_key: None,
        },
        InnerInitResult::Err(error, error_string) => CryptoMachine {
            error,
            error_string,
            runtime: rt,
            machine: None,
            pending_backup_decryption_key: None,
        },
    })
}

use cryptomachine::BackupRequest;
use encryption_info::EncryptionInfo;
use file_crypto::MediaEncryptionInfo;
use file_crypto::decrypt_file;
use file_crypto::encrypt_file;
use request::{
    ConfirmRequests, KeyVerificationRequest, KeysClaimRequest, OutgoingKeyVerificationRequest,
    OutgoingRequest, ToDeviceRequest, CrossSigningBootstrapRequests,
};
use verification::{CreatedSession, Emoji};

#[cxx::bridge]
mod ffi {
    struct OlmSessionData {
        libolm_pickle: String,
        sender_key: String,
        creation_time: u64,
        last_use_time: u64,
    }

    struct MegolmSessionData {
        libolm_pickle: String,
        sender_curve_key: String,
        room_id: String,
    }

    #[derive(Clone, PartialEq)]
    enum InitError {
        Ok,
        InvalidUserId,
        StoreFailed,
        DatabaseError,
    }

    enum OutgoingRequestType {
        KeysUpload,
        KeysQuery,
        KeysClaim,
        ToDevice,
        SignatureUpload,
        RoomMessage,
    }

    #[namespace = "crypto"]
    extern "Rust" {
        type CryptoMachine;
        type OutgoingRequest;
        type ToDeviceRequest;
        type KeysClaimRequest;
        type KeyVerificationRequest;
        type OutgoingKeyVerificationRequest;
        type Emoji;
        type CreatedSession;
        type EncryptionInfo;
        type MediaEncryptionInfo;
        type ConfirmRequests;
        type BackupRequest;
        type SyncChanges;
        type Key;
        type SyncChangesResult;
        type OutgoingRequestResult;
        type MissingSessionsResult;
        type ConfirmRequestsResult;
        type OutgoingKeyVerificationRequestResult;
        type ToDeviceRequestResult;
        type StringResult;
        type U8Result;
        type EmojiResult;
        type KeyResult;
        type BackupRequestResult;
        type CreatedSessionResult;
        type KeyVerificationRequestResult;
        type EncryptionInfoResult;
        type DecryptResult;
        type CrossSigningBootstrapRequests;
        type CrossSigningBootstrapRequestsResult;

        // General CryptoMachine functions
        fn init(
            user_id: String,
            device_id: String,
            path: String,
            pickle_key: String,
            account_pickle: String,
        ) -> Box<CryptoMachine>;
        fn outgoing_requests(self: &CryptoMachine) -> Box<OutgoingRequestResult>;
        fn receive_sync_changes(
            self: &mut CryptoMachine,
            sync_json: String,
        ) -> Box<SyncChangesResult>;
        fn receive_verification_event(self: &mut CryptoMachine, full_json: String);

        // Mark requests as sent
        fn mark_keys_upload_as_sent(self: &mut CryptoMachine, request_id: String, response: String);
        fn mark_keys_query_as_sent(self: &mut CryptoMachine, request_id: String, response: String);
        fn mark_keys_claim_as_sent(self: &mut CryptoMachine, request_id: String, response: String);
        fn mark_to_device_as_sent(self: &mut CryptoMachine, request_id: String, response: String);
        fn mark_room_message_as_sent(
            self: &mut CryptoMachine,
            request_id: String,
            response: String,
        );
        fn mark_signature_upload_as_sent(
            self: &mut CryptoMachine,
            request_id: String,
            response: String,
        );
        fn mark_keys_backup_as_sent(self: &mut CryptoMachine, response: String, request_id: String);

        // Specific CryptoMachine functions
        fn share_room_key(
            self: &mut CryptoMachine,
            room_id: String,
            user_ids: Vec<String>,
            only_trusted: bool,
            visibility: u8,
        ) -> Box<ToDeviceRequestResult>;
        fn get_missing_sessions(
            self: &mut CryptoMachine,
            user_ids: Vec<String>,
        ) -> Box<MissingSessionsResult>;
        fn encrypt_room_event(
            self: &mut CryptoMachine,
            room_id: String,
            content: String,
            matrix_type: String,
        ) -> Box<StringResult>;
        fn decrypt_room_event(
            self: &mut CryptoMachine,
            room_id: String,
            json: String,
        ) -> Box<StringResult>;
        fn update_tracked_users(self: &mut CryptoMachine, user_ids: Vec<String>);
        fn request_device_verification(
            self: &mut CryptoMachine,
            user_id: String,
            device_id: String,
        ) -> Box<CreatedSessionResult>;
        fn request_user_verification(
            self: &mut CryptoMachine,
            user_id: String,
            room_id: String,
            event_id: String,
        ) -> Box<KeyVerificationRequestResult>;
        fn request_user_verification_content(
            self: &mut CryptoMachine,
            user_id: String,
        ) -> Box<StringResult>;
        fn get_room_event_encryption_info(
            self: &CryptoMachine,
            event: String,
            room_id: String,
        ) -> Box<EncryptionInfoResult>;
        fn inbound_from_libolm_pickle(
            self: &CryptoMachine,
            session_data: Vec<MegolmSessionData>,
            pickle_key: String,
        );

        // Gettings parts of an OutgoingRequest
        fn id(self: &OutgoingRequest) -> String;
        fn keys_upload_device_keys(self: &OutgoingRequest) -> String;
        fn keys_upload_one_time_keys(self: &OutgoingRequest) -> String;
        fn keys_upload_fallback_keys(self: &OutgoingRequest) -> String;
        fn keys_query_device_keys(self: &OutgoingRequest) -> String;
        fn keys_query_timeout(self: &OutgoingRequest) -> usize;
        fn keys_claim_timeout(self: &OutgoingRequest) -> usize;
        fn keys_claim_one_time_keys(self: &OutgoingRequest) -> String;
        fn to_device_event_type(self: &OutgoingRequest) -> String;
        fn to_device_messages(self: &OutgoingRequest) -> String;
        fn upload_signature_signed_keys(self: &OutgoingRequest) -> String;
        fn request_type(self: &OutgoingRequest) -> OutgoingRequestType;
        fn room_msg_content(self: &OutgoingRequest) -> String;
        fn room_msg_room_id(self: &OutgoingRequest) -> String;
        fn room_msg_matrix_type(self: &OutgoingRequest) -> String;
        fn to_device_txn_id(self: &OutgoingRequest) -> String;
        fn room_msg_txn_id(self: &OutgoingRequest) -> String;

        fn timeout(self: &KeysClaimRequest) -> usize;
        fn id(self: &KeysClaimRequest) -> String;
        fn one_time_keys(self: &KeysClaimRequest) -> String;

        fn event_type(self: &ToDeviceRequest) -> String;
        fn txn_id(self: &ToDeviceRequest) -> String;
        fn messages(self: &ToDeviceRequest) -> String;

        fn accept_verification(
            self: &mut CryptoMachine,
            remote_user: String,
            verification_id: String,
        ) -> Box<OutgoingKeyVerificationRequestResult>;
        fn confirm_verification(
            self: &mut CryptoMachine,
            remote_user: String,
            verification_id: String,
        ) -> Box<ConfirmRequestsResult>;
        fn start_sas(
            self: &mut CryptoMachine,
            remote_user: String,
            verification_id: String,
        ) -> Box<OutgoingKeyVerificationRequestResult>;
        fn accept_sas(
            self: &mut CryptoMachine,
            remote_user: String,
            verification_id: String,
        ) -> Box<OutgoingKeyVerificationRequestResult>;
        fn cancel_verification(
            self: &mut CryptoMachine,
            remote_user: String,
            verification_id: String,
        ) -> Box<OutgoingKeyVerificationRequestResult>;
        fn verification_get_state(
            self: &mut CryptoMachine,
            remote_user: String,
            verification_id: String,
        ) -> Box<U8Result>;
        fn sas_get_state(
            self: &mut CryptoMachine,
            remote_user: String,
            verification_id: String,
        ) -> Box<U8Result>;
        fn remote_user_id(self: &KeyVerificationRequest) -> String;
        fn remote_device_id(self: &KeyVerificationRequest) -> String;
        fn verification_id(self: &KeyVerificationRequest) -> String;

        fn to_device_event_type(self: &OutgoingKeyVerificationRequest) -> String;
        fn to_device_messages(self: &OutgoingKeyVerificationRequest) -> String;
        fn in_room_room_id(self: &OutgoingKeyVerificationRequest) -> String;
        fn to_device_txn_id(self: &OutgoingKeyVerificationRequest) -> String;
        fn in_room_txn_id(self: &OutgoingKeyVerificationRequest) -> String;
        fn in_room_content(self: &OutgoingKeyVerificationRequest) -> String;
        fn in_room_event_type(self: &OutgoingKeyVerificationRequest) -> String;

        fn sas_emoji(
            self: &CryptoMachine,
            remote_user: String,
            verification_id: String,
        ) -> Box<EmojiResult>;
        fn symbol(self: &Emoji) -> String;
        fn description(self: &Emoji) -> String;

        fn to_device_event_type(self: &CreatedSession) -> String;
        fn to_device_txn_id(self: &CreatedSession) -> String;
        fn to_device_messages(self: &CreatedSession) -> String;
        fn verification_id(self: &CreatedSession) -> String;

        fn is_verified(self: &EncryptionInfo) -> bool;

        fn encrypt_file(bytes: &mut [u8]) -> Box<MediaEncryptionInfo>;
        fn media_encryption_info_bytes(self: &MediaEncryptionInfo) -> Vec<u8>;
        fn media_encryption_info_iv(self: &MediaEncryptionInfo) -> String;
        fn media_encryption_info_key(self: &MediaEncryptionInfo) -> String;
        fn media_encryption_info_hash(self: &MediaEncryptionInfo) -> String;

        fn decrypt_file(
            bytes: &mut [u8],
            iv: String,
            key: String,
            hash: String,
        ) -> Box<DecryptResult>;

        fn import_from_backup(self: &mut CryptoMachine, response: String, version: String) -> Vec<Key>;
        fn migrate_secrets(
            self: &mut CryptoMachine,
            master_key: String,
            self_signing_key: String,
            user_signing_key: String,
            backup_key: String,
            version: String,
        );

        fn load_secrets(
            self: &mut CryptoMachine,
            passphrase: String,
            key_id: String,
            iterations: u64,
            salt: String,
            iv: String,
            mac: String,
            backup_key_iv: String,
            backup_key_cipher: String,
            backup_key_mac: String,
            master_key_iv: String,
            master_key_cipher: String,
            master_key_mac: String,
            self_key_iv: String,
            self_key_cipher: String,
            self_key_mac: String,
            user_key_iv: String,
            user_key_cipher: String,
            user_key_mac: String,
            version: String,
        ) -> Box<StringResult>;

        fn verification_requests(self: &ConfirmRequests) -> Vec<OutgoingKeyVerificationRequest>;
        fn has_signature_request(self: &ConfirmRequests) -> bool;
        fn signature_request_content(self: &ConfirmRequests) -> String;
        fn request_self_verification(self: &mut CryptoMachine) -> Box<CreatedSessionResult>;
        fn request_secrets_from_devices(self: &mut CryptoMachine);

        fn has_pending_backup_key(self: &CryptoMachine) -> bool;
        fn has_initialized_backup(self: &CryptoMachine) -> bool;
        fn initialize_existing_backup(self: &mut CryptoMachine, version_reply: String);
        fn backup_keys(self: &mut CryptoMachine) -> Box<BackupRequestResult>;

        fn has_request(self: &BackupRequest) -> bool;
        fn rooms(self: &BackupRequest) -> String;
        fn transaction_id(self: &BackupRequest) -> String;
        fn version(self: &BackupRequest) -> String;

        fn sessions(self: &SyncChanges) -> Vec<KeyVerificationRequest>;
        fn keys(self: &SyncChanges) -> Vec<Key>;
        fn secrets_received(self: &SyncChanges) -> bool;
        fn self_verified(self: &SyncChanges) -> bool;
        fn received_done_events(self: &SyncChanges) -> Vec<String>;

        fn session_id(self: &Key) -> String;
        fn room_id(self: &Key) -> String;

        fn export_keys(self: &CryptoMachine, passphrase: String) -> Box<StringResult>;
        fn import_keys(
            self: &CryptoMachine,
            passphrase: String,
            ciphertext: String,
        ) -> Box<KeyResult>;
        fn all_sessions_verified(self: &CryptoMachine, user_id: String) -> bool;
        fn is_user_verified(self: &CryptoMachine, user_id: String) -> bool;
        fn is_e2ee_device(self: &CryptoMachine, user_id: String, device_id: String) -> bool;
        fn is_verified_device(self: &CryptoMachine, user_id: String, device_id: String) -> bool;

        fn is_ok(self: &CryptoMachine) -> bool;
        fn error(self: &CryptoMachine) -> InitError;
        fn error_string(self: &CryptoMachine) -> String;

        fn has_error(self: &SyncChangesResult) -> bool;
        fn error_string(self: &SyncChangesResult) -> String;
        fn value(self: &SyncChangesResult) -> Box<SyncChanges>;

        fn has_error(self: &MissingSessionsResult) -> bool;
        fn has_request(self: &MissingSessionsResult) -> bool;
        fn error_string(self: &MissingSessionsResult) -> String;
        fn request(self: &MissingSessionsResult) -> Box<KeysClaimRequest>;

        fn has_error(self: &ConfirmRequestsResult) -> bool;
        fn error_string(self: &ConfirmRequestsResult) -> String;
        fn value(self: &ConfirmRequestsResult) -> Box<ConfirmRequests>;

        fn has_error(self: &ToDeviceRequestResult) -> bool;
        fn error_string(self: &ToDeviceRequestResult) -> String;
        fn value(self: &ToDeviceRequestResult) -> Vec<ToDeviceRequest>;

        fn has_error(self: &OutgoingKeyVerificationRequestResult) -> bool;
        fn error_string(self: &OutgoingKeyVerificationRequestResult) -> String;
        fn value(
            self: &OutgoingKeyVerificationRequestResult,
        ) -> Box<OutgoingKeyVerificationRequest>;

        fn has_error(self: &StringResult) -> bool;
        fn error_string(self: &StringResult) -> String;
        fn value(self: &StringResult) -> String;
        fn is_invalid_passphrase(self: &StringResult) -> bool;

        fn has_error(self: &U8Result) -> bool;
        fn error_string(self: &U8Result) -> String;
        fn value(self: &U8Result) -> u8;

        fn has_error(self: &OutgoingRequestResult) -> bool;
        fn error_string(self: &OutgoingRequestResult) -> String;
        fn value(self: &OutgoingRequestResult) -> Vec<OutgoingRequest>;

        fn has_error(self: &EmojiResult) -> bool;
        fn error_string(self: &EmojiResult) -> String;
        fn value(self: &EmojiResult) -> Vec<Emoji>;

        fn has_error(self: &KeyResult) -> bool;
        fn error_string(self: &KeyResult) -> String;
        fn error_code(self: &KeyResult) -> u8;
        fn value(self: &KeyResult) -> Vec<Key>;

        fn has_error(self: &BackupRequestResult) -> bool;
        fn has_request(self: &BackupRequestResult) -> bool;
        fn error_string(self: &BackupRequestResult) -> String;
        fn value(self: &BackupRequestResult) -> Box<BackupRequest>;

        fn has_error(self: &CreatedSessionResult) -> bool;
        fn error_string(self: &CreatedSessionResult) -> String;
        fn value(self: &CreatedSessionResult) -> Box<CreatedSession>;

        fn has_error(self: &KeyVerificationRequestResult) -> bool;
        fn error_string(self: &KeyVerificationRequestResult) -> String;
        fn value(self: &KeyVerificationRequestResult) -> Box<KeyVerificationRequest>;

        fn has_error(self: &EncryptionInfoResult) -> bool;
        fn error_string(self: &EncryptionInfoResult) -> String;
        fn value(self: &EncryptionInfoResult) -> Box<EncryptionInfo>;

        fn has_error(self: &DecryptResult) -> bool;
        fn error_string(self: &DecryptResult) -> String;
        fn value(self: &DecryptResult) -> Vec<u8>;

        fn import_olm_sessions(
            self: &CryptoMachine,
            sessions_data: Vec<OlmSessionData>,
            pickle_key: String,
        );
        fn monitor_verification(
            self: &mut CryptoMachine,
            remote_user: String,
            verification_id: String,
            callback: usize,
        );
        fn monitor_sas(
            self: &mut CryptoMachine,
            remote_user: String,
            verification_id: String,
            callback: usize,
        );

        fn all_private_cs_keys_available(self: &CryptoMachine) -> bool;
        fn requires_cs_bootstrap(self: &mut CryptoMachine) -> bool;
        fn bootstrap_cs(self: &mut CryptoMachine) -> Box<CrossSigningBootstrapRequestsResult>;

        fn has_error(self: &CrossSigningBootstrapRequestsResult) -> bool;
        fn value(self: &CrossSigningBootstrapRequestsResult) -> Box<CrossSigningBootstrapRequests>;

        fn upload_signatures_content(self: &CrossSigningBootstrapRequests) -> String;
        fn self_signing_key_json(self: &CrossSigningBootstrapRequests) -> String;
        fn user_signing_key_json(self: &CrossSigningBootstrapRequests) -> String;
        fn master_key_json(self: &CrossSigningBootstrapRequests) -> String;
        fn has_upload_keys_request(self: &CrossSigningBootstrapRequests) -> bool;
        fn upload_keys_request(self: &CrossSigningBootstrapRequests) -> Box<OutgoingRequest>;

        pub(crate) fn secret_storage_key_id(self: &CrossSigningBootstrapRequests) -> String;
        pub(crate) fn master(self: &CrossSigningBootstrapRequests) -> String;
        pub(crate) fn self_signing(self: &CrossSigningBootstrapRequests) -> String;
        pub(crate) fn user_signing(self: &CrossSigningBootstrapRequests) -> String;
        pub(crate) fn backup(self: &CrossSigningBootstrapRequests) -> String;

        pub(crate) fn backup_key(self: &CrossSigningBootstrapRequests) -> String;
        pub(crate) fn secret_storage_event_content(self: &CrossSigningBootstrapRequests) -> String;

        pub(crate) fn secret_storage_event_type(self: &CrossSigningBootstrapRequests) -> String;

        pub(crate) fn backup_info(self: &CrossSigningBootstrapRequests) -> String;

        pub(crate) fn set_backup_version(self: &mut CryptoMachine, version: String);
    }
}
