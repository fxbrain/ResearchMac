//
// Created by Stefan Schwarz on 12/11/2016.
//

#include <stdio.h>
#include <CoreServices/CoreServices.h>

/**
 * GetOSVersionComponent.
 * Retrieves the version information component
 * @param component the component of interest
 * @return
 */
static inline int GetOSVersionComponent(int component)
{
    char cmd[64] ;
    sprintf(
            cmd,
            "sw_vers -productVersion | awk -F '.' '{print $%d}'",
            component
    ) ;
    FILE* stdoutFile = popen(cmd, "r") ;

    int answer = 0 ;
    if (stdoutFile)
    {
        char buff[16] ;
        char *stdout = fgets(buff, sizeof(buff), stdoutFile) ;
        pclose(stdoutFile) ;
        sscanf(stdout, "%d", &answer) ;
    }

    return answer ;
}

static int GetGestaltOSVersion()
{
    SInt32 major=0, minor=0, bugfix=0;
    //OSErr err = if (err == noErr)
    Gestalt(gestaltSystemVersionMajor, &major);
    Gestalt(gestaltSystemVersionMinor, &minor);
    Gestalt(gestaltSystemVersionBugFix, &bugfix);


    printf("OS Version : %i.%i.%i\n", major, minor, bugfix);
    return 0;
}

int main(int argc, char **argv)
{
    printf("OS Version : %i.%i.%i\n", GetOSVersionComponent(1), GetOSVersionComponent(2), GetOSVersionComponent(3));
    GetGestaltOSVersion();
    return 0;
}