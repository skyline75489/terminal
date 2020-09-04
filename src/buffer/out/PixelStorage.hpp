#pragma once

#include <vector>
#include <unordered_map>


struct PixelRegion final {
    til::size size;
    std::unique_ptr<std::vector<std::vector<COLORREF>>> data;
    bool exclusive;

    PixelRegion(std::unique_ptr<std::vector<std::vector<COLORREF>>> inData, bool exclusive) :
        data(std::move(inData)),
        exclusive(exclusive)
    {
        auto rawData = data.get();
        if (rawData != nullptr)
        {
            size_t height = rawData->size();
            size_t width = 0;
            if (height > 0)
            {
                width = rawData->at(0).size();
            }

            size = til::size(width, height);
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

    const std::vector<til::size> &GetAllRegion() const;

    void Erase(const key_type key) noexcept;

private:
    std::unordered_map<key_type, mapped_type> _map;
    std::vector<til::size> _regions;
};
