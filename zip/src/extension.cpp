#include <dmsdk/sdk.h>

#include "extension.h"
#include "zip.h"
#include "utils.h"

class ZipArchive {
	public:
		zip_t *zip;
		uint8_t *buffer;
		size_t buffer_size;
};

static int extension_open(lua_State *L) {
	utils::check_arg_count(L, 1);
	if (utils::check_arg_type(L, "extension_open", "buffer", 1, LUA_TSTRING)) {
		size_t buffer_size;
		const void *buffer = lua_tolstring(L, 1, &buffer_size);
		ZipArchive *zip_archive = new ZipArchive();
		zip_archive->buffer = new uint8_t[buffer_size];
		zip_archive->buffer_size = buffer_size;
		memcpy(zip_archive->buffer, buffer, buffer_size);
		zip_archive->zip = zip_stream_open((const char*)zip_archive->buffer, zip_archive->buffer_size, 0, 'r');
		if (!zip_archive->zip) {
			delete []zip_archive->buffer;
			delete zip_archive;
			lua_pushnil(L);
			lua_pushstring(L, "Failed to open zip archive from memory buffer");
			return 2;
		}
		lua_pushlightuserdata(L, zip_archive);
		return 1;
	}
	return 0;
}

static int extension_close(lua_State *L) {
	utils::check_arg_count(L, 1);
	if (utils::check_arg_type(L, "extension_close", "archive", 1, LUA_TLIGHTUSERDATA)) {
		ZipArchive *zip_archive = (ZipArchive *)lua_touserdata(L, 1);
		if (zip_archive) {
			zip_close(zip_archive->zip);
			delete []zip_archive->buffer;
			delete zip_archive;
		}
	}
	return 0;
}

static int extension_get_number_of_entries(lua_State *L) {
	utils::check_arg_count(L, 1);
	if (utils::check_arg_type(L, "get_number_of_entries", "archive", 1, LUA_TLIGHTUSERDATA)) {
		ZipArchive *zip_archive = (ZipArchive *)lua_touserdata(L, 1);
		if (zip_archive) {
			lua_pushnumber(L, zip_entries_total(zip_archive->zip));
			return 1;
		}
	}
	return 0;
}

static int extension_extract_by_index(lua_State *L) {
	utils::check_arg_count(L, 2);
	if (utils::check_arg_type(L, "extract_by_index", "archive", 1, LUA_TLIGHTUSERDATA)
	&& utils::check_arg_type(L, "extract_by_index", "index", 2, LUA_TNUMBER)) {
		ZipArchive *zip_archive = (ZipArchive *)lua_touserdata(L, 1);
		int index = lua_tointeger(L, 2);
		if (zip_archive) {
			if (zip_entry_openbyindex(zip_archive->zip, index) < 0) {
				lua_pushnil(L);
				lua_pushstring(L, "Failed to open zip entry by index");
				return 2;
			}
			lua_newtable(L);

			lua_pushstring(L, zip_entry_name(zip_archive->zip));
			lua_setfield(L, -2, "name");

			int is_dir = zip_entry_isdir(zip_archive->zip) == 1;
			lua_pushboolean(L, is_dir);
			lua_setfield(L, -2, "is_dir");

			unsigned long long buffer_size = zip_entry_size(zip_archive->zip);
			lua_pushinteger(L, buffer_size);
			lua_setfield(L, -2, "size");

			lua_pushinteger(L, zip_entry_crc32(zip_archive->zip));
			lua_setfield(L, -2, "checksum");

			if (is_dir == 0 && buffer_size > 0) {
				uint8_t *buffer = new uint8_t[buffer_size];
				ssize_t read_size = zip_entry_noallocread(zip_archive->zip, (void *)buffer, (size_t)buffer_size);
				if (read_size > 0) {
					lua_pushlstring(L, (const char *)buffer, (size_t)read_size);
					lua_setfield(L, -2, "content");
				} else {
					lua_pushnil(L);
					lua_setfield(L, -2, "content");
				}
				delete []buffer;
			}
			zip_entry_close(zip_archive->zip);
			return 1;
		}
	}
	return 0;
}

static const luaL_reg lua_functions[] = {
	{"open", extension_open},
	{"get_number_of_entries", extension_get_number_of_entries},
	{"extract_by_index", extension_extract_by_index},
	{"close", extension_close},
	{0, 0}
};

dmExtension::Result APP_INITIALIZE(dmExtension::AppParams *params) {
	return dmExtension::RESULT_OK;
}

dmExtension::Result APP_FINALIZE(dmExtension::AppParams *params) {
	return dmExtension::RESULT_OK;
}

dmExtension::Result INITIALIZE(dmExtension::Params *params) {
	lua_State *L = params->m_L;
	luaL_register(L, EXTENSION_NAME_STRING, lua_functions);

	lua_pop(params->m_L, 1);

	return dmExtension::RESULT_OK;
}

dmExtension::Result UPDATE(dmExtension::Params *params) {
	return dmExtension::RESULT_OK;
}

void EXTENSION_ON_EVENT(dmExtension::Params *params, const dmExtension::Event *event) {
	switch (event->m_Event) {
		case dmExtension::EVENT_ID_ACTIVATEAPP:
			break;
		case dmExtension::EVENT_ID_DEACTIVATEAPP:
			break;
		case dmExtension::EVENT_ID_ICONIFYAPP:
			break;
		case dmExtension::EVENT_ID_DEICONIFYAPP:
			break;
	}
}

dmExtension::Result FINALIZE(dmExtension::Params *params) {
	return dmExtension::RESULT_OK;
}

DM_DECLARE_EXTENSION(EXTENSION_NAME, EXTENSION_NAME_STRING, APP_INITIALIZE, APP_FINALIZE, INITIALIZE, UPDATE, EXTENSION_ON_EVENT, FINALIZE)