// SPDX-FileCopyrightText: 2026 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

use crate::{
    encryption_info::EncryptionInfo,
    ffi::InitError,
    request::{
        ConfirmRequests, KeyVerificationRequest, KeysClaimRequest, OutgoingKeyVerificationRequest,
        ToDeviceRequest,
    },
    verification::CreatedSession,
    verification::Emoji,
};
use log::error;
use matrix_sdk_common::stream::StreamExt;
use matrix_sdk_common::{
    deserialized_responses::ProcessedToDeviceEvent,
    ruma::{
        DeviceId, DeviceKeyAlgorithm, DeviceKeyId, EventId, OneTimeKeyAlgorithm, OwnedDeviceId,
        OwnedTransactionId, OwnedUserId, RoomId, SecondsSinceUnixEpoch, UInt, UserId,
        api::client::{
            backup::{RoomKeyBackup, add_backup_keys},
            keys::{
                claim_keys::{self, v3::OneTimeKeys},
                get_keys, upload_keys,
                upload_signatures::{self, v3::Failure},
            },
            message::send_message_event,
            sync::sync_events::DeviceLists,
            to_device,
        },
        encryption::{CrossSigningKey, DeviceKeys},
        events::room::history_visibility::HistoryVisibility,
        events::{
            AnyToDeviceEvent,
            secret::request::SecretName,
            secret_storage::key::{
                PassPhrase, SecretStorageEncryptionAlgorithm, SecretStorageKeyEventContent,
                SecretStorageV1AesHmacSha2Properties,
            },
        },
        serde::Base64,
        serde::{Raw, base64::Standard},
    },
};
use matrix_sdk_crypto::{
    CrossSigningKeyExport, DecryptionSettings, EncryptionSettings, EncryptionSyncChanges,
    OlmMachine, SasState, TrustRequirement, UserIdentity, Verification, VerificationRequestState,
    olm,
    olm::ExportedRoomKey,
    secret_storage::{AesHmacSha2EncryptedData, SecretStorageKey},
    store::types::{BackupDecryptionKey, Changes},
    types::requests::{AnyIncomingResponse, KeysBackupRequest},
    types::{DeviceKey, EventEncryptionAlgorithm, RoomKeyBackupInfo},
};
use serde::Deserialize;
use serde_json::{Value, Value::Object};
use std::{collections::BTreeMap, error::Error, mem::ManuallyDrop, ops::Deref};
use matrix_sdk_crypto::vodozemac::Curve25519PublicKey;
use matrix_sdk_crypto::vodozemac::megolm::InboundGroupSession;

pub(crate) struct CryptoMachine {
    pub(crate) error: InitError,
    pub(crate) error_string: String,
    pub(crate) runtime: tokio::runtime::Runtime,
    pub(crate) machine: Option<ManuallyDrop<OlmMachine>>,
    pub(crate) pending_backup_decryption_key: Option<BackupDecryptionKey>,
}

impl Drop for CryptoMachine {
    fn drop(&mut self) {
        self.runtime.block_on(async {
            let machine = self
                .machine
                .take()
                .expect("CryptoMachine should not be destroyed more than once");
            drop(ManuallyDrop::into_inner(machine));
        })
    }
}

#[derive(Clone, Debug)]
pub(crate) struct Key {
    session_id: String,
    room_id: String,
}

impl Key {
    pub(crate) fn session_id(&self) -> String {
        self.session_id.clone()
    }

    pub(crate) fn room_id(&self) -> String {
        self.room_id.clone()
    }
}

#[derive(Clone)]
pub(crate) struct SyncChanges {
    sessions: Vec<KeyVerificationRequest>,
    keys: Vec<Key>,
    secrets_received: bool,
    self_verified: bool,
    received_done_events: Vec<String>,
}

pub(crate) enum SyncChangesResult {
    Ok(Box<SyncChanges>),
    Err(Box<dyn Error>),
}

impl SyncChangesResult {
    pub(crate) fn has_error(&self) -> bool {
        matches!(self, Self::Err(_))
    }

    pub(crate) fn error_string(&self) -> String {
        if let Self::Err(error) = self {
            error.to_string()
        } else {
            Default::default()
        }
    }

    pub(crate) fn value(&self) -> Box<SyncChanges> {
        match self {
            Self::Ok(value) => value.clone(),
            Self::Err(_) => panic!(),
        }
    }
}

impl SyncChanges {
    pub(crate) fn sessions(&self) -> Vec<KeyVerificationRequest> {
        self.sessions.clone()
    }

    pub(crate) fn keys(&self) -> Vec<Key> {
        self.keys.clone()
    }

    pub(crate) fn secrets_received(&self) -> bool {
        self.secrets_received
    }

    pub(crate) fn self_verified(&self) -> bool { self.self_verified }

    pub(crate) fn received_done_events(&self) -> Vec<String> {
        self.received_done_events.clone()
    }
}

pub(crate) enum OutgoingRequestResult {
    Ok(Vec<crate::request::OutgoingRequest>),
    Err(Box<dyn Error>),
}

impl OutgoingRequestResult {
    pub(crate) fn has_error(&self) -> bool {
        matches!(self, Self::Err(_))
    }

    pub(crate) fn error_string(&self) -> String {
        if let Self::Err(error) = self {
            error.to_string()
        } else {
            Default::default()
        }
    }

    pub(crate) fn value(&self) -> Vec<crate::request::OutgoingRequest> {
        match self {
            Self::Ok(value) => value.clone(),
            Self::Err(_) => panic!(),
        }
    }
}

pub(crate) enum CrossSigningBootstrapRequestsResult {
    Ok(Box<crate::request::CrossSigningBootstrapRequests>),
    Err(Box<dyn Error>)
}

impl CrossSigningBootstrapRequestsResult {
    pub(crate) fn has_error(&self) -> bool {
        matches!(self, Self::Err(_))
    }

    pub(crate) fn error_string(&self) -> String {
        if let Self::Err(error) = self {
            error.to_string()
        } else {
            Default::default()
        }
    }

    pub(crate) fn value(&self) -> Box<crate::request::CrossSigningBootstrapRequests> {
        match self {
            Self::Ok(value) => value.clone(),
            Self::Err(_) => panic!("CrossSigningBootstrapRequests does not have a value"),
        }
    }
}

pub(crate) enum MissingSessionsResult {
    Request(Box<KeysClaimRequest>),
    NoRequest,
    Err(Box<dyn Error>),
}

impl MissingSessionsResult {
    pub(crate) fn has_error(&self) -> bool {
        matches!(self, Self::Err(_))
    }

    pub(crate) fn error_string(&self) -> String {
        if let Self::Err(error) = self {
            error.to_string()
        } else {
            Default::default()
        }
    }

    pub(crate) fn has_request(&self) -> bool {
        matches!(self, Self::Request(_))
    }

    pub(crate) fn request(&self) -> Box<KeysClaimRequest> {
        if let Self::Request(request) = self {
            request.clone()
        } else {
            panic!()
        }
    }
}

pub(crate) enum ConfirmRequestsResult {
    Ok(Box<ConfirmRequests>),
    Err(Box<dyn Error>),
}

impl ConfirmRequestsResult {
    pub(crate) fn has_error(&self) -> bool {
        matches!(self, Self::Err(_))
    }

    pub(crate) fn error_string(&self) -> String {
        if let Self::Err(error) = self {
            error.to_string()
        } else {
            Default::default()
        }
    }

    pub(crate) fn value(&self) -> Box<ConfirmRequests> {
        if let Self::Ok(requests) = self {
            requests.clone()
        } else {
            panic!()
        }
    }
}

pub(crate) enum OutgoingKeyVerificationRequestResult {
    Ok(Box<OutgoingKeyVerificationRequest>),
    Err(Box<dyn Error>),
}

impl OutgoingKeyVerificationRequestResult {
    pub(crate) fn has_error(&self) -> bool {
        matches!(self, Self::Err(_))
    }

    pub(crate) fn error_string(&self) -> String {
        if let Self::Err(error) = self {
            error.to_string()
        } else {
            Default::default()
        }
    }

    pub(crate) fn value(&self) -> Box<OutgoingKeyVerificationRequest> {
        match self {
            Self::Ok(value) => value.clone(),
            Self::Err(_) => panic!(),
        }
    }
}

pub(crate) enum ToDeviceRequestResult {
    Ok(Vec<ToDeviceRequest>),
    Err(Box<dyn Error>),
}

impl ToDeviceRequestResult {
    pub(crate) fn has_error(&self) -> bool {
        matches!(self, Self::Err(_))
    }

    pub(crate) fn error_string(&self) -> String {
        if let Self::Err(error) = self {
            error.to_string()
        } else {
            Default::default()
        }
    }

    pub(crate) fn value(&self) -> Vec<ToDeviceRequest> {
        match self {
            Self::Ok(value) => value.clone(),
            Self::Err(_) => panic!(),
        }
    }
}

pub(crate) enum StringResult {
    Ok(String),
    Err(Box<dyn Error>),
}

impl StringResult {
    pub(crate) fn has_error(&self) -> bool {
        matches!(self, Self::Err(_))
    }

    pub(crate) fn error_string(&self) -> String {
        if let Self::Err(error) = self {
            error.to_string()
        } else {
            Default::default()
        }
    }

    pub(crate) fn value(&self) -> String {
        if let Self::Ok(value) = self {
            value.clone()
        } else {
            Default::default()
        }
    }

    pub(crate) fn is_invalid_passphrase(&self) -> bool {
        if let Self::Err(error) = self {
            error
                .downcast_ref::<matrix_sdk_crypto::secret_storage::DecodeError>()
                .is_some()
        } else {
            false
        }
    }
}

pub(crate) enum U8Result {
    Ok(u8),
    Err(Box<dyn Error>),
}

impl U8Result {
    pub(crate) fn has_error(&self) -> bool {
        matches!(self, Self::Err(_))
    }

    pub(crate) fn error_string(&self) -> String {
        if let Self::Err(error) = self {
            error.to_string()
        } else {
            Default::default()
        }
    }

    pub(crate) fn value(&self) -> u8 {
        if let Self::Ok(value) = self {
            *value
        } else {
            Default::default()
        }
    }
}

pub(crate) enum EmojiResult {
    Ok(Vec<Emoji>),
    Err(Box<dyn Error>),
}

impl EmojiResult {
    pub(crate) fn has_error(&self) -> bool {
        matches!(self, Self::Err(_))
    }

    pub(crate) fn error_string(&self) -> String {
        if let Self::Err(error) = self {
            error.to_string()
        } else {
            Default::default()
        }
    }

    pub(crate) fn value(&self) -> Vec<Emoji> {
        if let Self::Ok(value) = self {
            value.clone()
        } else {
            panic!()
        }
    }
}

pub(crate) enum KeyResult {
    Ok(Vec<Key>),
    Err(Box<dyn Error>),
}

impl KeyResult {
    pub(crate) fn has_error(&self) -> bool {
        matches!(self, Self::Err(_))
    }

    pub(crate) fn error_string(&self) -> String {
        if let Self::Err(error) = self {
            error.to_string()
        } else {
            Default::default()
        }
    }

    pub(crate) fn error_code(&self) -> u8 {
        if let Self::Err(error) = self {
            use matrix_sdk_crypto::KeyExportError;
            if let Some(error) = error.downcast_ref::<KeyExportError>() {
                match error {
                    KeyExportError::InvalidMac => 1,
                    _ => 2,
                }
            } else {
                3
            }
        } else {
            0
        }
    }

    pub(crate) fn value(&self) -> Vec<Key> {
        if let Self::Ok(value) = self {
            value.clone()
        } else {
            panic!()
        }
    }
}

pub(crate) enum BackupRequestResult {
    NoRequest,
    Ok(Box<BackupRequest>),
    Err(Box<dyn Error>),
}

impl BackupRequestResult {
    pub(crate) fn has_error(&self) -> bool {
        matches!(self, Self::Err(_))
    }

    pub(crate) fn has_request(&self) -> bool {
        matches!(self, Self::Ok(_))
    }

    pub(crate) fn error_string(&self) -> String {
        if let Self::Err(error) = self {
            error.to_string()
        } else {
            Default::default()
        }
    }

    pub(crate) fn value(&self) -> Box<BackupRequest> {
        if let Self::Ok(value) = self {
            value.clone()
        } else {
            panic!()
        }
    }
}

pub(crate) enum CreatedSessionResult {
    Ok(Box<CreatedSession>),
    Err(Box<dyn Error>),
}

impl CreatedSessionResult {
    pub(crate) fn has_error(&self) -> bool {
        matches!(self, Self::Err(_))
    }

    pub(crate) fn error_string(&self) -> String {
        if let Self::Err(error) = self {
            error.to_string()
        } else {
            Default::default()
        }
    }

    pub(crate) fn value(&self) -> Box<CreatedSession> {
        if let Self::Ok(value) = self {
            value.clone()
        } else {
            panic!()
        }
    }
}

pub(crate) enum KeyVerificationRequestResult {
    Ok(Box<KeyVerificationRequest>),
    Err(Box<dyn Error>),
}

impl KeyVerificationRequestResult {
    pub(crate) fn has_error(&self) -> bool {
        matches!(self, Self::Err(_))
    }

    pub(crate) fn error_string(&self) -> String {
        if let Self::Err(error) = self {
            error.to_string()
        } else {
            Default::default()
        }
    }

    pub(crate) fn value(&self) -> Box<KeyVerificationRequest> {
        if let Self::Ok(value) = self {
            value.clone()
        } else {
            panic!()
        }
    }
}

pub(crate) enum EncryptionInfoResult {
    Ok(Box<EncryptionInfo>),
    Err(Box<dyn Error>),
}

impl EncryptionInfoResult {
    pub(crate) fn has_error(&self) -> bool {
        matches!(self, Self::Err(_))
    }

    pub(crate) fn error_string(&self) -> String {
        if let Self::Err(error) = self {
            error.to_string()
        } else {
            Default::default()
        }
    }

    pub(crate) fn value(&self) -> Box<EncryptionInfo> {
        if let Self::Ok(value) = self {
            value.clone()
        } else {
            panic!()
        }
    }
}

macro_rules! crypto_machine {
    ($machine:ident) => {
        $machine
            .machine
            .as_ref()
            .expect("The crypto machine must be initialized")
    };
}

impl CryptoMachine {
    pub(crate) fn requires_cs_bootstrap(&mut self) -> bool {
        self.runtime.block_on(async {
            let machine = crypto_machine!(self);
            !machine
                .get_identity(machine.user_id(), None)
                .await.unwrap()
                .is_some()
        })
    }

    pub(crate) fn set_backup_version(&mut self, version: String) {
        let pending_backup = self.pending_backup_decryption_key.take();
        self.runtime.block_on(async {
            crypto_machine!(self).backup_machine().save_decryption_key(pending_backup, Some(version)).await.unwrap();
        });
    }

    pub(crate) fn bootstrap_cs(&mut self) -> Box<CrossSigningBootstrapRequestsResult> {
        let result: Result<Box<crate::request::CrossSigningBootstrapRequests>, Box<dyn Error>> =
        self.runtime.block_on(async {
            let requests = crypto_machine!(self).bootstrap_cross_signing(false).await?;
            let secret_storage = SecretStorageKey::new();
            let backup_decryption_key = BackupDecryptionKey::new().unwrap();

            let mut backup_info = backup_decryption_key.to_backup_info();
            crypto_machine!(self).backup_machine().sign_backup(&mut backup_info).await?;
            crypto_machine!(self).backup_machine().enable_backup_v1(backup_decryption_key.megolm_v1_public_key()).await.unwrap();
            self.pending_backup_decryption_key = Some(backup_decryption_key.clone());

            #[derive(serde::Serialize)]
            struct AesEncrypted {
                iv: String,
                ciphertext: String,
                mac: String,
            }

            impl AesEncrypted {
                fn new(encrypted: AesHmacSha2EncryptedData) -> Self {
                    AesEncrypted {
                        iv: Base64::<Standard>::new(encrypted.iv.into()).encode(),
                        ciphertext: encrypted.ciphertext.to_string(),
                        mac: Base64::<Standard>::new(encrypted.mac.into()).encode(),
                    }
                }
            }

            let cs_keys = crypto_machine!(self).export_cross_signing_keys().await?.unwrap();
            let master = secret_storage.encrypt(cs_keys.master_key.as_ref().unwrap().clone().into_bytes(), &SecretName::CrossSigningMasterKey);
            let user_signing = secret_storage.encrypt(cs_keys.user_signing_key.clone().unwrap().into_bytes(), &SecretName::CrossSigningUserSigningKey);
            let self_signing = secret_storage.encrypt(cs_keys.self_signing_key.clone().unwrap().into_bytes(), &SecretName::CrossSigningSelfSigningKey);
            let backup = secret_storage.encrypt(backup_decryption_key.to_base64().into_bytes(), &SecretName::RecoveryKey);
            Ok(Box::new(crate::request::CrossSigningBootstrapRequests {
                bootstrap_requests: requests,
                secret_storage_key_id: secret_storage.key_id().to_string(),
                backup_info: serde_json::to_string(&backup_info).unwrap(),
                master: serde_json::to_string(&AesEncrypted::new(master)).unwrap(),
                self_signing: serde_json::to_string(&AesEncrypted::new(self_signing)).unwrap(),
                user_signing: serde_json::to_string(&AesEncrypted::new(user_signing)).unwrap(),
                backup: serde_json::to_string(&AesEncrypted::new(backup)).unwrap(),
                backup_key: secret_storage.to_base58(),
                secret_storage_event_content: serde_json::to_string(&secret_storage.event_content()).unwrap(),
                secret_storage_event_type: secret_storage.event_type().to_string(),
            }))
        });
        Box::new(match result {
            Ok(requests) => CrossSigningBootstrapRequestsResult::Ok(requests),
            Err(error) => {
                error!("Failed to process bootstrap_cs: {:?}", error);
                CrossSigningBootstrapRequestsResult::Err(error)
            }
        })
    }
    pub(crate) fn is_ok(&self) -> bool {
        self.error == InitError::Ok
    }
    pub(crate) fn error(&self) -> InitError {
        self.error.clone()
    }
    pub(crate) fn error_string(&self) -> String {
        self.error_string.clone()
    }

    pub(crate) fn outgoing_requests(&self) -> Box<OutgoingRequestResult> {
        let result: Result<Vec<crate::request::OutgoingRequest>, Box<dyn Error>> =
            self.runtime.block_on(async {
                Ok(crypto_machine!(self)
                    .outgoing_requests()
                    .await?
                    .iter()
                    .map(|it| crate::request::OutgoingRequest(it.clone()))
                    .collect())
            });
        Box::new(match result {
            Ok(requests) => OutgoingRequestResult::Ok(requests),
            Err(error) => {
                error!("Failed to process outgoing_requests: {:?}", error);
                OutgoingRequestResult::Err(error)
            }
        })
    }

    pub(crate) fn mark_keys_upload_as_sent(&mut self, response: String, request_id: String) {
        let result: Result<(), Box<dyn Error>> = self.runtime.block_on(async {
            #[derive(serde::Deserialize)]
            struct Response {
                one_time_key_counts: BTreeMap<OneTimeKeyAlgorithm, UInt>,
            }

            let response = serde_json::from_str::<Response>(response.as_str())?;

            Ok(crypto_machine!(self)
                .mark_request_as_sent(
                    request_id.as_str().into(),
                    AnyIncomingResponse::KeysUpload(&upload_keys::v3::Response::new(
                        response.one_time_key_counts,
                    )),
                )
                .await?)
        });
        if let Err(error) = result {
            error!("Failed to process mark_keys_upload_as_sent: {:?}", error);
        }
    }

    pub(crate) fn mark_keys_query_as_sent(&mut self, response: String, request_id: String) {
        let result: Result<(), Box<dyn Error>> = self.runtime.block_on(async {
            #[derive(Deserialize)]
            struct Response {
                failures: Option<BTreeMap<String, Value>>,
                device_keys:
                    Option<BTreeMap<OwnedUserId, BTreeMap<OwnedDeviceId, Raw<DeviceKeys>>>>,
                master_keys: Option<BTreeMap<OwnedUserId, Raw<CrossSigningKey>>>,
                self_signing_keys: Option<BTreeMap<OwnedUserId, Raw<CrossSigningKey>>>,
                user_signing_keys: Option<BTreeMap<OwnedUserId, Raw<CrossSigningKey>>>,
            }
            let r = serde_json::from_str::<Response>(response.as_str())?;
            let mut response = get_keys::v3::Response::new();
            response.failures = r.failures.unwrap_or_default();
            response.device_keys = r.device_keys.unwrap_or_default();
            response.master_keys = r.master_keys.unwrap_or_default();
            response.self_signing_keys = r.self_signing_keys.unwrap_or_default();
            response.user_signing_keys = r.user_signing_keys.unwrap_or_default();
            crypto_machine!(self)
                .mark_request_as_sent(
                    request_id.as_str().into(),
                    AnyIncomingResponse::KeysQuery(&response),
                )
                .await?;
            Ok(())
        });
        if let Err(error) = result {
            error!("Failed to process mark_keys_query_as_sent: {:?}", error);
        };
    }

    pub(crate) fn mark_keys_claim_as_sent(&mut self, response: String, request_id: String) {
        let result: Result<(), Box<dyn Error>> = self.runtime.block_on(async {
            #[derive(Deserialize)]
            struct Response {
                failures: Option<BTreeMap<String, Value>>,
                one_time_keys: Option<BTreeMap<OwnedUserId, OneTimeKeys>>,
            }
            let r = serde_json::from_str::<Response>(response.as_str())?;
            let mut request = claim_keys::v3::Response::new(r.one_time_keys.unwrap_or_default());
            request.failures = r.failures.unwrap_or_default();
            crypto_machine!(self)
                .mark_request_as_sent(
                    request_id.as_str().into(),
                    AnyIncomingResponse::KeysClaim(&request),
                )
                .await?;
            Ok(())
        });
        if let Err(error) = result {
            error!("Failed to process mark_keys_claim_as_sent: {:?}", error);
        };
    }

    pub(crate) fn mark_to_device_as_sent(&mut self, _: String, request_id: String) {
        let result: Result<(), Box<dyn Error>> = self.runtime.block_on(async {
            crypto_machine!(self)
                .mark_request_as_sent(
                    request_id.as_str().into(),
                    AnyIncomingResponse::ToDevice(
                        &to_device::send_event_to_device::v3::Response::new(),
                    ),
                )
                .await?;
            Ok(())
        });
        if let Err(error) = result {
            error!("Failed to process mark_to_device_as_sent: {:?}", error);
        };
    }

    pub(crate) fn mark_signature_upload_as_sent(&mut self, response: String, request_id: String) {
        let result: Result<(), Box<dyn Error>> = self.runtime.block_on(async {
            #[derive(Deserialize)]
            struct Response {
                failures: Option<BTreeMap<OwnedUserId, BTreeMap<String, Failure>>>,
            }
            let r = serde_json::from_str::<Response>(response.as_str())?;
            let mut request = upload_signatures::v3::Response::new();
            request.failures = r.failures.unwrap_or_default();
            crypto_machine!(self)
                .mark_request_as_sent(
                    request_id.as_str().into(),
                    AnyIncomingResponse::SignatureUpload(&request),
                )
                .await?;
            Ok(())
        });
        if let Err(error) = result {
            error!(
                "Failed to process mark_signature_upload_as_sent: {:?}",
                error
            );
        };
    }

    pub(crate) fn mark_room_message_as_sent(&mut self, event_id: String, request_id: String) {
        let result: Result<(), Box<dyn Error>> = self.runtime.block_on(async {
            let event_id = EventId::parse(&event_id)?;
            crypto_machine!(self)
                .mark_request_as_sent(
                    request_id.as_str().into(),
                    AnyIncomingResponse::RoomMessage(&send_message_event::v3::Response::new(
                        event_id,
                    )),
                )
                .await?;
            Ok(())
        });
        if let Err(error) = result {
            error!("Failed to process mark_room_message_as_sent: {:?}", error);
        };
    }

    pub(crate) fn mark_keys_backup_as_sent(&mut self, response: String, request_id: String) {
        let result: Result<(), Box<dyn Error>> = self.runtime.block_on(async {
            #[derive(Deserialize)]
            struct Response {
                etag: String,
                count: UInt,
            }
            let r = serde_json::from_str::<Response>(response.as_str())?;
            crypto_machine!(self)
                .mark_request_as_sent(
                    request_id.as_str().into(),
                    AnyIncomingResponse::KeysBackup(&add_backup_keys::v3::Response::new(
                        r.etag, r.count,
                    )),
                )
                .await?;
            Ok(())
        });
        if let Err(error) = result {
            error!("Failed to process mark_keys_backup_as_sent: {:?}", error);
        };
    }

    pub(crate) fn receive_sync_changes(&mut self, sync_json: String) -> Box<SyncChangesResult> {
        let result: Result<Box<SyncChanges>, Box<dyn Error>> = self.runtime.block_on(async {
            let machine = crypto_machine!(self);
            #[derive(serde::Deserialize)]
            struct Changes {
                to_device: Option<BTreeMap<String, Vec<Raw<AnyToDeviceEvent>>>>,
                device_lists: Option<DeviceLists>,
                device_one_time_keys_count: BTreeMap<OneTimeKeyAlgorithm, UInt>,
                next_batch: Option<String>,
                device_unused_fallback_key_types: Option<Vec<OneTimeKeyAlgorithm>>,
            }

            let changes = serde_json::from_str::<Changes>(sync_json.as_str())?;
            let changes = machine
                .receive_sync_changes(
                    EncryptionSyncChanges {
                        to_device_events: if let Some(events) = changes.to_device {
                            events["events"].clone()
                        } else {
                            Default::default()
                        },
                        changed_devices: &changes.device_lists.unwrap_or_default(),
                        one_time_keys_counts: &changes.device_one_time_keys_count,
                        unused_fallback_keys: changes.device_unused_fallback_key_types.as_deref(),
                        next_batch_token: changes.next_batch,
                    },
                    &DecryptionSettings {
                        sender_device_trust_requirement: TrustRequirement::Untrusted,
                    },
                )
                .await?;

            let mut events = vec![];
            let mut secrets_received = false;
            let mut self_verified = false;
            let mut received_done_events = vec![];
            for to_device_event in changes.0 {
                // NOTE: Do not use the question mark operator in for loop.
                match to_device_event {
                    ProcessedToDeviceEvent::Decrypted { raw, .. }
                    | ProcessedToDeviceEvent::PlainText(raw) => match raw.deserialize() {
                        Ok(AnyToDeviceEvent::KeyVerificationRequest(request)) => {
                            if let Some(request) = machine.get_verification_request(
                                &request.sender,
                                request.content.transaction_id.clone(),
                            ) {
                                events.push(KeyVerificationRequest(request));
                            } else {
                                error!(
                                    "No request for {} {}",
                                    request.sender, request.content.transaction_id
                                );
                            }
                        }
                        Ok(AnyToDeviceEvent::KeyVerificationDone(done)) => {
                            let machine = crypto_machine!(self);
                            if done.sender == machine.user_id() && machine.get_identity(&UserId::parse(&done.sender)?, None)
                                .await?
                                .ok_or("Identity not found")?
                                .is_verified() {
                                self_verified = true;
                            }
                            received_done_events.push(done.content.transaction_id.to_string());
                        }
                        Ok(AnyToDeviceEvent::SecretSend(_)) => {
                            secrets_received = true;
                        }
                        Ok(_) => {}
                        Err(error) => {
                            error!("Failed to deserialize to_device event: {}", error);
                        }
                    },
                    _ => {}
                }
            }
            Ok(Box::new(SyncChanges {
                sessions: events,
                keys: changes
                    .1
                    .iter()
                    .map(|it| Key {
                        session_id: it.session_id.clone(),
                        room_id: it.room_id.to_string(),
                    })
                    .collect(),
                secrets_received,
                self_verified,
                received_done_events,
            }))
        });
        Box::new(match result {
            Ok(changes) => SyncChangesResult::Ok(changes),
            Err(error) => {
                error!("Failed to process receive_sync_changes: {:?}", error);
                SyncChangesResult::Err(error)
            }
        })
    }

    pub(crate) fn share_room_key(
        &mut self,
        room_id: String,
        user_ids: Vec<String>,
        _only_trusted: bool,
        visibility: u8,
    ) -> Box<ToDeviceRequestResult> {
        let result: Result<Vec<ToDeviceRequest>, Box<dyn Error>> = self.runtime.block_on(async {
            let room_id = RoomId::parse(room_id)?;
            let user_ids: Vec<OwnedUserId> = user_ids
                .iter()
                .filter_map(|it| UserId::parse(it).ok())
                .collect();
            Ok(self
                .machine
                .as_ref()
                .ok_or("No crypto machine")?
                .share_room_key(
                    &room_id,
                    user_ids.iter().map(Deref::deref),
                    EncryptionSettings {
                        history_visibility: match visibility {
                            0 => HistoryVisibility::Invited,
                            1 => HistoryVisibility::Joined,
                            2 => HistoryVisibility::Shared,
                            3 => HistoryVisibility::WorldReadable,
                            _ => panic!("Invalid value for visibility"),
                        },
                        ..Default::default()
                    },
                )
                .await?
                .iter()
                .map(|it| ToDeviceRequest {
                    txn_id: it.txn_id.to_string(),
                    event_type: it.event_type.to_string(),
                    messages: it.messages.clone(),
                })
                .collect())
        });
        Box::new(match result {
            Ok(requests) => ToDeviceRequestResult::Ok(requests),
            Err(error) => {
                error!("Failed to share room key: {:?}", error);
                ToDeviceRequestResult::Err(error)
            }
        })
    }

    pub(crate) fn get_missing_sessions(
        &mut self,
        user_ids: Vec<String>,
    ) -> Box<MissingSessionsResult> {
        let result: Result<Box<KeysClaimRequest>, Box<dyn Error>> = self.runtime.block_on(async {
            let user_ids: Vec<OwnedUserId> = user_ids
                .iter()
                .filter_map(|it| UserId::parse(it).ok())
                .collect();

            let maybe_request = self
                .machine
                .as_ref()
                .ok_or("No crypto machine")?
                .get_missing_sessions(user_ids.iter().map(Deref::deref))
                .await?;

            if let Some((id, request)) = maybe_request {
                Ok(Box::new(KeysClaimRequest {
                    id: id.to_string(),
                    timeout: request.timeout,
                    one_time_keys: request.one_time_keys,
                }))
            } else {
                Ok(Box::new(KeysClaimRequest {
                    id: "".to_string(),
                    timeout: None,
                    one_time_keys: Default::default(),
                }))
            }
        });
        Box::new(match result {
            Ok(request) => {
                if request.id.is_empty() {
                    MissingSessionsResult::NoRequest
                } else {
                    MissingSessionsResult::Request(request)
                }
            }
            Err(error) => {
                error!("Failed to process get_missing_sessions: {:?}", error);
                MissingSessionsResult::Err(error)
            }
        })
    }

    pub(crate) fn encrypt_room_event(
        &mut self,
        room_id: String,
        content: String,
        matrix_type: String,
    ) -> Box<StringResult> {
        let result: Result<String, Box<dyn Error>> = self.runtime.block_on(async {
            let room_id = RoomId::parse(room_id)?;
            let encrypted = self
                .machine
                .as_ref()
                .ok_or("No crypto machine")?
                .encrypt_room_event_raw(
                    &room_id,
                    &matrix_type,
                    &serde_json::from_str(content.as_str())?,
                )
                .await?;

            Ok(serde_json::to_string(&encrypted)?)
        });
        Box::new(match result {
            Ok(event) => StringResult::Ok(event),
            Err(error) => {
                error!("Failed to process encrypt_room_event: {:?}", error);
                StringResult::Err(error)
            }
        })
    }

    pub(crate) fn decrypt_room_event(
        &mut self,
        room_id: String,
        json: String,
    ) -> Box<StringResult> {
        let result: Result<String, Box<dyn Error>> = self.runtime.block_on(async {
            let room_id = RoomId::parse(room_id)?;
            let machine = crypto_machine!(self);
            if !machine
                .is_room_key_available(&{ serde_json::from_str(json.as_str())? }, &room_id)
                .await
                .unwrap_or(false)
            {
                return Err("No room key available".into());
            }
            let event = machine
                .decrypt_room_event(
                    &{ serde_json::from_str(json.as_str())? },
                    &room_id,
                    &DecryptionSettings {
                        sender_device_trust_requirement: TrustRequirement::Untrusted,
                    },
                )
                .await?;
            Ok(serde_json::to_string(&event.event)?)
        });
        Box::new(match result {
            Ok(event) => StringResult::Ok(event),
            Err(error) => {
                // This happens if we don't have the key
                // error!("Failed to process decrypt_room_event: {:?}", error);
                StringResult::Err(error)
            }
        })
    }

    pub(crate) fn update_tracked_users(&mut self, user_ids: Vec<String>) {
        let result: Result<(), Box<dyn Error>> = self.runtime.block_on(async {
            let user_ids: Vec<OwnedUserId> = user_ids
                .iter()
                .filter_map(|it| UserId::parse(it).ok())
                .collect();
            crypto_machine!(self)
                .update_tracked_users(user_ids.iter().map(Deref::deref))
                .await?;
            Ok(())
        });
        if let Err(error) = result {
            error!("Failed to process update_tracked_users: {:?}", error);
        }
    }

    pub(crate) fn accept_verification(
        &mut self,
        remote_user: String,
        verification_id: String,
    ) -> Box<OutgoingKeyVerificationRequestResult> {
        let result: Result<Box<OutgoingKeyVerificationRequest>, Box<dyn Error>> =
            self.runtime.block_on(async {
                //TODO: why is there sometimes no request?
                Ok(Box::new(OutgoingKeyVerificationRequest(
                    crypto_machine!(self)
                        .get_verification_request(
                            &UserId::parse(remote_user.clone())?,
                            &verification_id,
                        )
                        .ok_or("No session")?
                        .accept()
                        .ok_or("No request")?,
                )))
            });
        Box::new(match result {
            Ok(request) => OutgoingKeyVerificationRequestResult::Ok(request),
            Err(error) => {
                error!("Failed to process accept_verification: {:?}", error);
                OutgoingKeyVerificationRequestResult::Err(error)
            }
        })
    }

    pub(crate) fn cancel_verification(
        &mut self,
        remote_user: String,
        verification_id: String,
    ) -> Box<OutgoingKeyVerificationRequestResult> {
        let result: Result<Box<OutgoingKeyVerificationRequest>, Box<dyn Error>> =
            self.runtime.block_on(async {
                let user_id = UserId::parse(remote_user.clone())?;
                //TODO: why is there sometimes no request?
                Ok(Box::new(OutgoingKeyVerificationRequest(
                    crypto_machine!(self)
                        .get_verification_request(&user_id, &verification_id)
                        .ok_or("No session")?
                        .cancel()
                        .ok_or("No request")?,
                )))
            });
        Box::new(match result {
            Ok(request) => OutgoingKeyVerificationRequestResult::Ok(request),
            Err(error) => {
                error!("Failed to process cancel_verification: {:?}", error);
                OutgoingKeyVerificationRequestResult::Err(error)
            }
        })
    }

    pub(crate) fn confirm_verification(
        &mut self,
        remote_user: String,
        verification_id: String,
    ) -> Box<ConfirmRequestsResult> {
        let result: Result<Box<ConfirmRequests>, Box<dyn Error>> = self.runtime.block_on(async {
            let user_id = UserId::parse(remote_user)?;
            let (outgoing_verification_requests, signature_upload_request) = match self
                .machine
                .as_ref()
                .ok_or("No crypto machine")?
                .get_verification_request(&user_id, &verification_id)
                .ok_or("No verification session for {remote_user} {verification_id}")?
                .state()
            {
                VerificationRequestState::Transitioned {
                    verification: Verification::SasV1(sas),
                    ..
                } => Some(sas),
                _ => None,
            }
            .ok_or("Unexpected state")?
            .confirm()
            .await?;

            let outgoing_verification_requests = outgoing_verification_requests
                .iter()
                .map(|it| OutgoingKeyVerificationRequest(it.clone()))
                .collect();

            Ok(Box::new(ConfirmRequests {
                verification: outgoing_verification_requests,
                signature: signature_upload_request,
            }))
        });
        Box::new(match result {
            Ok(result) => ConfirmRequestsResult::Ok(result),
            Err(error) => {
                error!("Failed to process confirm_verification: {:?}", error);
                ConfirmRequestsResult::Err(error)
            }
        })
    }

    pub(crate) fn start_sas(
        &mut self,
        remote_user: String,
        verification_id: String,
    ) -> Box<OutgoingKeyVerificationRequestResult> {
        let result: Result<Box<OutgoingKeyVerificationRequest>, Box<dyn Error>> =
            self.runtime.block_on(async {
                let user_id = UserId::parse(&remote_user)?;
                let result = self
                    .machine
                    .as_ref()
                    .ok_or("No crypto machine")?
                    .get_verification_request(&user_id, &verification_id)
                    .ok_or("No verification request")?
                    .start_sas()
                    .await?
                    .ok_or("No request")?;
                Ok(Box::new(OutgoingKeyVerificationRequest(result.1)))
            });
        Box::new(match result {
            Ok(request) => OutgoingKeyVerificationRequestResult::Ok(request),
            Err(error) => {
                error!("Failed to process start_sas: {:?}", error);
                OutgoingKeyVerificationRequestResult::Err(error)
            }
        })
    }

    pub(crate) fn accept_sas(
        self: &mut CryptoMachine,
        remote_user: String,
        verification_id: String,
    ) -> Box<OutgoingKeyVerificationRequestResult> {
        let result: Result<Box<OutgoingKeyVerificationRequest>, Box<dyn Error>> =
            self.runtime.block_on(async {
                let user_id = UserId::parse(remote_user)?;
                let session = self
                    .machine
                    .as_ref()
                    .ok_or("No crypto machine")?
                    .get_verification_request(&user_id, &verification_id)
                    .ok_or("No verification request")?;

                Ok(Box::new(OutgoingKeyVerificationRequest(
                    match session.state() {
                        VerificationRequestState::Transitioned {
                            verification: Verification::SasV1(sas),
                            ..
                        } => Some(sas),
                        _ => None,
                    }
                    .ok_or("Invalid state")?
                    .accept()
                    .ok_or("No request")?,
                )))
            });
        Box::new(match result {
            Ok(request) => OutgoingKeyVerificationRequestResult::Ok(request),
            Err(error) => {
                error!("Failed to process accept_sas: {:?}", error);
                OutgoingKeyVerificationRequestResult::Err(error)
            }
        })
    }

    pub(crate) fn verification_get_state(
        &mut self,
        remote_user: String,
        verification_id: String,
    ) -> Box<U8Result> {
        let result: Result<u8, Box<dyn Error>> = self.runtime.block_on(async {
            let user_id = UserId::parse(remote_user)?;
            Ok(
                match self
                    .machine
                    .as_ref()
                    .ok_or("No crypto machine")?
                    .get_verification_request(&user_id, &verification_id)
                {
                    Some(session) => match session.state() {
                        VerificationRequestState::Created { .. } => 0,
                        VerificationRequestState::Requested { .. } => 1,
                        VerificationRequestState::Ready { .. } => 2,
                        VerificationRequestState::Transitioned { .. } => 3,
                        VerificationRequestState::Done => 4,
                        VerificationRequestState::Cancelled(_) => 5,
                    },
                    None => 6,
                },
            )
        });
        Box::new(match result {
            Ok(state) => U8Result::Ok(state),
            Err(error) => {
                error!("Failed to process verification_get_state: {:?}", error);
                U8Result::Err(error)
            }
        })
    }

    pub(crate) fn monitor_verification(
        &mut self,
        remote_user: String,
        verification_id: String,
        callback: usize,
    ) {
        let crypto_machine = crypto_machine!(self).clone();
        self.runtime.spawn(async move {
            let user_id = UserId::parse(&remote_user).unwrap();
            let request = crypto_machine.get_verification_request(&user_id, &verification_id);
            let mut changes = request.unwrap().changes();
            while changes.next().await.is_some() {
                let func =
                    unsafe { std::mem::transmute::<usize, fn(String, String, String)>(callback) };
                func(
                    crypto_machine.user_id().to_string(),
                    remote_user.clone(),
                    verification_id.clone(),
                );
            }
        });
    }

    pub(crate) fn monitor_sas(
        &mut self,
        remote_user: String,
        verification_id: String,
        callback: usize,
    ) {
        let crypto_machine = crypto_machine!(self).clone();
        self.runtime.spawn(async move {
            let user_id = UserId::parse(&remote_user).unwrap();
            let request = crypto_machine
                .get_verification_request(&user_id, &verification_id)
                .unwrap();
            let VerificationRequestState::Transitioned {
                verification: Verification::SasV1(sas),
                ..
            } = request.state()
            else {
                return;
            };
            let mut changes = sas.changes();
            while let Some(_) = changes.next().await {
                let func =
                    unsafe { std::mem::transmute::<usize, fn(String, String, String)>(callback) };
                func(
                    crypto_machine.user_id().to_string(),
                    remote_user.clone(),
                    verification_id.clone(),
                );
            }
        });
    }

    pub(crate) fn sas_get_state(
        &mut self,
        remote_user: String,
        verification_id: String,
    ) -> Box<U8Result> {
        let user_id = UserId::parse(remote_user).unwrap();

        let result: Result<u8, Box<dyn Error>> = self.runtime.block_on(async {
            let session = crypto_machine!(self)
                .get_verification_request(&user_id, &verification_id)
                .ok_or("No session")?;

            Ok(
                if let VerificationRequestState::Transitioned {
                    verification: Verification::SasV1(sas),
                    ..
                } = session.state()
                {
                    match sas.state() {
                        SasState::Started { .. } => 0,
                        SasState::Accepted { .. } => 1,
                        SasState::KeysExchanged { .. } => 2,
                        SasState::Confirmed => 3,
                        SasState::Done { .. } => 4,
                        SasState::Cancelled(_) => 5,
                        SasState::Created { .. } => 6,
                    }
                } else {
                    // this is (in the current setup) mostly normal, since we're always querying sas state.
                    6
                },
            )
        });
        Box::new(match result {
            Ok(state) => U8Result::Ok(state),
            Err(error) => {
                error!("Failed to process sas_get_state: {:?}", error);
                U8Result::Err(error)
            }
        })
    }

    pub(crate) fn sas_emoji(
        &self,
        remote_user: String,
        verification_id: String,
    ) -> Box<EmojiResult> {
        let result: Result<Vec<Emoji>, Box<dyn Error>> = self.runtime.block_on(async {
            Ok(match self
                .machine
                .as_ref()
                .ok_or("No crypto machine")?
                .get_verification_request(&UserId::parse(&remote_user)?, &verification_id)
                .ok_or("No request")?
                .state()
            {
                VerificationRequestState::Transitioned {
                    verification: Verification::SasV1(sas),
                    ..
                } => Some(sas),
                _ => None,
            }
            .ok_or("Session in invalid state")?
            .emoji()
            .expect("Emoji can't be presented yet")
            .iter()
            .map(|e| Emoji(e.clone()))
            .collect())
        });
        Box::new(match result {
            Ok(emoji) => EmojiResult::Ok(emoji),
            Err(error) => {
                error!("Failed to process sas_emoji: {:?}", error);
                EmojiResult::Err(error)
            }
        })
    }
    pub(crate) fn request_device_verification(
        &mut self,
        user_id: String,
        device_id: String,
    ) -> Box<CreatedSessionResult> {
        let result: Result<Box<CreatedSession>, Box<dyn Error>> = self.runtime.block_on(async {
            let user_id = UserId::parse(user_id)?;
            let device_id: Box<DeviceId> = device_id.into();
            let device = self
                .machine
                .as_ref()
                .ok_or("No crypto machine")?
                .get_device(&user_id, &device_id, None)
                .await?
                .ok_or("No device")?;
            let (session, outgoing) = device.request_verification();
            Ok(Box::new(CreatedSession(session, outgoing)))
        });
        Box::new(match result {
            Ok(session) => CreatedSessionResult::Ok(session),
            Err(error) => {
                error!("Failed to process request_device_verification: {:?}", error);
                CreatedSessionResult::Err(error)
            }
        })
    }

    pub(crate) fn request_user_verification(
        &mut self,
        user_id: String,
        room_id: String,
        request_event_id: String,
    ) -> Box<KeyVerificationRequestResult> {
        let result: Result<Box<KeyVerificationRequest>, Box<dyn Error>> =
            self.runtime.block_on(async {
                let user_id = UserId::parse(user_id)?;
                let room_id = RoomId::parse(room_id)?;
                let event_id = EventId::parse(request_event_id)?;
                let identity = self
                    .machine
                    .as_ref()
                    .expect("Crypto machine must be valid")
                    .get_identity(&user_id, None)
                    .await?
                    .ok_or("No request")?;
                let other = match identity {
                    UserIdentity::Other(other) => Some(other),
                    _ => None,
                }
                .ok_or("Not a different user")?;
                Ok(Box::new(KeyVerificationRequest(
                    other.request_verification(&room_id, &event_id, None),
                )))
            });
        Box::new(match result {
            Ok(request) => KeyVerificationRequestResult::Ok(request),
            Err(error) => {
                error!("Failed to process request_user_verification: {:?}", error);
                KeyVerificationRequestResult::Err(error)
            }
        })
    }

    pub(crate) fn request_user_verification_content(
        &mut self,
        user_id: String,
    ) -> Box<StringResult> {
        let result: Result<String, Box<dyn Error>> = self.runtime.block_on(async {
            let user_id = UserId::parse(user_id).expect("User id must be valid");
            let other = match crypto_machine!(self)
                .get_identity(&user_id, None)
                .await?
                .ok_or("User not found")?
            {
                UserIdentity::Other(other) => Some(other),
                _ => None,
            }
            .ok_or("Not for a remote user")?;
            Ok(serde_json::to_string(
                &other.verification_request_content(None),
            )?)
        });
        Box::new(match result {
            Ok(request) => StringResult::Ok(request),
            Err(error) => {
                error!(
                    "Failed to process request_user_verification_content: {:?}",
                    error
                );
                StringResult::Err(error)
            }
        })
    }

    pub(crate) fn get_room_event_encryption_info(
        &self,
        event: String,
        room_id: String,
    ) -> Box<EncryptionInfoResult> {
        let result: Result<Box<EncryptionInfo>, Box<dyn Error>> = self.runtime.block_on(async {
            let room_id = RoomId::parse(room_id)?;
            let info = self
                .machine
                .as_ref()
                .ok_or("No crypto machine")?
                .get_room_event_encryption_info(&serde_json::from_str(&event)?, &room_id)
                .await?;
            Ok(Box::new(EncryptionInfo(info)))
        });
        Box::new(match result {
            Ok(encryption_info) => EncryptionInfoResult::Ok(encryption_info),
            Err(error) => {
                // This just happens when we don't have the key
                // error!(
                //     "Failed to process get_room_event_encryption_info: {:?}",
                //     error
                // );
                EncryptionInfoResult::Err(error)
            }
        })
    }

    pub(crate) fn receive_verification_event(&mut self, full_json: String) {
        let result: Result<(), Box<dyn Error>> = self.runtime.block_on(async {
            crypto_machine!(self)
                .receive_verification_event(&serde_json::from_str(&full_json)?)
                .await?;
            Ok(())
        });
        if let Err(error) = result {
            error!("Failed to process receive_verification_event: {:?}", error);
        }
    }

    pub(crate) fn migrate_secrets(
        &mut self,
        master_key: String,
        self_signing_key: String,
        user_signing_key: String,
        backup_key: String,
        version: String,
    ) {
        let result: Result<(), Box<dyn Error>> = self.runtime.block_on(async {
            crypto_machine!(self)
                .import_cross_signing_keys(CrossSigningKeyExport {
                    master_key: if master_key.is_empty() {
                        None
                    } else {
                        Some(master_key)
                    },
                    self_signing_key: if self_signing_key.is_empty() {
                        None
                    } else {
                        Some(self_signing_key)
                    },
                    user_signing_key: if user_signing_key.is_empty() {
                        None
                    } else {
                        Some(user_signing_key)
                    },
                })
                .await?;

            if !backup_key.is_empty() {
                let backup_decryption_key = BackupDecryptionKey::from_base64(&backup_key)?;
                let backup_machine = crypto_machine!(self).backup_machine();
                backup_machine
                    .save_decryption_key(Some(backup_decryption_key.clone()), Some(version.clone()))
                    .await?;
                let backup_key = backup_decryption_key.megolm_v1_public_key();
                backup_key.set_version(version);
                backup_machine.enable_backup_v1(backup_key).await?;
            }
            Ok(())
        });
        if let Err(err) = result {
            error!("{}", err);
        }
    }

    pub(crate) fn load_secrets(
        &mut self,
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
    ) -> Box<StringResult> {
        let result: Result<String, Box<dyn Error>> = self.runtime.block_on(async {
            use matrix_sdk_common::ruma::serde::Base64;
            let mut content = SecretStorageKeyEventContent::new(
                key_id,
                SecretStorageEncryptionAlgorithm::V1AesHmacSha2(
                    SecretStorageV1AesHmacSha2Properties::new(
                        Base64::parse(iv).ok(),
                        Base64::parse(mac).ok(),
                    ),
                ),
            );
            content.passphrase = Some(PassPhrase::new(salt, UInt::new_wrapping(iterations)));

            let ss_key = SecretStorageKey::from_account_data(&passphrase, content)?;

            let backup_decryption_key = BackupDecryptionKey::from_base64(&String::from_utf8(
                ss_key.decrypt(
                    &AesHmacSha2EncryptedData {
                        iv: Base64::<Standard>::parse(backup_key_iv)?
                            .as_bytes()
                            .try_into()?,
                        ciphertext: Base64::parse(backup_key_cipher)?,
                        mac: Base64::<Standard>::parse(backup_key_mac)?
                            .as_bytes()
                            .try_into()?,
                    },
                    &SecretName::RecoveryKey,
                )?,
            )?)?;

            let master_private = String::from_utf8(
                ss_key.decrypt(
                    &AesHmacSha2EncryptedData {
                        iv: Base64::<Standard>::parse(master_key_iv)?
                            .as_bytes()
                            .try_into()?,
                        ciphertext: Base64::<Standard>::parse(master_key_cipher)?,
                        mac: Base64::<Standard>::parse(master_key_mac)?
                            .as_bytes()
                            .try_into()?,
                    },
                    &SecretName::CrossSigningMasterKey,
                )?,
            )?;

            let self_private = String::from_utf8(
                ss_key.decrypt(
                    &AesHmacSha2EncryptedData {
                        iv: Base64::<Standard>::parse(self_key_iv)?
                            .as_bytes()
                            .try_into()?,
                        ciphertext: Base64::parse(self_key_cipher)?,
                        mac: Base64::<Standard>::parse(self_key_mac)?
                            .as_bytes()
                            .try_into()?,
                    },
                    &SecretName::CrossSigningSelfSigningKey,
                )?,
            )?;

            let user_private = String::from_utf8(
                ss_key.decrypt(
                    &AesHmacSha2EncryptedData {
                        iv: Base64::<Standard>::parse(user_key_iv)?
                            .as_bytes()
                            .try_into()?,
                        ciphertext: Base64::parse(user_key_cipher)?,
                        mac: Base64::<Standard>::parse(user_key_mac)?
                            .as_bytes()
                            .try_into()?,
                    },
                    &SecretName::CrossSigningUserSigningKey,
                )?,
            )?;

            let machine = crypto_machine!(self);
            machine
                .backup_machine()
                .save_decryption_key(Some(backup_decryption_key.clone()), Some(version.clone()))
                .await?;

            let backup_key = backup_decryption_key.megolm_v1_public_key();
            backup_key.set_version(version);
            machine
                .backup_machine()
                .enable_backup_v1(backup_key)
                .await?;

            machine
                .import_cross_signing_keys(CrossSigningKeyExport {
                    master_key: Some(master_private),
                    self_signing_key: Some(self_private),
                    user_signing_key: Some(user_private),
                })
                .await?;

            Ok(serde_json::to_string(
                &machine
                    .get_device(machine.user_id(), machine.device_id(), None)
                    .await?
                    .ok_or("Own device not found")?
                    .verify()
                    .await?
                    .signed_keys,
            )?)
        });
        Box::new(match result {
            Ok(upload_request) => StringResult::Ok(upload_request),
            Err(error) => {
                error!("Error while loading secrets: {:?}", error);
                StringResult::Err(error)
            }
        })
    }

    pub(crate) fn import_from_backup(&mut self, response: String, version: String) -> Vec<Key> {
        let result: Result<Vec<Key>, Box<dyn Error>> = self.runtime.block_on(async {
            tracing::debug!("Before importing: {} keys", crypto_machine!(self).store().get_inbound_group_sessions().await?.len());
            #[derive(Deserialize)]
            struct Response {
                rooms: BTreeMap<matrix_sdk_common::ruma::OwnedRoomId, RoomKeyBackup>,
            }
            let backed_up_keys = serde_json::from_str::<Response>(&response)?;

            let decryption_key = self
                .machine
                .as_ref()
                .ok_or("No crypto machine")?
                .backup_machine()
                .get_backup_keys()
                .await?
                .decryption_key
                .ok_or("No backup key")?;

            let mut decrypted_room_keys: Vec<_> = Vec::new();
            let mut data = vec!();

            for (room_id, room_keys) in backed_up_keys.rooms {
                for (session_id, room_key) in room_keys.sessions {
                    let Ok(room_key) = room_key.deserialize() else {
                        continue;
                    };

                    let Ok(room_key) = decryption_key.decrypt_session_data(room_key.session_data)
                    else {
                        continue;
                    };

                    data.push(Key {
                        session_id: session_id.clone(),
                        room_id: room_id.to_string(),
                    });
                    decrypted_room_keys.push(ExportedRoomKey::from_backed_up_room_key(
                        room_id.to_owned(),
                        session_id,
                        room_key,
                    ));
                }
            }

            crypto_machine!(self)
                .store()
                .import_room_keys(decrypted_room_keys, Some(&version), |_, _| {})
                .await?;
            tracing::debug!("After importing: {} keys", crypto_machine!(self).store().get_inbound_group_sessions().await?.len());
            Ok(data)
        });
        result.unwrap_or_else(|err| {
            error!("Failed to process import_from_backup: {}", err);
            vec!()
        })
    }
    pub(crate) fn request_self_verification(&mut self) -> Box<CreatedSessionResult> {
        let result: Result<Box<CreatedSession>, Box<dyn Error>> = self.runtime.block_on(async {
            let machine = crypto_machine!(self);
            let own = match machine
                .get_identity(machine.user_id(), None)
                .await?
                .ok_or("Own identity not found")?
            {
                UserIdentity::Own(own) => Some(own),
                _ => None,
            }
            .ok_or("Identity not found")?;

            let session = own.request_verification().await?;
            Ok(Box::new(CreatedSession(session.0, session.1)))
        });
        Box::new(match result {
            Ok(session) => CreatedSessionResult::Ok(session),
            Err(error) => {
                error!("Failed to process request_self_verification: {:?}", error);
                CreatedSessionResult::Err(error)
            }
        })
    }

    pub(crate) fn request_secrets_from_devices(&mut self) {
        let result: Result<(), Box<dyn Error>> = self.runtime.block_on(async {
            crypto_machine!(self)
                .query_missing_secrets_from_other_sessions()
                .await?;
            Ok(())
        });
        if let Err(err) = result {
            error!("Failed to process request_secrets_from_devices: {}", err);
        }
    }

    pub(crate) fn has_pending_backup_key(&self) -> bool {
        let result: Result<bool, Box<dyn Error>> = self.runtime.block_on(async {
            let machine = crypto_machine!(self);
            Ok(machine
                .store()
                .get_secrets_from_inbox(&SecretName::RecoveryKey)
                .await?
                .len()
                > 0
                && machine
                    .backup_machine()
                    .get_backup_keys()
                    .await?
                    .decryption_key
                    .is_none())
        });
        result.unwrap_or_else(|error| {
            error!("Failed to process has_pending_backup_key: {:?}", error);
            false
        })
    }

    pub(crate) fn initialize_existing_backup(&mut self, version_reply: String) {
        let result: Result<(), Box<dyn Error>> = self.runtime.block_on(async {
            let machine = crypto_machine!(self);
            for secret in machine
                .store()
                .get_secrets_from_inbox(&SecretName::RecoveryKey)
                .await?
            {
                let decryption_key =
                    BackupDecryptionKey::from_base64(&secret.event.content.secret)?;
                let version = {
                    let Object(object) = serde_json::from_str::<Value>(&version_reply)? else {
                        return Err("Invalid version reply".into());
                    };
                    let Some(version) = object.get("version").and_then(|v| v.as_str()) else {
                        return Err("Invalid version reply".into());
                    };
                    version.to_string()
                };
                let backup_info = serde_json::from_str::<RoomKeyBackupInfo>(&version_reply)?.into();
                if decryption_key.backup_key_matches(&backup_info) {
                    machine.backup_machine().disable_backup().await?;
                    let backup_key = decryption_key.megolm_v1_public_key();
                    backup_key.set_version(version.clone());

                    machine
                        .backup_machine()
                        .save_decryption_key(Some(decryption_key.to_owned()), Some(version))
                        .await?;
                    machine
                        .backup_machine()
                        .enable_backup_v1(backup_key)
                        .await?;
                    break;
                }
            }
            machine
                .store()
                .delete_secrets_from_inbox(&SecretName::RecoveryKey)
                .await?;
            Ok(())
        });
        if let Err(err) = result {
            error!("Failed to process initialize_existing_backup: {:?}", err);
        }
    }
    pub(crate) fn backup_keys(&mut self) -> Box<BackupRequestResult> {
        let result: Result<Option<Box<BackupRequest>>, Box<dyn Error>> =
            self.runtime.block_on(async {
                let backup_request = self
                    .machine
                    .as_ref()
                    .ok_or("No crypto machine")?
                    .backup_machine()
                    .backup()
                    .await?;
                if let Some(backup_request) = backup_request {
                    Ok(Some(Box::new(BackupRequest {
                        transaction_id: Some(backup_request.0),
                        request: Some(backup_request.1),
                    })))
                } else {
                    Ok(None)
                }
            });
        Box::new(match result {
            Ok(Some(result)) => BackupRequestResult::Ok(result),
            Ok(None) => BackupRequestResult::NoRequest,
            Err(error) => {
                error!("Failed to process backup_keys: {:?}", error);
                BackupRequestResult::Err(error)
            }
        })
    }

    pub(crate) fn export_keys(&self, passphrase: String) -> Box<StringResult> {
        use matrix_sdk_crypto::encrypt_room_key_export;
        let result: Result<String, Box<dyn Error>> = self.runtime.block_on(async {
            Ok(encrypt_room_key_export(
                &self
                    .machine
                    .as_ref()
                    .ok_or("No crypto machine")?
                    .store()
                    .export_room_keys(|_| true)
                    .await?,
                &passphrase,
                500_000,
            )?)
        });
        Box::new(match result {
            Ok(data) => StringResult::Ok(data),
            Err(error) => {
                error!("Failed to process export_keys: {:?}", error);
                StringResult::Err(error)
            }
        })
    }

    pub(crate) fn import_olm_sessions(
        &self,
        sessions_data: Vec<crate::ffi::OlmSessionData>,
        pickle_key: String,
    ) {
        self.runtime.block_on(async {
            let device_id: OwnedDeviceId = crypto_machine!(self).device_id().into();
            let identity_keys = crypto_machine!(self).identity_keys();
            let device_keys = matrix_sdk_crypto::types::DeviceKeys::new(
                UserId::parse(crypto_machine!(self).user_id()).unwrap(),
                device_id.clone(),
                Default::default(),
                BTreeMap::from([
                    (
                        DeviceKeyId::from_parts(DeviceKeyAlgorithm::Ed25519, &device_id),
                        DeviceKey::Ed25519(identity_keys.ed25519),
                    ),
                    (
                        DeviceKeyId::from_parts(DeviceKeyAlgorithm::Curve25519, &device_id),
                        DeviceKey::Curve25519(identity_keys.curve25519),
                    ),
                ]),
                Default::default(),
            );
            let mut sessions: Vec<olm::Session> = vec![];
            let pickle_key = Base64::<Standard>::parse(pickle_key.clone()).unwrap();
            let pickle_key = pickle_key.as_bytes();
            for session_data in sessions_data {
                let Ok(session) = matrix_sdk_crypto::vodozemac::olm::Session::from_libolm_pickle(
                    &session_data.libolm_pickle,
                    pickle_key,
                ) else {
                    continue;
                };
                sessions.push({
                    let Ok(session) = olm::Session::from_pickle(
                        device_keys.clone(),
                        olm::PickledSession {
                            pickle: session.pickle(),
                            sender_key: {
                                let Ok(key) =
                                    Curve25519PublicKey::from_base64(&session_data.sender_key)
                                else {
                                    continue;
                                };
                                key
                            },
                            created_using_fallback_key: false,
                            creation_time: SecondsSinceUnixEpoch({
                                let Some(secs) = UInt::new(session_data.creation_time) else {
                                    continue;
                                };
                                secs
                            }),
                            last_use_time: SecondsSinceUnixEpoch({
                                let Some(secs) = UInt::new(session_data.last_use_time) else {
                                    continue;
                                };
                                secs
                            }),
                        },
                    ) else {
                        continue;
                    };
                    session
                });
            }
            let mut changes = Changes::default();
            changes.sessions = sessions;
            crypto_machine!(self)
                .store()
                .save_changes(changes)
                .await
                .unwrap();
        });
    }

    pub(crate) fn inbound_from_libolm_pickle(
        &self,
        session_data: Vec<crate::ffi::MegolmSessionData>,
        pickle_key: String,
    ) {
        self.runtime.block_on(async {
            let Ok(pickle_key) = Base64::<Standard>::parse(&pickle_key) else {
                return;
            };
            let pickle_key = pickle_key.as_bytes();
            let mut sessions = vec![];
            for session_data in session_data {
                let Ok(inbound) = InboundGroupSession::from_libolm_pickle(
                    &session_data.libolm_pickle,
                    pickle_key,
                ) else {
                    continue;
                };
                sessions.push(ExportedRoomKey {
                    algorithm: EventEncryptionAlgorithm::MegolmV1AesSha2,
                    room_id: {
                        let Ok(room_id) = RoomId::parse(session_data.room_id) else {
                            continue;
                        };
                        room_id
                    },
                    sender_key: {
                        let Ok(key) =
                            Curve25519PublicKey::from_base64(&session_data.sender_curve_key)
                        else {
                            continue;
                        };
                        key
                    },
                    session_id: inbound.session_id(),
                    session_key: inbound.export_at_first_known_index(),
                    sender_claimed_keys: Default::default(),
                    forwarding_curve25519_key_chain: vec![],
                    shared_history: false,
                });
            }
            crypto_machine!(self)
                .store()
                .import_exported_room_keys(sessions, |_, _| {})
                .await
                .unwrap();
        })
    }

    pub(crate) fn import_keys(&self, passphrase: String, ciphertext: String) -> Box<KeyResult> {
        use matrix_sdk_crypto::decrypt_room_key_export;
        let result: Result<Vec<Key>, Box<dyn Error>> = self.runtime.block_on(async {
            let keys = decrypt_room_key_export(ciphertext.as_bytes(), &passphrase)?;
            let keys = self
                .machine
                .as_ref()
                .ok_or("No crypto machine")?
                .store()
                .import_exported_room_keys(keys, |_, _| {})
                .await?
                .keys;

            let mut keys_vec = vec![];
            for room_id in keys.keys() {
                for sender in keys[room_id].keys() {
                    for session_id in &keys[room_id][sender] {
                        keys_vec.push(Key {
                            room_id: room_id.to_string(),
                            session_id: session_id.to_owned(),
                        });
                    }
                }
            }
            Ok(keys_vec)
        });
        Box::new(match result {
            Ok(keys) => KeyResult::Ok(keys),
            Err(error) => {
                error!("Failed to process import_keys: {:?}", error);
                KeyResult::Err(error)
            }
        })
    }
    pub(crate) fn all_sessions_verified(&self, user_id: String) -> bool {
        let result: Result<bool, Box<dyn Error>> = self.runtime.block_on(async {
            let user_id = UserId::parse(user_id)?;
            Ok(self
                .machine
                .as_ref()
                .ok_or("No crypto machine")?
                .get_user_devices(&user_id, None)
                .await?
                .devices()
                .all(|it| it.is_cross_signed_by_owner()))
        });
        result.unwrap_or_else(|error| {
            error!("Failed to process all_sessions_verified: {:?}", error);
            false
        })
    }

    pub(crate) fn is_user_verified(&self, user_id: String) -> bool {
        let result: Result<bool, Box<dyn Error>> = self.runtime.block_on(async {
            Ok(crypto_machine!(self)
                .get_identity(&UserId::parse(&user_id)?, None)
                .await?
                .ok_or("Identity not found")?
                .is_verified())
        });
        result.unwrap_or_else(|error| {
            error!("Failed to process is_user_verified: {:?}", error);
            false
        })
    }

    pub(crate) fn is_e2ee_device(&self, user_id: String, device_id: String) -> bool {
        let result: Result<bool, Box<dyn Error>> = self.runtime.block_on(async {
            let user_id = UserId::parse(user_id)?;
            let device_id: Box<DeviceId> = device_id.into();
            Ok(crypto_machine!(self)
                .get_device(&user_id, &device_id, None)
                .await?
                .is_some())
        });
        result.unwrap_or_else(|error| {
            error!("Failed to process is_e2ee_device: {:?}", error);
            false
        })
    }

    pub(crate) fn is_verified_device(&self, user_id: String, device_id: String) -> bool {
        let result: Result<bool, Box<dyn Error>> = self.runtime.block_on(async {
            let user_id = UserId::parse(user_id)?;
            let device_id: Box<DeviceId> = device_id.into();
            Ok(crypto_machine!(self)
                .get_device(&user_id, &device_id, None)
                .await?
                .ok_or("Unknown device")?
                .is_cross_signing_trusted())
        });
        result.unwrap_or_else(|error| {
            error!("Failed to process is_verified_device: {:?}", error);
            false
        })
    }

    pub(crate) fn has_initialized_backup(&self) -> bool {
        self.runtime
            .block_on(async { crypto_machine!(self).backup_machine().enabled().await })
    }

    pub(crate) fn all_private_cs_keys_available(&self) -> bool {
        self.runtime.block_on(async {
            crypto_machine!(self).cross_signing_status().await.is_complete()
        })
    }
}

#[derive(Clone)]
pub(crate) struct BackupRequest {
    transaction_id: Option<OwnedTransactionId>,
    request: Option<KeysBackupRequest>,
}

impl BackupRequest {
    pub(crate) fn transaction_id(&self) -> String {
        self.transaction_id
            .clone()
            .expect("BackupRequest::transaction_id must not be called when has_request is false")
            .to_string()
    }
    pub(crate) fn version(&self) -> String {
        self.request
            .clone()
            .expect("BackupRequest::version must not be called when has_request is false")
            .version
    }
    pub(crate) fn rooms(&self) -> String {
        serde_json::to_string(
            &self
                .request
                .clone()
                .expect("BackupRequest::rooms must not be called when has_request is false")
                .rooms,
        )
        .unwrap_or(Default::default())
    }
    pub(crate) fn has_request(&self) -> bool {
        self.request.is_some()
    }
}
