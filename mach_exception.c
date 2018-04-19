#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <IOKit/IOReturn.h>
#include <mach/mach.h>
#include <pthread.h>

#include <mach/exc.h>
//#include <mach/i386/thread_status.h>
//#include <mach/mach_error.h>
//#include <mach/mach_init.h>
//#include <mach/mach_port.h>
//#include <mach/task.h>
//#include <mach/thread_act.h>
//#include <mach/thread_status.h>

#define THREAD_STATE_FLAVOR 0

#ifdef __MigPackStructs
#pragma pack(4)
#endif
typedef struct {
    mach_msg_header_t Head;
    /* start of the kernel processed data */
    mach_msg_body_t msgh_body;
    mach_msg_port_descriptor_t thread;
    mach_msg_port_descriptor_t task;
    /* end of the kernel processed data */
    NDR_record_t NDR;
    exception_type_t exception;
    mach_msg_type_number_t codeCnt;
    int64_t code[2];
    int flavor;
    mach_msg_type_number_t old_stateCnt;
    natural_t old_state[224];
} __Request__mach_exception_raise_state_identity_t;
#ifdef __MigPackStructs
#pragma pack()
#endif

/* Local short names, for convenience. */
typedef __Request__mach_exception_raise_state_identity_t protRequestStruct;
typedef __Reply__exception_raise_state_identity_t protReplyStruct;

static mach_port_name_t protExcPort = MACH_PORT_NULL;

static void protBuildReply(protReplyStruct* reply, protRequestStruct* request,
        kern_return_t ret_code)
{
    mach_msg_size_t state_size;
    reply->Head.msgh_bits =
            MACH_MSGH_BITS(MACH_MSGH_BITS_REMOTE(request->Head.msgh_bits), 0);
    reply->Head.msgh_remote_port = request->Head.msgh_remote_port;
    reply->Head.msgh_local_port = MACH_PORT_NULL;
    reply->Head.msgh_reserved = 0;
    reply->Head.msgh_id = request->Head.msgh_id+100;
    reply->NDR = request->NDR;
    reply->RetCode = ret_code;
    reply->flavor = request->flavor;
    reply->new_stateCnt = request->old_stateCnt;
    state_size = reply->new_stateCnt*sizeof(natural_t);
    memcpy(reply->new_state, request->old_state, state_size);
    /* If you use sizeof(reply) for reply->Head.msgh_size then the state
       gets ignored. */
    reply->Head.msgh_size =
            offsetof(protReplyStruct, new_state)+state_size;
}

static void protMustSend(mach_msg_header_t* head)
{
    kern_return_t kr;
    kr = mach_msg(head, MACH_SEND_MSG, head->msgh_size,
            /* recv_size */ 0, MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE,
            MACH_PORT_NULL);
    if (kr!=KERN_SUCCESS) {
        mach_error("ERROR: MPS mach_msg send", kr); /* .trans.must */
    }
}

static void protCatchOne(void)
{
    protRequestStruct request;
    mach_msg_return_t mr;
    protReplyStruct reply;

    mr = mach_msg(&request.Head,
            /* option */ MACH_RCV_MSG,
            /* send_size */ 0,
            /* receive_limit */ sizeof(request),
            /* receive_name */ protExcPort,
            /* timeout */ MACH_MSG_TIMEOUT_NONE,
            /* notify */ MACH_PORT_NULL);
    if (mr!=MACH_MSG_SUCCESS) {
        mach_error("ERROR: MPS mach_msg recv\n", mr); /* .trans.must */
    }
    protBuildReply(&reply, &request, KERN_SUCCESS);
    protMustSend(&reply.Head);
    return;
}

static inline void native_cpuid(unsigned int* eax, unsigned int* ebx,
        unsigned int* ecx, unsigned int* edx)
{
    /* ecx is often an input as well as an output. */
    __asm__ __volatile__("cpuid"
    : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
    : "0"(*eax), "2"(*ecx));
}

static void* protCatchThread(void* p)
{
    for (; ;) {
        protCatchOne();
    }
}

extern void ProtThreadRegister(bool setup)
{
    kern_return_t kr;
    mach_msg_type_number_t old_exception_count;
    exception_mask_t old_exception_masks;
    exception_behavior_t behavior;
    mach_port_t old_exception_ports;
    exception_behavior_t old_behaviors;
    thread_state_flavor_t old_flavors;
    mach_port_t self;
    static mach_port_t setupThread = MACH_PORT_NULL;

    self = mach_thread_self();

    /* Avoid setting up the exception handler for the thread that calls
       ProtSetup twice, in the case where the mutator registers that thread
       explicitly.  We need a special case because we don't require thread
       registration of the sole thread of a single-threaded mutator. */
    if (setup) {
        setupThread = self;
    }
    else {
        if (self==setupThread) {
            return;
        }
    }

    /* Ask to receive EXC_BAD_ACCESS exceptions on our port, complete
       with thread state and identity information in the message.
       The MACH_EXCEPTION_CODES flag causes the code fields to be
       passed 64-bits wide, matching protRequestStruct [Fuller_2013]. */
    behavior = (exception_behavior_t) (EXCEPTION_STATE_IDENTITY |
            MACH_EXCEPTION_CODES);
    kr = thread_swap_exception_ports(
            self, EXC_MASK_BAD_ACCESS, protExcPort, behavior, THREAD_STATE_NONE,
            &old_exception_masks, &old_exception_count, &old_exception_ports,
            &old_behaviors, &old_flavors);
    if (kr!=KERN_SUCCESS) {
        mach_error("ERROR: MPS thread_swap_exception_ports",
                kr); /* .trans.must */
    }
}

static void protSetupInner(void)
{
    kern_return_t kr;
    int pr;
    pthread_t excThread;
    mach_port_t self;

    /* Create a port to send and receive exceptions. */
    self = mach_task_self();
    kr = mach_port_allocate(self, MACH_PORT_RIGHT_RECEIVE, &protExcPort);
    if (kr!=KERN_SUCCESS) {
        mach_error("ERROR: MPS mach_port_allocate",
                kr); /* .trans.must */
    }

    /* Allow me to send exceptions on this port. */
    /* TODO: Find out why this is necessary. */
    self = mach_task_self();
    kr = mach_port_insert_right(self, protExcPort, protExcPort,
            MACH_MSG_TYPE_MAKE_SEND);

    if (kr!=KERN_SUCCESS) {
        mach_error("ERROR: MPS mach_port_insert_right",
                kr); /* .trans.must */
    }

    ProtThreadRegister(true);

    /* Launch the exception handling thread.  We use pthread_create because
       it's much simpler than setting up a thread from scratch using Mach,
       and that's basically what it does.  See [Libc]
       <http://www.opensource.apple.com/source/Libc/Libc-825.26/pthreads/pthread.c>
       */
    pr = pthread_create(&excThread, NULL, protCatchThread, NULL);
    if (pr!=0) {
        fprintf(stderr, "ERROR: MPS pthread_create: %d\n",
                pr); /* .trans.must */
    }
}

void ProtSetup(void)
{
    int pr;
    static pthread_once_t prot_setup_once = PTHREAD_ONCE_INIT;

    pr = pthread_once(&prot_setup_once, protSetupInner);
    if (pr!=0) {
        fprintf(stderr, "ERROR: MPS pthread_once: %d\n",
                pr); /* .trans.must */
    }
}

int main()
{
    uint32_t eax, ebx, ecx, edx;
    char str[9];
    char PSN[30];

    ProtSetup();
    // protCatchOne();
//    std::vector<int> testvector;
//    testvector[0] = 1;

    eax = 1; /* processor info and feature bits */
    native_cpuid(&eax, &ebx, &ecx, &edx);

    sprintf(str, "%08X", eax); // i.e. XXXX-XXXX-xxxx-xxxx-xxxx-xxxx
    sprintf(PSN, "%c%c%c%c-%c%c%c%c", str[0], str[1], str[2], str[3],
            str[4], str[5], str[6], str[7]);

    // get the LSB (64-bit) ind edx and ecx
    //__asm__ __volatile__ ("cpuid"   : "=a" (eax), "=b" (ebx), "=c" (ecx),
    //"=d"
    //(edx) : "a" (3));
    sprintf(str, "%08X", edx); // i.e. xxxx-xxxx-XXXX-XXXX-xxxx-xxxx
    sprintf(PSN, "%s-%c%c%c%c-%c%c%c%c", PSN, str[0], str[1], str[2],
            str[3], str[4], str[5], str[6], str[7]);
    sprintf(str, "%08X", ecx); // i.e. xxxx-xxxx-xxxx-xxxx-XXXX-XXXX
    sprintf(PSN, "%s-%c%c%c%c-%c%c%c%c", PSN, str[0], str[1], str[2],
            str[3], str[4], str[5], str[6], str[7]);

    return 0;
}


