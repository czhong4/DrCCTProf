/* 
 *  Copyright (c) 2020-2021 Xuhpclab. All rights reserved.
 *  Licensed under the MIT License.
 *  See LICENSE file for more information.
 */

#ifndef _GO_DEFINES_H_
#define _GO_DEFINES_H_

#include <cstdint>

#define  go_iota 0

struct _go_sudog_t;
struct _go_hchan_t;

typedef enum _go_kind_t: uint8_t {
	kindBool = 1 + go_iota,
	kindInt,
	kindInt8,
	kindInt16,
	kindInt32,
	kindInt64,
	kindUint,
	kindUint8,
	kindUint16,
	kindUint32,
	kindUint64,
	kindUintptr,
	kindFloat32,
	kindFloat64,
	kindComplex64,
	kindComplex128,
	kindArray,
	kindChan,
	kindFunc,
	kindInterface,
	kindMap,
	kindPtr,
	kindSlice,
	kindString,
	kindStruct,
	kindUnsafePointer,

	kindDirectIface = 1 << 5,
	kindGCProg      = 1 << 6,
	kindMask        = (1 << 5) - 1
} go_kind_t;

typedef struct _go_slice_t {
    void* data;
    int64_t len;
    int64_t cap;
} go_slice_t;

typedef struct _go_type_t{
#if defined go1_6_4
	// size       uintptr
	void* size;
	// ptrdata    uintptr // size of memory prefix holding all pointers
	void* ptrdata;
	// hash       uint32
	uint32_t hash;
	// _unused    uint8
	uint8_t _unused;
	// align      uint8
	uint8_t align;
	// fieldAlign uint8
	uint8_t fieldAlign;
	// kind       uint8
	uint8_t kind;
	// alg        *typeAlg
	void* alg;
	// // gcdata stores the GC type data for the garbage collector.
	// // If the KindGCProg bit is set in kind, gcdata is a GC program.
	// // Otherwise it is a ptrmask bitmap. See mbitmap.go for details.
	// gcdata    *byte
	uint8_t* gcdata;
	// _string *string
	void* _string;
	// x       *uncommontype
	void* x;
	// ptrto   *_type
	void* ptrto;
#endif

#if defined go1_11_13
	// size       uintptr
	void* size;
	// ptrdata    uintptr // size of memory prefix holding all pointers
	void* ptrdata;
	// hash       uint32
	uint32_t hash;
	// tflag      tflag
	uint8_t tflag;
	// align      uint8
	uint8_t align;
	// fieldAlign uint8
	uint8_t fieldAlign;
	// kind       uint8
	uint8_t kind;
	// alg        *typeAlg
	void* alg;
	// // gcdata stores the GC type data for the garbage collector.
	// // If the KindGCProg bit is set in kind, gcdata is a GC program.
	// // Otherwise it is a ptrmask bitmap. See mbitmap.go for details.
	// gcdata    *byte
	uint8_t* gcdata;
	// str       nameOff
	int32_t str;
	// ptrToThis typeOff
    int32_t ptrToThis;
#endif

#if defined go1_13_15
	// size       uintptr
	void* size;
	// ptrdata    uintptr // size of memory prefix holding all pointers
	void* ptrdata;
	// hash       uint32
	uint32_t hash;
	// tflag      tflag
	uint8_t tflag;
	// align      uint8
	uint8_t align;
	// fieldAlign uint8
	uint8_t fieldAlign;
	// kind       uint8
	uint8_t kind;
	// alg        *typeAlg
	void* alg;
	// // gcdata stores the GC type data for the garbage collector.
	// // If the KindGCProg bit is set in kind, gcdata is a GC program.
	// // Otherwise it is a ptrmask bitmap. See mbitmap.go for details.
	// gcdata    *byte
	uint8_t* gcdata;
	// str       nameOff
	int32_t str;
	// ptrToThis typeOff
    int32_t ptrToThis;
#endif

#if defined go1_15_6
	// size       uintptr
	void* size;
	// ptrdata    uintptr // size of memory prefix holding all pointers
	void* ptrdata;
	// hash       uint32
	uint32_t hash;
	// tflag      tflag
	uint8_t tflag;
	// align      uint8
	uint8_t align;
	// fieldAlign uint8
	uint8_t fieldAlign;
	// kind       uint8
	uint8_t kind;
	// // function for comparing objects of this type
	// // (ptr to object A, ptr to object B) -> ==?
	// equal func(unsafe.Pointer, unsafe.Pointer) bool
	void* equal;
	// // gcdata stores the GC type data for the garbage collector.
	// // If the KindGCProg bit is set in kind, gcdata is a GC program.
	// // Otherwise it is a ptrmask bitmap. See mbitmap.go for details.
	// gcdata    *byte
	uint8_t* gcdata;
	// str       nameOff
	int32_t str;
	// ptrToThis typeOff
    int32_t ptrToThis;
#endif
} go_type_t;

#if defined go1_11_13 || defined go1_13_15 || defined go1_15_6
typedef struct _go_name_t{
	// bytes *byte
	uint8_t* byte;
} go_name_t;
#endif

typedef struct _go_struct_type_t{
#if defined go1_6_4
	// rtype
	go_type_t type;
	// fields  []structField // sorted by offset
	go_slice_t fields;
#endif

#if defined go1_11_13
	// rtype
	go_type_t type;
	// pkgPath name
	go_name_t pkgPath;
	// fields  []structField // sorted by offset
	go_slice_t fields;
#endif

#if defined go1_13_15
	// rtype
	go_type_t type;
	// pkgPath name
	go_name_t pkgPath;
	// fields  []structField // sorted by offset
	go_slice_t fields;
#endif

#if defined go1_15_6
	// rtype
	go_type_t type;
	// pkgPath name
	go_name_t pkgPath;
	// fields  []structField // sorted by offset
	go_slice_t fields;
#endif
} go_struct_type_t;

typedef struct _go_struct_field_t{
#if defined go1_6_4
	// name    *string
	void* name;
	// pkgpath *string
	void* pkgpath;
	// typ        *_type
	go_type_t* typ;
	// tag     *string
	void* tag;
	// offset  uintptr
	void* offset;
#endif

#if defined go1_11_13
	// name       name
	go_name_t name;
	// typ        *_type
	go_type_t* typ;
	// offsetAnon uintptr
	void* offsetAnon;
#endif

#if defined go1_13_15
	// name       name
	go_name_t name;
	// typ        *_type
	go_type_t* typ;
	// offsetAnon uintptr
	void* offsetAnon;
#endif

#if defined go1_15_6
	// name       name
	go_name_t name;
	// typ        *_type
	go_type_t* typ;
	// offsetAnon uintptr
	void* offsetAnon;
#endif
} go_struct_field_t;

#if defined go1_11_13 || defined go1_13_15 || defined go1_15_6
// ancestorInfo records details of where a goroutine was started.
typedef struct _go_ancestor_info_t {
	// pcs  []uintptr // pcs from the stack of this goroutine
    go_slice_t pcs;
	// goid int64     // goroutine id of this goroutine; original goroutine possibly dead
    int64_t goid;
	// gopc uintptr   // pc of go statement that created this goroutine
    void* gopc;
} go_ancestor_info_t;
#endif

typedef struct _go_string_t {
    void* data;
    int64_t len;
} go_string_t;

typedef struct _go_bitvector_t {
	int32_t n; // # of bits
	uint8_t* bytedata;
} go_bitvector_t;

typedef struct _go_hmap_t {
#if defined go1_6_4
    // count     int // # live cells == size of map.  Must be first (used by len() builtin)
    int64_t count;
	// flags     uint8
    uint8_t flags; 
	// B         uint8  // log_2 of # of buckets (can hold up to loadFactor * 2^B items)
    uint8_t B;
	// hash0     uint32 // hash seed
    uint32_t hash0;

	// buckets    unsafe.Pointer // array of 2^B Buckets. may be nil if count==0.
    void* buckets;
	// oldbuckets unsafe.Pointer // previous bucket array of half the size, non-nil only when growing
    void* oldbuckets;
	// nevacuate  uintptr        // progress counter for evacuation (buckets less than this have been evacuated)
    void* nevacuate;

	// If both key and value do not contain pointers and are inline, then we mark bucket
	// type as containing no pointers. This avoids scanning such maps.
	// However, bmap.overflow is a pointer. In order to keep overflow buckets
	// alive, we store pointers to all overflow buckets in hmap.overflow.
	// Overflow is used only if key and value do not contain pointers.
	// overflow[0] contains overflow buckets for hmap.buckets.
	// overflow[1] contains overflow buckets for hmap.oldbuckets.
	// The first indirection allows us to reduce static size of hmap.
	// The second indirection allows to store a pointer to the slice in hiter.
	// overflow *[2]*[]*bmap
	void* overflow;
#endif

#if defined go1_11_13
    // count     int // # live cells == size of map.  Must be first (used by len() builtin)
    int64_t count;
	// flags     uint8
    uint8_t flags; 
	// B         uint8  // log_2 of # of buckets (can hold up to loadFactor * 2^B items)
    uint8_t B;
	// noverflow uint16 // approximate number of overflow buckets; see incrnoverflow for details
    uint16_t noverflow;
	// hash0     uint32 // hash seed
    uint32_t hash0;

	// buckets    unsafe.Pointer // array of 2^B Buckets. may be nil if count==0.
    void* buckets;
	// oldbuckets unsafe.Pointer // previous bucket array of half the size, non-nil only when growing
    void* oldbuckets;
	// nevacuate  uintptr        // progress counter for evacuation (buckets less than this have been evacuated)
    void* nevacuate;

	// extra *mapextra // optional fields
    void* extra;
#endif

#if defined go1_13_15
    // count     int // # live cells == size of map.  Must be first (used by len() builtin)
    int64_t count;
	// flags     uint8
    uint8_t flags; 
	// B         uint8  // log_2 of # of buckets (can hold up to loadFactor * 2^B items)
    uint8_t B;
	// noverflow uint16 // approximate number of overflow buckets; see incrnoverflow for details
    uint16_t noverflow;
	// hash0     uint32 // hash seed
    uint32_t hash0;

	// buckets    unsafe.Pointer // array of 2^B Buckets. may be nil if count==0.
    void* buckets;
	// oldbuckets unsafe.Pointer // previous bucket array of half the size, non-nil only when growing
    void* oldbuckets;
	// nevacuate  uintptr        // progress counter for evacuation (buckets less than this have been evacuated)
    void* nevacuate;

	// extra *mapextra // optional fields
    void* extra;
#endif

#if defined go1_15_6
    // count     int // # live cells == size of map.  Must be first (used by len() builtin)
    int64_t count;
	// flags     uint8
    uint8_t flags; 
	// B         uint8  // log_2 of # of buckets (can hold up to loadFactor * 2^B items)
    uint8_t B;
	// noverflow uint16 // approximate number of overflow buckets; see incrnoverflow for details
    uint16_t noverflow;
	// hash0     uint32 // hash seed
    uint32_t hash0;

	// buckets    unsafe.Pointer // array of 2^B Buckets. may be nil if count==0.
    void* buckets;
	// oldbuckets unsafe.Pointer // previous bucket array of half the size, non-nil only when growing
    void* oldbuckets;
	// nevacuate  uintptr        // progress counter for evacuation (buckets less than this have been evacuated)
    void* nevacuate;

	// extra *mapextra // optional fields
    void* extra;
#endif
} go_hmap_t;

// typedef struct _go_type_name_t {
//     uint8_t* data;
// } go_type_name_t;

typedef struct _go_sync_mutex_t {
    int32_t state;
    uint32_t sema;
} go_sync_mutex_t;

typedef struct _go_sync_rwmutex_t {
	// w           Mutex  // held if there are pending writers
	go_sync_mutex_t w;
	// writerSem   uint32 // semaphore for writers to wait for completing readers
	uint32_t writerSem;
	// readerSem   uint32 // semaphore for readers to wait for completing writers
	uint32_t readerSem;
	// readerCount int32  // number of pending readers
	int32_t readerCount;
	// readerWait  int32  // number of departing readers
	int32_t readerWait;
} go_sync_rwmutex_t;

typedef struct _go_moduledata_t {
#if defined go1_6_4
    // pclntable    []byte
	go_slice_t pclntable;
    // ftab         []functab
	go_slice_t ftab;
	// filetab      []uint32
	go_slice_t filetab;
    // findfunctab  uintptr
	void* findfunctab;
	// minpc, maxpc uintptr
    void* minpc;
    void* maxpc;

	// text, etext           uintptr
    void* text;
    void* etext;
	// noptrdata, enoptrdata uintptr
    void* noptrdata;
    void* enoptrdata;
	// data, edata           uintptr
    void* data;
    void* edata;
	// bss, ebss             uintptr
    void* bss;
    void* ebss;
	// noptrbss, enoptrbss   uintptr
    void* noptrbss;
    void* enoptrbss;
	// end, gcdata, gcbss    uintptr
	void* end;
    void* gcdata;
    void* gcbss;
	// typelinks []*_type
	go_slice_t typelinks;

	// modulename   string
	go_string_t modulename;
	// modulehashes []modulehash
    go_slice_t modulehashes;

	// gcdatamask, gcbssmask bitvector
    go_bitvector_t gcdatamask;
    go_bitvector_t gcbssmask;
	// next *moduledata
    struct _go_moduledata_t* next;
#endif

#if defined go1_11_13
    // pclntable    []byte
	go_slice_t pclntable;
    // ftab         []functab
	go_slice_t ftab;
	// filetab      []uint32
	go_slice_t filetab;
    // findfunctab  uintptr
	void* findfunctab;
	// minpc, maxpc uintptr
    void* minpc;
    void* maxpc;

	// text, etext           uintptr
    void* text;
    void* etext;
	// noptrdata, enoptrdata uintptr
    void* noptrdata;
    void* enoptrdata;
	// data, edata           uintptr
    void* data;
    void* edata;
	// bss, ebss             uintptr
    void* bss;
    void* ebss;
	// noptrbss, enoptrbss   uintptr
    void* noptrbss;
    void* enoptrbss;
	// end, gcdata, gcbss    uintptr
	void* end;
    void* gcdata;
    void* gcbss;
    // types, etypes         uintptr
    void* types;
    void* etypes;

	// textsectmap []textsect
    go_slice_t textsectmap;
	// typelinks   []int32 // offsets from types
    go_slice_t typelinks;
	// itablinks   []*itab
    go_slice_t itablinks;

	// ptab []ptabEntry
    go_slice_t ptab;

	// pluginpath string
    go_string_t pluginpath;
	// pkghashes  []modulehash
    go_slice_t pkghashes;

	// modulename   string
    go_string_t modulename;
	// modulehashes []modulehash
    go_slice_t modulehashes;

	// hasmain uint8 // 1 if module contains the main function, 0 otherwise
    uint8_t hasmain;

	// gcdatamask, gcbssmask bitvector
    go_bitvector_t gcdatamask;
    go_bitvector_t gcbssmask;

	// typemap map[typeOff]*_type // offset to *_rtype in previous module
    go_hmap_t typemap;
	// bad bool // module failed to load and should be ignored
    bool bad;
	// next *moduledata
    struct _go_moduledata_t* next;
#endif

#if defined go1_13_15
    // pclntable    []byte
	go_slice_t pclntable;
    // ftab         []functab
	go_slice_t ftab;
	// filetab      []uint32
	go_slice_t filetab;
    // findfunctab  uintptr
	void* findfunctab;
	// minpc, maxpc uintptr
    void* minpc;
    void* maxpc;

	// text, etext           uintptr
    void* text;
    void* etext;
	// noptrdata, enoptrdata uintptr
    void* noptrdata;
    void* enoptrdata;
	// data, edata           uintptr
    void* data;
    void* edata;
	// bss, ebss             uintptr
    void* bss;
    void* ebss;
	// noptrbss, enoptrbss   uintptr
    void* noptrbss;
    void* enoptrbss;
	// end, gcdata, gcbss    uintptr
	void* end;
    void* gcdata;
    void* gcbss;
    // types, etypes         uintptr
    void* types;
    void* etypes;

	// textsectmap []textsect
    go_slice_t textsectmap;
	// typelinks   []int32 // offsets from types
    go_slice_t typelinks;
	// itablinks   []*itab
    go_slice_t itablinks;

	// ptab []ptabEntry
    go_slice_t ptab;

	// pluginpath string
    go_string_t pluginpath;
	// pkghashes  []modulehash
    go_slice_t pkghashes;

	// modulename   string
    go_string_t modulename;
	// modulehashes []modulehash
    go_slice_t modulehashes;

	// hasmain uint8 // 1 if module contains the main function, 0 otherwise
    uint8_t hasmain;

	// gcdatamask, gcbssmask bitvector
    go_bitvector_t gcdatamask;
    go_bitvector_t gcbssmask;

	// typemap map[typeOff]*_type // offset to *_rtype in previous module
    go_hmap_t typemap;
	// bad bool // module failed to load and should be ignored
    bool bad;
	// next *moduledata
    struct _go_moduledata_t* next;
#endif

#if defined go1_15_6
    // pclntable    []byte
	go_slice_t pclntable;
    // ftab         []functab
	go_slice_t ftab;
	// filetab      []uint32
	go_slice_t filetab;
    // findfunctab  uintptr
	void* findfunctab;
	// minpc, maxpc uintptr
    void* minpc;
    void* maxpc;

	// text, etext           uintptr
    void* text;
    void* etext;
	// noptrdata, enoptrdata uintptr
    void* noptrdata;
    void* enoptrdata;
	// data, edata           uintptr
    void* data;
    void* edata;
	// bss, ebss             uintptr
    void* bss;
    void* ebss;
	// noptrbss, enoptrbss   uintptr
    void* noptrbss;
    void* enoptrbss;
	// end, gcdata, gcbss    uintptr
	void* end;
    void* gcdata;
    void* gcbss;
    // types, etypes         uintptr
    void* types;
    void* etypes;

	// textsectmap []textsect
    go_slice_t textsectmap;
	// typelinks   []int32 // offsets from types
    go_slice_t typelinks;
	// itablinks   []*itab
    go_slice_t itablinks;

	// ptab []ptabEntry
    go_slice_t ptab;

	// pluginpath string
    go_string_t pluginpath;
	// pkghashes  []modulehash
    go_slice_t pkghashes;

	// modulename   string
    go_string_t modulename;
	// modulehashes []modulehash
    go_slice_t modulehashes;

	// hasmain uint8 // 1 if module contains the main function, 0 otherwise
    uint8_t hasmain;

	// gcdatamask, gcbssmask bitvector
    go_bitvector_t gcdatamask;
    go_bitvector_t gcbssmask;

	// typemap map[typeOff]*_type // offset to *_rtype in previous module
    go_hmap_t typemap;
	// bad bool // module failed to load and should be ignored
    bool bad;
	// next *moduledata
    struct _go_moduledata_t* next;
#endif
} go_moduledata_t;

typedef struct _go_stack_t {
    // lo uintptr
    void* lo;
	// hi uintptr
    void* hi;
} go_stack_t;

typedef struct _go_gobuf_t {
	// // The offsets of sp, pc, and g are known to (hard-coded in) libmach.
	// //
	// // ctxt is unusual with respect to GC: it may be a
	// // heap-allocated funcval, so GC needs to track it, but it
	// // needs to be set and cleared from assembly, where it's
	// // difficult to have write barriers. However, ctxt is really a
	// // saved, live register, and we only ever exchange it between
	// // the real register and the gobuf. Hence, we treat it as a
	// // root during stack scanning, which means assembly that saves
	// // and restores it doesn't need write barriers. It's still
	// // typed as a pointer so that any other writes from Go get
	// // write barriers.
	// sp   uintptr
    void* sp;
	// pc   uintptr
    void* pc;
	// g    guintptr
    void* g;
	// ctxt unsafe.Pointer
    void* ctxt;
	// ret  sys.Uintreg
    uint64_t ret;
	// lr   uintptr
    void* lr;
	// bp   uintptr // for GOEXPERIMENT=framepointer
    void* bp;
} go_gobuf_t;

typedef struct _go_g_t {
#if defined go1_6_4
    // // Stack parameters.
	// // stack describes the actual stack memory: [stack.lo, stack.hi).
	// // stackguard0 is the stack pointer compared in the Go stack growth prologue.
	// // It is stack.lo+StackGuard normally, but can be StackPreempt to trigger a preemption.
	// // stackguard1 is the stack pointer compared in the C stack growth prologue.
	// // It is stack.lo+StackGuard on g0 and gsignal stacks.
	// // It is ~0 on other goroutine stacks, to trigger a call to morestackc (and crash).
	// stack       stack   // offset known to runtime/cgo
    go_stack_t stack;
	// stackguard0 uintptr // offset known to liblink
    void* stackguard0;
	// stackguard1 uintptr // offset known to liblink
    void* stackguard1;

	// _panic       *_panic // innermost panic - offset known to liblink
    void* _panic;
	// _defer       *_defer // innermost defer
    void* _defer;
	// m            *m      // current m; offset known to arm liblink
	void* m;
	// stackAlloc     uintptr // stack allocation is [stack.lo,stack.lo+stackAlloc)
	void* stackAlloc;
    // sched        gobuf
    go_gobuf_t sched;
	// syscallsp    uintptr        // if status==Gsyscall, syscallsp = sched.sp to use during gc
	void* syscallsp;
    // syscallpc    uintptr        // if status==Gsyscall, syscallpc = sched.pc to use during gc
	void* syscallpc;
	// stkbar         []stkbar       // stack barriers, from low to high (see top of mstkbar.go)
	go_slice_t stkbar;
	// stkbarPos      uintptr        // index of lowest stack barrier not hit
	void* stkbarPos;
    // stktopsp     uintptr        // expected sp at top of stack, to check in traceback
	void* stktopsp;
    // param        unsafe.Pointer // passed parameter on wakeup
	void* param;
    // atomicstatus uint32
    uint32_t atomicstatus;
	// stackLock    uint32 // sigprof/scang lock; TODO: fold in to atomicstatus
	uint32_t stackLock;
    // goid         int64
	int64_t goid;
	// waitsince    int64      // approx time when the g become blocked
    int64_t waitsince;
	// waitreason   waitReason // if status==Gwaiting
    uint8_t waitreason;
	// schedlink    guintptr
    void* schedlink;
	// preempt       bool // preemption signal, duplicates stackguard0 = stackpreempt
    bool preempt;
	// paniconfault bool // panic (instead of crash) on unexpected fault address
    bool paniconfault;
	// preemptscan    bool   // preempted g does scan for gc
	bool preemptscan;
	// gcscandone   bool // g has scanned stack; protected by _Gscan bit in status
    bool gcscandone;
	// gcscanvalid    bool   // false at start of gc cycle, true if G has not run since last scan
	bool gcscanvalid;
	// throwsplit   bool // must not split stack
    bool throwsplit;
	// raceignore     int8     // ignore race detection events
    int8_t raceignore;
	// sysblocktraced bool     // StartTrace has emitted EvGoInSyscall about this goroutine
    bool sysblocktraced;
	// sysexitticks   int64    // cputicks when syscall has returned (for tracing)
    int64_t sysexitticks;
	// sysexitseq     uint64 // trace seq when syscall has returned (for tracing)
	uint64_t sysexitseq;
	// lockedm        muintptr
	void* lockedm;
    // sig            uint32
	uint32_t sig;
    // writebuf       []byte
	go_slice_t writebuf;
    // sigcode0       uintptr
    void* sigcode0;
	// sigcode1       uintptr
    void* sigcode1;
	// sigpc          uintptr
    void* sigpc;
	// gopc           uintptr         // pc of go statement that created this goroutine
	void* gopc;
    // startpc        uintptr         // pc of goroutine function
    void* startpc;
	// racectx        uintptr
    void* racectx;
	// waiting        *sudog         // sudog structures this g is waiting on (that have a valid elem ptr); in lock order
	struct _go_sudog_t* waiting;

	// // Per-G GC state

	// // gcAssistBytes is this G's GC assist credit in terms of
	// // bytes allocated. If this is positive, then the G has credit
	// // to allocate gcAssistBytes bytes without assisting. If this
	// // is negative, then the G must correct this by performing
	// // scan work. We track this in bytes to make it fast to update
	// // and check for debt in the malloc hot path. The assist ratio
	// // determines how this corresponds to scan work debt.
	// gcAssistBytes int64
    int64_t gcAssistBytes;
#endif

#if defined go1_11_13
    // // Stack parameters.
	// // stack describes the actual stack memory: [stack.lo, stack.hi).
	// // stackguard0 is the stack pointer compared in the Go stack growth prologue.
	// // It is stack.lo+StackGuard normally, but can be StackPreempt to trigger a preemption.
	// // stackguard1 is the stack pointer compared in the C stack growth prologue.
	// // It is stack.lo+StackGuard on g0 and gsignal stacks.
	// // It is ~0 on other goroutine stacks, to trigger a call to morestackc (and crash).
	// stack       stack   // offset known to runtime/cgo
    go_stack_t stack;
	// stackguard0 uintptr // offset known to liblink
    void* stackguard0;
	// stackguard1 uintptr // offset known to liblink
    void* stackguard1;

	// _panic       *_panic // innermost panic - offset known to liblink
    void* _panic;
	// _defer       *_defer // innermost defer
    void* _defer;
	// m            *m      // current m; offset known to arm liblink
	void* m;
    // sched        gobuf
    go_gobuf_t sched;
	// syscallsp    uintptr        // if status==Gsyscall, syscallsp = sched.sp to use during gc
	void* syscallsp;
    // syscallpc    uintptr        // if status==Gsyscall, syscallpc = sched.pc to use during gc
	void* syscallpc;
    // stktopsp     uintptr        // expected sp at top of stack, to check in traceback
	void* stktopsp;
    // param        unsafe.Pointer // passed parameter on wakeup
	void* param;
    // atomicstatus uint32
    uint32_t atomicstatus;
	// stackLock    uint32 // sigprof/scang lock; TODO: fold in to atomicstatus
	uint32_t stackLock;
    // goid         int64
	int64_t goid;
    // schedlink    guintptr
    void* schedlink;
	// waitsince    int64      // approx time when the g become blocked
    int64_t waitsince;
	// waitreason   waitReason // if status==Gwaiting
    uint8_t waitreason;

	// preempt       bool // preemption signal, duplicates stackguard0 = stackpreempt
    bool preempt;

	// paniconfault bool // panic (instead of crash) on unexpected fault address
    bool paniconfault;
	// preemptscan    bool   // preempted g does scan for gc
	bool preemptscan;
	// gcscandone   bool // g has scanned stack; protected by _Gscan bit in status
    bool gcscandone;
	// gcscanvalid    bool   // false at start of gc cycle, true if G has not run since last scan
	bool gcscanvalid;
	// throwsplit   bool // must not split stack
    bool throwsplit;

	// raceignore     int8     // ignore race detection events
    int8_t raceignore;
	// sysblocktraced bool     // StartTrace has emitted EvGoInSyscall about this goroutine
    bool sysblocktraced;
	// sysexitticks   int64    // cputicks when syscall has returned (for tracing)
    int64_t sysexitticks;
	// traceseq       uint64   // trace event sequencer
    uint64_t traceseq;
	// tracelastp     puintptr // last P emitted an event for this goroutine
    void* tracelastp;
	// lockedm        muintptr
	void* lockedm;
    // sig            uint32
	uint32_t sig;
    // writebuf       []byte
	go_slice_t writebuf;
    // sigcode0       uintptr
    void* sigcode0;
	// sigcode1       uintptr
    void* sigcode1;
	// sigpc          uintptr
    void* sigpc;
	// gopc           uintptr         // pc of go statement that created this goroutine
	void* gopc;
    // ancestors      *[]ancestorInfo // ancestor information goroutine(s) that created this goroutine (only used if debug.tracebackancestors)
	go_slice_t* ancestors;
    // startpc        uintptr         // pc of goroutine function
    void* startpc;
	// racectx        uintptr
    void* racectx;
	// waiting        *sudog         // sudog structures this g is waiting on (that have a valid elem ptr); in lock order
	struct _go_sudog_t* waiting;
    // cgoCtxt        []uintptr      // cgo traceback context
	go_slice_t cgoCtxt;
	// labels         unsafe.Pointer // profiler labels
    void* labels;
	// timer          *timer         // cached timer for time.Sleep
    void* timer;
	// selectDone     uint32         // are we participating in a select and did someone win the race?
    uint32_t selectDone;

	// // Per-G GC state

	// // gcAssistBytes is this G's GC assist credit in terms of
	// // bytes allocated. If this is positive, then the G has credit
	// // to allocate gcAssistBytes bytes without assisting. If this
	// // is negative, then the G must correct this by performing
	// // scan work. We track this in bytes to make it fast to update
	// // and check for debt in the malloc hot path. The assist ratio
	// // determines how this corresponds to scan work debt.
	// gcAssistBytes int64
    int64_t gcAssistBytes;
#endif

#if defined go1_13_15
    // // Stack parameters.
	// // stack describes the actual stack memory: [stack.lo, stack.hi).
	// // stackguard0 is the stack pointer compared in the Go stack growth prologue.
	// // It is stack.lo+StackGuard normally, but can be StackPreempt to trigger a preemption.
	// // stackguard1 is the stack pointer compared in the C stack growth prologue.
	// // It is stack.lo+StackGuard on g0 and gsignal stacks.
	// // It is ~0 on other goroutine stacks, to trigger a call to morestackc (and crash).
	// stack       stack   // offset known to runtime/cgo
    go_stack_t stack;
	// stackguard0 uintptr // offset known to liblink
    void* stackguard0;
	// stackguard1 uintptr // offset known to liblink
    void* stackguard1;

	// _panic       *_panic // innermost panic - offset known to liblink
    void* _panic;
	// _defer       *_defer // innermost defer
    void* _defer;
	// m            *m      // current m; offset known to arm liblink
	void* m;
    // sched        gobuf
    go_gobuf_t sched;
	// syscallsp    uintptr        // if status==Gsyscall, syscallsp = sched.sp to use during gc
	void* syscallsp;
    // syscallpc    uintptr        // if status==Gsyscall, syscallpc = sched.pc to use during gc
	void* syscallpc;
    // stktopsp     uintptr        // expected sp at top of stack, to check in traceback
	void* stktopsp;
    // param        unsafe.Pointer // passed parameter on wakeup
	void* param;
    // atomicstatus uint32
    uint32_t atomicstatus;
	// stackLock    uint32 // sigprof/scang lock; TODO: fold in to atomicstatus
	uint32_t stackLock;
    // goid         int64
	int64_t goid;
    // schedlink    guintptr
    void* schedlink;
	// waitsince    int64      // approx time when the g become blocked
    int64_t waitsince;
	// waitreason   waitReason // if status==Gwaiting
    uint8_t waitreason;

	// preempt       bool // preemption signal, duplicates stackguard0 = stackpreempt
    bool preempt;

	// paniconfault bool // panic (instead of crash) on unexpected fault address
    bool paniconfault;
	// preemptscan    bool   // preempted g does scan for gc
	bool preemptscan;
	// gcscandone   bool // g has scanned stack; protected by _Gscan bit in status
    bool gcscandone;
	// gcscanvalid    bool   // false at start of gc cycle, true if G has not run since last scan
	bool gcscanvalid;
	// throwsplit   bool // must not split stack
    bool throwsplit;

	// raceignore     int8     // ignore race detection events
    int8_t raceignore;
	// sysblocktraced bool     // StartTrace has emitted EvGoInSyscall about this goroutine
    bool sysblocktraced;
	// sysexitticks   int64    // cputicks when syscall has returned (for tracing)
    int64_t sysexitticks;
	// traceseq       uint64   // trace event sequencer
    uint64_t traceseq;
	// tracelastp     puintptr // last P emitted an event for this goroutine
    void* tracelastp;
	// lockedm        muintptr
	void* lockedm;
    // sig            uint32
	uint32_t sig;
    // writebuf       []byte
	go_slice_t writebuf;
    // sigcode0       uintptr
    void* sigcode0;
	// sigcode1       uintptr
    void* sigcode1;
	// sigpc          uintptr
    void* sigpc;
	// gopc           uintptr         // pc of go statement that created this goroutine
	void* gopc;
    // ancestors      *[]ancestorInfo // ancestor information goroutine(s) that created this goroutine (only used if debug.tracebackancestors)
	go_slice_t* ancestors;
    // startpc        uintptr         // pc of goroutine function
    void* startpc;
	// racectx        uintptr
    void* racectx;
	// waiting        *sudog         // sudog structures this g is waiting on (that have a valid elem ptr); in lock order
	struct _go_sudog_t* waiting;
    // cgoCtxt        []uintptr      // cgo traceback context
	go_slice_t cgoCtxt;
	// labels         unsafe.Pointer // profiler labels
    void* labels;
	// timer          *timer         // cached timer for time.Sleep
    void* timer;
	// selectDone     uint32         // are we participating in a select and did someone win the race?
    uint32_t selectDone;

	// // Per-G GC state

	// // gcAssistBytes is this G's GC assist credit in terms of
	// // bytes allocated. If this is positive, then the G has credit
	// // to allocate gcAssistBytes bytes without assisting. If this
	// // is negative, then the G must correct this by performing
	// // scan work. We track this in bytes to make it fast to update
	// // and check for debt in the malloc hot path. The assist ratio
	// // determines how this corresponds to scan work debt.
	// gcAssistBytes int64
    int64_t gcAssistBytes;
#endif

#if defined go1_15_6
    // // Stack parameters.
	// // stack describes the actual stack memory: [stack.lo, stack.hi).
	// // stackguard0 is the stack pointer compared in the Go stack growth prologue.
	// // It is stack.lo+StackGuard normally, but can be StackPreempt to trigger a preemption.
	// // stackguard1 is the stack pointer compared in the C stack growth prologue.
	// // It is stack.lo+StackGuard on g0 and gsignal stacks.
	// // It is ~0 on other goroutine stacks, to trigger a call to morestackc (and crash).
	// stack       stack   // offset known to runtime/cgo
    go_stack_t stack;
	// stackguard0 uintptr // offset known to liblink
    void* stackguard0;
	// stackguard1 uintptr // offset known to liblink
    void* stackguard1;

	// _panic       *_panic // innermost panic - offset known to liblink
    void* _panic;
	// _defer       *_defer // innermost defer
    void* _defer;
	// m            *m      // current m; offset known to arm liblink
	void* m;
    // sched        gobuf
    go_gobuf_t sched;
	// syscallsp    uintptr        // if status==Gsyscall, syscallsp = sched.sp to use during gc
	void* syscallsp;
    // syscallpc    uintptr        // if status==Gsyscall, syscallpc = sched.pc to use during gc
	void* syscallpc;
    // stktopsp     uintptr        // expected sp at top of stack, to check in traceback
	void* stktopsp;
    // param        unsafe.Pointer // passed parameter on wakeup
	void* param;
    // atomicstatus uint32
    uint32_t atomicstatus;
	// stackLock    uint32 // sigprof/scang lock; TODO: fold in to atomicstatus
	uint32_t stackLock;
    // goid         int64
	int64_t goid;
    // schedlink    guintptr
    void* schedlink;
	// waitsince    int64      // approx time when the g become blocked
    int64_t waitsince;
	// waitreason   waitReason // if status==Gwaiting
    uint8_t waitreason;

	// preempt       bool // preemption signal, duplicates stackguard0 = stackpreempt
    bool preempt;
	// preemptStop   bool // transition to _Gpreempted on preemption; otherwise, just deschedule
    bool preemptStop;
	// preemptShrink bool // shrink stack at synchronous safe point
    bool preemptShrink;

	// // asyncSafePoint is set if g is stopped at an asynchronous
	// // safe point. This means there are frames on the stack
	// // without precise pointer information.
	// asyncSafePoint bool
    bool asyncSafePoint;

	// paniconfault bool // panic (instead of crash) on unexpected fault address
    bool paniconfault;
	// gcscandone   bool // g has scanned stack; protected by _Gscan bit in status
    bool gcscandone;
	// throwsplit   bool // must not split stack
    bool throwsplit;
	// // activeStackChans indicates that there are unlocked channels
	// // pointing into this goroutine's stack. If true, stack
	// // copying needs to acquire channel locks to protect these
	// // areas of the stack.
	// activeStackChans bool
    bool activeStackChans;
	// // parkingOnChan indicates that the goroutine is about to
	// // park on a chansend or chanrecv. Used to signal an unsafe point
	// // for stack shrinking. It's a boolean value, but is updated atomically.
	// parkingOnChan uint8
    uint8_t parkingOnChan;

	// raceignore     int8     // ignore race detection events
    int8_t raceignore;
	// sysblocktraced bool     // StartTrace has emitted EvGoInSyscall about this goroutine
    bool sysblocktraced;
	// sysexitticks   int64    // cputicks when syscall has returned (for tracing)
    int64_t sysexitticks;
	// traceseq       uint64   // trace event sequencer
    uint64_t traceseq;
	// tracelastp     puintptr // last P emitted an event for this goroutine
    void* tracelastp;
	// lockedm        muintptr
	void* lockedm;
    // sig            uint32
	uint32_t sig;
    // writebuf       []byte
	go_slice_t writebuf;
    // sigcode0       uintptr
    void* sigcode0;
	// sigcode1       uintptr
    void* sigcode1;
	// sigpc          uintptr
    void* sigpc;
	// gopc           uintptr         // pc of go statement that created this goroutine
	void* gopc;
    // ancestors      *[]ancestorInfo // ancestor information goroutine(s) that created this goroutine (only used if debug.tracebackancestors)
	go_slice_t* ancestors;
    // startpc        uintptr         // pc of goroutine function
    void* startpc;
	// racectx        uintptr
    void* racectx;
	// waiting        *sudog         // sudog structures this g is waiting on (that have a valid elem ptr); in lock order
	struct _go_sudog_t* waiting;
    // cgoCtxt        []uintptr      // cgo traceback context
	go_slice_t cgoCtxt;
	// labels         unsafe.Pointer // profiler labels
    void* labels;
	// timer          *timer         // cached timer for time.Sleep
    void* timer;
	// selectDone     uint32         // are we participating in a select and did someone win the race?
    uint32_t selectDone;

	// // Per-G GC state

	// // gcAssistBytes is this G's GC assist credit in terms of
	// // bytes allocated. If this is positive, then the G has credit
	// // to allocate gcAssistBytes bytes without assisting. If this
	// // is negative, then the G must correct this by performing
	// // scan work. We track this in bytes to make it fast to update
	// // and check for debt in the malloc hot path. The assist ratio
	// // determines how this corresponds to scan work debt.
	// gcAssistBytes int64
    int64_t gcAssistBytes;
#endif
} go_g_t;

typedef struct _go_sudog_t {
#if defined go1_6_4
	// The following fields are protected by the hchan.lock of the
	// channel this sudog is blocking on. shrinkstack depends on
	// this for sudogs involved in channel ops.

	// g *g
	go_g_t* g;
	// selectdone  *uint32
	uint32_t* selectdone;

	// next *sudog
	struct _go_sudog_t* next;
	// prev *sudog
	struct _go_sudog_t* prev;
	// elem unsafe.Pointer // data element (may point to stack)
	void* elem;

	// The following fields are never accessed concurrently.
	// For channels, waitlink is only accessed by g.
	// For semaphores, all fields (including the ones above)
	// are only accessed when holding a semaRoot lock.
	// releasetime int64
	int64_t releasetime;
	// nrelease    int32  // -1 for acquire
	int32_t nrelease;
	// waitlink *sudog // g.waiting list or semaRoot
	struct _go_sudog_t* waitlink;
#endif

#if defined go1_11_13
	// The following fields are protected by the hchan.lock of the
	// channel this sudog is blocking on. shrinkstack depends on
	// this for sudogs involved in channel ops.

	// g *g
	go_g_t* g;
	// isSelect indicates g is participating in a select, so
	// g.selectDone must be CAS'd to win the wake-up race.
	// isSelect bool
	bool isSelect;

	// next *sudog
	struct _go_sudog_t* next;
	// prev *sudog
	struct _go_sudog_t* prev;
	// elem unsafe.Pointer // data element (may point to stack)
	void* elem;

	// The following fields are never accessed concurrently.
	// For channels, waitlink is only accessed by g.
	// For semaphores, all fields (including the ones above)
	// are only accessed when holding a semaRoot lock.
	// acquiretime int64
	int64_t acquiretime;
	// releasetime int64
	int64_t releasetime;
	// ticket      uint32
	uint32_t ticket;
	// parent   *sudog // semaRoot binary tree
	struct _go_sudog_t* parent;
	// waitlink *sudog // g.waiting list or semaRoot
	struct _go_sudog_t* waitlink;
	// waittail *sudog // semaRoot
	struct _go_sudog_t* waittail;
	// c        *hchan // channel
	struct _go_hchan_t* c;
#endif

#if defined go1_13_15
	// The following fields are protected by the hchan.lock of the
	// channel this sudog is blocking on. shrinkstack depends on
	// this for sudogs involved in channel ops.

	// g *g
	go_g_t* g;
	// isSelect indicates g is participating in a select, so
	// g.selectDone must be CAS'd to win the wake-up race.
	// isSelect bool
	bool isSelect;

	// next *sudog
	struct _go_sudog_t* next;
	// prev *sudog
	struct _go_sudog_t* prev;
	// elem unsafe.Pointer // data element (may point to stack)
	void* elem;

	// The following fields are never accessed concurrently.
	// For channels, waitlink is only accessed by g.
	// For semaphores, all fields (including the ones above)
	// are only accessed when holding a semaRoot lock.
	// acquiretime int64
	int64_t acquiretime;
	// releasetime int64
	int64_t releasetime;
	// ticket      uint32
	uint32_t ticket;
	// parent   *sudog // semaRoot binary tree
	struct _go_sudog_t* parent;
	// waitlink *sudog // g.waiting list or semaRoot
	struct _go_sudog_t* waitlink;
	// waittail *sudog // semaRoot
	struct _go_sudog_t* waittail;
	// c        *hchan // channel
	struct _go_hchan_t* c;
#endif

#if defined go1_15_6
	// The following fields are protected by the hchan.lock of the
	// channel this sudog is blocking on. shrinkstack depends on
	// this for sudogs involved in channel ops.

	// g *g
	go_g_t* g;

	// next *sudog
	struct _go_sudog_t* next;
	// prev *sudog
	struct _go_sudog_t* prev;
	// elem unsafe.Pointer // data element (may point to stack)
	void* elem;

	// The following fields are never accessed concurrently.
	// For channels, waitlink is only accessed by g.
	// For semaphores, all fields (including the ones above)
	// are only accessed when holding a semaRoot lock.
	// acquiretime int64
	int64_t acquiretime;
	// releasetime int64
	int64_t releasetime;
	// ticket      uint32
	uint32_t ticket;
	// isSelect indicates g is participating in a select, so
	// g.selectDone must be CAS'd to win the wake-up race.
	// isSelect bool
	bool isSelect;
	// parent   *sudog // semaRoot binary tree
	struct _go_sudog_t* parent;
	// waitlink *sudog // g.waiting list or semaRoot
	struct _go_sudog_t* waitlink;
	// waittail *sudog // semaRoot
	struct _go_sudog_t* waittail;
	// c        *hchan // channel
	struct _go_hchan_t* c;
#endif
} go_sudog_t;

typedef struct _go_waitq_t {
	go_sudog_t* first;
	go_sudog_t* last;
} go_waitq_t;

typedef struct _go_runtime_mutex_t {
#if defined go1_6_4
	void* key;
#endif

#if defined go1_11_13
	void* key;
#endif

#if defined go1_13_15
	void* key;
#endif

#if defined go1_15_6
	// int64_t rank;
	// int64_t pad;
	void* key;
#endif
} go_runtime_mutex_t;

typedef struct _go_hchan_t {
	// qcount   uint           // total data in the queue
	uint64_t qcount;
	// dataqsiz uint           // size of the circular queue
	uint64_t dataqsiz;
	// buf      unsafe.Pointer // points to an array of dataqsiz elements
	void* buf;
	// elemsize uint16
	uint16_t elemsize;
	// closed   uint32
	uint32_t closed;
	// elemtype *_type // element type
	go_type_t* elemtype;
	// sendx    uint   // send index
	uint64_t sendx;
	// recvx    uint   // receive index
	uint64_t recvx;
	// recvq    waitq  // list of recv waiters
	go_waitq_t recvq;
	// sendq    waitq  // list of send waiters
	go_waitq_t sendq;

	// lock protects all fields in hchan, as well as several
	// fields in sudogs blocked on this channel.
	//
	// Do not change another G's status while holding this lock
	// (in particular, do not ready a G), as this can deadlock
	// with stack shrinking.
	// lock mutex
	go_runtime_mutex_t lock;
} go_hchan_t;

typedef struct _go_sync_waitgroup_t {
#if defined go1_6_4
	uint8_t state1[12];
	uint32_t sema;
#endif

#if defined go1_11_13
	uint32_t state1[3];
#endif

#if defined go1_13_15
	uint32_t state1[3];
#endif

#if defined go1_15_6
	uint32_t state1[3];
#endif
} go_sync_waitgroup_t;


#endif // _GO_DEFINES_H_