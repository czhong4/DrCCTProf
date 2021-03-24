/* 
 *  Copyright (c) 2020-2021 Xuhpclab. All rights reserved.
 *  Licensed under the MIT License.
 *  See LICENSE file for more information.
 */
#include <vector>
#include <string>
#include <string.h>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <queue>

#include "dr_api.h"
#include "drmgr.h"
#include "drreg.h"
#include "drutil.h"
#include "drsyms.h"
#include "drwrap.h"
#include "drcctlib.h"

#include "dr_go_wrap.h"
#include "elf_utils.h"
#include "cgo_funcs.h"

using namespace std;

#define DRCCTLIB_PRINTF(_FORMAT, _ARGS...) \
    DRCCTLIB_PRINTF_TEMPLATE("goroutines", _FORMAT, ##_ARGS)
#define DRCCTLIB_EXIT_PROCESS(_FORMAT, _ARGS...) \
    DRCCTLIB_CLIENT_EXIT_PROCESS_TEMPLATE("goroutines", _FORMAT, ##_ARGS)

#ifdef ARM_CCTLIB
#    define OPND_CREATE_CCT_INT OPND_CREATE_INT
#else
#    ifdef CCTLIB_64
#        define OPND_CREATE_CCT_INT OPND_CREATE_INT64
#    else
#        define OPND_CREATE_CCT_INT OPND_CREATE_INT32
#    endif
#endif

typedef struct _mutex_ctxt_t{
    app_pc state_addr;
    context_handle_t create_context;
    app_pc container_addr;
    int64_t cur_unlock_slow_goid;
} mutex_ctxt_t;

typedef struct _waitgroup_ctxt_t {
    app_pc addr;
    context_handle_t create_context;
    int64_t counter;
    context_handle_t wait_context;
} waitgroup_ctxt_t;

typedef struct _mem_ref_t {
    app_pc addr;
    uint64_t state;
} mem_ref_t;

typedef struct _per_thread_t {
    thread_id_t thread_id;
    mem_ref_t *cur_buf_list;
    void *cur_buf;
    context_handle_t last_newobject_ctxt_hndl;
    vector<context_handle_t>* call_rt_exec_list;
    vector<int64_t>* goid_list;
    vector<vector<int64_t>>* go_ancestors_list;
} per_thread_t;

struct lock_record_t {
    bool op;
    app_pc mutex_addr;
    context_handle_t ctxt;

    lock_record_t(bool o, app_pc addr, context_handle_t c): op(o), mutex_addr(addr), ctxt(c) { }
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

#define TLS_MEM_REF_BUFF_SIZE 100

static int tls_idx;
enum {
    INSTRACE_TLS_OFFS_BUF_PTR,
    INSTRACE_TLS_COUNT, /* total number of TLS slots allocated */
};

static reg_id_t tls_seg;
static uint tls_offs;
#define TLS_SLOT(tls_base, enum_val) (void **)((byte *)(tls_base) + tls_offs + (enum_val))
#define BUF_PTR(tls_base, type, offs) *(type **)TLS_SLOT(tls_base, offs)
#define MINSERT instrlist_meta_preinsert

static file_t gTraceFile;
static void *thread_sync_lock;

static std::vector<std::string> *blacklist;
static go_moduledata_t *go_firstmoduledata;

static vector<mutex_ctxt_t> *mutex_ctxt_list;
static vector<waitgroup_ctxt_t> *wg_ctxt_list;
static unordered_map<int64_t, vector<pair<bool, context_handle_t>>> *lock_records;
static unordered_map<int64_t, vector<lock_record_t>> *test_lock_records;
static unordered_map<int64_t, vector<chan_op_record_t>> *chan_op_records;
static unordered_map<app_pc, vector<chan_op_record_t>> *op_records_per_chan;

// client want to do
void
CheckLockState(void *drcontext, int64_t cur_goid, context_handle_t cur_ctxt_hndl, mem_ref_t *ref)
{
    app_pc addr = ref->addr;
    // DRCCTLIB_PRINTF("addr %p", ref->addr);
    for (size_t i = 0; i < mutex_ctxt_list->size(); i++) {
        if (addr == (*mutex_ctxt_list)[i].state_addr && cur_goid != (*mutex_ctxt_list)[i].cur_unlock_slow_goid) {
            (*lock_records)[cur_goid].emplace_back(1, (*mutex_ctxt_list)[i].create_context);
            context_handle_t cur_context = drcctlib_get_context_handle(drcontext);
            (*test_lock_records)[cur_goid].emplace_back(1, (*mutex_ctxt_list)[i].state_addr, cur_context);
            DRCCTLIB_PRINTF("GOID(%d) LOCK %p(%d), context: %d", cur_goid, (*mutex_ctxt_list)[i].state_addr, ref->state, cur_context);
            break;
        }
    }
}

void
CheckUnlockState(void *drcontext, int64_t cur_goid, context_handle_t cur_ctxt_hndl, mem_ref_t *ref)
{
    app_pc addr = ref->addr;
    // DRCCTLIB_PRINTF("addr %p", ref->addr);
    for (size_t i = 0; i < mutex_ctxt_list->size(); i++) {
        if (addr == (*mutex_ctxt_list)[i].state_addr) {
            (*lock_records)[cur_goid].emplace_back(0, (*mutex_ctxt_list)[i].create_context);
            context_handle_t cur_context = drcctlib_get_context_handle(drcontext);
            (*test_lock_records)[cur_goid].emplace_back(0, (*mutex_ctxt_list)[i].state_addr, cur_context);
            DRCCTLIB_PRINTF("GOID(%d) Unlock %p(%d), context: %d", cur_goid, (*mutex_ctxt_list)[i].state_addr, ref->state, cur_context);
            break;
        }
    }
}

// dr clean call
void
InsertCleancall(int32_t slot, int32_t num, int32_t state)
{
    void *drcontext = dr_get_current_drcontext();
    per_thread_t *pt = (per_thread_t *)drmgr_get_tls_field(drcontext, tls_idx);
    context_handle_t cur_ctxt_hndl = drcctlib_get_context_handle(drcontext, slot);
    int64_t cur_goid = 0;
    if(pt->goid_list->size() > 0) {
        cur_goid = pt->goid_list->back();
    }
    for (int i = 0; i < num; i++) {
        if (pt->cur_buf_list[i].addr != 0 && pt->cur_buf_list[i].state == 0) {
            if(state == 1) {
                CheckLockState(drcontext, cur_goid, cur_ctxt_hndl, &pt->cur_buf_list[i]);
            } else if (state == 2) {
                CheckUnlockState(drcontext, cur_goid, cur_ctxt_hndl, &pt->cur_buf_list[i]);
            }
        }
    }
    BUF_PTR(pt->cur_buf, mem_ref_t, INSTRACE_TLS_OFFS_BUF_PTR) = pt->cur_buf_list;
}

// insert
static void
InstrumentMem(void *drcontext, instrlist_t *ilist, instr_t *where, opnd_t ref,
              reg_id_t free_reg, uint64_t state)
{
    /* We need two scratch registers */
    reg_id_t reg_mem_ref_ptr;
    if (drreg_reserve_register(drcontext, ilist, where, NULL, &reg_mem_ref_ptr) !=
        DRREG_SUCCESS) {
        DRCCTLIB_EXIT_PROCESS("InstrumentMem drreg_reserve_register != DRREG_SUCCESS");
    }
    if (!drutil_insert_get_mem_addr(drcontext, ilist, where, ref, free_reg,
                                    reg_mem_ref_ptr)) {
        MINSERT(ilist, where,
                XINST_CREATE_load_int(drcontext, opnd_create_reg(free_reg),
                                      OPND_CREATE_CCT_INT(0)));
    }
    dr_insert_read_raw_tls(drcontext, ilist, where, tls_seg,
                           tls_offs + INSTRACE_TLS_OFFS_BUF_PTR, reg_mem_ref_ptr);
    // store mem_ref_t->addr
    MINSERT(ilist, where,
            XINST_CREATE_store(
                drcontext, OPND_CREATE_MEMPTR(reg_mem_ref_ptr, offsetof(mem_ref_t, addr)),
                opnd_create_reg(free_reg)));
    
    // store mem_ref_t->addr
    MINSERT(ilist, where,
            XINST_CREATE_load_int(drcontext, opnd_create_reg(free_reg),
                                  OPND_CREATE_CCT_INT(state)));
    MINSERT(ilist, where,
            XINST_CREATE_store(
                drcontext, OPND_CREATE_MEMPTR(reg_mem_ref_ptr, offsetof(mem_ref_t, state)),
                opnd_create_reg(free_reg)));

#ifdef ARM_CCTLIB
    MINSERT(ilist, where,
            XINST_CREATE_load_int(drcontext, opnd_create_reg(free_reg),
                                  OPND_CREATE_CCT_INT(sizeof(mem_ref_t))));
    MINSERT(ilist, where,
            XINST_CREATE_add(drcontext, opnd_create_reg(reg_mem_ref_ptr),
                             opnd_create_reg(free_reg)));
#else
    MINSERT(ilist, where,
            XINST_CREATE_add(drcontext, opnd_create_reg(reg_mem_ref_ptr),
                             OPND_CREATE_CCT_INT(sizeof(mem_ref_t))));
#endif
    dr_insert_write_raw_tls(drcontext, ilist, where, tls_seg,
                            tls_offs + INSTRACE_TLS_OFFS_BUF_PTR, reg_mem_ref_ptr);
    /* Restore scratch registers */
    if (drreg_unreserve_register(drcontext, ilist, where, reg_mem_ref_ptr) !=
        DRREG_SUCCESS) {
        DRCCTLIB_EXIT_PROCESS("InstrumentMem drreg_unreserve_register != DRREG_SUCCESS");
    }
}


// analysis
void
InstrumentInsCallback(void *drcontext, instr_instrument_msg_t *instrument_msg)
{
    instrlist_t *bb = instrument_msg->bb;
    instr_t *instr = instrument_msg->instr;
    int32_t slot = instrument_msg->slot;
    int64_t state = 0;
    if (instr_get_prefix_flag(instr, PREFIX_LOCK) &&
        (instr_get_opcode(instr) == OP_cmpxchg ||
         instr_get_opcode(instr) == OP_cmpxchg8b ||
         instr_get_opcode(instr) == OP_cmpxchg16b)) {
        state = 1;
    }

    if (instr_get_prefix_flag(instr, PREFIX_LOCK) &&
        instr_get_opcode(instr) == OP_xadd) {
        state = 2;
    }
    if (state == 0){
        return;
    }
    int num = 0;
#ifdef x86_CCTLIB
    if (drreg_reserve_aflags(drcontext, bb, instr) != DRREG_SUCCESS) {
        DRCCTLIB_EXIT_PROCESS("instrument_before_every_instr_meta_instr "
                              "drreg_reserve_aflags != DRREG_SUCCESS");
    }
#endif
    reg_id_t reg_temp;
    if (drreg_reserve_register(drcontext, bb, instr, NULL, &reg_temp) != DRREG_SUCCESS) {
        DRCCTLIB_EXIT_PROCESS(
            "InstrumentInsCallback drreg_reserve_register != DRREG_SUCCESS");
    }
    for (int i = 0; i < instr_num_srcs(instr); i++) {
        if (opnd_is_memory_reference(instr_get_src(instr, i))) {
            InstrumentMem(drcontext, bb, instr, instr_get_src(instr, i), reg_temp, 0);
            num++;
        }
    }
    for (int i = 0; i < instr_num_dsts(instr); i++) {
        if (opnd_is_memory_reference(instr_get_dst(instr, i))) {
            InstrumentMem(drcontext, bb, instr, instr_get_dst(instr, i), reg_temp, 1);
            num++;
        }
    }
    if (drreg_unreserve_register(drcontext, bb, instr, reg_temp) != DRREG_SUCCESS) {
        DRCCTLIB_EXIT_PROCESS(
            "InstrumentInsCallback drreg_unreserve_register != DRREG_SUCCESS");
    }
#ifdef x86_CCTLIB
    if (drreg_unreserve_aflags(drcontext, bb, instr) != DRREG_SUCCESS) {
        DRCCTLIB_EXIT_PROCESS("drreg_unreserve_aflags != DRREG_SUCCESS");
    }
#endif
    dr_insert_clean_call(drcontext, bb, instr, (void *)InsertCleancall, false, 3,
                        OPND_CREATE_CCT_INT(slot), OPND_CREATE_CCT_INT(num), OPND_CREATE_CCT_INT(state));
    // }
}

static void
WrapBeforeRTExecute(void *wrapcxt, void **user_data)
{
    void *drcontext = (void *)drwrap_get_drcontext(wrapcxt);
    per_thread_t *pt = (per_thread_t *)drmgr_get_tls_field(drcontext, tls_idx);
    go_g_t* go_g_ptr = (go_g_t*)dgw_get_go_func_arg(wrapcxt, 0);
    context_handle_t cur_context = drcctlib_get_context_handle(drcontext);
    pt->call_rt_exec_list->push_back(cur_context);
    pt->goid_list->push_back(go_g_ptr->goid);

    vector<int64_t> ancestors;
    go_slice_t* ancestors_ptr = go_g_ptr->ancestors;
    if (ancestors_ptr != NULL) {
        go_ancestor_info_t* ancestor_infor_array = (go_ancestor_info_t*)ancestors_ptr->data;
        for(int i = 0; i < ancestors_ptr->len; i++) {
            ancestors.push_back(ancestor_infor_array[i].goid);
        }
    }
    pt->go_ancestors_list->push_back(ancestors);
}

static void
WrapBeforeRTNewObj(void *wrapcxt, void **user_data)
{
    go_type_t* go_type_ptr = (go_type_t*)dgw_get_go_func_arg(wrapcxt, 0);
    if(cgo_type_kind_is(go_type_ptr, go_kind_t::kindStruct)) {
        *user_data = (void*)(go_type_ptr);
    } else {
        *user_data = NULL;
    }
}

static void
WrapEndRTNewObj(void *wrapcxt, void *user_data)
{
    if(user_data == NULL) {
        return;
    }
    void* drcontext = (void *)drwrap_get_drcontext(wrapcxt);
    if(drcontext == NULL) {
        drcontext = dr_get_current_drcontext();
        if (drcontext == NULL) {
            DRCCTLIB_EXIT_PROCESS("ERROR: WrapEndRTNewObj drcontext == NULL");
        }
    }
    context_handle_t cur_context = drcctlib_get_context_handle(drcontext);
    go_type_t* go_type_ptr = (go_type_t*)user_data;
    string type_str = cgo_get_type_name_string(go_type_ptr, go_firstmoduledata);
    if(strcmp(type_str.c_str(), "sync.Mutex") == 0) {
        go_sync_mutex_t* ret_ptr = (go_sync_mutex_t*)dgw_get_go_func_retaddr(wrapcxt, 1, 0);
        mutex_ctxt_t mutxt_ctxt = {(app_pc)(&(ret_ptr->state)), cur_context, (app_pc)(ret_ptr), -1};
        DRCCTLIB_PRINTF("mutxt_ctxt %p %p %d", ret_ptr, mutxt_ctxt.state_addr, mutxt_ctxt.create_context);
        mutex_ctxt_list->push_back(mutxt_ctxt);
    } else if (strcmp(type_str.c_str(), "sync.WaitGroup") == 0) {
        go_sync_waitgroup_t* ret_ptr = (go_sync_waitgroup_t*) dgw_get_go_func_retaddr(wrapcxt, 1, 0);
        waitgroup_ctxt_t waitgroup_ctxt = {(app_pc) ret_ptr, cur_context, 0, 0};
        wg_ctxt_list->push_back(waitgroup_ctxt);
        DRCCTLIB_PRINTF("wg_ctxt %p %d", ret_ptr, waitgroup_ctxt.create_context);
    } else {
        void* ret_ptr = NULL;
        uint64_t offset = 0;
        go_struct_type_t* go_struct_type_ptr = (go_struct_type_t*)go_type_ptr;
        for(int64_t i = 0; i < cgo_get_struct_fields_length(go_struct_type_ptr); i++) {
            go_type_t* field_type = cgo_get_struct_field_type(go_struct_type_ptr, i);
            if (field_type) {
                string field_type_str = cgo_get_type_name_string(field_type, go_firstmoduledata);
                if(strcmp(field_type_str.c_str(), "sync.Mutex") == 0) {
                    if (!ret_ptr) {
                        ret_ptr = (void*)dgw_get_go_func_retaddr(wrapcxt, 1, 0);
                        if (!ret_ptr) {
                            continue;
                        }
                    }
                    go_sync_mutex_t* mutex_ptr = (go_sync_mutex_t*)((uint64_t)ret_ptr + offset);
                    mutex_ctxt_t mutxt_ctxt = {(app_pc)(&(mutex_ptr->state)), cur_context, (app_pc)(ret_ptr), -1};
                    DRCCTLIB_PRINTF("mutxt_ctxt %p %p %d", ret_ptr, mutxt_ctxt.state_addr, mutxt_ctxt.create_context);
                    mutex_ctxt_list->push_back(mutxt_ctxt);
                } else if (strcmp(field_type_str.c_str(), "sync.WaitGroup") == 0) {
                    if (!ret_ptr) {
                        ret_ptr = (void*)dgw_get_go_func_retaddr(wrapcxt, 1, 0);
                        if (!ret_ptr) {
                            continue;
                        }
                    }
                    go_sync_waitgroup_t* wg_ptr = (go_sync_waitgroup_t*) ((uint64_t)ret_ptr + offset);
                    waitgroup_ctxt_t waitgroup_ctxt = {(app_pc) wg_ptr, cur_context, 0, 0};
                    wg_ctxt_list->push_back(waitgroup_ctxt);
                    DRCCTLIB_PRINTF("wg_ctxt %p %d", ret_ptr, waitgroup_ctxt.create_context);
                }
            }
            offset += (uint64_t)field_type->size;
        }
        // DRCCTLIB_PRINTF("[%s]", type_str.c_str());
        // DRCCTLIB_PRINTF("[%s]{%ld}", type_str.c_str(), go_type_ptr->size);
        // DRCCTLIB_PRINTF("[%s]{%ld}", type_str.c_str(), cgo_get_struct_fields_length((go_struct_type_t*)go_type_ptr));
    }
}

static void
WrapBeforeSyncUnlockSlow(void *wrapcxt, void **user_data)
{
    void *drcontext = (void *)drwrap_get_drcontext(wrapcxt);
    per_thread_t *pt = (per_thread_t *)drmgr_get_tls_field(drcontext, tls_idx);
    void* mutex_ptr = (void*)dgw_get_go_func_arg(wrapcxt, 0);
    mutex_ctxt_t* unlock_slow_mutex_ctxt = NULL;
    for (size_t i = 0; i < mutex_ctxt_list->size(); i++) {
        if ((app_pc)mutex_ptr == (*mutex_ctxt_list)[i].state_addr) {
            unlock_slow_mutex_ctxt = &(*mutex_ctxt_list)[i];
            break;
        }
    }
    unlock_slow_mutex_ctxt->cur_unlock_slow_goid = pt->goid_list->back();
    *user_data = (void*)(unlock_slow_mutex_ctxt);
}

static void
WrapEndSyncUnlockSlow(void *wrapcxt, void *user_data)
{
    mutex_ctxt_t* unlock_slow_mutex_ctxt = (mutex_ctxt_t*)user_data;
    if(unlock_slow_mutex_ctxt) {
        unlock_slow_mutex_ctxt->cur_unlock_slow_goid = -1;
    }
}

static void
WrapEndRTMakechan(void *wrapcxt, void *user_data)
{
    void* drcontext = (void *)drwrap_get_drcontext(wrapcxt);
    if(drcontext == NULL) {
        drcontext = dr_get_current_drcontext();
        if (drcontext == NULL) {
            DRCCTLIB_EXIT_PROCESS("ERROR: WrapEndRTNewObj drcontext == NULL");
        }
    }
    context_handle_t cur_context = drcctlib_get_context_handle(drcontext);
    go_hchan_t *chan_ptr = (go_hchan_t*) dgw_get_go_func_retaddr(wrapcxt, 2, 0);
    string chan_type_str = cgo_get_type_name_string((go_type_t*) chan_ptr->elemtype, go_firstmoduledata);
    DRCCTLIB_PRINTF("channel: %p, type: %s, size: %ld\n", chan_ptr, chan_type_str.c_str(), chan_ptr->dataqsiz);
}

static void
WrapBeforeRTChansend(void *wrapcxt, void **user_data)
{
    go_hchan_t *chan_ptr = (go_hchan_t*) dgw_get_go_func_arg(wrapcxt, 0);
    void *drcontext = dr_get_current_drcontext();
    per_thread_t *pt = (per_thread_t *)drmgr_get_tls_field(drcontext, tls_idx);
    int64_t cur_goid = pt->goid_list->back();
    context_handle_t cur_context = drcctlib_get_context_handle(drcontext);
    (*chan_op_records)[cur_goid].emplace_back(cur_goid, 1, (app_pc) chan_ptr, cur_context);
    (*op_records_per_chan)[(app_pc) chan_ptr].emplace_back(cur_goid, 1, (app_pc) chan_ptr, cur_context);
    DRCCTLIB_PRINTF("send to channel: %p, context: %ld\n", chan_ptr, cur_context);
}

static void
WrapBeforeRTChanrecv(void *wrapcxt, void **user_data)
{
    go_hchan_t *chan_ptr = (go_hchan_t*) dgw_get_go_func_arg(wrapcxt, 0);
    void *drcontext = dr_get_current_drcontext();
    per_thread_t *pt = (per_thread_t *)drmgr_get_tls_field(drcontext, tls_idx);
    int64_t cur_goid = pt->goid_list->back();
    context_handle_t cur_context = drcctlib_get_context_handle(drcontext);
    (*chan_op_records)[cur_goid].emplace_back(cur_goid, 2, (app_pc) chan_ptr, cur_context);
    (*op_records_per_chan)[(app_pc) chan_ptr].emplace_back(cur_goid, 2, (app_pc) chan_ptr, cur_context);
    DRCCTLIB_PRINTF("Receive from channel: %p, context: %ld\n", chan_ptr, cur_context);
}

static void
WrapBeforeRTClosechan(void *wrapcxt, void **user_data)
{
    go_hchan_t *chan_ptr = (go_hchan_t*) dgw_get_go_func_arg(wrapcxt, 0);
    void *drcontext = dr_get_current_drcontext();
    per_thread_t *pt = (per_thread_t *)drmgr_get_tls_field(drcontext, tls_idx);
    int64_t cur_goid = pt->goid_list->back();
    context_handle_t cur_context = drcctlib_get_context_handle(drcontext);
    (*chan_op_records)[cur_goid].emplace_back(cur_goid, 0, (app_pc) chan_ptr, cur_context);
    (*op_records_per_chan)[(app_pc) chan_ptr].emplace_back(cur_goid, 0, (app_pc) chan_ptr, cur_context);
    DRCCTLIB_PRINTF("Close channel: %p, context: %ld\n", chan_ptr, cur_context);
}

static void
WrapBeforeSyncAdd(void *wrapcxt, void **user_data)
{
    go_sync_waitgroup_t *wg_ptr = (go_sync_waitgroup_t*) dgw_get_go_func_arg(wrapcxt, 0);
    int64_t wg_delta = (int64_t) dgw_get_go_func_arg(wrapcxt, 1);
    for (size_t i = 0; i < wg_ctxt_list->size(); i++) {
        if ((app_pc) wg_ptr == (*wg_ctxt_list)[i].addr) {
            (*wg_ctxt_list)[i].counter += wg_delta;
            DRCCTLIB_PRINTF("add %ld to waitgroup %p, now %ld", wg_delta, wg_ptr, (*wg_ctxt_list)[i].counter);
            break;
        }
    }
}

static void
WrapBeforeSyncWait(void *wrapcxt, void **user_data)
{
    void *drcontext = dr_get_current_drcontext();
    context_handle_t cur_context = drcctlib_get_context_handle(drcontext);
    go_sync_waitgroup_t *wg_ptr = (go_sync_waitgroup_t*) dgw_get_go_func_arg(wrapcxt, 0);
    for (size_t i = 0; i < wg_ctxt_list->size(); i++) {
        if ((app_pc) wg_ptr == (*wg_ctxt_list)[i].addr) {
            (*wg_ctxt_list)[i].wait_context = cur_context;
            DRCCTLIB_PRINTF("%p is waiting at %d", wg_ptr, cur_context);
            break;
        }
    }
}

static go_moduledata_t*
GetGoFirstmoduledata(const module_data_t *info)
{
    file_t fd = dr_open_file(info->full_path, DR_FILE_READ);
    uint64 file_size;
    if (fd == INVALID_FILE) {
        if (strcmp(info->full_path, "[vdso]") != 0) {
            DRCCTLIB_PRINTF("------ unable to open %s", info->full_path);
        }
        return NULL;
    }
    if (!dr_file_size(fd, &file_size)) {
        DRCCTLIB_PRINTF("------ unable to get file size %s", info->full_path);
        return NULL;
    }
    size_t map_size = file_size;
    void *map_base = dr_map_file(fd, &map_size, 0, NULL, DR_MEMPROT_READ, DR_MAP_PRIVATE);
    /* map_size can be larger than file_size */
    if (map_base == NULL || map_size < file_size) {
        DRCCTLIB_PRINTF("------ unable to map %s", info->full_path);
        return NULL;
    }
    go_moduledata_t* firstmoduledata = NULL;
    // in memory
    Elf *elf = elf_memory((char *)map_base, map_size); // Initialize 'elf' pointer to our file descriptor
    if(find_elf_section_by_name(elf, ".go.buildinfo")) {
        uint64_t gopclntab_addr = 0;
        Elf_Scn *gopclntab_scn = find_elf_section_by_name(elf, ".gopclntab");
        if(gopclntab_scn) {
            Elf_Shdr *section_header = elf_getshdr(gopclntab_scn);
            gopclntab_addr = section_header->sh_addr;
            // DRCCTLIB_PRINTF(".gopclntab start addr %p", gopclntab_addr);
        }
        Elf_Scn *noptrdata_scn = find_elf_section_by_name(elf, ".noptrdata");
        if(noptrdata_scn) {
            Elf_Shdr *section_header = elf_getshdr(noptrdata_scn);
            uint64_t start_addr = section_header->sh_addr;
            uint64_t end_addr = start_addr + section_header->sh_size * 8;
            // DRCCTLIB_PRINTF("module start addr %p, end addr %p", start_addr, end_addr);
            for(uint64_t temp = start_addr; temp < end_addr; temp += 8) {
                if (*((uint64_t*)temp)== gopclntab_addr) {
                    firstmoduledata = (go_moduledata_t*) temp;
                    break;
                }
            }
        }
    }
    dr_unmap_file(map_base, map_size);
    dr_close_file(fd);
    return firstmoduledata;
}


static inline app_pc
moudle_get_function_entry(const module_data_t *info, const char *func_name,
                          bool check_internal_func)
{
    app_pc functionEntry;
    if (check_internal_func) {
        size_t offs;
        if (drsym_lookup_symbol(info->full_path, func_name, &offs, DRSYM_DEMANGLE) ==
            DRSYM_SUCCESS) {
            functionEntry = offs + info->start;
        } else {
            functionEntry = NULL;
        }
    } else {
        functionEntry = (app_pc)dr_get_proc_address(info->handle, func_name);
    }
    return functionEntry;
}

static void
OnMoudleLoad(void *drcontext, const module_data_t *info,
                                    bool loaded)
{
    const char *modname = dr_module_preferred_name(info);
    for (std::vector<std::string>::iterator i = blacklist->begin();
            i != blacklist->end(); ++i) {
        if(strstr(modname, i->c_str())) {
            return;
        }
    }
    if(go_firstmoduledata == NULL) {
        go_firstmoduledata = GetGoFirstmoduledata(info);
    }
    app_pc func_rt_newobj_entry = moudle_get_function_entry(info, "runtime.newobject", true);
    if (func_rt_newobj_entry != NULL) {
        drwrap_wrap(func_rt_newobj_entry, WrapBeforeRTNewObj, WrapEndRTNewObj);
    }
    app_pc func_sync_ulock_slow_entry = moudle_get_function_entry(info, "sync.(*Mutex).unlockSlow", true);
    if (func_sync_ulock_slow_entry != NULL) {
        drwrap_wrap(func_sync_ulock_slow_entry, WrapBeforeSyncUnlockSlow, WrapEndSyncUnlockSlow);
    }
    app_pc func_entry = moudle_get_function_entry(info, "runtime.execute", true);
    if (func_entry != NULL) {
        drwrap_wrap(func_entry, WrapBeforeRTExecute, NULL);
    }
    app_pc func_makechan_entry = moudle_get_function_entry(info, "runtime.makechan", true);
    if (func_makechan_entry != NULL) {
        drwrap_wrap(func_makechan_entry, NULL, WrapEndRTMakechan);
    }
    app_pc func_chansend_entry = moudle_get_function_entry(info, "runtime.chansend", true);
    if (func_chansend_entry != NULL) {
        drwrap_wrap(func_chansend_entry, WrapBeforeRTChansend, NULL);
    }
    app_pc func_chanrecv_entry = moudle_get_function_entry(info, "runtime.chanrecv", true);
    if (func_chanrecv_entry != NULL) {
        drwrap_wrap(func_chanrecv_entry, WrapBeforeRTChanrecv, NULL);
    }
    app_pc func_closechan_entry = moudle_get_function_entry(info, "runtime.closechan", true);
    if (func_closechan_entry != NULL) {
        drwrap_wrap(func_closechan_entry, WrapBeforeRTClosechan, NULL);
    }
    app_pc func_sync_add_entry = moudle_get_function_entry(info, "sync.(*WaitGroup).Add", true);
    if (func_sync_add_entry != NULL) {
        drwrap_wrap(func_sync_add_entry, WrapBeforeSyncAdd, NULL);
    }
    app_pc func_sync_wait_entry = moudle_get_function_entry(info, "sync.(*WaitGroup).Wait", true);
    if (func_sync_wait_entry != NULL) {
        drwrap_wrap(func_sync_wait_entry, WrapBeforeSyncWait, NULL);
    }
    // DRCCTLIB_PRINTF("finish module name %s", modname);
}

static void
PrintAllRTExec(per_thread_t *pt)
{
    dr_mutex_lock(thread_sync_lock);

    for (uint64_t i = 0; i < pt->goid_list->size(); i++) {
        context_handle_t exec_ctxt = (*(pt->call_rt_exec_list))[i];
        dr_fprintf(gTraceFile, "\nthread(%ld) runtime.execute to test_goid(%d)", pt->thread_id, (*(pt->goid_list))[i]);    
        drcctlib_print_ctxt_hndl_msg(gTraceFile, exec_ctxt, false, false);

        if((*(pt->go_ancestors_list))[i].size() > 0) {
            dr_fprintf(gTraceFile, "created by Goroutine(s) ");
            for (uint64_t j = 0; j < (*(pt->go_ancestors_list))[i].size(); j++) {
                if (j) {
                    dr_fprintf(gTraceFile, " -> ");
                }
                dr_fprintf(gTraceFile, "%ld", (*(pt->go_ancestors_list))[i][j]);
            }
            dr_fprintf(gTraceFile, "\n");
        }

        dr_fprintf(gTraceFile,
                   "====================================================================="
                   "===========\n");
        drcctlib_print_full_cct(gTraceFile, exec_ctxt, true, true,
                                -1);
        dr_fprintf(gTraceFile,
                   "====================================================================="
                   "===========\n\n\n");
    }
    dr_mutex_unlock(thread_sync_lock);
}

static void
ClientThreadStart(void *drcontext)
{
    per_thread_t *pt = (per_thread_t *)dr_thread_alloc(drcontext, sizeof(per_thread_t));
    if (pt == NULL) {
        DRCCTLIB_EXIT_PROCESS("pt == NULL");
    }
    drmgr_set_tls_field(drcontext, tls_idx, (void *)pt);
    pt->thread_id = dr_get_thread_id(drcontext);
    pt->cur_buf = dr_get_dr_segment_base(tls_seg);
    pt->cur_buf_list =
        (mem_ref_t *)dr_global_alloc(TLS_MEM_REF_BUFF_SIZE * sizeof(mem_ref_t));
    BUF_PTR(pt->cur_buf, mem_ref_t, INSTRACE_TLS_OFFS_BUF_PTR) = pt->cur_buf_list;
    pt->call_rt_exec_list = new vector<context_handle_t>;
    pt->goid_list = new vector<int64_t>;
    pt->go_ancestors_list = new vector<vector<int64_t>>;
}

static void
ClientThreadEnd(void *drcontext)
{
    per_thread_t *pt = (per_thread_t *)drmgr_get_tls_field(drcontext, tls_idx);
    PrintAllRTExec(pt);
    dr_global_free(pt->cur_buf_list, TLS_MEM_REF_BUFF_SIZE * sizeof(mem_ref_t));
    delete pt->call_rt_exec_list;
    delete pt->goid_list;
    delete pt->go_ancestors_list;
    dr_thread_free(drcontext, pt, sizeof(per_thread_t));
}


bool
InterestInstrFilter(instr_t *instr)
{
    return  instr_get_prefix_flag(instr, PREFIX_LOCK) && 
        (instr_get_opcode(instr) == OP_cmpxchg ||
        instr_get_opcode(instr) == OP_cmpxchg8b ||
        instr_get_opcode(instr) == OP_cmpxchg16b ||
        instr_get_opcode(instr) == OP_xadd);
}


static void
InitMoudlesBlacklist()
{
    blacklist->push_back("libdrcctlib_goroutines.so");
    blacklist->push_back("libdynamorio.so");
    blacklist->push_back("linux-vdso.so");
}

static void
InitBuffer()
{
    blacklist = new std::vector<std::string>();
    mutex_ctxt_list = new vector<mutex_ctxt_t>();
    wg_ctxt_list = new vector<waitgroup_ctxt_t>();
    lock_records = new unordered_map<int64_t, vector<pair<bool, context_handle_t>>>();
    test_lock_records = new unordered_map<int64_t, vector<lock_record_t>>();
    chan_op_records = new unordered_map<int64_t, vector<chan_op_record_t>>();
    op_records_per_chan = new unordered_map<app_pc, vector<chan_op_record_t>>();
}

static void
FreeBuffer()
{
    delete blacklist;
    delete mutex_ctxt_list;
    delete wg_ctxt_list;
    delete lock_records;
    delete test_lock_records;
    delete chan_op_records;
    delete op_records_per_chan;
}

static void
DetectDeadlock()
{

    vector<vector<deadlock_t>> deadlock_list;
    unordered_set<int64_t> finished_set;
    unordered_map<int64_t, unordered_multimap<app_pc, unordered_set<app_pc>>> lock_sequences;
    struct lock_pair {
        app_pc m1;
        app_pc m2;

        bool operator==(const lock_pair &l) const
        {
            return m1 == l.m1 && m2 == l.m2;
        }
    };
    struct hash_func
    {
        size_t operator() (const lock_pair &rhs) const
        {
            size_t h1 = hash<app_pc>()(rhs.m1);
            size_t h2 = hash<app_pc>()(rhs.m2);
            return h1 ^ h2;
        }
    };
    unordered_map<lock_pair, unordered_set<int64_t>, hash_func> lock_pair_goid_map;
    // create other mutex lock sequences after a mutex lock
    for (auto it = test_lock_records->begin(); it != test_lock_records->end(); it++) {
        unordered_map<app_pc, unordered_set<app_pc>> active_sets;
        unordered_set<app_pc> active_mutexes;
        for (const auto &record : it->second) {
            if (record.op) {
                // if it is a lock, add it into other active mutexes' sets and make the mutex active
                for (app_pc mutex : active_mutexes) {
                    if (record.mutex_addr != mutex) {
                        active_sets[mutex].insert(record.mutex_addr);
                        lock_pair temp = {mutex < record.mutex_addr ? mutex : record.mutex_addr, 
                                          mutex > record.mutex_addr ? mutex : record.mutex_addr};
                        lock_pair_goid_map[temp].insert(it->first);
                    }
                }
                active_mutexes.insert(record.mutex_addr);
            } else {
                // if it is an unlock, add the mutex's sets to lock_sequences and make the mutex inactive
                if (!active_sets[record.mutex_addr].empty()) {
                    lock_sequences[it->first].emplace(record.mutex_addr, active_sets[record.mutex_addr]);
                }
                active_sets.erase(record.mutex_addr);
                active_mutexes.erase(record.mutex_addr);
            }
        }
    }

    // detect deadlocks based on the lock_sequences map (two mutexes)
    for (const auto &goid_based_seq : lock_sequences) {
        for (const auto &mutex_based_seq : goid_based_seq.second) {
            for (const auto &m : mutex_based_seq.second) {
                lock_pair temp = {mutex_based_seq.first < m ? mutex_based_seq.first : m, 
                                  mutex_based_seq.first > m ? mutex_based_seq.first : m};
                for (auto it = lock_pair_goid_map[temp].begin(); it != lock_pair_goid_map[temp].end(); it++) {
                    if (*it != goid_based_seq.first && 
                        finished_set.find(*it) == finished_set.end()) {
                        
                        for (auto search = lock_sequences[*it].find(m);
                             search != lock_sequences[*it].end(); search++) {
                        
                            if (search->second.find(mutex_based_seq.first) != 
                                search->second.end()) {
                                
                                vector<deadlock_t> current_deadlock_group;
                                current_deadlock_group.push_back({goid_based_seq.first, mutex_based_seq.first, m});
                                current_deadlock_group.push_back({*it, m, mutex_based_seq.first});
                                deadlock_list.push_back(current_deadlock_group);
                            }
                        }
                    }
                }
            }
        }
        finished_set.insert(goid_based_seq.first);
    }
    finished_set.clear();

    //multiple mutexes
    for (const auto &goid_based_seq : lock_sequences) {
        for (const auto &mutex_based_seq : goid_based_seq.second) {
            for (const auto &m : mutex_based_seq.second) {
                vector<deadlock_t> current_deadlock_group;
                current_deadlock_group.push_back({goid_based_seq.first, mutex_based_seq.first, m});
                finished_set.insert(goid_based_seq.first);

                app_pc mutex_target = mutex_based_seq.first;
                bool has_deadlock = false;
                bool has_mutex_target = false;
                for (size_t i = 0; i < lock_sequences.size(); ++i) {
                    for (const auto &sub_goid_based_seq : lock_sequences) {
                        if (finished_set.find(sub_goid_based_seq.first) == finished_set.end()) {
                            for (const auto &sub_mutex_based_seq : sub_goid_based_seq.second) {
                                if (sub_mutex_based_seq.first != mutex_target) {
                                    // target mutex found
                                    if (sub_mutex_based_seq.second.find(mutex_target) != 
                                        sub_mutex_based_seq.second.end()) {
                                        
                                        finished_set.insert(sub_goid_based_seq.first);
                                        current_deadlock_group.push_back({sub_goid_based_seq.first, 
                                                                          sub_mutex_based_seq.first, 
                                                                          mutex_target});
                                        // has circular waiting or not
                                        if (sub_mutex_based_seq.first == m) {
                                            has_deadlock = true;
                                        } else {
                                            mutex_target = sub_mutex_based_seq.first;
                                            has_mutex_target = true;
                                        }
                                    }
                                }
                                if (has_mutex_target || has_deadlock) {
                                    break;
                                }
                            }
                        }
                        if (has_mutex_target || has_deadlock) {
                            break;
                        }
                    }
                    if (has_deadlock) {
                        break;
                    } else if (!has_mutex_target) {
                        break;
                    }
                    has_mutex_target = false;
                }
                if (has_deadlock) {
                    deadlock_list.push_back(current_deadlock_group);
                    has_deadlock = false;
                }
                finished_set.clear();
            }
        }
    }

    // channel related deadlock
    struct mutex_before_each_chan {
        int op;
        app_pc chan_addr;
        unordered_set<app_pc> locked_mutex_set;
        unordered_set<app_pc> unlocked_mutex_set;
    };
    unordered_map<int64_t, vector<mutex_before_each_chan>> mutex_before_chans;
    vector<vector<chan_deadlock_t>> chan_deadlock_list;

    // construct locked and unlocked mutex set before the channle operation
    for (auto it = chan_op_records->begin(); it != chan_op_records->end(); ++it) {
        for (const auto &chan_op_record : it->second) {
            mutex_before_each_chan temp;
            temp.op = chan_op_record.op;
            temp.chan_addr = chan_op_record.chan_addr;
            for (const auto &mutex_op : (*test_lock_records)[it->first]) {
                if (mutex_op.ctxt >= chan_op_record.ctxt) {
                    break;
                }
                if (mutex_op.op) {
                    temp.locked_mutex_set.insert(mutex_op.mutex_addr);
                    temp.unlocked_mutex_set.insert(mutex_op.mutex_addr);
                } else {
                    temp.locked_mutex_set.erase(mutex_op.mutex_addr);
                }
            }
            mutex_before_chans[it->first].push_back(temp);
        }
    }

    // detect deadlock with channel
    for (auto it1 = mutex_before_chans.begin(); it1 != mutex_before_chans.end(); ++it1) {
        for (const auto &send_op : it1->second) {
            if (send_op.op == 1) {
                for (auto it2 = mutex_before_chans.begin(); it2 != mutex_before_chans.end(); ++it2) {
                    if (it2->first != it1->first) {
                        for (const auto &recv_op : it2->second) {
                            if ((recv_op.op == 2) && (send_op.chan_addr == recv_op.chan_addr)) {
                                for (const auto &locked_mutex : send_op.locked_mutex_set) {
                                    if (recv_op.locked_mutex_set.find(locked_mutex) != recv_op.locked_mutex_set.end()) {
                                        vector<chan_deadlock_t> cur_chan_deadlock;
                                        cur_chan_deadlock.push_back({it1->first, send_op.chan_addr, locked_mutex});
                                        cur_chan_deadlock.push_back({it2->first, send_op.chan_addr, locked_mutex});
                                        chan_deadlock_list.push_back(cur_chan_deadlock);
                                    }
                                    if (recv_op.unlocked_mutex_set.find(locked_mutex) != recv_op.unlocked_mutex_set.end()) {
                                        vector<chan_deadlock_t> cur_chan_deadlock;
                                        cur_chan_deadlock.push_back({it1->first, send_op.chan_addr, locked_mutex});
                                        cur_chan_deadlock.push_back({it2->first, send_op.chan_addr, locked_mutex});
                                        chan_deadlock_list.push_back(cur_chan_deadlock);
                                    }
                                }
                                for (const auto &unlocked_mutex : send_op.unlocked_mutex_set) {
                                    if (recv_op.locked_mutex_set.find(unlocked_mutex) != recv_op.locked_mutex_set.end()) {
                                        vector<chan_deadlock_t> cur_chan_deadlock;
                                        cur_chan_deadlock.push_back({it1->first, send_op.chan_addr, unlocked_mutex});
                                        cur_chan_deadlock.push_back({it2->first, send_op.chan_addr, unlocked_mutex});
                                        chan_deadlock_list.push_back(cur_chan_deadlock);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    //detect blocked channel
    vector<chan_op_record_t> blocked_channel_list;
    for (auto it = op_records_per_chan->begin(); it != op_records_per_chan->end(); ++it) {
        std::queue<chan_op_record_t> sends;
        std::queue<chan_op_record_t> recvs;
        for (const auto &record : it->second) {
            if (record.op == 1) {
                if (recvs.empty()) {
                    sends.push(record);
                } else {
                    recvs.pop();
                }
            } else if (record.op == 2) {
                if (sends.empty()) {
                    recvs.push(record);
                } else {
                    sends.pop();
                }
            } else {
                while ( !(sends.empty() && recvs.empty()) ) {
                    if (!sends.empty()) {
                        sends.pop();
                    }
                    if (!recvs.empty()) {
                        recvs.pop();
                    }
                }
                break;
            }
        }
        if (!sends.empty()) {
            blocked_channel_list.push_back({sends.front().goid, sends.front().op, 
                                            sends.front().chan_addr, sends.front().ctxt});
        } else if (!recvs.empty()) {
            blocked_channel_list.push_back({sends.front().goid, recvs.front().op, 
                                            recvs.front().chan_addr, recvs.front().ctxt});
        }
    }

    //detect blocked wait group
    dr_fprintf(gTraceFile, "Blocked wait groups:\n");
    for (size_t i = 0; i < wg_ctxt_list->size(); i++) {
        if ((*wg_ctxt_list)[i].counter > 0) {
            dr_fprintf(gTraceFile, "%p is blocked at %d\n", 
                       (*wg_ctxt_list)[i].addr, (*wg_ctxt_list)[i].wait_context);
        }
    }
    
    if (!deadlock_list.empty()) {
        dr_fprintf(gTraceFile, "Deadlocks:\n");
        for (const auto &deadlock_group : deadlock_list) {
            dr_fprintf(gTraceFile, "deadlock: \n");
            for (const auto &deadlock : deadlock_group) {
                dr_fprintf(gTraceFile, "          goid: %ld, mutex: %p, mutex: %p\n", 
                        deadlock.goid, deadlock.mutex1, deadlock.mutex2);
            }
            dr_fprintf(gTraceFile, "\n");
        }
    }

    if (!chan_deadlock_list.empty()) {
        dr_fprintf(gTraceFile, "Channel related deadlocks:\n");
        for (const auto &chan_deadlock_group : chan_deadlock_list) {
            dr_fprintf(gTraceFile, "deadlock: \n");
            for (const auto &deadlock : chan_deadlock_group) {
                dr_fprintf(gTraceFile, "        goid: %ld, chan: %p, mutex: %p\n",
                        deadlock.goid, deadlock.chan, deadlock.mutex);
            }
            dr_fprintf(gTraceFile, "\n");
        }
    }

    if (!blocked_channel_list.empty()) {
        dr_fprintf(gTraceFile, "Blocked channel:\n");
        for (const auto &blocked_channel : blocked_channel_list) {
            if (blocked_channel.op == 1) {
                dr_fprintf(gTraceFile, "        goid: %ld, channel: %p, context: %d, operation: send to\n", 
                        blocked_channel.goid, blocked_channel.chan_addr, blocked_channel.ctxt);
            } else {
                dr_fprintf(gTraceFile, "        goid: %ld, channel: %p, context: %d, operation: receive from\n", 
                        blocked_channel.goid, blocked_channel.chan_addr, blocked_channel.ctxt);
            }
        }
    }
}

static void
PorcessEndPrint()
{
    for (auto it = lock_records->begin(); it != lock_records->end(); it++) {
        dr_fprintf(gTraceFile, "goid %ld: \n", it->first);
        for (uint64_t i = 0; i < it->second.size(); i++) {
            if (it->second[i].first) {
                dr_fprintf(gTraceFile, "Lock %d\n", it->second[i].second);
            } else {
                dr_fprintf(gTraceFile, "Unlock %d\n", it->second[i].second);
            }
        }
        dr_fprintf(gTraceFile, "\n");
    }

    for (auto it = test_lock_records->begin(); it != test_lock_records->end(); it++) {
        dr_fprintf(gTraceFile, "goid %ld: \n", it->first);
        for (uint64_t i = 0; i < it->second.size(); i++) {
            if (it->second[i].op) {
                dr_fprintf(gTraceFile, "Lock %p\n", it->second[i].mutex_addr);
            } else {
                dr_fprintf(gTraceFile, "Unlock %p\n", it->second[i].mutex_addr);
            }
        }
        dr_fprintf(gTraceFile, "\n");
    }

    DetectDeadlock();
    
}

static void
ClientInit(int argc, const char *argv[])
{
    char name[MAXIMUM_PATH] = "";
    DRCCTLIB_INIT_LOG_FILE_NAME(name, "drcctlib_goroutines", "out");
    DRCCTLIB_PRINTF("Creating log file at:%s", name);

    gTraceFile = dr_open_file(name, DR_FILE_WRITE_OVERWRITE | DR_FILE_ALLOW_LARGE);
    DR_ASSERT(gTraceFile != INVALID_FILE);
    // print the arguments passed
    dr_fprintf(gTraceFile, "\n");
    for (int i = 0; i < argc; i++) {
        dr_fprintf(gTraceFile, "%d %s ", i, argv[i]);
    }
    dr_fprintf(gTraceFile, "\n");

    if (!drmgr_init()) {
        DRCCTLIB_EXIT_PROCESS("ERROR: drcctlib_goroutines "
                              "unable to initialize drmgr");
    }
    drreg_options_t ops = { sizeof(ops), 4 /*max slots needed*/, false };
    if (drreg_init(&ops) != DRREG_SUCCESS) {
        DRCCTLIB_EXIT_PROCESS(
            "ERROR: drcctlib_reuse_distance_client_cache unable to initialize drreg");
    }
    if (!drutil_init()) {
        DRCCTLIB_EXIT_PROCESS(
            "ERROR: drcctlib_reuse_distance_client_cache unable to initialize drutil");
    }
    if (!drwrap_init()) {
        DRCCTLIB_EXIT_PROCESS("ERROR: drcctlib_goroutines "
                              "unable to initialize drwrap");
    }

    tls_idx = drmgr_register_tls_field();
    if (tls_idx == -1) {
        DRCCTLIB_EXIT_PROCESS("ERROR: drcctlib_goroutines "
                              "drmgr_register_tls_field fail");
    }
    if (!dr_raw_tls_calloc(&tls_seg, &tls_offs, INSTRACE_TLS_COUNT, 0)) {
        DRCCTLIB_EXIT_PROCESS(
            "ERROR: drcctlib_reuse_distance_client_cache dr_raw_tls_calloc fail");
    }
    drmgr_priority_t after_drcctlib_thread_init_pri = { sizeof(after_drcctlib_thread_init_pri),
                                         "drcctlib_goroutines-thread_init", NULL, NULL,
                                         DRCCTLIB_THREAD_EVENT_PRI + 1 };
    drmgr_priority_t before_drcctlib_thread_exit_pri = { sizeof(before_drcctlib_thread_exit_pri),
                                         "drcctlib_goroutines-thread-exit", NULL, NULL,
                                         DRCCTLIB_THREAD_EVENT_PRI - 1 };
    drmgr_register_thread_init_event_ex(ClientThreadStart, &after_drcctlib_thread_init_pri);
    drmgr_register_thread_exit_event_ex(ClientThreadEnd, &before_drcctlib_thread_exit_pri);

    drmgr_priority_t after_drcctlib_module_load = { sizeof(after_drcctlib_module_load), "after_drcctlib_module_load",
                                         NULL, NULL, DRCCTLIB_MODULE_REGISTER_PRI + 1 };
    drmgr_register_module_load_event_ex(OnMoudleLoad, &after_drcctlib_module_load);

    drcctlib_init(InterestInstrFilter, INVALID_FILE, InstrumentInsCallback,
                  false);
    if (drsym_init(0) != true) {
        DRCCTLIB_EXIT_PROCESS("ERROR: drcctlib_goroutines "
                              "unable to initialize drsym");
    }
    thread_sync_lock = dr_mutex_create();
    InitBuffer();
    InitMoudlesBlacklist();
}

static void
ClientExit(void)
{
    PorcessEndPrint();
    drcctlib_exit();

    if (!dr_raw_tls_cfree(tls_offs, INSTRACE_TLS_COUNT)) {
        DRCCTLIB_EXIT_PROCESS(
            "ERROR: drcctlib_reuse_distance_client_cache dr_raw_tls_calloc fail");
    }

    if (!drmgr_unregister_thread_init_event(ClientThreadStart) ||
        !drmgr_unregister_thread_exit_event(ClientThreadEnd) ||
        !drmgr_unregister_tls_field(tls_idx)) {
        DRCCTLIB_PRINTF(
            "ERROR: drcctlib_goroutines failed to "
            "unregister in ClientExit");
    }

    if (drsym_exit() != DRSYM_SUCCESS) {
        DRCCTLIB_PRINTF("failed to exit drsym");
    }
    drwrap_exit();
    drmgr_exit();
    if (drreg_exit() != DRREG_SUCCESS) {
        DRCCTLIB_PRINTF("failed to exit drreg");
    }
    drutil_exit();
    dr_mutex_destroy(thread_sync_lock);
    FreeBuffer();
}

#ifdef __cplusplus
extern "C" {
#endif

DR_EXPORT void
dr_client_main(client_id_t id, int argc, const char *argv[])
{
    dr_set_client_name(
        "DynamoRIO Client 'drcctlib_goroutines'",
        "http://dynamorio.org/issues");
    ClientInit(argc, argv);
    dr_register_exit_event(ClientExit);
}

#ifdef __cplusplus
}
#endif