//
// Created by Stefan Schwarz on 15.03.18.
//

#include <Security/Security.h>
#include <Security/SecImportExport.h>
#include <CoreServices/CoreServices.h>

static void searchKeystore()
{
    // Search the user keychain list for all X509 certificates.
    SecKeychainSearchRef keychainItemSearch = NULL;
    OSStatus err = SecKeychainSearchCreateFromAttributes(NULL, kSecCertificateItemClass, NULL, &keychainItemSearch);
    SecKeychainItemRef theItem = NULL;
    OSErr searchResult = noErr;

    do
    {
        searchResult = SecKeychainSearchCopyNext(keychainItemSearch, &theItem);
        if (searchResult == noErr)
        {
            // Make a byte array with the DER-encoded contents of the certificate.
            SecCertificateRef certRef = (SecCertificateRef)theItem;
            CSSM_DATA currCertificate;
            err = SecCertificateGetData(certRef, &currCertificate);
            printf("Length: %d, Data: 0x%x\n", currCertificate.Length, currCertificate.Data);
        }
    } while (searchResult == noErr);

    if (keychainItemSearch != NULL)
    {
        CFRelease(keychainItemSearch);
    }
}

int main(void)
{
    searchKeystore();
    return EXIT_SUCCESS;
}