// Copyright(C) 2025 InfiniFlow, Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

module;

#include <tuple>
#include <vector>

module kv_utility;

import stl;
import kv_code;
import kv_store;
// import status;
import index_base;
import third_party;
import utility;
// import rocksdb_merge_operator;
import logger;
import new_catalog;
import infinity_context;
import buffer_handle;
import buffer_obj;
import block_version;
import buffer_manager;
import infinity_exception;

namespace infinity {

Vector<SegmentID> GetTableSegments(KVInstance *kv_instance, const String &db_id_str, const String &table_id_str, TxnTimeStamp begin_ts) {
    auto start = GetNowTime();
    auto t0 = GetNowTime();
    Vector<SegmentID> segment_ids;
    String segment_id_prefix = KeyEncode::CatalogTableSegmentKeyPrefix(db_id_str, table_id_str);

    t0 = GetNowTime();
    auto iter = kv_instance->GetIterator();
    iter->Seek(segment_id_prefix);
    WriteStatus(t0, "DB::kv_utility::GetTableSegments::Iterator");
    while (iter->Valid() && iter->Key().starts_with(segment_id_prefix)) {
        TxnTimeStamp commit_ts = std::stoull(iter->Value().ToString());
        if (commit_ts > begin_ts and commit_ts != std::numeric_limits<TxnTimeStamp>::max()) {
            LOG_DEBUG(fmt::format("SKIP SEGMENT: {} {} {}", iter->Key().ToString(), commit_ts, begin_ts));
            iter->Next();
            continue;
        }
        // the key is committed before the txn or the key isn't committed
        SegmentID segment_id = std::stoull(iter->Key().ToString().substr(segment_id_prefix.size()));
        segment_ids.push_back(segment_id);
        t0 = GetNowTime();
        iter->Next();
        WriteStatus(t0, "DB::kv_utility::GetTableSegments::Iterator");
    }
    std::sort(segment_ids.begin(), segment_ids.end());

    WriteStatus(start, "DB::kv_utility::GetTableSegments");
    return segment_ids;
}

Vector<SegmentID> GetTableIndexSegments(KVInstance *kv_instance,
                                        const String &db_id_str,
                                        const String &table_id_str,
                                        const String &index_id_str,
                                        TxnTimeStamp begin_ts) {
    auto start = GetNowTime();
    auto t0 = GetNowTime();
    Vector<SegmentID> segment_ids;

    String segment_id_prefix = KeyEncode::CatalogIdxSegmentKeyPrefix(db_id_str, table_id_str, index_id_str);

    t0 = GetNowTime();
    auto iter = kv_instance->GetIterator();
    iter->Seek(segment_id_prefix);
    WriteStatus(t0, "DB::kv_utility::GetTableIndexSegments::Iterator");
    while (iter->Valid() && iter->Key().starts_with(segment_id_prefix)) {
        String key = iter->Key().ToString();
        auto [segment_id, is_segment_id] = ExtractU64FromStringSuffix(key, segment_id_prefix.size());
        if (is_segment_id) {
            TxnTimeStamp commit_ts = std::stoull(iter->Value().ToString());
            if (commit_ts > begin_ts and commit_ts != std::numeric_limits<TxnTimeStamp>::max()) {
                LOG_DEBUG(fmt::format("SKIP SEGMENT INDEX: {} {} {}", iter->Key().ToString(), commit_ts, begin_ts));
                t0 = GetNowTime();
                iter->Next();
                WriteStatus(t0, "DB::kv_utility::GetTableIndexSegments::Iterator");
                continue;
            }
            // the key is committed before the txn or the key isn't committed
            segment_ids.push_back(segment_id);
        } else {
            LOG_DEBUG(fmt::format("Key: {} isn't segment id", iter->Key().ToString()));
        }
        t0 = GetNowTime();
        iter->Next();
        WriteStatus(t0, "DB::kv_utility::GetTableIndexSegments::Iterator");
    }
    std::sort(segment_ids.begin(), segment_ids.end());

    WriteStatus(start, "DB::kv_utility::GetTableIndexSegments");
    return segment_ids;
}

Vector<BlockID> GetTableSegmentBlocks(KVInstance *kv_instance,
                                      const String &db_id_str,
                                      const String &table_id_str,
                                      SegmentID segment_id,
                                      TxnTimeStamp begin_ts,
                                      TxnTimeStamp commit_ts) {
    auto t0 = GetNowTime();
    Vector<BlockID> block_ids;

    String block_id_prefix = KeyEncode::CatalogTableSegmentBlockKeyPrefix(db_id_str, table_id_str, segment_id);
    auto iter = kv_instance->GetIterator();
    iter->Seek(block_id_prefix);
    while (iter->Valid() && iter->Key().starts_with(block_id_prefix)) {
        TxnTimeStamp block_commit_ts = std::stoull(iter->Value().ToString());
        if (block_commit_ts > begin_ts and block_commit_ts != commit_ts and block_commit_ts != std::numeric_limits<TxnTimeStamp>::max()) {
            LOG_DEBUG(fmt::format("SKIP BLOCK: {} {} {}", iter->Key().ToString(), block_commit_ts, begin_ts));
            iter->Next();
            continue;
        }
        // the key is committed before the txn or the key isn't committed
        BlockID block_id = std::stoull(iter->Key().ToString().substr(block_id_prefix.size()));
        block_ids.push_back(block_id);
        iter->Next();
    }

    std::sort(block_ids.begin(), block_ids.end());
    WriteStatus(t0, "DB::kv_utility::GetTableSegmentBlocks");
    return block_ids;
}

Vector<ColumnID> GetTableSegmentBlockColumns(KVInstance *kv_instance,
                                             const String &db_id_str,
                                             const String &table_id_str,
                                             SegmentID segment_id,
                                             BlockID block_id,
                                             TxnTimeStamp begin_ts) {
    auto t0 = GetNowTime();
    Vector<ColumnID> column_ids;
    String block_column_id_prefix = KeyEncode::CatalogTableSegmentBlockColumnKeyPrefix(db_id_str, table_id_str, segment_id, block_id);
    auto iter = kv_instance->GetIterator();
    iter->Seek(block_column_id_prefix);
    while (iter->Valid() && iter->Key().starts_with(block_column_id_prefix)) {
        TxnTimeStamp commit_ts = std::stoull(iter->Value().ToString());
        if (commit_ts > begin_ts) {
            LOG_DEBUG(fmt::format("SKIP BLOCK COLUMN: {} {} {}", iter->Key().ToString(), commit_ts, begin_ts));
            iter->Next();
            continue;
        }
        ColumnID column_id = std::stoull(iter->Key().ToString().substr(block_column_id_prefix.size()));
        column_ids.push_back(column_id);
        iter->Next();
    }
    std::sort(column_ids.begin(), column_ids.end());
    WriteStatus(t0, "DB::kv_utility::GetTableSegmentBlockColumns");
    return column_ids;
}

SharedPtr<IndexBase>
GetTableIndexDef(KVInstance *kv_instance, const String &db_id_str, const String &table_id_str, const String &index_id_str, TxnTimeStamp begin_ts) {
    auto start = GetNowTime();
    auto t0 = GetNowTime();
    String index_def_key = KeyEncode::CatalogIndexTagKey(db_id_str, table_id_str, index_id_str, "index_base");
    String index_def_str;

    t0 = GetNowTime();
    Status status = kv_instance->Get(index_def_key, index_def_str);
    WriteStatus(t0, "DB::kv_utility::GetTableIndexDef::Get");
    if (!status.ok()) {
        LOG_ERROR(fmt::format("Fail to get index definition from kv store, key: {}, cause: {}", index_def_key, status.message()));
        return nullptr;
    }

    t0 = GetNowTime();
    SharedPtr<IndexBase> index_base = IndexBase::Deserialize(index_def_str);
    WriteStatus(t0, "DB::kv_utility::GetTableIndexDef::Deserialize");

    WriteStatus(start, "DB::kv_utility::GetTableIndexDef");
    return index_base;
}

SizeT GetBlockRowCount(KVInstance *kv_instance,
                       const String &db_id_str,
                       const String &table_id_str,
                       SegmentID segment_id,
                       BlockID block_id,
                       TxnTimeStamp begin_ts,
                       TxnTimeStamp commit_ts) {
    auto t0 = GetNowTime();
    NewCatalog *new_catalog = InfinityContext::instance().storage()->new_catalog();
    String block_lock_key = KeyEncode::CatalogTableSegmentBlockTagKey(db_id_str, table_id_str, segment_id, block_id, "lock");

    SharedPtr<BlockLock> block_lock;
    Status status = new_catalog->GetBlockLock(block_lock_key, block_lock);
    if (!status.ok()) {
        UnrecoverableError("Failed to get block lock");
    }

    BufferManager *buffer_mgr = InfinityContext::instance().storage()->buffer_manager();
    String version_filepath = fmt::format("{}/db_{}/tbl_{}/seg_{}/blk_{}/{}",
                                          InfinityContext::instance().config()->DataDir(),
                                          db_id_str,
                                          table_id_str,
                                          segment_id,
                                          block_id,
                                          BlockVersion::PATH);
    BufferObj *version_buffer = buffer_mgr->GetBufferObject(version_filepath);
    if (version_buffer == nullptr) {
        UnrecoverableError(fmt::format("Get version buffer failed: {}", version_filepath));
    }

    BufferHandle buffer_handle = version_buffer->Load();
    const auto *block_version = reinterpret_cast<const BlockVersion *>(buffer_handle.GetData());

    SizeT row_cnt = 0;
    {
        std::shared_lock lock(block_lock->mtx_);
        row_cnt = block_version->GetRowCount(begin_ts);
        auto [offset, commit_cnt] = block_version->GetCommitRowCount(commit_ts);
        row_cnt += commit_cnt;
    }
    WriteStatus(t0, "DB::kv_utility::GetBlockRowCount");
    return row_cnt;
}

SizeT GetSegmentRowCount(KVInstance *kv_instance,
                         const String &db_id_str,
                         const String &table_id_str,
                         SegmentID segment_id,
                         TxnTimeStamp begin_ts,
                         TxnTimeStamp commit_ts) {
    auto t0 = GetNowTime();
    Vector<BlockID> blocks = GetTableSegmentBlocks(kv_instance, db_id_str, table_id_str, segment_id, begin_ts, commit_ts);
    SizeT segment_row_count = 0;
    for (BlockID block_id : blocks) {
        SizeT block_row_cnt = GetBlockRowCount(kv_instance, db_id_str, table_id_str, segment_id, block_id, begin_ts, commit_ts);
        segment_row_count += block_row_cnt;
    }
    WriteStatus(t0, "DB::kv_utility::GetSegmentRowCount");
    return segment_row_count;
}

} // namespace infinity