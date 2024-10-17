use std::{
    collections::BTreeMap, mem::ManuallyDrop, ops::Deref, path::Path, time::Duration
};
use serde::Deserialize;

use matrix_sdk_common::{deserialized_responses::VerificationState, ruma::{
    api::client::{
        backup::add_backup_keys,
        keys::{
            claim_keys::{self, v3::OneTimeKeys},
            get_keys, upload_keys,
            upload_signatures::{self, v3::Failure},
        },
        message::send_message_event,
        sync::sync_events::DeviceLists,
        to_device,
    }, encryption::{CrossSigningKey, DeviceKeys}, events::{AnyMessageLikeEventContent, AnyToDeviceEvent, AnyToDeviceEventContent}, serde::Raw, to_device::DeviceIdOrAllDevices, DeviceId, DeviceKeyAlgorithm, EventId, OwnedDeviceId, OwnedUserId, RoomId, UInt, UserId
}};

use matrix_sdk_crypto::{EncryptionSettings, EncryptionSyncChanges, IncomingResponse, OlmMachine, OutgoingRequests, OutgoingVerificationRequest, SasState, UserIdentities, Verification, VerificationRequest, VerificationRequestState};
use matrix_sdk_sqlite::SqliteCryptoStore;

struct CryptoMachine {
    runtime: tokio::runtime::Runtime,
    machine: Option<ManuallyDrop<OlmMachine>>,
}

impl Drop for CryptoMachine {
    fn drop(&mut self) {
        self.runtime.block_on(async {
            let machine = self.machine.take().unwrap();
            drop(ManuallyDrop::into_inner(machine));
        })
    }
}

fn init(user_id: String, device_id: String, path: String, pickle_key: String) -> Box<CryptoMachine> {
    let rt = tokio::runtime::Runtime::new().unwrap();
    let _ = rt.enter();

    let machine = rt.block_on(async {
        let user_id = UserId::parse(user_id).unwrap();
        let device_id: Box<DeviceId> = device_id.into();

        let store = SqliteCryptoStore::open(Path::new(&path), Some(&pickle_key)).await.unwrap();

        OlmMachine::with_store(&user_id, &device_id, store)
            .await
            .expect("Failed to load crypto database")
    });

    Box::new(CryptoMachine {
        runtime: rt,
        machine: Some(ManuallyDrop::new(machine)),
    })
}

struct KeysClaimRequest {
    id: String,
    timeout: Option<Duration>,
    one_time_keys: BTreeMap<OwnedUserId, BTreeMap<OwnedDeviceId, DeviceKeyAlgorithm>>,
}

impl KeysClaimRequest {
    fn id(&self) -> String {
        self.id.clone()
    }
    fn timeout(&self) -> usize {
        self.timeout.unwrap_or(Duration::from_millis(0)).as_millis() as usize
    }
    fn one_time_keys(&self) -> String {
        serde_json::to_string(&self.one_time_keys).unwrap()
    }
}

struct ToDeviceRequest {
    event_type: String,
    txn_id: String,
    messages: BTreeMap<OwnedUserId, BTreeMap<DeviceIdOrAllDevices, Raw<AnyToDeviceEventContent>>>,
}

impl ToDeviceRequest {
    fn event_type(&self) -> String {
        self.event_type.clone()
    }
    fn txn_id(&self) -> String {
        self.txn_id.clone()
    }
    fn messages(&self) -> String {
        serde_json::to_string(&self.messages).unwrap()
    }
}

struct OutgoingRequest(matrix_sdk_crypto::requests::OutgoingRequest);

impl OutgoingRequest {
    fn id(&self) -> String {
        self.0.request_id().to_string()
    }

    fn request_type(&self) -> u8 {
        match self.0.request() {
            OutgoingRequests::KeysUpload(_) => 0,
            OutgoingRequests::KeysQuery(_) => 1,
            OutgoingRequests::KeysClaim(_) => 2,
            OutgoingRequests::ToDeviceRequest(_) => 3,
            OutgoingRequests::SignatureUpload(_) => 4,
            OutgoingRequests::RoomMessage(_) => 5,
            OutgoingRequests::KeysBackup(_) => 6,
        }
    }

    fn keys_upload_device_keys(&self) -> String {
        if let OutgoingRequests::KeysUpload(request) = self.0.request() {
            serde_json::to_string(&request.device_keys).unwrap()
        } else {
            Default::default()
        }
    }

    fn keys_upload_fallback_keys(&self) -> String {
        if let OutgoingRequests::KeysUpload(request) = self.0.request() {
            serde_json::to_string(&request.fallback_keys).unwrap()
        } else {
            Default::default()
        }
    }

    fn keys_upload_one_time_keys(&self) -> String {
        if let OutgoingRequests::KeysUpload(request) = self.0.request() {
            serde_json::to_string(&request.one_time_keys).unwrap()
        } else {
            Default::default()
        }
    }

    fn keys_query_device_keys(&self) -> String {
        if let OutgoingRequests::KeysQuery(request) = self.0.request() {
            serde_json::to_string(&request.device_keys).unwrap()
        } else {
            Default::default()
        }
    }

    fn keys_query_timeout(&self) -> usize {
        if let OutgoingRequests::KeysQuery(request) = self.0.request() {
            request
                .timeout
                .unwrap_or(Duration::from_millis(0))
                .as_millis() as usize
        } else {
            Default::default()
        }
    }

    fn keys_claim_one_time_keys(&self) -> String {
        if let OutgoingRequests::KeysClaim(request) = self.0.request() {
            serde_json::to_string(&request.one_time_keys).unwrap()
        } else {
            Default::default()
        }
    }

    fn to_device_event_type(&self) -> String {
        if let OutgoingRequests::ToDeviceRequest(request) = self.0.request() {
            request.event_type.to_string()
        } else {
            Default::default()
        }
    }

    fn to_device_txn_id(&self) -> String {
        if let OutgoingRequests::ToDeviceRequest(request) = self.0.request() {
            request.txn_id.to_string()
        } else {
            Default::default()
        }
    }

    fn to_device_messages(&self) -> String {
        if let OutgoingRequests::ToDeviceRequest(request) = self.0.request() {
            serde_json::to_string(&request.messages).unwrap()
        } else {
            Default::default()
        }
    }

    fn room_msg_room_id(&self) -> String {
        if let OutgoingRequests::RoomMessage(request) = self.0.request() {
            request.room_id.to_string()
        } else {
            Default::default()
        }
    }

    fn room_msg_txn_id(&self) -> String {
        if let OutgoingRequests::RoomMessage(request) = self.0.request() {
            request.txn_id.to_string()
        } else {
            Default::default()
        }
    }

    fn room_msg_content(&self) -> String {
        if let OutgoingRequests::RoomMessage(request) = self.0.request() {
            serde_json::to_string(&request.content).unwrap()
        } else {
            Default::default()
        }
    }

    fn room_msg_matrix_type(&self) -> String {
        if let OutgoingRequests::RoomMessage(request) = self.0.request() {
            match request.content {
                AnyMessageLikeEventContent::KeyVerificationReady(_) => "m.key.verification.ready",
                AnyMessageLikeEventContent::KeyVerificationStart(_) => "m.key.verification.start",
                AnyMessageLikeEventContent::KeyVerificationCancel(_) => "m.key.verification.cancel",
                AnyMessageLikeEventContent::KeyVerificationAccept(_) => "m.key.verification.accept",
                AnyMessageLikeEventContent::KeyVerificationKey(_) => "m.key.verification.key",
                AnyMessageLikeEventContent::KeyVerificationMac(_) => "m.key.verification.mac",
                AnyMessageLikeEventContent::KeyVerificationDone(_) => "m.key.verification.done",
                AnyMessageLikeEventContent::RoomEncrypted(_) => "m.room.encrypted",
                _ => {
                    println!("Requesting to send unexpected event type {:?}", request.content);
                    Default::default() }
            }.to_string()
        } else {
            Default::default()
        }
    }

    fn upload_signature_signed_keys(&self) -> String {
        if let OutgoingRequests::SignatureUpload(request) = self.0.request() {
            serde_json::to_string(&request.signed_keys).unwrap()
        } else {
            Default::default()
        }
    }
}

impl CryptoMachine {
    fn outgoing_requests(&self) -> Vec<OutgoingRequest> {
        self.runtime.block_on(async {
            match self.machine.as_ref().expect("Should not happen")
                .outgoing_requests()
                .await {
                    Ok(requests) => {
                        requests.iter()
                        .map(|it| OutgoingRequest(it.clone()))
                        .collect()
                    }
                    Err(err) => {
                        println!("Failed to load outgoing requests");
                        println!("{:?}", err);
                        Default::default()
                    }
                }
        })
    }

    fn mark_keys_upload_as_sent(&mut self, response: String, request_id: String) {
        self.runtime.block_on(async {
            #[derive(serde::Deserialize)]
            struct Response {
                one_time_key_counts: BTreeMap<DeviceKeyAlgorithm, UInt>,
            }

            self.machine.as_ref().unwrap()
                .mark_request_as_sent(
                    request_id.as_str().into(),
                    IncomingResponse::KeysUpload(&upload_keys::v3::Response::new(
                        serde_json::from_str::<Response>(response.as_str())
                            .unwrap()
                            .one_time_key_counts,
                    )),
                )
                .await
                .unwrap();
        });
    }

    fn mark_keys_query_as_sent(&mut self, response: String, request_id: String) {
        self.runtime.block_on(async {
            #[derive(Deserialize)]
            struct Response {
                failures: Option<BTreeMap<String, serde_json::Value>>,
                device_keys:
                    Option<BTreeMap<OwnedUserId, BTreeMap<OwnedDeviceId, Raw<DeviceKeys>>>>,
                master_keys: Option<BTreeMap<OwnedUserId, Raw<CrossSigningKey>>>,
                self_signing_keys: Option<BTreeMap<OwnedUserId, Raw<CrossSigningKey>>>,
                user_signing_keys: Option<BTreeMap<OwnedUserId, Raw<CrossSigningKey>>>,
            }
            let r: Response = serde_json::from_str(response.as_str()).unwrap();
            let mut response = get_keys::v3::Response::new();
            response.failures = r.failures.unwrap_or_default();
            response.device_keys = r.device_keys.unwrap_or_default();
            response.master_keys = r.master_keys.unwrap_or_default();
            response.self_signing_keys = r.self_signing_keys.unwrap_or_default();
            response.user_signing_keys = r.user_signing_keys.unwrap_or_default();
            self.machine.as_ref().unwrap()
                .mark_request_as_sent(
                    request_id.as_str().into(),
                    IncomingResponse::KeysQuery(&response),
                )
                .await
                .unwrap();
        });
    }

    fn mark_keys_claim_as_sent(&mut self, response: String, request_id: String) {
        self.runtime.block_on(async {
            #[derive(Deserialize)]
            struct Response {
                failures: Option<BTreeMap<String, serde_json::Value>>,
                one_time_keys: Option<BTreeMap<OwnedUserId, OneTimeKeys>>,
            }
            let r: Response = serde_json::from_str(response.as_str()).unwrap();
            let mut request = claim_keys::v3::Response::new(r.one_time_keys.unwrap_or_default());
            request.failures = r.failures.unwrap_or_default();
            self.machine.as_ref().unwrap()
                .mark_request_as_sent(
                    request_id.as_str().into(),
                    IncomingResponse::KeysClaim(&request),
                )
                .await
                .unwrap();
        });
    }

    fn mark_to_device_as_sent(&mut self, _: String, request_id: String) {
        self.runtime.block_on(async {
            self.machine.as_ref().unwrap()
                .mark_request_as_sent(
                    request_id.as_str().into(),
                    IncomingResponse::ToDevice(
                        &to_device::send_event_to_device::v3::Response::new(/* no content */),
                    ),
                )
                .await
                .unwrap();
        });
    }

    fn mark_signature_upload_as_sent(&mut self, response: String, request_id: String) {
        self.runtime.block_on(async {
            #[derive(Deserialize)]
            struct Response {
                failures: Option<BTreeMap<OwnedUserId, BTreeMap<String, Failure>>>,
            }
            let r: Response = serde_json::from_str(response.as_str()).unwrap();
            let mut request = upload_signatures::v3::Response::new();
            request.failures = r.failures.unwrap_or_default();
            self.machine.as_ref().unwrap()
                .mark_request_as_sent(
                    request_id.as_str().into(),
                    IncomingResponse::SignatureUpload(&request),
                )
                .await
                .unwrap();
        });
    }

    fn mark_room_message_as_sent(&mut self, event_id: String, request_id: String) {
        self.runtime.block_on(async {
            self.machine.as_ref().unwrap()
                .mark_request_as_sent(
                    request_id.as_str().into(),
                    IncomingResponse::RoomMessage(&send_message_event::v3::Response::new(EventId::parse(event_id).unwrap())),
                )
                .await
                .unwrap();
        });
    }

    fn mark_keys_backup_as_sent(&mut self, response: String, request_id: String) {
        self.runtime.block_on(async {
            #[derive(Deserialize)]
            struct Response {
                etag: String,
                count: UInt,
            }
            let r: Response = serde_json::from_str(response.as_str()).unwrap();
            self.machine.as_ref().unwrap()
                .mark_request_as_sent(
                    request_id.as_str().into(),
                    IncomingResponse::KeysBackup(&add_backup_keys::v3::Response::new(
                        r.etag, r.count,
                    )),
                )
                .await
                .unwrap();
        });
    }

    fn receive_sync_changes(&mut self, sync_json: String) -> Vec<KeyVerificationRequest> {
        self.runtime.block_on(async {
            #[derive(serde::Deserialize)]
            struct SyncChanges {
                to_device: Option<BTreeMap<String, Vec<Raw<AnyToDeviceEvent>>>>,
                device_lists: Option<DeviceLists>,
                device_one_time_keys_count: BTreeMap<DeviceKeyAlgorithm, UInt>,
                next_batch: Option<String>,
            }

            let changes = serde_json::from_str::<SyncChanges>(sync_json.as_str()).unwrap();

            let changes = self.machine.as_ref().unwrap()
                .receive_sync_changes(EncryptionSyncChanges {
                    to_device_events: if let Some(events) = changes.to_device {
                        events["events"].clone()
                    } else {
                        Default::default()
                    },
                    changed_devices: &changes.device_lists.unwrap_or_default(),
                    one_time_keys_counts: &changes.device_one_time_keys_count,
                    unused_fallback_keys: None, //TODO
                    next_batch_token: changes.next_batch,
                })
                .await
                .unwrap();


            let mut events = vec!();
            for to_device_event in changes.0 {
                if let AnyToDeviceEvent::KeyVerificationRequest(request) = to_device_event.deserialize().unwrap() {
                    events.push(KeyVerificationRequest(self.machine.as_ref().unwrap().get_verification_request(&request.sender, request.content.transaction_id).unwrap()));
                }
            }
            events
        })
    }

    fn share_room_key(&mut self, room_id: String, user_ids: Vec<String>) -> Vec<ToDeviceRequest> {
        self.runtime
            .block_on(async {
                let room_id = RoomId::parse(room_id).unwrap();
                let user_ids: Vec<matrix_sdk_common::ruma::OwnedUserId> = user_ids
                    .iter()
                    .map(|it| UserId::parse(it).unwrap())
                    .collect();
                self.machine.as_ref().unwrap()
                    .share_room_key(
                        &room_id,
                        user_ids.iter().map(Deref::deref),
                        EncryptionSettings::default(),
                    )
                    .await
                    .unwrap() //TODO settings?
            })
            .iter()
            .map(|it| ToDeviceRequest {
                txn_id: it.txn_id.to_string(),
                event_type: it.event_type.to_string(),
                messages: it.messages.clone(),
            })
            .collect()
    }

    //TODO lock
    fn get_missing_sessions(&mut self, user_ids: Vec<String>) -> Box<KeysClaimRequest> {
        self.runtime.block_on(async {
            let user_ids: Vec<matrix_sdk_common::ruma::OwnedUserId> = user_ids
                .iter()
                .map(|it| UserId::parse(it).unwrap())
                .collect();
            if let Some((id, request)) = self
                .machine.as_ref().unwrap()
                .get_missing_sessions(user_ids.iter().map(Deref::deref))
                .await
                .unwrap()
            {
                Box::new(KeysClaimRequest {
                    id: id.to_string(),
                    timeout: request.timeout,
                    one_time_keys: request.one_time_keys,
                })
            } else {
                Box::new(KeysClaimRequest {
                    id: "".to_string(),
                    timeout: None,
                    one_time_keys: Default::default(),
                })
            }
        })
    }

    fn encrypt_room_event(&mut self, room_id: String, content: String, matrix_type: String) -> String {
        self.runtime.block_on(async {
            let room_id = RoomId::parse(room_id).unwrap();
            serde_json::to_string(
                &self
                    .machine.as_ref().unwrap()
                    //TODO: Don't hardcode this
                    .encrypt_room_event_raw(
                        &room_id,
                        &matrix_type,
                        &serde_json::from_str(content.as_str()).unwrap(),
                    )
                    .await
                    .unwrap(),
            )
            .unwrap()
        })
    }

    fn decrypt_room_event(&mut self, room_id: String, json: String) -> String {
        self.runtime.block_on(async {
            let room_id = RoomId::parse(room_id).unwrap();
            if let Ok(event) = self
                .machine.as_ref().unwrap()
                .decrypt_room_event(&serde_json::from_str(json.as_str()).unwrap(), &room_id)
                .await
            {
                serde_json::to_string(&event.event).unwrap()
            } else {
                Default::default()
            }
        })
    }

    fn update_tracked_users(&mut self, user_ids: Vec<String>) {
        self.runtime.block_on(async {
            let user_ids: Vec<matrix_sdk_common::ruma::OwnedUserId> = user_ids
                .iter()
                .map(|it| UserId::parse(it).unwrap())
                .collect();
            self.machine.as_ref().unwrap()
                .update_tracked_users(user_ids.iter().map(Deref::deref))
                .await
                .unwrap();
        });
    }

    fn accept_verification(&mut self, remote_user: String, verification_id: String) -> Box<OutgoingKeyVerificationRequest> {
        let user_id = UserId::parse(remote_user).unwrap();
        //TODO: why is there sometimes no request?
        Box::new(OutgoingKeyVerificationRequest(self.machine.as_ref().unwrap().get_verification_request(&user_id, &verification_id).unwrap().accept().unwrap()))
    }

    fn confirm_verification(&mut self, remote_user: String, verification_id: String) -> Vec<OutgoingKeyVerificationRequest> {
        self.runtime.block_on(async {
            //TODO signature upload
            let user_id = UserId::parse(remote_user).unwrap();
            if let VerificationRequestState::Transitioned{ verification } = self.machine.as_ref().unwrap().get_verification_request(&user_id, &verification_id).unwrap().state() {
                if let Verification::SasV1(sas) = verification {
                    sas.confirm().await.unwrap().0.iter().map(|it| OutgoingKeyVerificationRequest(it.clone())).collect()
                } else {
                    panic!()
                }
            } else {
                panic!()
            }
        })
    }

    fn start_sas(&mut self, remote_user: String, verification_id: String) -> Box<OutgoingKeyVerificationRequest> {
        self.runtime.block_on(async {
            let user_id = UserId::parse(&remote_user).unwrap();
            let result = self.machine.as_ref().unwrap().get_verification_request(&user_id, &verification_id).unwrap().start_sas().await.unwrap().unwrap();
            Box::new(OutgoingKeyVerificationRequest(result.1))
        })
    }

    fn accept_sas(self: &mut CryptoMachine, remote_user: String, verification_id: String) -> Box<OutgoingKeyVerificationRequest> {
        let user_id = UserId::parse(remote_user).unwrap();
        let Some(session) = self.machine.as_ref().unwrap().get_verification_request(&user_id, &verification_id) else {
            panic!()
        };

        if let VerificationRequestState::Transitioned{ verification } = session.state() {
            if let Verification::SasV1(sas) = verification {
                Box::new(OutgoingKeyVerificationRequest(sas.accept().unwrap()))
            } else {
                panic!()
            }
        } else {
            panic!()
        }
    }


    fn verification_get_state(&mut self, remote_user: String, verification_id: String) -> u8 {
        let user_id = UserId::parse(remote_user).unwrap();
        match self.machine.as_ref().expect("Should not happen").get_verification_request(&user_id, &verification_id) {
            Some(session) => {
                match session.state() {
                    VerificationRequestState::Created { .. } => 0,
                    VerificationRequestState::Requested { .. } => 1,
                    VerificationRequestState::Ready { .. } => 2,
                    VerificationRequestState::Transitioned { .. } => 3,
                    VerificationRequestState::Done => 4,
                    VerificationRequestState::Cancelled(_) => 5,
                }
            }
            None => {
                6
            },
        }
    }

    fn sas_get_state(&mut self, remote_user: String, verification_id: String) -> u8 {
        let user_id = UserId::parse(remote_user).unwrap();
        let Some(session) = self.machine.as_ref().unwrap().get_verification_request(&user_id, &verification_id) else {
            return 6;
        };

        if let VerificationRequestState::Transitioned{ verification } = session.state() {
            if let Verification::SasV1(sas) = verification {
                match sas.state() {
                    SasState::Started { .. } => 0,
                    SasState::Accepted { .. } => 1,
                    SasState::KeysExchanged { .. } => 2,
                    SasState::Confirmed => 3,
                    SasState::Done { .. } => 4,
                    SasState::Cancelled(_) => 5,
                }
            } else {
                panic!()
            }
        } else {
            // this is (in the current setup) mostly normal, since we're always querying sas state.
            6
        }
    }

    fn sas_emoji(&self, remote_user: String, verification_id: String) -> Vec<Emoji> {
        let user_id = UserId::parse(remote_user).unwrap();
        if let VerificationRequestState::Transitioned{ verification } = self.machine.as_ref().unwrap().get_verification_request(&user_id, &verification_id).unwrap().state() {
            if let Verification::SasV1(sas) = verification {
                sas.emoji().expect("Emoji can't be presented yet").iter().map(|e| Emoji(e.clone())).collect()
            } else {
                panic!()
            }
        } else {
            panic!()
        }
    }

    fn request_device_verification(&mut self, user_id: String, device_id: String) -> Box<CreatedSession> {
        self.runtime.block_on(async {
            let user_id = UserId::parse(user_id).unwrap();
            let device_id: Box<DeviceId> = device_id.into();
            let device = self.machine.as_ref().unwrap().get_device(&user_id, &device_id, None).await.unwrap().unwrap();
            let (session, outgoing) = device.request_verification().await;
            Box::new(CreatedSession(session, outgoing))
        })
    }

    fn request_user_verification(&mut self, user_id: String, room_id: String, request_event_id: String) -> Box<KeyVerificationRequest> {
        self.runtime.block_on(async {
            let user_id = UserId::parse(user_id).unwrap();
            let room_id = RoomId::parse(room_id).unwrap();
            let event_id = EventId::parse(request_event_id).unwrap();
            let identity = self.machine.as_ref().unwrap().get_identity(&user_id, None).await.unwrap().unwrap();
            if let UserIdentities::Other(other) = identity {
                Box::new(KeyVerificationRequest(other.request_verification(&room_id, &event_id, None /*TODO?*/).await))
            } else {
                panic!()
            }
        })
    }

    fn request_user_verification_content(&mut self, user_id: String) -> String {
        self.runtime.block_on(async {
            let user_id = UserId::parse(user_id).unwrap();
            let identity = self.machine.as_ref().unwrap().get_identity(&user_id, None).await.unwrap().unwrap();
            if let UserIdentities::Other(other) = identity {
                serde_json::to_string(&other.verification_request_content(None /*TODO ?*/).await).unwrap()
            } else {
                Default::default()
            }
        })
    }


    fn get_room_event_encryption_info(&self, event: String, room_id: String) -> Box<EncryptionInfo> {
        self.runtime.block_on(async {
            let room_id = RoomId::parse(room_id).unwrap();
            let info = self.machine.as_ref().unwrap().get_room_event_encryption_info(&serde_json::from_str(&event).unwrap(), &room_id).await.unwrap();
            Box::new(EncryptionInfo(info))
        })
    }

    fn receive_verification_event(&mut self, full_json: String) {
        self.runtime.block_on(async {
            self.machine.as_ref().unwrap().receive_verification_event(&serde_json::from_str(&full_json).unwrap()).await.unwrap();
        })
    }
}

struct CreatedSession(VerificationRequest, OutgoingVerificationRequest);

impl CreatedSession {
    fn verification_id(&self) -> String {
        self.0.flow_id().as_str().to_string()
    }
    fn to_device_event_type(&self) -> String {
        if let OutgoingVerificationRequest::ToDevice(request) = &self.1 {
            request.event_type.to_string()
        } else {
            Default::default()
        }
    }

    fn to_device_txn_id(&self) -> String {
        if let OutgoingVerificationRequest::ToDevice(request) = &self.1 {
            request.txn_id.to_string()
        } else {
            Default::default()
        }
    }

    fn to_device_messages(&self) -> String {
        if let OutgoingVerificationRequest::ToDevice(request) = &self.1 {
            serde_json::to_string(&request.messages).unwrap()
        } else {
            Default::default()
        }
    }
}

struct Emoji(matrix_sdk_crypto::Emoji);

impl Emoji {
    fn symbol(&self) -> String {
        self.0.symbol.to_string()
    }
    fn description(&self) -> String {
        self.0.description.to_string()
    }
}

struct OutgoingKeyVerificationRequest(OutgoingVerificationRequest);

impl OutgoingKeyVerificationRequest {
    //TODO type

    //TODO: deduplicate with other things
    fn to_device_event_type(&self) -> String {
        if let OutgoingVerificationRequest::ToDevice(request) = &self.0 {
            request.event_type.to_string()
        } else {
            panic!()
        }
    }

    fn to_device_txn_id(&self) -> String {
        if let OutgoingVerificationRequest::ToDevice(request) = &self.0 {
            request.txn_id.to_string()
        } else {
            panic!()
        }
    }

    fn to_device_messages(&self) -> String {
        if let OutgoingVerificationRequest::ToDevice(request) = &self.0 {
            serde_json::to_string(&request.messages).unwrap()
        } else {
            panic!()
        }
    }

    fn in_room_room_id(&self) -> String {
        if let OutgoingVerificationRequest::InRoom(request) = &self.0 {
            request.room_id.to_string()
        } else {
            panic!()
        }
    }

    fn in_room_txn_id(&self) -> String {
        if let OutgoingVerificationRequest::InRoom(request) = &self.0 {
            request.txn_id.to_string()
        } else {
            panic!()
        }
    }

    fn in_room_content(&self) -> String {
        if let OutgoingVerificationRequest::InRoom(request) = &self.0 {
            serde_json::to_string(&request.content).unwrap()
        } else {
            panic!()
        }
    }

    fn in_room_event_type(&self) -> String {
        if let OutgoingVerificationRequest::InRoom(request) = &self.0 {
            match request.content {
                AnyMessageLikeEventContent::KeyVerificationReady(_) => "m.key.verification.ready",
                AnyMessageLikeEventContent::KeyVerificationStart(_) => "m.key.verification.start",
                AnyMessageLikeEventContent::KeyVerificationCancel(_) => "m.key.verification.cancel",
                AnyMessageLikeEventContent::KeyVerificationAccept(_) => "m.key.verification.accept",
                AnyMessageLikeEventContent::KeyVerificationKey(_) => "m.key.verification.key",
                AnyMessageLikeEventContent::KeyVerificationMac(_) => "m.key.verification.mac",
                AnyMessageLikeEventContent::KeyVerificationDone(_) => "m.key.verification.done",
                AnyMessageLikeEventContent::RoomEncrypted(_) => "m.room.encrypted",
                _ => {
                    println!("Requesting to send unexpected event type {:?}", request.content);
                    Default::default() }
            }.to_string()
        } else {
            panic!()
        }
    }
}

struct KeyVerificationRequest(VerificationRequest);

impl KeyVerificationRequest {
    fn remote_user_id(&self) -> String {
        self.0.other_user().to_string()
    }
    fn remote_device_id(&self) -> String {
        self.0.other_device_id().map(|it| it.to_string()).unwrap_or(Default::default()) //TODO: or panic?
    }

    fn verification_id(&self) -> String {
        self.0.flow_id().as_str().into()
    }
}

struct EncryptionInfo(matrix_sdk_common::deserialized_responses::EncryptionInfo);

impl EncryptionInfo {
    fn is_verified(&self) -> bool {
        matches!(self.0.verification_state, VerificationState::Verified)
    }
}

#[cxx::bridge]
mod ffi {
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

        // General CryptoMachine functions
        fn init(user_id: String, device_id: String, path: String, pickle_key: String) -> Box<CryptoMachine>;
        fn outgoing_requests(self: &CryptoMachine) -> Vec<OutgoingRequest>;
        fn receive_sync_changes(self: &mut CryptoMachine, sync_json: String) -> Vec<KeyVerificationRequest>;
        fn receive_verification_event(self: &mut CryptoMachine, full_json: String);

        // Mark requests as sent
        fn mark_keys_upload_as_sent(self: &mut CryptoMachine, request_id: String, response: String);
        fn mark_keys_query_as_sent(self: &mut CryptoMachine, request_id: String, response: String);
        fn mark_keys_claim_as_sent(self: &mut CryptoMachine, request_id: String, response: String);
        fn mark_to_device_as_sent(self: &mut CryptoMachine, request_id: String, response: String);
        fn mark_room_message_as_sent(self: &mut CryptoMachine, request_id: String, response: String);
        fn mark_signature_upload_as_sent(self: &mut CryptoMachine, request_id: String, response: String);
        fn mark_keys_backup_as_sent(self: &mut CryptoMachine, request_id: String, response: String);

        // Specific CryptoMachine functions
        fn share_room_key(self: &mut CryptoMachine, room_id: String, user_ids: Vec<String>) -> Vec<ToDeviceRequest>;
        fn get_missing_sessions(self: &mut CryptoMachine, user_ids: Vec<String>) -> Box<KeysClaimRequest>;
        fn encrypt_room_event(self: &mut CryptoMachine, room_id: String, content: String, matrix_type: String) -> String;
        fn decrypt_room_event(self: &mut CryptoMachine, room_id: String, json: String) -> String;
        fn update_tracked_users(self: &mut CryptoMachine, user_ids: Vec<String>);
        fn request_device_verification(self: &mut CryptoMachine, user_id: String, device_id: String) -> Box<CreatedSession>;
        fn request_user_verification(self: &mut CryptoMachine, user_id: String, room_id: String, event_id: String) -> Box<KeyVerificationRequest>;
        fn request_user_verification_content(self: &mut CryptoMachine, user_id: String) -> String;
        fn get_room_event_encryption_info(self: &CryptoMachine, event: String, room_id: String) -> Box<EncryptionInfo>;

        // Gettings parts of an OutgoingRequest
        fn id(self: &OutgoingRequest) -> String;
        fn keys_upload_device_keys(self: &OutgoingRequest) -> String;
        fn keys_upload_one_time_keys(self: &OutgoingRequest) -> String;
        fn keys_upload_fallback_keys(self: &OutgoingRequest) -> String;
        fn keys_query_device_keys(self: &OutgoingRequest) -> String;
        fn keys_query_timeout(self: &OutgoingRequest) -> usize;
        fn keys_claim_one_time_keys(self: &OutgoingRequest) -> String;
        fn to_device_event_type(self: &OutgoingRequest) -> String;
        fn to_device_messages(self: &OutgoingRequest) -> String;
        fn upload_signature_signed_keys(self: &OutgoingRequest) -> String;
        fn request_type(self: &OutgoingRequest) -> u8;
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

        fn accept_verification(self: &mut CryptoMachine, remote_user: String, verification_id: String) -> Box<OutgoingKeyVerificationRequest>;
        fn confirm_verification(self: &mut CryptoMachine, remote_user: String, verification_id: String) -> Vec<OutgoingKeyVerificationRequest>;
        fn start_sas(self: &mut CryptoMachine, remote_user: String, verification_id: String) -> Box<OutgoingKeyVerificationRequest>;
        fn accept_sas(self: &mut CryptoMachine, remote_user: String, verification_id: String) -> Box<OutgoingKeyVerificationRequest>;
        fn verification_get_state(self: &mut CryptoMachine, remote_user: String, verification_id: String) -> u8;
        fn sas_get_state(self: &mut CryptoMachine, remote_user: String, verification_id: String) -> u8;
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

        fn sas_emoji(self: &CryptoMachine, remote_user: String, verification_id: String) -> Vec<Emoji>;
        fn symbol(self: &Emoji) -> String;
        fn description(self: &Emoji) -> String;

        fn to_device_event_type(self: &CreatedSession) -> String;
        fn to_device_txn_id(self: &CreatedSession) -> String;
        fn to_device_messages(self: &CreatedSession) -> String;
        fn verification_id(self: &CreatedSession) -> String;

        fn is_verified(self: &EncryptionInfo) -> bool;
    }
}
