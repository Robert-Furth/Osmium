#ifndef HANDLEWRAPPER_H
#define HANDLEWRAPPER_H

#include <cstdint>
#include <memory>
#include <vector>

namespace osmium {

class HandleWrapper {
public:
    HandleWrapper(uint32_t handle = 0);

    HandleWrapper(const HandleWrapper&) = delete;
    HandleWrapper& operator=(const HandleWrapper&) = delete;
    HandleWrapper(HandleWrapper&&) noexcept;
    HandleWrapper& operator=(HandleWrapper&&) noexcept;
    ~HandleWrapper();

    uint32_t operator*() const;

    void set_extra_data(int n);
    const std::unique_ptr<int>& extra_data_ptr();
    void set_soundfonts(const std::vector<unsigned long>&);

private:
    uint32_t m_handle;
    std::unique_ptr<int> m_extra_data;
    std::vector<unsigned long> m_soundfont_handles;
};

} // namespace osmium
#endif // HANDLEWRAPPER_H
