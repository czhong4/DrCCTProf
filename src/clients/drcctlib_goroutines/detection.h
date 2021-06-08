#ifndef _DETECTION_H_
#define _DETECTION_H_


enum {
    READ,
    WRITE
};

enum {
    LOCK = 1,
    UNLOCK = 0
};

enum {
    RLOCK,
    RUNLOCK,
    WLOCK,
    WUNLOCK
};

enum {
    CLOSE, 
    SEND,
    RECEIVE
};

typedef struct _mutex_ctxt_t {
    uint64_t num;
    app_pc state_addr;
    context_handle_t create_context;
    app_pc container_addr;
    int64_t cur_unlock_slow_goid;
    uint64_t indegree;
    uint64_t outdegree;
    int64_t mode;
} mutex_ctxt_t;

typedef struct _rwmutex_ctxt_t {
    app_pc addr;
    context_handle_t create_context;
} rwmutex_ctxt_t;

typedef struct _waitgroup_ctxt_t {
    app_pc addr;
    context_handle_t create_context;
    int64_t counter;
    context_handle_t wait_context;
} waitgroup_ctxt_t;

typedef struct _mem_ref_t {
    uint64_t src_reg1;
    uint64_t src_reg2;
    app_pc src_addr;
    uint64_t dst_reg;
    app_pc dst_addr;
} mem_ref_t;

struct lock_record_t {
    bool op;
    app_pc mutex_addr;
    context_handle_t ctxt;

    lock_record_t(bool o, app_pc addr, context_handle_t c): op(o), mutex_addr(addr), ctxt(c) { }

    bool operator==(const lock_record_t &rhs) const {
        return op == rhs.op && mutex_addr == rhs.mutex_addr && ctxt == rhs.ctxt;
    }
};

struct rwlock_record_t {
    int op;
    app_pc rwmutex_addr;
    context_handle_t ctxt;

    rwlock_record_t(int o, app_pc addr, context_handle_t c): op(o), rwmutex_addr(addr), ctxt(c) { }
    
    bool operator==(const rwlock_record_t &rhs) const {
        return op == rhs.op && rwmutex_addr == rhs.rwmutex_addr && ctxt == rhs.ctxt;
    }
};

struct chan_op_record_t {
    int64_t goid;
    int op;
    app_pc chan_addr;
    context_handle_t ctxt;

    chan_op_record_t(int64_t g, int o, app_pc addr, context_handle_t c): goid(g), op(o), chan_addr(addr), ctxt(c) { }
};

struct deadlock_t {
    int64_t goid;
    app_pc mutex1;
    app_pc mutex2;

    deadlock_t(int64_t g, app_pc m1, app_pc m2): goid(g), mutex1(m1), mutex2(m2) { }
};

struct chan_deadlock_t {
    int64_t goid;
    app_pc chan;
    app_pc mutex;

    chan_deadlock_t(int64_t g, app_pc c, app_pc m): goid(g), chan(c), mutex(m) { }
};

struct blocked_channel_t {
    app_pc chan_addr;
    context_handle_t ctxt;

    blocked_channel_t(app_pc addr, context_handle_t c): chan_addr(addr), ctxt(c) { }
};

struct double_lock_t {
    int64_t goid;
    app_pc mutex;
    context_handle_t ctxt1;
    context_handle_t ctxt2;

    double_lock_t(int64_t g, app_pc m, context_handle_t c1, context_handle_t c2): 
                  goid(g), mutex(m), ctxt1(c1), ctxt2(c2) { }
};

struct double_rwlock_t {
    int64_t goid;
    app_pc rwmutex;
    context_handle_t ctxt1;
    context_handle_t ctxt2;

    double_rwlock_t(int64_t g, app_pc rw, context_handle_t c1, context_handle_t c2): 
                   goid(g), rwmutex(rw), ctxt1(c1), ctxt2(c2) { }
};

struct go_context_t {
    unsigned char** ctx;
    app_pc cancel;
    context_handle_t create_context;
    bool with_cancel;

    go_context_t(unsigned char** c, app_pc canc, context_handle_t cc, bool w): 
                ctx(c), cancel(canc), create_context(cc), with_cancel(w) { }
};

struct mutex_before_each_chan {
    int op;
    app_pc chan_addr;
    std::unordered_set<app_pc> locked_mutex_set;
    std::unordered_set<app_pc> unlocked_mutex_set;
};

struct lock_pair {
    app_pc m1;
    app_pc m2;

    bool operator==(const lock_pair &rhs) const
    {
        return m1 == rhs.m1 && m2 == rhs.m2;
    }
};

struct lock_dependency
{
    context_handle_t ctxt;
    std::unordered_set<app_pc> L;
};

struct hash_func
{
    size_t operator() (const lock_pair &rhs) const
    {
        size_t h1 = std::hash<app_pc>()(rhs.m1);
        size_t h2 = std::hash<app_pc>()(rhs.m2);
        return h1 ^ h2;
    }
};

#endif
