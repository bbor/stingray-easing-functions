#include <engine_plugin_api/plugin_api.h>
#include <plugin_foundation/platform.h>
#include <plugin_foundation/math.h>
#include <plugin_foundation/id_string.h>

extern "C" {
#include "easing.h"
}

#if _DEBUG
	#include <stdlib.h>
	#include <time.h>
#endif

namespace PLUGIN_NAMESPACE {

	LoggingApi *log = nullptr;
	LuaApi *lua = nullptr;
	FlowNodesApi *flow = nullptr;
	ScriptApi *capi = nullptr;

// Expose an API for other C engine plugins

unsigned EASING_API_ID = 0x1ed2147a;
struct easing_api {
	enum EASING_MODE {
		EASING_MODE_LINEAR,
		EASING_MODE_QUADRATIC_IN,
		EASING_MODE_QUADRATIC_OUT,
		EASING_MODE_QUADRATIC_IN_OUT,
		EASING_MODE_CUBIC_IN,
		EASING_MODE_CUBIC_OUT,
		EASING_MODE_CUBIC_IN_OUT,
		EASING_MODE_QUARTIC_IN,
		EASING_MODE_QUARTIC_OUT,
		EASING_MODE_QUARTIC_IN_OUT,
		EASING_MODE_QUINTIC_IN,
		EASING_MODE_QUINTIC_OUT,
		EASING_MODE_QUINTIC_IN_OUT,
		EASING_MODE_SINE_IN,
		EASING_MODE_SINE_OUT,
		EASING_MODE_SINE_IN_OUT,
		EASING_MODE_CIRCULAR_IN,
		EASING_MODE_CIRCULAR_OUT,
		EASING_MODE_CIRCULAR_IN_OUT,
		EASING_MODE_EXPONENTIAL_IN,
		EASING_MODE_EXPONENTIAL_OUT,
		EASING_MODE_EXPONENTIAL_IN_OUT,
		EASING_MODE_ELASTIC_IN,
		EASING_MODE_ELASTIC_OUT,
		EASING_MODE_ELASTIC_IN_OUT,
		EASING_MODE_BACK_IN,
		EASING_MODE_BACK_OUT,
		EASING_MODE_BACK_IN_OUT,
		EASING_MODE_BOUNCE_IN,
		EASING_MODE_BOUNCE_OUT,
		EASING_MODE_BOUNCE_IN_OUT,
	};

	float(*ease_ratio)(EASING_MODE mode, float time_ratio);
	float(*ease_values)(EASING_MODE mode, float start_value, float end_value, float time_ratio);
	float(*ease_values_over_time)(EASING_MODE mode, float start_value, float end_value, float start_time, float total_time);
};


// Stuff that does the work

typedef float(*EasingFunction)(float);
EasingFunction easing_functions[] = {
	&LinearInterpolation,
	&QuadraticEaseIn,
	&QuadraticEaseOut,
	&QuadraticEaseInOut,
	&CubicEaseIn,
	&CubicEaseOut,
	&CubicEaseInOut,
	&QuarticEaseIn,
	&QuarticEaseOut,
	&QuarticEaseInOut,
	&QuinticEaseIn,
	&QuinticEaseOut,
	&QuinticEaseInOut,
	&SineEaseIn,
	&SineEaseOut,
	&SineEaseInOut,
	&CircularEaseIn,
	&CircularEaseOut,
	&CircularEaseInOut,
	&ExponentialEaseIn,
	&ExponentialEaseOut,
	&ExponentialEaseInOut,
	&ElasticEaseIn,
	&ElasticEaseOut,
	&ElasticEaseInOut,
	&BackEaseIn,
	&BackEaseOut,
	&BackEaseInOut,
	&BounceEaseIn,
	&BounceEaseOut,
	&BounceEaseInOut
};

float ease_ratio(easing_api::EASING_MODE mode, float time_ratio) {
	float ratio_result = easing_functions[mode](time_ratio);
	return stingray_plugin_foundation::math::clamp(ratio_result, 0.0f, 1.0f);
}

float ease_values(easing_api::EASING_MODE mode, float start, float end, float time_ratio)
{
	float dist = end - start;
	return start + (dist * ease_ratio(mode, time_ratio));
}

float ease_values_over_time(easing_api::EASING_MODE mode, float start, float end, float start_time, float total_time)
{
	if (total_time < 0)
		return 0;
	float elapsed_time = capi->Application->time_since_launch() - start_time;
	float time_ratio = elapsed_time / total_time;
	return ease_values(mode, start, end, time_ratio);
}


// LUA API

int lua_ease_ratio(lua_State *L)
{
	easing_api::EASING_MODE mode = (easing_api::EASING_MODE)lua->tointeger(L, 1);
	float time_ratio = lua->tonumber(L, 2);
	float result = ease_ratio(mode, time_ratio);
	lua->pushnumber(L, result);
	return 1;
}

int lua_ease_values(lua_State *L)
{
	easing_api::EASING_MODE mode = (easing_api::EASING_MODE)lua->tointeger(L, 1);
	float start = lua->tonumber(L, 2);
	float end = lua->tonumber(L, 3);
	float time_ratio = lua->tonumber(L, 4);
	float result = ease_values(mode, start, end, time_ratio);
	lua->pushnumber(L, result);
	return 1;
}

int lua_ease_values_over_time(lua_State *L)
{
	easing_api::EASING_MODE mode = (easing_api::EASING_MODE)lua->tointeger(L, 1);
	float start = lua->tonumber(L, 2);
	float end = lua->tonumber(L, 3);
	float start_time = lua->tonumber(L, 4);
	float total_time = lua->tonumber(L, 5);
	float result = ease_values_over_time(mode, start, end, start_time, total_time);
	lua->pushnumber(L, result);
	return 1;
}

// FLOW NODE IMPLEMENTATIONS

unsigned flow_node_id_ease = stingray_plugin_foundation::IdString32("ease").id();
extern "C" void flow_node_trigger_ease(struct FlowTriggerContext* tc, const struct FlowData *fd, const struct FlowParameters *fp)
{
	const struct NodeData {
		const int *easing_mode;
		const float *start_value;
		const float *end_value;
		const float *time_ratio;
		float &result;
	} &node_data = (const NodeData&)*fp;

	node_data.result = ease_values((easing_api::EASING_MODE)*node_data.easing_mode, *node_data.start_value, *node_data.end_value, *node_data.time_ratio);
}

unsigned flow_node_id_ease_over_time = stingray_plugin_foundation::IdString32("ease_over_time").id();
extern "C" void flow_node_trigger_ease_over_time(struct FlowTriggerContext* tc, const struct FlowData *fd, const struct FlowParameters *fp)
{
	const struct NodeData {
		const int *easing_mode;
		const float *start_value;
		const float *end_value;
		const float *start_time;
		const float *total_time;
		float &result;
	} &node_data = (const NodeData&)*fp;

	float start_time = *node_data.start_time;
	float current_time = capi->Application->time_since_launch();
	if (node_data.start_time == nullptr)
	{
		start_time = current_time;
	}

	node_data.result = ease_values_over_time((easing_api::EASING_MODE)*node_data.easing_mode, *node_data.start_value, *node_data.end_value, start_time, *node_data.total_time);

	if (current_time > start_time + *node_data.total_time)
	{
		flow->trigger_out_event(tc, fd, 0);
	}
}

// Stingray plug-in API functions

const char* get_name() { return "easing_plugin"; }

/**
 * Setup runtime and compiler common resources, such as allocators.
 */
void setup_common_api(GetApiFunction get_engine_api)
{
	log = (LoggingApi*)get_engine_api(LOGGING_API_ID);
	lua = (LuaApi*)get_engine_api(LUA_API_ID);
	flow = (FlowNodesApi*)get_engine_api(FLOW_NODES_API_ID);
	capi = (ScriptApi*)get_engine_api(C_API_ID);
}

/**
 * Setup plugin runtime resources.
 */
void setup_plugin(GetApiFunction get_engine_api)
{
	setup_common_api(get_engine_api);
	lua->set_module_number("EasingModes", "LINEAR", easing_api::EASING_MODE_LINEAR);
	lua->set_module_number("EasingModes", "QUADRATIC_IN", easing_api::EASING_MODE_QUADRATIC_IN);
	lua->set_module_number("EasingModes", "QUADRATIC_OUT", easing_api::EASING_MODE_QUADRATIC_OUT);
	lua->set_module_number("EasingModes", "QUADRATIC_IN_OUT", easing_api::EASING_MODE_QUADRATIC_IN_OUT);
	lua->set_module_number("EasingModes", "CUBIC_IN", easing_api::EASING_MODE_CUBIC_IN);
	lua->set_module_number("EasingModes", "CUBIC_OUT", easing_api::EASING_MODE_CUBIC_OUT);
	lua->set_module_number("EasingModes", "CUBIC_IN_OUT", easing_api::EASING_MODE_CUBIC_IN_OUT);
	lua->set_module_number("EasingModes", "QUARTIC_IN", easing_api::EASING_MODE_QUARTIC_IN);
	lua->set_module_number("EasingModes", "QUARTIC_OUT", easing_api::EASING_MODE_QUARTIC_OUT);
	lua->set_module_number("EasingModes", "QUARTIC_IN_OUT", easing_api::EASING_MODE_QUARTIC_IN_OUT);
	lua->set_module_number("EasingModes", "QUINTIC_IN", easing_api::EASING_MODE_QUINTIC_IN);
	lua->set_module_number("EasingModes", "QUINTIC_OUT", easing_api::EASING_MODE_QUINTIC_OUT);
	lua->set_module_number("EasingModes", "QUINTIC_IN_OUT", easing_api::EASING_MODE_QUINTIC_IN_OUT);
	lua->set_module_number("EasingModes", "SINE_IN", easing_api::EASING_MODE_SINE_IN);
	lua->set_module_number("EasingModes", "SINE_OUT", easing_api::EASING_MODE_SINE_OUT);
	lua->set_module_number("EasingModes", "SINE_IN_OUT", easing_api::EASING_MODE_SINE_IN_OUT);
	lua->set_module_number("EasingModes", "CIRCULAR_IN", easing_api::EASING_MODE_CIRCULAR_IN);
	lua->set_module_number("EasingModes", "CIRCULAR_OUT", easing_api::EASING_MODE_CIRCULAR_OUT);
	lua->set_module_number("EasingModes", "CIRCULAR_IN_OUT", easing_api::EASING_MODE_CIRCULAR_IN_OUT);
	lua->set_module_number("EasingModes", "EXPONENTIAL_IN", easing_api::EASING_MODE_EXPONENTIAL_IN);
	lua->set_module_number("EasingModes", "EXPONENTIAL_OUT", easing_api::EASING_MODE_EXPONENTIAL_OUT);
	lua->set_module_number("EasingModes", "EXPONENTIAL_IN_OUT", easing_api::EASING_MODE_EXPONENTIAL_IN_OUT);
	lua->set_module_number("EasingModes", "ELASTIC_IN", easing_api::EASING_MODE_ELASTIC_IN);
	lua->set_module_number("EasingModes", "ELASTIC_OUT", easing_api::EASING_MODE_ELASTIC_OUT);
	lua->set_module_number("EasingModes", "ELASTIC_IN_OUT", easing_api::EASING_MODE_ELASTIC_IN_OUT);
	lua->set_module_number("EasingModes", "BACK_IN", easing_api::EASING_MODE_BACK_IN);
	lua->set_module_number("EasingModes", "BACK_OUT", easing_api::EASING_MODE_BACK_OUT);
	lua->set_module_number("EasingModes", "BACK_IN_OUT", easing_api::EASING_MODE_BACK_IN_OUT);
	lua->set_module_number("EasingModes", "BOUNCE_IN", easing_api::EASING_MODE_BOUNCE_IN);
	lua->set_module_number("EasingModes", "BOUNCE_OUT", easing_api::EASING_MODE_BOUNCE_OUT);
	lua->set_module_number("EasingModes", "BOUNCE_IN_OUT", easing_api::EASING_MODE_BOUNCE_IN_OUT);

	lua->add_module_function("Easing", "ease_ratio", lua_ease_ratio);
	lua->add_module_function("Easing", "ease_values", lua_ease_values);
	lua->add_module_function("Easing", "ease_values_over_time", lua_ease_values_over_time);

	flow->setup_trigger_function(flow_node_id_ease, flow_node_trigger_ease);
	flow->setup_trigger_function(flow_node_id_ease_over_time, flow_node_trigger_ease_over_time);
}

void shutdown_plugin()
{
	lua->remove_all_module_entries("EasingModes");
	lua->remove_all_module_entries("Easing");

	flow->unregister_flow_node(flow_node_id_ease);
	flow->unregister_flow_node(flow_node_id_ease_over_time);
}

}

extern "C" {

	/**
	 * Load and define plugin APIs.
	 */
	PLUGIN_DLLEXPORT void *get_plugin_api(unsigned api)
	{
		using namespace PLUGIN_NAMESPACE;

		if (api == PLUGIN_API_ID) {
			static PluginApi plugin_api = { nullptr };
			plugin_api.get_name = get_name;
			plugin_api.setup_game = setup_plugin;
			plugin_api.shutdown_game = shutdown_plugin;
			return &plugin_api;
		}
		else if (api == EASING_API_ID) {
			static easing_api easing_api = { nullptr };
			easing_api.ease_ratio = PLUGIN_NAMESPACE::ease_ratio;
			easing_api.ease_values = PLUGIN_NAMESPACE::ease_values;
			easing_api.ease_values_over_time = PLUGIN_NAMESPACE::ease_values_over_time;
			return &easing_api;
		}
		return nullptr;
	}

}
