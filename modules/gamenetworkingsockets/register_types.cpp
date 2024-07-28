#include "register_types.h"

#include "core/object/class_db.h"
#include "GodotNetworkingSockets.h"
#include "audio_effect_netsend.h"
#include "audio_stream_netreceive.h"

void initialize_gamenetworkingsockets_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	ClassDB::register_class<GodotNetSockets>();
	ClassDB::register_class<GodotNetMessageOut>();
	ClassDB::register_class<GodotNetMessageIn>();
	ClassDB::register_class<GodotNetServer>();
	ClassDB::register_class<GodotNetClient>();
	ClassDB::register_class<AudioEffectNetSend>();
	ClassDB::register_class<AudioStreamNetReceive>();
}

void uninitialize_gamenetworkingsockets_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}
