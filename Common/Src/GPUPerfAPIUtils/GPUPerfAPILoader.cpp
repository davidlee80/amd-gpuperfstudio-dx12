#include "GPUPerfAPILoader.h"

GPUPerfAPILoader::GPUPerfAPILoader()
{
#ifdef _WIN32
    m_hMod = NULL;
#else
    pHandle = NULL;
#endif

    // set all GPA public function pointers to zero
#define GPA_FUNCTION_PREFIX( f )                  \
    f = NULL;
#include "GPAFunctions.h"
//#include "GPAFunctions-Mantle.h"
#undef GPA_FUNCTION_PREFIX
}


GPUPerfAPILoader::~GPUPerfAPILoader()
{
    Unload();
}


void GPUPerfAPILoader::Unload()
{
#ifdef _WIN32

    if (m_hMod)
    {
        FreeLibrary(m_hMod);
        m_hMod = NULL;
    }

#else

    if (pHandle != NULL)
    {
        dlclose(pHandle);
        pHandle = NULL;
    }

#endif
}


bool GPUPerfAPILoader::Loaded()
{
#ifdef _WIN32

    if (m_hMod)
    {
        return true;
    }

#else

    if (pHandle != NULL)
    {
        return true;
    }

#endif
    return false;
}

#ifdef _WIN32
bool GPUPerfAPILoader::Load(const char* dllPath, GPA_API_Type api, const char** errorMessage)
{
    std::string dllFullPath = GetGPADllName(std::string(dllPath), api);

    m_hMod = LoadLibraryA(dllFullPath.c_str());

    if (m_hMod == NULL)
    {
        *errorMessage = "LoadLibrary failed, dll not found.\n";
        return false;
    }

    // Execute GetProcAddress on every public function. If fail assert and return error
#define GPA_FUNCTION_PREFIX( func )                                                             \
    func = ( func##PtrType ) GetProcAddress( m_hMod, #func );                                   \
    assert( func != NULL );                                                                     \
    if ( func == NULL )                                                                         \
    {                                                                                           \
        *errorMessage = #func" not located in the dll. Incorrect or out of date dll.\n";        \
        Unload();                                                                               \
        return false;                                                                           \
    }
#include "GPAFunctions.h"

    if (api == GPA_API_MANTLE)
    {
//#include "GPAFunctions-Mantle.h"
    }

#undef GPA_FUNCTION_PREFIX

    return true;
}
#else

bool GPUPerfAPILoader::Load(const char* dllPath, GPA_API_Type api, const char** errorMessage)
{
    std::string dllFullPath = GetGPADllName(std::string(dllPath), api);

    pHandle = dlopen(dllFullPath.c_str(), RTLD_LAZY);

    if (pHandle == NULL)
    {
        *errorMessage = dlerror();//"dlopen failed, shared library not found.\n";
        return false;
    }

    // Execute GetProcAddress on every public function. If fail assert and return error
#define GPA_FUNCTION_PREFIX( func )                                                                            \
    func = ( func##PtrType ) dlsym( pHandle, #func );                                                          \
    assert( func != NULL );                                                                                    \
    if ( func == NULL )                                                                                        \
    {                                                                                                          \
        *errorMessage = #func" not located in the shared library. Incorrect or out of date shared library.\n"; \
        Unload();                                                                                              \
        return false;                                                                                          \
    }
#include "GPAFunctions.h"

    if (api == GPA_API_MANTLE)
    {
#include "GPAFunctions-Mantle.h"
    }

#undef GPA_FUNCTION_PREFIX

    return true;
}

#endif

#ifdef _WIN32
    #define LIB_PREFIX ""
    #define LIB_SUFFIX ".dll"
#else
    #define LIB_PREFIX "lib"
    #define LIB_SUFFIX ".so"
#endif

#ifndef GDT_PLATFORM_SUFFIX
    #ifdef PLATFORM_SUFFIX
        #define GDT_PLATFORM_SUFFIX PLATFORM_SUFFIX
    #endif
#endif

#ifdef USE_DEBUG_GPA
    #define LIB_DEBUG_SUFFIX "-d"
#else
    #define LIB_DEBUG_SUFFIX ""
#endif

std::string GPUPerfAPILoader::GetGPADllName(const std::string& dllPath, GPA_API_Type api)
{
    std::string dllFullPath = dllPath;
    dllFullPath.append(LIB_PREFIX);

    switch (api)
    {
        case GPA_API_DIRECTX_11:
            dllFullPath.append("GPUPerfAPIDX11");
            break;

        case GPA_API_DIRECTX_12:
            dllFullPath.append("GPUPerfAPIDX12");
            break;

        case GPA_API_OPENGL:
            dllFullPath.append("GPUPerfAPIGL");
            break;

        case GPA_API_OPENGLES:
            dllFullPath.append("GPUPerfAPIGLES");
            break;

        case GPA_API_OPENCL:
            dllFullPath.append("GPUPerfAPICL");
            break;

        case GPA_API_HSA:
            dllFullPath.append("GPUPerfAPIHSA");
            break;

        case GPA_API_MANTLE:
            dllFullPath.append("GPUPerfAPIMantle");
            break;

        default:
            assert("unknown API type");
    }

    dllFullPath.append(GDT_PLATFORM_SUFFIX);
    dllFullPath.append(LIB_DEBUG_SUFFIX);
    dllFullPath.append(GDT_BUILD_SUFFIX);
    dllFullPath.append(LIB_SUFFIX);

    return dllFullPath;
}

