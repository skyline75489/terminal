#pragma once

#include <vector>
#include <unordered_map>


struct PixelRegion final {
    size_t width;
    size_t height;
    std::unique_ptr<std::vector<std::vector<COLORREF>>> data;
    bool exclusive;

    PixelRegion(std::unique_ptr<std::vector<std::vector<COLORREF>>> inData, bool exclusive) :
        data(std::move(inData)),
        exclusive(exclusive)
    {
        auto rawData = data.get();
        if (rawData != nullptr)
        {
            height = rawData->size();
            if (height > 0)
            {
                width = rawData->at(0).size();
            }
        }
    }
};

class PixelStorage final
{
public:

    using key_type = typename COORD;
    using mapped_type = typename std::unique_ptr<PixelRegion>;

    PixelStorage() noexcept;

    const mapped_type& GetData(const key_type key) const;

    void StoreData(const key_type key, mapped_type data);

    const bool HasData(const key_type key) const;

    const std::vector<til::rectangle> &GetAllRegion() const;

    void Erase(const key_type key) noexcept;

private:
    std::unordered_map<key_type, mapped_type> _map;
    std::vector<til::rectangle> _regions;
};
