/**
 * Copyright (C) 2005-2008 Christoph Rupp (chris@crupp.de).
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version.
 *
 * See file COPYING.GPL2 and COPYING.GPL3 for License information.
 *
 */

#include <string.h>
#include "log.h"
#include "os.h"

#define LOG_DEFAULT_THRESHOLD   64

static ham_status_t
my_log_clear_file(ham_log_t *log, int idx)
{
    ham_status_t st;

    st=os_truncate(log_get_fd(log, idx), sizeof(log_header_t));
    if (st)
        return (db_set_error(log_get_db(log), st));

    /* after truncate, the file pointer is far beyond the new end of file;
     * reset the file pointer, or the next write will resize the file to
     * the original size */
    st=os_seek(log_get_fd(log, idx), sizeof(log_header_t), HAM_OS_SEEK_SET);
    if (st)
        return (db_set_error(log_get_db(log), st));

    /* clear the transaction counters */
    log_set_open_txn(log, idx, 0);
    log_set_closed_txn(log, idx, 0);

    return (0);
}

static ham_status_t
my_insert_checkpoint(ham_log_t *log)
{
    ham_status_t st;
    
    /*
     * first, flush the file; then append the checkpoint
     *
     * TODO for this flush, we don't need to insert 
     * LOG_ENTRY_TYPE_FLUSH_PAGE
     *
     * TODO
     * also make sure that the flush itself does not insert a checkpoint!
     */
    st=ham_flush(log_get_db(log), 0);
    if (st)
        return (st);

    return (ham_log_append_checkpoint(log));
}

ham_log_t *
ham_log_create(ham_db_t *db, const char *dbpath, 
        ham_u32_t mode, ham_u32_t flags)
{
    int i;
    log_header_t header;
    ham_status_t st;
    char filename[HAM_OS_MAX_PATH];
    ham_log_t *log=(ham_log_t *)ham_mem_calloc(db, sizeof(ham_log_t));
    if (!log) {
        db_set_error(db, HAM_OUT_OF_MEMORY);
        return (0);
    }

    log_set_db(log, db);
    log_set_lsn(log, 1);
    log_set_flags(log, flags);
    log_set_threshold(log, LOG_DEFAULT_THRESHOLD);

    /* create the two files */
    snprintf(filename, sizeof(filename), "%s.log%d", dbpath, 0);
    st=os_create(filename, 0, mode, &log_get_fd(log, 0));
    if (st) {
        ham_mem_free(db, log);
        db_set_error(db, st);
        return (0);
    }

    snprintf(filename, sizeof(filename), "%s.log%d", dbpath, 1);
    st=os_create(filename, 0, mode, &log_get_fd(log, 1));
    if (st) {
        os_close(log_get_fd(log, 0), 0);
        ham_mem_free(db, log);
        db_set_error(db, st);
        return (0);
    }

    /* write the magic to both files */
    memset(&header, 0, sizeof(header));
    log_header_set_magic(&header, HAM_LOG_HEADER_MAGIC);

    for (i=0; i<2; i++) {
        st=os_write(log_get_fd(log, i), &header, sizeof(header));
        if (st) {
            (void)ham_log_close(log);
            db_set_error(db, st);
            return (0);
        }
    }

    return (log);
}

ham_log_t *
ham_log_open(ham_db_t *db, const char *dbpath, ham_u32_t flags)
{
    int i;
    log_header_t header;
    ham_status_t st;
    char filename[HAM_OS_MAX_PATH];
    ham_log_t *log=(ham_log_t *)ham_mem_calloc(db, sizeof(ham_log_t));
    if (!log) {
        db_set_error(db, HAM_OUT_OF_MEMORY);
        return (0);
    }

    log_set_db(log, db);
    log_set_flags(log, flags);

    /* open the two files */
    snprintf(filename, sizeof(filename), "%s.log%d", dbpath, 0);
    st=os_open(filename, 0, &log_get_fd(log, 0));
    if (st) {
        ham_mem_free(db, log);
        db_set_error(db, st);
        return (0);
    }

    snprintf(filename, sizeof(filename), "%s.log%d", dbpath, 1);
    st=os_open(filename, 0, &log_get_fd(log, 1));
    if (st) {
        os_close(log_get_fd(log, 0), 0);
        ham_mem_free(db, log);
        db_set_error(db, st);
        return (0);
    }

    /* check the magic in both files */
    memset(&header, 0, sizeof(header));
    log_header_set_magic(&header, HAM_LOG_HEADER_MAGIC);

    for (i=0; i<2; i++) {
        st=os_pread(log_get_fd(log, i), 0, &header, sizeof(header));
        if (st) {
            (void)ham_log_close(log);
            db_set_error(db, st);
            return (0);
        }
        if (log_header_get_magic(&header)!=HAM_LOG_HEADER_MAGIC) {
            ham_trace(("logfile has unknown magic or is corrupt"));
            (void)ham_log_close(log);
            db_set_error(db, HAM_LOG_INV_FILE_HEADER);
            return (0);
        }
    }

    return (log);
}

ham_status_t
ham_log_is_empty(ham_log_t *log, ham_bool_t *isempty)
{
    ham_status_t st; 
    ham_offset_t size;
    int i;

    for (i=0; i<2; i++) {
        st=os_get_filesize(log_get_fd(log, i), &size);
        if (st)
            return (st);
        if (size && size!=sizeof(log_header_t)) {
            *isempty=HAM_FALSE;
            return (0);
        }
    }

    *isempty=HAM_TRUE;
    return (0);
}

ham_status_t
ham_log_append_entry(ham_log_t *log, int fdidx, log_entry_t *entry, 
        ham_size_t size)
{
    ham_status_t st;
   
    st=os_write(log_get_fd(log, fdidx), entry, size);
    if (st)
        return (db_set_error(log_get_db(log), st));

    st=os_flush(log_get_fd(log, fdidx));
    return (db_set_error(log_get_db(log), st));
}

ham_status_t
ham_log_append_txn_begin(ham_log_t *log, ham_txn_t *txn)
{
    ham_status_t st;
    log_entry_t entry;
    int cur=log_get_current_file(log);
    int other=cur ? 0 : 1;

    memset(&entry, 0, sizeof(entry));
    log_entry_set_lsn(&entry, log_get_lsn(log));
    log_set_lsn(log, log_get_lsn(log)+1);
    log_entry_set_prev_lsn(&entry, txn_get_last_lsn(txn));
    log_entry_set_txn_id(&entry, txn_get_id(txn));
    log_entry_set_type(&entry, LOG_ENTRY_TYPE_TXN_BEGIN);

    /* 
     * determine the log file which is used for this transaction 
     *
     * if the "current" file is not yet full, continue to write to this file
     */
    if (log_get_open_txn(log, cur)+log_get_closed_txn(log, cur)<
            log_get_threshold(log)) {
        txn_set_log_desc(txn, cur);
        log_set_open_txn(log, cur, log_get_open_txn(log, cur)+1);
    }
    /*
     * otherwise, if the other file does no longer have open transactions,
     * insert a checkpoint, delete the other file and use the other file
     * as the current file
     */
    else if (log_get_open_txn(log, other)==0) {
        /* checkpoint! */
        st=my_insert_checkpoint(log);
        if (st)
            return (db_set_error(log_get_db(log), st));
        /* now clear the first file */
        st=my_log_clear_file(log, cur);
        if (st)
            return (db_set_error(log_get_db(log), st));
        /* continue writing to the other file */
        log_set_current_file(log, other);
        cur=other;
        txn_set_log_desc(txn, cur);
        log_set_open_txn(log, cur, log_get_open_txn(log, cur)+1);
    }
    /*
     * otherwise continue writing to the current file, till the other file
     * can be deleted safely
     */
    else {
        txn_set_log_desc(txn, cur);
        log_set_open_txn(log, cur, log_get_open_txn(log, cur)+1);
    }

    st=ham_log_append_entry(log, cur, &entry, sizeof(entry));
    if (st)
        return (db_set_error(log_get_db(log), st));
    txn_set_last_lsn(txn, log_entry_get_lsn(&entry));

    return (0);
}

ham_status_t
ham_log_append_txn_abort(ham_log_t *log, ham_txn_t *txn)
{
    int idx;
    ham_status_t st;
    log_entry_t entry;

    memset(&entry, 0, sizeof(entry));
    log_entry_set_lsn(&entry, log_get_lsn(log));
    log_set_lsn(log, log_get_lsn(log)+1);
    log_entry_set_prev_lsn(&entry, txn_get_last_lsn(txn));
    log_entry_set_txn_id(&entry, txn_get_id(txn));
    log_entry_set_type(&entry, LOG_ENTRY_TYPE_TXN_ABORT);

    /*
     * update the transaction counters of this logfile
     */
    idx=txn_get_log_desc(txn);
    log_set_open_txn(log, idx, log_get_open_txn(log, idx)-1);
    log_set_closed_txn(log, idx, log_get_closed_txn(log, idx)+1);

    st=ham_log_append_entry(log, idx, &entry, sizeof(entry));
    if (st)
        return (db_set_error(log_get_db(log), st));
    txn_set_last_lsn(txn, log_entry_get_lsn(&entry));

    return (0);
}

ham_status_t
ham_log_append_txn_commit(ham_log_t *log, ham_txn_t *txn)
{
    int idx;
    ham_status_t st;
    log_entry_t entry;

    memset(&entry, 0, sizeof(entry));
    log_entry_set_lsn(&entry, log_get_lsn(log));
    log_set_lsn(log, log_get_lsn(log)+1);
    log_entry_set_prev_lsn(&entry, txn_get_last_lsn(txn));
    log_entry_set_txn_id(&entry, txn_get_id(txn));
    log_entry_set_type(&entry, LOG_ENTRY_TYPE_TXN_COMMIT);

    /*
     * update the transaction counters of this logfile
     */
    idx=txn_get_log_desc(txn);
    log_set_open_txn(log, idx, log_get_open_txn(log, idx)-1);
    log_set_closed_txn(log, idx, log_get_closed_txn(log, idx)+1);

    st=ham_log_append_entry(log, idx, &entry, sizeof(entry));
    if (st)
        return (db_set_error(log_get_db(log), st));
    txn_set_last_lsn(txn, log_entry_get_lsn(&entry));

    return (0);
}

ham_status_t
ham_log_append_checkpoint(ham_log_t *log)
{
    ham_status_t st;
    log_entry_t entry;

    memset(&entry, 0, sizeof(entry));
    log_entry_set_lsn(&entry, log_get_lsn(log));
    log_set_lsn(log, log_get_lsn(log)+1);
    log_entry_set_type(&entry, LOG_ENTRY_TYPE_CHECKPOINT);

    /* always write the checkpoint to the newer file */
    /* TODO what if the checkpoint is forced from the user, and there is
     * no "newer" file?? */
    st=ham_log_append_entry(log, 1, &entry, sizeof(entry));
    if (st)
        return (db_set_error(log_get_db(log), st));

    return (0);
}

ham_status_t
ham_log_clear(ham_log_t *log)
{
    ham_status_t st; 
    int i;

    for (i=0; i<2; i++) {
        if ((st=my_log_clear_file(log, i)))
            return (db_set_error(log_get_db(log), st));
    }

    return (0);
}

ham_status_t
ham_log_close(ham_log_t *log)
{
    ham_status_t st; 
    int i;

    st=ham_log_clear(log);
    if (st)
        return (db_set_error(log_get_db(log), st));

    for (i=0; i<2; i++) {
        if (log_get_fd(log, i)!=HAM_INVALID_FD) {
            if ((st=os_close(log_get_fd(log, i), 0)))
                return (db_set_error(log_get_db(log), st));
            log_set_fd(log, i, HAM_INVALID_FD);
        }
    }

    ham_mem_free(log_get_db(log), log);
    return (0);
}
