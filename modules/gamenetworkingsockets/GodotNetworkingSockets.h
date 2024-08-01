#ifndef GODOTNETWORKINGSOCKETS_H
#define GODOTNETWORKINGSOCKETS_H

#include "core/os/thread.h"
#include "core/os/mutex.h"
#include "core/object/ref_counted.h"
#include "core/templates/vector.h"
#include "steam/steamnetworkingsockets.h"

class GodotNetSockets : public RefCounted {
	GDCLASS(GodotNetSockets, RefCounted);

protected:
	static void _bind_methods();

public:
	static void SetPrintFunction(Callable func);

	static bool Init();
	static String GetIdentity();
};

class GodotNetMessageOut : public RefCounted {
	GDCLASS(GodotNetMessageOut, RefCounted);

	uint32_t m_sizeBytes;
	size_t m_realSize;
	void* m_data;
	bool m_ownsData;
protected:
	static void _bind_methods();

public:
	GodotNetMessageOut();
	~GodotNetMessageOut();

	static Ref<GodotNetMessageOut> Build(uint32_t sizeBytes);

	void* GetData();
	uint32_t GetSize();
	void RemoveOwnership();

	void SetBool(bool value);
	void SetInt8(int8_t value);
	void SetInt16(int16_t value);
	void SetInt32(int32_t value);
	void SetInt64(int64_t value);
	void SetUInt8(uint8_t value);
	void SetUInt16(uint16_t value);
	void SetUInt32(uint32_t value);
	void SetUInt64(uint64_t value);
	void SetFloat(float value);
	void SetDouble(double value);
	void SetVector2(Vector2 value);
	void SetVector3(Vector3 value);
	void SetQuaternion(Quaternion value);
};

class GodotNetMessageIn : public RefCounted {
	GDCLASS(GodotNetMessageIn, RefCounted);

	size_t m_sizeBytes;
	size_t m_pointer;
	void* m_data;
	ISteamNetworkingMessage* m_msg;
	bool m_released;

protected:
	static void _bind_methods();

public:
	GodotNetMessageIn();
	GodotNetMessageIn(ISteamNetworkingMessage* msg);
	~GodotNetMessageIn();

	void Release();

	bool GetBool();
	int8_t GetInt8();
	int16_t GetInt16();
	int32_t GetInt32();
	int64_t GetInt64();
	uint8_t GetUInt8();
	uint16_t GetUInt16();
	uint32_t GetUInt32();
	uint64_t GetUInt64();
	float GetFloat();
	double GetDouble();
	Vector2 GetVector2();
	Vector3 GetVector3();
	Quaternion GetQuaternion();
};

class GodotNetServer : public RefCounted {
	GDCLASS(GodotNetServer, RefCounted);

	struct Client {
		uint32_t m_clientID;
		HSteamNetConnection m_connection;

		bool operator==(const Client& other) const {
			return m_clientID == other.m_clientID;
		}
	};

	static HSteamListenSocket m_listenSocket;
	static HSteamNetPollGroup m_pollGroup;
	static Vector<Client> m_connections;
	static Thread m_pollThread;
	static bool m_running;
	static Mutex m_runningMutex;

	static Callable m_onMessage;
	static Callable m_onClientConnected;
	static Callable m_onClientDisconnected;

	static void OnSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pCallback);
	static void PollThread(void* arg);
	static void Poll();

protected:
	static void _bind_methods();

public:
	static bool HostServer(uint16_t port);
	//static bool HostP2P();
	static void Close();
	static bool IsRunning();
	static bool IsClientConnected(uint32_t clientId);
	static void DisconnectClient(uint32_t clientId);
	static void SetOnMessage(Callable func);
	static void SetOnClientConnected(Callable func);
	static void SetOnClientDisconnected(Callable func);

	static void SendTo(uint32_t clientId, Ref<GodotNetMessageOut> msg, int flags, uint16_t lane);
	static void SendToAll(Ref<GodotNetMessageOut> msg, int flags, uint16_t lane);
	static void SendToAllExcept(uint32_t clientId, Ref<GodotNetMessageOut> msg, int flags, uint16_t lane);
};

class GodotNetClient : public RefCounted {
	GDCLASS(GodotNetClient, RefCounted);

	static HSteamNetConnection m_connection;
	static Thread m_pollThread;
	static bool m_running;
	static Mutex m_runningMutex;

	static Callable m_onMessage;
	static Callable m_onConnected;
	static Callable m_onDisconnected;

	static void OnSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pCallback);
	static void PollThread(void* arg);
	static void Poll();

protected:
	static void _bind_methods();

public:
	static bool ConnectServer(String address, uint16_t port);
	//static bool ConnectP2P();
	static void Close();
	static bool IsConnected();
	static void SetOnMessage(Callable func);
	static void SetOnConnected(Callable func);
	static void SetOnDisconnected(Callable func);

	static void Send(Ref<GodotNetMessageOut> msg, int flags, uint16_t lane);
	static void SendRaw(void* data, uint32_t size, int flags, uint16_t lane);
};

#endif
