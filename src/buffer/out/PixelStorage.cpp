// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "precomp.h"
#include "PixelStorage.hpp"

PixelStorage::PixelStorage() noexcept :
    _map{},
    _regions{}
{
}

// Routine Description:
// - fetches the pixel associated with key
// Arguments:
// - key - the key into the storage
// Return Value:
// - the pixel data associated with key
// Note: will throw exception if key is not stored yet
const PixelStorage::mapped_type& PixelStorage::GetData(const key_type key) const
{
    return _map.at(key);
}

// Routine Description:
// - stores pixel data associated with key.
// Arguments:
// - key - the key into the storage
// - glyph - the glyph data to store
void PixelStorage::StoreData(const key_type key, mapped_type data)
{
    _regions.emplace_back(til::size(key));
    _map.insert_or_assign(key, std::move(data));
}

const bool PixelStorage::HasData(const key_type key) const
{
    return _map.find(key) != _map.end();
}

const std::vector<til::size>& PixelStorage::GetAllRegion() const
{
    return _regions;
}

// Routine Description:
// - erases key and its associated data from the storage
// Arguments:
// - key - the key to remove
void PixelStorage::Erase(const key_type key) noexcept
{
    _map.erase(key);
}
