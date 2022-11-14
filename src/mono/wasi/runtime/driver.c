// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>

#define INVARIANT_GLOBALIZATION 1

#include <mono/metadata/assembly.h>
#include <mono/metadata/tokentype.h>
#include <mono/metadata/threads.h>
#include <mono/metadata/image.h>
#include <mono/metadata/mono-gc.h>
#include <mono/metadata/loader.h>
#include <mono/metadata/object.h>
#include <mono/metadata/metadata.h>
#include <mono/metadata/class.h>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/mono-debug.h>

#include <mono/metadata/mono-private-unstable.h>

#include <mono/utils/mono-logger.h>
#include <mono/utils/mono-dl-fallback.h>
#include <mono/jit/jit.h>
#include <mono/jit/mono-private-unstable.h>

#include "pinvoke.h"

#ifdef GEN_PINVOKE
#include "wasm_m2n_invoke.g.h"
#endif

#if !defined(ENABLE_AOT) || defined(EE_MODE_LLVMONLY_INTERP)
#define NEED_INTERP 1
#ifndef LINK_ICALLS
// FIXME: llvm+interp mode needs this to call icalls
#define NEED_NORMAL_ICALL_TABLES 1
#endif
#endif

void mono_wasm_enable_debugging (int);

int mono_wasm_register_root (char *start, size_t size, const char *name);
void mono_wasm_deregister_root (char *addr);

void mono_ee_interp_init (const char *opts);
void mono_marshal_ilgen_init (void);
void mono_marshal_lightweight_init (void);
void mono_method_builder_ilgen_init (void);
void mono_sgen_mono_ilgen_init (void);
void mono_icall_table_init (void);
void mono_aot_register_module (void **aot_info);
char *monoeg_g_getenv(const char *variable);
int monoeg_g_setenv(const char *variable, const char *value, int overwrite);
int32_t monoeg_g_hasenv(const char *variable);
void mono_free (void*);
int32_t mini_parse_debug_option (const char *option);
char *mono_method_get_full_name (MonoMethod *method);

int mono_wasm_enable_gc = 1;

int
mono_string_instance_is_interned (MonoString *str_raw);

void mono_trace_init (void);

#define g_new(type, size)  ((type *) malloc (sizeof (type) * (size)))
#define g_new0(type, size) ((type *) calloc (sizeof (type), (size)))

static MonoDomain *root_domain;

#define RUNTIMECONFIG_BIN_FILE "runtimeconfig.bin"

static void
wasi_trace_logger (const char *log_domain, const char *log_level, const char *message, mono_bool fatal, void *user_data)
{
    printf("[wasi_trace_logger] %s\n", message);
	if (fatal) {
		// make it trap so we could see the stack trace
		// (*(int*)(void*)-1)++;
		exit(1);
	}
	
}

typedef uint32_t target_mword;
typedef target_mword SgenDescriptor;
typedef SgenDescriptor MonoGCDescriptor;

typedef struct WasmAssembly_ WasmAssembly;

struct WasmAssembly_ {
	MonoBundledAssembly assembly;
	WasmAssembly *next;
};

static WasmAssembly *assemblies;
static int assembly_count;

int
mono_wasm_add_assembly (const char *name, const unsigned char *data, unsigned int size)
{
	int len = strlen (name);
	if (!strcasecmp (".pdb", &name [len - 4])) {
		char *new_name = strdup (name);
		//FIXME handle debugging assemblies with .exe extension
		strcpy (&new_name [len - 3], "dll");
		mono_register_symfile_for_assembly (new_name, data, size);
		return 1;
	}
	WasmAssembly *entry = g_new0 (WasmAssembly, 1);
	entry->assembly.name = strdup (name);
	entry->assembly.data = data;
	entry->assembly.size = size;
	entry->next = assemblies;
	assemblies = entry;
	++assembly_count;
	return mono_has_pdb_checksum ((char*)data, size);
}

typedef struct WasmSatelliteAssembly_ WasmSatelliteAssembly;

struct WasmSatelliteAssembly_ {
	MonoBundledSatelliteAssembly *assembly;
	WasmSatelliteAssembly *next;
};

static WasmSatelliteAssembly *satellite_assemblies;
static int satellite_assembly_count;

int32_t time(int32_t x) {
	// In the current prototype, libSystem.Native.a is built using Emscripten, whereas the WASI-enabled runtime is being built
	// using WASI SDK. Emscripten says that time() returns int32, whereas WASI SDK says it returns int64.
	// TODO: Build libSystem.Native.a using WASI SDK.
	// In the meantime, as a workaround we can define an int32-returning implementation for time() here.
	struct timeval time;
	return (gettimeofday(&time, NULL) == 0) ? time.tv_sec : 0;
}

typedef struct
{
    int32_t Flags;     // flags for testing if some members are present (see FileStatusFlags)
    int32_t Mode;      // file mode (see S_I* constants above for bit values)
    uint32_t Uid;      // user ID of owner
    uint32_t Gid;      // group ID of owner
    int64_t Size;      // total size, in bytes
    int64_t ATime;     // time of last access
    int64_t ATimeNsec; //     nanosecond part
    int64_t MTime;     // time of last modification
    int64_t MTimeNsec; //     nanosecond part
    int64_t CTime;     // time of last status change
    int64_t CTimeNsec; //     nanosecond part
    int64_t BirthTime; // time the file was created
    int64_t BirthTimeNsec; // nanosecond part
    int64_t Dev;       // ID of the device containing the file
    int64_t Ino;       // inode number of the file
    uint32_t UserFlags; // user defined flags
} FileStatus;

char* gai_strerror(int code) {
    char* result = malloc(256);
    sprintf(result, "Error code %i", code);
    return result;
}

void
mono_wasm_add_satellite_assembly (const char *name, const char *culture, const unsigned char *data, unsigned int size)
{
	WasmSatelliteAssembly *entry = g_new0 (WasmSatelliteAssembly, 1);
	entry->assembly = mono_create_new_bundled_satellite_assembly (name, culture, data, size);
	entry->next = satellite_assemblies;
	satellite_assemblies = entry;
	++satellite_assembly_count;
}

static void *sysglobal_native_handle;

static void*
wasm_dl_load (const char *name, int flags, char **err, void *user_data)
{
	void* handle = wasm_dl_lookup_pinvoke_table (name);
	if (handle)
		return handle;

	if (!strcmp (name, "System.Globalization.Native"))
		return sysglobal_native_handle;

#if WASM_SUPPORTS_DLOPEN
	return dlopen(name, flags);
#endif

	return NULL;
}

static void*
wasm_dl_symbol (void *handle, const char *name, char **err, void *user_data)
{
	if (handle == sysglobal_native_handle)
		assert (0);

#if WASM_SUPPORTS_DLOPEN
	if (!wasm_dl_is_pinvoke_tables (handle)) {
		return dlsym (handle, name);
	}
#endif

	PinvokeImport *table = (PinvokeImport*)handle;
	for (int i = 0; table [i].name; ++i) {
		if (!strcmp (table [i].name, name))
			return table [i].func;
	}
	return NULL;
}

#define NEED_INTERP 1

/*
 * get_native_to_interp:
 *
 *   Return a pointer to a wasm function which can be used to enter the interpreter to
 * execute METHOD from native code.
 * EXTRA_ARG is the argument passed to the interp entry functions in the runtime.
 */
void*
get_native_to_interp (MonoMethod *method, void *extra_arg)
{
	MonoClass *klass = mono_method_get_class (method);
	MonoImage *image = mono_class_get_image (klass);
	MonoAssembly *assembly = mono_image_get_assembly (image);
	MonoAssemblyName *aname = mono_assembly_get_name (assembly);
	const char *name = mono_assembly_name_get_name (aname);
	const char *class_name = mono_class_get_name (klass);
	const char *method_name = mono_method_get_name (method);
	char key [128];
	int len;

	assert (strlen (name) < 100);
	snprintf (key, sizeof(key), "%s_%s_%s", name, class_name, method_name);
	len = strlen (key);
	for (int i = 0; i < len; ++i) {
		if (key [i] == '.')
			key [i] = '_';
	}

    assert(0); return 0;
	//void *addr = wasm_dl_get_native_to_interp (key, extra_arg);
	//return addr;
}

void
mono_wasm_register_bundled_satellite_assemblies (void)
{
	/* In legacy satellite_assembly_count is always false */
	if (satellite_assembly_count) {
		MonoBundledSatelliteAssembly **satellite_bundle_array =  g_new0 (MonoBundledSatelliteAssembly *, satellite_assembly_count + 1);
		WasmSatelliteAssembly *cur = satellite_assemblies;
		int i = 0;
		while (cur) {
			satellite_bundle_array [i] = cur->assembly;
			cur = cur->next;
			++i;
		}
		mono_register_bundled_satellite_assemblies ((const MonoBundledSatelliteAssembly **)satellite_bundle_array);
	}
}

void
cleanup_runtime_config (MonovmRuntimeConfigArguments *args, void *user_data)
{
	free (args);
	free (user_data);
}

void
mono_wasm_load_runtime (const char *argv, int debug_level)
{
	const char *interp_opts = "";

#ifndef INVARIANT_GLOBALIZATION
	mono_wasm_link_icu_shim ();
#else
	monoeg_g_setenv ("DOTNET_SYSTEM_GLOBALIZATION_INVARIANT", "true", 1);
#endif

#ifdef DEBUG
	monoeg_g_setenv ("MONO_LOG_LEVEL", "debug", 0);
	monoeg_g_setenv ("MONO_LOG_MASK", "all", 0);
    // Setting this env var allows Diagnostic.Debug to write to stderr.  In a browser environment this
    // output will be sent to the console.  Right now this is the only way to emit debug logging from
    // corlib assemblies.
	monoeg_g_setenv ("COMPlus_DebugWriteToStdErr", "1", 0);
#endif

#if TODOWASI
	char* debugger_fd = monoeg_g_getenv ("DEBUGGER_FD");
	if (debugger_fd != 0)
	{
		const char *debugger_str = "--debugger-agent=transport=wasi_socket,debugger_fd=%-2s,loglevel=0";
		char *debugger_str_with_fd = (char *)malloc (sizeof (char) * (strlen(debugger_str) + strlen(debugger_fd) + 1));
		snprintf (debugger_str_with_fd, strlen(debugger_str) + strlen(debugger_fd) + 1, debugger_str, debugger_fd);
		mono_jit_parse_options (1, &debugger_str_with_fd);
		mono_debug_init (MONO_DEBUG_FORMAT_MONO);
		// Disable optimizations which interfere with debugging
		interp_opts = "-all";
		free (debugger_str_with_fd);
	}
	// When the list of app context properties changes, please update RuntimeConfigReservedProperties for
	// target _WasmGenerateRuntimeConfig in WasmApp.targets file
	const char *appctx_keys[2];
	appctx_keys [0] = "APP_CONTEXT_BASE_DIRECTORY";
	appctx_keys [1] = "RUNTIME_IDENTIFIER";

	const char *appctx_values[2];
	appctx_values [0] = "/";
	appctx_values [1] = "wasi-wasm";
	const char *file_name = RUNTIMECONFIG_BIN_FILE;
	int str_len = strlen (file_name) + 1; // +1 is for the "/"
	char *file_path = (char *)malloc (sizeof (char) * (str_len +1)); // +1 is for the terminating null character
	int num_char = snprintf (file_path, (str_len + 1), "/%s", file_name);
	struct stat buffer;

	assert (num_char > 0 && num_char == str_len);

	if (stat (file_path, &buffer) == 0) {
		MonovmRuntimeConfigArguments *arg = (MonovmRuntimeConfigArguments *)malloc (sizeof (MonovmRuntimeConfigArguments));
		arg->kind = 0;
		arg->runtimeconfig.name.path = file_path;
		monovm_runtimeconfig_initialize (arg, cleanup_runtime_config, file_path);
	} else {
		free (file_path);
	}
	monovm_initialize (2, appctx_keys, appctx_values);
#else
	monovm_initialize (0, NULL, NULL);
#endif /* TODOWASI */

#if TODOWASI
	mini_parse_debug_option ("top-runtime-invoke-unhandled");
#endif /* TODOWASI */

	mono_dl_fallback_register (wasm_dl_load, wasm_dl_symbol, NULL, NULL);
	mono_wasm_install_get_native_to_interp_tramp (get_native_to_interp);
	
#ifdef GEN_PINVOKE
	mono_wasm_install_interp_to_native_callback (mono_wasm_interp_to_native_callback);
#endif

	mono_jit_set_aot_mode (MONO_AOT_MODE_INTERP_ONLY);

#ifdef LINK_ICALLS
	#error /* TODOWASI */
#endif
#ifdef NEED_NORMAL_ICALL_TABLES
	mono_icall_table_init ();
#endif
	mono_ee_interp_init (interp_opts);
	mono_marshal_ilgen_init ();
	mono_method_builder_ilgen_init ();
	mono_sgen_mono_ilgen_init ();

	if (assembly_count) {
		MonoBundledAssembly **bundle_array = g_new0 (MonoBundledAssembly*, assembly_count + 1);
		WasmAssembly *cur = assemblies;
		int i = 0;
		while (cur) {
			bundle_array [i] = &cur->assembly;
			cur = cur->next;
			++i;
		}
		mono_register_bundled_assemblies ((const MonoBundledAssembly **)bundle_array);
	}

	mono_wasm_register_bundled_satellite_assemblies ();
	mono_trace_init ();
	mono_trace_set_log_handler (wasi_trace_logger, NULL);

	root_domain = mono_jit_init_version ("mono", NULL);
	mono_thread_set_main (mono_thread_current ());
}

MonoAssembly*
mono_wasm_assembly_load (const char *name)
{
	MonoImageOpenStatus status;
	MonoAssemblyName* aname = mono_assembly_name_new (name);
	if (!name)
		return NULL;

	MonoAssembly *res = mono_assembly_load (aname, NULL, &status);
	mono_assembly_name_free (aname);

	return res;
}

MonoClass*
mono_wasm_find_corlib_class (const char *namespace, const char *name)
{
	return mono_class_from_name (mono_get_corlib (), namespace, name);
}

MonoClass*
mono_wasm_assembly_find_class (MonoAssembly *assembly, const char *namespace, const char *name)
{
	return mono_class_from_name (mono_assembly_get_image (assembly), namespace, name);
}

MonoMethod*
mono_wasm_assembly_find_method (MonoClass *klass, const char *name, int arguments)
{
	return mono_class_get_method_from_name (klass, name, arguments);
}

MonoMethod*
mono_wasm_get_delegate_invoke (MonoObject *delegate)
{
	return mono_get_delegate_invoke(mono_object_get_class (delegate));
}

MonoObject*
mono_wasm_box_primitive (MonoClass *klass, void *value, int value_size)
{
	if (!klass)
		return NULL;

	MonoType *type = mono_class_get_type (klass);
	int alignment;
	if (mono_type_size (type, &alignment) > value_size)
		return NULL;

	// TODO: use mono_value_box_checked and propagate error out
	return mono_value_box (root_domain, klass, value);
}

void
mono_wasm_invoke_method_ref (MonoMethod *method, MonoObject **this_arg_in, void *params[], MonoObject **out_exc, MonoObject **out_result)
{
	MonoObject* temp_exc = NULL;
	if (out_exc)
		*out_exc = NULL;
	else
		out_exc = &temp_exc;

	if (out_result) {
		*out_result = NULL;
		*out_result = mono_runtime_invoke (method, this_arg_in ? *this_arg_in : NULL, params, out_exc);
	} else {
		mono_runtime_invoke (method, this_arg_in ? *this_arg_in : NULL, params, out_exc);
	}

	if (*out_exc && out_result) {
		MonoObject *exc2 = NULL;
		*out_result = (MonoObject*)mono_object_to_string (*out_exc, &exc2);
		if (exc2)
			*out_result = (MonoObject*) mono_string_new (root_domain, "Exception Double Fault");
		return;
	}
}

MonoMethod*
mono_wasm_assembly_get_entry_point (MonoAssembly *assembly)
{
	MonoImage *image;
	MonoMethod *method;

	image = mono_assembly_get_image (assembly);
	uint32_t entry = mono_image_get_entry_point (image);
	if (!entry)
		return NULL;

	mono_domain_ensure_entry_assembly (root_domain, assembly);
	method = mono_get_method (image, entry, NULL);

	/*
	 * If the entry point looks like a compiler generated wrapper around
	 * an async method in the form "<Name>" then try to look up the async methods
	 * "<Name>$" and "Name" it could be wrapping.  We do this because the generated
	 * sync wrapper will call task.GetAwaiter().GetResult() when we actually want
	 * to yield to the host runtime.
	 */
	if (mono_method_get_flags (method, NULL) & 0x0800 /* METHOD_ATTRIBUTE_SPECIAL_NAME */) {
		const char *name = mono_method_get_name (method);
		int name_length = strlen (name);

		if ((*name != '<') || (name [name_length - 1] != '>'))
			return method;

		MonoClass *klass = mono_method_get_class (method);
		char *async_name = malloc (name_length + 2);
		snprintf (async_name, name_length + 2, "%s$", name);

		// look for "<Name>$"
		MonoMethodSignature *sig = mono_method_get_signature (method, image, mono_method_get_token (method));
		MonoMethod *async_method = mono_class_get_method_from_name (klass, async_name, mono_signature_get_param_count (sig));
		if (async_method != NULL) {
			free (async_name);
			return async_method;
		}

		// look for "Name" by trimming the first and last character of "<Name>"
		async_name [name_length - 1] = '\0';
		async_method = mono_class_get_method_from_name (klass, async_name + 1, mono_signature_get_param_count (sig));

		free (async_name);
		if (async_method != NULL)
			return async_method;
	}
	return method;
}

char *
mono_wasm_string_get_utf8 (MonoString *str)
{
	return mono_string_to_utf8 (str); //XXX JS is responsible for freeing this
}

MonoString *
mono_wasm_string_from_js (const char *str)
{
	if (str)
		return mono_string_new (root_domain, str);
	else
		return NULL;
}

int
mono_unbox_int (MonoObject *obj)
{
	if (!obj)
		return 0;
	MonoType *type = mono_class_get_type (mono_object_get_class(obj));

	void *ptr = mono_object_unbox (obj);
	switch (mono_type_get_type (type)) {
	case MONO_TYPE_I1:
	case MONO_TYPE_BOOLEAN:
		return *(signed char*)ptr;
	case MONO_TYPE_U1:
		return *(unsigned char*)ptr;
	case MONO_TYPE_I2:
		return *(short*)ptr;
	case MONO_TYPE_U2:
		return *(unsigned short*)ptr;
	case MONO_TYPE_I4:
	case MONO_TYPE_I:
		return *(int*)ptr;
	case MONO_TYPE_U4:
		return *(unsigned int*)ptr;
	case MONO_TYPE_CHAR:
		return *(short*)ptr;
	// WASM doesn't support returning longs to JS
	// case MONO_TYPE_I8:
	// case MONO_TYPE_U8:
	default:
		printf ("Invalid type %d to mono_unbox_int\n", mono_type_get_type (type));
		return 0;
	}
}

int
mono_wasm_array_length (MonoArray *array)
{
	return mono_array_length (array);
}

MonoObject*
mono_wasm_array_get (MonoArray *array, int idx)
{
	return mono_array_get (array, MonoObject*, idx);
}

MonoArray*
mono_wasm_obj_array_new (int size)
{
	return mono_array_new (root_domain, mono_get_object_class (), size);
}

void
mono_wasm_obj_array_set (MonoArray *array, int idx, MonoObject *obj)
{
	mono_array_setref (array, idx, obj);
}

MonoArray*
mono_wasm_string_array_new (int size)
{
	return mono_array_new (root_domain, mono_get_string_class (), size);
}

void
mono_wasm_string_get_data_ref (
	MonoString **string, mono_unichar2 **outChars, int *outLengthBytes, int *outIsInterned
) {
	if (!string || !(*string)) {
		if (outChars)
			*outChars = 0;
		if (outLengthBytes)
			*outLengthBytes = 0;
		if (outIsInterned)
			*outIsInterned = 1;
		return;
	}

	if (outChars)
		*outChars = mono_string_chars (*string);
	if (outLengthBytes)
		*outLengthBytes = mono_string_length (*string) * 2;
	if (outIsInterned)
		*outIsInterned = mono_string_instance_is_interned (*string);
	return;
}

void
mono_wasm_string_get_data (
	MonoString *string, mono_unichar2 **outChars, int *outLengthBytes, int *outIsInterned
) {
	mono_wasm_string_get_data_ref(&string, outChars, outLengthBytes, outIsInterned);
}

void add_assembly(const char* base_dir, const char *name) {
    FILE *fileptr;
    unsigned char *buffer;
    long filelen;
    char filename[256];
    sprintf(filename, "%s/%s", base_dir, name);
    // printf("Loading %s...\n", filename);

    fileptr = fopen(filename, "rb");
    if (fileptr == 0) {
        printf("Failed to load %s\n", filename);
        fflush(stdout);
    }

    fseek(fileptr, 0, SEEK_END);
    filelen = ftell(fileptr);
    rewind(fileptr);

    buffer = (unsigned char *)malloc(filelen * sizeof(char));
    if(!fread(buffer, filelen, 1, fileptr)) {
        printf("Failed to load %s\n", filename);
        fflush(stdout);
    }
    fclose(fileptr);

    assert(mono_wasm_add_assembly(name, buffer, filelen));
}

MonoMethod* lookup_dotnet_method(const char* assembly_name, const char* namespace, const char* type_name, const char* method_name, int num_params) {
    MonoAssembly* assembly = mono_wasm_assembly_load (assembly_name);
	assert (assembly);
    MonoClass* class = mono_wasm_assembly_find_class (assembly, namespace, type_name);
	assert (class);
    MonoMethod* method = mono_wasm_assembly_find_method (class, method_name, num_params);
	assert (method);
	return method;
}

int main() {
    // Assume the runtime pack has been copied into the output directory as 'runtime'
    // Otherwise we have to mount an unrelated part of the filesystem within the WASM environment
    mono_set_assemblies_path(".:./runtime/native:./runtime/lib/net7.0");
    mono_wasm_load_runtime("", 0);

    MonoAssembly* assembly = mono_wasm_assembly_load ("Wasi.Console.Sample");
    MonoMethod* entry_method = mono_wasm_assembly_get_entry_point (assembly);
    MonoObject* out_exc;
    MonoObject* out_res;
    mono_wasm_invoke_method_ref (entry_method, NULL, NULL, &out_exc, &out_res);
	if (out_exc)
	{
		mono_print_unhandled_exception(out_exc);
		exit(1);
	}
	if(out_res)
	{
		int r= mono_unbox_int (out_res);
		return r;
	}
	return 0;
}
