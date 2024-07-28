#include "GodotNetworkingSockets.h"
#include "steam/isteamnetworkingutils.h"
#include <chrono>
#include <cstdarg>

constexpr uint16_t NUM_LANES = 2;
const int32_t LANE_PRIORITIES[NUM_LANES] = { 1, 0 };

void GodotNetSockets::_bind_methods() {
	ClassDB::bind_static_method("GodotNetSockets", D_METHOD("set_print_function", "func"), &GodotNetSockets::SetPrintFunction);
	ClassDB::bind_static_method("GodotNetSockets", D_METHOD("init"), &GodotNetSockets::Init);
	ClassDB::bind_static_method("GodotNetSockets", D_METHOD("get_identity"), &GodotNetSockets::GetIdentity);

	ClassDB::bind_integer_constant("GodotNetSockets", "Flags", "Unreliable", k_nSteamNetworkingSend_Unreliable, true);
	ClassDB::bind_integer_constant("GodotNetSockets", "Flags", "NoNagle", k_nSteamNetworkingSend_NoNagle, true);
	ClassDB::bind_integer_constant("GodotNetSockets", "Flags", "UnreliableNoNagle", k_nSteamNetworkingSend_UnreliableNoNagle, true);
	ClassDB::bind_integer_constant("GodotNetSockets", "Flags", "NoDelay", k_nSteamNetworkingSend_NoDelay, true);
	ClassDB::bind_integer_constant("GodotNetSockets", "Flags", "UnreliableNoDelay", k_nSteamNetworkingSend_UnreliableNoDelay, true);
	ClassDB::bind_integer_constant("GodotNetSockets", "Flags", "Reliable", k_nSteamNetworkingSend_Reliable, true);
	ClassDB::bind_integer_constant("GodotNetSockets", "Flags", "ReliableNoNagle", k_nSteamNetworkingSend_ReliableNoNagle, true);
}

Callable printFunction;

void GodotNetSockets::SetPrintFunction(Callable func) {
	printFunction = func;
}

#if 1
inline void GDPrint(const char *format, ...) {
	if (printFunction.is_null()) {
		return;
	}

	char buffer[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	printFunction.call(String(buffer));
}
#else
inline void GDPrint(const char *format, ...) {
}
#endif

ISteamNetworkingSockets *pInterface = nullptr;
ISteamNetworkingUtils *pUtils = nullptr;

bool GodotNetSockets::Init() {
	SteamDatagramErrMsg errMsg;
	if (!GameNetworkingSockets_Init(nullptr, errMsg)) {
		GDPrint("Failed to initialize game networking sockets: %s", errMsg);
		return false;
	}

	pInterface = SteamNetworkingSockets();
	pUtils = SteamNetworkingUtils();

	GDPrint("Game networking sockets initialized");
	return true;
}

String GodotNetSockets::GetIdentity() {
	if (pInterface == nullptr) {
		return "";
	}

	SteamNetworkingIdentity identity;
	if (!pInterface->GetIdentity(&identity)) {
		return "";
	}

	char identityMsg[64];
	identity.ToString(identityMsg, sizeof(identityMsg));

	return String(identityMsg);
}

void GodotNetMessageOut::_bind_methods() {
	ClassDB::bind_static_method("GodotNetMessageOut", D_METHOD("build", "sizeBytes"), &GodotNetMessageOut::Build);

	ClassDB::bind_method(D_METHOD("set_bool", "value"), &GodotNetMessageOut::SetBool);
	ClassDB::bind_method(D_METHOD("set_int8", "value"), &GodotNetMessageOut::SetInt8);
	ClassDB::bind_method(D_METHOD("set_int16", "value"), &GodotNetMessageOut::SetInt16);
	ClassDB::bind_method(D_METHOD("set_int32", "value"), &GodotNetMessageOut::SetInt32);
	ClassDB::bind_method(D_METHOD("set_int64", "value"), &GodotNetMessageOut::SetInt64);
	ClassDB::bind_method(D_METHOD("set_uint8", "value"), &GodotNetMessageOut::SetUInt8);
	ClassDB::bind_method(D_METHOD("set_uint16", "value"), &GodotNetMessageOut::SetUInt16);
	ClassDB::bind_method(D_METHOD("set_uint32", "value"), &GodotNetMessageOut::SetUInt32);
	ClassDB::bind_method(D_METHOD("set_uint64", "value"), &GodotNetMessageOut::SetUInt64);
	ClassDB::bind_method(D_METHOD("set_float", "value"), &GodotNetMessageOut::SetFloat);
	ClassDB::bind_method(D_METHOD("set_double", "value"), &GodotNetMessageOut::SetDouble);
	ClassDB::bind_method(D_METHOD("set_vector2", "value"), &GodotNetMessageOut::SetVector2);
	ClassDB::bind_method(D_METHOD("set_vector3", "value"), &GodotNetMessageOut::SetVector3);
	ClassDB::bind_method(D_METHOD("set_quaternion", "value"), &GodotNetMessageOut::SetQuaternion);
}

GodotNetMessageOut::GodotNetMessageOut() {
	m_sizeBytes = 0;
	m_realSize = 0;
	m_data = nullptr;
	m_ownsData = true;
}

Ref<GodotNetMessageOut> GodotNetMessageOut::Build(uint32_t sizeBytes) {
	Ref<GodotNetMessageOut> msg = memnew(GodotNetMessageOut);

	msg->m_sizeBytes = sizeBytes;
	msg->m_realSize = 0;
	msg->m_data = memalloc(sizeBytes);

	return msg;
}

GodotNetMessageOut::~GodotNetMessageOut() {
	if (m_ownsData) {
		memfree(m_data);
	}
}

void *GodotNetMessageOut::GetData() {
	return m_data;
}

uint32_t GodotNetMessageOut::GetSize() {
	return m_realSize;
}

void GodotNetMessageOut::RemoveOwnership() {
	m_ownsData = false;
}

#define SET_IMPL(name, type)                                      \
	void GodotNetMessageOut::Set##name##(type value) {            \
		if (m_realSize + sizeof(type) > m_sizeBytes) {            \
			GDPrint("Failed to set, not enough space in buffer"); \
			return;                                               \
		}                                                         \
                                                                  \
		*(type *)((uint8_t *)m_data + m_realSize) = value;        \
		m_realSize += sizeof(type);                               \
	}

SET_IMPL(Bool, bool)
SET_IMPL(Int8, int8_t)
SET_IMPL(Int16, int16_t)
SET_IMPL(Int32, int32_t)
SET_IMPL(Int64, int64_t)
SET_IMPL(UInt8, uint8_t)
SET_IMPL(UInt16, uint16_t)
SET_IMPL(UInt32, uint32_t)
SET_IMPL(UInt64, uint64_t)
SET_IMPL(Float, float)
SET_IMPL(Double, double)

void GodotNetMessageOut::SetVector2(Vector2 value) {
	if (m_realSize + sizeof(float) * 2 > m_sizeBytes) {
		GDPrint("Failed to set, not enough space in buffer");
		return;
	}

	*(float *)((uint8_t *)m_data + m_realSize) = value.x;
	m_realSize += sizeof(float);
	*(float *)((uint8_t *)m_data + m_realSize) = value.y;
	m_realSize += sizeof(float);
}

void GodotNetMessageOut::SetVector3(Vector3 value) {
	if (m_realSize + sizeof(float) * 3 > m_sizeBytes) {
		GDPrint("Failed to set, not enough space in buffer");
		return;
	}

	*(float *)((uint8_t *)m_data + m_realSize) = value.x;
	m_realSize += sizeof(float);
	*(float *)((uint8_t *)m_data + m_realSize) = value.y;
	m_realSize += sizeof(float);
	*(float *)((uint8_t *)m_data + m_realSize) = value.z;
	m_realSize += sizeof(float);
}

void GodotNetMessageOut::SetQuaternion(Quaternion value) {
	if (m_realSize + sizeof(float) * 4 > m_sizeBytes) {
		GDPrint("Failed to set, not enough space in buffer");
		return;
	}

	*(float *)((uint8_t *)m_data + m_realSize) = value.x;
	m_realSize += sizeof(float);
	*(float *)((uint8_t *)m_data + m_realSize) = value.y;
	m_realSize += sizeof(float);
	*(float *)((uint8_t *)m_data + m_realSize) = value.z;
	m_realSize += sizeof(float);
	*(float *)((uint8_t *)m_data + m_realSize) = value.w;
	m_realSize += sizeof(float);
}

void GodotNetMessageIn::_bind_methods() {
	ClassDB::bind_method(D_METHOD("release"), &GodotNetMessageIn::Release);

	ClassDB::bind_method(D_METHOD("get_bool"), &GodotNetMessageIn::GetBool);
	ClassDB::bind_method(D_METHOD("get_int8"), &GodotNetMessageIn::GetInt8);
	ClassDB::bind_method(D_METHOD("get_int16"), &GodotNetMessageIn::GetInt16);
	ClassDB::bind_method(D_METHOD("get_int32"), &GodotNetMessageIn::GetInt32);
	ClassDB::bind_method(D_METHOD("get_int64"), &GodotNetMessageIn::GetInt64);
	ClassDB::bind_method(D_METHOD("get_uint8"), &GodotNetMessageIn::GetUInt8);
	ClassDB::bind_method(D_METHOD("get_uint16"), &GodotNetMessageIn::GetUInt16);
	ClassDB::bind_method(D_METHOD("get_uint32"), &GodotNetMessageIn::GetUInt32);
	ClassDB::bind_method(D_METHOD("get_uint64"), &GodotNetMessageIn::GetUInt64);
	ClassDB::bind_method(D_METHOD("get_float"), &GodotNetMessageIn::GetFloat);
	ClassDB::bind_method(D_METHOD("get_double"), &GodotNetMessageIn::GetDouble);
	ClassDB::bind_method(D_METHOD("get_vector2"), &GodotNetMessageIn::GetVector2);
	ClassDB::bind_method(D_METHOD("get_vector3"), &GodotNetMessageIn::GetVector3);
	ClassDB::bind_method(D_METHOD("get_quaternion"), &GodotNetMessageIn::GetQuaternion);
}

GodotNetMessageIn::GodotNetMessageIn() {
	m_data = nullptr;
	m_sizeBytes = 0;
	m_pointer = 0;
	m_msg = nullptr;
	m_released = false;
}

GodotNetMessageIn::GodotNetMessageIn(ISteamNetworkingMessage *msg) {
	m_data = msg->m_pData;
	m_sizeBytes = msg->m_cbSize;
	m_pointer = 0;
	m_msg = msg;
	m_released = false;
}

GodotNetMessageIn::~GodotNetMessageIn() {
	if (!m_released) {
		GDPrint("GodotNetMessageIn was not released, releasing now");
		m_msg->Release();
	}
}

void GodotNetMessageIn::Release() {
	if (m_released) {
		GDPrint("GodotNetMessageIn was already released");
		return;
	}

	m_msg->Release();
	m_released = true;
}

#define GET_IMPL(name, type)                                   \
	type GodotNetMessageIn::Get##name##() {                    \
		if (m_pointer + sizeof(type) > m_sizeBytes) {          \
			GDPrint("Failed to get, out of bounds");           \
			return type();                                     \
		}                                                      \
                                                               \
		if (m_released) {                                      \
			GDPrint("Failed to get, message was released");    \
			return type();                                     \
		}                                                      \
                                                               \
		type value = *(type *)((uint8_t *)m_data + m_pointer); \
		m_pointer += sizeof(type);                             \
		return value;                                          \
	}

GET_IMPL(Bool, bool)
GET_IMPL(Int8, int8_t)
GET_IMPL(Int16, int16_t)
GET_IMPL(Int32, int32_t)
GET_IMPL(Int64, int64_t)
GET_IMPL(UInt8, uint8_t)
GET_IMPL(UInt16, uint16_t)
GET_IMPL(UInt32, uint32_t)
GET_IMPL(UInt64, uint64_t)
GET_IMPL(Float, float)
GET_IMPL(Double, double)

Vector2 GodotNetMessageIn::GetVector2() {
	if (m_pointer + sizeof(float) * 2 > m_sizeBytes) {
		GDPrint("Failed to get, out of bounds");
		return Vector2();
	}

	if (m_released) {
		GDPrint("Failed to get, message was released");
		return Vector2();
	}

	float x = *(float *)((uint8_t *)m_data + m_pointer);
	m_pointer += sizeof(float);
	float y = *(float *)((uint8_t *)m_data + m_pointer);
	m_pointer += sizeof(float);

	return Vector2(x, y);
}

Vector3 GodotNetMessageIn::GetVector3() {
	if (m_pointer + sizeof(float) * 3 > m_sizeBytes) {
		GDPrint("Failed to get, out of bounds");
		return Vector3();
	}

	if (m_released) {
		GDPrint("Failed to get, message was released");
		return Vector3();
	}

	float x = *(float *)((uint8_t *)m_data + m_pointer);
	m_pointer += sizeof(float);
	float y = *(float *)((uint8_t *)m_data + m_pointer);
	m_pointer += sizeof(float);
	float z = *(float *)((uint8_t *)m_data + m_pointer);
	m_pointer += sizeof(float);

	return Vector3(x, y, z);
}

Quaternion GodotNetMessageIn::GetQuaternion() {
	if (m_pointer + sizeof(float) * 4 > m_sizeBytes) {
		GDPrint("Failed to get, out of bounds");
		return Quaternion();
	}

	if (m_released) {
		GDPrint("Failed to get, message was released");
		return Quaternion();
	}

	float x = *(float *)((uint8_t *)m_data + m_pointer);
	m_pointer += sizeof(float);
	float y = *(float *)((uint8_t *)m_data + m_pointer);
	m_pointer += sizeof(float);
	float z = *(float *)((uint8_t *)m_data + m_pointer);
	m_pointer += sizeof(float);
	float w = *(float *)((uint8_t *)m_data + m_pointer);
	m_pointer += sizeof(float);

	return Quaternion(x, y, z, w);
}

void GodotNetServer::_bind_methods() {
	ClassDB::bind_static_method("GodotNetServer", D_METHOD("host_server", "port"), &GodotNetServer::HostServer);
	ClassDB::bind_static_method("GodotNetServer", D_METHOD("close"), &GodotNetServer::Close);
	ClassDB::bind_static_method("GodotNetServer", D_METHOD("is_running"), &GodotNetServer::IsRunning);
	ClassDB::bind_static_method("GodotNetServer", D_METHOD("is_client_connected", "clientID"), &GodotNetServer::IsClientConnected);
	ClassDB::bind_static_method("GodotNetServer", D_METHOD("disconnect_client", "clientID"), &GodotNetServer::DisconnectClient);

	ClassDB::bind_static_method("GodotNetServer", D_METHOD("set_on_message", "func"), &GodotNetServer::SetOnMessage);
	ClassDB::bind_static_method("GodotNetServer", D_METHOD("set_on_client_connected", "func"), &GodotNetServer::SetOnClientConnected);
	ClassDB::bind_static_method("GodotNetServer", D_METHOD("set_on_client_disconnected", "func"), &GodotNetServer::SetOnClientDisconnected);

	ClassDB::bind_static_method("GodotNetServer", D_METHOD("send_to", "clientID", "msg", "flags", "lane"), &GodotNetServer::SendTo);
	ClassDB::bind_static_method("GodotNetServer", D_METHOD("send_to_all", "msg", "flags", "lane"), &GodotNetServer::SendToAll);
	ClassDB::bind_static_method("GodotNetServer", D_METHOD("send_to_all_except", "clientID", "msg", "flags", "lane"), &GodotNetServer::SendToAllExcept);
}

HSteamListenSocket GodotNetServer::m_listenSocket;
HSteamNetPollGroup GodotNetServer::m_pollGroup;
Vector<GodotNetServer::Client> GodotNetServer::m_connections;
Callable GodotNetServer::m_onMessage;
Callable GodotNetServer::m_onClientConnected;
Callable GodotNetServer::m_onClientDisconnected;
Thread GodotNetServer::m_pollThread;
bool GodotNetServer::m_running = false;
Mutex GodotNetServer::m_runningMutex;

bool GodotNetServer::HostServer(uint16_t port) {
	if (pInterface == nullptr) {
		return false;
	}

	SteamNetworkingIPAddr addr;
	addr.Clear();
	addr.m_port = port;

	SteamNetworkingConfigValue_t opt;
	opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void *)GodotNetServer::OnSteamNetConnectionStatusChanged);
	m_listenSocket = pInterface->CreateListenSocketIP(addr, 1, &opt);
	if (m_listenSocket == k_HSteamListenSocket_Invalid) {
		GDPrint("Failed to create listen socket");
		return false;
	}

	m_pollGroup = pInterface->CreatePollGroup();
	if (m_pollGroup == k_HSteamNetPollGroup_Invalid) {
		GDPrint("Failed to create poll group");
		return false;
	}

	m_runningMutex.lock();
	m_running = true;
	m_runningMutex.unlock();

	m_pollThread.start(GodotNetServer::PollThread, nullptr);

	GDPrint("Server listening on port %d", port);
	return true;
}

void GodotNetServer::OnSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t *pInfo) {
	switch (pInfo->m_info.m_eState) {
		case k_ESteamNetworkingConnectionState_None:
			break;

		case k_ESteamNetworkingConnectionState_ClosedByPeer:
		case k_ESteamNetworkingConnectionState_ProblemDetectedLocally: {
			if (pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connected) {
				// Find the connection in the client list and remove
				Vector<GodotNetServer::Client>::Iterator it;
				for (it = m_connections.begin(); it != m_connections.end(); ++it) {
					if (it->m_connection == pInfo->m_hConn) {
						break;
					}
				}

				if (it != m_connections.end()) {
					GDPrint("Connection from %s closed", pInfo->m_info.m_szConnectionDescription);
					m_onClientDisconnected.call(it->m_clientID);
					m_connections.erase(*it);
				} else {
					GDPrint("Connection closed, but it was not found in the client list");
				}
			}

			pInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
			break;
		}

		case k_ESteamNetworkingConnectionState_Connecting: {
			Vector<GodotNetServer::Client>::Iterator it;
			for (it = m_connections.begin(); it != m_connections.end(); ++it) {
				if (it->m_connection == pInfo->m_hConn) {
					break;
				}
			}

			if (it != m_connections.end()) {
				GDPrint("New connection, but it was already in the client list");
			}

			GDPrint("New connection from %s", pInfo->m_info.m_szConnectionDescription);

			if (pInterface->AcceptConnection(pInfo->m_hConn) != k_EResultOK) {
				pInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
				GDPrint("Failed to accept connection. (It was already closed?)");
				break;
			}

			if (!pInterface->SetConnectionPollGroup(pInfo->m_hConn, m_pollGroup)) {
				pInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
				GDPrint("Failed to set connection poll group");
			}

			if (pInterface->ConfigureConnectionLanes(pInfo->m_hConn, NUM_LANES, LANE_PRIORITIES, NULL) != k_EResultOK) {
				pInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
				GDPrint("Failed to configure connection lanes");
				break;
			}

			static uint32_t clientID = 0;
			if (!pInterface->SetConnectionUserData(pInfo->m_hConn, clientID)) {
				pInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
				GDPrint("Failed to set connection user data");
				break;
			}

			GodotNetServer::Client client;
			client.m_clientID = clientID++;
			client.m_connection = pInfo->m_hConn;

			m_connections.push_back(client);

			if (!m_onClientConnected.is_null()) {
				m_onClientConnected.call(client.m_clientID);
			}

			GDPrint("Connection established, ClientID: %u", client.m_clientID);
			break;
		}

		case k_ESteamNetworkingConnectionState_Connected: {
			break;
		}

		default:
			break;
	}
}

void GodotNetServer::Close() {
	if (pInterface == nullptr) {
		return;
	}

	m_runningMutex.lock();
	m_running = false;
	m_runningMutex.unlock();

	m_pollThread.wait_to_finish();
}

bool GodotNetServer::IsRunning() {
	m_runningMutex.lock();
	bool running = m_running;
	m_runningMutex.unlock();
	return running;
}

bool GodotNetServer::IsClientConnected(uint32_t clientID) {
	Vector<GodotNetServer::Client>::Iterator it;
	for (it = m_connections.begin(); it != m_connections.end(); ++it) {
		if (it->m_clientID == clientID) {
			return true;
		}
	}

	return false;
}

void GodotNetServer::DisconnectClient(uint32_t clientID) {
	Vector<GodotNetServer::Client>::Iterator it;
	for (it = m_connections.begin(); it != m_connections.end(); ++it) {
		if (it->m_clientID == clientID) {
			break;
		}
	}

	if (it == m_connections.end()) {
		GDPrint("Failed to disconnect client, client not found");
		return;
	}

	pInterface->CloseConnection(it->m_connection, 0, nullptr, true);
}

void GodotNetServer::SetOnMessage(Callable func) {
	m_onMessage = func;
}

void GodotNetServer::SetOnClientConnected(Callable func) {
	m_onClientConnected = func;
}

void GodotNetServer::SetOnClientDisconnected(Callable func) {
	m_onClientDisconnected = func;
}

void GodotNetServer::PollThread(void *arg) {
	m_runningMutex.lock();
	while (m_running) {
		m_runningMutex.unlock();
		GodotNetServer::Poll();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		m_runningMutex.lock();
	}

	pInterface->CloseListenSocket(m_listenSocket);
	pInterface->DestroyPollGroup(m_pollGroup);

	m_listenSocket = k_HSteamListenSocket_Invalid;
	m_pollGroup = k_HSteamNetPollGroup_Invalid;
	m_connections.clear();
}

void GodotNetServer::Poll() {
	if (pInterface == nullptr) {
		return;
	}

	while (true) {
		ISteamNetworkingMessage *message = nullptr;
		int numMessages = pInterface->ReceiveMessagesOnPollGroup(m_pollGroup, &message, 1);

		if (numMessages == 0) {
			break;
		}

		if (numMessages < 0) {
			GDPrint("Failed to receive message");
			break;
		}

		if (!m_onMessage.is_null() && message->GetSize() >= sizeof(uint16_t)) {
			const void *data = message->GetData();
			uint16_t packetId = ((uint16_t *)data)[0];
			uint32_t clientId = (uint32_t)message->m_nConnUserData;

			// GDPrint("Received message, packetId: %u, clientId: %u, size: %u", packetId, clientId, message->GetSize());

			if (packetId == 1) {
				// VOIP packet, broadcast to all clients except the sender
				*(uint32_t *)((uint8_t *)data + sizeof(uint16_t)) = clientId;
				for (int i = 0; i < m_connections.size(); i++) {
					if (m_connections[i].m_clientID == clientId) {
						continue;
					}

					// Implicitly on lane 0 (voip lane)
					pInterface->SendMessageToConnection(m_connections[i].m_connection, data, message->GetSize(), k_nSteamNetworkingSend_NoDelay, nullptr);
				}

				message->Release();
				continue;
			}

			Ref<GodotNetMessageIn> msg = memnew(GodotNetMessageIn(message));
			m_onMessage.call(msg, clientId);
		} else {
			message->Release();
		}
	}

	pInterface->RunCallbacks();
}

void GodotNetServer::SendTo(uint32_t clientID, Ref<GodotNetMessageOut> msg, int flags, uint16_t lane) {
	if (pInterface == nullptr) {
		return;
	}

	Vector<GodotNetServer::Client>::Iterator it;
	for (it = m_connections.begin(); it != m_connections.end(); ++it) {
		if (it->m_clientID == clientID) {
			break;
		}
	}

	if (it == m_connections.end()) {
		GDPrint("Failed to send message, client not found");
		return;
	}

	SteamNetworkingMessage_t *netMsg = pUtils->AllocateMessage(0);
	netMsg->m_conn = it->m_connection;
	netMsg->m_nFlags = flags;
	netMsg->m_idxLane = lane;

	msg->RemoveOwnership();
	netMsg->m_pData = msg->GetData();
	netMsg->m_cbSize = msg->GetSize();
	netMsg->m_pfnFreeData = [](SteamNetworkingMessage_t *data) {
		memfree(data->m_pData);
	};

	pInterface->SendMessages(1, &netMsg, nullptr);
}

void GodotNetServer::SendToAll(Ref<GodotNetMessageOut> msg, int flags, uint16_t lane) {
	if (pInterface == nullptr) {
		return;
	}

	for (int i = 0; i < m_connections.size(); i++) {
		SteamNetworkingMessage_t *netMsg = pUtils->AllocateMessage(msg->GetSize());
		netMsg->m_conn = m_connections[i].m_connection;
		netMsg->m_nFlags = flags;
		netMsg->m_idxLane = lane;

		memcpy(netMsg->m_pData, msg->GetData(), msg->GetSize());
		pInterface->SendMessages(1, &netMsg, nullptr);
	}
}

void GodotNetServer::SendToAllExcept(uint32_t clientID, Ref<GodotNetMessageOut> msg, int flags, uint16_t lane) {
	if (pInterface == nullptr) {
		return;
	}

	for (int i = 0; i < m_connections.size(); i++) {
		if (m_connections[i].m_clientID == clientID) {
			continue;
		}

		SteamNetworkingMessage_t *netMsg = pUtils->AllocateMessage(msg->GetSize());
		netMsg->m_conn = m_connections[i].m_connection;
		netMsg->m_nFlags = flags;
		netMsg->m_idxLane = lane;

		memcpy(netMsg->m_pData, msg->GetData(), msg->GetSize());
		pInterface->SendMessages(1, &netMsg, nullptr);
	}
}

void GodotNetClient::_bind_methods() {
	ClassDB::bind_static_method("GodotNetClient", D_METHOD("connect_server", "address", "port"), &GodotNetClient::ConnectServer);
	ClassDB::bind_static_method("GodotNetClient", D_METHOD("close"), &GodotNetClient::Close);
	ClassDB::bind_static_method("GodotNetClient", D_METHOD("connected"), &GodotNetClient::IsConnected);

	ClassDB::bind_static_method("GodotNetClient", D_METHOD("set_on_message", "func"), &GodotNetClient::SetOnMessage);
	ClassDB::bind_static_method("GodotNetClient", D_METHOD("set_on_connected", "func"), &GodotNetClient::SetOnConnected);
	ClassDB::bind_static_method("GodotNetClient", D_METHOD("set_on_disconnected", "func"), &GodotNetClient::SetOnDisconnected);

	ClassDB::bind_static_method("GodotNetClient", D_METHOD("send", "msg", "flags", "lane"), &GodotNetClient::Send);
}

HSteamNetConnection GodotNetClient::m_connection;
Callable GodotNetClient::m_onMessage;
Callable GodotNetClient::m_onConnected;
Callable GodotNetClient::m_onDisconnected;
Thread GodotNetClient::m_pollThread;
bool GodotNetClient::m_running = false;
Mutex GodotNetClient::m_runningMutex;
HashMap<uint16_t, AudioStreamPlaybackNetReceive *> GodotNetClient::m_audioReceivers;

bool GodotNetClient::ConnectServer(String address, uint16_t port) {
	if (pInterface == nullptr) {
		return false;
	}

	SteamNetworkingIPAddr addr;
	addr.ParseString(address.utf8().get_data());
	addr.m_port = port;

	SteamNetworkingConfigValue_t opt;
	opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void *)GodotNetClient::OnSteamNetConnectionStatusChanged);
	m_connection = pInterface->ConnectByIPAddress(addr, 1, &opt);
	if (m_connection == k_HSteamNetConnection_Invalid) {
		GDPrint("Failed to connect to server");
		return false;
	}

	if (pInterface->ConfigureConnectionLanes(m_connection, NUM_LANES, LANE_PRIORITIES, NULL) != k_EResultOK) {
		pInterface->CloseConnection(m_connection, 0, nullptr, false);
		GDPrint("Failed to configure connection lanes");
		return false;
	}

	m_runningMutex.lock();
	m_running = true;
	m_runningMutex.unlock();

	m_pollThread.start(GodotNetClient::PollThread, nullptr);

	return true;
}

void GodotNetClient::OnSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t *pInfo) {
	switch (pInfo->m_info.m_eState) {
		case k_ESteamNetworkingConnectionState_None:
			break;

		case k_ESteamNetworkingConnectionState_ClosedByPeer:
		case k_ESteamNetworkingConnectionState_ProblemDetectedLocally: {
			Close();
			break;
		}

		case k_ESteamNetworkingConnectionState_Connecting: {
			break;
		}

		case k_ESteamNetworkingConnectionState_Connected: {
			GDPrint("Connected to server");
			if (!m_onConnected.is_null()) {
				m_onConnected.call();
			}
			break;
		}

		default:
			break;
	}
}

void GodotNetClient::Close() {
	if (pInterface == nullptr) {
		return;
	}

	m_runningMutex.lock();
	m_running = false;
	m_runningMutex.unlock();

	m_pollThread.wait_to_finish();
}

bool GodotNetClient::IsConnected() {
	m_runningMutex.lock();
	bool connected = m_running;
	m_runningMutex.unlock();
	return connected;
}

void GodotNetClient::SetOnMessage(Callable func) {
	m_onMessage = func;
}

void GodotNetClient::SetOnConnected(Callable func) {
	m_onConnected = func;
}

void GodotNetClient::SetOnDisconnected(Callable func) {
	m_onDisconnected = func;
}

void GodotNetClient::PollThread(void *arg) {
	m_runningMutex.lock();
	while (m_running) {
		m_runningMutex.unlock();
		GodotNetClient::Poll();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		m_runningMutex.lock();
	}

	if (!m_onDisconnected.is_null()) {
		m_onDisconnected.call();
	}

	pInterface->CloseConnection(m_connection, 0, nullptr, false);
	m_connection = k_HSteamNetConnection_Invalid;
	GDPrint("Connection closed");
}

void GodotNetClient::Poll() {
	if (pInterface == nullptr) {
		return;
	}

	while (true) {
		ISteamNetworkingMessage *message = nullptr;
		int numMessages = pInterface->ReceiveMessagesOnConnection(m_connection, &message, 1);

		if (numMessages == 0) {
			break;
		}

		if (numMessages < 0) {
			GDPrint("Failed to receive message, closing connection");
			Close();
			break;
		}

		if (message->GetSize() >= sizeof(uint16_t)) {
			const void *data = message->GetData();
			uint16_t packetId = ((uint16_t *)data)[0];

			// GDPrint("Received message, packetId: %u, size: %u", packetId, message->GetSize());

			if (packetId == 1) {
				// VOIP packet
				uint32_t clientId = *(uint32_t *)((uint8_t *)data + sizeof(uint16_t));
				if (m_audioReceivers.has(clientId)) {
					AudioStreamPlaybackNetReceive *receiver = m_audioReceivers[clientId];
					const void* audioData = (uint8_t *)data + sizeof(uint16_t) + sizeof(uint32_t);
					uint32_t audioSize = message->GetSize() - sizeof(uint16_t) - sizeof(uint32_t);
					receiver->add_buffer(audioData, audioSize);
				}

				message->Release();
				continue;
			}

			Ref<GodotNetMessageIn> msg = memnew(GodotNetMessageIn(message));
			if (!m_onMessage.is_null()) {
				m_onMessage.call(msg);
			}
		} else {
			message->Release();
		}
	}

	pInterface->RunCallbacks();
}

void GodotNetClient::Send(Ref<GodotNetMessageOut> msg, int flags, uint16_t lane) {
	if (pInterface == nullptr) {
		return;
	}

	if (m_connection == k_HSteamNetConnection_Invalid) {
		GDPrint("Failed to send message, not connected");
		return;
	}

	SteamNetworkingMessage_t *netMsg = pUtils->AllocateMessage(0);
	netMsg->m_conn = m_connection;
	netMsg->m_nFlags = flags;
	netMsg->m_idxLane = lane;

	msg->RemoveOwnership();
	netMsg->m_pData = msg->GetData();
	netMsg->m_cbSize = msg->GetSize();
	netMsg->m_pfnFreeData = [](SteamNetworkingMessage_t *data) {
		memfree(data->m_pData);
	};

	pInterface->SendMessages(1, &netMsg, nullptr);
}

void GodotNetClient::SendRaw(void *data, uint32_t size, int flags, uint16_t lane) {
	if (pInterface == nullptr) {
		return;
	}

	if (m_connection == k_HSteamNetConnection_Invalid) {
		GDPrint("Failed to send message, not connected");
		return;
	}

	SteamNetworkingMessage_t *netMsg = pUtils->AllocateMessage(size);
	netMsg->m_conn = m_connection;
	netMsg->m_nFlags = flags;
	netMsg->m_idxLane = lane;

	memcpy(netMsg->m_pData, data, size);
	pInterface->SendMessages(1, &netMsg, nullptr);
}

void GodotNetClient::RegisterAudioReceiver(uint16_t clientId, AudioStreamPlaybackNetReceive* receiver) {
	if (m_audioReceivers.has(clientId)) {
		GDPrint("Audio receiver already registered for client %u", clientId);
		return;
	}

	GDPrint("Registering audio receiver for client %u", clientId);
	m_audioReceivers[clientId] = receiver;
}

void GodotNetClient::UnregisterAudioReceiver(uint16_t clientId) {
	if (!m_audioReceivers.has(clientId)) {
		GDPrint("Audio receiver not registered for client %u", clientId);
		return;
	}

	GDPrint("Unregistering audio receiver for client %u", clientId);
	m_audioReceivers.erase(clientId);
}
