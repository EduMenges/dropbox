#pragma once

namespace dropbox {

/// Composite interface because C++ don't work well with circular references.
class CompositeInterface {
   public:
    virtual ~CompositeInterface()                                         = default;
    [[nodiscard]] virtual const std::string& GetUsername() const noexcept = 0;

    virtual void Remove(int id)                                                 = 0;
    virtual bool BroadcastUpload(int origin, const std::filesystem::path& path) = 0;
    virtual bool BroadcastDelete(int origin, const std::filesystem::path& path) = 0;
};

}
