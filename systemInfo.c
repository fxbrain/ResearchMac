//
// Created by Stefan Schwarz on 24/02/2017.
//

#include <CoreServices/CoreServices.h>
#include <stdbool.h>

/*
 * Convert a CFString to a UTF-8-encoded C string; the resulting string
 * is allocated with g_malloc().  Returns NULL if the conversion fails.
 */
char *
CFString_to_C_string(CFStringRef cfstring)
{
    CFIndex string_len;
    char *string;

    string_len = CFStringGetMaximumSizeForEncoding(CFStringGetLength(cfstring),
            kCFStringEncodingUTF8);
    string = (char *)malloc(string_len + 1);
    if (!CFStringGetCString(cfstring, string, string_len + 1,
            kCFStringEncodingUTF8)) {
        free(string);
        return NULL;
    }
    return string;
}

/*
 * Fetch a string, as a UTF-8 C string, from a dictionary, given a key.
 */
static char *
get_string_from_dictionary(CFPropertyListRef dict, CFStringRef key)
{
    CFStringRef cfstring;

    cfstring = (CFStringRef)CFDictionaryGetValue((CFDictionaryRef)dict,
            (const void *)key);
    if (cfstring == NULL)
        return NULL;
    if (CFGetTypeID(cfstring) != CFStringGetTypeID()) {
        /* It isn't a string.  Punt. */
        return NULL;
    }
    return CFString_to_C_string(cfstring);
}

static bool
get_os_x_version_info(char *str)
{
    static const UInt8 server_version_plist_path[] =
            "/System/Library/CoreServices/ServerVersion.plist";
    static const UInt8 system_version_plist_path[] =
            "/System/Library/CoreServices/SystemVersion.plist";
    CFURLRef version_plist_file_url;
    CFReadStreamRef version_plist_stream;
    CFDictionaryRef version_dict;
    char* string;

    /*
     * On OS X, report the OS X version number as the OS, and put
     * the Darwin information in parentheses.
     *
     * Alas, Gestalt() is deprecated in Mountain Lion, so the build
     * fails if you treat deprecation warnings as fatal.  I don't
     * know of any replacement API, so we fall back on reading
     * /System/Library/CoreServices/ServerVersion.plist if it
     * exists, otherwise /System/Library/CoreServices/SystemVersion.plist,
     * and using ProductUserVisibleVersion.  We also get the build
     * version from ProductBuildVersion and the product name from
     * ProductName.
     */
    version_plist_file_url = CFURLCreateFromFileSystemRepresentation(NULL,
            server_version_plist_path, sizeof server_version_plist_path-1,
            false);
    if (version_plist_file_url==NULL)
        return FALSE;
    version_plist_stream = CFReadStreamCreateWithFile(NULL,
            version_plist_file_url);
    CFRelease(version_plist_file_url);
    if (version_plist_stream==NULL)
        return FALSE;
    if (!CFReadStreamOpen(version_plist_stream)) {
        CFRelease(version_plist_stream);

        /*
         * Try SystemVersion.plist.
         */
        version_plist_file_url = CFURLCreateFromFileSystemRepresentation(NULL,
                system_version_plist_path, sizeof system_version_plist_path-1,
                false);
        if (version_plist_file_url==NULL)
            return FALSE;
        version_plist_stream = CFReadStreamCreateWithFile(NULL,
                version_plist_file_url);
        CFRelease(version_plist_file_url);
        if (version_plist_stream==NULL)
            return FALSE;
        if (!CFReadStreamOpen(version_plist_stream)) {
            CFRelease(version_plist_stream);
            return FALSE;
        }
    }
//#ifdef HAVE_CFPROPERTYLISTCREATEWITHSTREAM
    version_dict = (CFDictionaryRef)CFPropertyListCreateWithStream(NULL,
        version_plist_stream, 0, kCFPropertyListImmutable,
        NULL, NULL);
//#else
//    version_dict = (CFDictionaryRef) CFPropertyListCreateFromStream(NULL,
//            version_plist_stream, 0, kCFPropertyListImmutable,
//            NULL, NULL);
//#endif
    if (version_dict==NULL) {
        CFRelease(version_plist_stream);
        return FALSE;
    }
    if (CFGetTypeID(version_dict)!=CFDictionaryGetTypeID()) {
        /* This is *supposed* to be a dictionary.  Punt. */
        CFRelease(version_dict);
        CFReadStreamClose(version_plist_stream);
        CFRelease(version_plist_stream);
        return FALSE;
    }
    /* Get the product name string. */
    string = get_string_from_dictionary(version_dict,
            CFSTR("ProductName"));
    if (string==NULL) {
        CFRelease(version_dict);
        CFReadStreamClose(version_plist_stream);
        CFRelease(version_plist_stream);
        return FALSE;
    }

    /* Get the OS version string. */
    string = get_string_from_dictionary(version_dict,
            CFSTR("ProductUserVisibleVersion"));
    if (string==NULL) {
        CFRelease(version_dict);
        CFReadStreamClose(version_plist_stream);
        CFRelease(version_plist_stream);
        return FALSE;
    }
    return TRUE;
}

int main(void) {
    char* str;
    get_os_x_version_info(str);
    return EXIT_SUCCESS;
}