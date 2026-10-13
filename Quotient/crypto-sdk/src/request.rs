use log::error;
use matrix_sdk_common::ruma::{
    OneTimeKeyAlgorithm, OwnedDeviceId, OwnedUserId,
    api::client::keys::upload_signatures,
    events::{AnyMessageLikeEventContent, AnyToDeviceEventContent},
    serde::Raw,
    to_device::DeviceIdOrAllDevices,
};
use matrix_sdk_crypto::{
    VerificationRequest,
    types::requests::{AnyOutgoingRequest, OutgoingVerificationRequest},
};
use std::{collections::BTreeMap, time::Duration};

#[derive(Clone)]
pub(crate) struct OutgoingKeyVerificationRequest(pub(crate) OutgoingVerificationRequest);

impl OutgoingKeyVerificationRequest {
    pub(crate) fn to_device_event_type(&self) -> String {
        if let OutgoingVerificationRequest::ToDevice(request) = &self.0 {
            request.event_type.to_string()
        } else {
            panic!()
        }
    }

    pub(crate) fn to_device_txn_id(&self) -> String {
        if let OutgoingVerificationRequest::ToDevice(request) = &self.0 {
            request.txn_id.to_string()
        } else {
            panic!()
        }
    }

    pub(crate) fn to_device_messages(&self) -> String {
        if let OutgoingVerificationRequest::ToDevice(request) = &self.0 {
            serde_json::to_string(&request.messages).unwrap()
        } else {
            panic!()
        }
    }

    pub(crate) fn in_room_room_id(&self) -> String {
        if let OutgoingVerificationRequest::InRoom(request) = &self.0 {
            request.room_id.to_string()
        } else {
            panic!()
        }
    }

    pub(crate) fn in_room_txn_id(&self) -> String {
        if let OutgoingVerificationRequest::InRoom(request) = &self.0 {
            request.txn_id.to_string()
        } else {
            panic!()
        }
    }

    pub(crate) fn in_room_content(&self) -> String {
        if let OutgoingVerificationRequest::InRoom(request) = &self.0 {
            serde_json::to_string(&request.content).unwrap()
        } else {
            panic!()
        }
    }

    pub(crate) fn in_room_event_type(&self) -> String {
        if let OutgoingVerificationRequest::InRoom(request) = &self.0 {
            match *request.content {
                AnyMessageLikeEventContent::KeyVerificationReady(_) => "m.key.verification.ready",
                AnyMessageLikeEventContent::KeyVerificationStart(_) => "m.key.verification.start",
                AnyMessageLikeEventContent::KeyVerificationCancel(_) => "m.key.verification.cancel",
                AnyMessageLikeEventContent::KeyVerificationAccept(_) => "m.key.verification.accept",
                AnyMessageLikeEventContent::KeyVerificationKey(_) => "m.key.verification.key",
                AnyMessageLikeEventContent::KeyVerificationMac(_) => "m.key.verification.mac",
                AnyMessageLikeEventContent::KeyVerificationDone(_) => "m.key.verification.done",
                AnyMessageLikeEventContent::RoomEncrypted(_) => "m.room.encrypted",
                _ => {
                    error!(
                        "Requesting to send unexpected event type {:?}",
                        request.content
                    );
                    Default::default()
                }
            }
            .to_string()
        } else {
            panic!()
        }
    }
}

#[derive(Clone)]
pub(crate) struct KeysClaimRequest {
    pub(crate) id: String,
    pub(crate) timeout: Option<Duration>,
    pub(crate) one_time_keys: BTreeMap<OwnedUserId, BTreeMap<OwnedDeviceId, OneTimeKeyAlgorithm>>,
}

impl KeysClaimRequest {
    pub(crate) fn id(&self) -> String {
        self.id.clone()
    }
    pub(crate) fn timeout(&self) -> usize {
        self.timeout.unwrap_or(Duration::from_millis(0)).as_millis() as usize
    }
    pub(crate) fn one_time_keys(&self) -> String {
        serde_json::to_string(&self.one_time_keys).unwrap()
    }
}

#[derive(Clone)]
pub(crate) struct ToDeviceRequest {
    pub(crate) event_type: String,
    pub(crate) txn_id: String,
    pub(crate) messages:
        BTreeMap<OwnedUserId, BTreeMap<DeviceIdOrAllDevices, Raw<AnyToDeviceEventContent>>>,
}

impl ToDeviceRequest {
    pub(crate) fn event_type(&self) -> String {
        self.event_type.clone()
    }
    pub(crate) fn txn_id(&self) -> String {
        self.txn_id.clone()
    }
    pub(crate) fn messages(&self) -> String {
        serde_json::to_string(&self.messages).unwrap()
    }
}

#[derive(Clone)]
pub(crate) struct OutgoingRequest(pub(crate) matrix_sdk_crypto::types::requests::OutgoingRequest);

impl OutgoingRequest {
    pub(crate) fn id(&self) -> String {
        self.0.request_id().to_string()
    }

    pub(crate) fn request_type(&self) -> crate::ffi::OutgoingRequestType {
        match self.0.request() {
            AnyOutgoingRequest::KeysUpload(_) => crate::ffi::OutgoingRequestType::KeysUpload,
            AnyOutgoingRequest::KeysQuery(_) => crate::ffi::OutgoingRequestType::KeysQuery,
            AnyOutgoingRequest::KeysClaim(_) => crate::ffi::OutgoingRequestType::KeysClaim,
            AnyOutgoingRequest::ToDeviceRequest(_) => crate::ffi::OutgoingRequestType::ToDevice,
            AnyOutgoingRequest::SignatureUpload(_) => {
                crate::ffi::OutgoingRequestType::SignatureUpload
            }
            AnyOutgoingRequest::RoomMessage(_) => crate::ffi::OutgoingRequestType::RoomMessage,
        }
    }

    pub(crate) fn keys_upload_device_keys(&self) -> String {
        if let AnyOutgoingRequest::KeysUpload(request) = self.0.request() {
            serde_json::to_string(&request.device_keys).unwrap()
        } else {
            panic!()
        }
    }

    pub(crate) fn keys_upload_fallback_keys(&self) -> String {
        if let AnyOutgoingRequest::KeysUpload(request) = self.0.request() {
            serde_json::to_string(&request.fallback_keys).unwrap()
        } else {
            panic!()
        }
    }

    pub(crate) fn keys_upload_one_time_keys(&self) -> String {
        if let AnyOutgoingRequest::KeysUpload(request) = self.0.request() {
            serde_json::to_string(&request.one_time_keys).unwrap()
        } else {
            panic!()
        }
    }

    pub(crate) fn keys_query_device_keys(&self) -> String {
        if let AnyOutgoingRequest::KeysQuery(request) = self.0.request() {
            serde_json::to_string(&request.device_keys).unwrap()
        } else {
            panic!()
        }
    }

    pub(crate) fn keys_query_timeout(&self) -> usize {
        if let AnyOutgoingRequest::KeysQuery(request) = self.0.request() {
            request
                .timeout
                .unwrap_or(Duration::from_millis(0))
                .as_millis() as usize
        } else {
            panic!()
        }
    }

    pub(crate) fn keys_claim_timeout(&self) -> usize {
        if let AnyOutgoingRequest::KeysClaim(request) = self.0.request() {
            request
                .timeout
                .unwrap_or(Duration::from_millis(0))
                .as_millis() as usize
        } else {
            panic!()
        }
    }

    pub(crate) fn keys_claim_one_time_keys(&self) -> String {
        if let AnyOutgoingRequest::KeysClaim(request) = self.0.request() {
            serde_json::to_string(&request.one_time_keys).unwrap()
        } else {
            panic!()
        }
    }

    pub(crate) fn to_device_event_type(&self) -> String {
        if let AnyOutgoingRequest::ToDeviceRequest(request) = self.0.request() {
            request.event_type.to_string()
        } else {
            panic!()
        }
    }

    pub(crate) fn to_device_txn_id(&self) -> String {
        if let AnyOutgoingRequest::ToDeviceRequest(request) = self.0.request() {
            request.txn_id.to_string()
        } else {
            panic!()
        }
    }

    pub(crate) fn to_device_messages(&self) -> String {
        if let AnyOutgoingRequest::ToDeviceRequest(request) = self.0.request() {
            serde_json::to_string(&request.messages).unwrap()
        } else {
            panic!()
        }
    }

    pub(crate) fn room_msg_room_id(&self) -> String {
        if let AnyOutgoingRequest::RoomMessage(request) = self.0.request() {
            request.room_id.to_string()
        } else {
            panic!()
        }
    }

    pub(crate) fn room_msg_txn_id(&self) -> String {
        if let AnyOutgoingRequest::RoomMessage(request) = self.0.request() {
            request.txn_id.to_string()
        } else {
            panic!()
        }
    }

    pub(crate) fn room_msg_content(&self) -> String {
        if let AnyOutgoingRequest::RoomMessage(request) = self.0.request() {
            serde_json::to_string(&request.content).unwrap()
        } else {
            panic!()
        }
    }

    pub(crate) fn room_msg_matrix_type(&self) -> String {
        if let AnyOutgoingRequest::RoomMessage(request) = self.0.request() {
            match *request.content {
                AnyMessageLikeEventContent::KeyVerificationReady(_) => "m.key.verification.ready",
                AnyMessageLikeEventContent::KeyVerificationStart(_) => "m.key.verification.start",
                AnyMessageLikeEventContent::KeyVerificationCancel(_) => "m.key.verification.cancel",
                AnyMessageLikeEventContent::KeyVerificationAccept(_) => "m.key.verification.accept",
                AnyMessageLikeEventContent::KeyVerificationKey(_) => "m.key.verification.key",
                AnyMessageLikeEventContent::KeyVerificationMac(_) => "m.key.verification.mac",
                AnyMessageLikeEventContent::KeyVerificationDone(_) => "m.key.verification.done",
                AnyMessageLikeEventContent::RoomEncrypted(_) => "m.room.encrypted",
                _ => {
                    error!(
                        "Requesting to send unexpected event type {:?}",
                        request.content
                    );
                    Default::default()
                }
            }
            .to_string()
        } else {
            panic!()
        }
    }

    pub(crate) fn upload_signature_signed_keys(&self) -> String {
        if let AnyOutgoingRequest::SignatureUpload(request) = self.0.request() {
            serde_json::to_string(&request.signed_keys).unwrap()
        } else {
            panic!()
        }
    }
}

impl ConfirmRequests {
    pub(crate) fn has_signature_request(&self) -> bool {
        self.signature.is_some()
    }

    pub(crate) fn verification_requests(&self) -> Vec<OutgoingKeyVerificationRequest> {
        self.verification.clone()
    }

    pub(crate) fn signature_request_content(&self) -> String {
        serde_json::to_string(&self.signature.as_ref().unwrap().signed_keys).unwrap()
    }
}

#[derive(Clone)]
pub(crate) struct ConfirmRequests {
    pub(crate) verification: Vec<OutgoingKeyVerificationRequest>,
    pub(crate) signature: Option<upload_signatures::v3::Request>,
}

#[derive(Clone)]
pub(crate) struct KeyVerificationRequest(pub(crate) VerificationRequest);

impl KeyVerificationRequest {
    pub(crate) fn remote_user_id(&self) -> String {
        self.0.other_user().to_string()
    }
    pub(crate) fn remote_device_id(&self) -> String {
        self.0
            .other_device_id()
            .map(|it| it.to_string())
            .unwrap_or(Default::default())
    }

    pub(crate) fn verification_id(&self) -> String {
        self.0.flow_id().as_str().into()
    }
}

#[derive(Clone)]
pub(crate) struct CrossSigningBootstrapRequests {
    pub bootstrap_requests: matrix_sdk_crypto::CrossSigningBootstrapRequests,
    pub secret_storage_key_id: String,
    pub(crate) backup_info: String,
    pub(crate) master: String,
    pub(crate) self_signing: String,
    pub(crate) user_signing: String,
    pub(crate) backup: String,
    pub(crate) backup_key: String,
    pub(crate) secret_storage_event_content: String,
    pub(crate) secret_storage_event_type: String,
}

impl CrossSigningBootstrapRequests {
    pub(crate) fn upload_signatures_content(&self) -> String {
        serde_json::to_string(&self.bootstrap_requests.upload_signatures_req.signed_keys).unwrap()
    }
    pub(crate) fn self_signing_key_json(&self) -> String {
        serde_json::to_string(&self.bootstrap_requests.upload_signing_keys_req.self_signing_key.as_ref().unwrap()).unwrap()
    }
    pub(crate) fn user_signing_key_json(&self) -> String {
        serde_json::to_string(&self.bootstrap_requests.upload_signing_keys_req.user_signing_key.as_ref().unwrap()).unwrap()
    }
    pub(crate) fn master_key_json(&self) -> String {
        serde_json::to_string(&self.bootstrap_requests.upload_signing_keys_req.master_key.as_ref().unwrap()).unwrap()
    }
    pub(crate) fn has_upload_keys_request(&self) -> bool {
        self.bootstrap_requests.upload_keys_req.is_some()
    }
    pub(crate) fn upload_keys_request(&self) -> Box<OutgoingRequest> {
        Box::new(OutgoingRequest(self.bootstrap_requests.upload_keys_req.clone().unwrap()))
    }

    pub(crate) fn secret_storage_key_id(&self) -> String {
        self.secret_storage_key_id.clone()
    }

    pub(crate) fn master(&self) -> String {
        self.master.clone()
    }
    pub(crate) fn self_signing(&self) -> String {
        self.self_signing.clone()
    }

    pub(crate) fn user_signing(&self) -> String {
        self.user_signing.clone()
    }

    pub(crate) fn backup(&self) -> String {
        self.backup.clone()
    }

    pub(crate) fn backup_key(&self) -> String { self.backup_key.clone() }

    pub(crate) fn secret_storage_event_content(&self) -> String { self.secret_storage_event_content.clone() }

    pub(crate) fn secret_storage_event_type(&self) -> String { self.secret_storage_event_type.clone() }

    pub(crate) fn backup_info(&self) -> String { self.backup_info.clone() }
}
