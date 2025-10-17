// Copyright(C) 2023 InfiniFlow, Inc. All rights reserved.
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

#pragma once

import infinity_core;
import std.compat;

using namespace infinity;

namespace benchmark {

template <typename T>
std::tuple<size_t, i32, std::unique_ptr<T[]>> DecodeFvecsDataset(const std::filesystem::path &path) {
    auto [file_handle, status] = VirtualStore::Open(path.string(), FileAccessMode::kRead);
    if (!status.ok()) {
        UnrecoverableError(status.message());
    }
    i32 dim = 0;
    file_handle->Read(&dim, sizeof(dim));
    size_t file_size = file_handle->FileSize();
    size_t vec_num = file_size / (dim * sizeof(T) + sizeof(dim));
    auto data = std::make_unique_for_overwrite<T[]>(vec_num * dim);
    for (size_t i = 0; i < vec_num - 1; ++i) {
        file_handle->Read(data.get() + i * dim, dim * sizeof(T));
        i32 dim1 = 0;
        file_handle->Read(&dim1, sizeof(dim1));
        if (dim1 != dim) {
            UnrecoverableError("dim not match");
        }
    }
    file_handle->Read(data.get() + (vec_num - 1) * dim, dim * sizeof(T));
    return {vec_num, dim, std::move(data)};
}


template <typename DataType>
std::unique_ptr<DataType[]> DecodeFvecsById(const std::filesystem::path &path, i32 id) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        UnrecoverableError("Faild to open file");
    }
    i32 dim = 0;
    file.read(reinterpret_cast<char *>(&dim), sizeof(i32));

    size_t vec_size = dim * sizeof(DataType);
    size_t row_size = sizeof(i32) + vec_size;
    file.seekg(id * row_size, std::ios::beg);
    if (!file) {
        UnrecoverableError("Faild to seek");
    }

    i32 vec_dim = 0;
    file.read(reinterpret_cast<char *>(&vec_dim), sizeof(i32));
    if (vec_dim != dim) {
        UnrecoverableError("dim not match");
    }

    auto vec = std::make_unique<DataType[]>(dim);
    file.read(reinterpret_cast<char *>(vec.get()), vec_size);
    if (file.gcount() != static_cast<i64>(vec_size)) {
        UnrecoverableError("Faild to read vec");
    }

    file.close();
    return std::move(vec);
}

} // namespace benchmark
